
#ifndef RESCUEROBOT_H
#define RESCUEROBOT_H

#include "robot.h"

class RescueRobot : public Robot
{
private:
    int equipmentCount;

public:
    RescueRobot(int robotID, string robotName,
                int equipment);

    void displayInfo() override;
};

#endif
