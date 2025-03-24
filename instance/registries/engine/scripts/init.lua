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
local flags = 1

local test_level = le.create_level("test")
if test_level == nil then
    log_error("error creating level")
    os.exit()
end

if test_level:load_registry("engine") == false then
    log_error("error loading registry")
    os.exit()
end

local menu_room = test_level:new_room("menu", world_size, world_size)
if menu_room == nil then
    log_error("error creating room")
    os.exit()
end

local floor_l = menu_room:new_layer("engine", 2, 2, flags)
if floor_l == nil then
    log_error("error creating layer")
    os.exit()
end

floor_l:paste_block(4, 4, 1) -- x, y, id
floor_l:paste_block(4, 6, 2) -- x, y, id
-- floor_l:bprint(1, 1, 1, 16, "test string, 16 chars\n" .. "magic word: " .. skibidi)

log_message("blocks pasted")

local width, height = render_rules.get_size(g_render_rules)

local render_order = {0}
local slice = {
    x = 0,
    y = 0,
    w = width,
    h = height,
    zoom = 4,
    ref = floor_l
}

try(function()
    render_rules.set_order(g_render_rules, render_order)
    render_rules.set_slice(g_render_rules, 0, slice)
end, function(e)
    log_error(e)
    os.exit()
end)
