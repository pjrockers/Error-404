
#ifndef DELIVERYROBOT_H
#define DELIVERYROBOT_H

#include "robot.h"

class DeliveryRobot : public Robot
{
private:
    int packageCapacity;
    int packagesCarried;

public:
    DeliveryRobot(int robotID, string robotName,
                  int capacity);

    void loadPackages(int count);
    int getPackageCapacity() const;
int getPackagesCarried() const;

bool setPackageCapacity(int capacity);
bool setPackagesCarried(int count);
    void displayInfo() override;
};

#endif
