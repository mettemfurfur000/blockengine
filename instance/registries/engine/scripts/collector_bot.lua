local game_data = require("registries.engine.scripts.game_data")

local current_block = scripting_current_block_id

local MAX_FUEL = 60
local MOVES_PER_FUEL = 25
local FUEL_DRAIN_INTERVAL_MS = 10000
local RETURN_RESERVE = 2
local MOVE_INTERVAL_MS = scripting_current_block_interp_takes
local PATH_IGNORE_SOURCE = 1

local BEACON_OFFSETS = {
    { x = 1,  y = 1 },
    { x = 0,  y = 1 },
    { x = -1, y = 1 },
    { x = 1,  y = -1 },
    { x = 0,  y = -1 },
    { x = -1, y = -1 },
    { x = 1,  y = 0 },
    { x = -1, y = 0 }
}

local reservations = {}
local next_owner_id = 1

local function reservation_key(x, y)
    return x .. ":" .. y
end

local function owner_id(vars)
    local id = vars:get_i16("a") or 0
    if id == 0 then
        id = next_owner_id
        next_owner_id = next_owner_id + 1
        vars:set_i16("a", id)
    end
    return id
end

local function release_reservation(vars)
    local id = owner_id(vars)
    for key, owner in pairs(reservations) do
        if owner == id then
            reservations[key] = nil
        end
    end
end

local function reserve_target(vars, x, y)
    local key = reservation_key(x, y)
    local id = owner_id(vars)
    local owner = reservations[key]
    if owner ~= nil and owner ~= id then
        return false
    end

    release_reservation(vars)
    reservations[key] = id
    return true
end

local function owns_reservation(vars, x, y)
    return reservations[reservation_key(x, y)] == owner_id(vars)
end

local function path_between(x, y, target_x, target_y)
    return G_view_menu.objects.layer:find_path(
        x, y, target_x, target_y, G_view_menu.objects.layer, PATH_IGNORE_SOURCE)
end

local function move_along_path(layer, vars, x, y, target_x, target_y, fuel)
    local path = path_between(x, y, target_x, target_y)
    if not path or #path < 2 then
        print("Failed to find path for collector bot from (" ..
            x .. ", " .. y .. ") to (" .. target_x .. ", " .. target_y .. ")")
        return false
    end

    local next_node = path[2]
    local move_x = next_node.x - x
    local move_y = next_node.y - y

    -- check if your next block is non-empty, if so, don't move and wait for the next tick
    if layer:get_id(x + move_x, y + move_y) ~= 0 then
        return false
    end

    if not layer:move_block(x, y, move_x, move_y) then
        print("Failed to move collector bot from (" ..
            x .. ", " .. y .. ") to (" .. (x + move_x) .. ", " .. (y + move_y) .. ")")
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

