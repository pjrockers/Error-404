

extends Node3D

@export var floor_node: MeshInstance3D
@export var obstacle_container: Node3D
var obstacle_nodes: Dictionary = {}
var destination_nodes: Dictionary = {}
var robot_start_grid_positions: Dictionary = {}
var robot_initial_batteries: Dictionary = {}
var robot_ids: Array[String] = []
var robot_battery_labels: Dictionary = {}
var grid_rows: int = 20
var grid_cols: int = 20
var robot_nodes: Array[Node3D] = []
var last_robot_grid_positions: Dictionary = {}
var robot_template: Node3D

var simulation_running = false
var robot_start_positions = {}
var robot_speed = 2.0
var robot_battery = 100.0
var battery_drain_rate = 2.0

func _ready():

	for node in get_children():
		if node is Node3D and node.name.to_lower().begins_with("robot"):
			if not robot_nodes.has(node):
				robot_nodes.append(node)

			robot_start_positions[node] = node.position

	update_battery()
	read_robot_state()
	update_robots_from_cpp()
	

var state_poll_timer = 0.0

func _process(delta):
	if not simulation_running:
		return

	state_poll_timer += delta

	if state_poll_timer >= 0.1:
		state_poll_timer = 0.0
		update_robots_from_cpp()


func _on_button_start_pressed():
	print("START HANDLER CALLED AT: ", Time.get_ticks_msec())
	var file = FileAccess.open(
		"C:/roboSwarmBridge/command.txt",
		FileAccess.WRITE
	)

	if file == null:
		push_error("Could not send Start command to C++.")
		update_status("Start failed")
		return

	file.store_string("START")
	file.close()

	simulation_running = true
	state_poll_timer = 0.0
	update_status("Running")


func _on_pause_pressed():
	simulation_running = false
	update_status("Paused")
	
func _on_reset_pressed():
	simulation_running = false
	state_poll_timer = 0.0
	robot_battery = 100.0

	for i in range(robot_nodes.size()):
		var robot = robot_nodes[i]

		if not is_instance_valid(robot):
			continue

		var robot_id = str(i + 1)
		var start_pos: Vector2i = robot_start_grid_positions.get(
			robot_id, Vector2i(0, 0)
		)

		robot.global_position = grid_to_world(
			start_pos.x, start_pos.y
		)

	update_battery()
	update_status("Ready")

func update_status(message: String):
	var label = get_node_or_null(
        "CanvasLayer/Control/Dashboard/VBoxContainer/StatusLabel"
	)

	if label != null:
		label.text = "Status: " + message
	else:
		print("StatusLabel not found. Check its node path.")



func update_battery():
	var total_battery = 0.0
	var count = 0

	# Calculate average using each robot's battery label
	for robot_id in robot_battery_labels.keys():
		var robot_label = robot_battery_labels[robot_id]

		if is_instance_valid(robot_label):
			var battery_text = robot_label.text
			var battery_value = battery_text.replace("Battery: ", "").replace("%", "")

			total_battery += float(battery_value)
			count += 1

	if count == 0:
		print("No robot battery labels found!")
		return

	robot_battery = total_battery / float(count)

	# Find the EXISTING normal 2D dashboard Label, not a robot's Label3D
	var labels = get_tree().current_scene.find_children("*", "Label", true, false)

	for label in labels:
		if label.text.begins_with("Battery:"):
			label.text = "Battery: %d%%" % roundi(robot_battery)
			print("Average battery updated: ", roundi(robot_battery), "%")
			return

	print("Dashboard battery Label not found!")
	
