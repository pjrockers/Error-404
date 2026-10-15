
#ifndef STATEEXPORTER_H
#define STATEEXPORTER_H

#include <vector>
#include <string>
#include "robotstate.h"

class StateExporter
{
public:
    static bool exportToCSV(
        const std::vector<RobotState>& states,
        const std::string& filename
    );

    static bool exportToJSON(
        const std::vector<RobotState>& states,
        const std::string& filename
    );
};

#endif
