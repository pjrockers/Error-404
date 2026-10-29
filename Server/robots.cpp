
#include "robots.h"
#include "database.h"
#include "sqlite/sqlite3.h"

#include <nlohmann/json.hpp>
#include <sodium.h>

#include <string>
#include <ctime>
#include <cmath>
#include <stdexcept>

using namespace std;
using json = nlohmann::json;

// Hash tokens in the same way as the existing backend.
static string hashToken(const string& token)
{
    unsigned char hash[crypto_generichash_BYTES];

    crypto_generichash(
        hash, sizeof(hash),
        reinterpret_cast<const unsigned char*>(token.data()),
        token.size(), NULL, 0
    );

    char hex[crypto_generichash_BYTES * 2 + 1];

    sodium_bin2hex(
        hex, sizeof(hex), hash, sizeof(hash)
    );

    return string(hex);
}

// Validate the session and return its user ID.
// The caller must close the database connection.
static bool getUserId(
    sqlite3* db,
    const string& token,
    sqlite3_int64& userId
)
{
    const char* sql =
        "SELECT user_id FROM sessions "
        "WHERE id = ? AND expires_at > ?;";

    sqlite3_stmt* statement = NULL;

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, NULL
    );

    if (result != SQLITE_OK)
        return false;

    string tokenHash = hashToken(token);

    sqlite3_bind_text(
        statement, 1, tokenHash.c_str(),
        -1, SQLITE_TRANSIENT
    );

    sqlite3_bind_int64(
        statement, 2,
        static_cast<sqlite3_int64>(time(NULL))
    );

    result = sqlite3_step(statement);

    if (result == SQLITE_ROW)
        userId = sqlite3_column_int64(statement, 0);

    sqlite3_finalize(statement);

    return result == SQLITE_ROW;
}

static bool readToken(
    const httplib::Request& req,
    httplib::Response& res,
    string& token
)
{
    string header = req.get_header_value("Authorization");

    if (header.length() <= 7 ||
        header.substr(0, 7) != "Bearer ")
    {
        res.status = 401;
        res.set_content(
            R"({"error":"Missing or invalid authorization token."})",
            "application/json"
        );
        return false;
    }

    token = header.substr(7);

    if (token.empty())
    {
        res.status = 401;
        res.set_content(
            R"({"error":"Missing or invalid authorization token."})",
            "application/json"
        );
        return false;
    }

    return true;
}

static bool readRobotData(
    const httplib::Request& req,
    httplib::Response& res,
    json& data
)
{
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
        return false;
    }

    if (!data.is_object() ||
        !data.contains("name") ||
        !data.contains("robot_type") ||
        !data.contains("x") ||
        !data.contains("y") ||
        !data.contains("battery") ||
        !data.contains("speed") ||
        !data.contains("priority") ||
        !data.contains("status") ||
        !data["name"].is_string() ||
        !data["robot_type"].is_string() ||
        !data["x"].is_number_integer() ||
        !data["y"].is_number_integer() ||
        !data["battery"].is_number() ||
        !data["speed"].is_number() ||
        !data["priority"].is_number_integer() ||
        !data["status"].is_string() ||
        (data.contains("attributes") &&
         !data["attributes"].is_object()))
    {
        res.status = 400;
        res.set_content(
            R"({"error":"Missing or invalid robot fields."})",
            "application/json"
        );
        return false;
    }

    string name = data["name"].get<string>();
    string type = data["robot_type"].get<string>();
    string status = data["status"].get<string>();

    double battery = data["battery"].get<double>();
    double speed = data["speed"].get<double>();
    int priority = data["priority"].get<int>();

    // Reject excessively large coordinates before converting to int.
    long long x = data["x"].get<long long>();
    long long y = data["y"].get<long long>();

    if (name.empty() || name.length() > 50 ||
        type.empty() || type.length() > 50 ||
        status.empty() || status.length() > 30 ||
        x < 0 || x > 100000 ||
        y < 0 || y > 100000 ||
        !isfinite(battery) ||
        !isfinite(speed) ||
        battery < 0 || battery > 100 ||
        speed <= 0 || speed > 100 ||
        priority < 1 || priority > 4)
    {
        res.status = 400;
        res.set_content(
            R"({"error":"Robot values are out of range."})",
            "application/json"
        );
        return false;
    }

    if (data.contains("attributes") &&
        !data["attributes"].is_object())
    {
        res.status = 400;
        res.set_content(
            R"({"error":"Attributes must be a JSON object."})",
            "application/json"
        );
        return false;
    }

    return true;
}

static void sendDatabaseError(httplib::Response& res)
{
    res.status = 500;
    res.set_content(
        R"({"error":"Database operation failed."})",
        "application/json"
    );
}

