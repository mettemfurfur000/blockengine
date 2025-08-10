require("registries.engine.scripts.constants")

screen_width, screen_height = render_rules.get_size(g_render_rules)

local camera_limit_x, camera_limit_y = screen_width, screen_height

local zoom_min = 1
local zoom_max = 4

local cur_zoom = global_zoom

function recalc_camera_limits()
    screen_width, screen_height = render_rules.get_size(g_render_rules)
    camera_limit_x, camera_limit_y = screen_width / cur_zoom, screen_height / cur_zoom
end

function camera_set_target(pos, do_center)
    do_center = do_center or false
    for k, v in pairs(g_menu) do
        if v.is_ui ~= true then
            local slice = v.slice

            local actual_block_width = cur_zoom * g_block_size

            if (do_center) then
                pos.x = pos.x + 0.5
                pos.y = pos.y + 0.5
            end

            slice.x = (pos.x) * actual_block_width - slice.w / 2
            slice.y = (pos.y) * actual_block_width - slice.h / 2

            slice.x = math.min(math.max(slice.x, 0), camera_limit_x)
            slice.y = math.min(math.max(slice.y, 0), camera_limit_y)

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
