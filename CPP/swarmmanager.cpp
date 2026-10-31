#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
// Portable sleep (does not need <thread>, which old MinGW builds lack).
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif
#include "swarmmanager.h"
#include "pathfinder.h"
using std::vector;
using std::string;
using std::ostream;
using namespace std;

namespace
{
    bool isCellOccupied(
        const vector<Robot*>& robots,
        int x, int y,
        const Robot* currentRobot)
    {
        for (int i = 0; i < static_cast<int>(robots.size()); i++)
        {
            if (robots[i] != currentRobot &&
                static_cast<int>(robots[i]->getX()) == x &&
                static_cast<int>(robots[i]->getY()) == y)
            {
                return true;
            }
        }

        return false;
    }
}

SwarmManager::SwarmManager(GridMap& grid)
    : map(grid),
      currentStep(0),
      blockedRetryCount(0),
      simulationFinished(false),
      finishReason(NOT_FINISHED),
      verbose(true),
      godotExportPath("C:/roboSwarmBridge/robot_state.json"),
      nullStream(nullptr)
{
}

ostream& SwarmManager::log() const
{
    if (verbose)
        return cout;

    return nullStream;
}

void SwarmManager::setVerbose(bool value)
{
    verbose = value;
}

void SwarmManager::setGodotExportPath(const string& path)
{
    godotExportPath = path;
}

bool SwarmManager::addRobot(Robot* robot)
{
    if (robot == NULL)
    {
        cout << "Invalid robot.\n";
        return false;
    }

    // Prevent two robots from having the same destination.
    for (size_t i = 0; i < robots.size(); i++)
    {
        if (robots[i]->getDestX() == robot->getDestX() &&
            robots[i]->getDestY() == robot->getDestY())
        {
            cout << "Error: Destination ("
                 << robot->getDestX() << ", "
                 << robot->getDestY()
                 << ") is already assigned to Robot "
                 << robots[i]->getID() << ".\n";

            return false;
        }
    }

    int x = static_cast<int>(robot->getX());
    int y = static_cast<int>(robot->getY());
    int dx = static_cast<int>(robot->getDestX());
    int dy = static_cast<int>(robot->getDestY());

    if (!map.isWalkable(x, y))
    {
        cout << "Robot " << robot->getID()
             << " has an invalid starting position.\n";
        return false;
    }

    if (!map.isWalkable(dx, dy))
    {
        cout << "Robot " << robot->getID()
             << " has an invalid destination.\n";
        return false;
    }

    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        if (robots[i]->getX() == robot->getX() &&
            robots[i]->getY() == robot->getY())
        {
            cout << "Another robot already occupies this cell.\n";
            return false;
        }

        if (robots[i]->getID() == robot->getID())
        {
            cout << "Robot ID already exists.\n";
            return false;
        }
    }

    robots.push_back(robot);
    waitCounters.push_back(0);

    // Remember the start state so resetSimulation() can restore it.
    InitialState initial;
    initial.x = robot->getX();
    initial.y = robot->getY();
    initial.battery = robot->getBattery();
    initial.status = robot->getStatus();
    initialStates.push_back(initial);

    cout << robot->getStatus()
         << " robot registered successfully. ID: "
         << robot->getID() << endl;

    return true;
}

void SwarmManager::displayRobots() const
{
    cout << "\n===== REGISTERED ROBOTS =====\n";

    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        robots[i]->displayInfo();
    }
}

int SwarmManager::getRobotCount() const
{
    return static_cast<int>(robots.size());
}

vector<RobotState> SwarmManager::getRobotStates() const
{
    vector<RobotState> states;

    for (size_t i = 0; i < robots.size(); i++)
    {
        Robot* robot = robots[i];

        RobotState state;

        state.id = robot->getID();
        state.name = robot->getName();
        state.type = robot->getType();

        state.x = robot->getX();
        state.y = robot->getY();

        state.destX = robot->getDestX();
        state.destY = robot->getDestY();

        state.battery = robot->getBattery();
        state.speed = robot->getSpeed();

        state.priority = robot->getPriority();
        state.status = robot->getStatus();

        states.push_back(state);
    }

    return states;
}

