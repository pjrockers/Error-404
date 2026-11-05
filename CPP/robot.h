
#ifndef ROBOT_H
#define ROBOT_H
#include <vector>
#include <utility>
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

vector<pair<double, double> > destinations;
int currentDestinationIndex;

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
    void addDestination(double dx, double dy);
void clearDestinations();

vector<pair<double, double> > getDestinations() const;

bool hasMoreDestinations() const;
void moveToNextDestination();
void resetDestinationProgress();
    int getID() const;
    string getName() const;
    string getType() const;
    double getSpeed() const;
    bool setSpeed(double newSpeed);
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
