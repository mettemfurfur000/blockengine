local game_data = require("registries.engine.scripts.game_data")

local M = {}

function M.pixels_to_blocks(pos, zoom)
    if G_camera ~= nil then
        local topleft = G_camera:get_position()
        zoom = G_camera:get_zoom()
        return {
            x = math.floor((pos.x + topleft.x) / zoom / G_block_size),
            y = math.floor((pos.y + topleft.y) / zoom / G_block_size)
        }
    end
    zoom = zoom or G_global_zoom
    return {
        x = math.floor(pos.x / G_block_size / zoom),
        y = math.floor(pos.y / G_block_size / zoom)
    }
end

function M.pixel_to_blocks_no_offset(pos, zoom)
    zoom = zoom or (G_camera ~= nil and G_camera:get_zoom()) or G_global_zoom
    return {
        x = math.floor(pos.x / G_block_size / zoom),
        y = math.floor(pos.y / G_block_size / zoom)
    }
end

function M.pixel_to_layer_blocks(layer_index, pos, zoom)
    for _, view in pairs(G_view_menu) do
        if view.index == layer_index and view.is_ui then
            zoom = zoom or (G_camera ~= nil and G_camera:get_zoom()) or G_global_zoom
            return {
                x = math.floor(pos.x / G_block_size / zoom),
                y = math.floor(pos.y / G_block_size / zoom)
            }
        end
    end

    return M.pixels_to_blocks(pos, zoom)
end

local ADJACENT_OFFSETS = {
    { x = 1,  y = 1 },
    { x = 0,  y = 1 },
    { x = -1, y = 1 },
    { x = 1,  y = -1 },
    { x = 0,  y = -1 },
    { x = -1, y = -1 },
    { x = 1,  y = 0 },
    { x = -1, y = 0 }
}

function M.adjacent_blocks_all(layer, x, y)
    local ret = {}
    for _, offset in ipairs(ADJACENT_OFFSETS) do
        local nx, ny = x + offset.x, y + offset.y
        local id = layer:get_id(nx, ny)

        if id ~= 0 then
            table.insert(ret, { id = id, x = nx, y = ny })
        end
    end

    return ret
end

function M.adjacent_block(layer, x, y, id)
    for _, offset in ipairs(ADJACENT_OFFSETS) do
        local nx, ny = x + offset.x, y + offset.y
        if layer:get_id(nx, ny) == id then
            return nx, ny
        end
    end
    return nil, nil
end

function M.adjacent_blocks(layer, x, y, id)
    local ret = {}
    for _, offset in ipairs(ADJACENT_OFFSETS) do
        local nx, ny = x + offset.x, y + offset.y
        if layer:get_id(nx, ny) == id then
            table.insert(ret, { id = id, x = nx, y = ny })
        end
    end

    return ret
end


function M.consume_fuel(layer, x, y)
    local fuel_id = game_data.id("fuel_cell")

    local block_id = layer:get_id(x, y)
    if block_id == 0 then return 0 end

    local fuel_layer = G_view_menu.items.layer
    local fuel_items = M.adjacent_blocks(fuel_layer, x, y, fuel_id)

    for _, pos in ipairs(fuel_items) do
        local dx, dy = pos.x - x, pos.y - y
        if fuel_layer:get_id(x + dx, y + dy) == fuel_id then
            fuel_layer:paste_block(x + dx, y + dy, 0)

            return true
        end
    end

    return false
end


return M
