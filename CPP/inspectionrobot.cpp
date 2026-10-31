
#include <iostream>
#include "inspectionrobot.h"

using namespace std;

InspectionRobot::InspectionRobot(int robotID,
                                 string robotName)
    : Robot(robotID, robotName, "Inspection"),
      inspectionsCompleted(0)
{
}

void InspectionRobot::completeInspection()
{
    inspectionsCompleted++;

    cout << name << " completed an inspection.\n";
}

void InspectionRobot::displayInfo()
{
    Robot::displayInfo();

    cout << "Inspections Completed: "
         << inspectionsCompleted << endl;
}
int InspectionRobot::getInspectionsCompleted() const
{
    return inspectionsCompleted;
}

bool InspectionRobot::setInspectionsCompleted(int count)
{
    if (count < 0)
        return false;

    inspectionsCompleted = count;
    return true;
}
