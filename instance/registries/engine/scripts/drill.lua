local game_data = require("registries.engine.scripts.game_data")
local block_utils = require("registries.engine.scripts.block_utils")

local current_block = scripting_current_block_id
local refuelable_trait = scripting_current_block_api.refuelable

local function maintain_fuel(layer, x, y)
    local fuel_current = refuelable_trait.get_fuel(layer, x, y)

    if fuel_current ~= 0 then
        return fuel_current
    end

    if block_utils.consume_fuel(layer, x, y) then
        refuelable_trait.add_fuel(layer, x, y, game_data.fuel_value)

        fuel_current = refuelable_trait.get_fuel(layer, x, y)
    end

    return fuel_current
end

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    function(layer, x, y, value)
        local vars = layer:get_vars(x, y)
        if not vars then return end

        local current_fuel = maintain_fuel(layer, x, y)

        if current_fuel == 0 then
            vars:set_u8("v", 0)
            return
        end

        local pile_id = game_data.id("scrap_pile")
        for dy = -1, 1 do
            for dx = -1, 1 do
                if G_view_menu.floor.layer:get_id(x + dx, y + dy) == pile_id then
                    vars:set_u8("v", 1 + G_tick % 2) -- oscillate the drill bit every tick

                    -- 1% chance to break the pile every tick
                    -- 5% chance to generate scrap every tick

                    if math.random(1, 100) <= 5 then
                        local bx, by = block_utils.adjacent_block(G_view_menu.items.layer, x, y, 0)
                        if not bx or not by then
                            vars:set_u8("v", 0)
                            return
                        end

                        local scrap = game_data.random_scrap_id()
                        if scrap ~= 0 then
                            game_data.place_item(G_view_menu.items.layer, bx, by, scrap, x, y)
                            refuelable_trait.spend_fuel(layer, x, y, game_data.mine_cost)
                        end
                    end

                    if math.random(1, 100) <= 1 then
                        G_view_menu.floor.layer:paste_block(x + dx, y + dy, 0)
                    end

                    return
                end
            end
        end

        vars:set_u8("v", 0)
    end
)