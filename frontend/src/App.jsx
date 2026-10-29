
import { useEffect, useState } from "react";
import Auth from "./Auth";
import "./App.css";
import {
  getCurrentUser,
  logoutUser,
  getRobotTemplates,
  saveRobotTemplate,
  getRobots,
  createRobot,
  updateRobot,
  deleteRobot,
} from "./api";

const initialRobots = [
  { id: 1, name: "Alpha", type: "Delivery", x: 1, y: 1, battery: 100, status: "Idle" },
  { id: 2, name: "Beta", type: "Rescue", x: 6, y: 4, battery: 85, status: "Idle" },
];

function App() {
const [templates, setTemplates] = useState([]);
const [templatesLoading, setTemplatesLoading] = useState(false);
const [templateError, setTemplateError] = useState("");
const [templateName, setTemplateName] = useState("");
const [templateType, setTemplateType] = useState("Delivery");
const [templateBattery, setTemplateBattery] = useState(100);
const [templateSpeed, setTemplateSpeed] = useState(5);
const [templatePriority, setTemplatePriority] = useState(2);
const [templateCapacity, setTemplateCapacity] = useState(10);
const [savingTemplate, setSavingTemplate] = useState(false);
const [deletingRobotId, setDeletingRobotId] = useState(null); 
  const [token, setToken] = useState(
    () => localStorage.getItem("robotSwarmToken")
  );
  const [user, setUser] = useState(null);
  const [authChecking, setAuthChecking] = useState(true);

  
  useEffect(() => {
    let cancelled = false;

    async function verifySession() {
      if (!token) {
        setUser(null);
        setAuthChecking(false);
        return;
      }

      setAuthChecking(true);

      try {
        const profile = await getCurrentUser(token);

        if (!cancelled) {
          setUser(profile);
        }
      } catch {
        localStorage.removeItem("robotSwarmToken");

        if (!cancelled) {
          setToken(null);
          setUser(null);
        }
      } finally {
        if (!cancelled) {
          setAuthChecking(false);
        }
      }
    }

    verifySession();

    return () => {
      cancelled = true;
    };
  }, [token]);

  async function loadTemplates() {
    if (!token) return;

    setTemplatesLoading(true);
    setTemplateError("");

    try {
      const data = await getRobotTemplates(token);

      if (!Array.isArray(data)) {
        throw new Error("Unexpected template data from the server.");
      }

      setTemplates(data);
    } catch (error) {
      setTemplateError(error.message || "Could not load templates.");
    } finally {
      setTemplatesLoading(false);
    }
  }
  async function loadRobots() {
  if (!token) return;

  setRobotsLoading(true);
  setRobotsError("");

  try {
    const data = await getRobots(token);

    if (!Array.isArray(data)) {
      throw new Error("Unexpected robot data from the server.");
    }

    setRobots(
      data.map((robot) => ({
        ...robot,
        id: Number(robot.id),
        type: robot.type,
        x: Number(robot.x),
        y: Number(robot.y),
        battery: Number(robot.battery),
        speed: Number(robot.speed ?? 5),
        priority: Number(robot.priority ?? 1),
        attributes: robot.attributes || {},
      }))
    );
  } catch (error) {
    setRobotsError(error.message || "Could not load robots.");
  } finally {
    setRobotsLoading(false);
  }
}
async function handleDeleteRobot(id) {
  const confirmed = window.confirm(
    "Are you sure you want to delete this robot?"
  );

  if (!confirmed) return;

  setDeletingRobotId(id);
  setRobotsError("");

  try {
    await deleteRobot(token, id);

    setRobots((current) =>
      current.filter((robot) => robot.id !== id)
    );
  } catch (error) {
    setRobotsError(error.message || "Could not delete robot.");
  } finally {
    setDeletingRobotId(null);
  }
}
  async function handleSaveTemplate(event) {
    event.preventDefault();
    setTemplateError("");

    if (!templateName.trim()) {
      setTemplateError("Enter a template name.");
      return;
    }

    setSavingTemplate(true);

    try {
      await saveRobotTemplate(token, {
        template_name: templateName.trim(),
        robot_type: templateType,
        battery: Number(templateBattery),
        speed: Number(templateSpeed),
        priority: Number(templatePriority),
        attributes: {
          packageCapacity: Number(templateCapacity),
        },
      });

      setTemplateName("");
      await loadTemplates();
    } catch (error) {
      setTemplateError(error.message || "Could not save template.");
    } finally {
      setSavingTemplate(false);
    }
  }
  useEffect(() => {
    if (token && user) {
      loadTemplates();
    }
  }, [token, user]);

  useEffect(() => {
  if (token && user) {
    loadRobots();
  }
}, [token, user]);
function handleEditRobot(robot) {
  console.log("Robot selected for editing:", robot);
  console.log("Robot ID:", robot.id);

  setEditingRobot(robot);
  setEditRobotName(robot.name);
  setRobotsError("");
}
  function handleLogin(newToken) {
    localStorage.setItem("robotSwarmToken", newToken);
    setToken(newToken);
  }

  async function handleLogout() {
    try {
      if (token) {
        await logoutUser(token);
      }
    } catch (error) {
      console.error("Logout request failed:", error);
    } finally {
      localStorage.removeItem("robotSwarmToken");
      setToken(null);
      setUser(null);
      setRunning(false);
    }
  }

  const [activePage, setActivePage] = useState("Dashboard");
  const [rows, setRows] = useState(10);
  const [cols, setCols] = useState(10);
  const [robots, setRobots] = useState(initialRobots);
  const [robotsLoading, setRobotsLoading] = useState(false);
  const [robotsError, setRobotsError] = useState("");
  const [editingRobot, setEditingRobot] = useState(null);
  const [editRobotName, setEditRobotName] = useState("");
  const [savingRobotEdit, setSavingRobotEdit] = useState(false);
  const [selectedRobotId, setSelectedRobotId] = useState(null);
  const [obstacles, setObstacles] = useState([]);
  const [selectedTool, setSelectedTool] = useState("obstacle");
  const [running, setRunning] = useState(false);
  const [selectedTemplateId, setSelectedTemplateId] = useState("");
  const [newRobotName, setNewRobotName] = useState("");
  const [newRobotX, setNewRobotX] = useState(0);
  const [newRobotY, setNewRobotY] = useState(0);
  
async function handleCellClick(x, y) {
  if (selectedTool === "obstacle") {
    const occupied = robots.some(
      (robot) => robot.x === x && robot.y === y
    );

    if (occupied) return;

    setObstacles((current) => {
      const exists = current.some(
        (cell) => cell.x === x && cell.y === y
      );

      return exists
        ? current.filter((cell) => cell.x !== x || cell.y !== y)
        : [...current, { x, y }];
    });

    return;
  }

  const robot = robots.find(
    (item) => item.id === selectedTool
  );

  if (!robot) return;

  const occupied = robots.some(
    (item) =>
      item.id !== robot.id &&
      item.x === x &&
      item.y === y
  );

  const blocked = obstacles.some(
    (cell) => cell.x === x && cell.y === y
  );

  if (occupied || blocked) return;

  // Don't save if the robot is already in this cell.
  if (robot.x === x && robot.y === y) return;

  setRobotsError("");

  try {
    // Save the new position to the backend first.
    await updateRobot(token, robot.id, {
      ...robot,
      x,
      y,
    });

    // Update the UI only after the save succeeds.
    setRobots((current) =>
      current.map((item) =>
        item.id === robot.id
          ? { ...item, x, y }
          : item
      )
    );
  } catch (error) {
    setRobotsError(
      error.message || "Could not save the robot's position."
    );
  }
}


async function createRobotFromTemplate(event) {
  event.preventDefault();

  const template = templates.find(
    (item) => String(item.id) === String(selectedTemplateId)
  );

  if (!template) {
    setTemplateError("Please select a saved template.");
    return;
  }

  const x = Number(newRobotX);
  const y = Number(newRobotY);

  if (
    !Number.isInteger(x) ||
    !Number.isInteger(y) ||
    x < 0 ||
    y < 0 ||
    x >= cols ||
    y >= rows
  ) {
    setTemplateError("Starting coordinates must be inside the grid.");
    return;
  }

  if (obstacles.some((cell) => cell.x === x && cell.y === y)) {
    setTemplateError("A robot cannot start on an obstacle.");
    return;
  }

  if (robots.some((robot) => robot.x === x && robot.y === y)) {
    setTemplateError("Another robot already occupies that cell.");
    return;
  }

  
setTemplateError("");
setRobotsError("");

const robotToCreate = {
  name: newRobotName.trim() || `Robot ${Date.now()}`,
  type: template.robot_type,
  x,
  y,
  battery: Number(template.battery),
  speed: Number(template.speed),
  priority: Number(template.priority),
  attributes: template.attributes || {},
  status: "Idle",
};

try {
  const result = await createRobot(token, robotToCreate);

  if (!result.id) {
    throw new Error("The server did not return the saved robot ID.");
  }

  const savedRobot = {
    ...robotToCreate,
    id: Number(result.id),
  };

  setRobots((current) => [...current, savedRobot]);

  setNewRobotName("");
  setSelectedTemplateId("");
  setTemplateError("");
} catch (error) {
  setTemplateError(error.message || "Could not create robot.");
}


  setNewRobotName("");
  setSelectedTemplateId("");
  setTemplateError("");
}

 
async function addRobot() {
  if (!token) {
    setRobotsError("Please log in before adding a robot.");
    return;
  }

  // Find an unoccupied cell that is not an obstacle.
  let freeCell = null;

  for (let y = 0; y < rows && !freeCell; y++) {
    for (let x = 0; x < cols; x++) {
      const occupied = robots.some(
        (robot) => Number(robot.x) === x && Number(robot.y) === y
      );

      const blocked = obstacles.some(
        (obstacle) =>
          Number(obstacle.x) === x && Number(obstacle.y) === y
      );

      if (!occupied && !blocked) {
        freeCell = { x, y };
        break;
      }
    }
  }

  if (!freeCell) {
    setRobotsError("No free cell is available for a new robot.");
    return;
  }

  const robotToCreate = {
    name: `Robot ${robots.length + 1}`,
    type: "General",
    x: freeCell.x,
    y: freeCell.y,
    battery: 100,
    speed: 5,
    priority: 1,
    status: "Idle",
    attributes: {},
  };

  try {
    setRobotsError("");

    const result = await createRobot(token, robotToCreate);
    console.log("Dashboard create robot response:", result);

    if (!result.id) {
      throw new Error("The server did not return the saved robot ID.");
    }

    const savedRobot = {
      ...robotToCreate,
      id: Number(result.id),
    };

    setRobots((current) => [...current, savedRobot]);
    setSelectedRobotId(savedRobot.id);
  } catch (error) {
    setRobotsError(error.message || "Could not create robot.");
  }
}


  function resetSimulation() {
    setRunning(false);
    setObstacles([]);
    setRobots(initialRobots);
    setSelectedTool("obstacle");
  }

  const occupiedCount = obstacles.length + robots.length;
  
  if (authChecking) {
    return (
      <div className="auth-page">
        <p>Checking your session...</p>
      </div>
    );
  }

  if (!token || !user) {
    return <Auth onLogin={handleLogin} />;
  }


  return (
    <div className="app-shell">
      <aside className="sidebar">
        <div className="brand">
          <div className="brand-icon">R</div>
          <div>
            <h2>RobotSwarm</h2>
            <p>SIMULATION LAB</p>
          </div>
        </div>

        <p className="nav-heading">WORKSPACE</p>
        {["Dashboard", "Grid Simulator", "Robots", "History"].map((page) => (
          <button
            key={page}
            className={`nav-item ${activePage === page ? "active" : ""}`}
            onClick={() => setActivePage(page)}
          >
            <span>{{
              Dashboard: "▦",
              "Grid Simulator": "⊞",
              Robots: "◈",
              History: "◷",
            }[page]}</span>
            {page}
          </button>
        ))}

        <div className="sidebar-bottom">
          <div className="connection-dot" />
          <div>
            <strong>Local workspace</strong>
            <p>Frontend preview</p>
          </div>
        </div>
      </aside>

      <main className="main-content">
        <header className="topbar">
          <div>
            <p className="eyebrow">ROBOT SWARM SIMULATOR / WORKSPACE</p>
            <h1>{activePage}</h1>
          </div>
          
<div className="topbar-right">
  <span className="demo-label">
    {user.username}
  </span>
  <div className="avatar">
    {user.username.slice(0, 1).toUpperCase()}
  </div>
  <button className="secondary-button" onClick={handleLogout}>
    Log out
  </button>
</div>

        </header>

        {activePage === "Dashboard" && (
          <>
            <section className="welcome-row">
              <div>
                <h2>Swarm overview</h2>
                <p>Configure your environment and manage your robot fleet.</p>
              </div>
              <button className="primary-button" onClick={() => setActivePage("Grid Simulator")}>
                Open simulator <span>↗</span>
              </button>
            </section>

            <section className="stats-grid">
              <StatCard label="TOTAL ROBOTS" value={robots.length} detail="In this workspace" icon="◈" />
              <StatCard label="GRID SIZE" value={`${rows} × ${cols}`} detail={`${rows * cols} cells`} icon="▦" />
              <StatCard label="OBSTACLES" value={obstacles.length} detail="Placed on grid" icon="⬡" />
              <StatCard label="SIMULATION" value={running ? "Running" : "Idle"} detail="Local demo only" icon="▷" />
            </section>

            <section className="content-grid">
              <div className="panel grid-panel">
                <div className="panel-heading">
                  <div>
                    <h3>Environment preview</h3>
                    <p>Your current 2D grid</p>
                  </div>
                  <button className="text-button" onClick={() => setActivePage("Grid Simulator")}>
                    Edit grid →
                  </button>
                </div>
                <GridPreview
                  rows={rows}
                  cols={cols}
                  robots={robots}
                  obstacles={obstacles}
                  onCellClick={handleCellClick}
                  interactive={false}
                  selectedRobotId={selectedRobotId}
                  onRobotSelect={(robot) => setSelectedRobotId(robot.id)}
                />
                <div className="legend">
                  <span><i className="legend-robot" /> Robot</span>
                  <span><i className="legend-obstacle" /> Obstacle</span>
                  <span><i className="legend-empty" /> Free cell</span>
                </div>
              </div>
              
{(() => {
  const selectedRobot = robots.find(
    (robot) => robot.id === selectedRobotId
  );

  if (!selectedRobot) {
    return (
      <div className="robot-inspector">
        <h3>Robot details</h3>
        <p>Select a robot on the grid to inspect its properties.</p>
      </div>
    );
  }

  return (
    <div className="robot-inspector">
      <div className="panel-heading">
  <div>
    <h3>{selectedRobot.name}</h3>
    <p>{selectedRobot.type} robot · ID {selectedRobot.id}</p>
  </div>

  <div className="inspector-actions">
    <span className="inspector-status">
      {selectedRobot.status || "Idle"}
    </span>

<button
  type="button"
  className="secondary-button"
  onClick={() => {
    setActivePage("Robots");
    handleEditRobot(selectedRobot);
  }}
  style={{
    position: "relative",
    zIndex: 9999,
    pointerEvents: "auto",
    cursor: "pointer",
  }}
>
  Edit Robot
</button>

  </div>
</div>

      <div className="inspector-grid">
        <div>
          <span>Position</span>
          <strong>
            ({selectedRobot.x}, {selectedRobot.y})
          </strong>
        </div>

        <div>
          <span>Battery</span>
          <strong>{selectedRobot.battery}%</strong>
        </div>

        <div>
          <span>Speed</span>
          <strong>{selectedRobot.speed ?? 5}</strong>
        </div>

        <div>
          <span>Priority</span>
          <strong>{selectedRobot.priority ?? 1} / 4</strong>
        </div>
      </div>
    </div>
  );
})()}


              <div className="panel fleet-panel">
                <div className="panel-heading">
                  <div>
                    <h3>Robot fleet</h3>
                    <p>Registered in this session</p>
                  </div>
                  <button className="text-button" onClick={() => setActivePage("Robots")}>View all →</button>
                </div>
                
<RobotList
  robots={robots.slice(0, 4)}
  onEdit={(robot) => {
    setSelectedRobotId(robot.id);
    setActivePage("Robots");
    handleEditRobot(robot);
  }}
  onDelete={handleDeleteRobot}
  deletingRobotId={deletingRobotId}
/>

                <button className="secondary-button full-button" onClick={addRobot}>+ Add robot</button>
              </div>
            </section>
          </>
        )}

        {activePage === "Grid Simulator" && (
          <section className="panel simulator-panel">
            <div className="panel-heading">
              <div>
                <h3>2D environment editor</h3>
                <p>Click cells to place or remove obstacles, or select a robot to reposition it.</p>
              </div>
              <span className={`status-pill ${running ? "running" : ""}`}>
                <i /> {running ? "Running (demo)" : "Ready"}
              </span>
            </div>

            <div className="simulator-controls">
              <label>Rows
                <input type="number" min="2" max="20" value={rows}
                  onChange={(e) => setRows(Math.max(2, Math.min(20, Number(e.target.value) || 2)))} />
              </label>
              <label>Columns
                <input type="number" min="2" max="20" value={cols}
                  onChange={(e) => setCols(Math.max(2, Math.min(20, Number(e.target.value) || 2)))} />
              </label>
              <label>Editing tool
                <select value={selectedTool} onChange={(e) =>
                  setSelectedTool(e.target.value === "obstacle" ? "obstacle" : Number(e.target.value))
                }>
                  <option value="obstacle">Place obstacles</option>
                  {robots.map((robot) => (
                    <option key={robot.id} value={robot.id}>Move {robot.name}</option>
                  ))}
                </select>
              </label>
            </div>

            <GridPreview
              rows={rows}
              cols={cols}
              robots={robots}
              obstacles={obstacles}
              onCellClick={handleCellClick}
              interactive
            />

            <div className="simulator-footer">
              <div className="legend">
                <span><i className="legend-robot" /> Robot</span>
                <span><i className="legend-obstacle" /> Obstacle</span>
                <span><i className="legend-empty" /> Free cell</span>
              </div>
              <div className="action-row">
                <button className="secondary-button" onClick={resetSimulation}>Reset</button>
                <button className="primary-button" onClick={() => setRunning(!running)}>
                  {running ? "Stop demo" : "▶ Start demo"}
                </button>
              </div>
            </div>
            <p className="notice">
              Demo only: this interface does not yet run the C++ pathfinding engine.
            </p>
          </section>
        )}

        
{activePage === "Robots" && (
  <section className="panel">
    <div className="panel-heading">
      <div>
        <h3>Robot management</h3>
        <p>Create robot templates that are saved to your account.</p>
      </div>
      <button className="secondary-button" onClick={loadTemplates}>
        Refresh templates
      </button>
    </div>

    <RobotList
  robots={robots}
  onDelete={handleDeleteRobot}
  deletingRobotId={deletingRobotId}
  onEdit={handleEditRobot}
/>

{editingRobot && (
  <form
    className="template-form"
    onSubmit={async (event) => {
      event.preventDefault();

      const updatedRobot = {
        ...editingRobot,
        name: editRobotName.trim(),
        battery: Number(editingRobot.battery),
        speed: Number(editingRobot.speed),
        priority: Number(editingRobot.priority),
        x: Number(editingRobot.x),
        y: Number(editingRobot.y),
      };

      
if (
  !Number.isInteger(updatedRobot.x) ||
  !Number.isInteger(updatedRobot.y) ||
  updatedRobot.x < 0 ||
  updatedRobot.x >= cols ||
  updatedRobot.y < 0 ||
  updatedRobot.y >= rows
) {
  setRobotsError("Robot coordinates must be inside the grid.");
  return;
}

const occupied = robots.some(
  (robot) =>
    robot.id !== updatedRobot.id &&
    robot.x === updatedRobot.x &&
    robot.y === updatedRobot.y
);

if (occupied) {
  setRobotsError("Another robot already occupies that cell.");
  return;
}

const blocked = obstacles.some(
  (cell) =>
    cell.x === updatedRobot.x &&
    cell.y === updatedRobot.y
);

if (blocked) {
  setRobotsError("A robot cannot be placed on an obstacle.");
  return;
}

if (
  !Number.isFinite(updatedRobot.battery) ||
  updatedRobot.battery < 0 ||
  updatedRobot.battery > 100
) {
  setRobotsError("Battery must be between 0 and 100.");
  return;
}

if (
  !Number.isFinite(updatedRobot.speed) ||
  updatedRobot.speed <= 0 ||
  updatedRobot.speed > 100
) {
  setRobotsError("Speed must be greater than 0 and at most 100.");
  return;
}

const priority = Number(updatedRobot.priority);

if (
  !Number.isInteger(priority) ||
  priority < 1 ||
  priority > 4
) {
  setRobotsError("Priority must be between 1 and 4.");
  return;
}

updatedRobot.priority = priority;

      if (!updatedRobot.name) {
        setRobotsError("Robot name cannot be empty.");
        return;
      }

      setSavingRobotEdit(true);
      setRobotsError("");

      try {
        await updateRobot(token, editingRobot.id, updatedRobot);

        setRobots((current) =>
          current.map((robot) =>
            robot.id === editingRobot.id
              ? updatedRobot
              : robot
          )
        );

        setEditingRobot(null);
        setEditRobotName("");
      } catch (error) {
        setRobotsError(
          error.message || "Could not update robot."
        );
      } finally {
        setSavingRobotEdit(false);
      }
    }}
  >
    <div className="panel-heading">
      <div>
        <h3>Edit robot</h3>
        <p>
          Editing {editingRobot.name} (ID {editingRobot.id})
        </p>
      </div>
    </div>

    <label>
      Robot name
      <input
        value={editRobotName}
        onChange={(event) =>
          setEditRobotName(event.target.value)
        }
        maxLength={50}
        required
      />
    </label>

    <label>
      Battery (%)
      <input
        type="number"
        min="0"
        max="100"
        value={editingRobot.battery}
        onChange={(event) =>
          setEditingRobot((current) => ({
            ...current,
            battery: event.target.value,
          }))
        }
        required
      />
    </label>

    <label>
      Speed
      <input
        type="number"
        min="0.1"
        max="100"
        step="0.1"
        value={editingRobot.speed}
        onChange={(event) =>
          setEditingRobot((current) => ({
            ...current,
            speed: event.target.value,
          }))
        }
        required
      />
    </label>

    <label>
      Priority
      <select
        value={editingRobot.priority}
        onChange={(event) =>
          setEditingRobot((current) => ({
            ...current,
            priority: Number(event.target.value),
          }))
        }
      >
        <option value={1}>1 - Low</option>
        <option value={2}>2 - Normal</option>
        <option value={3}>3 - High</option>
        <option value={4}>4 - Critical</option>
      </select>
    </label>

    <label>
      X coordinate
      <input
        type="number"
        min="0"
        max={cols - 1}
        value={editingRobot.x}
        onChange={(event) =>
          setEditingRobot((current) => ({
            ...current,
            x: event.target.value,
          }))
        }
        required
      />
    </label>

    <label>
      Y coordinate
      <input
        type="number"
        min="0"
        max={rows - 1}
        value={editingRobot.y}
        onChange={(event) =>
          setEditingRobot((current) => ({
            ...current,
            y: event.target.value,
          }))
        }
        required
      />
    </label>

    {robotsError && (
      <p className="auth-error">{robotsError}</p>
    )}

    <div className="form-actions">
      <button
        type="submit"
        className="primary-button"
        disabled={savingRobotEdit}
      >
        {savingRobotEdit ? "Saving..." : "Save changes"}
      </button>

      <button
        type="button"
        className="delete-button"
        disabled={savingRobotEdit}
        onClick={() => {
          setEditingRobot(null);
          setEditRobotName("");
          setRobotsError("");
        }}
      >
        Cancel
      </button>
    </div>
  </form>
)}

    <div className="panel-heading" style={{ marginTop: 28 }}>
      <div>
        <h3>Create robot template</h3>
        <p>Save a reusable robot configuration to your database.</p>
      </div>
    </div>

    <form onSubmit={handleSaveTemplate} className="template-form">
      <label>
        Template name
        <input
          value={templateName}
          onChange={(event) => setTemplateName(event.target.value)}
          maxLength={50}
          placeholder="e.g. Fast Delivery"
          required
        />
      </label>

      <label>
        Robot type
        <select
          value={templateType}
          onChange={(event) => setTemplateType(event.target.value)}
        >
          <option value="General">General</option>
          <option value="Delivery">Delivery</option>
          <option value="Rescue">Rescue</option>
          <option value="Inspection">Inspection</option>
        </select>
      </label>

      <label>
        Battery (%)
        <input
          type="number"
          min="0"
          max="100"
          value={templateBattery}
          onChange={(event) => setTemplateBattery(event.target.value)}
          required
        />
      </label>

      <label>
        Speed
        <input
          type="number"
          min="0.1"
          max="100"
          step="0.1"
          value={templateSpeed}
          onChange={(event) => setTemplateSpeed(event.target.value)}
          required
        />
      </label>

      <label>
        Priority (1–4)
        <select
          value={templatePriority}
          onChange={(event) => setTemplatePriority(Number(event.target.value))}
        >
          <option value={1}>1 - Low</option>
          <option value={2}>2 - Normal</option>
          <option value={3}>3 - High</option>
          <option value={4}>4 - Critical</option>
        </select>
      </label>

      {templateType === "Delivery" && (
        <label>
          Package capacity
          <input
            type="number"
            min="0"
            max="10000"
            value={templateCapacity}
            onChange={(event) => setTemplateCapacity(event.target.value)}
            required
          />
        </label>
      )}

      {templateError && (
        <p className="auth-error">{templateError}</p>
      )}

      <button className="primary-button" disabled={savingTemplate}>
        {savingTemplate ? "Saving..." : "Save template"}
      </button>
    </form>

    <div style={{ marginTop: 28 }}>
      
<div style={{ marginTop: 28 }}>
  <h3>Create robot from template</h3>
  <p>Choose a saved configuration and place the robot on your grid.</p>

  <form onSubmit={createRobotFromTemplate} className="template-form">
    <label>
      Saved template
      <select
        value={selectedTemplateId}
        onChange={(event) => setSelectedTemplateId(event.target.value)}
        required
      >
        <option value="">Select a template</option>
        {templates.map((template) => (
          <option key={template.id} value={String(template.id)}>
            {template.template_name} ({template.robot_type})
          </option>
        ))}
      </select>
    </label>

    <label>
      Robot name
      <input
        value={newRobotName}
        onChange={(event) => setNewRobotName(event.target.value)}
        maxLength={50}
        placeholder="e.g. Delivery-03"
      />
    </label>

    <label>
      Starting X
      <input
        type="number"
        min="0"
        max={cols - 1}
        value={newRobotX}
        onChange={(event) => setNewRobotX(event.target.value)}
        required
      />
    </label>

    <label>
      Starting Y
      <input
        type="number"
        min="0"
        max={rows - 1}
        value={newRobotY}
        onChange={(event) => setNewRobotY(event.target.value)}
        required
      />
    </label>

    <button
      className="primary-button"
      disabled={templates.length === 0}
    >
      Create robot
    </button>
  </form>
</div>

      <h3>Saved templates</h3>

      {templatesLoading && <p>Loading templates...</p>}

      {!templatesLoading && templates.length === 0 && (
        <p className="notice">No saved templates yet.</p>
      )}

      <div className="template-list">
        {templates.map((template) => (
          <div className="template-card" key={template.id}>
            <h4>{template.template_name}</h4>
            <p>Type: {template.robot_type}</p>
            <p>Battery: {template.battery}%</p>
            <p>Speed: {template.speed}</p>
            <p>Priority: {template.priority}</p>
            <p>
              Attributes: {JSON.stringify(template.attributes)}
            </p>
          </div>
        ))}
      </div>
    </div>
  </section>
)}


        {activePage === "History" && (
          <section className="panel history-placeholder">
            <div className="history-icon">◷</div>
            <h3>Simulation history</h3>
            <p>Your backend already has history APIs. We'll connect this page after the dashboard and grid editor are ready.</p>
            <button className="primary-button" onClick={() => setActivePage("Grid Simulator")}>Open grid simulator</button>
          </section>
        )}
      </main>
    </div>
  );
}

