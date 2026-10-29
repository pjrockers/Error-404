
const API_BASE = "/api";

export async function apiRequest(endpoint, options = {}) {
  const response = await fetch(`${API_BASE}${endpoint}`, {
    ...options,
    headers: {
      "Content-Type": "application/json",
      ...options.headers,
    },
  });

  const text = await response.text();

  let data = {};

  if (text) {
    try {
      data = JSON.parse(text);
    } catch {
      throw new Error("The server returned an invalid response.");
    }
  }

  if (!response.ok) {
    throw new Error(
      data.error || data.message || "Something went wrong."
    );
  }

  return data;
}

export async function registerUser(username, password) {
  return apiRequest("/register", {
    method: "POST",
    body: JSON.stringify({ username, password }),
  });
}

export async function loginUser(username, password) {
  return apiRequest("/login", {
    method: "POST",
    body: JSON.stringify({ username, password }),
  });
}

export async function getCurrentUser(token) {
  return apiRequest("/me", {
    headers: {
      Authorization: `Bearer ${token}`,
    },
  });
}

export async function logoutUser(token) {
  return apiRequest("/logout", {
    method: "POST",
    headers: {
      Authorization: `Bearer ${token}`,
    },
  });
}


export async function getRobotTemplates(token) {
  return apiRequest("/robots/templates", {
    headers: {
      Authorization: `Bearer ${token}`,
    },
  });
}

export async function saveRobotTemplate(token, template) {
  return apiRequest("/robots/templates", {
    method: "POST",
    headers: {
      Authorization: `Bearer ${token}`,
    },
    body: JSON.stringify(template),
  });
}

export async function getRobots(token) {
  return apiRequest("/robots", {
    headers: {
      Authorization: `Bearer ${token}`,
    },
  });
}

export async function createRobot(token, robot) {
  return apiRequest("/robots", {
    method: "POST",
    headers: {
      Authorization: `Bearer ${token}`,
    },
    body: JSON.stringify({
      name: robot.name,
      robot_type: robot.type,
      x: Number(robot.x),
      y: Number(robot.y),
      battery: Number(robot.battery),
      speed: Number(robot.speed ?? 5),
      priority: Number(robot.priority ?? 1),
      status: robot.status ?? "Idle",
      attributes: robot.attributes ?? {},
    }),
  });
}

export async function updateRobot(token, id, robot) {
  return apiRequest(`/robots/${id}`, {
    method: "PUT",
    headers: {
      Authorization: `Bearer ${token}`,
    },
    body: JSON.stringify({
      name: robot.name,
      robot_type: robot.type,
      x: Number(robot.x),
      y: Number(robot.y),
      battery: Number(robot.battery),
      speed: Number(robot.speed ?? 5),
      priority: Number(robot.priority ?? 1),
      status: robot.status ?? "Idle",
      attributes: robot.attributes ?? {},
    }),
  });
}

export async function deleteRobot(token, id) {
  return apiRequest(`/robots/${id}`, {
    method: "DELETE",
    headers: {
      Authorization: `Bearer ${token}`,
    },
  });
}


