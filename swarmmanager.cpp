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

// godot function which waits for the start button to be pressed

bool waitForGodotStart()
{
    const string commandPath =
        "C:/roboSwarmBridge/command.txt";

    cout << "\nInitial state exported to Godot."
         << "\nPress START in Godot."
         << "\nWaiting for command...\n";

    while (true)
    {
        ifstream file(commandPath.c_str());

        if (file.is_open())
        {
            string command;
            getline(file, command);
            file.close();

            // Remove a possible Windows carriage return.
            if (!command.empty() && command.back() == '\r')
                command.pop_back();

            cout << "Command read: [" << command << "]\n";

            if (command == "START")
            {
                ofstream clearFile(commandPath.c_str(),
                                   ios::trunc);
                clearFile.close();

                cout << "START received. Running simulation!\n";
                return true;
            }
        }
        else
        {
            cout << "Cannot open command.txt\n";
        }

        Sleep(500);
    }
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
        


        // Export obstacle positions for Godot
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
        // obstacle ending godot 


file << "\n  ],\n";

        file << "  \"robots\": [\n";

        for (size_t i = 0; i < robots.size(); i++)
        {
            Robot* robot = robots[i];
            file << "    {\n";
            file << "      \"id\": " << robot->getID() << ",\n";
            file << "      \"x\": " << robot->getX() << ",\n";
            file << "      \"y\": " << robot->getY() << ",\n";
            file << "      \"start_x\": 0,\n";
            file << "      \"start_y\": 0,\n";
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
    // Wait until the user presses START in Godot.
    waitForGodotStart();
    const int MAX_STEPS = 1000;

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

        // Higher-priority robots move first.
        sort(order.begin(), order.end(),
            [](const Robot* a, const Robot* b)
            {
                return a->getPriority() > b->getPriority();
            });

        cout << "\n--- Simulation Step "
             << step << " ---\n";

        for (int i = 0; i < static_cast<int>(order.size()); i++)
        {
            Robot* robot = order[i];

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
            for (int j = 0; j < static_cast<int>(robots.size()); j++)
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

            vector<pair<int, int> > path = finder.findPath(map, x, y, dx, dy);

            // Remove only temporary obstacles added above.
            for (int j = 0; j < static_cast<int>(temporaryObstacles.size()); j++)
            {
                map.removeObstacle(
                    temporaryObstacles[j].first,
                    temporaryObstacles[j].second);
            }

            if (path.size() < 2)
            {
                robot->setStatus("Waiting");

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

                cout << "Robot " << robot->getID()
                     << " is waiting: next cell ("
                     << nextX << ", " << nextY
                     << ") is occupied.\n";
                continue;
            }

            if (robot->moveToNextCell(nextX, nextY))
            {
                anyMoved = true;
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

                cout << "Robot " << robot->getID()
                     << " could not move to the next cell.\n";
            }
        }
        exportRobotState();//json 
        Sleep(100); // json 
        if (allFinished)
        {
            cout << "\nAll robots reached their destinations!\n";
             showSummary();
            return;
        }

        displayCurrentMap();

        if (!anyMoved)
        {
            cout << "\nSimulation paused: no robot could move.\n";
            cout << "Unfinished robots may be blocked, "
                 << "out of battery, or unable to reach "
                 << "their destinations.\n";
            return;
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
