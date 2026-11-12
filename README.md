# 🤖 Robot Swarm Simulator

A multi-robot simulation platform that demonstrates autonomous navigation, pathfinding, collision avoidance, and coordination of multiple robots operating in a shared environment.
The system allows users to create and manage robots, configure simulation environments, assign destinations, and observe how multiple robots coordinate their movements while handling obstacles, priorities, and battery limitations.

## 📌 Overview

In environments such as warehouses, industrial facilities, and disaster-management operations, multiple autonomous robots often need to work simultaneously.

Coordinating these robots introduces challenges such as:
- Finding valid paths around obstacles.
- Preventing collisions between robots.
- Managing movement conflicts and blocked paths.
- Prioritizing important tasks.
- Monitoring battery levels and robot states.

**Robot Swarm Simulator** addresses these challenges through a configurable grid-based environment and a multi-robot coordination system.

The project combines a C++ simulation engine with a React frontend and a C++ backend to provide an interactive simulation experience.


## ✨ Features
**1. User Authentication and Profiles**
- User registration and login.
- Individual user profiles.
- Persistent user data.

**2. Robot Management**
- Create and manage multiple robots.
- Built-in robot types:
  - General Robot
  - Delivery Robot
  - Rescue Robot
  - Inspection Robot
- Custom robot types and attributes.
- Edit robot information and properties.
- Configure robot parameters such as battery, priority, and destination.

**3. Environment Configuration**
- Create custom grid-based environments.
- Configure grid rows and columns.
- Add obstacles at selected coordinates.
- Configure robot starting positions and destinations.
- Add charging stations.

**4. Pathfinding**
- Breadth-First Search (BFS) for pathfinding.
- Obstacle-aware navigation.
- Route validation before simulation.
- Detection of unreachable destinations.

**5. Multi-Robot Coordination**
- Simultaneous operation of multiple robots.
- Collision avoidance.
- Detection of occupied cells.
- Handling of blocked movement and waiting robots.
- Priority-based movement scheduling.

**6. Battery Management**
- Battery consumption during movement.
- Low-battery detection.
- Charging station support.
- Robot status monitoring.

**7. Simulation Visualization**
- Interactive grid-based simulation.
- Live robot position updates.
- Visual representation of obstacles, destinations, and charging stations.
- Robot status and battery information.

**8. Simulation History and Replay**
- Save completed simulation records.
- View previous simulations.
- Replay simulation steps.
- Inspect robot positions, destinations, battery levels, and statuses at different steps.
- Visualize the environment during replay.

## 🛠️ Technologies Used

| Technology | Purpose |
|---|---|
| C++ | Core simulation engine and backend |
| Object-Oriented Programming | Robot classes and modular design |
| React.js | Frontend user interface |
| JavaScript | Frontend functionality |
| CSS | Interface styling |
| SQLite | Persistent data storage |
| JSON | Data exchange and simulation records |
| HTTP APIs | Frontend-backend communication |
| BFS | Pathfinding algorithm |
| Git and GitHub | Version control and collaboration |




## 🏗️ System Architecture

The project follows a modular architecture consisting of a frontend, backend server, simulation engine, and database.

```text
                   USER
                     |
                     v
              REACT FRONTEND
                     |
                HTTP / JSON
                     |
                     v
             C++ BACKEND SERVER
                     |
          +----------+----------+
          |                     |
          v                     v
   USER / ROBOT            SIMULATION
   MANAGEMENT              CONTROLLER
          |                     |
          v              +------+------+
       SQLITE             |      |      |
       DATABASE           v      v      v
                       ROBOT   GRID   PATHFINDER
                       CLASSES  MAP
                              |
                              v
                        SWARM MANAGER
                              |
                              v
                    MULTI-ROBOT MOVEMENT
                              |
                              v
                     SIMULATION STATES
                              |
                              v
                     HISTORY AND REPLAY

```
## ⚙️ Installation and Setup

