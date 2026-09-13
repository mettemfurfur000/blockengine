local wrappers = require("registries.engine.scripts.wrappers")
local game_data = require("registries.engine.scripts.game_data")

local current_block = scripting_current_block_id

local MAX_FUEL = 10

local function feed_fuel(layer, x, y)
    local vars = layer:get_vars(x, y)
    if not vars then return end
    local fuel = vars:get_u8("f") or 0
    if fuel >= MAX_FUEL then return end

    local fuel_id = game_data.id("fuel_cell")
    for dy = -1, 1 do
        for dx = -1, 1 do
            if dx ~= 0 or dy ~= 0 then
                if G_view_menu.items.layer:get_id(x + dx, y + dy) == fuel_id then
                    G_view_menu.items.layer:paste_block(x + dx, y + dy, 0)
                    vars:set_u8("f", fuel + 20)
                    return
                end
            end
        end
    end
end

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
    return x, y + 1
end

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    function(layer, x, y, value)
        local vars = layer:get_vars(x, y)
        if not vars then return end

        feed_fuel(layer, x, y)
        local fuel = vars:get_u8("f") or 0
        if fuel <= 0 then
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
                    -- 20% chance to consume fuel every tick

                    if math.random(1, 100) <= 5 then
                        local bx, by = free_items_cell(x, y)
                        local scrap = game_data.random_scrap_id()
                        if scrap ~= 0 then
                            G_view_menu.items.layer:paste_block(bx, by, scrap)
                        end
                    end

                    if math.random(1, 100) <= 1 then
                        G_view_menu.floor.layer:paste_block(x + dx, y + dy, 0)
                    end

                    if math.random(1, 100) <= 20 then
                        vars:set_u8("f", fuel - 1)
                    end

                    return
                end
            end
        end

        vars:set_u8("v", 0)
    end
)
