#include "crow_all.h"
#include <sql.h>
#include <sqlext.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include "Backend/PuzzleLogic.h"

using namespace std;
using namespace crow;

#define PORT_NUMBER 18080

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
    Retrieves database data
*/
vector<LeaderboardEntry> getLeaderboard() {
    vector<LeaderboardEntry> entries;

    SQLHENV hEnv;
    SQLRETURN ret;
    SQLHDBC hDbc;
    SQLHSTMT hStmt;

    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, (SQLHANDLE*)&hEnv);
    if (!SQL_SUCCEEDED(ret)){
        throw runtime_error("Failed to allocate ODBC environment");
    }

    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    
    ret = SQLAllocHandle(SQL_HANDLE_DBC, (SQLHANDLE)hEnv, (SQLHANDLE*)&hDbc);
    if (!SQL_SUCCEEDED(ret)){
        throw runtime_error("Failed to allocate ODBC connection");
    }

    auto cfg = loadConfig("db.conf");

    std::string connStr =
        "Driver={" + cfg["Driver"] + "};" +
        "Server=" + cfg["Server"] + ";" +
        "Database=" + cfg["Database"] + ";" +
        "Uid=" + cfg["User"] + ";" +
        "Pwd=" + cfg["Password"] + ";" +
        "Encrypt=" + cfg["Encrypt"] + ";" +
        "TrustServerCertificate=" + cfg["TrustServerCertificate"] + ";" +
        "Connection Timeout=" + cfg["Timeout"] + ";";
    
    const SQLCHAR* sqlConnStr = reinterpret_cast<const SQLCHAR*>(connStr.c_str());

    ret = SQLDriverConnect(
        hDbc,
        NULL,
        (SQLCHAR*)sqlConnStr,
        SQL_NTS,
        NULL,
        0,
        NULL,
        SQL_DRIVER_COMPLETE
    );

    if (!SQL_SUCCEEDED(ret)){
        SQLCHAR sqlState[6], message[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_DBC, hDbc, 1, sqlState, &nativeError, message, sizeof(message), &textLength);

        string err = "ODBC connection failed: ";
        err += (char*)message;

        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

        throw runtime_error(err);
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)){
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        throw runtime_error("Failed to allocate SQL statement");
    }
    
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT ID, Name, NumOfMoves, Time FROM dbo.PlayerRanks ORDER BY NumOfMoves ASC, Time ASC", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        throw runtime_error("Failed to execute SELECT on database");
    }

    SQLINTEGER id, moves;
    SQLCHAR time[9];
    SQLCHAR name[50];

    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_SLONG, &id, 0, NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 3, SQL_C_SLONG, &moves, 0, NULL);
        SQLGetData(hStmt, 4, SQL_C_CHAR, time, sizeof(time), NULL);

        entries.push_back({id, string((char*)name), moves, string((char*)time)});
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

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
    crow::SimpleApp app;
    // Game state (in-memory)
    static std::vector<int> board;
    static int moveCount = 0;

    // GET Home page
    CROW_ROUTE(app, "/")(makePageRoute("Home", "home.html"));

    // GET Leaderboard page
    CROW_ROUTE(app, "/leaderboard")(makePageRoute("Leaderboard", "leaderboard.html"));

    // GET Game page
    CROW_ROUTE(app, "/game")(makePageRoute("Play Game", "game.html"));

    // NEW GAME
    CROW_ROUTE(app, "/api/new")([](){
        board = createShuffledBoard();
        moveCount = 0;

        crow::json::wvalue out;
        out["board"] = board;
        out["moves"] = moveCount;
        out["solved"] = isSolved(board);
        out["validMoves"] = getValidMoves(board);
        return crow::response(200, out);
    });

    // MAKE MOVE
    CROW_ROUTE(app, "/api/move").methods("POST"_method)
    ([](const crow::request& req){
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
        if (moved) moveCount++;

        crow::json::wvalue out;
        out["moved"] = moved;
        out["board"] = board;
        out["moves"] = moveCount;
        out["solved"] = isSolved(board);
        out["validMoves"] = getValidMoves(board);
        return crow::response(200, out);
    });


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

    //Post game stats to the db
    CROW_ROUTE(app, "/api/submit").methods("POST"_method)
    ([](const crow::request& req){
        try {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("name") || !body.has("moves") || !body.has("timeSeconds"))
                return crow::response(400, "Missing fields");

            std::string name = body["name"].s();
            int moves = body["moves"].i();
            int timeSeconds = body["timeSeconds"].i();

            // Force ABC format (prevents SQL injection too)
            std::string clean;
            for (char c : name) {
                if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
                if (c >= 'A' && c <= 'Z') clean.push_back(c);
                if (clean.size() == 3) break;
            }
            if (clean.size() != 3) return crow::response(400, "Name must be 3 letters");

            if (moves < 0) moves = 0;
            if (timeSeconds < 0) timeSeconds = 0;

            // Convert seconds to SQL TIME string "HH:MM:SS"
            int hh = (timeSeconds / 3600) % 24;
            int mm = (timeSeconds / 60) % 60;
            int ss = timeSeconds % 60;

            char timeBuf[9];
            sprintf_s(timeBuf, "%02d:%02d:%02d", hh, mm, ss);

            // Connect exactly like your getLeaderboard() does:
            SQLHENV hEnv; SQLHDBC hDbc; SQLHSTMT hStmt; SQLRETURN ret;

            ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, (SQLHANDLE*)&hEnv);
            SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
            ret = SQLAllocHandle(SQL_HANDLE_DBC, (SQLHANDLE)hEnv, (SQLHANDLE*)&hDbc);

            auto cfg = loadConfig("db.conf");
            std::string connStr =
                "Driver={" + cfg["Driver"] + "};" +
                "Server=" + cfg["Server"] + ";" +
                "Database=" + cfg["Database"] + ";" +
                "Uid=" + cfg["User"] + ";" +
                "Pwd=" + cfg["Password"] + ";" +
                "Encrypt=" + cfg["Encrypt"] + ";" +
                "TrustServerCertificate=" + cfg["TrustServerCertificate"] + ";" +
                "Connection Timeout=" + cfg["Timeout"] + ";";

            ret = SQLDriverConnectA(hDbc, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
            if (!SQL_SUCCEEDED(ret)) {
                std::string err = odbc_error(SQL_HANDLE_DBC, hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
                SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
                return crow::response(500, "DB connect failed: " + err);
            }

            ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
            if (!SQL_SUCCEEDED(ret)) {
                std::string err = odbc_error(SQL_HANDLE_DBC, hDbc);
                SQLDisconnect(hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
                SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
                return crow::response(500, "STMT alloc failed: " + err);
            }

            
            std::string sql =
                "INSERT INTO dbo.PlayerRanks (Rank, Name, NumOfMoves, Time) VALUES (0, N'" +
                clean + "', " + std::to_string(moves) + ", '" + timeBuf + "')";

            ret = SQLExecDirectA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);

            if (!SQL_SUCCEEDED(ret)) {
                std::string err = odbc_error(SQL_HANDLE_STMT, hStmt);
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                SQLDisconnect(hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
                SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
                return crow::response(500, "INSERT failed: " + err);
            }

            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            SQLDisconnect(hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

            return crow::response(200, "OK");
        }
        catch (...) {
            return crow::response(500, "Server error");
        }
    });

    app.port(PORT_NUMBER).multithreaded().run();
    return 0;
}
