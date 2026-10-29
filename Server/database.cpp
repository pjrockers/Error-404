
#include "sqlite/sqlite3.h"
#include "database.h"

#include <iostream>

using namespace std;

bool openDatabase(sqlite3** db)
{
    if (db == NULL)
        return false;

    *db = NULL;

    int result = sqlite3_open("robotswarm.db", db);

    if (result != SQLITE_OK)
    {
        if (*db != NULL)
        {
            cerr << "Database connection failed: "
                 << sqlite3_errmsg(*db) << endl;

            sqlite3_close(*db);
            *db = NULL;
        }
        else
        {
            cerr << "Database connection failed." << endl;
        }

        return false;
    }

    char* errorMessage = NULL;

    result = sqlite3_exec(
        *db,
        "PRAGMA foreign_keys = ON;",
        NULL,
        NULL,
        &errorMessage
    );

    if (result != SQLITE_OK)
    {
        cerr << "Failed to enable foreign keys: "
             << (errorMessage != NULL
                     ? errorMessage
                     : sqlite3_errmsg(*db))
             << endl;

        sqlite3_free(errorMessage);
        sqlite3_close(*db);
        *db = NULL;

        return false;
    }

    sqlite3_free(errorMessage);

    return true;
}

bool initializeDatabase()
{
    sqlite3* db = NULL;

    int result = sqlite3_open("robotswarm.db", &db);

    if (result != SQLITE_OK)
    {
        cerr << "Database error: "
             << (db != NULL ? sqlite3_errmsg(db)
                            : "Unable to allocate database connection")
             << endl;

        if (db != NULL)
            sqlite3_close(db);

        return false;
    }

    const char* sql =
        "PRAGMA foreign_keys = ON;"

        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL UNIQUE,"
        "password_hash TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");"

        "CREATE TABLE IF NOT EXISTS robot_templates ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "template_name TEXT NOT NULL,"
        "robot_type TEXT NOT NULL,"
        "battery REAL NOT NULL DEFAULT 100,"
        "speed REAL NOT NULL DEFAULT 5,"
        "priority INTEGER NOT NULL DEFAULT 1,"
        "attributes TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"

        
        "CREATE TABLE IF NOT EXISTS robots ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "name TEXT NOT NULL,"
        "robot_type TEXT NOT NULL,"
        "x INTEGER NOT NULL DEFAULT 0,"
        "y INTEGER NOT NULL DEFAULT 0,"
        "battery REAL NOT NULL DEFAULT 100,"
        "speed REAL NOT NULL DEFAULT 5,"
        "priority INTEGER NOT NULL DEFAULT 1,"
        "status TEXT NOT NULL DEFAULT 'Idle',"
        "attributes TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"


        "CREATE TABLE IF NOT EXISTS simulation_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "simulation_name TEXT NOT NULL,"
        "grid_rows INTEGER NOT NULL,"
        "grid_cols INTEGER NOT NULL,"
        "robot_count INTEGER NOT NULL,"
        "result TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"

        "CREATE TABLE IF NOT EXISTS sessions ("
        "id TEXT PRIMARY KEY,"
        "user_id INTEGER NOT NULL,"
        "expires_at INTEGER NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";

    char* errorMessage = NULL;

    result = sqlite3_exec(db, sql, NULL, NULL, &errorMessage);

    if (result != SQLITE_OK)
    {
        cerr << "Table creation failed: "
             << (errorMessage != NULL
                     ? errorMessage
                     : sqlite3_errmsg(db))
             << endl;

        sqlite3_free(errorMessage);
        sqlite3_close(db);
        return false;
    }

    sqlite3_free(errorMessage);
    sqlite3_close(db);

    cout << "Database initialized successfully." << endl;

    return true;
}

