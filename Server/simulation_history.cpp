
#include "simulation_history.h"
#include "auth.h"
#include "database.h"
#include <string>
#include <iostream>
#include <cctype>
#include <ctime>
#include <nlohmann/json.hpp>
#include "sqlite/sqlite3.h"

using namespace std;
using json = nlohmann::json;
bool isValidHistorySessionToken(const string& token)
{
    if (token.length() != 64)
        return false;

    for (unsigned char ch : token)
    {
        if (!isxdigit(ch))
            return false;
    }

    return true;
}
void registerSimulationHistoryRoutes(httplib::Server& server)
{
    // Save simulation history for the logged-in user
    server.Post("/simulations/history",
        [](const httplib::Request& req, httplib::Response& res)
        {
            string authHeader = req.get_header_value("Authorization");

            if (authHeader.length() <= 7 ||
                authHeader.substr(0, 7) != "Bearer ")
            {
                res.status = 401;
                res.set_content(
                    R"({"error":"Missing or invalid authorization token."})",
                    "application/json");
                return;
            }

            string token = authHeader.substr(7);

if (!isValidHistorySessionToken(token))
{
    res.status = 401;
    res.set_content(
        R"({"error":"Invalid session token."})",
        "application/json"
    );
    return;
}

string tokenHash = hashSessionToken(token);
            json data;

            try
            {
                data = json::parse(req.body);
            }
            catch (...)
            {
                res.status = 400;
                res.set_content(R"({"error":"Invalid JSON."})", "application/json");
                return;
            }

            if (!data.is_object() ||
                !data.contains("simulation_name") ||
                !data.contains("grid_rows") ||
                !data.contains("grid_cols") ||
                !data.contains("robot_count") ||
                !data.contains("result") ||
                !data["simulation_name"].is_string() ||
                !data["grid_rows"].is_number_integer() ||
                !data["grid_cols"].is_number_integer() ||
                !data["robot_count"].is_number_integer() ||
                !data["result"].is_string())
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid or missing simulation fields."})",
                    "application/json");
                return;
            }

            string simulationName = data["simulation_name"].get<string>();
            string simulationResult = data["result"].get<string>();
            int rows = data["grid_rows"].get<int>();
            int cols = data["grid_cols"].get<int>();
            int robotCount = data["robot_count"].get<int>();

           if (simulationName.find_first_not_of(" \t\n\r") == string::npos ||
                simulationName.length() > 100 ||
                simulationResult.empty() ||
                simulationResult.length() > 10000 ||
                rows < 2 || rows > 100 ||
                cols < 2 || cols > 100 ||
                robotCount < 1 || robotCount > 20)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Simulation values are out of range."})",
                    "application/json");
                return;
            }

            sqlite3* db = NULL;

            if (!openDatabase(&db))
            {
                if (db != NULL) sqlite3_close(db);
                res.status = 500;
                res.set_content(
                    R"({"error":"Database connection failed."})",
                    "application/json");
                return;
            }
            DatabaseGuard dbGuard(db);

            const char* sessionSql =
                "SELECT user_id FROM sessions "
                "WHERE id = ? AND expires_at > ?;";

            sqlite3_stmt* statement = NULL;
            int result = sqlite3_prepare_v2(
                db, sessionSql, -1, &statement, NULL);

            if (result == SQLITE_OK)
            {
                sqlite3_bind_text(statement, 1, tokenHash.c_str(),
                                  -1, SQLITE_TRANSIENT);
                sqlite3_bind_int64(statement, 2,
                    static_cast<sqlite3_int64>(time(NULL)));
                result = sqlite3_step(statement);
            }

            if (result != SQLITE_ROW)
{
    if (result != SQLITE_DONE)
    {
        cerr << "History session lookup failed: "
             << sqlite3_errmsg(db) << endl;
    }

    if (statement != NULL)
        sqlite3_finalize(statement);

    if (result == SQLITE_DONE)
    {
        res.status = 401;
        res.set_content(
            R"({"error":"Invalid or expired session."})",
            "application/json"
        );
    }
    else
    {
        res.status = 500;
        res.set_content(
            R"({"error":"Session lookup failed."})",
            "application/json"
        );
    }

    return;
}

            sqlite3_int64 userId = sqlite3_column_int64(statement, 0);
            sqlite3_finalize(statement);

            const char* insertSql =
                "INSERT INTO simulation_history "
                "(user_id, simulation_name, grid_rows, grid_cols, "
                "robot_count, result) VALUES (?, ?, ?, ?, ?, ?);";

            
result = sqlite3_prepare_v2(
    db, insertSql, -1, &statement, NULL
);

if (result == SQLITE_OK)
{
    result = sqlite3_bind_int64(statement, 1, userId);

    if (result == SQLITE_OK)
        result = sqlite3_bind_text(
            statement, 2, simulationName.c_str(),
            -1, SQLITE_TRANSIENT
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 3, rows);

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 4, cols);

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 5, robotCount);

    if (result == SQLITE_OK)
        result = sqlite3_bind_text(
            statement, 6, simulationResult.c_str(),
            -1, SQLITE_TRANSIENT
        );

    if (result == SQLITE_OK)
        result = sqlite3_step(statement);
}

