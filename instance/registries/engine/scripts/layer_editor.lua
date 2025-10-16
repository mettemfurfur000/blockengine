local block_utils = require("registries.engine.scripts.block_utils")
local wrappers = require("registries.engine.scripts.wrappers")

local current_block = scripting_current_block_id

G_pallete_width = 8
G_ui_width = 32
G_ui_offset_layer_name = 0
G_ui_offset_mode = 1
G_ui_offset_block_name = 3
G_ui_offset_preview = 4
G_ui_offset_position_debug = 5
G_ui_offset_vars_debug = 6

local editor = {
    mode = "none",
    pallete_layer = nil,
    pallete_index = nil,
    layer_being_edited = nil,
    layer_being_edited_index = nil,
    is_shown = false,
    selected_block = nil,
    keystate = {}
}

local function get_block_by_id(id)
    for k, v in pairs(G_engine_table) do
        if v.id == id then
            return v
        end
    end
end

local function place_pallete(is_shown)
    local x, y = 0, 0
    local ranged_begin = 0

    for k, v in pairs(G_engine_table) do
        if ranged_begin ~= nil and ranged_begin ~= 0 then
            x = x + 1
            ranged_begin = ranged_begin - 1

            if x >= v.atlas_info.frames then
                x = 0
                y = y + 1
            end -- place them all on the same line if its a repeated block
        elseif v.all_fields ~= nil and v.all_fields.repeat_times ~= nil and ranged_begin == 0 then
            ranged_begin = v.all_fields.repeat_times - 1
        else
            x = 0
            y = y + 1
        end

        editor.pallete_layer:paste_block(x, y, (not is_shown) and 0 or v.id)
    end
end

-- does actions and allows to inspect blocks
local function mouse_action(x, y, button)
    if not editor.is_shown then
        return
    end

    local block_highlight = block_utils.pixels_to_blocks({
        x = x,
        y = y
    })

    wrappers.world_print(G_pallete_width, G_ui_offset_position_debug, G_ui_width,
        (block_highlight.x .. ", " .. block_highlight.y))

    local success, vars = editor.layer_being_edited.layer:get_vars(block_highlight.x, block_highlight.y)
    if success and vars:is_valid() then
        wrappers.world_print(G_pallete_width, G_ui_offset_vars_debug, G_ui_width, vars:__tostring())
    end

    if button == 1 then
        local block_coords = block_utils.pixels_to_blocks({
            x = x,
            y = y
        })

        if editor.mode == "place" then
            local id = editor.layer_being_edited.layer:get_id(block_coords.x, block_coords.y)
            -- print("id found " .. id)
            if id == 0 then
                editor.layer_being_edited.layer:paste_block(block_coords.x, block_coords.y, editor.selected_block.id)
            end
        end

        if editor.mode == "remove" then
            editor.layer_being_edited.layer:paste_block(block_coords.x, block_coords.y, 0)
        end
    end
end

local function get_layer_by_id(id)
    for k, v in pairs(G_menu) do
        if v.index == id then
            return v
        end
    end
    return nil
end

local function select_layer(number)
    local desired_layer = math.max(0, math.min(G_layers_amount, (editor.layer_being_edited_index or 0) + number))
    editor.layer_being_edited_index = desired_layer
    editor.layer_being_edited = get_layer_by_id(desired_layer)
end

-- gets all the data
blockengine.register_handler(engine_events.ENGINE_INIT, function()
    ---@class Layer
    editor.pallete_layer = G_menu.pallete.layer
    ---@type number
    editor.pallete_index = G_menu.pallete.index

    place_pallete(editor.is_shown)

    select_layer(-1)
end)

-- resets the whole editor thing
local function ui_update()
    if not editor.is_shown then
        wrappers.world_fill(0, 0, G_ui_width, 64)
        return
    end

    if editor.layer_being_edited ~= nil then
        wrappers.world_print(G_pallete_width, G_ui_offset_layer_name, G_ui_width,
            editor.layer_being_edited.name .. ":" .. editor.layer_being_edited_index)
    end

    if editor.selected_block ~= nil and editor.selected_block.id ~= 0 then
        local filename = string.match(editor.selected_block.all_fields.source_filename, "[^/\\]*%.%w+$"):sub(1, -5)
        wrappers.world_print(G_pallete_width, G_ui_offset_block_name, G_ui_width, "" .. filename)

        G_menu.text.layer:paste_block(G_pallete_width, G_ui_offset_preview, editor.selected_block.id)
    end

    wrappers.world_print(G_pallete_width, G_ui_offset_mode, G_ui_width, editor.mode)
end

-- Scrolls the pallete
blockengine.register_handler(sdl_events.SDL_MOUSEWHEEL, function(x, y, pos_x, pos_y)
    local slice = render_rules.get_slice(g_render_rules, editor.pallete_index)
    slice.y = math.max(0, math.min(G_screen_height, (slice.y or 0) - y * 32))
    render_rules.set_slice(g_render_rules, editor.pallete_index, slice)
end)

-- clicks on the pallete choose the tile
blockengine.register_handler(sdl_events.SDL_MOUSEBUTTONDOWN, function(x, y, state, clicks, button)
    local blk = block_utils.pixel_to_layer_blocks(editor.pallete_index, {
        x = x,
        y = y
    })

    if editor.is_shown then -- attempt to select a block if pallete is visible
        if blk.x > G_pallete_width then
            goto attempt_select_layer_skip
        end

        -- local id = blk.x + blk.y * pallete_width
        local id = editor.pallete_layer:get_id(blk.x, blk.y)

        if id >= G_total_blocks then
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
        editor.is_shown = not editor.is_shown
    end

    place_pallete(editor.is_shown)
    ui_update()
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
