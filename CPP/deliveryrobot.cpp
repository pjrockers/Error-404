
#include <iostream>
#include "deliveryrobot.h"

using namespace std;

DeliveryRobot::DeliveryRobot(int robotID,
                             string robotName,
                             int capacity)
    : Robot(robotID, robotName, "Delivery")
{
    packageCapacity = capacity;
    packagesCarried = 0;
}

void DeliveryRobot::loadPackages(int count)
{
    if (count <= 0)
    {
        cout << "Package count must be positive.\n";
        return;
    }

    if (packagesCarried + count > packageCapacity)
    {
        cout << "Cannot load packages. Capacity exceeded!\n";
        return;
    }

    packagesCarried += count;

    cout << count << " package(s) loaded successfully.\n";
}

void DeliveryRobot::displayInfo()
{
    Robot::displayInfo();

    cout << "Package Capacity: " << packageCapacity << endl;
    cout << "Packages Carried: " << packagesCarried << endl;
}
