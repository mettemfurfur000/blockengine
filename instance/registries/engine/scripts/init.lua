local sdl = require("registries.engine.scripts.definitions.sdl")
local blockengine = require("registries.engine.scripts.definitions.blockengine")
local wrappers = require("registries.engine.scripts.wrappers")
local level_editor = require("registries.engine.scripts.definitions.level_editor")
local camera_utils = require("registries.engine.scripts.camera_utils")
local game_data = require("registries.engine.scripts.game_data")

G_block_size = 16
G_global_zoom = 1
G_block_width_pixels = G_block_size * G_global_zoom

--- loads and registers layer editor functions
local _ = require("registries.engine.scripts.layer_editor")

if render_rules == nil then
    wrappers.log_error("render_rules module not found")
    return
end

G_screen_width, G_screen_height = render_rules.get_size(g_render_rules)

G_width_blocks = math.floor(2 * G_screen_width / (G_block_size * G_global_zoom))
G_height_blocks = math.floor(2 * G_screen_height / (G_block_size * G_global_zoom))

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
    return layer_slice(0, 0, G_screen_width, G_screen_height, G_global_zoom, lay_ref)
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
                    def_entry.use_vars or false,
                    def_entry.use_entities or false,
                    def_entry.is_ui or false
                ),
                def_entry.is_ui)
        end
    end

    return view
end

local function create_menu()
    G_level = level_editor.create_level("main_menu")

    if G_level == nil then
        wrappers.log_error("error creating level")
        os.exit()
    end

    -- G_level:add_existing(G_block_registry) -- add the existing engine registry to the level

    G_menu_room = wrappers.safe_room_create(G_level, "menu", G_width_blocks, G_height_blocks)
end

G_menu_definition = {
    [1] = { name = "floor", bytes = 1, use_vars = false, is_ui = false, use_entities = false },
    [2] = { name = "objects", bytes = 1, use_vars = true, is_ui = false, use_entities = true },
    [3] = { name = "items", bytes = 1, use_vars = false, is_ui = false, use_entities = true },
    [4] = { name = "pallete", bytes = 1, use_vars = true, is_ui = true, use_entities = false },
    [5] = { name = "text", bytes = 1, use_vars = true, is_ui = true, use_entities = false },
    [6] = { name = "mouse", bytes = 1, use_vars = true, is_ui = false, use_entities = false },
}

local function init_menu()
    G_level = level_editor.load_level("main_menu", G_block_registry)

    local loaded = G_level ~= nil

    if loaded then                              -- level was loaded, all the rooms are here, all the layers are here as well
        G_menu_room = G_level:find_room("menu") -- menu should be here
        assert(G_menu_room)
        print("loaded existing menu level")
    else
        create_menu()                          -- only creates the room, no layers yert
        G_level:add_existing(G_block_registry) -- add the existing engine registry to the level
        print("created new menu level")
    end

    G_level_loaded_marker = loaded

    G_engine = G_block_registry -- might not work if used multiple registries

    -- safety check
    if G_level:get_registries()[1] == nil then
        print("engine registry not found in menu level")
        os.exit()
    end

    -- if the room was just created, build_view will allocate layers for said menu definition
    G_view_menu = build_view(G_menu_definition, "engine", loaded)
end

G_tick = 0
G_sdl_tick = 0

blockengine.register_handler(events.ENGINE_TICK, function(code) -- tick over all existing jumpers
    G_tick = G_tick + 1
    G_sdl_tick = sdl.get_ticks()

    G_view_menu.objects.layer:tick(0) -- default tick - resets all values in a preparation for an actual pass
    G_view_menu.items.layer:tick(0)
    -- G_view_menu.objects.layer:tick(1)
    G_menu_room:box2d_tick()
end)

blockengine.register_handler(events.ENGINE_FRAME_PRE, function(code) -- tick over all existing jumpers
end)

blockengine.register_handler(events.SDL_QUIT, function(code) -- tick over all existing jumpers
    if G_level then
        level_editor.save_level(G_level)
        print("Saved level")
    end

    -- test serialize
    -- G_level:serialize_registry(0)
end)

blockengine.register_handler(events.SDL_WINDOWEVENT, function(width, height)
    if width == G_screen_width and height == G_screen_height then return end
    if width == nil or height == nil then return end

    print("updated window size to " .. width .. "x" .. height)

    G_screen_width = width
    G_screen_height = height

    camera_utils.recalc_camera_limits()

    G_view_menu = build_view(G_menu_definition, "engine", true)

    set_render_rule_order(G_view_menu)
end)

