
#include <iostream>
#include <vector>
#include <queue>
#include <utility>

#include "pathfinder.h"

using namespace std;

vector<pair<int, int> > Pathfinder::findPath(
    const GridMap& map,
    int startX, int startY,
    int destX, int destY)
{
    vector<pair<int, int> > path;

    // Check whether start and destination are valid.
    if (!map.isWalkable(startX, startY) ||
        !map.isWalkable(destX, destY))
    {
        cout << "Invalid start or destination!\n";
        return path;
    }

    int rows = map.getRows();
    int cols = map.getCols();

    // Track visited cells.
    vector<vector<bool> > visited(
        rows, vector<bool>(cols, false));

    // Store the previous cell used to reach each cell.
    vector<vector<pair<int, int> > > parent(
        rows,
        vector<pair<int, int> >(cols, make_pair(-1, -1)));

    queue<pair<int, int> > q;

    q.push(make_pair(startX, startY));
    visited[startY][startX] = true;

    // Right, left, down, up.
    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};

    bool found = false;

    while (!q.empty())
    {
        pair<int, int> current = q.front();
        q.pop();

        int x = current.first;
        int y = current.second;

        if (x == destX && y == destY)
        {
            found = true;
            break;
        }

        for (int i = 0; i < 4; i++)
        {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (map.isWalkable(nx, ny) &&
                !visited[ny][nx])
            {
                visited[ny][nx] = true;

                parent[ny][nx] = make_pair(x, y);

                q.push(make_pair(nx, ny));
            }
        }
    }

    if (!found)
    {
        cout << "No path exists!\n";
        return path;
    }

    // Reconstruct the path from destination to start.
    int x = destX;
    int y = destY;

    while (x != startX || y != startY)
    {
        path.push_back(make_pair(x, y));

        pair<int, int> previous = parent[y][x];

        x = previous.first;
        y = previous.second;
    }

    path.push_back(make_pair(startX, startY));

    // Reverse it so the path starts at the robot.
    vector<pair<int, int> > correctPath;

    for (int i = static_cast<int>(path.size()) - 1;
         i >= 0; i--)
    {
        correctPath.push_back(path[i]);
    }

    return correctPath;
}