func read_robot_state():
	var file_path = "C:/roboSwarmBridge/robot_state.json"
	if not FileAccess.file_exists(file_path):
		print("ERROR: Robot state file not found: ", file_path)
		return

	var file = FileAccess.open(file_path, FileAccess.READ)

	if file == null:
		print("ERROR: Could not open robot state file.")
		return

	var json_text = file.get_as_text()
	file.close()

	var json = JSON.new()
	var error = json.parse(json_text)

	if error != OK:
		print("ERROR: Invalid JSON data.")
		print("Line: ", json.get_error_line())
		print("Details: ", json.get_error_message())
		return

	var data = json.data

	if not data.has("robots"):
		print("ERROR: No robots list found.")
		return

	for robot in data["robots"]:
		print(
			"Robot ID: ", robot["id"],
			" | Position: (", robot["x"], ", ", robot["y"], ")",
			" | Destination: (", robot["dest_x"], ", ", robot["dest_y"], ")",
			" | Battery: ", robot["battery"], "%"
		)

func update_robots_from_cpp():
	var file_path = "C:/roboSwarmBridge/robot_state.json"

	if not FileAccess.file_exists(file_path):
		push_warning("JSON file not found: " + file_path)
		return

	var file = FileAccess.open(file_path, FileAccess.READ)
	if file == null:
		push_warning("Could not open JSON file.")
		return

	var json_text = file.get_as_text()
	file.close()

	var json = JSON.new()
	var error = json.parse(json_text)

	if error != OK:
		push_warning(
			"JSON parse error: " + json.get_error_message()
			+ " at line " + str(json.get_error_line())
		)
		return

	var data = json.data

	if not data is Dictionary:
		push_warning("Invalid JSON dictionary.")
		return
	# Display validation errors received from C++.
	var sim_status = str(
	data.get("simulation_status", "ready")
	)

	if sim_status == "route_error":
		simulation_running = false
		
		var error_message = str(
			data.get("error_message", "Route validation failed.")
	)

		update_status(error_message)
		print("SIMULATION ERROR: ", error_message)
# Read grid dimensions FIRST.
	if data.has("grid") and data["grid"] is Dictionary:
		grid_rows = int(data["grid"].get("rows", grid_rows))
		grid_cols = int(data["grid"].get("cols", grid_cols))

	# Resize the floor to match the C++ grid.
		if floor_node != null:
			var plane := floor_node.mesh as PlaneMesh
			if plane != null:
				plane.size = Vector2(grid_cols, grid_rows)
			else:
				push_warning("The floor mesh is not a PlaneMesh.")

# Now place obstacles using the updated grid dimensions.
	if data.has("obstacles") and data["obstacles"] is Array:
		update_obstacles_from_json(data["obstacles"])
	
	if not data is Dictionary or not data.has("robots"):
		push_warning("JSON has no valid robots dictionary.")
		return

	if not data["robots"] is Array:
		push_warning("JSON robots field is not an array.")
		return

	print("Godot read robot data: ", data["robots"])

	var robot_data: Array = data["robots"]


	# Synchronize Godot robot visuals with the JSON robot count.
	sync_robot_nodes(robot_data.size())

	if robot_nodes.is_empty() and not robot_data.is_empty():
		push_warning("No robot visuals available in the scene.")
		return

	robot_ids.clear()

	var count = mini(robot_nodes.size(), robot_data.size())

	for i in range(count):
		var node: Node3D = robot_nodes[i]
		var cpp_robot = robot_data[i]

		if not is_instance_valid(node):
			continue

		if not cpp_robot is Dictionary:
			continue

		var robot_id = str(cpp_robot.get("id", i))
		robot_ids.append(robot_id)

		# Save the starting grid position.
		# Your C++ exporter sets all starting positions to (0, 0).
		var start_x = int(cpp_robot.get("start_x", 0))
		var start_y = int(cpp_robot.get("start_y", 0))

		robot_start_grid_positions[robot_id] = Vector2i(
			start_x,
			start_y
		)

		# Save the initial battery only once per robot ID.
		if cpp_robot.has("battery"):
			if not robot_initial_batteries.has(robot_id):
				robot_initial_batteries[robot_id] = float(
					cpp_robot["battery"]
				)

		
		
		# Update robot visual using the latest C++ coordinates.
		if cpp_robot.has("x") and cpp_robot.has("y"):
			var grid_x: int = int(cpp_robot["x"])
			var grid_y: int = int(cpp_robot["y"])

			if (
				grid_x >= 0 and grid_x < grid_cols
				and grid_y >= 0 and grid_y < grid_rows
			):
				if floor_node == null:
					push_error("floor_node is not assigned in the Inspector.")
				else:
					var new_position: Vector3 = grid_to_world(
						grid_x, grid_y
					)

					print(
						"Robot ", robot_id,
						" | Grid: ", grid_x, ",", grid_y,
						" | World: ", new_position,
						" | Visual: ", node.name
					)

					node.global_position = new_position

		# Read the robot's DESTINATION coordinates.
		var dest_x = int(cpp_robot.get("dest_x", 0))
		var dest_y = int(cpp_robot.get("dest_y", 0))
		update_destination_tile(robot_id, dest_x, dest_y)
		# Convert destination grid coordinates to world coordinates.
		var destination_world: Vector3 = grid_to_world(
			dest_x,
			dest_y
		)

		# Update this robot's individual battery label.
		if cpp_robot.has("battery"):
			update_robot_battery_label(
				node,
				robot_id,
				float(cpp_robot["battery"])
			)
	update_battery()


