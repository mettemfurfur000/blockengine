log_error = function(msg)
    log_msg(2, msg)
end

log_message = function(msg)
    log_msg(1, msg)
end

function try(f, catch_f)
    local status, exception = pcall(f)
    if not status then
        catch_f(exception)
    end
end

local world_size = 32

local test_level = le.create_level("test")
if test_level == nil then
    log_error("error creating level")
    os.exit()
end

if test_level:load_registry("engine") == false then
    log_error("error loading engine registry")
    os.exit()
end

if test_level:load_registry("floors") == false then
    log_error("error loading floor registry")
    os.exit()
end

local menu_room = test_level:new_room("menu", world_size, world_size)
if menu_room == nil then
    log_error("error creating room")
    os.exit()
end

local floor_layer = menu_room:new_layer("floors", 1, 0, 0)
if floor_layer == nil then
    log_error("error creating a floor")
    os.exit()
end

local object_layer = menu_room:new_layer("engine", 2, 2, 1)
if object_layer == nil then
    log_error("error creating layer")
    os.exit()
end

object_layer:paste_block(4, 4, 2) -- x, y, id

local w, h = 8, 8
for i = 1, w do
    for j = 1, h do
        floor_layer:paste_block(2 + i, 2 + j, 2) -- x, y, id
    end
end

local width, height = render_rules.get_size(g_render_rules)

object_layer:bprint(1, 1, 1, 16, "test string, 16 chars\n" .. "newline")

local slice_floor = {
    x = 0,
    y = 0,
    w = width,
    h = height,
    zoom = 2,
    ref = floor_layer
}

local slice_objects = {
    x = 0,
    y = 0,
    w = width,
    h = height,
    zoom = 2,
    ref = object_layer
}

g_object_layer = object_layer
g_object_layer_index = 1

try(function()
    render_rules.set_order(g_render_rules, {0, 1})
    render_rules.set_slice(g_render_rules, 0, slice_floor)
    render_rules.set_slice(g_render_rules, 1, slice_objects)
end, function(e)
    log_error(e)
    os.exit()
end)
