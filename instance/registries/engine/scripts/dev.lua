local vec = require("registries.engine.scripts.vector_additions")
local camera_utils = require("registries.engine.scripts.camera_utils")

local current_block = scripting_current_block_id
local movement = scripting_current_block_api.movement
local wasd = scripting_current_block_api.wasd

print("loading a controller block id " .. current_block)

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    ---@param layer Layer
    function(layer, x, y, value)
        local vars = layer:get_vars(x, y)
        if not vars then
            error("error getting vars for the dev, nuking it")
            layer:set_id(x, y, 0)
            return
        end

        if movement.moved_this_tick(vars) then
            return
        end

        local pos = {
            x = x,
            y = y
        }

        local delta = wasd.delta()
        if delta.x == 0 and delta.y == 0 then
            if wasd.down(' ') then
                vars:set_u8("v", 3) -- bonk
                local dir = vars:get_u8("t")
                delta = vec.delta(dir)
                local next_pos = vec.add(pos, delta)
                layer:paste_block(next_pos.x, next_pos.y, 0)
            else
                vars:set_u8("v", 0)
            end
            return
        end

        vars:set_u8("v", 1 + G_tick % 2)

        local dir = vec.direction(delta.x, delta.y)

        vars:set_u8("t", dir)
        local next_pos = vec.add(pos, delta)
        local id = layer:get_id(next_pos.x, next_pos.y)

        if id == 0 then -- only advance on empty blocks
            local status = layer:move_block(pos.x, pos.y, delta.x, delta.y)
            if status == false then
                print("failed to move dev to " .. next_pos.x .. ":" .. next_pos.y)
            end

            movement.begin(vars, delta.x, delta.y)

            camera_utils.set_target(vec.mult(next_pos, G_block_size))
        end
    end
)