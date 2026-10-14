
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <limits>

#include "robot.h"
#include "deliveryrobot.h"
#include "rescuerobot.h"
#include "inspectionrobot.h"
#include "gridmap.h"
#include "swarmmanager.h"
#include "pathfinder.h"

using namespace std;

int readInt(string prompt)
{
    int value;

    while (true)
    {
        cout << prompt;

        if (cin >> value)
            return value;

        cout << "Invalid input. Enter a whole number.\n";

        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

double readDouble(string prompt)
{
    double value;

    while (true)
    {
        cout << prompt;

        if (cin >> value)
            return value;

        cout << "Invalid input. Enter a number.\n";

        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int readBoundedInt(string prompt, int minimum, int maximum)
{
    while (true)
    {
        int value = readInt(prompt);

        if (value >= minimum && value <= maximum)
            return value;

        cout << "Enter a value between "
             << minimum << " and " << maximum << ".\n";
    }
}

pair<int, int> readWalkableCell(
    const GridMap& map,
    string prompt)
{
    while (true)
    {
        cout << prompt << endl;

        int x = readInt("  X coordinate: ");
        int y = readInt("  Y coordinate: ");

        if (map.isWalkable(x, y))
            return make_pair(x, y);

        cout << "Invalid cell. It is outside the map "
             << "or contains an obstacle.\n";
    }
}

bool validateRobotRoutes(
    const GridMap& map,
    const vector<Robot*>& robots)
{
    Pathfinder finder;
    bool allValid = true;

    cout << "\n===== ROUTE VALIDATION =====\n";

    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        Robot* robot = robots[i];

        vector<pair<int, int> > path =
            finder.findPath(
                map,
                static_cast<int>(robot->getX()),
                static_cast<int>(robot->getY()),
                static_cast<int>(robot->getDestX()),
                static_cast<int>(robot->getDestY())
            );

        if (path.empty())
        {
            cout << "Robot " << robot->getID()
                 << " (" << robot->getStatus()
                 << "): NO ROUTE FOUND.\n";

            allValid = false;
        }
        else
        {
            cout << "Robot " << robot->getID()
                 << ": Route available. "
                 << "Moves required: " << path.size() - 1
                 << endl;
        }
    }

    return allValid;
}

void displayRobotGrid(
    const GridMap& map,
    const vector<Robot*>& robots)
{
    vector<pair<int, int> > positions;
    vector<pair<int, int> > destinations;
    vector<int> ids;

    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        positions.push_back(make_pair(
            static_cast<int>(robots[i]->getX()),
            static_cast<int>(robots[i]->getY())));

        destinations.push_back(make_pair(
            static_cast<int>(robots[i]->getDestX()),
            static_cast<int>(robots[i]->getDestY())));

        ids.push_back(robots[i]->getID());
    }

    map.displayMap(positions, destinations, ids);
}

int main()
{
    cout << "====================================\n";
    cout << "       ROBOT SWARM SIMULATOR\n";
    cout << "====================================\n";

    // 1. Configure the grid.
    int rows = readBoundedInt(
        "Enter grid rows (2-100): ", 2, 100);

    int cols = readBoundedInt(
        "Enter grid columns (2-100): ", 2, 100);

    GridMap map(rows, cols);

    // 2. Configure obstacles.
    int obstacleCount = readBoundedInt(
        "Enter number of obstacles (0 to " +
        to_string(rows * cols - 1) + "): ",
        0, rows * cols - 1);

    for (int i = 0; i < obstacleCount; i++)
    {
        cout << "\nObstacle " << i + 1 << endl;

        int x = readInt("  X coordinate: ");
        int y = readInt("  Y coordinate: ");

        if (!map.addObstacle(x, y))
        {
            cout << "Invalid or duplicate obstacle. Try again.\n";
            i--;
        }
        else
        {
            cout << "Obstacle added.\n";
        }
    }

    cout << "\n===== CONFIGURED MAP =====\n";
    map.displayMap();

    // 3. Configure the number of robots.
    int robotCount = readBoundedInt(
        "\nHow many robots? (1-20): ", 1, 20);

    vector<Robot*> robots;
    vector<pair<int, int> > startingCells;

    // 4. Configure each robot.
    for (int i = 0; i < robotCount; i++)
    {
        cout << "\n====================================\n";
        cout << "CONFIGURING ROBOT " << i + 1 << endl;
        cout << "====================================\n";

        int id = i + 1;

        cout << "Choose robot type:\n";
        cout << "1. General Robot\n";
        cout << "2. Delivery Robot\n";
        cout << "3. Rescue Robot\n";
        cout << "4. Inspection Robot\n";

        int typeChoice = readBoundedInt(
            "Enter choice (1-4): ", 1, 4);

        string name;

        cout << "Enter robot name (one word): ";
        cin >> name;

        Robot* robot = NULL;

        if (typeChoice == 1)
        {
            robot = new Robot(id, name, "General");
        }
        else if (typeChoice == 2)
        {
            int capacity = readBoundedInt(
                "Package capacity (1-100): ", 1, 100);

            robot = new DeliveryRobot(id, name, capacity);
        }
        else if (typeChoice == 3)
        {
            int equipment = readBoundedInt(
                "Number of rescue equipment items (0-100): ",
                0, 100);

            robot = new RescueRobot(id, name, equipment);
        }
        else
        {
            robot = new InspectionRobot(id, name);
        }

        // Select a valid, unoccupied starting cell.
        pair<int, int> start;

        while (true)
        {
            start = readWalkableCell(
                map, "Enter starting position:");

            bool occupied = false;

            for (int j = 0;
                 j < static_cast<int>(startingCells.size()); j++)
            {
                if (startingCells[j] == start)
                {
                    occupied = true;
                    break;
                }
            }

            if (!occupied)
                break;

            cout << "Another robot already starts there. "
                 << "Choose another cell.\n";
        }

        // Select a valid destination.
        pair<int, int> destination =
            readWalkableCell(map, "Enter destination:");

        robot->setPosition(start.first, start.second);

        robot->setDestination(
            destination.first, destination.second);

        int priority = readBoundedInt(
            "Priority (1=Low, 2=Normal, 3=High, 4=Critical): ",
            1, 4);

        robot->setPriority(priority);

        double battery;

        while (true)
        {
            battery = readDouble(
                "Starting battery (0-100%): ");

            if (robot->setBattery(battery))
                break;

            cout << "Battery must be between 0 and 100.\n";
        }

        startingCells.push_back(start);
        robots.push_back(robot);

        cout << "\nRobot configured successfully.\n";
    }
displayRobotGrid(map, robots);
    // 5. Register all robots and run the simulation.
    {
        SwarmManager manager(map);

        bool registrationFailed = false;

        for (int i = 0;
             i < static_cast<int>(robots.size()); i++)
        {
            if (!manager.addRobot(robots[i]))
            {
                registrationFailed = true;
                break;
            }
        }

        if (!registrationFailed)
        {
            cout << "\n===== SIMULATION CONFIGURATION =====\n";
            cout << "Grid: " << rows << " x " << cols << endl;
            cout << "Obstacles: " << obstacleCount << endl;
            cout << "Robots: " << manager.getRobotCount() << endl;

            manager.displayRobots();

            int choice = readBoundedInt(
                "\n1. Start simulation\n2. Exit\nEnter choice: ",
                1, 2);

           
if (choice == 1)
{
    if (validateRobotRoutes(map, robots))
    {
        manager.runSimulation();
    }
    else
    {
        cout << "\nSimulation cancelled because "
             << "one or more robots have no route.\n";
    }

    cout << "\n===== FINAL ROBOT STATUS =====\n";
    manager.displayRobots();
}

        }
        else
        {
            cout << "\nRegistration failed. "
                 << "Simulation cancelled.\n";
        }
    }

    // SwarmManager stores non-owning pointers.
    // We delete the robots here, after using the manager.
    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        delete robots[i];
    }

    robots.clear();

    cout << "\nSimulator closed.\n";

    return 0;
}
