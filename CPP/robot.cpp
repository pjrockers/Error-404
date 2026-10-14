#include <iostream>
#include "robot.h"

using namespace std;

Robot::Robot(int robotID, string robotName, string robotType)
{
    id = robotID;
    name = robotName;
    type = robotType;

    x = 0;
    y = 0;

    destX = 0;
    destY = 0;

    battery = 100;
    speed = 5;
    priority = 1;
    status = "Idle";
}

void Robot::displayInfo()
{
    cout << "\n--- Robot Information ---\n";

    cout << "ID: " << id << endl;
    cout << "Name: " << name << endl;
    cout << "Type: " << type << endl;

    cout << "Position: (" << x << ", " << y << ")\n";
    cout << "Destination: (" << destX << ", " << destY << ")\n";

    cout << "Battery: " << battery << "%\n";
    cout << "Speed: " << speed << endl;
    cout << "Priority: " << priority << endl;
    cout << "Status: " << status << endl;
}

Robot::~Robot()
{
}

void Robot::setPriority(int p)
{
    if (p >= 1 && p <= 4)
        priority = p;
    else
        cout << "Invalid priority! Enter a value from 1 to 4.\n";
}

int Robot::getPriority() const
{
    return priority;
}

void Robot::setDestination(double dx, double dy)
{
    destX = dx;
    destY = dy;
    status = "Moving";
}

int Robot::getID() const
{
    return id;
}

double Robot::getBattery() const
{
    return battery;
}

string Robot::getStatus() const
{
    return status;
}

bool Robot::setPosition(double px, double py)
{
    // Grid coordinates must be whole numbers.
    if (px < 0 || py < 0 ||
        px != static_cast<int>(px) ||
        py != static_cast<int>(py))
    {
        return false;
    }

    x = px;
    y = py;
    return true;
}

double Robot::getX() const
{
    return x;
}

double Robot::getY() const
{
    return y;
}

double Robot::getDestX() const
{
    return destX;
}

double Robot::getDestY() const
{
    return destY;
}
bool Robot::moveToNextCell(int nextX, int nextY)
{
    int distanceX = nextX - static_cast<int>(x);
    int distanceY = nextY - static_cast<int>(y);

    // A robot may move only one cell horizontally
    // or vertically at a time.
    if (distanceX < -1 || distanceX > 1 ||
        distanceY < -1 || distanceY > 1)
    {
        return false;
    }

    // Do not allow diagonal movement.
    if (distanceX != 0 && distanceY != 0)
    {
        return false;
    }

    if (distanceX == 0 && distanceY == 0)
    {
        return true;
    }

    x = nextX;
    y = nextY;

    return true;
}
void Robot::setStatus(string newStatus)
{
    status = newStatus;
}

void Robot::consumeBattery(double amount)
{
    if (amount <= 0)
        return;

    battery -= amount;

    if (battery < 0)
        battery = 0;
}

bool Robot::hasBattery() const
{
    return battery > 0;
}
bool Robot::setBattery(double amount)
{
    if (amount < 0 || amount > 100)
        return false;

    battery = amount;
    return true;
}
