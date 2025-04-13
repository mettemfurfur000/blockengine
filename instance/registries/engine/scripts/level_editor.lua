-- UI for testing
function control_points_clear(points)
    if points ~= nil then
        for i, v in ipairs(points) do
            v.action_reset()
        end
    end
end

local function set_generator(x, y, val)
    return function(mouse)
        -- print("trigger " .. x .. ", " .. y)
        g_menu.ui_text.layer:bprint(g_character_id, x, y, 1, "X")
        mouse.vars:set_number("t", val, 1, 0) -- 1 for 1 byte and 0 for unsigned
    end
end

local function reset_generator(x, y)
    return function(mouse)
        -- print("untrigger " .. x .. ", " .. y)
        g_menu.ui_text.layer:bprint(g_character_id, x, y, 1, " ")
    end
end

local function new_button(pos, name, mouse_state)
    return {
        x = pos.x,
        y = pos.y,
        name = name,
        action_set = set_generator(pos.x, pos.y, mouse_state),
        action_reset = reset_generator(pos.x, pos.y)
    }
end

function new_selector(x, y, name, options, is_multiselector)
    g_menu.ui_text.layer:bprint(g_character_id, x, y, g_width_blocks, name)
    -- g_menu.ui_text.layer:bprint(g_character_id, x, y + 1, g_width_blocks, options_str)

    local control_points = {}

    for i, v in ipairs(options) do
        g_menu.ui_text.layer:bprint(g_character_id, x, y + i, g_width_blocks, "[ ]" .. v)
        table.insert(control_points, new_button({
            x = x,
            y = y + i
        }, v, i))
    end

    return control_points
end

function layer_names_get()
    local layer_names = {}
    local layer_indexes = {}

    for k, v in pairs(g_menu) do
        if v.is_ui ~= true then
            table.insert(layer_names, v.name)
            table.insert(layer_indexes, v.index)
        end
    end

    return layer_indexes, layer_names
end

function place_ui_init()
    local width, height = render_rules.get_size(g_render_rules)

    -- setting up availiable mouse modes and layers to select the edition
    -- print("placing ui")
    -- local layer_names, layer_indexies = layer_names_get()
    -- local edit_modes = {"None", "Place", "Remove", "Copy"}

    -- new_selector(0, 0, "Modes:", edit_modes)
    -- new_selector(0, 0, "Layers:", layer_names)

    -- g_menu.ui_text.layer:bprint(g_character_id, 0, 0, 16, "Mode:\n[ ]None\n[ ]Place\n[ ]Remove\n[ ]Copy\nLayer:\n")
    -- g_menu.ui_text.layer:bprint(g_character_id, 0, 6, 16, options)

    -- g_menu.ui_text.layer:bprint(g_character_id, width / g_block_size - 14, 0, 16, "Blocks:")

    -- table.insert(control_points, new_button({
    --     x = 1,
    --     y = 1
    -- }, "None", 0))
    -- table.insert(control_points, new_button({
    --     x = 1,
    --     y = 2
    -- }, "Place", 1))
    -- table.insert(control_points, new_button({
    --     x = 1,
    --     y = 3
    -- }, "Remove", 2))
    -- table.insert(control_points, new_button({
    --     x = 1,
    --     y = 4
    -- }, "Copy", 3))

    -- print_table(control_points)
end
