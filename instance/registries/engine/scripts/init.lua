require("registries.engine.scripts.wrappers")
require("registries.engine.scripts.constants")
-- require("registries.engine.scripts.level_editor")

screen_width, screen_height = render_rules.get_size(g_render_rules)

g_width_blocks, g_height_blocks = screen_width / g_block_size, screen_height / g_block_size

local function slice_gen(x, y, w, h, z, lay_ref)
    if lay_ref == nil then
        print("no ref layer to generate a slice")
        os.exit(0)
    end
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
    return slice_gen(0, 0, screen_width, screen_height, global_zoom, lay_ref)
end

last_layer_element_index = 0

local function layer_append_render(table_dest, __name, lay_ref, __is_ui)
    table_dest[__name] = {
        index = last_layer_element_index,
        name = __name,
        layer = lay_ref,
        is_ui = __is_ui or false,
        slice = slice_basic(lay_ref)
    }
    last_layer_element_index = last_layer_element_index + 1
end

local function layer_append_existing(table_dest, room_to_lookup, __name, __is_ui)
    local lay_ref = room_to_lookup:get_layer(last_layer_element_index)

    table_dest[__name] = {
        index = last_layer_element_index,
        name = __name,
        layer = lay_ref,
        is_ui = __is_ui or false,
        slice = slice_basic(lay_ref)
    }

    print("generated " .. __name)

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

local function make_test_level()
    test_level = le.create_level("test")

    if test_level == nil then
        log_error("error creating level")
        os.exit()
    end
end

local function generate_view_and_layers()
    local menu = {}

    -- layer_new_invisible(g_menu, "dev", safe_layer_create(menu_room, "engine", 1, 0))
    layer_append_render(menu, "floor", safe_layer_create(menu_room, "engine", 1, 0))
    layer_append_render(menu, "objects", safe_layer_create(menu_room, "engine", 1, 2))
    layer_append_render(menu, "pallete", safe_layer_create(menu_room, "engine", 1, 0))
    layer_append_render(menu, "text", safe_layer_create(menu_room, "engine", 1, 2), true)
    layer_append_render(menu, "mouse", safe_layer_create(menu_room, "engine", 1, 2))
    -- layer_append_render(g_menu, "ui_back", safe_layer_create(menu_room, "engine", 1, 2), true)
    -- layer_append_render(g_menu, "ui_floor_select", safe_layer_create(menu_room, "engine", 1, 0), true)
    -- layer_append_render(g_menu, "ui_engine_select", safe_layer_create(menu_room, "engine", 1, 0), true)

    return menu
end

local function reflective_view_build()
    local menu = {}

    layer_append_existing(menu, menu_room, "floor")
    layer_append_existing(menu, menu_room, "objects")
    layer_append_existing(menu, menu_room, "pallete")
    layer_append_existing(menu, menu_room, "text", true)
    layer_append_existing(menu, menu_room, "mouse")

    return menu
end

test_level = le.load_level("test")

if test_level == nil then
    print("test.lvl file not found, creating from scratch")

    make_test_level() -- allocates sum space for the level

    g_engine = safe_registry_load(test_level, "engine") -- load the engine registry

    menu_room = safe_menu_create(test_level, "menu", g_width_blocks, g_height_blocks) -- our first and only room

    g_menu = generate_view_and_layers() -- creates all the layers
else -- level was loaded, all the rooms are here, all the layers are here as well
    print("loaded test.lvl")
    -- we have to find all the objects ourselvs
    g_engine = test_level:get_registries()[1]

    menu_room = test_level:find_room("menu") -- menu should be here

    if menu_room == nil then
        -- print("menu not found")
        log_error("menu not found")
        os.exit(0)
    end

    -- now assemble the g_menu view structure
    g_menu = reflective_view_build()
end

-- print_table(g_menu)

-- utils
g_engine_table = g_engine:to_table()
g_total_blocks = tablelength(g_engine_table)
g_character_id = find_block(g_engine_table, "character").id

try(function()
    set_render_rule_order(g_menu)
    set_slices(g_menu)
end, function(e)
    log_error(e)
    os.exit()
end)