function StatCard({ label, value, detail, icon }) {
  return (
    <div className="stat-card">
      <div className="stat-top"><span>{label}</span><b>{icon}</b></div>
      <h2>{value}</h2>
      <p>{detail}</p>
    </div>
  );
}

function GridPreview({
  rows,
  cols,
  robots,
  obstacles,
  onCellClick,
  interactive,
  selectedRobotId,
  onRobotSelect,
}) {
  const robotAt = (x, y) => robots.find((robot) => robot.x === x && robot.y === y);
  const obstacleAt = (x, y) => obstacles.some((cell) => cell.x === x && cell.y === y);

  return (
    <div className={`grid-viewport ${interactive ? "interactive-grid" : ""}`}>
      <div
        className="grid-board"
        style={{ gridTemplateColumns: `repeat(${cols}, minmax(0, 1fr))` }}
      >
        {Array.from({ length: rows * cols }, (_, index) => {
          const x = index % cols;
          const y = Math.floor(index / cols);
          const robot = robotAt(x, y);
          const obstacle = obstacleAt(x, y);

          return (
            <button
              key={`${x}-${y}`}
              className={`grid-cell ${
  robot
    ? `robot-cell ${robot.id === selectedRobotId ? "selected-robot-cell" : ""}`
    : obstacle
      ? "obstacle-cell"
      : ""
}`}
              onClick={() => {
  if (robot && onRobotSelect) {
    onRobotSelect(robot);
  } else if (interactive && onCellClick) {
    onCellClick(x, y);
  }
}}
              title={`Row ${y}, column ${x}`}
              disabled={interactive ? false : !robot}            >
              {robot ? <span className="robot-marker">{robot.name.slice(0, 1)}</span>
                : obstacle ? <span className="obstacle-marker">×</span> : null}
            </button>
          );
        })}
      </div>
    </div>
  );
}



function RobotList({
  robots,
  onDelete,
  deletingRobotId,
  onEdit,
}) {
  if (!robots.length) {
    return <p className="empty-state">No robots added yet.</p>;
  }

  return (
    <div className="robot-list">
      {robots.map((robot) => (
        <div className="robot-row" key={robot.id}>
          <div className="robot-avatar">
            {robot.name.slice(0, 1)}
          </div>

          <div className="robot-details">
            <strong>{robot.name}</strong>
            <p>
              {robot.type} · ID {robot.id} · ({robot.x}, {robot.y})
            </p>
          </div>

          <div className="robot-battery">
            <div className="battery-track">
              <span
                style={{ width: `${robot.battery}%` }}
              />
            </div>
            <small>{robot.battery}%</small>
          </div>

          <button
            type="button"
            className="edit-button"
            onClick={() => onEdit(robot)}
          >
            Edit
          </button>

          <button
            type="button"
            className="delete-button"
            onClick={() => onDelete(robot.id)}
            disabled={deletingRobotId === robot.id}
          >
            {deletingRobotId === robot.id
              ? "Deleting..."
              : "Delete"}
          </button>
        </div>
      ))}
    </div>
  );
}


export default App;
