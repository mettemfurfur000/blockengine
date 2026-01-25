local block_utils = require("registries.engine.scripts.block_utils")

local current_block = scripting_current_block_id

G_mouse = {
    pos = {
        x = -1,
        y = -1
    },
    offset = {
        x = 0,
        y = 0
    },
    home_layer = nil,
    home_layer_index = nil,
    home_room = nil,
    vars = nil
}

blockengine.register_handler(events.ENGINE_INIT, function()
    local width, height = render_rules.get_size(g_render_rules)

    local pos = { -- puts it just wherevr
        x = 0,
        y = 0
    }

    ---@class Layer
    G_mouse.home_layer = G_view_menu.mouse.layer
    G_mouse.home_layer_index = G_view_menu.mouse.index
    G_mouse.pos = pos

    G_mouse.home_layer:for_each(current_block, function(x, y)
        print("found an existing mouse, OBLITERATE")
        G_mouse.home_layer:paste_block(x, y, 0)
    end)

    if G_mouse.home_layer:paste_block(G_mouse.pos.x, G_mouse.pos.y, current_block) == false then
        error("failed to paste a mouse block")
    end

    local status, vars = G_mouse.home_layer:get_vars(G_mouse.pos.x, G_mouse.pos.y)
    if status == false then
        error("error getting vars for the mouse")
        return
    end

    G_mouse.vars = vars

    print("mouse initialized")
end)

local function mouse_move(layer, slice, cur_pos_pixels, old_pos_blocks)
    local new_pos = block_utils.pixel_to_blocks_no_offset(cur_pos_pixels, slice.zoom)

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

blockengine.register_handler(events.SDL_MOUSEMOTION, function(x, y, state, clicks)
    if G_mouse.home_layer == nil then
        return
    end

    local cur_slice = render_rules.get_slice(g_render_rules, G_view_menu.mouse.index);
    G_mouse.pos = mouse_move(G_mouse.home_layer, cur_slice, {
        x = x,
        y = y
    }, G_mouse.pos)
end)

blockengine.register_handler(events.SDL_MOUSEWHEEL, function(x, y, pos_x, pos_y)
    if G_mouse.home_layer == nil then
        return
    end

    G_mouse.pos = mouse_move(G_mouse.home_layer, render_rules.get_slice(g_render_rules, G_mouse.home_layer_index), {
        x = pos_x,
        y = pos_y
    }, G_mouse.pos)
end)

-- print("registering a mouse input")

blockengine.register_handler(events.SDL_MOUSEBUTTONDOWN, function(x, y, state, clicks, button)
    if G_mouse.home_layer == nil then
        return
    end

    local cur_slice = render_rules.get_slice(g_render_rules, G_view_menu.mouse.index);
    local actual_pos = block_utils.pixels_to_blocks({
        x = x,
        y = y
    }, cur_slice.zoom)

    -- print("click at " .. actual_pos.x .. ", " .. actual_pos.y)

    local func = G_view_menu.objects.layer:get_input_handler(actual_pos.x, actual_pos.y, "click")

    if func then
        func(G_view_menu.objects.layer, actual_pos.x, actual_pos.y, 1)
    end
end)
