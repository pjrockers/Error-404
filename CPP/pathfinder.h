
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>
#include <utility>
#include "gridmap.h"

using namespace std;

class Pathfinder
{
public:
    vector<pair<int, int> > findPath(
        const GridMap& map,
        int startX, int startY,
        int destX, int destY
    );
};

#endif

