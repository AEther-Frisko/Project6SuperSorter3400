#include "crow_all.h"
#include <sql.h>
#include <sqlext.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include "Backend/PuzzleLogic.h"

using namespace std;
using namespace crow;

#define PORT_NUMBER 18080
static SQLHENV hEnv = SQL_NULL_HANDLE;
static SQLHDBC hDbc = SQL_NULL_HANDLE;

mutex boardMutex; // Mutex to protect game state in case of concurrent access (Hopefullly stops crashes)
mutex dbMutex;
 std::vector<int> board;
 int moveCount = 0;
 std::stack<int> moveHistory;

/*
    Struct for handling the database data
*/
struct LeaderboardEntry{
    int Id;
    string Name;
    int NumOfMoves;
    string Time;
    int Rank;
};

/*
    Loads in the config file for connecting to the database
    
    No longer used due to .env file!!
*/
unordered_map<string, string> loadConfig(const string& path) {
    unordered_map<string, string> cfg;
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Failed to open config file: " + path);
    }

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == string::npos)
            continue;

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);

        cfg[key] = value;
    }
    return cfg;
}

/* helper to get readable ODBC error */
static std::string odbc_error(SQLSMALLINT handleType, SQLHANDLE handle) {
    SQLCHAR state[6]{};
    SQLCHAR msg[1024]{};
    SQLINTEGER native{};
    SQLSMALLINT len{};
    if (SQLGetDiagRecA(handleType, handle, 1, state, &native, msg, sizeof(msg), &len) == SQL_SUCCESS) {
        return std::string((char*)state) + ": " + (char*)msg;
    }
    return "Unknown ODBC error";
}

/*
    Creates a proper connection string for the Azure database 
*/
string createConnStr(){
    const char* server = getenv("DB_SERVER");
    const char* db = getenv("DB_NAME");
    const char* user = getenv("DB_USER");
    const char* pass = getenv("DB_PASS");

    string connStr =
        "Driver={ODBC Driver 18 for SQL Server};"
        "Server=tcp:" + string(server) + ",1433;"
        "Database=" + string(db) + ";"
        "Uid=" + string(user) + ";"
        "Pwd=" + string(pass) + ";"
        "Encrypt=yes;"
        "TrustServerCertificate=no;"
        "Connection Timeout=30;";
    
    return connStr;
}

void initDB() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, (SQLHANDLE*)&hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, (SQLHANDLE)hEnv, (SQLHANDLE*)&hDbc);

    string connStr = createConnStr();
    SQLRETURN ret = SQLDriverConnectA(hDbc, NULL, (SQLCHAR*)connStr.c_str(),
        SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);

    if (!SQL_SUCCEEDED(ret)) {
        throw runtime_error("DB init failed: " + odbc_error(SQL_HANDLE_DBC, hDbc));
    }
}

/*
    Retrieves database data
*/
vector<LeaderboardEntry> getLeaderboard() {
    std::lock_guard<std::mutex> lock(dbMutex);
    vector<LeaderboardEntry> entries;
    SQLHSTMT hStmt;

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret))
        throw runtime_error("Failed to allocate SQL statement");

    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT ID, Name, NumOfMoves, Time FROM dbo.PlayerRanks ORDER BY NumOfMoves ASC, Time ASC", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        throw runtime_error("Failed to execute SELECT on database");
    }

    SQLINTEGER id, moves;
    SQLCHAR time[9];
    SQLCHAR name[4];

    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_SLONG, &id, 0, NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 3, SQL_C_SLONG, &moves, 0, NULL);
        SQLGetData(hStmt, 4, SQL_C_CHAR, time, sizeof(time), NULL);
        entries.push_back({ id, string((char*)name), moves, string((char*)time) });
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return entries;
}

/*
    Renders an html page using the base template + page-specific content
*/
response renderPage(const string& title, const string& content_file){
    auto base = crow::mustache::load("base.html");
    auto content = crow::mustache::load_text(content_file);

    crow::mustache::context ctx;
    ctx["title"] = title;
    ctx["content"] = content;

    return base.render(ctx);
}

/*
    Helper for making the standard html page routes
*/
auto makePageRoute = [](const string& title, const string& file){
    return [title, file](){
        try {
            auto page = renderPage(title, file);
            return crow::response(200, page.body);
        } catch(...){
            return crow::response(500, "Failed to load page");
        }
    };
};

