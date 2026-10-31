#include "robotfactory.h"

#include "deliveryrobot.h"
#include "rescuerobot.h"
#include "inspectionrobot.h"
#include <climits>
#include <string>
#include <cmath>

using namespace std;
using json = nlohmann::json;

bool readIntAttribute(
    const json& attributes,
    const string& key,
    int defaultValue,
    int& result)
{
    if (!attributes.contains(key))
    {
        result = defaultValue;
        return true;
    }

    if (!attributes[key].is_number_integer())
        return false;

    long long value = attributes[key].get<long long>();

    if (value < INT_MIN || value > INT_MAX)
        return false;

    result = static_cast<int>(value);
    return true;
}  
Robot* createRobot(const json& data)
{
    // Check that the configuration is valid.
    if (!data.is_object())
        return NULL;

    if (!data.contains("id") ||
        !data.contains("name") ||
        !data.contains("robot_type") ||
        !data.contains("x") ||
        !data.contains("y") ||
        !data.contains("dest_x") ||
        !data.contains("dest_y") ||
        !data.contains("battery") ||
        !data.contains("speed") ||
        !data.contains("priority"))
    {
        return NULL;
    }

    if (!data["id"].is_number_integer() ||
        !data["name"].is_string() ||
        !data["robot_type"].is_string() ||
        !data["x"].is_number_integer() ||
        !data["y"].is_number_integer() ||
        !data["dest_x"].is_number_integer() ||
        !data["dest_y"].is_number_integer() ||
        !data["battery"].is_number() ||
        !data["speed"].is_number() ||
        !data["priority"].is_number_integer())
    {
        return NULL;
    }

    int id = data["id"].get<int>();

    string name = data["name"].get<string>();
    string type = data["robot_type"].get<string>();

    int x = data["x"].get<int>();
    int y = data["y"].get<int>();

    int destX = data["dest_x"].get<int>();
    int destY = data["dest_y"].get<int>();

    double battery = data["battery"].get<double>();
    double speed = data["speed"].get<double>();

    int priority = data["priority"].get<int>();

    if (id <= 0 ||
        name.empty() ||
        x < 0 || y < 0 ||
        destX < 0 || destY < 0 ||
        !isfinite(battery) ||
        !isfinite(speed) ||
        battery < 0 || battery > 100 ||
        speed <= 0 || speed > 100 ||
        priority < 1 || priority > 4)
    {
        return NULL;
    }

    json attributes = json::object();

    if (data.contains("attributes"))
    {
        if (!data["attributes"].is_object())
            return NULL;

        attributes = data["attributes"];
    }

    Robot* robot = NULL;

    // Create the correct robot subtype.
    if (type == "General")
    {
        robot = new Robot(id, name, "General");
    }
    else if (type == "Delivery")
    {
        int capacity;
int carried;

if (!readIntAttribute(attributes, "packageCapacity", 1, capacity) ||
    !readIntAttribute(attributes, "packagesCarried", 0, carried))
{
    return NULL;
}

        if (capacity <= 0 ||
            carried < 0 ||
            carried > capacity)
        {
            return NULL;
        }

        DeliveryRobot* delivery =
            new DeliveryRobot(id, name, capacity);

        if (!delivery->setPackagesCarried(carried))
        {
            delete delivery;
            return NULL;
        }

        robot = delivery;
    }
    else if (type == "Rescue")
    {
        int equipment;

if (!readIntAttribute(attributes, "equipmentCount", 0, equipment))
    return NULL;

        if (equipment < 0)
            return NULL;

        robot = new RescueRobot(id, name, equipment);
    }
    else if (type == "Inspection")
    {
        int completed;

if (!readIntAttribute(attributes, "inspectionsCompleted", 0, completed))
    return NULL;

        if (completed < 0)
            return NULL;

        InspectionRobot* inspection =
            new InspectionRobot(id, name);

        if (!inspection->setInspectionsCompleted(completed))
        {
            delete inspection;
            return NULL;
        }

        robot = inspection;
    }
    else
    {
        return NULL;
    }

    // Restore the common robot properties.
    if (!robot->setPosition(x, y) ||
        !robot->setBattery(battery) ||
        !robot->setSpeed(speed))
    {
        delete robot;
        return NULL;
    }

    robot->setDestination(destX, destY);
    robot->setPriority(priority);

    if (data.contains("status") &&
        data["status"].is_string())
    {
        robot->setStatus(data["status"].get<string>());
    }

    return robot;
}