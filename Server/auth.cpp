
#include "auth.h"
#include <iostream>
#include <string>
#include <cctype>
#include <nlohmann/json.hpp>
#include <sodium.h>
#include "database.h"
#include "sqlite/sqlite3.h"
#include <ctime>

using namespace std;
using json = nlohmann::json;

void registerAuthRoutes(httplib::Server& server)
{
    // User registration endpoint
    
    server.Post("/register",
        [](const httplib::Request& req,
   httplib::Response& res)
        {
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
            json requestData;

            try
            {
                requestData = json::parse(req.body);
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

            if (!requestData.is_object() ||
                !requestData.contains("username") ||
                !requestData.contains("password") ||
                !requestData["username"].is_string() ||
                !requestData["password"].is_string())
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Username and password are required as strings."})",
                    "application/json"
                );
                return;
            }

            string username = requestData["username"].get<string>();
            string password = requestData["password"].get<string>();

            if (username.length() < 3 || username.length() > 30)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Username must be 3 to 30 characters long."})",
                    "application/json"
                );
                return;
            }

            for (unsigned char ch : username)
            {
                if (!isalnum(ch) && ch != '_' && ch != '-')
                {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Username may contain only letters, numbers, underscores, and hyphens."})",
                        "application/json"
                    );
                    return;
                }
            }

            if (password.length() < 8 || password.length() > 128)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Password must be 8 to 128 bytes long."})",
                    "application/json"
                );
                return;
            }

            char passwordHash[crypto_pwhash_STRBYTES];

            if (crypto_pwhash_str(
                    passwordHash,
                    password.c_str(),
                    static_cast<unsigned long long>(password.length()),
                    crypto_pwhash_OPSLIMIT_INTERACTIVE,
                    crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Password hashing failed."})",
                    "application/json"
                );
                return;
            }

            const char* sql =
                "INSERT INTO users (username, password_hash) "
                "VALUES (?, ?);";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_text(
                    statement, 1, username.c_str(),
                    -1, SQLITE_TRANSIENT
                );

                sqlite3_bind_text(
                    statement, 2, passwordHash,
                    -1, SQLITE_TRANSIENT
                );

                result = sqlite3_step(statement);
            }

            if (result == SQLITE_DONE)
            {
                res.status = 201;
                res.set_content(
                    R"({"message":"Registration successful."})",
                    "application/json"
                );
            }
            else if ((result & 0xFF) == SQLITE_CONSTRAINT)
            {
                res.status = 409;
                res.set_content(
                    R"({"error":"Username already exists."})",
                    "application/json"
                );
            }
            else
            {
                cerr << "Registration database error: "
                     << sqlite3_errmsg(db) << endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Registration failed."})",
                    "application/json"
                );
            }

            if (statement != NULL)
                sqlite3_finalize(statement);
            sqlite3_close(db);
        }
    );


    // User login endpoint with session creation
    server.Post("/login",
        [](const httplib::Request& req,
   httplib::Response& res)
        {
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
            json requestData;

            try
            {
                requestData = json::parse(req.body);
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

            if (!requestData.is_object() ||
                !requestData.contains("username") ||
                !requestData.contains("password") ||
                !requestData["username"].is_string() ||
                !requestData["password"].is_string())
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Username and password are required."})",
                    "application/json"
                );
                return;
            }

            string username = requestData["username"].get<string>();
            string password = requestData["password"].get<string>();

            if (username.empty() || username.length() > 30 ||
                password.empty() || password.length() > 128)
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid username or password format."})",
                    "application/json"
                );
                return;
            }

            const char* sql =
                "SELECT id, password_hash FROM users WHERE username = ?;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
            );

            if (result != SQLITE_OK)
            {
                if (statement != NULL)
                    sqlite3_finalize(statement);

                res.status = 500;
                res.set_content(
                    R"({"error":"Database error."})",
                    "application/json"
                );
                return;
            }

            sqlite3_bind_text(
                statement, 1, username.c_str(),
                -1, SQLITE_TRANSIENT
            );

            result = sqlite3_step(statement);

            bool authenticated = false;
            sqlite3_int64 userId = 0;

            if (result == SQLITE_ROW)
            {
                userId = sqlite3_column_int64(statement, 0);

                const unsigned char* storedHash =
                    sqlite3_column_text(statement, 1);

                if (storedHash != NULL)
                {
                    authenticated =
                        crypto_pwhash_str_verify(
                            reinterpret_cast<const char*>(storedHash),
                            password.c_str(),
                            static_cast<unsigned long long>(password.length())
                        ) == 0;
                }
            }

            sqlite3_finalize(statement);

            if (!authenticated)
            {
                res.status = 401;
                res.set_content(
                    R"({"error":"Invalid username or password."})",
                    "application/json"
                );
                return;
            }

            // Generate a random session token
            unsigned char tokenBytes[32];
            randombytes_buf(tokenBytes, sizeof(tokenBytes));

            char tokenHex[65];

            sodium_bin2hex(
                tokenHex, sizeof(tokenHex),
                tokenBytes, sizeof(tokenBytes)
            );

            // Store only the hash of the token
            string tokenHashHex = hashSessionToken(tokenHex);

            sqlite3_int64 expiresAt =
                static_cast<sqlite3_int64>(time(NULL)) + 86400;

            const char* insertSql =
                "INSERT INTO sessions (id, user_id, expires_at) "
                "VALUES (?, ?, ?);";

            statement = NULL;

            result = sqlite3_prepare_v2(
                db, insertSql, -1, &statement, NULL
            );

            if (result == SQLITE_OK)
            {
                sqlite3_bind_text(
                    statement, 1, tokenHashHex.c_str(),
                    -1, SQLITE_TRANSIENT
                );

                sqlite3_bind_int64(statement, 2, userId);
                sqlite3_bind_int64(statement, 3, expiresAt);

                result = sqlite3_step(statement);
            }

            if (statement != NULL)
                sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not create session."})",
                    "application/json"
                );
                return;
            }

            json responseData = {
                {"message", "Login successful."},
                {"token", tokenHex},
                {"expires_in", 86400}
            };

            res.status = 200;
            res.set_content(responseData.dump(), "application/json");
            sqlite3_close(db);
        }
    );

    
    // Get the currently authenticated user's profile
    server.Get("/me",
        [](const httplib::Request& req,
   httplib::Response& res)
        {
            sqlite3* db = NULL;

if (sqlite3_open("robotswarm.db", &db) != SQLITE_OK)
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

            const char* sql =
                "SELECT users.id, users.username "
                "FROM sessions "
                "JOIN users ON sessions.user_id = users.id "
                "WHERE sessions.id = ? AND sessions.expires_at > ?;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
            );

            if (result != SQLITE_OK)
            {
                if (statement != NULL)
                    sqlite3_finalize(statement);

                res.status = 500;
                res.set_content(
                    R"({"error":"Database query failed."})",
                    "application/json"
                );
                return;
            }

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
            {
                sqlite3_int64 userId =
                    sqlite3_column_int64(statement, 0);

                const unsigned char* usernameText =
                    sqlite3_column_text(statement, 1);

                string username =
                    usernameText != NULL
                    ? reinterpret_cast<const char*>(usernameText)
                    : "";

                json responseData = {
                    {"id", userId},
                    {"username", username}
                };

                sqlite3_finalize(statement);

                res.status = 200;
                res.set_content(
                    responseData.dump(),
                    "application/json"
                );
            }
            else if (result == SQLITE_DONE)
            {
                sqlite3_finalize(statement);

                res.status = 401;
                res.set_content(
                    R"({"error":"Invalid or expired session."})",
                    "application/json"
                );
            }
            else
            {
                sqlite3_finalize(statement);

                res.status = 500;
                res.set_content(
                    R"({"error":"Session lookup failed."})",
                    "application/json"
                );
            }
            sqlite3_close(db);
        }
    );

    // Log out and invalidate the current session
    server.Post("/logout",
        [](const httplib::Request& req,
   httplib::Response& res)
        {
            sqlite3* db = NULL;

if (sqlite3_open("robotswarm.db", &db) != SQLITE_OK)
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

            const char* sql =
                "DELETE FROM sessions "
                "WHERE id = ? AND expires_at > ?;";

            sqlite3_stmt* statement = NULL;

            int result = sqlite3_prepare_v2(
                db, sql, -1, &statement, NULL
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

            int deleted = sqlite3_changes(db);

            if (statement != NULL)
                sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                res.status = 500;
                res.set_content(
                    R"({"error":"Logout failed."})",
                    "application/json"
                );
                return;
            }

            if (deleted == 0)
            {
                res.status = 401;
                res.set_content(
                    R"({"error":"Invalid or expired session."})",
                    "application/json"
                );
                return;
            }

            res.status = 200;
            res.set_content(
                R"({"message":"Logout successful."})",
                "application/json"
            );
            sqlite3_close(db);
        }
    );
}

