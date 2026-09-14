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

return M
