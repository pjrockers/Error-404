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

// Check for duplicate template name
const char* nameCheckSql =
    "SELECT id FROM robot_templates "
    "WHERE user_id = ? "
    "AND lower(trim(template_name)) = lower(trim(?));";

statement = NULL;

result = sqlite3_prepare_v2(
    db, nameCheckSql, -1, &statement, NULL
);

if (result == SQLITE_OK)
    result = sqlite3_bind_int64(statement, 1, userId);

if (result == SQLITE_OK)
    result = sqlite3_bind_text(
        statement, 2, templateName.c_str(),
        -1, SQLITE_TRANSIENT
    );

if (result == SQLITE_OK)
    result = sqlite3_step(statement);

if (result == SQLITE_ROW)
{
    sqlite3_finalize(statement);

    res.status = 409;
    res.set_content(
        R"({"error":"A template with this name already exists."})",
        "application/json"
    );
    return;
}

sqlite3_finalize(statement);

if (result != SQLITE_DONE)
{
    res.status = 500;
    res.set_content(
        R"({"error":"Could not check template name."})",
        "application/json"
    );
    return;
}

// Check for duplicate template configuration
const char* duplicateCheckSql =
    "SELECT id FROM robot_templates "
    "WHERE user_id = ? "
    "AND lower(trim(robot_type)) = lower(trim(?)) "
    "AND battery = ? AND speed = ? AND priority = ? "
    "AND attributes = ? LIMIT 1;";

statement = NULL;

result = sqlite3_prepare_v2(
    db, duplicateCheckSql, -1, &statement, NULL
);

if (result == SQLITE_OK)
    result = sqlite3_bind_int64(statement, 1, userId);

if (result == SQLITE_OK)
    result = sqlite3_bind_text(
        statement, 2, robotType.c_str(),
        -1, SQLITE_TRANSIENT
    );

if (result == SQLITE_OK)
    result = sqlite3_bind_double(statement, 3, battery);

if (result == SQLITE_OK)
    result = sqlite3_bind_double(statement, 4, speed);

if (result == SQLITE_OK)
    result = sqlite3_bind_int(statement, 5, priority);

if (result == SQLITE_OK)
    result = sqlite3_bind_text(
        statement, 6, attributes.c_str(),
        -1, SQLITE_TRANSIENT
    );

if (result == SQLITE_OK)
    result = sqlite3_step(statement);

if (result == SQLITE_ROW)
{
    sqlite3_finalize(statement);

    res.status = 409;
    res.set_content(
        R"({"error":"An identical robot template already exists."})",
        "application/json"
    );
    return;
}

sqlite3_finalize(statement);

