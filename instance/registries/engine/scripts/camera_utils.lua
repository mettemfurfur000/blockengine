require("registries.engine.scripts.constants")

local sdl = require("registries.engine.scripts.definitions.sdl")

screen_width, screen_height = render_rules.get_size(g_render_rules)

local camera_limit_x, camera_limit_y = screen_width, screen_height

local zoom_min = 1
local zoom_max = 4

local cur_zoom = global_zoom

function recalc_camera_limits()
    screen_width, screen_height = render_rules.get_size(g_render_rules)
    camera_limit_x, camera_limit_y = screen_width / cur_zoom, screen_height / cur_zoom
end

camera_current_pos = {
    x = 0,
    y = 0
}

function camera_set_target(pos)
    camera_current_pos = pos
    -- do_center = do_center or false
    for k, v in pairs(g_menu) do
        if v.is_ui ~= true then
            local slice = v.slice

            local actual_block_width = cur_zoom * g_block_size

            -- print(camera_current_pos.x .. " : ".. camera_current_pos.y)
            -- print_table(slice)

            -- local pixels_y = (camera_current_pos.y * actual_block_width) - slice.h / 2
            -- local pixels_x = (camera_current_pos.x * actual_block_width) - slice.w / 2

            local pixels_y = camera_current_pos.y - slice.h / 2
            local pixels_x = camera_current_pos.x - slice.w / 2

            pixels_x = pixels_x - math.fmod(pixels_x, actual_block_width) -- minus error to snap to grid
            pixels_y = pixels_y - math.fmod(pixels_y, actual_block_width)

            slice.old_x = slice.x
            slice.old_y = slice.y

            slice.timestamp_old = sdl.get_ticks()

            slice.x = math.floor(math.min(math.max(pixels_x, 0), camera_limit_x)) 
            slice.y = math.floor(math.min(math.max(pixels_y, 0), camera_limit_y))

            -- update current mouse offset

            mouse.offset = {
                x = slice.x,
                y = slice.y
            }

            render_rules.set_slice(g_render_rules, v.index, slice)
        end
    end
end

function camera_set_zoom(change_zoom)
    cur_zoom = math.max(zoom_min, math.min(zoom_max, cur_zoom + change_zoom))

    for k, v in pairs(g_menu) do
        if v.is_ui == false then
            local slice = v.slice

            slice.zoom = cur_zoom

            render_rules.set_slice(g_render_rules, v.index, slice)
        end
    end

    recalc_camera_limits()
end
