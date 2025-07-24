-- This script handles arrow blocks, setting their rotation towards the player

local arrow_block_id = scripting_current_block_id

print("loading an arrow block id " .. arrow_block_id)

active_arrows = {}

blockengine.register_handler(engine_events.ENGINE_BLOCK_CREATE, function(room, layer, new_id, old_id, x, y)
    -- records newly created arrow into active_arrows
    if new_id ~= arrow_block_id or layer:uuid() ~= g_menu.objects.layer:uuid() then
        -- print("ignoring block create, not an arrow or wrong layer", new_id, layer:uuid(), g_menu.objects.layer:uuid())
        return
    end

    local pos = {
        x = x,
        y = y
    }

    local status, vars = layer:get_vars(pos.x, pos.y)
    if status == false then
        log_error("error getting vars for arrow at " .. pos.x .. ", " .. pos.y)
        return
    end

    table.insert(active_arrows, {
        -- id = new_id,
        pos = pos,
        vars = vars
    })

    print("registered arrow at " .. pos.x .. ", " .. pos.y)
end)

blockengine.register_handler(engine_events.ENGINE_TICK, function(code)  -- iterate over all arrows and set their rotation towards the player
    if not player or not player.state == 0 then
        -- print("player does not exist, skipping arrow rotation")
        return
    end

    local player_pos = player.pos
    for i, arrow in pairs(active_arrows) do
        local delta_x = player_pos.x - arrow.pos.x
        local delta_y = player_pos.y - arrow.pos.y

        if delta_x ~= 0 or delta_y ~= 0 then
            local angle = math.atan2(delta_y, delta_x)
            arrow.vars:set_i16("r", math.deg(angle))
            -- print("calculated angle for arrow at " .. arrow.pos.x .. ", " .. arrow.pos.y .. ": " .. math.deg(angle))
        end
    end

end)