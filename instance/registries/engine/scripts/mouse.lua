local constants = require("registries.engine.scripts.constants")

local this_block_id = scripting_current_block_id

local mouse = {
    pos = {
        x = -1,
        y = -1
    },
    home_layer = nil,
    home_room = nil,
    vars = nil
}

blockengine.register_handler(EVENT_IDS.ENGINE_INIT, function()
    local width, height = render_rules.get_size(g_render_rules)

    local pos = {
        x = width / 32,
        y = height / 32
    }

    mouse.home_layer = g_ui_layer
    mouse.pos = pos

    mouse.home_layer:paste_block(mouse.pos.x, mouse.pos.y, this_block_id) -- x, y, id

    local status, vars = mouse.home_layer:get_vars(mouse.pos.x, mouse.pos.y)
    if status == false then
        log_error("error getting vars for the mouse")
        return
    end

    mouse.vars = vars

    print("mouse initialized")
end)

blockengine.register_handler(EVENT_IDS.SDL_MOUSEMOTION, function(x, y, state, clicks)
    if mouse.home_layer == nil then
        return
    end

    local slice = render_rules.get_slice(g_render_rules, g_ui_layer_index)

    local new_pos = {
        x = math.floor(x / g_block_size / slice.zoom),
        y = math.floor(y / g_block_size / slice.zoom)
    }

    local delta = {
        x = new_pos.x - mouse.pos.x,
        y = new_pos.y - mouse.pos.y
    }

    local status = mouse.home_layer:move_block(mouse.pos.x, mouse.pos.y, delta.x, delta.y)

    if status == true then
        mouse.pos = new_pos
    end
end)

-- TODO: implement clicking and operation changing
-- TODO: implement scrolling