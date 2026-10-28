#include "robot_templates.h"
#include "auth.h"
#include "database.h"
#include <string>
#include <ctime>
#include <cmath>
#include <iostream>
#include <cctype>
#include <nlohmann/json.hpp>
#include "sqlite/sqlite3.h"

using namespace std;
using json = nlohmann::json;
bool isValidTemplateSessionToken(const string& token)
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
void registerRobotTemplateRoutes(httplib::Server& server)
{
   server.Post("/robots/templates",
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

if (!isValidTemplateSessionToken(token))
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

           if (templateName.find_first_not_of(" \t\n\r") == string::npos ||
    templateName.length() > 50 ||
    robotType.find_first_not_of(" \t\n\r") == string::npos ||
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
            DatabaseGuard dbGuard(db);

            // Find the user associated with the active session
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
        cerr << "Template session lookup failed: "
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

            sqlite3_int64 userId =
                sqlite3_column_int64(statement, 0);

            sqlite3_finalize(statement);

            // Save the template under this user's ID
            const char* insertSql =
                "INSERT INTO robot_templates "
                "(user_id, template_name, robot_type, battery, "
                "speed, priority, attributes) "
                "VALUES (?, ?, ?, ?, ?, ?, ?);";

            result = sqlite3_prepare_v2(
                db, insertSql, -1, &statement, NULL
            );

            
if (result == SQLITE_OK)
{
    result = sqlite3_bind_int64(statement, 1, userId);

    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(
            statement, 2, templateName.c_str(),
            -1, SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(
            statement, 3, robotType.c_str(),
            -1, SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK)
        result = sqlite3_bind_double(statement, 4, battery);

    if (result == SQLITE_OK)
        result = sqlite3_bind_double(statement, 5, speed);

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 6, priority);

    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(
            statement, 7, attributes.c_str(),
            -1, SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK)
        result = sqlite3_step(statement);
}


            sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                cerr << "Template insertion failed: "
     << sqlite3_errmsg(db) << endl;
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not save robot template."})",
                    "application/json"
                );
                return;
            }

            json responseData = {
                {"message", "Robot template created."},
                {"template_name", templateName}
            };

            res.status = 201;
            res.set_content(
                responseData.dump(),
                "application/json"
            );
        }
    );

    server.Get("/robots/templates",
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

if (!isValidTemplateSessionToken(token))
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
                if (db != NULL)
                    sqlite3_close(db);

                res.status = 500;
                res.set_content(
                    R"({"error":"Database connection failed."})",
                    "application/json"
                );
                return;
            }
            DatabaseGuard dbGuard(db);

            // Verify the session and retrieve its user ID
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
        cerr << "Template session lookup failed: "
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

            sqlite3_int64 userId =
                sqlite3_column_int64(statement, 0);

            sqlite3_finalize(statement);

            // Retrieve only this user's templates
            const char* templatesSql =
                "SELECT id, template_name, robot_type, battery, "
                "speed, priority, attributes, created_at "
                "FROM robot_templates WHERE user_id = ? "
                "ORDER BY id DESC;";

            
result = sqlite3_prepare_v2(
    db, templatesSql, -1, &statement, NULL
);

if (result != SQLITE_OK)
{
    cerr << "Template query preparation failed: "
         << sqlite3_errmsg(db) << endl;

    res.status = 500;
    res.set_content(
        R"({"error":"Could not retrieve templates."})",
        "application/json"
    );
    return;
}

result = sqlite3_bind_int64(statement, 1, userId);

if (result != SQLITE_OK)
{
    cerr << "Template query binding failed: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    res.status = 500;
    res.set_content(
        R"({"error":"Could not retrieve templates."})",
        "application/json"
    );
    return;
}
            json templates = json::array();

            while ((result = sqlite3_step(statement)) == SQLITE_ROW)
            {
                const unsigned char* name =
                    sqlite3_column_text(statement, 1);

                const unsigned char* type =
                    sqlite3_column_text(statement, 2);

                const unsigned char* attributes =
                    sqlite3_column_text(statement, 6);

                const unsigned char* createdAt =
                    sqlite3_column_text(statement, 7);

                json customAttributes = json::object();

                if (attributes != NULL)
                {
                    try
                    {
                        customAttributes = json::parse(
                            reinterpret_cast<const char*>(attributes)
                        );
                    }
                    catch (...)
                    {
                        customAttributes = json::object();
                    }
                }

                templates.push_back({
                    {"id", sqlite3_column_int64(statement, 0)},
                    {"template_name", name != NULL
                        ? reinterpret_cast<const char*>(name) : ""},
                    {"robot_type", type != NULL
                        ? reinterpret_cast<const char*>(type) : ""},
                    {"battery", sqlite3_column_double(statement, 3)},
                    {"speed", sqlite3_column_double(statement, 4)},
                    {"priority", sqlite3_column_int(statement, 5)},
                    {"attributes", customAttributes},
                    {"created_at", createdAt != NULL
                        ? reinterpret_cast<const char*>(createdAt) : ""}
                });
            }

            sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                cerr << "Template retrieval failed: "
     << sqlite3_errmsg(db) << endl;
                res.status = 500;
                res.set_content(
                    R"({"error":"Failed while reading templates."})",
                    "application/json"
                );
                return;
            }

            res.status = 200;
            res.set_content(
                templates.dump(4),
                "application/json"
            );
        }
    );

}