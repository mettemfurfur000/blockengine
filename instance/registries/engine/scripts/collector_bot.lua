local game_data = require("registries.engine.scripts.game_data")

local current_block = scripting_current_block_id

local MAX_FUEL = 60
local MOVES_PER_FUEL = 10
local FUEL_DRAIN_INTERVAL_MS = 10000
local RETURN_RESERVE = 2
local DROP_EXCLUSION_RADIUS = 2
local MOVE_INTERVAL_MS = scripting_current_block_interp_takes

local function free_items_cell(x, y)
    for dy = -1, 1 do
        for dx = -1, 1 do
            if (dx ~= 0 or dy ~= 0) and G_view_menu.items.layer:get_id(x + dx, y + dy) == 0 then
                return x + dx, y + dy
            end
        end
    end
    return nil, nil
end

local function is_own_drop(vars, x, y)
    return vars:get_i16("d") == x and vars:get_i16("e") == y
end

local function path_between(x, y, target_x, target_y)
    return G_view_menu.objects.layer:find_path(x, y, target_x, target_y, G_view_menu.objects.layer)
end

local function move_along_path(layer, vars, x, y, target_x, target_y, fuel)
    local path = path_between(x, y, target_x, target_y)
    if not path or #path < 2 then
        return false
    end

    local next_node = path[2]
    local move_x = next_node.x - x
    local move_y = next_node.y - y
    if not layer:move_block(x, y, move_x, move_y) then
        return false
    end

    vars:set_i16("x", -move_x * G_block_width_pixels)
    vars:set_i16("y", -move_y * G_block_width_pixels)
    vars:set_u32("T", G_sdl_tick)
    local moves = (vars:get_u8("n") or 0) + 1
    if moves >= MOVES_PER_FUEL then
        fuel = fuel - 1
        moves = 0
    end
    vars:set_u8("n", moves)
    vars:set_u8("f", fuel)
    vars:set_u8("v", 1 + G_tick % 2)
    vars:set_u8("t", math.random(0, 3))
    return true
end

local function find_closest_block(x, y, ids)
    return G_view_menu.items.layer:find_closest(
        x, y, G_view_menu.items.layer, ids, G_view_menu.objects.layer)
end

local function find_nearest_scrap(x, y, vars)
    local home_x = vars:get_i16("h") or x
    local home_y = vars:get_i16("k") or y
    local target = G_view_menu.items.layer:find_closest(
        x, y, G_view_menu.items.layer, game_data.scrap_ids(), G_view_menu.objects.layer,
        home_x, home_y, DROP_EXCLUSION_RADIUS)
    if target and not is_own_drop(vars, target.x, target.y) then
        target.path = path_between(x, y, target.x, target.y)
        target.id = G_view_menu.items.layer:get_id(target.x, target.y)
        return target
    end
    return nil
end

local function consume_adjacent_battery(layer, x, y, fuel, fuel_id)
    for dy = -1, 1 do
        for dx = -1, 1 do
            if dx ~= 0 or dy ~= 0 then
                if layer:get_id(x + dx, y + dy) == fuel_id then
                    layer:paste_block(x + dx, y + dy, 0)
                    return MAX_FUEL
                end
            end
        end
    end
    return fuel
end

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    function(layer, x, y, value)
        local vars = layer:get_vars(x, y)
        if not vars then return end

        local moved_on_tick = vars:get_u32("T")
        if moved_on_tick == G_sdl_tick then return end

        local fuel = vars:get_u8("f") or 0
        local carrying = vars:get_u8("c") or 0

        if (vars:get_u8("s") or 0) == 0 then
            vars:set_i16("h", x)
            vars:set_i16("k", y)
            vars:set_u8("s", 1)
        end

        local fuel_id = game_data.id("fuel_cell")
        local now = G_sdl_tick or 0
        local last_drain = vars:get_u32("D") or 0
        if last_drain == 0 then
            vars:set_u32("D", now)
        elseif now > 0 and now - last_drain >= FUEL_DRAIN_INTERVAL_MS then
            fuel = math.max(0, fuel - 1)
            vars:set_u32("D", now)
            vars:set_u8("f", fuel)
        end
        if fuel == 0 then
            fuel = consume_adjacent_battery(G_view_menu.items.layer, x, y, fuel, fuel_id)
            vars:set_u8("f", fuel)
        end

        if fuel <= 0 then
            vars:set_u8("v", 0)
            vars:set_u8("t", 0)
            return
        end

        if moved_on_tick ~= 0 and now > 0 and now - moved_on_tick < MOVE_INTERVAL_MS then
            -- skipping this tick to respect the move interval
            return
        end

        if carrying ~= 0 then
            local home_x = vars:get_i16("h") or x
            local home_y = vars:get_i16("k") or y
            if x == home_x and y == home_y then
                local drop_x, drop_y = free_items_cell(x, y)
                if drop_x then
                    game_data.place_item(G_view_menu.items.layer, drop_x, drop_y, vars:get_u16("q") or 0, home_x, home_y)
                    vars:set_i16("d", drop_x)
                    vars:set_i16("e", drop_y)
                    vars:set_u16("q", 0)
                    vars:set_u8("c", 0)
                end
                vars:set_u8("v", 0)
                vars:set_u8("t", 0)
                return
            end

            move_along_path(layer, vars, x, y, home_x, home_y, fuel)
            return
        end

        local target = find_nearest_scrap(x, y, vars)
        if target then
            local home_x = vars:get_i16("h") or x
            local home_y = vars:get_i16("k") or y
            local return_path = path_between(target.x, target.y, home_x, home_y)
            local return_distance = return_path and (#return_path - 1) or nil
            local required_fuel = return_distance and (#target.path - 1) + return_distance + RETURN_RESERVE or nil

            if not required_fuel or required_fuel > MAX_FUEL then
                return
            end

            if required_fuel and fuel < required_fuel then
                local battery = find_closest_block(x, y, { fuel_id })
                if battery and battery.distance <= fuel + 1 then
                    if battery.distance == 1 then
                        G_view_menu.items.layer:paste_block(battery.x, battery.y, 0)
                        fuel = MAX_FUEL
                        vars:set_u8("f", fuel)
                        if fuel >= required_fuel then
                            target.path = path_between(x, y, target.x, target.y)
                        else
                            return
                        end
                    else
                        battery.path = path_between(x, y, battery.x, battery.y)
                        move_along_path(layer, vars, x, y, battery.x, battery.y, fuel)
                        return
                    end
                else
                    return
                end
            end

            if #target.path > 2 then
                move_along_path(layer, vars, x, y, target.x, target.y, fuel)
                return
            end

            local px, py = free_items_cell(x, y)
            if px then
                local item_id = G_view_menu.items.layer:get_id(target.x, target.y)
                G_view_menu.items.layer:paste_block(target.x, target.y, 0)
                vars:set_u16("q", item_id)
                vars:set_u8("c", 1)
                vars:set_u8("v", 1 + G_tick % 2)
                vars:set_u8("t", math.random(0, 3))
            end
            return
        end

        vars:set_u8("v", 0)
        vars:set_u8("t", 0)
    end
)
