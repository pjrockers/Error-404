
#ifndef ROBOTSTATE_H
#define ROBOTSTATE_H

#include <string>

struct RobotState
{
    int id;
    std::string name;
    std::string type;

    double x;
    double y;

    double destX;
    double destY;

    double battery;
    double speed;

    int priority;
    std::string status;
};

#endif
