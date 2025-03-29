require("registries.engine.scripts.wrappers")
require("registries.engine.scripts.constants")
require("registries.engine.scripts.level_editor")

local width, height = render_rules.get_size(g_render_rules)

g_width_blocks, g_height_blocks = width / g_block_size, height / g_block_size

g_camera_limit_x, g_camera_limit_y = g_width_blocks * g_block_size - width, g_height_blocks * g_block_size - height

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
    return slice_gen(0, 0, width, height, 1, lay_ref)
end

local function layer_new_renderable(table_dest, lay_index, name, lay_ref, is_ui)
    table_dest[name] = {
        index = lay_index,
        name = name,
        layer = lay_ref,
        show = true,
        is_ui = is_ui,
        slice = slice_basic(lay_ref)
    }
end

local function set_render_rule_order(ref_table)
    local order = {}

    for k, v in pairs(ref_table) do
        table.insert(order, v.index)
    end

    print("setting render rule order")
    print_table(order)

    render_rules.set_order(g_render_rules, order)
end

local function set_slices(ref_table)
    for k, v in pairs(ref_table) do
        render_rules.set_slice(g_render_rules, v.index, v.slice)
    end
end

local test_level = le.create_level("test")
if test_level == nil then
    log_error("error creating level")
    os.exit()
end

g_engine = safe_registry_load(test_level, "engine")
g_floors = safe_registry_load(test_level, "floors")

g_engine_table = g_engine:to_table()

g_character_id = find_block(g_engine_table, "character").id

menu_room = safe_menu_create(test_level, "menu", g_width_blocks, g_height_blocks)

g_menu = {}

layer_new_renderable(g_menu, 0, "floor", safe_layer_create(menu_room, "floors", 1, 0))
layer_new_renderable(g_menu, 1, "objects", safe_layer_create(menu_room, "engine", 1, 2))
layer_new_renderable(g_menu, 2, "ui_back", safe_layer_create(menu_room, "engine", 1, 2), true)
layer_new_renderable(g_menu, 3, "ui_text", safe_layer_create(menu_room, "engine", 1, 2), true)
layer_new_renderable(g_menu, 4, "mouse", safe_layer_create(menu_room, "engine", 1, 2), true)

place_ui_init()

try(function()
    set_render_rule_order(g_menu)
    set_slices(g_menu)
end, function(e)
    log_error(e)
    os.exit()
end)