**# Prerequisites**

Install the following tools:

- Git
- Visual Studio Code
- MSYS2 with the UCRT64 GCC toolchain
- Node.js and npm
- SQLite
- libsodium

**### 1. Clone the Repository**

```bash
git clone https://github.com/pjrockers/Error-404.git
cd Error-404
```

**### 2. Compile the C++ Backend**

Open the project in Visual Studio Code and make sure the MSYS2 UCRT64 GCC toolchain is configured correctly.

Using the MSYS2 UCRT64 environment, compile the backend with the required source files and dependencies.

```powershell
C:\msys64\ucrt64\bin\g++.exe -Wall -Wextra -std=c++11 -pthread `
Server/server.cpp `
Server/auth.cpp `
Server/robots.cpp `
Server/database.cpp `
Server/simulation_history.cpp `
Server/robot_templates.cpp `
Server/simulation_routes.cpp `
CPP/robot.cpp `
CPP/deliveryrobot.cpp `
CPP/rescuerobot.cpp `
CPP/inspectionrobot.cpp `
CPP/gridmap.cpp `
CPP/pathfinder.cpp `
CPP/swarmmanager.cpp `
CPP/stateexporter.cpp `
CPP/SimulationController.cpp `
Server/sqlite/sqlite3.o `
-o Server.exe `
-lsodium -lws2_32
```

 **Note:** The compilation command assumes that the required SQLite object file and other dependencies are available at the specified paths.

**### 3. Start the Backend Server**

After successful compilation, run:

```powershell
.\Server.exe
```

Keep the backend terminal running.

**### 4. Install Frontend Dependencies**

Open a new terminal and navigate to the frontend directory:

```bash
cd frontend
npm install
```

**### 5. Start the Frontend**

Run:

```bash
npm run dev
```

Vite will display a local URL in the terminal. Open that URL in your browser.

**### 6. Run the Application**

Make sure that:

1. The C++ backend server is running.
2. The frontend development server is running.
3. The required dependencies and configuration files are available.

The frontend communicates with the backend to interact with the robot swarm simulation.



## 🚀 How To Use

1. Register a new account or log in.
2. Open the robot management section.
3. Create robots using built-in templates or custom configurations.
4. Configure the simulation grid.
5. Add obstacles and charging stations.
6. Assign starting positions and destinations to robots.
7. Configure robot priorities and battery levels.
8. Validate the robot routes.
9. Start the simulation.
10. Observe robot movement, coordination, and status updates.
11. Open simulation history to review previous runs.
12. Use the replay controls to inspect individual simulation steps.


## 🧠 Core Concepts and Data Structures

| Concept | Application |
|---|---|
| Classes and Objects | Represent robots and simulation components |
| Inheritance | Implement specialized robot types |
| Polymorphism | Support common robot interfaces |
| Encapsulation | Manage robot properties and operations |
| Vectors | Store robots and simulation data |
| 2D Vectors | Represent the grid environment |
| Queue | Support BFS traversal |
| Graph Traversal | Find valid routes |
| Sorting | Process robots according to priority |
| Mutex | Protect shared simulation data |
| JSON | Store and exchange simulation information |


## 🔮 Future Improvements

- Improve robot movement and coordination strategies.
- Introduce more advanced pathfinding algorithms such as A*.
- Improve handling of complex multi-robot traffic conflicts.
- Add more robot categories and customizable capabilities.
- Enhance simulation performance for larger environments.
- Expand environment configuration and simulation controls.
- Improve visualization and user experience.


## 👥 Team

**Project:** Robot Swarm Simulator  
**Repository:** [Error-404](https://github.com/pjrockers/Error-404)

**Contributors:**
- Parth Jain
- Samiksha Verma
- Raj Gupta


## 📄 License

This project was developed as an academic Project-Based Learning (PBL) project.

License details can be added according to the team's requirements.


⭐ If you find this project interesting, consider giving the repository a star! 