int main(){

    try {
        initDB();
    }
    catch (const runtime_error& e) {
        std::cerr << "Failed to connect to database: " << e.what() << std::endl;
        return 1;
    }
    crow::SimpleApp app;
    // Game state (in-memory)
    

    // GET Home page
    CROW_ROUTE(app, "/")(makePageRoute("Home", "home.html"));

    // GET Leaderboard page
    CROW_ROUTE(app, "/leaderboard")(makePageRoute("Leaderboard", "leaderboard.html"));

    // GET Game page
    CROW_ROUTE(app, "/game")(makePageRoute("Play Game", "game.html"));

    // NEW GAME THROUGH NEWGAME Button
    CROW_ROUTE(app, "/api/new").methods("DELETE"_method)([](const crow::request& req) {
		lock_guard<mutex> lock(boardMutex); // Ensure thread safety when modifying game state
        
        board = createShuffledBoard();
        moveCount = 0;
        invalidateCache();

        crow::json::wvalue out;
        out["board"] = board;
        out["moves"] = moveCount;
        out["solved"] = isSolved(board);
        out["validMoves"] = getValidMoves(board);
        return crow::response(200, out);
    });

    CROW_ROUTE(app, "/api/startgame")([]() {
		lock_guard<mutex> lock(boardMutex); // Ensure thread safety when modifying game state
        board = createShuffledBoard();
        moveCount = 0;
        invalidateCache();

        crow::json::wvalue out;
        out["board"] = board;
        out["moves"] = moveCount;
        out["solved"] = isSolved(board);
        out["validMoves"] = getValidMoves(board);
        return crow::response(200, out);
        });

    // MAKE MOVE
    CROW_ROUTE(app, "/api/move").methods("PATCH"_method)
    ([](const crow::request& req){
		lock_guard<mutex> lock(boardMutex); // Ensure thread safety when modifying game state
        auto body = crow::json::load(req.body);
        if (!body || !body.has("pos")) {
            return crow::response(400, "Missing pos");
        }

        int pos = body["pos"].i();

        // If game wasn't started yet, start one
        if (board.empty()) {
            board = createShuffledBoard();
            moveCount = 0;
        }

        bool moved = makeMove(board, pos);
        if (moved) {
            moveCount++;
            invalidateCache();
        }

        crow::json::wvalue out;
        out["moved"] = moved;
        out["board"] = board;
        out["moves"] = moveCount;
        out["solved"] = isSolved(board);
        out["validMoves"] = getValidMoves(board);
        return crow::response(200, out);
    });

    // UNDO MOVE
        
    CROW_ROUTE(app, "/api/undo").methods("PUT"_method)
        ([](const crow::request& req) {
		lock_guard<mutex> lock(boardMutex); // Ensure thread safety when modifying game state
        bool undone = undoMove(board);
        if (undone && moveCount > 0) {
            moveCount--;
            invalidateCache();
        }

        crow::json::wvalue out;
        out["undone"] = undone;
        out["board"] = board;
        out["moves"] = moveCount;
        out["solved"] = isSolved(board);
        out["validMoves"] = getValidMoves(board);
        return crow::response(200, out);
            });

    
    // Options route 
    //CROW_ROUTE(app, "/api/move").methods("OPTIONS"_method)([](const crow::request& req) {
        //crow::response res;
        //req.add_header("Access-Control-Allow-Methods", "PATCH, OPTIONS");
        //req.add_header("Access-Control-Allow-Headers", "Content-Type");
        //return res;
		//});
    // GET Leaderboard entries from database
    CROW_ROUTE(app, "/api/leaderboard")([](){
        try {
            crow::json::wvalue result;
            vector<LeaderboardEntry> entries = getLeaderboard();

            if (entries.empty()) {
                return crow::response(204);
            }

            int rank = 1;
            for (auto &e : entries) {
                string key = to_string(rank);
                result[key]["Id"] = e.Id;
                result[key]["Name"] = e.Name;
                result[key]["NumOfMoves"] = e.NumOfMoves;
                result[key]["Time"] = e.Time;
                result[key]["Rank"] = rank++;
            }

            return crow::response(200, result);
        }
        catch (const runtime_error &e){
            return crow::response(500, string("Runtime error: ") + e.what());
        }
        catch (const exception& ex){
            return crow::response(500, string("Database error: ") + ex.what());
        }
        catch (...){
            return crow::response(500, "Unknown server error while fetching leaderboard");
        }
    });

    CROW_ROUTE(app, "/api/hint").methods("GET"_method)
        ([](const crow::request& req) {
		lock_guard<mutex> lock(boardMutex); // Ensure thread safety when accessing game state
        crow::response res(200);
        res.set_header("Content-Type", "application/json");
        crow::json::wvalue out;

        if (board.empty()) {
            board = createShuffledBoard();
            moveCount = 0;
        }

        vector<int> moves = getSolution();
        if (moves.empty()) {
            out["message"] = "No solution found";
        }
        else {
            out["hint"] = moves[0];
        }
        res.body = out.dump();
        return res;
            });

    //Post game stats to the db
    CROW_ROUTE(app, "/api/submit").methods("POST"_method)
        ([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("name") || !body.has("moves") || !body.has("timeSeconds"))
                return crow::response(400, "Missing fields");

            std::string name = body["name"].s();
            int moves = body["moves"].i();
            int timeSeconds = body["timeSeconds"].i();

            std::string clean;
            for (char c : name) {
                if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
                if (c >= 'A' && c <= 'Z') clean.push_back(c);
                if (clean.size() == 3) break;
            }
            if (clean.size() != 3) return crow::response(400, "Name must be 3 letters");

            if (moves < 0) moves = 0;
            if (timeSeconds < 0) timeSeconds = 0;

            int hh = (timeSeconds / 3600) % 24;
            int mm = (timeSeconds / 60) % 60;
            int ss = timeSeconds % 60;
            char timeBuf[9];
            sprintf(timeBuf, "%02d:%02d:%02d", hh, mm, ss);

            // Just allocate a statement on the existing connection
            std::lock_guard<std::mutex> lock(dbMutex);
            SQLHSTMT hStmt;
            SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
            if (!SQL_SUCCEEDED(ret))
                return crow::response(500, "STMT alloc failed: " + odbc_error(SQL_HANDLE_DBC, hDbc));

            std::string sql =
                "INSERT INTO dbo.PlayerRanks (Name, NumOfMoves, Time) VALUES ('" +
                clean + "', " + std::to_string(moves) + ", '" + timeBuf + "')";

            ret = SQLExecDirectA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

            if (!SQL_SUCCEEDED(ret))
                return crow::response(500, "INSERT failed");

            return crow::response(200, "OK");
        }
        catch (...) {
            return crow::response(500, "Server error");
        }
            });

    app.port(PORT_NUMBER).multithreaded().run();
    return 0;
}