did_init = false

local function world_generate()
    local ground_id = wrappers.find_block(G_engine_table, "ground").id
    local cave_id = wrappers.find_block(G_engine_table, "cave_wall").id
    local keeper_id = wrappers.find_block(G_engine_table, "keeper").id
    local sell_id = wrappers.find_block(G_engine_table, "sell_pad").id
    local player_id = wrappers.find_block(G_engine_table, "player_bot").id
    local fuel_id = wrappers.find_block(G_engine_table, "fuel_cell").id
    local pile_id = wrappers.find_block(G_engine_table, "scrap_pile").id
    local rocks_id = wrappers.find_block(G_engine_table, "rocks").id

    local floor = G_view_menu.floor.layer
    local objects = G_view_menu.objects.layer
    local items = G_view_menu.items.layer

    local W, H = G_width_blocks, G_height_blocks

    for y = 0, H - 1 do
        for x = 0, W - 1 do
            floor:paste_block(x, y, ground_id)
            objects:paste_block(x, y, cave_id)
        end
    end

    local seed = sdl.get_ticks()
    if os ~= nil and os.time then seed = seed + os.time() end
    math.randomseed(seed)

    local rooms = {}
    local function carve_rect(x, y, w, h)
        for j = y, math.min(y + h - 1, H - 2) do
            for i = x, math.min(x + w - 1, W - 2) do
                objects:paste_block(i, j, 0)
            end
        end
    end

    local function corridor(x1, y1, x2, y2)
        local cx, cy = x1, y1
        while cx ~= x2 do
            objects:paste_block(cx, cy, 0)
            cx = cx + (cx < x2 and 1 or -1)
        end
        while cy ~= y2 do
            objects:paste_block(cx, cy, 0)
            cy = cy + (cy < y2 and 1 or -1)
        end
    end

    local room_count = 7
    for i = 1, room_count do
        local rw = math.random(5, 9)
        local rh = math.random(4, 7)
        local rx = math.random(2, W - rw - 2)
        local ry = math.random(2, H - rh - 2)
        carve_rect(rx, ry, rw, rh)
        table.insert(rooms, { x = rx + math.floor(rw / 2), y = ry + math.floor(rh / 2) })
    end

    carve_rect(2, 2, 12, 10) -- shop room
    table.insert(rooms, { x = 8, y = 7 })

    for i = 2, #rooms do
        corridor(rooms[i - 1].x, rooms[i - 1].y, rooms[i].x, rooms[i].y)
    end

    -- shop: keeper on a small platform + sell pad in front
    local shop_x, shop_y = 8, 6
    objects:paste_block(shop_x, shop_y, keeper_id)
    for j = shop_y + 1, shop_y + 3 do
        for i = shop_x, shop_x + 2 do
            objects:paste_block(i, j, 0)
            floor:paste_block(i, j, sell_id)
        end
    end

    -- starter fuel cells next to the shop
    items:paste_block(shop_x + 4, shop_y, fuel_id)
    items:paste_block(shop_x + 4, shop_y + 1, fuel_id)

    -- player spawn right under the shop platform
    local spawn_x, spawn_y = shop_x, shop_y + 4
    objects:paste_block(spawn_x, spawn_y, player_id)
    G_spawn_pos = { x = spawn_x, y = spawn_y }
    local pvars = objects:get_vars(spawn_x, spawn_y)
    if pvars then
        pvars:set_string("I", game_data.stack_init())
        pvars:set_u16("e", 60)
        pvars:set_u16("m", 60)
        pvars:set_u16("c", 100)
        pvars:set_u32("d", 0)
        pvars:set_u8("n", 0)
        pvars:set_u8("p", 0)
    end

    -- scatter scrap (rarer/more expensive further from the shop)
    local function dist(px, py)
        return math.abs(px - shop_x) + math.abs(py - shop_y)
    end
    for y = 0, H - 1 do
        for x = 0, W - 1 do
            if objects:get_id(x, y) == 0 then
                local d = dist(x, y)
                local roll = math.random()
                local chance = 0.02 + d * 0.0015
                if roll < chance then
                    local price_names = { "item_nut", "item_sheet", "item_pipe", "item_gear", "item_spring", "item_camera", "item_engine", "item_cpu" }
                    local pick = 1
                    if d > 30 then pick = math.random(5, 8)
                    elseif d > 18 then pick = math.random(3, 6)
                    elseif d > 8 then pick = math.random(1, 4)
                    else pick = math.random(1, 3) end
                    items:paste_block(x, y, wrappers.find_block(G_engine_table, price_names[pick]).id)
                elseif roll < chance + 0.02 then
                    floor:paste_block(x, y, rocks_id)
                end
            end
        end
    end

    -- scatter a few scrap piles for the drill
    for i = 1, 4 do
        local rx = math.random(6, W - 3)
        local ry = math.random(6, H - 3)
        for j = -1, 1 do
            for i2 = -1, 1 do
                if math.random() < 0.6 then
                    floor:paste_block(rx + i2, ry + j, pile_id)
                end
            end
        end
    end

    objects:build_ground_physics()
