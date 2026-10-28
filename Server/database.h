#ifndef DATABASE_H
#define DATABASE_H

#include "sqlite/sqlite3.h"

bool initializeDatabase();
bool openDatabase(sqlite3** db);

#endif