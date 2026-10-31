
#include "simulation_routes.h"

#include "../CPP/SimulationController.h"
#include "../CPP/robot.h"
#include "../CPP/robotstate.h"
#include "auth.h"

#include <nlohmann/json.hpp>

#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>
#include <utility>

using namespace std;
using json = nlohmann::json;

// One independent simulation for each authenticated user.
class UserSimulation
{
public:
    SimulationController controller;

private:
    thread worker;
    atomic<bool> stopWorker;
    mutex workerMutex;

public:
    UserSimulation() : stopWorker(false) {}

    ~UserSimulation()
    {
        stop();
    }

    void startWorker()
    {
        lock_guard<mutex> lock(workerMutex);

        // Join an earlier worker if it has already finished.
        if (worker.joinable())
        {
            worker.join();
        }

        stopWorker = false;

        worker = thread([this]()
        {
            while (!stopWorker)
            {
                string currentStatus = controller.getStatus();

                if (currentStatus == "paused")
                {
                    this_thread::sleep_for(chrono::milliseconds(100));
                    continue;
                }

                if (currentStatus != "running")
                {
                    break;
                }

                controller.stepSimulation();

                this_thread::sleep_for(chrono::milliseconds(500));
            }
        });
    }

    void stop()
    {
        stopWorker = true;

        lock_guard<mutex> lock(workerMutex);

        if (worker.joinable())
        {
            worker.join();
        }
    }
};

// Store simulations by authenticated user ID.
static map<sqlite3_int64, shared_ptr<UserSimulation>> userSimulations;
static mutex userSimulationsMutex;

static shared_ptr<UserSimulation> getOrCreateSimulation(sqlite3_int64 userId)
{
    lock_guard<mutex> lock(userSimulationsMutex);

    map<sqlite3_int64, shared_ptr<UserSimulation>>::iterator it =
        userSimulations.find(userId);

    if (it != userSimulations.end())
    {
        return it->second;
    }

    shared_ptr<UserSimulation> simulation(new UserSimulation());
    userSimulations[userId] = simulation;

    return simulation;
}

static json buildStateResponse(UserSimulation& simulation)
{
    json response;

    response["status"] = simulation.controller.getStatus();
    response["current_step"] = simulation.controller.getCurrentStep();
    response["finished"] = simulation.controller.isFinished();
    response["error"] = simulation.controller.getErrorMessage();

    vector<RobotState> states = simulation.controller.getRobotStates();

    response["robots"] = json::array();

    for (size_t i = 0; i < states.size(); i++)
    {
        json robot;

        robot["id"] = states[i].id;
        robot["name"] = states[i].name;
        robot["type"] = states[i].type;

        robot["x"] = states[i].x;
        robot["y"] = states[i].y;

        robot["destX"] = states[i].destX;
        robot["destY"] = states[i].destY;

        robot["battery"] = states[i].battery;
        robot["speed"] = states[i].speed;

        robot["priority"] = states[i].priority;
        robot["status"] = states[i].status;

        response["robots"].push_back(robot);
    }

    return response;
}

static bool getUserIdFromRequest(
    const httplib::Request& req,
    sqlite3_int64& userId)
{
    return getAuthenticatedUserId(req, userId);
}