int SwarmManager::indexOf(const Robot* robot) const
{
    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        if (robots[i] == robot)
            return i;
    }

    return -1;
}

// Original priority plus a bonus for every 3 consecutive waiting steps.
int SwarmManager::effectivePriority(const Robot* robot) const
{
    int index = indexOf(robot);

    if (index < 0)
        return robot->getPriority();

    return robot->getPriority() + waitCounters[index] / 3;
}

void SwarmManager::finish(FinishReason reason)
{
    simulationFinished = true;
    finishReason = reason;
}

bool SwarmManager::isFinished() const
{
    return simulationFinished;
}

int SwarmManager::getCurrentStep() const
{
    return currentStep;
}

SwarmManager::FinishReason SwarmManager::getFinishReason() const
{
    return finishReason;
}

string SwarmManager::getFinishReasonText() const
{
    switch (finishReason)
    {
        case ALL_ARRIVED:       return "all_arrived";
        case NO_BATTERY:        return "no_battery";
        case DEADLOCK:          return "deadlock";
        case MAX_STEPS_REACHED: return "max_steps";
        default:                return "";
    }
}

void SwarmManager::resetSimulation()
{
    for (size_t i = 0; i < robots.size(); i++)
    {
        robots[i]->setPosition(initialStates[i].x, initialStates[i].y);
        robots[i]->setBattery(initialStates[i].battery);
        robots[i]->setStatus(initialStates[i].status);
        waitCounters[i] = 0;
    }

    currentStep = 0;
    blockedRetryCount = 0;
    simulationFinished = false;
    finishReason = NOT_FINISHED;
}

void SwarmManager::printSummary() const
{
    int arrived = 0;
    int waiting = 0;
    int lowBattery = 0;

    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        Robot* robot = robots[i];

        int x = static_cast<int>(robot->getX());
        int y = static_cast<int>(robot->getY());
        int dx = static_cast<int>(robot->getDestX());
        int dy = static_cast<int>(robot->getDestY());

        if (x == dx && y == dy)
            arrived++;
        else if (!robot->hasBattery())
            lowBattery++;
        else if (robot->getStatus() == "Waiting")
            waiting++;
    }

    log() << "\n===== SIMULATION SUMMARY =====\n";
    log() << "Total robots: " << robots.size() << endl;
    log() << "Arrived: " << arrived << endl;
    log() << "Waiting/blocked: " << waiting << endl;
    log() << "Low battery: " << lowBattery << endl;
}

