
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
    int getInspectionsCompleted() const;
bool setInspectionsCompleted(int count);
    void displayInfo() override;
};

#endif
