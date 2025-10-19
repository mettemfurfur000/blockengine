local current_block = scripting_current_block_id
local vec = require("registries.engine.scripts.vector_additions")
local sdl = require("registries.engine.scripts.definitions.sdl")
local wrappers = require("registries.engine.scripts.wrappers")

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
---@param layer Layer
    function(layer, x, y, value)
        local status, vars = layer:get_vars(x, y)
        if status == false then
            error("error getting vars for the jumper")
            return
        end

        wrappers.try(
            function()
                if value == 0 then
                    vars:set_u8("f", 1) -- ready to perform a move
                    return
                end

                local flags = vars:get_u8("f")

                if flags == 0 then -- if 0 that means that it already moved
                    return
                end

                local pos = {
                    x = x,
                    y = y
                }

                local dir = vars:get_u8("w")
                local delta = vec.delta(dir)
                local next_pos = vec.add(pos, delta)

                local id = layer:get_id(next_pos.x, next_pos.y)

                if id == 0 then -- only advance on empty blocks
                    local status = layer:move_block(pos.x, pos.y, delta.x, delta.y)
                    if status == false then
                        print("failed to advance even though it should be fine at " .. next_pos.x .. ":" .. next_pos.y)
                    end

                    vars:set_i16("x", -delta.x * g_block_width_pixels)
                    vars:set_i16("y", -delta.y * g_block_width_pixels)

                    vars:set_u32("t", G_sdl_tick)

                    vars:set_u8("f", 0) -- moved
                else                    -- in case of a nil or a non-zero block, just rotate
                    vars:set_u8("w", vec.rotate(dir, 1))
                end
            end,
            function(e)
                local index, validation = vars:get_raw()
                wrappers.log_error("vars index - " .. index .. ", validation - " .. validation)
                wrappers.log_error(e)
                os.exit()
            end
        )
    end)
