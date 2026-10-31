#ifndef AUTH_H
#define AUTH_H

#include <string>
#include "cpp-httplib/httplib.h"
#include "sqlite/sqlite3.h"

std::string hashSessionToken(const std::string& token);

bool isValidSessionToken(const std::string& token);

bool getAuthenticatedUserId(
    const httplib::Request& req,
    sqlite3_int64& userId
);

void registerAuthRoutes(httplib::Server& server);

#endif