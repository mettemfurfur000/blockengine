local old_x_key = "x"
local old_y_key = "y"
local timestamp_key = "T"

trait.add_var(old_x_key, 2)
trait.add_var(old_y_key, 2)
trait.add_var(timestamp_key, 4)

trait.set_controller("offset_x", old_x_key)
trait.set_controller("offset_y", old_y_key)
trait.set_controller("interp_timestamp", timestamp_key)
trait.set_interp_takes(100)

local api = {}
api.interp_takes = 100

function api.moved_this_tick(vars)
    return (vars:get_u32(timestamp_key) or 0) == (G_sdl_tick or 0)
end

function api.last_move_tick(vars)
    return vars:get_u32(timestamp_key) or 0
end

function api.reset(vars)
    vars:set_u32(timestamp_key, 0)
    vars:set_i16(old_x_key, 0)
    vars:set_i16(old_y_key, 0)
end

function api.copy_movement_vars(vars, other_vars)
    vars:set_u32(timestamp_key, other_vars:get_u32(timestamp_key) or 0)
    vars:set_i16(old_x_key, other_vars:get_i16(old_x_key) or 0)
    vars:set_i16(old_y_key, other_vars:get_i16(old_y_key) or 0)
end

function api.copy_movement(dest_layer, dest_x, dest_y, source_vars)
    local id = dest_layer:get_id(dest_x, dest_y)
    if not id then
        return
    end

    if not trait.get_block_api(id).movement then
        error("block has no movement api, cannot copy movement vars : " .. dest_x .. ", " .. dest_y)
        -- return
    end

    local ivars = dest_layer:get_vars(dest_x, dest_y)

    if not ivars then
        -- return
        error("block has no vars, cannot copy movement vars : " .. dest_x .. ", " .. dest_y)
    end

    api.copy_movement_vars(ivars, source_vars)
end

function api.begin(vars, dx, dy)
    vars:set_i16(old_x_key, -dx * G_block_width_pixels)
    vars:set_i16(old_y_key, -dy * G_block_width_pixels)
    vars:set_u32(timestamp_key, G_sdl_tick)
end

trait.register_api("movement", api)
