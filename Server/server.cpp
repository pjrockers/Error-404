
#include <iostream>
#include <string>
#include <ctime>
#include <cmath>
#include <stdexcept>
#include "auth.h"
#include "database.h"
#include "simulation_history.h"
#include "cpp-httplib/httplib.h"
#include "sqlite/sqlite3.h"
#include <sodium.h>
#include "robot_templates.h"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;
string hashSessionToken(const string& token)
{
    unsigned char tokenHash[crypto_generichash_BYTES];

    crypto_generichash(
        tokenHash, sizeof(tokenHash),
        reinterpret_cast<const unsigned char*>(token.data()),
        token.size(), NULL, 0
    );

    char tokenHashHex[crypto_generichash_BYTES * 2 + 1];

    sodium_bin2hex(
        tokenHashHex, sizeof(tokenHashHex),
        tokenHash, sizeof(tokenHash)
    );

    return string(tokenHashHex);
}
int main()
{
    // Initialize password-hashing library
    if (sodium_init() < 0)
    {
        cerr << "Failed to initialize libsodium." << endl;
        return 1;
    }
    if (!initializeDatabase())
{
    cerr << "Database initialization failed. Server stopped." << endl;
    return 1;
}

    httplib::Server server;
    registerAuthRoutes(server);
    registerRobotTemplateRoutes(server);
    registerSimulationHistoryRoutes(server);
    // Home endpoint
    server.Get("/",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            (void)req;

            res.set_content(
                "Robot Swarm Simulator server is running!",
                "text/plain"
            );
        }
    );

    // Health endpoint
    server.Get("/health",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            (void)req;

            sqlite3* db = NULL;
            int result;
            if (!openDatabase(&db))
            {
                res.status = 500;
                res.set_content(
                    "Database connection failed.",
                    "text/plain"
                );

                if (db != NULL)
                    sqlite3_close(db);

                return;
            }

            const char* sql =
                "SELECT COUNT(*) FROM sqlite_master "
                "WHERE type = 'table' AND name IN "
                "('users', 'robot_templates', 'simulation_history');";

            sqlite3_stmt* statement = NULL;

            result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
            );

            if (result == SQLITE_OK &&
                sqlite3_step(statement) == SQLITE_ROW)
            {
                int tableCount =
                    sqlite3_column_int(statement, 0);

                if (tableCount == 3)
                {
                    res.set_content(
                        "Server running. SQLite connected. "
                        "All 3 tables found!",
                        "text/plain"
                    );
                }
                else
                {
                    res.status = 500;
                    res.set_content(
                        "One or more database tables are missing.",
                        "text/plain"
                    );
                }
            }
            else
            {
                res.status = 500;
                res.set_content(
                    "Failed to check database tables.",
                    "text/plain"
                );
            }

            sqlite3_finalize(statement);
            sqlite3_close(db);
        }
    );    
    // Update a robot template owned by the logged-in user
    server.Put(R"(/robots/templates/(\d+))",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            string authHeader =
                req.get_header_value("Authorization");

            if (authHeader.length() <= 7 ||
                authHeader.substr(0, 7) != "Bearer ")
            {
                res.status = 401;
                res.set_content(
                    R"({"error":"Missing or invalid authorization token."})",
                    "application/json"
                );
                return;
            }

            string token = authHeader.substr(7);
            string tokenHash = hashSessionToken(token);

            sqlite3_int64 templateId = 0;

            try
            {
                templateId = stoll(req.matches[1].str());
            }
            catch (...)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid template ID."})",
                    "application/json"
                );
                return;
            }

            if (templateId <= 0)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid template ID."})",
                    "application/json"
                );
                return;
            }

            json data;

            try
            {
                data = json::parse(req.body);
            }
            catch (...)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid JSON."})",
                    "application/json"
                );
                return;
            }

            if (!data.is_object() ||
                !data.contains("template_name") ||
                !data.contains("robot_type") ||
                !data.contains("battery") ||
                !data.contains("speed") ||
                !data.contains("priority") ||
                !data.contains("attributes") ||
                !data["template_name"].is_string() ||
                !data["robot_type"].is_string() ||
                !data["battery"].is_number() ||
                !data["speed"].is_number() ||
                !data["priority"].is_number_integer() ||
                !data["attributes"].is_object())
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid or missing template fields."})",
                    "application/json"
                );
                return;
            }

            string templateName =
                data["template_name"].get<string>();

            string robotType =
                data["robot_type"].get<string>();

            double battery = data["battery"].get<double>();
            double speed = data["speed"].get<double>();
            int priority = data["priority"].get<int>();

            if (templateName.empty() ||
                templateName.length() > 50 ||
                robotType.empty() ||
                robotType.length() > 50 ||
                !isfinite(battery) ||
                !isfinite(speed) ||
                battery < 0 || battery > 100 ||
                speed <= 0 || speed > 100 ||
                priority < 1 || priority > 4)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Template values are out of range."})",
                    "application/json"
                );
                return;
            }

            string attributes = data["attributes"].dump();

            sqlite3* db = NULL;

            if (!openDatabase(&db))
            {
                if (db != NULL)
                    sqlite3_close(db);

                res.status = 500;
                res.set_content(
                    R"({"error":"Database connection failed."})",
                    "application/json"
                );
                return;
            }

            // Validate session and retrieve user ID
            const char* sessionSql =
                "SELECT user_id FROM sessions "
                "WHERE id = ? AND expires_at > ?;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sessionSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_text(
                    statement, 1, tokenHash.c_str(),
                    -1, SQLITE_TRANSIENT
                );

                sqlite3_bind_int64(
                    statement, 2,
                    static_cast<sqlite3_int64>(time(NULL))
                );

                result = sqlite3_step(statement);
            }

            if (result != SQLITE_ROW)
            {
                sqlite3_finalize(statement);
                sqlite3_close(db);

                res.status = (result == SQLITE_DONE) ? 401 : 500;
                res.set_content(
                    result == SQLITE_DONE
                        ? R"({"error":"Invalid or expired session."})"
                        : R"({"error":"Session lookup failed."})",
                    "application/json"
                );
                return;
            }

            sqlite3_int64 userId =
                sqlite3_column_int64(statement, 0);

            sqlite3_finalize(statement);

            // Update only if the template belongs to this user
            const char* updateSql =
                "UPDATE robot_templates SET "
                "template_name = ?, robot_type = ?, battery = ?, "
                "speed = ?, priority = ?, attributes = ? "
                "WHERE id = ? AND user_id = ?;";

            result = sqlite3_prepare_v2(
                db, updateSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_text(
                    statement, 1, templateName.c_str(),
                    -1, SQLITE_TRANSIENT
                );

                sqlite3_bind_text(
                    statement, 2, robotType.c_str(),
                    -1, SQLITE_TRANSIENT
                );

                sqlite3_bind_double(statement, 3, battery);
                sqlite3_bind_double(statement, 4, speed);
                sqlite3_bind_int(statement, 5, priority);

                sqlite3_bind_text(
                    statement, 6, attributes.c_str(),
                    -1, SQLITE_TRANSIENT
                );

                sqlite3_bind_int64(statement, 7, templateId);
                sqlite3_bind_int64(statement, 8, userId);

                result = sqlite3_step(statement);
            }

            int changed = sqlite3_changes(db);

            sqlite3_finalize(statement);
            sqlite3_close(db);

            if (result != SQLITE_DONE)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not update template."})",
                    "application/json"
                );
                return;
            }

            if (changed == 0)
            {
                res.status = 404;
                res.set_content(
                    R"({"error":"Template not found or not owned by you."})",
                    "application/json"
                );
                return;
            }

            res.status = 200;
            res.set_content(
                R"({"message":"Robot template updated."})",
                "application/json"
            );
        }
    );
    
    // Delete a robot template owned by the logged-in user
    server.Delete(R"(/robots/templates/(\d+))",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            string authHeader =
                req.get_header_value("Authorization");

            if (authHeader.length() <= 7 ||
                authHeader.substr(0, 7) != "Bearer ")
            {
                res.status = 401;
                res.set_content(
                    R"({"error":"Missing or invalid authorization token."})",
                    "application/json"
                );
                return;
            }

            string token = authHeader.substr(7);
            string tokenHash = hashSessionToken(token);

            sqlite3_int64 templateId = 0;

            try
            {
                templateId = stoll(req.matches[1].str());
            }
            catch (...)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid template ID."})",
                    "application/json"
                );
                return;
            }

            if (templateId <= 0)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid template ID."})",
                    "application/json"
                );
                return;
            }

            sqlite3* db = NULL;

            if (!openDatabase(&db))
            {
                if (db != NULL)
                    sqlite3_close(db);

                res.status = 500;
                res.set_content(
                    R"({"error":"Database connection failed."})",
                    "application/json"
                );
                return;
            }

            // Verify the session and get its user ID
            const char* sessionSql =
                "SELECT user_id FROM sessions "
                "WHERE id = ? AND expires_at > ?;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sessionSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_text(
                    statement, 1, tokenHash.c_str(),
                    -1, SQLITE_TRANSIENT
                );

                sqlite3_bind_int64(
                    statement, 2,
                    static_cast<sqlite3_int64>(time(NULL))
                );

                result = sqlite3_step(statement);
            }

            if (result != SQLITE_ROW)
            {
                sqlite3_finalize(statement);
                sqlite3_close(db);

                res.status = (result == SQLITE_DONE) ? 401 : 500;
                res.set_content(
                    result == SQLITE_DONE
                        ? R"({"error":"Invalid or expired session."})"
                        : R"({"error":"Session lookup failed."})",
                    "application/json"
                );
                return;
            }

            sqlite3_int64 userId =
                sqlite3_column_int64(statement, 0);

            sqlite3_finalize(statement);

            // Delete only this user's template
            const char* deleteSql =
                "DELETE FROM robot_templates "
                "WHERE id = ? AND user_id = ?;";

            result = sqlite3_prepare_v2(
                db, deleteSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_int64(statement, 1, templateId);
                sqlite3_bind_int64(statement, 2, userId);

                result = sqlite3_step(statement);
            }

            int deleted = sqlite3_changes(db);

            sqlite3_finalize(statement);
            sqlite3_close(db);

            if (result != SQLITE_DONE)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not delete template."})",
                    "application/json"
                );
                return;
            }

            if (deleted == 0)
            {
                res.status = 404;
                res.set_content(
                    R"({"error":"Template not found or not owned by you."})",
                    "application/json"
                );
                return;
            }

            res.status = 200;
            res.set_content(
                R"({"message":"Robot template deleted."})",
                "application/json"
            );
        }
    );
    cout << "Robot Swarm Simulator server starting..." << endl;
    cout << "Home:     http://localhost:8080" << endl;
    cout << "Health:   http://localhost:8080/health" << endl;
    cout << "Register: POST http://localhost:8080/register" << endl;

    if (!server.listen("127.0.0.1", 8080))
    {
        cerr << "Failed to start server on port 8080." << endl;
        return 1;
    }

    return 0;
}
