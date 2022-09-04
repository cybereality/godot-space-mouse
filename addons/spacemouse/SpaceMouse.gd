# [Godot Space Mouse]
# created by Andres Hernandez
tool
extends EditorPlugin

# space mouse library interface
onready var spacemouse = preload("res://addons/spacemouse/bin/spacemouse.gdns").new()
# scene for the configuration dock
var space_dock = null
# if the device is connected
var connected = false
# currently selected object
var selection = null
# the editor interface
var editor = null
# main editor camera
var camera = null
# temporary camera transform used in updates
var camera_transform = null
# raw values read from the device
var space_translation = Vector3.ZERO
var space_rotation = Vector3.ZERO
# used for pivot point rotation
var select_origin = Vector3.ZERO
var camera_origin = Vector3.ZERO
# translation speed slows down when close
var offset_speed = 1.0;
# convert raw values into godot axis format
const flip = Vector3(-1.0, 1.0, -1.0)
# default speeds (modified by ui)
const base_translate_speed = 0.16
const base_rotate_speed = 0.0275
# main speed control for motion updates
var translate_speed = base_translate_speed
var rotate_speed = base_rotate_speed
# sets viewport camera to the first one
const camera_index = 0
# node to access configuration ui
var translation_speed_ui = null
var rotation_speed_ui = null
var control_type_ui = null
# name for control type
const ControlType = {OBJECT_TYPE = 0, CAMERA_TYPE = 1, HELICOPTER_TYPE=2}
# object or camera control
var control_type = ControlType.OBJECT_TYPE
# adjust fly camera translation
const adjust_translation = 0.24
const adjust_rotation = 0.98
# stored configuration
var config_data = {}

# on start add the configuration dock
func _enter_tree():
	# instance the dock
	space_dock = preload("res://addons/spacemouse/SpaceDock.tscn").instance()
	# attach signals for ui controls
	translation_speed_ui = space_dock.get_node("UI/TranslationSpeed")
	translation_speed_ui.connect("value_changed", self, "update_config")
	rotation_speed_ui = space_dock.get_node("UI/RotationSpeed")
	rotation_speed_ui.connect("value_changed", self, "update_config")
	control_type_ui = space_dock.get_node("UI/ControlType")
	control_type_ui.connect("item_selected", self, "update_config")
	# add ui dock to the editor slot
	add_control_to_dock(EditorPlugin.DOCK_SLOT_RIGHT_BL, space_dock)
	# load the saved configuration
	load_config_data()

# update the controls on ui changes
func update_config(val):
	# scale the translation and rotation speeds
	translate_speed = base_translate_speed * (translation_speed_ui.value * 0.01)
	rotate_speed = base_rotate_speed * (rotation_speed_ui.value * 0.01)
	# toggle camera control type
	control_type = control_type_ui.selected
	
# save current configuration
func save_config_data():
	# get the values for from the ui
	config_data["translation_speed"] = translation_speed_ui.value
	config_data["rotation_speed"] = rotation_speed_ui.value
	config_data["control_type"] = control_type_ui.selected
	# open the configuration save file
	var config_file = File.new()
	config_file.open("user://spacemouse.config", File.WRITE)
	# write the data for the configuration
	config_file.store_var(config_data)

# load saved settings
func load_config_data():
	# attempt to load and exit is file not found
	var config_file = File.new()
	if not config_file.file_exists("user://spacemouse.config"):
		return
	config_file.open("user://spacemouse.config", File.READ)
	# store the loaded config values
	config_data = config_file.get_var()
	# set the ui to the loaded config
	if config_data.has_all(["translation_speed", "rotation_speed", "control_type"]):
		translation_speed_ui.value = config_data["translation_speed"]
		rotation_speed_ui.value = config_data["rotation_speed"]
		control_type_ui.selected = config_data["control_type"]
	
# cleanup on exit
func _exit_tree():
	# save the settings
	save_config_data()
	# remove the dock
	remove_control_from_docks(space_dock)
	# free memory
	if is_instance_valid(space_dock):
		space_dock.queue_free()

# setup global parameters and connect to device
func _ready():
	# get access to the editor and viewport
	editor = get_editor_interface()
	var viewport = editor.get_editor_viewport()
	# find all the editor cameras available
	var cameras = find_camera(viewport, [])
	if cameras.size() > 0:
		# sets the camera to the first one
		camera = cameras[camera_index]
	# get access to object selections
	selection = editor.get_selection()
	# connect to the space mouse device
	connected = spacemouse.connect()
		