void registerRobotRoutes(httplib::Server& server)
{
    // GET /robots - list robots belonging to the logged-in user.
    server.Get("/robots",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            string token;

            if (!readToken(req, res, token))
                return;

            sqlite3* db = NULL;

            if (!openDatabase(&db))
            {
                if (db != NULL)
                    sqlite3_close(db);

                sendDatabaseError(res);
                return;
            }

            sqlite3_int64 userId = 0;

            if (!getUserId(db, token, userId))
            {
                sqlite3_close(db);
                res.status = 401;
                res.set_content(
                    R"({"error":"Invalid or expired session."})",
                    "application/json"
                );
                return;
            }

            const char* sql =
                "SELECT id, name, robot_type, x, y, battery, "
                "speed, priority, status, attributes, created_at "
                "FROM robots WHERE user_id = ? ORDER BY id;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
            );

            if (result != SQLITE_OK)
            {
                sqlite3_close(db);
                sendDatabaseError(res);
                return;
            }

            sqlite3_bind_int64(statement, 1, userId);

            json robots = json::array();

            while ((result = sqlite3_step(statement)) == SQLITE_ROW)
            {
                json robot;

                robot["id"] = sqlite3_column_int64(statement, 0);
                robot["name"] = reinterpret_cast<const char*>(
                    sqlite3_column_text(statement, 1));
                robot["type"] = reinterpret_cast<const char*>(
                    sqlite3_column_text(statement, 2));
                robot["x"] = sqlite3_column_int(statement, 3);
                robot["y"] = sqlite3_column_int(statement, 4);
                robot["battery"] = sqlite3_column_double(statement, 5);
                robot["speed"] = sqlite3_column_double(statement, 6);
                robot["priority"] = sqlite3_column_int(statement, 7);
                robot["status"] = reinterpret_cast<const char*>(
                    sqlite3_column_text(statement, 8));

                const unsigned char* attributes =
                    sqlite3_column_text(statement, 9);

                try
                {
                    robot["attributes"] =
                        attributes != NULL
                        ? json::parse(
                            reinterpret_cast<const char*>(attributes))
                        : json::object();
                }
                catch (...)
                {
                    robot["attributes"] = json::object();
                }

                const unsigned char* created =
                    sqlite3_column_text(statement, 10);

                robot["created_at"] =
                    created != NULL
                    ? reinterpret_cast<const char*>(created)
                    : "";

                robots.push_back(robot);
            }

            sqlite3_finalize(statement);
            sqlite3_close(db);

            if (result != SQLITE_DONE)
            {
                sendDatabaseError(res);
                return;
            }

            res.set_content(robots.dump(), "application/json");
        }
    );

    // POST /robots - create a robot.
    server.Post("/robots",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            string token;

            if (!readToken(req, res, token))
                return;

            json data;

            if (!readRobotData(req, res, data))
                return;

            string name = data["name"].get<string>();
            string type = data["robot_type"].get<string>();
            string status = data["status"].get<string>();

            string attributes =
                data.value("attributes", json::object()).dump();

            sqlite3* db = NULL;

            if (!openDatabase(&db))
            {
                if (db != NULL)
                    sqlite3_close(db);

                sendDatabaseError(res);
                return;
            }

            sqlite3_int64 userId = 0;

            if (!getUserId(db, token, userId))
            {
                sqlite3_close(db);
                res.status = 401;
                res.set_content(
                    R"({"error":"Invalid or expired session."})",
                    "application/json"
                );
                return;
            }

            const char* sql =
                "INSERT INTO robots "
                "(user_id, name, robot_type, x, y, battery, speed, "
                "priority, status, attributes) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_int64(statement, 1, userId);
                sqlite3_bind_text(statement, 2, name.c_str(),
                    -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(statement, 3, type.c_str(),
                    -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(statement, 4,
                    data["x"].get<int>());
                sqlite3_bind_int(statement, 5,
                    data["y"].get<int>());
                sqlite3_bind_double(statement, 6,
                    data["battery"].get<double>());
                sqlite3_bind_double(statement, 7,
                    data["speed"].get<double>());
                sqlite3_bind_int(statement, 8,
                    data["priority"].get<int>());
                sqlite3_bind_text(statement, 9, status.c_str(),
                    -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(statement, 10, attributes.c_str(),
                    -1, SQLITE_TRANSIENT);

                result = sqlite3_step(statement);
            }

            sqlite3_int64 newId = sqlite3_last_insert_rowid(db);

            sqlite3_finalize(statement);
            sqlite3_close(db);

            if (result != SQLITE_DONE)
            {
                sendDatabaseError(res);
                return;
            }

            res.status = 201;
            res.set_content(
                json({
                    {"id", newId},
                    {"message", "Robot created successfully."}
                }).dump(),
                "application/json"
            );
        }
    );

    // PUT /robots/<id> - update a robot owned by this user.
    server.Put(R"(/robots/(\d+))",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            string token;

            if (!readToken(req, res, token))
                return;

            sqlite3_int64 robotId = 0;

            try
            {
                robotId = stoll(req.matches[1].str());
            }
            catch (...)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid robot ID."})",
                    "application/json"
                );
                return;
            }

            if (robotId <= 0)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid robot ID."})",
                    "application/json"
                );
                return;
            }

            json data;

            if (!readRobotData(req, res, data))
                return;

            string name = data["name"].get<string>();
            string type = data["robot_type"].get<string>();
            string status = data["status"].get<string>();

            string attributes =
                data.value("attributes", json::object()).dump();

            sqlite3* db = NULL;

            if (!openDatabase(&db))
            {
                if (db != NULL)
                    sqlite3_close(db);

                sendDatabaseError(res);
                return;
            }

            sqlite3_int64 userId = 0;

            if (!getUserId(db, token, userId))
            {
                sqlite3_close(db);
                res.status = 401;
                res.set_content(
                    R"({"error":"Invalid or expired session."})",
                    "application/json"
                );
                return;
            }

            const char* sql =
                "UPDATE robots SET name = ?, robot_type = ?, "
                "x = ?, y = ?, battery = ?, speed = ?, priority = ?, "
                "status = ?, attributes = ? "
                "WHERE id = ? AND user_id = ?;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_text(statement, 1, name.c_str(),
                    -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(statement, 2, type.c_str(),
                    -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(statement, 3,
                    data["x"].get<int>());
                sqlite3_bind_int(statement, 4,
                    data["y"].get<int>());
                sqlite3_bind_double(statement, 5,
                    data["battery"].get<double>());
                sqlite3_bind_double(statement, 6,
                    data["speed"].get<double>());
                sqlite3_bind_int(statement, 7,
                    data["priority"].get<int>());
                sqlite3_bind_text(statement, 8, status.c_str(),
                    -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(statement, 9, attributes.c_str(),
                    -1, SQLITE_TRANSIENT);
                sqlite3_bind_int64(statement, 10, robotId);
                sqlite3_bind_int64(statement, 11, userId);

                result = sqlite3_step(statement);
            }

            int changed = sqlite3_changes(db);

            sqlite3_finalize(statement);
            sqlite3_close(db);

            if (result != SQLITE_DONE)
            {
                sendDatabaseError(res);
                return;
            }

            if (changed == 0)
            {
                res.status = 404;
                res.set_content(
                    R"({"error":"Robot not found or not owned by you."})",
                    "application/json"
                );
                return;
            }

            res.set_content(
                R"({"message":"Robot updated successfully."})",
                "application/json"
            );
        }
    );

    // DELETE /robots/<id> - delete a robot owned by this user.
    server.Delete(R"(/robots/(\d+))",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            string token;

            if (!readToken(req, res, token))
                return;

            sqlite3_int64 robotId = 0;

            try
            {
                robotId = stoll(req.matches[1].str());
            }
            catch (...)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid robot ID."})",
                    "application/json"
                );
                return;
            }

            if (robotId <= 0)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid robot ID."})",
                    "application/json"
                );
                return;
            }

            sqlite3* db = NULL;

            if (!openDatabase(&db))
            {
                if (db != NULL)
                    sqlite3_close(db);

                sendDatabaseError(res);
                return;
            }

            sqlite3_int64 userId = 0;

            if (!getUserId(db, token, userId))
            {
                sqlite3_close(db);
                res.status = 401;
                res.set_content(
                    R"({"error":"Invalid or expired session."})",
                    "application/json"
                );
                return;
            }

            const char* sql =
                "DELETE FROM robots WHERE id = ? AND user_id = ?;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_int64(statement, 1, robotId);
                sqlite3_bind_int64(statement, 2, userId);

                result = sqlite3_step(statement);
            }

            int deleted = sqlite3_changes(db);

            sqlite3_finalize(statement);
            sqlite3_close(db);

            if (result != SQLITE_DONE)
            {
                sendDatabaseError(res);
                return;
            }

            if (deleted == 0)
            {
                res.status = 404;
                res.set_content(
                    R"({"error":"Robot not found or not owned by you."})",
                    "application/json"
                );
                return;
            }

            res.set_content(
                R"({"message":"Robot deleted successfully."})",
                "application/json"
            );
        }
    );
}
