local zoom_min = 1
local zoom_max = 4

local M = {}

function M.recalc_camera_limits()
    if G_camera ~= nil then
        G_global_zoom = G_camera:get_zoom()
    end
end

G_camera_current_pos = {
    x = 0,
    y = 0
}

function M.set_target(pos)
    -- pos is the followed block's world-pixel top-left (block coords * G_block_size).
    G_camera_current_pos = pos

    if G_camera ~= nil then
        local cx = pos.x + G_block_size * 0.5
        local cy = pos.y + G_block_size * 0.5

        G_global_zoom = G_camera:get_zoom()
        G_camera:center_on(cx, cy)
    end
end

function M.camera_set_zoom(change_zoom)
    if G_camera == nil then
        return
    end

    local zoom = math.max(zoom_min, math.min(zoom_max, G_camera:get_zoom() + change_zoom))
    G_camera:set_zoom(zoom)
    G_global_zoom = G_camera:get_zoom()
    G_block_width_pixels = G_block_size * G_global_zoom
end

return M
