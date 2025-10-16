local sdl = require("registries.engine.scripts.definitions.sdl")
local blockengine = require("registries.engine.scripts.definitions.blockengine")
local wrappers = require("registries.engine.scripts.wrappers")
local level_editor = require("registries.engine.scripts.definitions.level_editor")

require("registries.engine.scripts.constants")

G_screen_width, G_screen_height = render_rules.get_size(g_render_rules)

G_width_blocks = 2 * G_screen_width / (g_block_size * global_zoom)
G_height_blocks = 2 * G_screen_height / (g_block_size * global_zoom)

local function slice_gen(x, y, w, h, z, lay_ref)
    if lay_ref == nil then
        print("no ref layer to generate a slice")
        os.exit(0)
    end
    return {
        x = x,
        y = y,
        old_x = x,
        old_y = y,
        timestamp_old = G_sdl_tick or sdl.get_ticks(),
        h = h,
        w = w,
        zoom = z,
        ref = lay_ref,
        flags = 0
    }
end

local function slice_basic(lay_ref)
    return slice_gen(0, 0, G_screen_width, G_screen_height, global_zoom, lay_ref)
end

G_layers_amount = 0

local function layer_append_render(table_dest, __name, lay_ref, __is_ui)
    table_dest[__name] = {
        index = G_layers_amount,
        name = __name,
        layer = lay_ref,
        is_ui = __is_ui or false,
        slice = slice_basic(lay_ref)
    }
    G_layers_amount = G_layers_amount + 1
end

local function layer_append_existing(table_dest, room_to_lookup, __name, __is_ui)
    local lay_ref = room_to_lookup:get_layer(G_layers_amount)

    table_dest[__name] = {
        index = G_layers_amount,
        name = __name,
        layer = lay_ref,
        is_ui = __is_ui or false,
        slice = slice_basic(lay_ref)
    }

    print("generated " .. __name)

    G_layers_amount = G_layers_amount + 1
end

local function layer_new_invisible(table_dest, __name, lay_ref)
    table_dest[__name] = {
        index = G_layers_amount,
        name = __name,
        layer = lay_ref,
        is_ui = false,
        slice = nil,
        slice_push_ignore = true
    }
    G_layers_amount = G_layers_amount + 1
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
    wrappers.print_table(order)

    render_rules.set_order(g_render_rules, order)
end

local function set_slices(ref_table)
    print("setting slices")
    for k, v in pairs(ref_table) do
        if v.slice_push_ignore ~= true then
            print(v.name .. " at index " .. v.index)
            render_rules.set_slice(g_render_rules, v.index, v.slice)
        end
    end
end

local function make_test_level()
    G_level = le.create_level("test")

    if G_level == nil then
        log_error("error creating level")
        os.exit()
    end
end

local function generate_view_and_layers()
    local menu = {}

    -- layer_new_invisible(g_menu, "dev", safe_layer_create(g_menu_room, "engine", 1, 0))
    layer_append_render(menu, "floor", wrappers.safe_layer_create(G_menu_room, "engine", 1, 0))
    layer_append_render(menu, "objects", wrappers.safe_layer_create(G_menu_room, "engine", 1, 2))
    layer_append_render(menu, "pallete", wrappers.safe_layer_create(G_menu_room, "engine", 1, 0), true)
    layer_append_render(menu, "text", wrappers.safe_layer_create(G_menu_room, "engine", 1, 2), true)
    layer_append_render(menu, "mouse", wrappers.safe_layer_create(G_menu_room, "engine", 1, 2), true)
    -- layer_append_render(g_menu, "ui_back", safe_layer_create(g_menu_room, "engine", 1, 2), true)
    -- layer_append_render(g_menu, "ui_floor_select", safe_layer_create(g_menu_room, "engine", 1, 0), true)
    -- layer_append_render(g_menu, "ui_engine_select", safe_layer_create(g_menu_room, "engine", 1, 0), true)

    return menu
end

local function reflective_view_build()
    local menu = {}

    layer_append_existing(menu, G_menu_room, "floor")
    layer_append_existing(menu, G_menu_room, "objects")
    layer_append_existing(menu, G_menu_room, "pallete", true)
    layer_append_existing(menu, G_menu_room, "text", true)
    layer_append_existing(menu, G_menu_room, "mouse", true)

    return menu
end

G_level = le.load_level("test")

if G_level == nil then
    print("test.lvl file not found, creating from scratch")

    make_test_level()                                                                         -- allocates sum space for the level

    G_engine = wrappers.safe_registry_load(G_level, "engine")                                 -- load the engine registry

    G_menu_room = wrappers.safe_menu_create(G_level, "menu", G_width_blocks, G_height_blocks) -- our first and only room

    G_menu = generate_view_and_layers()                                                       -- creates all the layers
else                                                                                          -- level was loaded, all the rooms are here, all the layers are here as well
    print("loaded test.lvl")
    -- we have to find all the objects ourselvs
    G_engine = G_level:get_registries()[1]

    G_menu_room = G_level:find_room("menu") -- menu should be here

    if G_menu_room == nil then
        -- print("menu not found")
        log_error("menu not found")
        os.exit(0)
    end

    -- now assemble the g_menu view structure
    G_menu = reflective_view_build()
end

-- print_table(g_menu)

-- utils
G_engine_table = G_engine:to_table()
G_total_blocks = wrappers.tablelength(G_engine_table)
G_character_id = wrappers.find_block(G_engine_table, "character").id

G_menu.text.layer:for_each(G_character_id, function(x, y) -- clear all left ova text
    G_menu.text.layer:paste_block(x, y, 0)
end)

G_tick = 0
G_sdl_tick = 0

blockengine.register_handler(engine_events.ENGINE_TICK, function(code) -- tick over all existing jumpers
    G_tick = G_tick + 1
    G_sdl_tick = sdl.get_ticks()
    G_menu.objects.layer:tick(0) -- default tick - resets all values in a preparation for an actual pass
    G_menu.objects.layer:tick(1)
end)

blockengine.register_handler(sdl_events.SDL_QUIT, function(code) -- tick over all existing jumpers
    if G_level then
        level_editor.save_level(G_level)
        print("Saved level ")
    end
end)

wrappers.try(function()
    set_render_rule_order(G_menu)
    set_slices(G_menu)
end, function(e)
    log_error(e)
    os.exit()
end)
