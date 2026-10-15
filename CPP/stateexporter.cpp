
#include <fstream>
#include "stateexporter.h"

using namespace std;

bool StateExporter::exportToCSV(
    const vector<RobotState>& states,
    const string& filename)
{
    ofstream file(filename.c_str());

    if (!file.is_open())
        return false;

    file << "id,name,type,x,y,destX,destY,"
         << "battery,speed,priority,status\n";

    for (size_t i = 0; i < states.size(); i++)
    {
        const RobotState& robot = states[i];

        file << robot.id << ","
             << robot.name << ","
             << robot.type << ","
             << robot.x << ","
             << robot.y << ","
             << robot.destX << ","
             << robot.destY << ","
             << robot.battery << ","
             << robot.speed << ","
             << robot.priority << ","
             << robot.status << "\n";
    }

    file.close();
    return true;
}
string escapeJSON(const string& text)
{
    string result;

    for (size_t i = 0; i < text.length(); i++)
    {
        char c = text[i];

        if (c == '"')
            result += "\\\"";
        else if (c == '\\')
            result += "\\\\";
        else if (c == '\n')
            result += "\\n";
        else if (c == '\r')
            result += "\\r";
        else if (c == '\t')
            result += "\\t";
        else if (static_cast<unsigned char>(c) < 32)
            result += ' ';
        else
            result += c;
    }

    return result;
}

bool StateExporter::exportToJSON(
    const vector<RobotState>& states,
    const string& filename)
{
    ofstream file(filename.c_str());

    if (!file.is_open())
        return false;

    file << "{\n";
    file << "  \"robots\": [\n";

    for (size_t i = 0; i < states.size(); i++)
    {
        const RobotState& robot = states[i];

        file << "    {\n";
        file << "      \"id\": " << robot.id << ",\n";
        file << "      \"name\": \"" << escapeJSON(robot.name) << "\",\n";
        file << "      \"type\": \"" << escapeJSON(robot.type) << "\",\n";
        file << "      \"x\": " << robot.x << ",\n";
        file << "      \"y\": " << robot.y << ",\n";
        file << "      \"destX\": " << robot.destX << ",\n";
        file << "      \"destY\": " << robot.destY << ",\n";
        file << "      \"battery\": " << robot.battery << ",\n";
        file << "      \"speed\": " << robot.speed << ",\n";
        file << "      \"priority\": " << robot.priority << ",\n";
        file << "      \"status\": \"" << escapeJSON(robot.status) << "\"\n";
        file << "    }";

        if (i + 1 < states.size())
            file << ",";

        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
    return !file.fail();
}
