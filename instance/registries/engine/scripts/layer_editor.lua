local cur_block_id = scripting_current_block_id
local pallete_width = 4

local editor = {
    mode = "none",
    pallete_layer = nil,
    layer_being_edited = nil,
    layer_being_edited_index = nil,
    hide_palette = false,
    selected_block = nil,
    keystate = {}
}

local function get_block_by_id(id)
    for k, v in pairs(g_engine_table) do
        if v.id == id then
            return v
        end
    end
end

local function place_pallete(actually_erase_pallete)
    for k, v in pairs(g_engine_table) do
        local x = v.id % pallete_width
        local y = math.floor(v.id / pallete_width)

        if actually_erase_pallete then
            editor.pallete_layer.layer:paste_block(x, y, 0)
        else
            editor.pallete_layer.layer:paste_block(x, y, v.id)
        end

        -- print("placing pallete " .. v.id .. " at " .. x .. ", " .. y)
    end
end

local function mouse_action(x, y, button)
    if button == 1 then
        local blk = block_pos({
            x = x,
            y = y
        })

        if editor.mode == "place" then
            local _, id = editor.layer_being_edited.layer:get_id(blk.x, blk.y)
            -- print("id found " .. id)
            if id == 0 then
                editor.layer_being_edited.layer:paste_block(blk.x, blk.y, editor.selected_block.id)
            end
        end

        if editor.mode == "remove" then
            editor.layer_being_edited.layer:paste_block(blk.x, blk.y, 0)
        end
    end
end

blockengine.register_handler(engine_events.ENGINE_INIT, function()
    editor.pallete_layer = g_menu.pallete

    -- editor.layer_being_edited = get_layer_by_id(0)
    -- editor.layer_being_edited_index = 0

    place_pallete(editor.hide_palette)
end)

local function get_layer_by_id(id)
    for k, v in pairs(g_menu) do
        if v.index == id then
            return v
        end
    end
    return nil
end

ui_width = 20
ui_offset_layer_name = 0
ui_offset_mode = 1
ui_offset_block_name = 3
ui_offset_preview = 5

local function ui_update(do_erase)
    if do_erase then
        world_print(pallete_width, ui_offset_layer_name, ui_width, " ")
        world_print(pallete_width, ui_offset_block_name, ui_width, " ")
        world_print(pallete_width, ui_offset_mode, ui_width, " ")

        editor.pallete_layer.layer:paste_block(pallete_width, ui_offset_preview, 0)
        return
    end

    if editor.layer_being_edited ~= nil then
        world_print(pallete_width, ui_offset_layer_name, ui_width, editor.layer_being_edited.name)
    end

    if editor.selected_block ~= nil and editor.selected_block.id ~= 0 then
        local filename = string.match(editor.selected_block.all_fields.source_filename, "[^/\\]*%.%w+$"):sub(1, -5)
        world_print(pallete_width, ui_offset_block_name, ui_width, "" .. filename)
    end

    editor.pallete_layer.layer:paste_block(pallete_width, ui_offset_preview, editor.selected_block.id) -- show the selected block

    world_print(pallete_width, ui_offset_mode, ui_width, editor.mode)
end

-- mouse wheel chooses the current layer for now
blockengine.register_handler(sdl_events.SDL_MOUSEWHEEL, function(x, y, pos_x, pos_y)
    local desired_layer = math.max(0, math.min(last_layer_element_index, (editor.layer_being_edited_index or 0) + y))
    editor.layer_being_edited_index = desired_layer

    -- print("layer " .. desired_layer .. " out of " .. last_layer_element_index)

    editor.layer_being_edited = get_layer_by_id(desired_layer)

    ui_update()
end)

-- clicks on the pallete choose the tile
blockengine.register_handler(sdl_events.SDL_MOUSEBUTTONDOWN, function(x, y, state, clicks, button)
    local blk = block_pos({
        x = x,
        y = y
    })

    if editor.hide_palette == false then -- attempt to select a block if pallete is visible
        if blk.x > pallete_width then
            goto attempt_select_layer_skip
        end

        local id = blk.x + blk.y * pallete_width

        if id >= g_total_blocks then
            -- print("out of bounds")
            goto attempt_select_layer_skip
        end

        editor.selected_block = get_block_by_id(id)

        ui_update()

        if editor.selected_block == nil or id == 0 then
            goto attempt_select_layer_skip
        end

        return
    end

    ::attempt_select_layer_skip::
    mouse_action(x, y, button)
end)

blockengine.register_handler(sdl_events.SDL_MOUSEMOTION, function(x, y, state, clicks, button)
    mouse_action(x, y, button)
end)

-- special keybinds, maybe
local function keybind_handle(keysym, char)
    if char == 's' then
        le.save_level(test_level)
        print("saved")
    end

    if char == 'r' then
        editor.mode = "remove"
    end

    if char == 'c' then
        editor.mode = "place"
    end

    if char == 'x' then
        editor.mode = "none"
    end

    if keysym == 1073742048 then -- left control
        editor.hide_palette = not editor.hide_palette
    end

    place_pallete(editor.hide_palette)
    ui_update(editor.hide_palette)
end

blockengine.register_handler(sdl_events.SDL_KEYDOWN, function(keysym, mod, state, rep)
    if rep ~= nil and rep == 0 and state == 1 then
        local char = keysym < 256 and string.char(keysym) or nil
        editor.keystate[char or -1] = 1
        keybind_handle(keysym, char)
    end
end)

blockengine.register_handler(sdl_events.SDL_KEYUP, function(keysym, mod, state, rep)
    if rep ~= nil and rep == 0 and state == 0 then
        local char = keysym < 256 and string.char(keysym) or nil
        editor.keystate[char or -1] = 0
    end
end)
