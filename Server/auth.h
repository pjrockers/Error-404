#ifndef AUTH_H
#define AUTH_H

#include <string>
#include "cpp-httplib/httplib.h"

std::string hashSessionToken(const std::string& token);

void registerAuthRoutes(httplib::Server& server);

#endif