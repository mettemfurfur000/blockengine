local game_data = require("registries.engine.scripts.game_data")

local current_block = scripting_current_block_id

local MAX_FUEL = 10

local function free_items_cell(x, y)
    for dy = -1, 1 do
        for dx = -1, 1 do
            if dx ~= 0 or dy ~= 0 then
                if G_view_menu.items.layer:get_id(x + dx, y + dy) == 0 then
                    return x + dx, y + dy
                end
            end
        end
    end
    return nil, nil
end

local function adjacent_scrap(x, y)
    local ids = {}
    for dy = -1, 1 do
        for dx = -1, 1 do
            if dx ~= 0 or dy ~= 0 then
                local id = G_view_menu.items.layer:get_id(x + dx, y + dy)
                if game_data.is_scrap(id) then
                    ids[#ids + 1] = { id = id, price = game_data.price_of_id(id), x = x + dx, y = y + dy }
                end
            end
        end
    end
    table.sort(ids, function(a, b) return a.price < b.price end)
    return ids
end

local function craft_target(total_price, threshold)
    local best, best_name = 0, nil
    for name, price in pairs(game_data.price) do
        if price <= total_price and price > threshold and price > best then
            best = price
            best_name = name
        end
    end
    if not best_name then return nil end
    local id = game_data.id(best_name)
    if id == 0 then return nil end
    return id
end

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    function(layer, x, y, value)
        local vars = layer:get_vars(x, y)
        if not vars then return end

        local fuel = vars:get_u8("f") or 0

        local fuel_id = game_data.id("fuel_cell")
        if fuel < MAX_FUEL then
            for dy = -1, 1 do
                for dx = -1, 1 do
                    if dx ~= 0 or dy ~= 0 then
                        if G_view_menu.items.layer:get_id(x + dx, y + dy) == fuel_id then
                            G_view_menu.items.layer:paste_block(x + dx, y + dy, 0)
                            fuel = fuel + 1
                            vars:set_u8("f", fuel)
                        end
                    end
                end
            end
        end

        local around = adjacent_scrap(x, y)
        if fuel <= 0 or #around < 2 then
            vars:set_u8("v", 0)
            return
        end

        local consumed = around[1]
        local second = around[2]
        local total_price = consumed.price + second.price
        local threshold = second.price

        local target = craft_target(total_price, threshold)
        if not target then
            vars:set_u8("v", 0)
            return
        end

        local bx, by = free_items_cell(x, y)
        if not bx then
            vars:set_u8("v", 0)
            return
        end

        G_view_menu.items.layer:paste_block(consumed.x, consumed.y, 0)
        G_view_menu.items.layer:paste_block(second.x, second.y, 0)

        vars:set_u8("f", fuel - 1)
        game_data.place_item(G_view_menu.items.layer, bx, by, target, x, y)
        vars:set_u8("v", 1 + G_tick % 3)
    end
)