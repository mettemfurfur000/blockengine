local M = {}

function M.pixels_to_blocks(pos, zoom)
    zoom = zoom or G_global_zoom
    if G_render_room_on and G_camera ~= nil then
        -- New camera renderer: the camera's slice top-left (returned by
        -- get_position) is in the same scaled space the renderer uses (screen =
        -- world*zoom - slice.x), so world = (screen + slice.x) / zoom. This makes
        -- picking match exactly what is on screen at any zoom or when clamped.
        local topleft = G_camera:get_position()
        zoom = G_camera:get_zoom()
        return {
            x = math.floor((pos.x + topleft.x) / zoom / G_block_size),
            y = math.floor((pos.y + topleft.y) / zoom / G_block_size)
        }
    end
    return {
        x = math.floor((pos.x + G_mouse.offset.x) / G_block_size / zoom),
        y = math.floor((pos.y + G_mouse.offset.y) / G_block_size / zoom)
    }
end

function M.pixel_to_blocks_no_offset(pos, zoom)
    zoom = zoom or render_rules.get_slice(g_render_rules, G_view_menu.mouse.index).zoom
    return {
        x = math.floor(pos.x / G_block_size / zoom),
        y = math.floor(pos.y / G_block_size / zoom)
    }
end

function M.pixel_to_layer_blocks(layer_index, pos, zoom)
    local slice = render_rules.get_slice(g_render_rules, layer_index)
    zoom = zoom or slice.zoom

    return {
        x = math.floor((pos.x + slice.x) / G_block_size / zoom),
        y = math.floor((pos.y + slice.y) / G_block_size / zoom)
    }
end

return M
