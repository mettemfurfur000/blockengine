local sdl = require("registries.engine.scripts.definitions.sdl")
local blockengine = require("registries.engine.scripts.definitions.blockengine")
local wrappers = require("registries.engine.scripts.wrappers")
local level_editor = require("registries.engine.scripts.definitions.level_editor")
local camera_utils = require("registries.engine.scripts.camera_utils")

require("registries.engine.scripts.constants")

--- loads and registers layer editor functions
require("registries.engine.scripts.layer_editor")

G_screen_width, G_screen_height = render_rules.get_size(g_render_rules)

G_width_blocks = math.floor(2 * G_screen_width / (g_block_size * global_zoom))
G_height_blocks = math.floor(2 * G_screen_height / (g_block_size * global_zoom))

-- constructs a layer slice that dictates how the layer is rendered
local function layer_slice(x, y, w, h, z, lay_ref)
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

local function make_basic_slice(lay_ref)
    return layer_slice(0, 0, G_screen_width, G_screen_height, global_zoom, lay_ref)
end

G_layers_amount = 0

local function layer_append_render(table_dest, __name, lay_ref, __is_ui)
    table_dest[__name] = {
        index = G_layers_amount,
        name = __name,
        layer = lay_ref,
        is_ui = __is_ui or false,
        slice = make_basic_slice(lay_ref)
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
        slice = make_basic_slice(lay_ref)
    }

    G_layers_amount = G_layers_amount + 1
end

local function set_slices(ref_table)
    for k, v in pairs(ref_table) do
        if v.slice_push_ignore ~= true then
            render_rules.set_slice(g_render_rules, v.index, v.slice)
        end
    end
end

local function set_render_rule_order(ref_table)
    local order = {}

    for k, v in pairs(ref_table) do
        if v.slice_push_ignore ~= true then
            table.insert(order, v.index)
        end
    end

    table.sort(order)

    render_rules.set_order(g_render_rules, order)

    set_slices(ref_table)
end

local function build_view(def, registry_name, loaded)
    loaded = loaded or false

    local view = {}

    G_layers_amount = 0 -- make sure to start over

    for key, def_entry in ipairs(def) do
        if loaded then
            layer_append_existing(view, G_menu_room, def_entry.name, def_entry.is_ui)
        else
            layer_append_render(view, def_entry.name,
                wrappers.safe_layer_create(
                    G_menu_room,
                    registry_name,
                    def_entry.bytes or 1,
                    def_entry.use_vars or false
                ),
                def_entry.is_ui)
        end
    end

    return view
end

local function define_layer(name, bytes, use_vars, is_ui)
    if name == nil then
        error("layer has to have a name")
    end
    return { name = name, bytes = bytes or 1, use_vars = use_vars or false, is_ui = is_ui or false }
end

local function create_menu()
    G_level = level_editor.create_level("main_menu")

    if G_level == nil then
        wrappers.log_error("error creating level")
        os.exit()
    end

    G_engine = wrappers.safe_registry_load(G_level, "engine")

    G_menu_room = wrappers.safe_room_create(G_level, "menu", G_width_blocks, G_height_blocks)
end

G_menu_definition = {
    [1] = define_layer("floor", 1),
    [2] = define_layer("objects", 1, true),
    [3] = define_layer("pallete", 1, true, true),
    [4] = define_layer("text", 1, true, true),
    [5] = define_layer("mouse", 1, true, true),
}

local function init_menu()
    G_level = level_editor.load_level("main_menu")

    local loaded = G_level ~= nil

    if loaded then                              -- level was loaded, all the rooms are here, all the layers are here as well
        G_engine = G_level:get_registries()[1]
        G_menu_room = G_level:find_room("menu") -- menu should be here
        assert(G_menu_room)
    else
        create_menu() -- only creates the room, no layers yert
    end

    -- if the room was just created, build_view will allocate layers for said menu definition
    G_view_menu = build_view(G_menu_definition, "engine", loaded)
end

init_menu()
-- print_table(g_menu)

-- utils
G_engine_table = G_engine:to_table()
G_total_blocks = wrappers.tablelength(G_engine_table)
G_character_id = wrappers.find_block(G_engine_table, "character").id

G_view_menu.text.layer:for_each(G_character_id, function(x, y) -- clear all left ova text
    G_view_menu.text.layer:paste_block(x, y, 0)
end)

G_tick = 0
G_sdl_tick = 0

blockengine.register_handler(engine_events.ENGINE_TICK, function(code) -- tick over all existing jumpers
    G_tick = G_tick + 1
    G_sdl_tick = sdl.get_ticks()
    G_view_menu.objects.layer:tick(0) -- default tick - resets all values in a preparation for an actual pass
    G_view_menu.objects.layer:tick(1)
end)

blockengine.register_handler(sdl_events.SDL_QUIT, function(code) -- tick over all existing jumpers
    if G_level then
        level_editor.save_level(G_level)
        print("Saved level")
    end
end)

blockengine.register_handler(sdl_events.SDL_WINDOWEVENT, function(width, height)
    if width == G_screen_width and height == G_screen_height then return end
    if width == nil or height == nil then return end

    print("updated window size to " .. width .. "x" .. height)

    G_screen_width = width
    G_screen_height = height

    camera_utils.recalc_camera_limits()

    G_view_menu = build_view(G_menu_definition, "engine", true)

    set_render_rule_order(G_view_menu)
end)

wrappers.try(function()
    set_render_rule_order(G_view_menu)
end, function(e)
    wrappers.log_error(e)
    os.exit()
end)
