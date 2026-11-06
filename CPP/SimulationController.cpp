
#include "SimulationController.h"

#include <utility>

using namespace std;

SimulationController::SimulationController()
    : started(false),
      paused(false),
      status("not_configured"),
      errorMessage("")
{
}

// Configure the grid and its obstacles.
bool SimulationController::configureSimulation(
    int rows,
    int cols,
    const vector<pair<int, int>>& obstacles)
{
    lock_guard<mutex> lock(simulationMutex);

    if (rows < 2 || rows > 100 ||
        cols < 2 || cols > 100)
    {
        errorMessage = "Grid dimensions must be between 2 and 100.";
        return false;
    }

    // Destroy the old manager before its grid and robots.
    swarm.reset();
    robots.clear();
    grid.reset();

    started = false;
    paused = false;
    status = "configuring";
    errorMessage.clear();

    grid.reset(new GridMap(rows, cols));

    // Add all requested obstacles.
    for (size_t i = 0; i < obstacles.size(); i++)
    {
        int x = obstacles[i].first;
        int y = obstacles[i].second;

        if (!grid->addObstacle(x, y))
        {
            errorMessage = "Invalid obstacle position.";

            grid.reset();
            status = "error";

            return false;
        }
    }

    // Create the manager after the grid is ready.
    swarm.reset(new SwarmManager(*grid));

    // Disable console output in backend mode.
    swarm->setVerbose(false);

    status = "configured";

    return true;
}

// Register a robot and transfer ownership to this controller.
bool SimulationController::addRobot(unique_ptr<Robot> robot)
{
    lock_guard<mutex> lock(simulationMutex);

    if (!grid || !swarm)
    {
        errorMessage = "Configure the simulation first.";
        return false;
    }

    if (started)
    {
        errorMessage = "Cannot add robots after simulation starts.";
        return false;
    }

    if (!robot)
    {
        errorMessage = "Invalid robot.";
        return false;
    }

    Robot* robotPointer = robot.get();

    if (!swarm->addRobot(robotPointer))
    {
        errorMessage = "Robot registration failed.";
        return false;
    }

    robots.push_back(move(robot));

    errorMessage.clear();

    return true;
}

// Register a charging station.
bool SimulationController::addChargingStation(int x, int y)
{
    lock_guard<mutex> lock(simulationMutex);

    if (!grid || !swarm)
    {
        errorMessage = "Configure the simulation first.";
        return false;
    }

    if (started)
    {
        errorMessage = "Cannot add charging stations after simulation starts.";
        return false;
    }

    if (!swarm->addChargingStation(x, y))
    {
        errorMessage = "Charging station registration failed.";
        return false;
    }

    errorMessage.clear();

    return true;
}
// Start the simulation.
bool SimulationController::startSimulation()
{
    lock_guard<mutex> lock(simulationMutex);

    if (!swarm)
    {
        errorMessage = "No simulation has been configured.";
        return false;
    }

    if (swarm->getRobotCount() == 0)
    {
        errorMessage = "Add at least one robot before starting.";
        return false;
    }

    if (swarm->isFinished())
    {
        errorMessage = "Simulation has already finished. Reset it first.";
        return false;
    }

    started = true;
    paused = false;
    status = "running";
    errorMessage.clear();

    return true;
}

// Execute exactly one simulation step.
bool SimulationController::stepSimulation()
{
    lock_guard<mutex> lock(simulationMutex);

    if (!swarm || !started)
    {
        errorMessage = "Start the simulation first.";
        return false;
    }

    if (paused)
    {
        errorMessage = "Simulation is paused.";
        return false;
    }

    if (swarm->isFinished())
    {
        status = "finished";
        return false;
    }

    bool keepRunning = swarm->runSimulationStep();

    if (!keepRunning)
    {
        status = "finished";
    }

    errorMessage.clear();

    return keepRunning;
}

