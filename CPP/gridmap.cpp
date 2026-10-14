
#include <iostream>
#include "gridmap.h"

using namespace std;

GridMap::GridMap(int r, int c)
{
    if (r < 1)
        r = 20;

    if (c < 1)
        c = 20;

    rows = r;
    cols = c;

    grid.resize(rows, vector<int>(cols, 0));
}

bool GridMap::isInside(int x, int y) const
{
    return x >= 0 && x < cols &&
           y >= 0 && y < rows;
}

bool GridMap::isWalkable(int x, int y) const
{
    return isInside(x, y) && grid[y][x] == 0;
}

bool GridMap::isObstacle(int x, int y) const
{
    return isInside(x, y) && grid[y][x] == 1;
}

bool GridMap::addObstacle(int x, int y)
{
    if (!isInside(x, y) || grid[y][x] == 1)
        return false;

    grid[y][x] = 1;
    return true;
}

bool GridMap::removeObstacle(int x, int y)
{
    if (!isObstacle(x, y))
        return false;

    grid[y][x] = 0;
    return true;
}

void GridMap::displayMap() const
{
    cout << "\n========== GRID MAP ==========\n";

    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            if (grid[y][x] == 1)
                cout << "# ";
            else
                cout << ". ";
        }

        cout << endl;
    }
}

int GridMap::getRows() const
{
    return rows;
}

int GridMap::getCols() const
{
    return cols;
}

void GridMap::displayMap(
    const vector<pair<int, int> >& robotPositions,
    const vector<pair<int, int> >& destinations,
    const vector<int>& robotIDs) const
{
    cout << "\n========== ROBOT GRID ==========\n";

    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            bool robotFound = false;
            bool destinationFound = false;
            int robotID = -1;

            for (int i = 0;
                 i < static_cast<int>(robotPositions.size()); i++)
            {
                if (robotPositions[i].first == x &&
                    robotPositions[i].second == y)
                {
                    robotFound = true;
                    robotID = robotIDs[i];
                    break;
                }
            }

            for (int i = 0;
                 i < static_cast<int>(destinations.size()); i++)
            {
                if (destinations[i].first == x &&
                    destinations[i].second == y)
                {
                    destinationFound = true;
                    break;
                }
            }

            if (robotFound)
            {
                cout << "R" << robotID << " ";
            }
            else if (grid[y][x] == 1)
            {
                cout << "## ";
            }
            else if (destinationFound)
            {
                cout << "D  ";
            }
            else
            {
                cout << ".  ";
            }
        }

        cout << endl;
    }

    cout << "\nLegend: R<number> = Robot, "
         << "D = Destination, ## = Obstacle, . = Empty\n";
}
