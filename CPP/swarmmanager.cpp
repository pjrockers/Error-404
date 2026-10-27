#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include "swarmmanager.h"
#include "pathfinder.h"

#include <thread>
#include <chrono>
#include <windows.h>

using namespace std;

SwarmManager::SwarmManager(GridMap& grid)
    : map(grid)
{
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

void SwarmManager::runSimulation()
{
    Pathfinder finder;

    //json writing 
    
    // Export current robot states for Godot.
    auto exportRobotState = [this]()
    {
        ofstream file("C:/roboSwarmBridge/robot_state.json");

        if (!file.is_open())
        {
            cerr << "ERROR: Could not write robot_state.json\n";
            return;
        }

        file << "{\n";
        file << "  \"grid\": {\n";
        file << "    \"rows\": " << map.getRows() << ",\n";
        file << "    \"cols\": " << map.getCols() << "\n";
        file << "  },\n";
        file << "  \"robots\": [\n";

        for (size_t i = 0; i < robots.size(); i++)
        {
            Robot* robot = robots[i];
            

            file << "    {\n";
            file << "      \"id\": " << robot->getID() << ",\n";
            file << "      \"x\": " << robot->getX() << ",\n";
            file << "      \"y\": " << robot->getY() << ",\n";
            file << "      \"dest_x\": " << robot->getDestX() << ",\n";
            file << "      \"dest_y\": " << robot->getDestY() << ",\n";
            file << "      \"battery\": " << robot->getBattery() << "\n";
            file << "    }";

            if (i + 1 < robots.size())
                file << ",";

            file << "\n";
        }

        file << "  ]\n";
        file << "}\n";

        file.close();
    };
// json ends

    cout << "\n===== STARTING SWARM SIMULATION =====\n";

    exportRobotState(); //json shit

    const int MAX_STEPS = 1000;
    int blockedRetryCount = 0;
    const int MAX_BLOCKED_RETRIES = 3;
        // Display a summary of the simulation.
    auto showSummary = [this]()
    {
        int arrived = 0;
        int waiting = 0;
        int lowBattery = 0;

        for (int i = 0;
             i < static_cast<int>(robots.size()); i++)
        {
            Robot* robot = robots[i];

            int x = static_cast<int>(robot->getX());
            int y = static_cast<int>(robot->getY());
            int dx = static_cast<int>(robot->getDestX());
            int dy = static_cast<int>(robot->getDestY());

            if (x == dx && y == dy)
            {
                arrived++;
            }
            else if (!robot->hasBattery())
            {
                lowBattery++;
            }
            else if (robot->getStatus() == "Waiting")
            {
                waiting++;
            }
        }

        cout << "\n===== SIMULATION SUMMARY =====\n";
        cout << "Total robots: " << robots.size() << endl;
        cout << "Arrived: " << arrived << endl;
        cout << "Waiting/blocked: " << waiting << endl;
        cout << "Low battery: " << lowBattery << endl;
    };

    for (int step = 1; step <= MAX_STEPS; step++)
    {
        bool allFinished = true;
        bool anyMoved = false;

        
vector<Robot*> order = robots;

// Calculate priority using the robot's original priority
// and how long it has been waiting.
auto effectivePriority = [this](Robot* robot)
{
    int index = 0;

    while (index < static_cast<int>(robots.size()) &&
           robots[index] != robot)
    {
        index++;
    }

    if (index >= static_cast<int>(robots.size()))
    {
        return robot->getPriority();
    }

    int waitingBonus = waitCounters[index] / 3;

    return robot->getPriority() + waitingBonus;
};

sort(order.begin(), order.end(),
    [this, &effectivePriority](Robot* a, Robot* b)
    {
        int priorityA = effectivePriority(a);
        int priorityB = effectivePriority(b);

        if (priorityA != priorityB)
        {
            return priorityA > priorityB;
        }

        // If effective priorities are equal,
        // prefer the robot with the higher original priority.
        return a->getPriority() > b->getPriority();
    });


        cout << "\n--- Simulation Step "
             << step << " ---\n";

        for (int i = 0; i < static_cast<int>(order.size()); i++)
        {
            Robot* robot = order[i];
            int robotIndex = 0;

while (robotIndex < static_cast<int>(robots.size()) &&
       robots[robotIndex] != robot)
{
    robotIndex++;
}

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

                cout << "Robot " << robot->getID()
                     << " cannot move: battery depleted.\n";
                continue;
            }

            vector<pair<int, int> > temporaryObstacles;

            // Temporarily block other robots, except when one
            // occupies this robot's destination. Keeping that
            // destination walkable lets BFS find a route to it.
            for (int j = 0;
                 j < static_cast<int>(robots.size()); j++)
            {
                Robot* other = robots[j];

                if (other == robot)
                    continue;

                int otherX =
                    static_cast<int>(other->getX());
                int otherY =
                    static_cast<int>(other->getY());

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

            // Remove only temporary obstacles added above.
            for (int j = 0;
                 j < static_cast<int>(
                     temporaryObstacles.size()); j++)
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
    cout << "Warning: Robot " << robot->getID()
         << " has been waiting for "
         << waitCounters[robotIndex]
         << " consecutive steps.\n";
}

                cout << "Robot " << robot->getID()
                     << " is waiting: no usable route "
                     << "is currently available.\n";
                continue;
            }

            int nextX = path[1].first;
            int nextY = path[1].second;

            // Never enter a cell occupied by another robot.
            if (isCellOccupied(
                    robots, nextX, nextY, robot))
            {
                robot->setStatus("Waiting");
                waitCounters[robotIndex]++;
                if (waitCounters[robotIndex] >= 3)
{
    cout << "Warning: Robot " << robot->getID()
         << " has been waiting for "
         << waitCounters[robotIndex]
         << " consecutive steps.\n";
}

                cout << "Robot " << robot->getID()
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

                cout << "Battery remaining: "
                     << robot->getBattery() << "%\n";

                cout << "Robot " << robot->getID()
                     << " moved to ("
                     << nextX << ", " << nextY << ")\n";

                if (nextX == dx && nextY == dy)
                {
                    robot->setStatus("Arrived");
                }
                else
                {
                    robot->setStatus("Moving");
                }
            }
            else
            {
                robot->setStatus("Waiting");
                waitCounters[robotIndex]++;
                if (waitCounters[robotIndex] >= 3)
{
    cout << "Warning: Robot " << robot->getID()
         << " has been waiting for "
         << waitCounters[robotIndex]
         << " consecutive steps.\n";
}

                cout << "Robot " << robot->getID()
                     << " could not move to the next cell.\n";
            }
        }
        exportRobotState();//json 
        Sleep(500); // json 
        if (allFinished)
        {
            cout << "\nAll robots reached their destinations!\n";
             showSummary();
            return;
        }

        displayCurrentMap();

        
if (!anyMoved)
{
    cout << "\nNo robot moved during this step.\n";

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
        cout << "No unfinished robot has enough battery to move.\n";
        showSummary();
        return;
    }

    blockedRetryCount++;

    if (blockedRetryCount <= MAX_BLOCKED_RETRIES)
    {
        cout << "Robots may be temporarily blocked.\n";
        cout << "Retry attempt: "
             << blockedRetryCount
             << " of "
             << MAX_BLOCKED_RETRIES
             << "\n";

        continue;
    }

    cout << "Deadlock detected: no robot could move after "
         << MAX_BLOCKED_RETRIES
         << " retry attempts.\n";

    showSummary();
    return;
}
else
{
    blockedRetryCount = 0;
}

    }

    cout << "\nMaximum simulation steps reached.\n";
    showSummary();
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