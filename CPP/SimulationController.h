
#ifndef SIMULATIONCONTROLLER_H
#define SIMULATIONCONTROLLER_H

#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <utility>
#include "swarmmanager.h"
#include "gridmap.h"
#include "robot.h"
#include "robotstate.h"

class SimulationController
{
private:
    // Owns the robot objects.
    std::vector<std::unique_ptr<Robot>> robots;

    // Owns the grid and simulation manager.
    std::unique_ptr<GridMap> grid;
    std::unique_ptr<SwarmManager> swarm;

    // Protects simulation data from simultaneous access.
    mutable std::mutex simulationMutex;

    bool started;
    bool paused;

    std::string status;
    std::string errorMessage;

public:
    SimulationController();

    // Configure a new simulation.
    bool configureSimulation(
        int rows,
        int cols,
        const std::vector<std::pair<int, int>>& obstacles
    );

    // Add a robot to the simulation.
    bool addRobot(std::unique_ptr<Robot> robot);
    // Add a charging station to the simulation.
bool addChargingStation(int x, int y);

    // Simulation controls.
    bool startSimulation();
    bool stepSimulation();
    bool pauseSimulation();
    bool resumeSimulation();
    bool resetSimulation();

    // Retrieve simulation information.
// Retrieve simulation information.
std::vector<RobotState> getRobotStates() const;

std::vector<std::vector<std::pair<int, int>>> getRobotPaths() const;
std::vector<std::pair<int, int>> getChargingStations() const;

// Grid information for simulation replay.
int getGridRows() const;
int getGridCols() const;
std::vector<std::pair<int, int>> getObstacles() const;
    std::string getStatus() const;
    std::string getErrorMessage() const;
    int getCurrentStep() const;
    bool isFinished() const;
};

#endif
