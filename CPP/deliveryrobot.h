
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
    void displayInfo() override;
};

#endif
