-- UI for testing
control_points = {}

function control_points_clear()
    if control_points == nil then
        return
    end

    for i, v in ipairs(control_points) do
        v.action_reset()
    end
end

local function set_generator(x, y, val)
    return function(mouse)
        print("trigger " .. x .. ", " .. y)
        g_menu.ui_text.layer:bprint(1, x, y, 1, "X")
        mouse.vars:set_number("t", val, 1, 0) -- 1 for 1 byte and 0 for unsigned
    end
end

local function reset_generator(x, y)
    return function(mouse)
        print("untrigger " .. x .. ", " .. y)
        g_menu.ui_text.layer:bprint(1, x, y, 1, " ")
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

function place_ui_init()
    local width, height = render_rules.get_size(g_render_rules)

    -- setting up availiable mouse modes and layers to select the edition
    local options = ""
    for k, v in pairs(g_menu) do
        if v.is_ui == true then
            goto continue
        end
        options = options .. "[ ] " .. v.index .. " - " .. v.name .. "\n"
        ::continue::
    end

    print("placing ui")
    g_menu.ui_text.layer:bprint(1, 0, 0, 16, "Mode:\n[ ]None\n[ ]Place\n[ ]Remove\n[ ]Copy\nLayer:\n")
    g_menu.ui_text.layer:bprint(1, 0, 6, 16, options)

    table.insert(control_points, new_button({
        x = 1,
        y = 1
    }, "None", 0))
    table.insert(control_points, new_button({
        x = 1,
        y = 2
    }, "Place", 1))
    table.insert(control_points, new_button({
        x = 1,
        y = 3
    }, "Remove", 2))
    table.insert(control_points, new_button({
        x = 1,
        y = 4
    }, "Copy", 3))

    -- print_table(control_points)
end
