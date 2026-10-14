
#ifndef GRIDMAP_H
#define GRIDMAP_H

#include <vector>
#include <utility>
using namespace std;

class GridMap
{
private:
    int rows;
    int cols;
    vector<vector<int> > grid;

public:
    GridMap(int r = 20, int c = 20);

    bool isInside(int x, int y) const;
    bool isWalkable(int x, int y) const;
    bool isObstacle(int x, int y) const;

    bool addObstacle(int x, int y);
    bool removeObstacle(int x, int y);

    void displayMap() const;

    int getRows() const;
    int getCols() const;
    void displayMap(
    const vector<pair<int, int> >& robotPositions,
    const vector<pair<int, int> >& destinations,
    const vector<int>& robotIDs) const;
};

#endif