// Pause the simulation.
bool SimulationController::pauseSimulation()
{
    lock_guard<mutex> lock(simulationMutex);

    if (!started || !swarm)
    {
        errorMessage = "Simulation has not started.";
        return false;
    }

    if (swarm->isFinished())
    {
        errorMessage = "Simulation has already finished.";
        return false;
    }

    if (paused)
    {
        errorMessage = "Simulation is already paused.";
        return false;
    }

    paused = true;
    status = "paused";
    errorMessage.clear();

    return true;
}

// Resume the simulation.
bool SimulationController::resumeSimulation()
{
    lock_guard<mutex> lock(simulationMutex);

    if (!started || !swarm)
    {
        errorMessage = "Simulation has not started.";
        return false;
    }

    if (swarm->isFinished())
    {
        errorMessage = "Simulation has already finished.";
        return false;
    }

    if (!paused)
    {
        errorMessage = "Simulation is not paused.";
        return false;
    }

    paused = false;
    status = "running";
    errorMessage.clear();

    return true;
}

// Restore the initial robot positions and battery levels.
bool SimulationController::resetSimulation()
{
    lock_guard<mutex> lock(simulationMutex);

    if (!swarm)
    {
        errorMessage = "No simulation is configured.";
        return false;
    }

    swarm->resetSimulation();

    started = false;
    paused = false;
    status = "configured";
    errorMessage.clear();

    return true;
}

// Return a safe copy of all robot states.
vector<RobotState> SimulationController::getRobotStates() const
{
    lock_guard<mutex> lock(simulationMutex);

    if (!swarm)
    {
        return vector<RobotState>();
    }

    return swarm->getRobotStates();
}

// Return a safe copy of all robot movement paths.
vector<vector<pair<int, int>>> SimulationController::getRobotPaths() const
{
    lock_guard<mutex> lock(simulationMutex);

    if (!swarm)
    {
        return vector<vector<pair<int, int>>>();
    }

    return swarm->getRobotPaths();
}
// Return the current simulation status.
// Return a safe copy of all charging station positions.
vector<pair<int, int>> SimulationController::getChargingStations() const
{
    lock_guard<mutex> lock(simulationMutex);

    if (!swarm)
    {
        return vector<pair<int, int>>();
    }

    return swarm->getChargingStations();
}
// Return the grid row count.
int SimulationController::getGridRows() const
{
    lock_guard<mutex> lock(simulationMutex);

    if (!grid)
        return 0;

    return grid->getRows();
}

// Return the grid column count.
int SimulationController::getGridCols() const
{
    lock_guard<mutex> lock(simulationMutex);

    if (!grid)
        return 0;

    return grid->getCols();
}

// Return all obstacle coordinates.
vector<pair<int, int>> SimulationController::getObstacles() const
{
    lock_guard<mutex> lock(simulationMutex);

    vector<pair<int, int>> obstacles;

    if (!grid)
        return obstacles;

    for (int y = 0; y < grid->getRows(); y++)
    {
        for (int x = 0; x < grid->getCols(); x++)
        {
            if (grid->isObstacle(x, y))
            {
                obstacles.push_back(make_pair(x, y));
            }
        }
    }

    return obstacles;
}
string SimulationController::getStatus() const
{
    lock_guard<mutex> lock(simulationMutex);

    return status;
}

// Return the most recent error.
string SimulationController::getErrorMessage() const
{
    lock_guard<mutex> lock(simulationMutex);

    return errorMessage;
}

// Return the current simulation step.
int SimulationController::getCurrentStep() const
{
    lock_guard<mutex> lock(simulationMutex);

    if (!swarm)
    {
        return 0;
    }

    return swarm->getCurrentStep();
}

// Check whether the simulation has finished.
bool SimulationController::isFinished() const
{
    lock_guard<mutex> lock(simulationMutex);

    if (!swarm)
    {
        return false;
    }

    return swarm->isFinished();
}
