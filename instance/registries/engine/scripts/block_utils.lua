local M = {}

function M.pixels_to_blocks(pos, zoom)
    zoom = zoom or render_rules.get_slice(g_render_rules, G_view_menu.mouse.index).zoom
    return {
        x = math.floor((pos.x + G_mouse.offset.x) / g_block_size / zoom),
        y = math.floor((pos.y + G_mouse.offset.y) / g_block_size / zoom)
    }
end

function M.pixel_to_blocks_no_offset(pos, zoom)
    zoom = zoom or render_rules.get_slice(g_render_rules, G_view_menu.mouse.index).zoom
    return {
        x = math.floor(pos.x / g_block_size / zoom),
        y = math.floor(pos.y / g_block_size / zoom)
    }
end

function M.pixel_to_layer_blocks(layer_index, pos, zoom)
    local slice = render_rules.get_slice(g_render_rules, layer_index)
    zoom = zoom or slice.zoom

    return {
        x = math.floor((pos.x + slice.x) / g_block_size / zoom),
        y = math.floor((pos.y + slice.y) / g_block_size / zoom)
    }
end

return M
