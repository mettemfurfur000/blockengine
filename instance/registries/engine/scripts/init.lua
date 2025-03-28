require("registries.engine.scripts.wrappers")
require("registries.engine.scripts.level_editor")

local world_size = 32

local test_level = le.create_level("test")
if test_level == nil then
    log_error("error creating level")
    os.exit()
end

g_engine = safe_registry_load(test_level, "engine")
g_floors = safe_registry_load(test_level, "floors")

menu_room = safe_menu_create(test_level, "menu", world_size, world_size)

g_floor_layer = safe_layer_create(menu_room, "floors", 1, 0)
g_floor_layer_index = 0
g_object_layer = safe_layer_create(menu_room, "engine", 1, 2)
g_object_layer_index = 1
g_ui_background_layer = safe_layer_create(menu_room, "engine", 1, 0)
g_ui_background_layer_index = 2
g_ui_layer = safe_layer_create(menu_room, "engine", 1, 2)
g_ui_layer_index = 3
g_mouse_layer = safe_layer_create(menu_room, "engine", 1, 1)
g_mouse_layer_index = 4

-- pasting a player
g_object_layer:paste_block(4, 4, 2) -- x, y, id

local w, h = 8, 8
for i = 1, w do
    for j = 1, h do
        g_floor_layer:paste_block(2 + i, 2 + j, 2) -- x, y, id
    end
end

local width, height = render_rules.get_size(g_render_rules)

local slice_floor = {
    x = 0,
    y = 0,
    w = width,
    h = height,
    zoom = 2,
    ref = g_floor_layer
}

local slice_objects = {
    x = 0,
    y = 0,
    w = width,
    h = height,
    zoom = 2,
    ref = g_object_layer
}

local slice_ui = {
    x = 0,
    y = 0,
    w = width,
    h = height,
    zoom = 1,
    ref = g_ui_layer
}

local slice_ui_background = {
    x = 0,
    y = 0,
    w = width,
    h = height,
    zoom = 1,
    ref = g_ui_background_layer
}

local slice_mouse = {
    x = 0,
    y = 0,
    w = width,
    h = height,
    zoom = 2,
    ref = g_mouse_layer
}

g_slices_affected_by_zoom = {0, 1, 3}

place_ui_init(g_slices_affected_by_zoom)

try(function()
    render_rules.set_order(g_render_rules, {0, 1, 2, 3, 4})
    render_rules.set_slice(g_render_rules, 0, slice_floor)
    render_rules.set_slice(g_render_rules, 1, slice_objects)
    render_rules.set_slice(g_render_rules, 2, slice_ui)
    render_rules.set_slice(g_render_rules, 3, slice_ui_background)
    render_rules.set_slice(g_render_rules, 4, slice_mouse)
end, function(e)
    log_error(e)
    os.exit()
end)
