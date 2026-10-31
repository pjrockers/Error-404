#ifndef SIMULATION_ROUTES_H
#define SIMULATION_ROUTES_H

#include "cpp-httplib/httplib.h"
#include "sqlite/sqlite3.h"

void registerSimulationRoutes(httplib::Server& server);

#endif