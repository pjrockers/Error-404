#ifndef DATABASE_H
#define DATABASE_H
#include <iostream>
#include "sqlite/sqlite3.h"

bool initializeDatabase();
bool openDatabase(sqlite3** db);

class DatabaseGuard
{
private:
    sqlite3* db;

public:
    DatabaseGuard(sqlite3* connection)
        : db(connection)
    {
    }

    ~DatabaseGuard()
    {
        if (db != NULL)
            sqlite3_close(db);
    }

    DatabaseGuard(const DatabaseGuard&) = delete;
    DatabaseGuard& operator=(const DatabaseGuard&) = delete;
};
#endif