func update_obstacles_from_json(obstacles: Array) -> void:
	if obstacle_container == null:
		push_warning("Assign DynamicObstacles in the Inspector!")
		return

	# Record the coordinates present in the latest JSON.
	var required_positions: Dictionary = {}

	for obstacle in obstacles:
		if not obstacle is Dictionary:
			continue

		var x: int = int(obstacle.get("x", -1))
		var y: int = int(obstacle.get("y", -1))

		if x < 0 or x >= grid_cols or y < 0 or y >= grid_rows:
			continue

		var cell := Vector2i(x, y)
		required_positions[cell] = true

		# Keep an existing obstacle at this coordinate.
		if obstacle_nodes.has(cell):
			continue

		# Create a new obstacle only if needed.
		var visual := MeshInstance3D.new()
		visual.name = "Obstacle_%d_%d" % [x, y]

		var obstacle_mesh := BoxMesh.new()
		obstacle_mesh.size = Vector3(0.8, 0.4, 0.8)
		visual.mesh = obstacle_mesh

		var material := StandardMaterial3D.new()
		material.albedo_color = Color.RED
		visual.material_override = material

		obstacle_container.add_child(visual)
		visual.global_position = grid_to_world(x, y)
		obstacle_nodes[cell] = visual

	# Remove obstacles missing from the latest JSON.
	for cell in obstacle_nodes.keys():
		if not required_positions.has(cell):
			var old_obstacle = obstacle_nodes[cell]

			if is_instance_valid(old_obstacle):
				old_obstacle.queue_free()

			obstacle_nodes.erase(cell)


func update_destination_tile(
	robot_id: String,
	dest_x: int,
	dest_y: int
) -> void:
	if floor_node == null:
		return

	if dest_x < 0 or dest_x >= grid_cols:
		return
	if dest_y < 0 or dest_y >= grid_rows:
		return

	var plane := floor_node.mesh as PlaneMesh
	if plane == null:
		return

	var floor_width = plane.size.x * floor_node.global_basis.x.length()
	var floor_depth = plane.size.y * floor_node.global_basis.z.length()

	var cell_width = floor_width / float(grid_cols)
	var cell_depth = floor_depth / float(grid_rows)

	var tile: MeshInstance3D = destination_nodes.get(robot_id)

	# Create one tile per robot.
	if not is_instance_valid(tile):
		tile = MeshInstance3D.new()
		tile.name = "Destination_" + robot_id

		var tile_mesh := BoxMesh.new()
		tile_mesh.size = Vector3(
			cell_width * 0.8,
			0.03,
			cell_depth * 0.8
		)
		tile.mesh = tile_mesh

		var material := StandardMaterial3D.new()
		material.albedo_color = Color(0.0, 0.8, 1.0)
		material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		tile.material_override = material

		add_child(tile)
		destination_nodes[robot_id] = tile

	# Position the tile at the destination cell.
	tile.global_position = grid_to_world(dest_x, dest_y)
	tile.global_position.y -= 0.48

	# Resize if the grid dimensions change.
	var tile_mesh := tile.mesh as BoxMesh
	if tile_mesh != null:
		tile_mesh.size = Vector3(
			cell_width * 0.8,
			0.03,
			cell_depth * 0.8
		)


