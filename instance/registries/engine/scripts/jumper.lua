local current_block = scripting_current_block_id

local function vec2d_add(a, b)
    return {
        x = a.x + b.x,
        y = a.y + b.y
    }
end

-- dir is a numba from 0 to 3
-- 0 means north and goes clockwis til 3, wher 3 means west 
local function delta_direction(pos, dir)
    return {
        x = dir == 1 and 1 or (dir == 3 and -1 or 0),
        y = dir == 2 and 1 or (dir == 0 and -1 or 0)
    }
end

local function advance_direction(pos, dir)
    return vec2d_add(v.pos, delta_direction(pos, dir))
end

local function rotate(dir, times)
    return (dir + times) % 4
end

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    function(layer, x, y, value)
        local status, vars = layer:get_vars(x, y)
        if status == false then
            log_error("error getting vars for de button (really weird!!)")
            return
        end

        local pos = {
            x = x,
            y = y
        }

        if value == 0 then
            vars:set_u8("f", 1) -- ready to perform a move
            return
        end

        local flags = vars:get_u8("f")

        if flags == 0 then -- if 0 that means that it already moved
            return
        end

        local dir = vars:get_u8("w")
        local delta = delta_direction(pos, dir)
        local next_pos = vec2d_add(pos, delta)

        local id = layer:get_id(next_pos.x, next_pos.y)

        if id == 0 then -- only advance on empty blocks
            local status = layer:move_block(pos.x, pos.y, delta.x, delta.y)
            if status == false then
                print("failed to advance even though it should be fine at " .. next_pos.x .. ":" .. next_pos.y)
            end
            vars:set_u8("f", 0)
        else -- in case of a nil or a non-zero block, just rotate
            vars:set_u8("w", rotate(dir, 1))
        end
    end)
