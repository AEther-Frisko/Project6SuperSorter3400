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
    int ID;
    string Name;
    int NumOfMoves;
    int TimeInSeconds;
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
    
    ret = SQLExecDirect(hStmt, (SQLCHAR*)"SELECT ID, Name, NumOfMoves, TimeInSeconds FROM Leaderboard ORDER BY NumOfMoves ASC, TimeInSeconds ASC", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        throw runtime_error("Failed to execute SELECT on Leaderboard");
    }

    SQLINTEGER id, moves, time;
    SQLCHAR name[50];

    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_SLONG, &id, 0, NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 3, SQL_C_SLONG, &moves, 0, NULL);
        SQLGetData(hStmt, 4, SQL_C_SLONG, &time, 0, NULL);

        entries.push_back({id, string((char*)name), moves, time});
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

    // GET Home page
    CROW_ROUTE(app, "/")(makePageRoute("Home", "home.html"));

    // GET Leaderboard page
    CROW_ROUTE(app, "/leaderboard")(makePageRoute("Leaderboard", "leaderboard.html"));

    // GET Game page
    CROW_ROUTE(app, "/game")(makePageRoute("Play Game", "game.html"));

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
                result[key]["ID"] = e.ID;
                result[key]["Name"] = e.Name;
                result[key]["NumOfMoves"] = e.NumOfMoves;
                result[key]["TimeInSeconds"] = e.TimeInSeconds;
                result[key]["Rank"] = rank++;
            }

            return crow::response(200, result);
        }
        catch (const exception& ex){
            return crow::response(500, string("Database error: ") + ex.what());
        }
        catch (...){
            return crow::response(500, "Unknown server error while fetching leaderboard");
        }
    });

    app.port(PORT_NUMBER).multithreaded().run();
    return 0;
}