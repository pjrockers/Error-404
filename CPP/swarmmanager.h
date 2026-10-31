#ifndef SWARMMANAGER_H
#define SWARMMANAGER_H

#include <vector>
#include <string>
#include <ostream>
#include "robot.h"
#include "gridmap.h"
#include "robotstate.h"

class SwarmManager
{
public:
    // Why the simulation stopped (NOT_FINISHED while it is still running).
    enum FinishReason
    {
        NOT_FINISHED,
        ALL_ARRIVED,
        NO_BATTERY,
        DEADLOCK,
        MAX_STEPS_REACHED
    };

private:
    static const int MAX_STEPS = 1000;
    static const int MAX_BLOCKED_RETRIES = 3;

    // What we need to restore a robot in resetSimulation().
    struct InitialState
    {
        double x;
        double y;
        double battery;
        string status;
    };

    GridMap& map;
    vector<Robot*> robots;
    vector<int> waitCounters;
    vector<InitialState> initialStates;

    // Persistent simulation state (these used to be local variables).
    int currentStep;
    int blockedRetryCount;
    bool simulationFinished;
    FinishReason finishReason;

    // Output control.
    bool verbose;
    string godotExportPath;      // empty = do not write the Godot file
    mutable ostream nullStream;  // swallows output when verbose == false

    ostream& log() const;
    int indexOf(const Robot* robot) const;
    int effectivePriority(const Robot* robot) const;
    void finish(FinishReason reason);
    void printSummary() const;

public:
    SwarmManager(GridMap& grid);

    bool addRobot(Robot* robot);
    void displayRobots() const;
    int getRobotCount() const;
    vector<RobotState> getRobotStates() const;
    void displayCurrentMap() const;

    // ---- Step-based API (for the backend) ----
    // Runs ONE iteration. Returns true while the simulation should
    // continue, false once it has finished (see getFinishReason()).
    bool runSimulationStep();

    // Restores robots to their registered start state, clears counters.
    void resetSimulation();

    bool isFinished() const;
    int getCurrentStep() const;
    FinishReason getFinishReason() const;
    string getFinishReasonText() const;   // "all_arrived", "deadlock", ...

    // ---- Console mode (same behaviour as before) ----
    // Loops runSimulationStep(), prints the map, writes the Godot file
    // and sleeps 500 ms per step, then prints the summary.
    void runSimulation();

    // ---- Godot bridge file ----
    // Writes the state file Godot reads. Does nothing if the path is empty.
    void exportGodotState(const string& status,
                          const string& errorMessage) const;
    void setGodotExportPath(const string& path);

    // ---- Configuration ----
    void setVerbose(bool value);
};

#endif