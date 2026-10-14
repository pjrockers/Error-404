#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include "swarmmanager.h"
#include "pathfinder.h"

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

    int x = static_cast<int>(robot->getX());
    int y = static_cast<int>(robot->getY());

    int dx = static_cast<int>(robot->getDestX());
    int dy = static_cast<int>(robot->getDestY());

    // Check that start and destination are walkable.
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

    // Reject robots that share a starting cell.
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

void SwarmManager::runSimulation()
{
    Pathfinder finder;

    cout << "\n===== STARTING SWARM SIMULATION =====\n";

    const int MAX_STEPS = 1000;

    for (int step = 1; step <= MAX_STEPS; step++)
    {
        bool allFinished = true;
        bool anyMoved = false;

        // Process robots with higher priority first.
        vector<Robot*> order = robots;

        sort(order.begin(), order.end(),
            [](const Robot* a, const Robot* b)
            {
                return a->getPriority() > b->getPriority();
            });

        cout << "\n--- Simulation Step "
             << step << " ---\n";

        for (int i = 0;
             i < static_cast<int>(order.size()); i++)
        {
            Robot* robot = order[i];

            int x = static_cast<int>(robot->getX());
            int y = static_cast<int>(robot->getY());

            int dx = static_cast<int>(robot->getDestX());
            int dy = static_cast<int>(robot->getDestY());

            // This robot has reached its destination.
            if (x == dx && y == dy)
{
    robot->setStatus("Arrived");
    continue;
}
if (!robot->hasBattery())
{
    robot->setStatus("Low Battery");

    cout << "Robot " << robot->getID()
         << " cannot move: battery depleted.\n";

    continue;
}

            allFinished = false;

vector<pair<int, int> > temporaryObstacles;

// Temporarily treat other robots as obstacles.
for (int j = 0; j < static_cast<int>(robots.size()); j++)
{
    Robot* other = robots[j];

    if (other == robot)
        continue;

    int otherX = static_cast<int>(other->getX());
    int otherY = static_cast<int>(other->getY());

    // Record only cells that we successfully block.
    if (map.addObstacle(otherX, otherY))
    {
        temporaryObstacles.push_back(
            make_pair(otherX, otherY));
    }
}

// Calculate a route around other robots.
vector<pair<int, int> > path =
    finder.findPath(map, x, y, dx, dy);

// Remove only the temporary obstacles we added.
for (int j = 0;
     j < static_cast<int>(temporaryObstacles.size()); j++)
{
    map.removeObstacle(
        temporaryObstacles[j].first,
        temporaryObstacles[j].second);
}


            // No route, or already at the destination.
            if (path.size() < 2)
            {
                cout << "Robot " << robot->getID()
                     << " cannot find a usable next step.\n";
                continue;
            }

            int nextX = path[1].first;
            int nextY = path[1].second;
            robot->setStatus("Moving");
            // Never enter a cell occupied by another robot.
            if (isCellOccupied(
                    robots, nextX, nextY, robot))
            {
                robot->setStatus("Waiting");
                cout << "Robot " << robot->getID()
                     << " is waiting: next cell is occupied.\n";
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

    if (robot->getX() == robot->getDestX() &&
        robot->getY() == robot->getDestY())
    {
        robot->setStatus("Arrived");
    }
}

        }
        
        if (allFinished)
        {
            cout << "\nAll robots reached their destinations!\n";
            return;
        }

        // Stop instead of running forever if nobody can move.
        if (!anyMoved)
        {
            cout << "\nSimulation paused: no robot could move.\n";
            cout << "Some robots may be blocked or have no route.\n";
            return;
        }
        displayCurrentMap();
    }

    cout << "\nMaximum simulation steps reached.\n";
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