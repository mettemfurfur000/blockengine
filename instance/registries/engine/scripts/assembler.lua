local game_data = require("registries.engine.scripts.game_data")
local block_utils = require("registries.engine.scripts.block_utils")

local current_block = scripting_current_block_id
local refuelable_component = scripting_current_block_api.refuelable

local function adjacent_scrap(x, y)
    local blocks = block_utils.adjacent_blocks_all(G_view_menu.items.layer, x, y)
    local scrap = {}

    for _, block in ipairs(blocks) do
        if game_data.is_scrap(block.id) then
            scrap[#scrap + 1] = { id = block.id, price = game_data.price_of_id(block.id), x = block.x, y = block.y }
        end
    end

    table.sort(scrap, function(a, b) return a.price < b.price end)
    return scrap
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

        local fuel = refuelable_component.get_fuel(layer, x, y)

        if fuel < game_data.machine_max_fuel then
            if block_utils.consume_fuel(layer, x, y) then
                refuelable_component.add_fuel(layer, x, y, game_data.fuel_value)
            end
        end

        local around = adjacent_scrap(x, y)
        if fuel <= 0 or #around < 2 then
            vars:set_u8("v", 0)
            return
        end

        if not refuelable_component.spend_fuel(G_view_menu.items.layer, x, y, game_data.craft_cost) then
            vars:set_u8("v", 0)
            return
        end

        local consumed = around[1]
        local second = around[2]
        local total_price = consumed.price + second.price
        local threshold = second.price

        local target = craft_target(total_price * 2, threshold)
        if not target then
            vars:set_u8("v", 0)
            return
        end

        local bx, by = block_utils.adjacent_block(G_view_menu.items.layer, x, y, 0)
        if not bx then
            vars:set_u8("v", 0)
            return
        end

        G_view_menu.items.layer:paste_block(consumed.x, consumed.y, 0)
        G_view_menu.items.layer:paste_block(second.x, second.y, 0)

        game_data.place_item(G_view_menu.items.layer, bx, by, target, x, y)
        vars:set_u8("v", 1 + G_tick % 3)
    end
)
