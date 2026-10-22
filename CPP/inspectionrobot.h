
#ifndef INSPECTIONROBOT_H
#define INSPECTIONROBOT_H

#include "robot.h"

class InspectionRobot : public Robot
{
private:
    int inspectionsCompleted;

public:
    InspectionRobot(int robotID, string robotName);

    void completeInspection();
    void displayInfo() override;
};

#endif
