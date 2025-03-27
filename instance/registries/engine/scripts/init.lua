local wrappers = require("registries.engine.scripts.wrappers")

local world_size = 32

local test_level = le.create_level("test")
if test_level == nil then
    log_error("error creating level")
    os.exit()
end

g_engine = safe_registry_load(test_level, "engine")
g_floors = safe_registry_load(test_level, "floors")
g_walls = safe_registry_load(test_level, "walls")

print_table(g_engine:to_table())

menu_room = safe_menu_create(test_level, "menu", world_size, world_size)

g_floor_layer = safe_layer_create(menu_room, "floors", 1, 0)
g_floor_layer_index = 0
g_object_layer = safe_layer_create(menu_room, "engine", 2, 2)
g_object_layer_index = 1
g_ui_layer = safe_layer_create(menu_room, "engine", 2, 2)
g_ui_layer_index = 2

g_object_layer:paste_block(4, 4, 2) -- x, y, id

local w, h = 8, 8
for i = 1, w do
    for j = 1, h do
        g_floor_layer:paste_block(2 + i, 2 + j, 2) -- x, y, id
    end
end

local width, height = render_rules.get_size(g_render_rules)

g_object_layer:bprint(1, 1, 1, width / 16, "test string, 16 chars\n" .. "newline")

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
    zoom = 2,
    ref = g_ui_layer
}

try(function()
    render_rules.set_order(g_render_rules, {0, 1, 2})
    render_rules.set_slice(g_render_rules, 0, slice_floor)
    render_rules.set_slice(g_render_rules, 1, slice_objects)
    render_rules.set_slice(g_render_rules, 2, slice_ui)
end, function(e)
    log_error(e)
    os.exit()
end)
