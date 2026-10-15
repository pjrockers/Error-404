
#include <iostream>
#include "rescuerobot.h"

using namespace std;

RescueRobot::RescueRobot(int robotID,
                         string robotName,
                         int equipment)
    : Robot(robotID, robotName, "Rescue"),
      equipmentCount(equipment > 0 ? equipment : 0)
{
    // Rescue robots start with high priority.
    setPriority(3);
}

void RescueRobot::displayInfo()
{
    Robot::displayInfo();

    cout << "Rescue Equipment: "
         << equipmentCount << endl;
}
