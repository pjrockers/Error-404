
#include <iostream>
#include "sqlite/sqlite3.h"

using namespace std;

int main()
{
    sqlite3* db = NULL;

    int result = sqlite3_open("test.db", &db);

    if (result == SQLITE_OK)
    {
        cout << "SQLite connected successfully!" << endl;

        sqlite3_close(db);
    }
    else
    {
        cout << "SQLite connection failed!" << endl;

        if (db != NULL)
        {
            sqlite3_close(db);
        }
    }

    return 0;
}
