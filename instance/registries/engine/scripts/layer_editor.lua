local current_block = scripting_current_block_id

pallete_width = 8
ui_width = 24
ui_offset_layer_name = 0
ui_offset_mode = 1
ui_offset_block_name = 3
ui_offset_preview = 4
ui_offset_vars_debug = 5

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
    local x, y = 0, 0
    local ranged_begin = 0

    for k, v in pairs(g_engine_table) do

        if ranged_begin ~= nil and ranged_begin ~= 0 then
            x = x + 1
            ranged_begin = ranged_begin - 1
            -- if x >= pallete_width then
            if x >= v.atlas_info.frames then
                x = 0
                y = y + 1
            end
        elseif v.all_fields ~= nil and v.all_fields.repeat_times ~= nil and ranged_begin == 0 then -- place them all on the same line if its a repeated block
            print("#### found ranged thing with id " .. v.id)
            print_table(v)
            ranged_begin = v.all_fields.repeat_times - 1
        else
            x = 0
            y = y + 1
        end

        editor.pallete_layer.layer:paste_block(x, y, actually_erase_pallete and 0 or v.id)

        -- print("placing pallete " .. v.id .. " at " .. x .. ", " .. y)
    end
end

local function mouse_action(x, y, button)
    if button == 1 then
        local blk = pixels_to_blocks({
            x = x,
            y = y
        })

        if editor.mode == "place" then
            local id = editor.layer_being_edited.layer:get_id(blk.x, blk.y)
            -- print("id found " .. id)
            if id == 0 then
                editor.layer_being_edited.layer:paste_block(blk.x, blk.y, editor.selected_block.id)
            end
        end

        if editor.mode == "remove" then
            editor.layer_being_edited.layer:paste_block(blk.x, blk.y, 0)
        end

        if editor.mode == "inspect" then
            local status, vars = editor.layer_being_edited.layer:get_vars(blk.x, blk.y)
            if status == true and vars:is_valid() then
                log_message("true vars found at " .. x .. ", " .. y)
                local vars = vars:__tostring()

                world_print(pallete_width, ui_offset_vars_debug, ui_width, vars)
            end
        end
    end
end

local function get_layer_by_id(id)
    for k, v in pairs(g_menu) do
        if v.index == id then
            return v
        end
    end
    return nil
end

local function select_layer(number)
    local desired_layer = math.max(0, math.min(total_layers, (editor.layer_being_edited_index or 0) + number))
    editor.layer_being_edited_index = desired_layer
    editor.layer_being_edited = get_layer_by_id(desired_layer)
end

blockengine.register_handler(engine_events.ENGINE_INIT, function()
    editor.pallete_layer = g_menu.pallete
    place_pallete(editor.hide_palette)

    select_layer(-1)
end)

local function ui_update(do_erase)
    if do_erase then
        world_print(pallete_width, ui_offset_layer_name, ui_width, " ")
        world_print(pallete_width, ui_offset_block_name, ui_width, " ")
        world_print(pallete_width, ui_offset_mode, ui_width, " ")

        -- editor.pallete_layer.layer:paste_block(pallete_width, ui_offset_preview, 0)
        g_menu.text.layer:paste_block(pallete_width, ui_offset_preview, 0)
        return
    end

    if editor.layer_being_edited ~= nil then
        world_print(pallete_width, ui_offset_layer_name, ui_width,
            editor.layer_being_edited.name .. ":" .. editor.layer_being_edited_index)
    end

    if editor.selected_block ~= nil and editor.selected_block.id ~= 0 then
        local filename = string.match(editor.selected_block.all_fields.source_filename, "[^/\\]*%.%w+$"):sub(1, -5)
        world_print(pallete_width, ui_offset_block_name, ui_width, "" .. filename)

        -- show the selected block
        -- editor.pallete_layer.layer:paste_block(pallete_width, ui_offset_preview, editor.selected_block.id) 
        g_menu.text.layer:paste_block(pallete_width, ui_offset_preview, editor.selected_block.id)
    end

    world_print(pallete_width, ui_offset_mode, ui_width, editor.mode)
end

blockengine.register_handler(sdl_events.SDL_MOUSEWHEEL, function(x, y, pos_x, pos_y)
    local slice = render_rules.get_slice(g_render_rules, editor.pallete_layer.index)
    slice.y = math.max(0, math.min(screen_height, (slice.y or 0) - y * 32))
    render_rules.set_slice(g_render_rules, editor.pallete_layer.index, slice)

    -- local desired_layer = math.max(0, math.min(total_layers, (editor.layer_being_edited_index or 0) + y))
    -- editor.layer_being_edited_index = desired_layer
    -- editor.layer_being_edited = get_layer_by_id(desired_layer)

    -- print("layer " .. desired_layer .. " out of " .. total_layers)

    -- ui_update()
end)

-- clicks on the pallete choose the tile
blockengine.register_handler(sdl_events.SDL_MOUSEBUTTONDOWN, function(x, y, state, clicks, button)
    -- local blk = pixel_to_blocks_no_offset({
    local blk = pixel_to_layer_blocks(editor.pallete_layer, {
        x = x,
        y = y
    })

    if editor.hide_palette == false then -- attempt to select a block if pallete is visible
        if blk.x > pallete_width then
            goto attempt_select_layer_skip
        end

        -- local id = blk.x + blk.y * pallete_width
        local id = editor.pallete_layer.layer:get_id(blk.x, blk.y)

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
    -- if char == 's' then
    --     le.save_level(test_level)
    --     print("saved")
    -- end

    -- if char == 'h' then
    --     -- for testing purposes make the object layer static
    --     if editor.layer_being_edited then
    --         render_rules.set_frozen(g_render_rules, 0, true)
    --     end
    -- end

    -- if char == 'l' then
    --     -- make it normal
    --     if editor.layer_being_edited then
    --         render_rules.set_frozen(g_render_rules, 0, false)
    --     end
    -- end

    if char == 'q' then
        select_layer(1)
    end

    if char == 'z' then
        select_layer(-1)
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

    if char == 'i' then
        editor.mode = "inspect"
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