local function find_beacon_drop_cell(x, y)
    local beacon_id = game_data.id("collector_beacon")
    if beacon_id == 0 then
        return nil
    end
    local beacon = G_view_menu.objects.layer:find_closest(
        x, y, G_view_menu.objects.layer, { beacon_id }, G_view_menu.objects.layer)
    if not beacon then
        return nil
    end

    local best = nil
    local width, height = G_view_menu.items.layer:get_size()
    for _, item_offset in ipairs(BEACON_OFFSETS) do
        local item_x = beacon.x + item_offset.x
        local item_y = beacon.y + item_offset.y
        if item_x >= 0 and item_y >= 0 and item_x < width and item_y < height and
            G_view_menu.objects.layer:get_id(item_x, item_y) == 0 and
            G_view_menu.items.layer:get_id(item_x, item_y) == 0 then
            local path = path_between(x, y, item_x, item_y)
            if path and (best == nil or #path < #best.path) then
                best = {
                    x = item_x,
                    y = item_y,
                    item_x = item_x,
                    item_y = item_y,
                    path = path
                }
            end
        end
    end
    return best
end

local function find_nearest_scrap(x, y, vars)
    local beacon_id = game_data.id("collector_beacon")
    local width, height = G_view_menu.items.layer:get_size()
    local best = nil

    local function is_beacon_storage_cell(cell_x, cell_y)
        if beacon_id == 0 then
            return false
        end
        for _, offset in ipairs(BEACON_OFFSETS) do
            if G_view_menu.objects.layer:get_id(cell_x + offset.x, cell_y + offset.y) == beacon_id then
                return true
            end
        end
        return false
    end

    for target_y = 0, height - 1 do
        for target_x = 0, width - 1 do
            local target_id = G_view_menu.items.layer:get_id(target_x, target_y)
            local distance = math.abs(target_x - x) + math.abs(target_y - y)
            local key = reservation_key(target_x, target_y)
            if game_data.is_scrap(target_id) and not is_beacon_storage_cell(target_x, target_y) and
                (reservations[key] == nil or owns_reservation(vars, target_x, target_y)) and
                (best == nil or distance < best.distance) then
                local path = path_between(x, y, target_x, target_y)
                if path then
                    best = {
                        x = target_x,
                        y = target_y,
                        id = target_id,
                        path = path,
                        distance = distance
                    }
                end
            end
        end
    end

    if best and reserve_target(vars, best.x, best.y) then
        return best
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
            if carrying == 0 then
                release_reservation(vars)
            end
            vars:set_u8("v", 0)
            vars:set_u8("t", 0)
            return
        end

        if moved_on_tick ~= 0 and now > 0 and now - moved_on_tick < MOVE_INTERVAL_MS then
            -- skipping this tick to respect the move interval
            return
        end

        if carrying ~= 0 then
            local item_x = vars:get_i16("d")
            local item_y = vars:get_i16("e")
            if not item_x or not item_y then
                item_x, item_y = x, y
            end
            if x == item_x and y == item_y then
                game_data.place_item(G_view_menu.items.layer, item_x, item_y,
                    vars:get_u16("q") or 0, x, y)
                vars:set_u16("q", 0)
                vars:set_u8("c", 0)
                release_reservation(vars)
                vars:set_u8("v", 0)
                vars:set_u8("t", 0)
                return
            end

            if G_view_menu.objects.layer:get_id(item_x, item_y) ~= 0 or
                G_view_menu.items.layer:get_id(item_x, item_y) ~= 0 then
                item_x, item_y = x, y
                vars:set_i16("d", item_x)
                vars:set_i16("e", item_y)
                game_data.place_item(G_view_menu.items.layer, item_x, item_y,
                    vars:get_u16("q") or 0, x, y)
                vars:set_u16("q", 0)
                vars:set_u8("c", 0)
                release_reservation(vars)
                vars:set_u8("v", 0)
                vars:set_u8("t", 0)
                return
            end

            local path = path_between(x, y, item_x, item_y)
            if path then
                move_along_path(layer, vars, x, y, item_x, item_y, fuel)
            else
                game_data.place_item(G_view_menu.items.layer, x, y, vars:get_u16("q") or 0, x, y)
                vars:set_u16("q", 0)
                vars:set_u8("c", 0)
                release_reservation(vars)
            end
            return
        end

        local target = nil
        if (vars:get_u8("s") or 0) ~= 0 then
            local target_x = vars:get_i16("h")
            local target_y = vars:get_i16("k")
            local target_id = target_x and target_y and
                G_view_menu.items.layer:get_id(target_x, target_y) or 0
            local target_key = target_x and target_y and reservation_key(target_x, target_y)
            if target_x and target_y and game_data.is_scrap(target_id) and
                (reservations[target_key] == nil or owns_reservation(vars, target_x, target_y)) then
                if not owns_reservation(vars, target_x, target_y) and
                    not reserve_target(vars, target_x, target_y) then
                    vars:set_u8("s", 0)
                else
                    target = {
                        x = target_x,
                        y = target_y,
                        id = target_id,
                        path = path_between(x, y, target_x, target_y)
                    }
                end
            else
                vars:set_u8("s", 0)
                release_reservation(vars)
            end
        end
        if not target then
            target = find_nearest_scrap(x, y, vars)
            if target then
                vars:set_i16("h", target.x)
                vars:set_i16("k", target.y)
                vars:set_u8("s", 1)
            end
        end
        if target then
            if not target.path then
                return
            end
            local drop = find_beacon_drop_cell(target.x, target.y)
            local return_distance = drop and (#drop.path - 1) or 0
            local required_fuel = (#target.path - 1) + return_distance + RETURN_RESERVE

            required_fuel = math.ceil(required_fuel / MOVES_PER_FUEL)

            if required_fuel > MAX_FUEL then
                release_reservation(vars)
                print("Cannot collect scrap at " ..
                    target.x ..
                    ", " ..
                    target.y .. ": required fuel is " .. (required_fuel or "unknown") .. ", max fuel is " .. MAX_FUEL)
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
                            release_reservation(vars)
                            return
                        end
                    else
                        battery.path = path_between(x, y, battery.x, battery.y)
                        move_along_path(layer, vars, x, y, battery.x, battery.y, fuel)
                        return
                    end
                else
                    release_reservation(vars)
                    return
                end
            end

            if #target.path > 2 then
                move_along_path(layer, vars, x, y, target.x, target.y, fuel)
                return
            end

            local item_id = G_view_menu.items.layer:get_id(target.x, target.y)
            if item_id ~= 0 and game_data.is_scrap(item_id) and
                owns_reservation(vars, target.x, target.y) then
                G_view_menu.items.layer:paste_block(target.x, target.y, 0)
                vars:set_u16("q", item_id)
                vars:set_u8("c", 1)
                vars:set_u8("s", 0)
                local drop = find_beacon_drop_cell(x, y)
                vars:set_i16("d", drop and drop.item_x or x)
                vars:set_i16("e", drop and drop.item_y or y)
                vars:set_u8("v", 1 + G_tick % 2)
                vars:set_u8("t", math.random(0, 3))
                release_reservation(vars)
            else
                vars:set_u8("s", 0)
                release_reservation(vars)
            end
            return
        end

        vars:set_u8("v", 0)
        vars:set_u8("t", 0)
    end
)