void registerSimulationRoutes(httplib::Server& server)
{
    // -------------------------------------------------
    // CONFIGURE SIMULATION
    // -------------------------------------------------
    server.Post("/api/simulation/configure",
        [](const httplib::Request& req, httplib::Response& res)
    {
        sqlite3_int64 userId;

        if (!getUserIdFromRequest(req, userId))
        {
            res.status = 401;
            res.set_content(
                R"({"error":"Unauthorized"})",
                "application/json"
            );
            return;
        }

        try
        {
            json body = json::parse(req.body);

            if (!body.contains("rows") ||
                !body.contains("cols") ||
                !body.contains("robots") ||
                !body["robots"].is_array())
            {
                res.status = 400;
                res.set_content(
                    R"({"error":"Missing grid dimensions or robots"})",
                    "application/json"
                );
                return;
            }

            int rows = body["rows"].get<int>();
            int cols = body["cols"].get<int>();

            vector<pair<int, int>> obstacles;

            if (body.contains("obstacles"))
            {
                if (!body["obstacles"].is_array())
                {
                    throw runtime_error("Obstacles must be an array.");
                }

                for (size_t i = 0; i < body["obstacles"].size(); i++)
                {
                    const json& obstacle = body["obstacles"][i];

                    if (!obstacle.contains("x") ||
                        !obstacle.contains("y"))
                    {
                        throw runtime_error("Invalid obstacle data.");
                    }

                    obstacles.push_back(make_pair(
                        obstacle["x"].get<int>(),
                        obstacle["y"].get<int>()
                    ));
                }
            }

            if (body["robots"].empty() ||
                body["robots"].size() > 20)
            {
                throw runtime_error("Add between 1 and 20 robots.");
            }

            // Create a new simulation for this user.
            shared_ptr<UserSimulation> simulation =
                getOrCreateSimulation(userId);

            // Stop the previous background worker before reconfiguration.
            simulation->stop();

            if (!simulation->controller.configureSimulation(
                    rows, cols, obstacles))
            {
                res.status = 400;
                res.set_content(
                    json({
                        {"error", simulation->controller.getErrorMessage()}
                    }).dump(),
                    "application/json"
                );
                return;
            }

            for (size_t i = 0; i < body["robots"].size(); i++)
            {
                const json& item = body["robots"][i];

                if (!item.contains("id") ||
                    !item.contains("name") ||
                    !item.contains("type") ||
                    !item.contains("x") ||
                    !item.contains("y") ||
                    !item.contains("destX") ||
                    !item.contains("destY"))
                {
                    throw runtime_error("Robot data is incomplete.");
                }

                int id = item["id"].get<int>();

                string name = item["name"].get<string>();
                string type = item["type"].get<string>();

                double x = item["x"].get<double>();
                double y = item["y"].get<double>();

                double destX = item["destX"].get<double>();
                double destY = item["destY"].get<double>();

                unique_ptr<Robot> robot(
                    new Robot(id, name, type)
                );

                if (!robot->setPosition(x, y))
                {
                    throw runtime_error("Invalid robot starting position.");
                }

                robot->setDestination(destX, destY);

                if (item.contains("battery") &&
                    !robot->setBattery(item["battery"].get<double>()))
                {
                    throw runtime_error("Invalid battery value.");
                }

                if (item.contains("speed") &&
                    !robot->setSpeed(item["speed"].get<double>()))
                {
                    throw runtime_error("Invalid robot speed.");
                }

                if (item.contains("priority"))
                {
                    int priority = item["priority"].get<int>();

                    if (priority < 1 || priority > 4)
                    {
                        throw runtime_error("Priority must be between 1 and 4.");
                    }

                    robot->setPriority(priority);
                }

                if (!simulation->controller.addRobot(move(robot)))
                {
                    throw runtime_error(
                        simulation->controller.getErrorMessage()
                    );
                }
            }

            res.status = 200;
            res.set_content(
                buildStateResponse(*simulation).dump(),
                "application/json"
            );
        }
        catch (const exception& e)
        {
            res.status = 400;
            res.set_content(
                json({{"error", e.what()}}).dump(),
                "application/json"
            );
        }
    });

    // -------------------------------------------------
    // START SIMULATION
    // -------------------------------------------------
    server.Post("/api/simulation/start",
        [](const httplib::Request& req, httplib::Response& res)
    {
        sqlite3_int64 userId;

        if (!getUserIdFromRequest(req, userId))
        {
            res.status = 401;
            res.set_content(R"({"error":"Unauthorized"})",
                            "application/json");
            return;
        }

        shared_ptr<UserSimulation> simulation =
            getOrCreateSimulation(userId);

        if (!simulation->controller.startSimulation())
        {
            res.status = 400;
            res.set_content(
                json({{"error", simulation->controller.getErrorMessage()}}).dump(),
                "application/json"
            );
            return;
        }

        simulation->startWorker();

        res.set_content(
            buildStateResponse(*simulation).dump(),
            "application/json"
        );
    });

    // -------------------------------------------------
    // PAUSE SIMULATION
    // -------------------------------------------------
    server.Post("/api/simulation/pause",
        [](const httplib::Request& req, httplib::Response& res)
    {
        sqlite3_int64 userId;

        if (!getUserIdFromRequest(req, userId))
        {
            res.status = 401;
            res.set_content(R"({"error":"Unauthorized"})",
                            "application/json");
            return;
        }

        shared_ptr<UserSimulation> simulation =
            getOrCreateSimulation(userId);

        if (!simulation->controller.pauseSimulation())
        {
            res.status = 400;
            res.set_content(
                json({{"error", simulation->controller.getErrorMessage()}}).dump(),
                "application/json"
            );
            return;
        }

        res.set_content(
            buildStateResponse(*simulation).dump(),
            "application/json"
        );
    });

    // -------------------------------------------------
    // RESUME SIMULATION
    // -------------------------------------------------
    server.Post("/api/simulation/resume",
        [](const httplib::Request& req, httplib::Response& res)
    {
        sqlite3_int64 userId;

        if (!getUserIdFromRequest(req, userId))
        {
            res.status = 401;
            res.set_content(R"({"error":"Unauthorized"})",
                            "application/json");
            return;
        }

        shared_ptr<UserSimulation> simulation =
            getOrCreateSimulation(userId);

        if (!simulation->controller.resumeSimulation())
        {
            res.status = 400;
            res.set_content(
                json({{"error", simulation->controller.getErrorMessage()}}).dump(),
                "application/json"
            );
            return;
        }

        res.set_content(
            buildStateResponse(*simulation).dump(),
            "application/json"
        );
    });

    // -------------------------------------------------
    // RESET SIMULATION
    // -------------------------------------------------
    server.Post("/api/simulation/reset",
        [](const httplib::Request& req, httplib::Response& res)
    {
        sqlite3_int64 userId;

        if (!getUserIdFromRequest(req, userId))
        {
            res.status = 401;
            res.set_content(R"({"error":"Unauthorized"})",
                            "application/json");
            return;
        }

        shared_ptr<UserSimulation> simulation =
            getOrCreateSimulation(userId);

        simulation->stop();

        if (!simulation->controller.resetSimulation())
        {
            res.status = 400;
            res.set_content(
                json({{"error", simulation->controller.getErrorMessage()}}).dump(),
                "application/json"
            );
            return;
        }

        res.set_content(
            buildStateResponse(*simulation).dump(),
            "application/json"
        );
    });

    // -------------------------------------------------
    // GET SIMULATION STATE
    // -------------------------------------------------
    server.Get("/api/simulation/state",
        [](const httplib::Request& req, httplib::Response& res)
    {
        sqlite3_int64 userId;

        if (!getUserIdFromRequest(req, userId))
        {
            res.status = 401;
            res.set_content(R"({"error":"Unauthorized"})",
                            "application/json");
            return;
        }

        shared_ptr<UserSimulation> simulation =
            getOrCreateSimulation(userId);

        res.set_content(
            buildStateResponse(*simulation).dump(),
            "application/json"
        );
    });
}