if (result != SQLITE_DONE)
{
    res.status = 500;
    res.set_content(
        R"({"error":"Could not check duplicate template configuration."})",
        "application/json"
    );
    return;
}

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

    // Update an existing robot template
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

            // Get template ID from URL
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

            // Parse request body
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

            // Find the logged-in user's ID
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
            }

            if (result == SQLITE_OK)
            {
                result = sqlite3_bind_int64(
                    statement, 2,
                    static_cast<sqlite3_int64>(time(NULL))
                );
            }

            if (result == SQLITE_OK)
                result = sqlite3_step(statement);

            if (result != SQLITE_ROW)
            {
                if (statement != NULL)
                    sqlite3_finalize(statement);

                res.status = (result == SQLITE_DONE) ? 401 : 500;
                res.set_content(
                    (result == SQLITE_DONE)
                        ? R"({"error":"Invalid or expired session."})"
                        : R"({"error":"Session lookup failed."})",
                    "application/json"
                );
                return;
            }

            sqlite3_int64 userId =
                sqlite3_column_int64(statement, 0);

            sqlite3_finalize(statement);

            // Check whether another template has the same name
            const char* nameCheckSql =
                "SELECT id FROM robot_templates "
                "WHERE user_id = ? "
                "AND lower(trim(template_name)) = lower(trim(?)) "
                "AND id != ? LIMIT 1;";

            statement = NULL;

            result = sqlite3_prepare_v2(
                db, nameCheckSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(statement, 1, userId);

            if (result == SQLITE_OK)
                result = sqlite3_bind_text(
                    statement, 2, templateName.c_str(),
                    -1, SQLITE_TRANSIENT
                );

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(statement, 3, templateId);

            if (result == SQLITE_OK)
                result = sqlite3_step(statement);

            if (result == SQLITE_ROW)
            {
                sqlite3_finalize(statement);

                res.status = 409;
                res.set_content(
                    R"({"error":"Another template already has this name."})",
                    "application/json"
                );
                return;
            }

            sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not check template name."})",
                    "application/json"
                );
                return;
            }

            // Check for identical configuration in another template
            const char* duplicateCheckSql =
                "SELECT id FROM robot_templates "
                "WHERE user_id = ? "
                "AND lower(trim(robot_type)) = lower(trim(?)) "
                "AND battery = ? AND speed = ? AND priority = ? "
                "AND attributes = ? AND id != ? LIMIT 1;";

            statement = NULL;

            result = sqlite3_prepare_v2(
                db, duplicateCheckSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(statement, 1, userId);

            if (result == SQLITE_OK)
                result = sqlite3_bind_text(
                    statement, 2, robotType.c_str(),
                    -1, SQLITE_TRANSIENT
                );

            if (result == SQLITE_OK)
                result = sqlite3_bind_double(statement, 3, battery);

            if (result == SQLITE_OK)
                result = sqlite3_bind_double(statement, 4, speed);

            if (result == SQLITE_OK)
                result = sqlite3_bind_int(statement, 5, priority);

            if (result == SQLITE_OK)
                result = sqlite3_bind_text(
                    statement, 6, attributes.c_str(),
                    -1, SQLITE_TRANSIENT
                );

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(statement, 7, templateId);

            if (result == SQLITE_OK)
                result = sqlite3_step(statement);

            if (result == SQLITE_ROW)
            {
                sqlite3_finalize(statement);

                res.status = 409;
                res.set_content(
                    R"({"error":"An identical robot template already exists."})",
                    "application/json"
                );
                return;
            }

            sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not check duplicate template configuration."})",
                    "application/json"
                );
                return;
            }

            // Update only a template belonging to this user
            const char* updateSql =
                "UPDATE robot_templates SET "
                "template_name = ?, robot_type = ?, battery = ?, "
                "speed = ?, priority = ?, attributes = ? "
                "WHERE id = ? AND user_id = ?;";

            statement = NULL;

            result = sqlite3_prepare_v2(
                db, updateSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
                result = sqlite3_bind_text(
                    statement, 1, templateName.c_str(),
                    -1, SQLITE_TRANSIENT
                );

            if (result == SQLITE_OK)
                result = sqlite3_bind_text(
                    statement, 2, robotType.c_str(),
                    -1, SQLITE_TRANSIENT
                );

            if (result == SQLITE_OK)
                result = sqlite3_bind_double(statement, 3, battery);

            if (result == SQLITE_OK)
                result = sqlite3_bind_double(statement, 4, speed);

            if (result == SQLITE_OK)
                result = sqlite3_bind_int(statement, 5, priority);

            if (result == SQLITE_OK)
                result = sqlite3_bind_text(
                    statement, 6, attributes.c_str(),
                    -1, SQLITE_TRANSIENT
                );

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(statement, 7, templateId);

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(statement, 8, userId);

            if (result == SQLITE_OK)
                result = sqlite3_step(statement);

            sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not update robot template."})",
                    "application/json"
                );
                return;
            }

            if (sqlite3_changes(db) == 0)
            {
                res.status = 404;
                res.set_content(
                    R"({"error":"Template not found."})",
                    "application/json"
                );
                return;
            }

            res.status = 200;
            res.set_content(
                R"({"message":"Robot template updated successfully."})",
                "application/json"
            );
        }
    );

    // Delete an existing robot template
    server.Delete(R"(/robots/templates/(\d+))",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            // Check authorization header
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

            // Get template ID from URL
            sqlite3_int64 templateId = 0;

            try
            {
                templateId = stoll(req.matches[1].str());

                if (templateId <= 0)
                    throw invalid_argument("Invalid ID");
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

            // Find the logged-in user's ID
            const char* sessionSql =
                "SELECT user_id FROM sessions "
                "WHERE id = ? AND expires_at > ?;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sessionSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
                result = sqlite3_bind_text(
                    statement, 1, tokenHash.c_str(),
                    -1, SQLITE_TRANSIENT
                );

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(
                    statement, 2,
                    static_cast<sqlite3_int64>(time(NULL))
                );

            if (result == SQLITE_OK)
                result = sqlite3_step(statement);

            if (result != SQLITE_ROW)
            {
                if (statement != NULL)
                    sqlite3_finalize(statement);

                res.status = (result == SQLITE_DONE) ? 401 : 500;

                res.set_content(
                    (result == SQLITE_DONE)
                        ? R"({"error":"Invalid or expired session."})"
                        : R"({"error":"Session lookup failed."})",
                    "application/json"
                );
                return;
            }

            sqlite3_int64 userId =
                sqlite3_column_int64(statement, 0);

            sqlite3_finalize(statement);

            // Delete only a template belonging to this user
            const char* deleteSql =
                "DELETE FROM robot_templates "
                "WHERE id = ? AND user_id = ?;";

            statement = NULL;

            result = sqlite3_prepare_v2(
                db, deleteSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(
                    statement, 1, templateId
                );

            if (result == SQLITE_OK)
                result = sqlite3_bind_int64(
                    statement, 2, userId
                );

            if (result == SQLITE_OK)
                result = sqlite3_step(statement);

            sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not delete robot template."})",
                    "application/json"
                );
                return;
            }

            if (sqlite3_changes(db) == 0)
            {
                res.status = 404;
                res.set_content(
                    R"({"error":"Template not found."})",
                    "application/json"
                );
                return;
            }

            res.status = 200;
            res.set_content(
                R"({"message":"Robot template deleted successfully."})",
                "application/json"
            );
        }
    );

}