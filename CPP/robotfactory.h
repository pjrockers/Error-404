#ifndef ROBOTFACTORY_H
#define ROBOTFACTORY_H

#include "robot.h"
#include <nlohmann/json.hpp>

// Creates the correct robot object from its configuration.
// The caller is responsible for deleting the returned pointer.
Robot* createRobot(const nlohmann::json& data);

#endif