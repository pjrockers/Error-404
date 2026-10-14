
#ifndef SWARMMANAGER_H
#define SWARMMANAGER_H

#include <vector>
#include "robot.h"
#include "gridmap.h"

class SwarmManager
{
private:
    GridMap& map;
    vector<Robot*> robots;

public:
    SwarmManager(GridMap& grid);

    bool addRobot(Robot* robot);
    void displayRobots() const;

    int getRobotCount() const;
    void runSimulation();
    void displayCurrentMap() const;
};

#endif
