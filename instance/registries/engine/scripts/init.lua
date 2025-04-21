require("registries.engine.scripts.wrappers")
require("registries.engine.scripts.constants")
-- require("registries.engine.scripts.level_editor")

screen_width, screen_height = render_rules.get_size(g_render_rules)

g_width_blocks, g_height_blocks = screen_width / g_block_size, screen_height / g_block_size

local function slice_gen(x, y, w, h, z, lay_ref)
    return {
        x = x,
        y = y,
        h = h,
        w = w,
        zoom = z,
        ref = lay_ref
    }
end

local function slice_basic(lay_ref)
    return slice_gen(0, 0, screen_width, screen_height, 1, lay_ref)
end

last_layer_element_index = 0

local function layer_new_renderable(table_dest, __name, lay_ref, __is_ui)
    table_dest[__name] = {
        index = last_layer_element_index,
        name = __name,
        layer = lay_ref,
        is_ui = __is_ui or false,
        slice = slice_basic(lay_ref)
    }
    last_layer_element_index = last_layer_element_index + 1
end

local function layer_new_invisible(table_dest, __name, lay_ref)
    table_dest[__name] = {
        index = last_layer_element_index,
        name = __name,
        layer = lay_ref,
        is_ui = false,
        slice = nil,
        slice_push_ignore = true
    }
    last_layer_element_index = last_layer_element_index + 1
end

local function set_render_rule_order(ref_table)
    local order = {}
    print("setting render rule order")

    for k, v in pairs(ref_table) do
        if v.slice_push_ignore ~= true then
            print("putting " .. v.name .. " at " .. v.index)
            table.insert(order, v.index)
        end
    end

    table.sort(order)

    print("order:")
    print_table(order)

    render_rules.set_order(g_render_rules, order)
end

local function set_slices(ref_table)
    print("setting slices")
    for k, v in pairs(ref_table) do
        if v.slice_push_ignore ~= true then
            print(v.name)
            render_rules.set_slice(g_render_rules, v.index, v.slice)
        end
    end
end

local test_level = le.create_level("test")
if test_level == nil then
    log_error("error creating level")
    os.exit()
end

g_engine = safe_registry_load(test_level, "engine")

g_engine_table = g_engine:to_table()

g_total_blocks = tablelength(g_engine_table)
-- print_table(g_engine_table)

g_character_id = find_block(g_engine_table, "character").id

menu_room = safe_menu_create(test_level, "menu", g_width_blocks, g_height_blocks)

g_menu = {}

layer_new_invisible(g_menu, "dev", safe_layer_create(menu_room, "engine", 1, 0))
layer_new_renderable(g_menu, "ui_text", safe_layer_create(menu_room, "engine", 1, 2), true)
layer_new_renderable(g_menu, "pallete", safe_layer_create(menu_room, "engine", 1, 0))
layer_new_renderable(g_menu, "floor", safe_layer_create(menu_room, "engine", 1, 0))
layer_new_renderable(g_menu, "objects", safe_layer_create(menu_room, "engine", 1, 2))
layer_new_renderable(g_menu, "mouse", safe_layer_create(menu_room, "engine", 1, 2))
-- layer_new_renderable(g_menu, "ui_back", safe_layer_create(menu_room, "engine", 1, 2), true)
-- layer_new_renderable(g_menu, "ui_floor_select", safe_layer_create(menu_room, "engine", 1, 0), true)
-- layer_new_renderable(g_menu, "ui_engine_select", safe_layer_create(menu_room, "engine", 1, 0), true)

try(function()
    set_render_rule_order(g_menu)
    set_slices(g_menu)
end, function(e)
    log_error(e)
    os.exit()
end)
