-- This script handles arrow blocks, setting their rotation towards the player
local current_block = scripting_current_block_id

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    function(layer, x, y, value)
        if not player or not player.state == 0 then
            return
        end
        local player_pos = player.pos

        local pos = {
            x = x,
            y = y
        }

        local delta_x = player_pos.x - pos.x
        local delta_y = player_pos.y - pos.y

        if delta_x ~= 0 or delta_y ~= 0 then
            local angle = math.atan2(delta_y, delta_x)
            local status, vars = layer:get_vars(x, y)

            if status == false then
                error("error getting vars for de button (really weird!!)")
                return
            end

            local old_deg = vars:get_i16("r") or 0
            local deg = (math.deg(angle) + old_deg) / 2
            
            vars:set_i16("r", deg)
        end
    end)
