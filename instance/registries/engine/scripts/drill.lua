local game_data = require("registries.engine.scripts.game_data")
local block_utils = require("registries.engine.scripts.block_utils")

local current_block = scripting_current_block_id
local refuelable_component = scripting_current_block_api.refuelable

local function feed_fuel(layer, x, y)
    if refuelable_component.get_fuel(layer, x, y) >= game_data.machine_max_fuel then
        return
    end

    if block_utils.consume_fuel(layer, x, y) then
        refuelable_component.add_fuel(layer, x, y, 20)
    end
end

local function free_items_cell(x, y)
    return block_utils.adjacent_block(G_view_menu.items.layer, x, y, 0)
end

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    function(layer, x, y, value)
        local vars = layer:get_vars(x, y)
        if not vars then return end

        feed_fuel(layer, x, y)

        if not refuelable_component.spend_fuel(G_view_menu.items.layer, x, y, game_data.mine_cost) then
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
                        local bx, by = free_items_cell(x, y)
                        if not bx or not by then
                            vars:set_u8("v", 0)
                            return
                        end
                        local scrap = game_data.random_scrap_id()
                        if scrap ~= 0 then
                            game_data.place_item(G_view_menu.items.layer, bx, by, scrap, x, y)
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
