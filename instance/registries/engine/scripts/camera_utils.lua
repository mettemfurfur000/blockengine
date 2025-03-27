
function camera_set_target(pos, index)
    local slice = render_rules.get_slice(g_render_rules, index)

    local actual_block_width = slice.zoom * g_block_size
    -- slice.x = (pos.x + 0.5) * actual_block_width - slice.w / 2
    -- slice.y = (pos.y + 0.5) * actual_block_width - slice.h / 2

    slice.x = (pos.x) * actual_block_width - slice.w / 2
    slice.y = (pos.y) * actual_block_width - slice.h / 2

    slice.x = math.max(slice.x, 0)
    slice.y = math.max(slice.y, 0)

    render_rules.set_slice(g_render_rules, index, slice)
end