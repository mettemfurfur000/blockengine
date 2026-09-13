local game_data = require("registries.engine.scripts.game_data")

local current_block = scripting_current_block_id

local MAX_FUEL = 10
local SEARCH_RADIUS = 6

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

        if fuel <= 0 then
            vars:set_u8("v", 0)
            vars:set_u8("t", 0)
            return
        end

        local found = false
        local find_x, find_y = nil, nil
        for r = 0, SEARCH_RADIUS do
            if found then break end
            for dy = -r, r do
                for dx = -r, r do
                    if (dx ~= 0 or dy ~= 0) and math.abs(dx) <= r and math.abs(dy) <= r then
                        local id = G_view_menu.items.layer:get_id(x + dx, y + dy)
                        if game_data.is_scrap(id) then
                            found = true
                            find_x, find_y = x + dx, y + dy
                            break
                        end
                    end
                end
                if found then break end
            end
        end

        if found then
            local px, py = free_items_cell(x, y)
            if px then
                local item_id = G_view_menu.items.layer:get_id(find_x, find_y)
                G_view_menu.items.layer:paste_block(find_x, find_y, 0)
                G_view_menu.items.layer:paste_block(px, py, item_id)
                vars:set_u8("f", fuel - 1)
                vars:set_u8("v", 1 + G_tick % 2)
                vars:set_u8("t", math.random(0, 3))
            end
            return
        end

        vars:set_u8("v", 0)
        vars:set_u8("t", 0)
    end
)