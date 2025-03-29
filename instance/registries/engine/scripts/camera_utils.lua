function camera_set_target(pos, index, center)
    local slice = render_rules.get_slice(g_render_rules, index)
    local width, height = render_rules.get_size(g_render_rules)

    local actual_block_width = slice.zoom * g_block_size
    if (center ~= nil) then
        slice.x = (pos.x + 0.5) * actual_block_width - slice.w / 2
        slice.y = (pos.y + 0.5) * actual_block_width - slice.h / 2
    else
        slice.x = (pos.x) * actual_block_width - slice.w / 2
        slice.y = (pos.y) * actual_block_width - slice.h / 2
    end

    slice.x = math.min(math.max(slice.x, 0), g_camera_limit_x)
    slice.y = math.min(math.max(slice.y, 0), g_camera_limit_y)

    render_rules.set_slice(g_render_rules, index, slice)
end