sqlite3_int64 historyId = 0;

if (result == SQLITE_DONE)
    historyId = sqlite3_last_insert_rowid(db);

sqlite3_finalize(statement);

if (result != SQLITE_DONE)
{
    cerr << "Failed to save simulation history: "
         << sqlite3_errmsg(db) << endl;

    res.status = 500;
    res.set_content(
        R"({"error":"Could not save simulation history."})",
        "application/json"
    );
    return;
}

            json responseData = {
                {"message", "Simulation history saved."},
                {"id", historyId}
            };

            res.status = 201;
            res.set_content(responseData.dump(), "application/json");
        }
    );

    // Retrieve simulation history for the logged-in user
    server.Get("/simulations/history",
        [](const httplib::Request& req, httplib::Response& res)
        {
            string authHeader = req.get_header_value("Authorization");

            if (authHeader.length() <= 7 ||
                authHeader.substr(0, 7) != "Bearer ")
            {
                res.status = 401;
                res.set_content(
                    R"({"error":"Missing or invalid authorization token."})",
                    "application/json");
                return;
            }

           string token = authHeader.substr(7);

if (!isValidHistorySessionToken(token))
{
    res.status = 401;
    res.set_content(
        R"({"error":"Invalid session token."})",
        "application/json"
    );
    return;
}

string tokenHash = hashSessionToken(token);
            sqlite3* db = NULL;

            if (!openDatabase(&db))
            {
                if (db != NULL) sqlite3_close(db);
                res.status = 500;
                res.set_content(
                    R"({"error":"Database connection failed."})",
                    "application/json");
                return;
            }
            DatabaseGuard dbGuard(db);

            const char* sessionSql =
                "SELECT user_id FROM sessions "
                "WHERE id = ? AND expires_at > ?;";

            sqlite3_stmt* statement = NULL;

int result = sqlite3_prepare_v2(
    db, sessionSql, -1, &statement, NULL
);

if (result == SQLITE_OK)
{
    result = sqlite3_bind_text(
        statement, 1, tokenHash.c_str(),
        -1, SQLITE_TRANSIENT
    );

    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_int64(
            statement, 2,
            static_cast<sqlite3_int64>(time(NULL))
        );
    }

    if (result == SQLITE_OK)
        result = sqlite3_step(statement);
}


            if (result != SQLITE_ROW)
{
    if (result != SQLITE_DONE)
    {
        cerr << "History session lookup failed: "
             << sqlite3_errmsg(db) << endl;
    }

    if (statement != NULL)
        sqlite3_finalize(statement);

    if (result == SQLITE_DONE)
    {
        res.status = 401;
        res.set_content(
            R"({"error":"Invalid or expired session."})",
            "application/json"
        );
    }
    else
    {
        res.status = 500;
        res.set_content(
            R"({"error":"Session lookup failed."})",
            "application/json"
        );
    }

    return;
}

            sqlite3_int64 userId = sqlite3_column_int64(statement, 0);
            sqlite3_finalize(statement);

           
const char* historySql =
    "SELECT id, simulation_name, grid_rows, grid_cols, "
    "robot_count, result, created_at "
    "FROM simulation_history "
    "WHERE user_id = ? ORDER BY id DESC;";

result = sqlite3_prepare_v2(
    db, historySql, -1, &statement, NULL
);

if (result != SQLITE_OK)
{
    cerr << "History query preparation failed: "
         << sqlite3_errmsg(db) << endl;

    res.status = 500;
    res.set_content(
        R"({"error":"Could not retrieve simulation history."})",
        "application/json"
    );
    return;
}

result = sqlite3_bind_int64(statement, 1, userId);

if (result != SQLITE_OK)
{
    cerr << "History query binding failed: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    res.status = 500;
    res.set_content(
        R"({"error":"Could not retrieve simulation history."})",
        "application/json"
    );
    return;
}

json history = json::array();

while ((result = sqlite3_step(statement)) == SQLITE_ROW)
{
    const unsigned char* name =
        sqlite3_column_text(statement, 1);

    const unsigned char* simulationResult =
        sqlite3_column_text(statement, 5);

    const unsigned char* createdAt =
        sqlite3_column_text(statement, 6);

    history.push_back({
        {"id", sqlite3_column_int64(statement, 0)},
        {"simulation_name", name != NULL
            ? reinterpret_cast<const char*>(name) : ""},
        {"grid_rows", sqlite3_column_int(statement, 2)},
        {"grid_cols", sqlite3_column_int(statement, 3)},
        {"robot_count", sqlite3_column_int(statement, 4)},
        {"result", simulationResult != NULL
            ? reinterpret_cast<const char*>(simulationResult) : ""},
        {"created_at", createdAt != NULL
            ? reinterpret_cast<const char*>(createdAt) : ""}
    });
}

sqlite3_finalize(statement);

if (result != SQLITE_DONE)
{
    cerr << "History retrieval failed: "
         << sqlite3_errmsg(db) << endl;

    res.status = 500;
    res.set_content(
        R"({"error":"Failed while reading simulation history."})",
        "application/json"
    );
    return;
}

res.status = 200;
res.set_content(history.dump(4), "application/json");

        }
    );
}

