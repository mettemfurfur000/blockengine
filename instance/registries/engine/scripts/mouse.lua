-- local constants = require("registries.engine.scripts.constants")
-- local cam_utils = require("registries.engine.scripts.camera_utils")
local current_block = scripting_current_block_id

mouse = {
    pos = {
        x = -1,
        y = -1
    },
    home_layer = nil,
    home_layer_index = nil,
    home_room = nil,
    vars = nil
}

function block_pos(mouse_pos, zoom)
    zoom = zoom or render_rules.get_slice(g_render_rules, g_menu.mouse.index).zoom
    return {
        x = math.floor(mouse_pos.x / g_block_size / zoom),
        y = math.floor(mouse_pos.y / g_block_size / zoom)
    }
end

blockengine.register_handler(engine_events.ENGINE_INIT, function()
    local width, height = render_rules.get_size(g_render_rules)

    local pos = { -- puts it just wherevr
        x = width / 32,
        y = height / 32
    }

    mouse.home_layer = g_menu.mouse.layer
    mouse.home_layer_index = g_menu.mouse.index
    mouse.pos = pos

    mouse.home_layer:for_each(current_block, function(x, y)
        print("found an existing mouse, OBLITERATE")
        mouse.home_layer:paste_block(x, y, 0)
    end)

    if mouse.home_layer:paste_block(mouse.pos.x, mouse.pos.y, current_block) == false then
        log_error("failed to paste a mouse block")
    end

    local status, vars = mouse.home_layer:get_vars(mouse.pos.x, mouse.pos.y)
    if status == false then
        log_error("error getting vars for the mouse")
        return
    end

    mouse.vars = vars

    print("mouse initialized")
end)

local function mouse_move(layer, slice, cur_pos_pixels, old_pos_blocks)
    local new_pos = block_pos(cur_pos_pixels, slice.zoom)

    local delta = {
        x = new_pos.x - old_pos_blocks.x,
        y = new_pos.y - old_pos_blocks.y
    }

    local status = layer:move_block(old_pos_blocks.x, old_pos_blocks.y, delta.x, delta.y)

    if status == true then
        return new_pos
    end
    return old_pos_blocks
end

blockengine.register_handler(sdl_events.SDL_MOUSEMOTION, function(x, y, state, clicks)
    if mouse.home_layer == nil then
        return
    end

    local cur_slice = render_rules.get_slice(g_render_rules, g_menu.mouse.index);
    mouse.pos = mouse_move(mouse.home_layer, cur_slice, {
        x = x,
        y = y
    }, mouse.pos)

    if state == 1 then
        -- print("clicked at ", mouse.pos.x, mouse.pos.y)
        if control_points == nil then
            return
        end
        -- print("valid")
        print_table(control_points)

        local block_pos = block_pos({
            x = x,
            y = y
        }, cur_slice.zoom)

        for i, v in ipairs(control_points) do
            -- print("checking " .. v.x .. ", " .. v.y .. " to " .. block_pos.x .. ", " .. block_pos.y)
            if block_pos.x == v.x and block_pos.y == v.y then
                control_points_clear()
                v.action_set(mouse)
                -- print("triggering")
            end
        end
    end
end)

blockengine.register_handler(sdl_events.SDL_MOUSEWHEEL, function(x, y, pos_x, pos_y)
    if mouse.home_layer == nil then
        return
    end

    mouse.pos = mouse_move(mouse.home_layer, render_rules.get_slice(g_render_rules, mouse.home_layer_index), {
        x = pos_x,
        y = pos_y
    }, mouse.pos)
end)

-- print("registering a mouse input")

blockengine.register_handler(sdl_events.SDL_MOUSEBUTTONDOWN, function(x, y, state, clicks, button)
    if mouse.home_layer == nil then
        return
    end

    local cur_slice = render_rules.get_slice(g_render_rules, g_menu.mouse.index);
    local actual_pos = block_pos({
        x = x,
        y = y
    }, cur_slice.zoom)

    print("click at " .. actual_pos.x .. ", " .. actual_pos.y)

    local func = g_menu.objects.layer:get_input_handler(actual_pos.x, actual_pos.y, "click")

    if func then
        func(g_menu.objects.layer, actual_pos.x, actual_pos.y, 1)
    end
end)

