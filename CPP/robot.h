
#ifndef ROBOT_H
#define ROBOT_H

#include <string>
#include "robotstate.h"
using namespace std;

class Robot
{
protected:
    int id;
    string name;
    string type;

    double x, y;
    double destX, destY;

    double battery;
    double speed;
    int priority;
    string status;

public:
    Robot(int robotID,
          string robotName = "Robot",
          string robotType = "General");

    virtual ~Robot();

    virtual void displayInfo();

    void setPriority(int p);
    int getPriority() const;

    void setDestination(double dx, double dy);

    int getID() const;
    string getName() const;
    string getType() const;
    double getSpeed() const;
    double getBattery() const;
    string getStatus() const;
    RobotState getState() const;
    bool setPosition(double px, double py);

double getX() const;
double getY() const;
double getDestX() const;
double getDestY() const;
bool moveToNextCell(int nextX, int nextY);
void setStatus(string newStatus);
void consumeBattery(double amount);
bool hasBattery() const;
bool setBattery(double amount);
};

#endif