// Writes the state file that Godot reads.
void SwarmManager::exportGodotState(
    const string& status,
    const string& errorMessage) const
{
    if (godotExportPath.empty())
        return;

    ofstream file(godotExportPath.c_str());

    if (!file.is_open())
    {
        cerr << "ERROR: Could not write " << godotExportPath << "\n";
        return;
    }

    file << "{\n";
    file << "  \"simulation_status\": \"" << status << "\",\n";
    file << "  \"error_message\": \"" << errorMessage << "\",\n";
    file << "  \"step\": " << currentStep << ",\n";
    file << "  \"grid\": {\n";
    file << "    \"rows\": " << map.getRows() << ",\n";
    file << "    \"cols\": " << map.getCols() << "\n";
    file << "  },\n";
    file << "  \"obstacles\": [\n";

    bool firstObstacle = true;

    for (int y = 0; y < map.getRows(); y++)
    {
        for (int x = 0; x < map.getCols(); x++)
        {
            if (map.isObstacle(x, y))
            {
                if (!firstObstacle)
                    file << ",\n";

                file << "    {\"x\": " << x
                     << ", \"y\": " << y << "}";

                firstObstacle = false;
            }
        }
    }

    file << "\n  ],\n";
    file << "  \"robots\": [\n";

    for (size_t i = 0; i < robots.size(); i++)
    {
        Robot* robot = robots[i];

        file << "    {\n";
        file << "      \"id\": " << robot->getID() << ",\n";
        file << "      \"x\": " << robot->getX() << ",\n";
        file << "      \"y\": " << robot->getY() << ",\n";
        file << "      \"start_x\": " << initialStates[i].x << ",\n";
        file << "      \"start_y\": " << initialStates[i].y << ",\n";
        file << "      \"dest_x\": " << robot->getDestX() << ",\n";
        file << "      \"dest_y\": " << robot->getDestY() << ",\n";
        file << "      \"battery\": " << robot->getBattery() << ",\n";
        file << "      \"status\": \"" << robot->getStatus() << "\"\n";
        file << "    }";

        if (i + 1 < robots.size())
            file << ",";

        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
}

// Runs ONE simulation step. Returns true while the simulation
// should keep going, false once it has finished.
bool SwarmManager::runSimulationStep()
{
    if (simulationFinished)
        return false;

    if (currentStep >= MAX_STEPS)
    {
        log() << "\nMaximum simulation steps reached.\n";
        finish(MAX_STEPS_REACHED);
        return false;
    }

    currentStep++;

    Pathfinder finder;

    bool allFinished = true;
    bool anyMoved = false;

    // Highest effective priority moves first.
    vector<Robot*> order = robots;

    sort(order.begin(), order.end(),
        [this](Robot* a, Robot* b)
        {
            int priorityA = effectivePriority(a);
            int priorityB = effectivePriority(b);

            if (priorityA != priorityB)
                return priorityA > priorityB;

            if (a->getPriority() != b->getPriority())
                return a->getPriority() > b->getPriority();

            // Consistent tie-breaker.
            return a->getID() < b->getID();
        });

    log() << "\n--- Simulation Step "
          << currentStep << " ---\n";

    for (int i = 0; i < static_cast<int>(order.size()); i++)
    {
        Robot* robot = order[i];
        int robotIndex = indexOf(robot);

        int x = static_cast<int>(robot->getX());
        int y = static_cast<int>(robot->getY());
        int dx = static_cast<int>(robot->getDestX());
        int dy = static_cast<int>(robot->getDestY());

        // Already arrived.
        if (x == dx && y == dy)
        {
            robot->setStatus("Arrived");
            continue;
        }

        // This robot is not finished yet.
        allFinished = false;

        if (!robot->hasBattery())
        {
            robot->setStatus("Low Battery");

            log() << "Robot " << robot->getID()
                  << " cannot move: battery depleted.\n";
            continue;
        }

        vector<pair<int, int> > temporaryObstacles;

        // Temporarily block other robots, except when one
        // occupies this robot's destination. Keeping that
        // destination walkable lets BFS find a route to it.
        for (int j = 0; j < static_cast<int>(robots.size()); j++)
        {
            Robot* other = robots[j];

            if (other == robot)
                continue;

            int otherX = static_cast<int>(other->getX());
            int otherY = static_cast<int>(other->getY());

            if (otherX == dx && otherY == dy)
                continue;

            if (map.addObstacle(otherX, otherY))
            {
                temporaryObstacles.push_back(
                    make_pair(otherX, otherY));
            }
        }

        vector<pair<int, int> > path =
            finder.findPath(map, x, y, dx, dy);

        // Remove only the temporary obstacles added above.
        for (int j = 0;
             j < static_cast<int>(temporaryObstacles.size()); j++)
        {
            map.removeObstacle(
                temporaryObstacles[j].first,
                temporaryObstacles[j].second);
        }

        if (path.size() < 2)
        {
            robot->setStatus("Waiting");
            waitCounters[robotIndex]++;

            if (waitCounters[robotIndex] >= 3)
            {
                log() << "WARNING: Robot " << robot->getID()
                      << " may be deadlocked after waiting for "
                      << waitCounters[robotIndex]
                      << " consecutive steps.\n";
            }

            log() << "Robot " << robot->getID()
                  << " is waiting: no usable route "
                  << "is currently available.\n";
            continue;
        }

        int nextX = path[1].first;
        int nextY = path[1].second;

        // Safety check: the next cell must be inside the grid
        // and must not contain a real obstacle.
        if (!map.isWalkable(nextX, nextY))
        {
            robot->setStatus("Waiting");
            waitCounters[robotIndex]++;

            log() << "Robot " << robot->getID()
                  << " blocked: next cell ("
                  << nextX << ", " << nextY
                  << ") is outside the grid or contains an obstacle.\n";
            continue;
        }

        // Never enter a cell occupied by another robot.
        if (isCellOccupied(robots, nextX, nextY, robot))
        {
            robot->setStatus("Waiting");
            waitCounters[robotIndex]++;

            if (waitCounters[robotIndex] >= 3)
            {
                log() << "Warning: Robot " << robot->getID()
                      << " has been waiting for "
                      << waitCounters[robotIndex]
                      << " consecutive steps.\n";
            }

            log() << "Robot " << robot->getID()
                  << " is waiting: next cell ("
                  << nextX << ", " << nextY
                  << ") is occupied.\n";
            continue;
        }

        if (robot->moveToNextCell(nextX, nextY))
        {
            anyMoved = true;
            waitCounters[robotIndex] = 0;
            robot->consumeBattery(1);

            log() << "Battery remaining: "
                  << robot->getBattery() << "%\n";

            log() << "Robot " << robot->getID()
                  << " moved to ("
                  << nextX << ", " << nextY << ")\n";

            if (nextX == dx && nextY == dy)
                robot->setStatus("Arrived");
            else
                robot->setStatus("Moving");
        }
        else
        {
            robot->setStatus("Waiting");
            waitCounters[robotIndex]++;

            if (waitCounters[robotIndex] >= 3)
            {
                log() << "Warning: Robot " << robot->getID()
                      << " has been waiting for "
                      << waitCounters[robotIndex]
                      << " consecutive steps.\n";
            }

            log() << "Robot " << robot->getID()
                  << " could not move to the next cell.\n";
        }
    }

    if (allFinished)
    {
        log() << "\nAll robots reached their destinations!\n";
        finish(ALL_ARRIVED);
        return false;
    }

    if (verbose)
        displayCurrentMap();

    if (anyMoved)
    {
        blockedRetryCount = 0;
        return true;
    }

    // Nobody moved during this step.
    log() << "\nNo robot moved during this step.\n";

    bool unfinishedRobotHasBattery = false;

    for (size_t i = 0; i < robots.size(); i++)
    {
        Robot* robot = robots[i];

        bool arrived =
            static_cast<int>(robot->getX()) ==
                static_cast<int>(robot->getDestX()) &&
            static_cast<int>(robot->getY()) ==
                static_cast<int>(robot->getDestY());

        if (!arrived && robot->hasBattery())
        {
            unfinishedRobotHasBattery = true;
            break;
        }
    }

    if (!unfinishedRobotHasBattery)
    {
        log() << "No unfinished robot has enough battery to move.\n";
        finish(NO_BATTERY);
        return false;
    }

    blockedRetryCount++;

    if (blockedRetryCount <= MAX_BLOCKED_RETRIES)
    {
        log() << "Robots may be temporarily blocked.\n";
        log() << "Retry attempt: "
              << blockedRetryCount
              << " of "
              << MAX_BLOCKED_RETRIES
              << "\n";
        return true;
    }

    log() << "Deadlock detected: no robot could move after "
          << MAX_BLOCKED_RETRIES
          << " retry attempts.\n";

    finish(DEADLOCK);
    return false;
}

// Console mode: same behaviour as the old monolithic function.
void SwarmManager::runSimulation()
{
    log() << "\n===== STARTING SWARM SIMULATION =====\n";

    exportGodotState("running", "");

    bool keepGoing = true;

    while (keepGoing)
    {
        keepGoing = runSimulationStep();

        exportGodotState("running", "");

        // Keep the existing visual update interval.
        if (verbose || !godotExportPath.empty())
            SLEEP_MS(500);
    }

    exportGodotState("finished", "");
    printSummary();
}

void SwarmManager::displayCurrentMap() const
{
    vector<pair<int, int> > positions;
    vector<pair<int, int> > destinations;
    vector<int> ids;

    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        positions.push_back(make_pair(
            static_cast<int>(robots[i]->getX()),
            static_cast<int>(robots[i]->getY())
        ));

        destinations.push_back(make_pair(
            static_cast<int>(robots[i]->getDestX()),
            static_cast<int>(robots[i]->getDestY())
        ));

        ids.push_back(robots[i]->getID());
    }

    map.displayMap(positions, destinations, ids);
}