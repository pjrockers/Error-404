
import { useEffect, useState, useRef  } from "react";
import Auth from "./Auth";
import "./App.css";
import {
  getCurrentUser,
  logoutUser,
  getRobotTemplates,
  saveRobotTemplate,
  updateRobotTemplate,
  deleteRobotTemplate,
  getRobots,
  createRobot,
  updateRobot,
  deleteRobot,
  configureSimulation,
  startSimulation,
  pauseSimulation,
  resumeSimulation,
  resetSimulation,
  getSimulationState,
  saveSimulationHistory,
getSimulationHistory,
setPlaybackSpeed,
} from "./api";

const initialRobots = [
  { id: 1, name: "Alpha", type: "Delivery", x: 1, y: 1, battery: 100, status: "Idle" },
  { id: 2, name: "Beta", type: "Rescue", x: 6, y: 4, battery: 85, status: "Idle" },
];

function App() {
const [activePage, setActivePage] = useState("Dashboard");
const [templates, setTemplates] = useState([]);
const [templatesLoading, setTemplatesLoading] = useState(false);
const [templateError, setTemplateError] = useState("");
const [templateName, setTemplateName] = useState("");
const [templateType, setTemplateType] = useState("Delivery");
const [customAttributes, setCustomAttributes] = useState([
  { key: "", value: "" },
]);
const [customRobotType, setCustomRobotType] = useState("");
const [templateBattery, setTemplateBattery] = useState(100);
const [templateSpeed, setTemplateSpeed] = useState(5);
const [templatePriority, setTemplatePriority] = useState(2);
const [templateCapacity, setTemplateCapacity] = useState(10);
const [savingTemplate, setSavingTemplate] = useState(false);
const [editingTemplateId, setEditingTemplateId] = useState(null);
const [deletingRobotId, setDeletingRobotId] = useState(null); 
  const [token, setToken] = useState(
    () => localStorage.getItem("robotSwarmToken")
  );
  const [user, setUser] = useState(null);
  const [authChecking, setAuthChecking] = useState(true);
  
  const [simulationStatus, setSimulationStatus] = useState("not_configured");
  const [playbackDelay, setPlaybackDelay] = useState(500);
const [playbackSpeedLoading, setPlaybackSpeedLoading] = useState(false);

  const [history, setHistory] = useState([]);
  const [historyLoading, setHistoryLoading] = useState(false);
  const [historyError, setHistoryError] = useState("");

  const historySavedRef = useRef(false);
  const startingPositionsRef = useRef([]);
  const [robotCreationError, setRobotCreationError] = useState("");
  
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
async function loadHistory() {
  if (!token) return;

  setHistoryLoading(true);
  setHistoryError("");

  try {
    const data = await getSimulationHistory(token);

    if (!Array.isArray(data)) {
      throw new Error("Unexpected history data from server.");
    }

    setHistory(data);
  } catch (error) {
    setHistoryError(
      error.message || "Could not load simulation history."
    );
  } finally {
    setHistoryLoading(false);
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
function addCustomAttribute() {
  setCustomAttributes((previous) => [
    ...previous,
    { key: "", value: "" },
  ]);
}

function updateCustomAttribute(index, field, value) {
  setCustomAttributes((previous) =>
    previous.map((attribute, i) =>
      i === index
        ? { ...attribute, [field]: value }
        : attribute
    )
  );
}

function removeCustomAttribute(index) {
  setCustomAttributes((previous) =>
    previous.filter((_, i) => i !== index)
  );
}
function handleEditTemplate(template) {
  setEditingTemplateId(template.id);

  setTemplateName(template.template_name);

  const builtInTypes = [
    "General",
    "Delivery",
    "Rescue",
    "Inspection",
  ];

  if (builtInTypes.includes(template.robot_type)) {
    setTemplateType(template.robot_type);
    setCustomRobotType("");
  } else {
    setTemplateType("Custom");
    setCustomRobotType(template.robot_type);
  }

  setTemplateBattery(template.battery);
  setTemplateSpeed(template.speed);
  setTemplatePriority(template.priority);

  const attributes = template.attributes || {};

  setTemplateCapacity(attributes.packageCapacity ?? 10);

  setCustomAttributes(
    Object.entries(attributes)
      .filter(([key]) => key !== "packageCapacity")
      .map(([key, value]) => ({
        key,
        value: String(value),
      }))
  );

  setTemplateError("");
}
async function handleDeleteTemplate(template) {
  const confirmed = window.confirm(
    `Are you sure you want to delete "${template.template_name}"?`
  );

  if (!confirmed) {
    return;
  }

  try {
    setTemplateError("");

    await deleteRobotTemplate(token, template.id);

    await loadTemplates();
  } catch (error) {
    setTemplateError(
      error.message || "Could not delete robot template."
    );
  }
}
async function handleSaveTemplate(event) {
  event.preventDefault();
  setTemplateError("");

  if (!templateName.trim()) {
    setTemplateError("Enter a template name.");
    return;
  }

  const finalRobotType =
    templateType === "Custom"
      ? customRobotType.trim()
      : templateType;

  if (!finalRobotType) {
    setTemplateError("Enter a custom robot type.");
    return;
  }

  if (finalRobotType.length > 50) {
    setTemplateError("Robot type cannot exceed 50 characters.");
    return;
  }

  // Build the attributes object
  const attributes = {};

  for (const attribute of customAttributes) {
    const key = attribute.key.trim();
    const value = attribute.value.trim();

    // Ignore completely empty rows
    if (!key && !value) {
      continue;
    }

    if (!key || !value) {
      setTemplateError("Complete both fields for every attribute.");
      return;
    }

    if (Object.prototype.hasOwnProperty.call(attributes, key)) {
      setTemplateError(`Duplicate attribute name: ${key}`);
      return;
    }

    // Convert numeric values into numbers
    if (value !== "" && !isNaN(value)) {
      attributes[key] = Number(value);
    } else if (value.toLowerCase() === "true") {
      attributes[key] = true;
    } else if (value.toLowerCase() === "false") {
      attributes[key] = false;
    } else {
      attributes[key] = value;
    }
  }

  // Preserve Delivery's existing package capacity field
  if (finalRobotType === "Delivery") {
    attributes.packageCapacity = Number(templateCapacity);
  }

  setSavingTemplate(true);

  try {
    const templateData = {
  template_name: templateName.trim(),
  robot_type: finalRobotType,
  battery: Number(templateBattery),
  speed: Number(templateSpeed),
  priority: Number(templatePriority),
  attributes,
};

if (editingTemplateId !== null) {
  await updateRobotTemplate(
    token,
    editingTemplateId,
    templateData
  );
} else {
  await saveRobotTemplate(token, templateData);
}

    setTemplateName("");
    setCustomRobotType("");
    setTemplateType("Delivery");
    setCustomAttributes([{ key: "", value: "" }]);
    setEditingTemplateId(null);

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
  setEditingRobot(robot);
  setEditRobotName(robot.name);
  setEditCustomAttributes(
    Object.entries(robot.attributes || {}).map(([key, value]) => ({
      key,
      value: String(value),
    }))
  );
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

  const [rows, setRows] = useState(10);
  const [cols, setCols] = useState(10);
  const [robots, setRobots] = useState(initialRobots);
  const [robotsLoading, setRobotsLoading] = useState(false);
  const [robotsError, setRobotsError] = useState("");
  const [editingRobot, setEditingRobot] = useState(null);
  const [editRobotName, setEditRobotName] = useState("");
  const [savingRobotEdit, setSavingRobotEdit] = useState(false);
  const [editCustomAttributes, setEditCustomAttributes] = useState([]);
  const [selectedRobotId, setSelectedRobotId] = useState(null);
  const [obstacles, setObstacles] = useState([]);
  const [selectedTool, setSelectedTool] = useState("obstacle");
  const [running, setRunning] = useState(false);
  // Real backend simulation state
const [simulationStep, setSimulationStep] = useState(0);
const [simulationError, setSimulationError] = useState("");
const [simulationFinished, setSimulationFinished] = useState(false);
const [simulationLoading, setSimulationLoading] = useState(false);
const [simulationRobots, setSimulationRobots] = useState([]);
// Destination coordinates for each robot
const [robotDestinations, setRobotDestinations] = useState({});
  const [selectedTemplateId, setSelectedTemplateId] = useState("");
  const [newRobotName, setNewRobotName] = useState("");
  const [newRobotX, setNewRobotX] = useState(0);
  const [newRobotY, setNewRobotY] = useState(0);

useEffect(() => {
  if (!token || !["running", "paused"].includes(simulationStatus)) {
    return;
  }

  let cancelled = false;

  const pollSimulation = async () => {
    try {
      const data = await getSimulationState(token);

      if (cancelled) return;

      // Save the simulation only once when it finishes.
      if (
        data.finished === true &&
        !historySavedRef.current
      ) {
        // Set the guard immediately to prevent duplicate POST requests.
        historySavedRef.current = true;

        const finalRobots = Array.isArray(data.robots)
          ? data.robots
          : [];

        const robotPaths = Array.isArray(data.paths) ? data.paths : [];

const result = [
  `Final status: ${data.status}`,
  `Total steps: ${data.current_step}`,
  "",
  "Robot results:",
  ...finalRobots.map((robot, index) => {
    const start = startingPositionsRef.current.find(
      (item) => item.id === Number(robot.id)
    );

    const startPosition = start
      ? `(${start.x}, ${start.y})`
      : "Not recorded";

    const finalPosition = `(${robot.x}, ${robot.y})`;

    const path = robotPaths[index] || [];

    const pathText = path
      .map((cell) => `(${cell.x}, ${cell.y})`)
      .join(" -> ");

    return [
      `${robot.name}:`,
      `  Starting position: ${startPosition}`,
      `  Final position: ${finalPosition}`,
      `  Movement path: ${pathText || "Not recorded"}`,
      `  Status: ${robot.status}`,
      `  Battery: ${robot.battery}%`,
    ].join("\n");
  }),
].join("\n");

        try {
          await saveSimulationHistory(token, {
            simulation_name: `Simulation ${new Date().toLocaleString()}`,
            grid_rows: Number(rows),
            grid_cols: Number(cols),
            robot_count: robots.length,
            result: result,
          });

          // Refresh history after successfully saving.
          const historyData = await getSimulationHistory(token);

          if (!cancelled && Array.isArray(historyData)) {
            setHistory(historyData);
          }
        } catch (error) {
            historySavedRef.current = false;
          if (!cancelled) {
            setHistoryError(
              error.message || "Could not save simulation history."
            );
          }
        }
      }

      if (!cancelled) {
        applySimulationState(data);
      }
    } catch (error) {
      if (!cancelled) {
        setSimulationError(
          error.message || "Could not retrieve simulation state."
        );
      }
    }
  };

  pollSimulation();

  const intervalId = setInterval(pollSimulation, 500);

  return () => {
    cancelled = true;
    clearInterval(intervalId);
  };
}, [token, simulationStatus]);

useEffect(() => {
  if (token && user && activePage === "History") {
    loadHistory();
  }
}, [token, user, activePage]);

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



function applySimulationState(data) {
  setSimulationStatus(data.status || "unknown");
  setSimulationStep(Number(data.current_step || 0));
  setSimulationFinished(Boolean(data.finished));
  setSimulationError(data.error || "");

  if (Array.isArray(data.robots)) {
    setSimulationRobots(data.robots);
  }

  setRunning(data.status === "running");
}

async function handleConfigureSimulation() {
  if (!token) return;

  if (robots.length === 0) {
    setSimulationError("Add at least one robot before configuring the simulation.");
    return;
  }
for (const robot of robots) {
  const destinations = robotDestinations[robot.id] || [
    { x: "", y: "" },
  ];

  for (let i = 0; i < destinations.length; i++) {
    const destination = destinations[i];

    if (
      destination.x === "" ||
      destination.y === "" ||
      destination.x === undefined ||
      destination.y === undefined
    ) {
      setSimulationError(
        `Complete Destination ${i + 1} for ${robot.name}.`
      );
      return;
    }

    const x = Number(destination.x);
    const y = Number(destination.y);

    if (
      !Number.isInteger(x) ||
      !Number.isInteger(y) ||
      x < 0 || x >= cols ||
      y < 0 || y >= rows
    ) {
      setSimulationError(
        `Invalid coordinates for Destination ${i + 1} of ${robot.name}.`
      );
      return;
    }
  }
}

const configuredRobots = robots.map((robot) => {
  const destinations = (
    robotDestinations[robot.id] || [{ x: "", y: "" }]
  ).map((destination) => ({
    x: Number(destination.x),
    y: Number(destination.y),
  }));

  return {
    id: Number(robot.id),
    name: robot.name,
    type: robot.type || "General",
    x: Number(robot.x),
    y: Number(robot.y),

    // First destination (for backward compatibility)
    destX: destinations[0].x,
    destY: destinations[0].y,

    // Complete destination sequence
    destinations,

    battery: Number(robot.battery),
    speed: Number(robot.speed ?? 5),
    priority: Number(robot.priority ?? 1),
  };
});
  startingPositionsRef.current = configuredRobots.map((robot) => ({
  id: robot.id,
  name: robot.name,
  x: robot.x,
  y: robot.y,
}));

  setSimulationLoading(true);
  setSimulationError("");

  try {
    historySavedRef.current = false;
    const data = await configureSimulation(token, {
      rows: Number(rows),
      cols: Number(cols),
      obstacles: obstacles.map((cell) => ({
        x: Number(cell.x),
        y: Number(cell.y),
      })),
      robots: configuredRobots,
    });

    applySimulationState(data);
  } catch (error) {
    setSimulationError(error.message || "Could not configure simulation.");
  } finally {
    setSimulationLoading(false);
  }
}

async function handleStartSimulation() {
  setSimulationLoading(true);
  setSimulationError("");

  try {
    const data = await startSimulation(token);
    applySimulationState(data);
  } catch (error) {
    setSimulationError(error.message || "Could not start simulation.");
  } finally {
    setSimulationLoading(false);
  }
}

async function handlePauseSimulation() {
  setSimulationLoading(true);

  try {
    const data = await pauseSimulation(token);
    applySimulationState(data);
  } catch (error) {
    setSimulationError(error.message || "Could not pause simulation.");
  } finally {
    setSimulationLoading(false);
  }
}

async function handleResumeSimulation() {
  setSimulationLoading(true);

  try {
    const data = await resumeSimulation(token);
    applySimulationState(data);
  } catch (error) {
    setSimulationError(error.message || "Could not resume simulation.");
  } finally {
    setSimulationLoading(false);
  }
}

async function handleResetSimulation() {
  setSimulationLoading(true);
  setSimulationError("");

  try {
    const data = await resetSimulation(token);
    applySimulationState(data);
  } catch (error) {
    setSimulationError(error.message || "Could not reset simulation.");
  } finally {
    setSimulationLoading(false);
  }
}
async function handlePlaybackSpeedChange(event) {
  const delay = Number(event.target.value);

  setPlaybackSpeedLoading(true);
  setSimulationError("");

  try {
    await setPlaybackSpeed(token, delay);
    setPlaybackDelay(delay);
  } catch (error) {
    setSimulationError(
      error.message || "Could not change playback speed."
    );
  } finally {
    setPlaybackSpeedLoading(false);
  }
}
function updateRobotDestination(robotId, index, field, value) {
  setRobotDestinations((current) => {
    const destinations = [
      ...(current[robotId] || [{ x: "", y: "" }]),
    ];

    destinations[index] = {
      ...destinations[index],
      [field]: value,
    };

    return {
      ...current,
      [robotId]: destinations,
    };
  });
}

function addRobotDestination(robotId) {
  setRobotDestinations((current) => ({
    ...current,
    [robotId]: [
      ...(current[robotId] || [{ x: "", y: "" }]),
      { x: "", y: "" },
    ],
  }));
}

function removeRobotDestination(robotId, index) {
  setRobotDestinations((current) => {
    const destinations = [
      ...(current[robotId] || [{ x: "", y: "" }]),
    ];

    if (destinations.length <= 1) return current;

    destinations.splice(index, 1);

    return {
      ...current,
      [robotId]: destinations,
    };
  });
}
async function createRobotFromTemplate(event) {
  event.preventDefault();

  setRobotCreationError("");

  const template = templates.find(
    (item) => String(item.id) === String(selectedTemplateId)
  );

  if (!template) {
    setRobotCreationError("Please select a saved template.");
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
    setRobotCreationError("Starting coordinates must be inside the grid.");
    return;
  }

  if (obstacles.some((cell) => cell.x === x && cell.y === y)) {
    setRobotCreationError("A robot cannot start on an obstacle.");
    return;
  }

  if (robots.some((robot) => robot.x === x && robot.y === y)) {
    setRobotCreationError("Another robot already occupies that cell.");
    return;
  }

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
    setRobotCreationError("");

  } catch (error) {
    setRobotCreationError(
      error.message || "Could not create robot."
    );
  }
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
              <StatCard label="SIMULATION" value={simulationStatus}
                detail={`Step ${simulationStep}`} icon="▷" />
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
      
      

      <div className="inspector-grid">
        {Object.entries(selectedRobot.attributes).map(
          ([key, value]) => (
            <div key={key}>
              <span>{key}</span>
              <strong>{String(value)}</strong>
            </div>
          )
        )}
      </div>
    </div>
  )
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
              <span
  className={`status-pill ${
    simulationStatus === "configured" ||
    simulationStatus === "running"
      ? "running"
      : ""
  }`}
>
  <i /> {simulationStatus}
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
              <label>
  Playback speed
  <select
    value={playbackDelay}
    onChange={handlePlaybackSpeedChange}
    disabled={
      playbackSpeedLoading ||
      !["configured", "running", "paused"].includes(simulationStatus)
    }
  >
    <option value={1000}>Slow (1 step/sec)</option>
    <option value={500}>Normal (2 steps/sec)</option>
    <option value={200}>Fast (5 steps/sec)</option>
  </select>
</label>
            </div>
            <div className="destination-panel">
  <div className="panel-heading">
    <div>
      <h3>Set Robot Destinations</h3>
      <p>Choose the target coordinates for each robot.</p>
    </div>
  </div>

  <div className="destination-list">
  {robots.map((robot) => {
    const destinations = robotDestinations[robot.id] || [
      { x: "", y: "" },
    ];

    return (
      <div className="destination-row" key={robot.id}>
        <div className="destination-robot">
          <strong>{robot.name}</strong>
          <span>{robot.type} Robot · ID {robot.id}</span>
        </div>

        {destinations.map((destination, index) => (
          <div className="destination-waypoint" key={index}>
            <strong>Destination {index + 1}</strong>

            <label>
              X coordinate
              <input
                type="number"
                min="0"
                max={cols - 1}
                placeholder="X"
                value={destination.x}
                disabled={["running", "paused"].includes(simulationStatus)}
                onChange={(event) =>
                  updateRobotDestination(
                    robot.id,
                    index,
                    "x",
                    event.target.value
                  )
                }
              />
            </label>

            <label>
              Y coordinate
              <input
                type="number"
                min="0"
                max={rows - 1}
                placeholder="Y"
                value={destination.y}
                disabled={["running", "paused"].includes(simulationStatus)}
                onChange={(event) =>
                  updateRobotDestination(
                    robot.id,
                    index,
                    "y",
                    event.target.value
                  )
                }
              />
            </label>

            {destinations.length > 1 && (
              <button
                type="button"
                className="delete-button"
                disabled={["running", "paused"].includes(simulationStatus)}
                onClick={() =>
                  removeRobotDestination(robot.id, index)
                }
              >
                Remove
              </button>
            )}
          </div>
        ))}

        <button
          type="button"
          className="secondary-button"
          disabled={["running", "paused"].includes(simulationStatus)}
          onClick={() => addRobotDestination(robot.id)}
        >
          + Add Destination
        </button>
      </div>
    );
  })}
</div>
</div>
{simulationError && (
  <p className="auth-error">
    {simulationError}
  </p>
)}

            <GridPreview
              rows={rows}
              cols={cols}
              robots={simulationRobots.length > 0 ? simulationRobots : robots}
              obstacles={obstacles}
              onCellClick={handleCellClick}
              interactive
            />
            {simulationLoading && (
  <p>Processing simulation...</p>
)}

            <div className="simulator-footer">
              <div className="legend">
                <span><i className="legend-robot" /> Robot</span>
                <span><i className="legend-obstacle" /> Obstacle</span>
                <span><i className="legend-empty" /> Free cell</span>
              </div>
              <div className="action-row">
  <button
    className="secondary-button"
    onClick={handleResetSimulation}
    disabled={simulationLoading || simulationStatus === "not_configured"}
  >
    Reset
  </button>

  <button
    className="secondary-button"
    onClick={handleConfigureSimulation}
    disabled={simulationLoading || simulationStatus === "running" || simulationStatus === "paused"}
  >
    Configure
  </button>

  {simulationStatus === "running" ? (
    <button
      className="primary-button"
      onClick={handlePauseSimulation}
      disabled={simulationLoading}
    >
      Pause
    </button>
  ) : simulationStatus === "paused" ? (
    <button
      className="primary-button"
      onClick={handleResumeSimulation}
      disabled={simulationLoading}
    >
      Resume
    </button>
  ) : (
    <button
      className="primary-button"
      onClick={handleStartSimulation}
      disabled={simulationLoading || simulationStatus !== "configured"}
    >
      Start simulation
    </button>
  )}
</div>
            </div>
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

      const attributes = {};

for (const attribute of editCustomAttributes) {
  const key = attribute.key.trim();
  const value = attribute.value.trim();

  if (!key && !value) {
    continue;
  }

  if (!key || !value) {
    setRobotsError("Complete both fields for every attribute.");
    return;
  }

  if (Object.prototype.hasOwnProperty.call(attributes, key)) {
    setRobotsError(`Duplicate attribute name: ${key}`);
    return;
  }

  if (!isNaN(value)) {
    attributes[key] = Number(value);
  } else if (value.toLowerCase() === "true") {
    attributes[key] = true;
  } else if (value.toLowerCase() === "false") {
    attributes[key] = false;
  } else {
    attributes[key] = value;
  }
}

const updatedRobot = {
  ...editingRobot,
  name: editRobotName.trim(),
  battery: Number(editingRobot.battery),
  speed: Number(editingRobot.speed),
  priority: Number(editingRobot.priority),
  x: Number(editingRobot.x),
  y: Number(editingRobot.y),
  attributes,
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
        setEditCustomAttributes([]);
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
   <div className="custom-attributes-section">
  <h4>Custom Attributes</h4>

  {customAttributes.map((attribute, index) => (
    <div key={index} className="custom-attribute-row">
      <input
        type="text"
        placeholder="Attribute name"
        value={attribute.key}
        onChange={(event) =>
          updateCustomAttribute(index, "key", event.target.value)
        }
      />

      <input
        type="text"
        placeholder="Attribute value"
        value={attribute.value}
        onChange={(event) =>
          updateCustomAttribute(index, "value", event.target.value)
        }
      />

      <button
        type="button"
        className="delete-button"
        onClick={() => removeCustomAttribute(index)}
      >
        Remove
      </button>
    </div>
  ))}

  <button
    type="button"
    className="secondary-button add-attribute-button"
    onClick={addCustomAttribute}
  >
    + Add Attribute
  </button>
</div>
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
          setEditCustomAttributes([]);
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
   <div className="custom-attributes-section">
  <h4>Custom Attributes</h4>

  {customAttributes.map((attribute, index) => (
    <div key={index} className="custom-attribute-row">
      <input
        type="text"
        placeholder="Name"
        value={attribute.key}
        onChange={(event) =>
          updateCustomAttribute(index, "key", event.target.value)
        }
      />

      <input
        type="text"
        placeholder="Value"
        value={attribute.value}
        onChange={(event) =>
          updateCustomAttribute(index, "value", event.target.value)
        }
      />

      <button
        type="button"
        className="delete-button"
        onClick={() => removeCustomAttribute(index)}
      >
        Remove
      </button>
    </div>
  ))}

  <button
    type="button"
    className="secondary-button add-attribute-button"
    onClick={addCustomAttribute}
  >
    + Add Attribute
  </button>
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
    <option value="Custom">Custom</option>
  </select>
</label>

{templateType === "Custom" && (
  <label>
    Custom robot type name
    <input
      type="text"
      value={customRobotType}
      onChange={(event) =>
        setCustomRobotType(event.target.value)
      }
      maxLength={50}
      placeholder="e.g. Agriculture, Medical, Surveillance"
      required
    />
  </label>
)}

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
  {savingTemplate
    ? "Saving..."
    : editingTemplateId !== null
      ? "Save Changes"
      : "Save Template"}
</button>

{editingTemplateId !== null && (
  <button
    type="button"
    className="delete-button"
    onClick={() => {
      setEditingTemplateId(null);
      setTemplateName("");
      setTemplateType("Delivery");
      setCustomRobotType("");
      setTemplateBattery(100);
      setTemplateSpeed(5);
      setTemplatePriority(2);
      setTemplateCapacity(10);
      setCustomAttributes([{ key: "", value: "" }]);
      setTemplateError("");
    }}
  >
    Cancel Editing
  </button>
)}
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
    {robotCreationError && (
  <p className="auth-error">
    {robotCreationError}
  </p>
)}

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
            {Object.entries(template.attributes || {}).map(([key, value]) => (
  <p key={key}>
    {key}: {String(value)}
  </p>
))}
<button
  type="button"
  className="edit-button"
  onClick={() => handleEditTemplate(template)}
>
  Edit Template
</button>
<button
  type="button"
  className="delete-button"
  onClick={() => handleDeleteTemplate(template)}
>
  Delete Template
</button>
          </div>
        ))}
      </div>
    </div>
  </section>
)}


        
{activePage === "History" && (
  <section className="panel history-panel">
    <div className="panel-heading">
      <div>
        <h3>Simulation history</h3>
        <p>View your previously completed simulations.</p>
      </div>

      <button
        className="secondary-button"
        onClick={loadHistory}
        disabled={historyLoading}
      >
        {historyLoading ? "Refreshing..." : "Refresh"}
      </button>
    </div>

    {historyError && (
      <p className="auth-error">{historyError}</p>
    )}

    {historyLoading && (
      <p>Loading simulation history...</p>
    )}

    {!historyLoading && !historyError && history.length === 0 && (
      <div className="empty-state">
        <div className="history-icon">◷</div>
        <h3>No simulations yet</h3>
        <p>Completed simulations will appear here.</p>
        <button
          className="primary-button"
          onClick={() => setActivePage("Grid Simulator")}
        >
          Open simulator
        </button>
      </div>
    )}

    {!historyLoading && history.length > 0 && (
      <div className="history-list">
        {history.map((item) => (
          <div className="template-card" key={item.id}>
            <div className="panel-heading">
              <div>
                <h4>{item.simulation_name}</h4>
                <p>{item.created_at}</p>
              </div>
              <span className="status-pill">
                Completed
              </span>
            </div>

            <div className="inspector-grid">
              <div>
                <span>Grid size</span>
                <strong>
                  {item.grid_rows} × {item.grid_cols}
                </strong>
              </div>

              <div>
                <span>Robots</span>
                <strong>{item.robot_count}</strong>
              </div>

              <div>
                <span>Simulation ID</span>
                <strong>#{item.id}</strong>
              </div>
            </div>

            
<div className="history-visual-section">
  <h4>Robot positions</h4>

  <div className="history-grid-comparison">
    <div>
      <h5>Starting positions</h5>
      <HistoryMiniGrid
        rows={Number(item.grid_rows)}
        cols={Number(item.grid_cols)}
        result={item.result}
        positionType="start"
      />
    </div>

    <div>
      <h5>Final positions</h5>
      <HistoryMiniGrid
        rows={Number(item.grid_rows)}
        cols={Number(item.grid_cols)}
        result={item.result}
        positionType="final"
      />
    </div>
  </div>
</div>

<div style={{ marginTop: 16 }}>
  <strong>Simulation result</strong>
  <pre style={{
    whiteSpace: "pre-wrap",
    overflowWrap: "anywhere",
    marginTop: 8
  }}>
    {item.result}
  </pre>
</div>

          </div>
        ))}
      </div>
    )}
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

function HistoryMiniGrid({ rows, cols, result, positionType }) {
  const robotPositions = [];
  const robotPaths = [];

  const matches = [
    ...result.matchAll(
      /([^\n]+):\n\s+Starting position: \((\d+), (\d+)\)\n\s+Final position: \((\d+), (\d+)\)\n\s+Movement path: ([^\n]*)/g
    ),
  ];

  matches.forEach((match, index) => {
    const path = match[6]
      .match(/\((\d+), (\d+)\)/g) || [];

    const cells = path.map((cell) => {
      const coordinates = cell.match(/\((\d+), (\d+)\)/);

      return {
        x: Number(coordinates[1]),
        y: Number(coordinates[2]),
      };
    });

    robotPaths.push({
      name: match[1].trim(),
      cells,
      index,
    });

    robotPositions.push({
      name: match[1].trim(),
      x: Number(positionType === "start" ? match[2] : match[4]),
      y: Number(positionType === "start" ? match[3] : match[5]),
    });
  });

  return (
    <div className="history-mini-grid">
      <div
        className="history-mini-board"
        style={{
          gridTemplateColumns: `repeat(${cols}, minmax(0, 1fr))`,
        }}
      >
        {Array.from({ length: rows * cols }, (_, index) => {
          const x = index % cols;
          const y = Math.floor(index / cols);

          const robot = robotPositions.find(
            (item) => item.x === x && item.y === y
          );

          const trail = robotPaths.find((item) =>
            item.cells.some((cell) => cell.x === x && cell.y === y)
          );

          return (
            <div
              key={`${x}-${y}`}
              className={`history-mini-cell ${
                trail ? `trail-${trail.index % 6}` : ""
              } ${robot ? "occupied" : ""}`}
              title={
                robot
                  ? `${robot.name} (${x}, ${y})`
                  : trail
                    ? `${trail.name}'s path (${x}, ${y})`
                    : `(${x}, ${y})`
              }
            >
              {robot ? robot.name.slice(0, 1).toUpperCase() : ""}
            </div>
          );
        })}
      </div>
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
  const [expandedRobots, setExpandedRobots] = useState({});
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
           {Object.entries(robot.attributes || {}).length > 0 && (
  <div className="robot-custom-attributes">
    {Object.entries(robot.attributes || {})
      .slice(0, expandedRobots[robot.id] ? undefined : 2)
      .map(([key, value]) => (
        <p key={key}>
          {key}: {String(value)}
        </p>
      ))}

    {Object.entries(robot.attributes || {}).length > 2 && (
      <button
        type="button"
        className="show-attributes-button"
        onClick={() =>
          setExpandedRobots((previous) => ({
            ...previous,
            [robot.id]: !previous[robot.id],
          }))
        }
      >
        {expandedRobots[robot.id] ? "Show Less" : "Show More"}
      </button>
    )}
  </div>
)}
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