end

-- gets all the data
-- blockengine.register_handler(events.ENGINE_INIT, function()
blockengine.register_handler(events.ENGINE_INIT_GLOBALS, function()
    if did_init then return end
    print("initing menu")
    init_menu()

    -- utils
    G_engine_table = G_engine:to_table()

    G_total_blocks = wrappers.tablelength(G_engine_table)
    G_character_id = wrappers.find_block(G_engine_table, "character").id
    G_dev_id = wrappers.find_block(G_engine_table, "dev").id
    G_player_bot_id = wrappers.find_block(G_engine_table, "player_bot").id

    local _game_data = require("registries.engine.scripts.game_data")

    G_view_menu.text.layer:for_each(G_character_id, function(x, y) -- clear all left ova text
        G_view_menu.text.layer:paste_block(x, y, 0)
    end)

    G_view_menu.objects.layer:build_ground_physics()

    if not G_level_loaded_marker then
        world_generate()
    else
        G_view_menu.objects.layer:for_each(G_player_bot_id, function(px, py)
            if G_spawn_pos == nil then G_spawn_pos = { x = px, y = py } end
        end)
    end

    wrappers.try(function()
        set_render_rule_order(G_view_menu)
    end, function(e)
        wrappers.log_error(e)
        os.exit()
    end)

    did_init = true
end)

-- =========================================================================
-- Room / camera renderer integration (opt-in, script-controlled)
-- =========================================================================
if render_room ~= nil then
    blockengine.register_handler(events.ENGINE_INIT_GLOBALS, function()
        G_camera = render_room.create_camera(G_screen_width, G_screen_height, G_global_zoom)
        G_active_room = G_menu_room
        G_render_room_on = true

        render_room.set_options({
            clear_background = true,
            background_color = {0.16, 0.16, 0.22, 1.0},
            draw_grid = false,
        })

        G_center_dev_menu = function()
            local bx, by, layer = nil, nil, nil
            if G_spawn_pos ~= nil then
                bx, by = G_spawn_pos.x, G_spawn_pos.y
            else
                for _, v in pairs(G_view_menu) do
                    if type(v) == "table" and v.layer then
                        v.layer:for_each(G_dev_id, function(x, y)
                            if bx == nil then bx, by, layer = x, y, v.layer end
                        end)
                    end
                end
            end
            if bx == nil then
                bx = math.floor(G_width_blocks / 2)
                by = math.floor(G_height_blocks / 2)
            end
            G_camera:center_on((bx + 0.5) * G_block_width_pixels, (by + 0.5) * G_block_width_pixels)
        end

        G_center_dev_menu()

        render_room.activate(G_active_room, G_camera)
    end)

    blockengine.register_handler(events.SDL_MOUSEWHEEL, function(x, y, px, py)
        if G_camera == nil then
            return
        end
        local z = G_camera:get_zoom()
        if y > 0 then
            z = z + 0.2
        elseif y < 0 then
            z = z - 0.2
        end
        z = math.max(1, math.min(8, z))
        G_camera:set_zoom(z)
        G_global_zoom = z
        G_block_width_pixels = G_block_size * z
        camera_utils.set_target(G_camera_current_pos)
    end)

    blockengine.register_handler(events.SDL_WINDOWEVENT, function(width, height)
        if G_camera ~= nil and width ~= nil and height ~= nil then
            G_camera:set_viewport(width, height)
        end
    end)
end