# main update process to adjust the viewport camera
func _process(delta):
	# make sure we are connected and then poll for the latest motion
	if is_instance_valid(camera) and connected and spacemouse.has_method("poll") \
			and spacemouse.poll() and !editor.is_playing_scene():
		# set the rotation pivot to the origin of the world
		select_origin = Vector3.ZERO
		# check for a selected object and set pivot to object center
		if is_instance_valid(selection):
			var selected = selection.get_selected_nodes()
			if selected.size() > 0 and selected[0] is Spatial:
				select_origin = selected[0].transform.origin
		# obtain the raw translation and rotation values from the device
		space_translation = spacemouse.translation() * flip
		space_rotation = spacemouse.rotation() * flip
		# save the transform so we can adjust the temporary variable in calculations
		camera_transform = camera.transform
		# save the camera origin then center at zero for accurate rotation
		camera_origin = camera_transform.origin
		camera_transform.origin = Vector3.ZERO
		# object control type
		if control_type == ControlType.OBJECT_TYPE:
			# make translation slower the closer you get to the pivot point
			offset_speed = (select_origin.distance_to(camera_origin) / 8.0) + 0.01
			offset_speed = clamp(offset_speed, 0.01, 8.0)
			# transform translation into camera local space
			space_translation = camera_transform.xform(space_translation)
			# adjust the raw readings into reasonable speeds and apply delta
			space_translation *= translate_speed * offset_speed * delta;
			space_rotation *= rotate_speed * delta
			# move the camera by the latest translation but remove the offset from the pivot
			camera_transform.origin += space_translation + camera_origin - select_origin
			# rotate the camera by the latest rotation as normal
			camera_transform = camera_transform.rotated(camera_transform.basis.x.normalized(), space_rotation.x)
			camera_transform = camera_transform.rotated(camera_transform.basis.y.normalized(), space_rotation.y)
			camera_transform = camera_transform.rotated(camera_transform.basis.z.normalized(), space_rotation.z)
			# adjust the camera back to the real location from the pivot point
			camera_transform.origin += select_origin
		# camera control type
		elif control_type == ControlType.CAMERA_TYPE:
			# adjust the raw readings into reasonable speeds and apply delta
			space_translation *= translate_speed * adjust_translation * delta;
			space_rotation *= rotate_speed * adjust_rotation * delta
			# rotate the camera in place
			camera_transform = camera_transform.rotated(camera_transform.basis.x.normalized(), -space_rotation.x)
			camera_transform = camera_transform.rotated(camera_transform.basis.y.normalized(), -space_rotation.y)
			camera_transform = camera_transform.rotated(camera_transform.basis.z.normalized(), -space_rotation.z)
			# transform translation into camera local space
			space_translation = camera_transform.xform(space_translation)
			# move the camera back to the original position
			camera_transform.origin = camera_origin - space_translation
		elif control_type == ControlType.HELICOPTER_TYPE:
			# adjust the raw readings into reasonable speeds and apply delta
			space_translation *= translate_speed * adjust_translation * delta;
			space_rotation *= rotate_speed * adjust_rotation * delta
			# rotate the camera in place
			# rotation is absolute for vertical axis to keep the horizon level
			camera_transform = camera_transform.rotated(Vector3.UP, -space_rotation.y)
			# Permits to tilt (dive up/down)
			camera_transform = camera_transform.rotated(camera_transform.basis.x.normalized(), -space_rotation.x)
			# transform translation into camera local space
			space_translation = camera_transform.xform(space_translation)
			# move the camera back to the original position
			camera_transform.origin = camera_origin - space_translation
		# apply the updated temporary transform to the actual viewport camera
		camera.transform = camera_transform
		
# search a tree and find a list of cameras for all children
func find_camera(node, list) :
	# make sure to only check viewports
	if node is Viewport:
		# get the camera, check it's 3D, and make sure it's not a user created camera
		var camera = node.get_camera()
		if is_instance_valid(camera) and camera is Camera and "@" in camera.name:
			# it's a valid camera, so add it to the list
			list.append(camera)
			# we found a camera, so we can return
			return list
	# check all the children of this node recursively
	for child in node.get_children():
		find_camera(child, list)
	# when done return the final camera list
	return list
