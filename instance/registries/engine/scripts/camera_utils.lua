local sdl = require("registries.engine.scripts.definitions.sdl")

if g_render_rules == nil then
    print("camera_utils.lua requires render_rules to be defined, camera utils will not work")
    return
end

G_screen_width, G_screen_height = render_rules.get_size(g_render_rules)

local camera_limit_x, camera_limit_y = G_screen_width, G_screen_height

local zoom_min = 1
local zoom_max = 4

local M = {}

function M.recalc_camera_limits()
    G_screen_width, G_screen_height = render_rules.get_size(g_render_rules)
    camera_limit_x, camera_limit_y = G_screen_width / G_global_zoom, G_screen_height / G_global_zoom
end

G_camera_current_pos = {
    x = 0,
    y = 0
}

function M.set_target(pos)
    -- pos is the followed block's world-pixel top-left (block coords * G_block_size).
    G_camera_current_pos = pos

    local cx = G_camera_current_pos.x + G_block_size * 0.5
    local cy = G_camera_current_pos.y + G_block_size * 0.5

    for k, v in pairs(G_view_menu) do
        if v.is_ui ~= true then
            local slice = v.slice

            assert(slice)

            local actual_block_width = G_global_zoom * G_block_size

            -- Legacy render_rules slice: operates in screen pixels.
            local screen_x = cx * G_global_zoom
            local screen_y = cy * G_global_zoom

            local pixels_x = screen_x - slice.w / 2
            local pixels_y = screen_y - slice.h / 2

            pixels_x = pixels_x - math.fmod(pixels_x, actual_block_width) -- minus error to snap to grid
            pixels_y = pixels_y - math.fmod(pixels_y, actual_block_width)

            slice.old_x = slice.x
            slice.old_y = slice.y

            slice.timestamp_old = G_sdl_tick or sdl.get_ticks()

            slice.x = math.floor(math.min(math.max(pixels_x, 0), camera_limit_x))
            slice.y = math.floor(math.min(math.max(pixels_y, 0), camera_limit_y))

            -- update current mouse offset (screen pixels, for the legacy path)

            G_mouse.offset = {
                x = slice.x,
                y = slice.y
            }

            render_rules.set_slice(g_render_rules, v.index, slice)
        end
    end

    -- Drive the new room/camera renderer if it is active. The dev block's script
    -- calls set_target() as it moves, which re-centers this camera so it follows
    -- the dev block. The camera clamps itself to the room bounds internally, and the
    -- editor reads the live camera position via camera:get_position(), so no stale
    -- G_mouse.offset needs to be maintained here.
    if G_camera ~= nil and G_render_room_on then
        G_global_zoom = G_camera:get_zoom()
        G_camera:center_on(cx, cy)
    end
end

function M.camera_set_zoom(change_zoom)
    G_global_zoom = math.max(zoom_min, math.min(zoom_max, G_global_zoom + change_zoom))

    for k, v in pairs(G_view_menu) do
        if v.is_ui == false then
            local slice = v.slice

            slice.zoom = G_global_zoom

            render_rules.set_slice(g_render_rules, v.index, slice)
        end
    end

    M.recalc_camera_limits()
end

return M