func grid_to_world(x: int, y: int) -> Vector3:
	if floor_node == null:
		return Vector3.ZERO

	var plane := floor_node.mesh as PlaneMesh
	if plane == null:
		return Vector3.ZERO

	var floor_width = plane.size.x * floor_node.global_basis.x.length()
	var floor_depth = plane.size.y * floor_node.global_basis.z.length()

	var cell_width = floor_width / float(grid_cols)
	var cell_depth = floor_depth / float(grid_rows)

	var local_x = -floor_width / 2.0 + (float(x) + 0.5) * cell_width
	var local_z = -floor_depth / 2.0 + (float(y) + 0.5) * cell_depth

	return floor_node.to_global(Vector3(
		local_x / maxf(floor_node.global_basis.x.length(), 0.001),
		0.5,
		local_z / maxf(floor_node.global_basis.z.length(), 0.001)
	))

func sync_robot_nodes(required_count: int) -> void:
	var found_robots: Array[Node3D] = []

	for node in get_children():
		if node is Node3D and node.name.to_lower().begins_with("robot"):
			found_robots.append(node)

	# Keep the first robot as the template.
	if found_robots.is_empty():
		push_error("No robot visual exists to use as a template.")
		return

	var template: Node3D = found_robots[0]

	# Create missing robot visuals.
	while found_robots.size() < required_count:
		var new_robot = template.duplicate() as Node3D
		if new_robot == null:
			break

		add_child(new_robot)
		new_robot.name = "RobotVisual%d" % (found_robots.size() + 1)
		found_robots.append(new_robot)

	# Remove extra robot visuals.
	while found_robots.size() > required_count:
		var extra_robot = found_robots.pop_back()
		if extra_robot != template:
			extra_robot.queue_free()

	robot_nodes = found_robots
	for robot in robot_nodes:
		if is_instance_valid(robot):
			if not robot_start_positions.has(robot):
				robot_start_positions[robot] = robot.position
func update_robot_battery_label(
	robot: Node3D,
	robot_id: String,
	battery: float
) -> void:
	var label: Label3D = robot_battery_labels.get(robot_id)
	if not is_instance_valid(label):
		label = Label3D.new()
		label.name = "BatteryLabel"
		label.position = Vector3(0, 2.0, 0)
		label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
		label.font_size = 32
		label.outline_size = 2
		label.pixel_size = 0.001
		label.fixed_size = true
		label.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR
		robot.add_child(label)
		robot_battery_labels[robot_id] = label  

	label.text = "Battery: %d%%" % roundi(battery)
func reset_robot_positions() -> void:
	simulation_running = false
	state_poll_timer = 0.0

	for i in range(mini(robot_nodes.size(), robot_ids.size())):
		var robot: Node3D = robot_nodes[i]
		var robot_id: String = robot_ids[i]

		if not is_instance_valid(robot):
			continue

		if robot_start_grid_positions.has(robot_id):
			var start: Vector2i = robot_start_grid_positions[robot_id]

			if floor_node != null:
				robot.global_position = grid_to_world(
					start.x,
					start.y
				)

		# Restore the initial battery display.
		if robot_initial_batteries.has(robot_id):
			var initial_battery: float = robot_initial_batteries[robot_id]

			update_robot_battery_label(
				robot,
				robot_id,
				initial_battery
			)

	update_battery()
	update_status("Ready")	
