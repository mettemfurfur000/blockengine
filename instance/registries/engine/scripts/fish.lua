local wrappers = require("registries.engine.scripts.wrappers")
local vec = require("registries.engine.scripts.vector_additions")

local current_block = scripting_current_block_id

local ticks = 0
local wait_time = 50

-- for debugging purposes

local G_fish_sounds = {}

-- blockengine.register_handler(events.ENGINE_TICK, function(code)
--     ticks = ticks + 1
--     if ticks % wait_time == 0 then
--         local sel = math.random(1, 4)
--         local sel_sound = G_fish_sounds[sel]
--         sel_sound.sound:play()
--         wait_time = math.random(45, 65);
--     end
-- end)

blockengine.register_handler(events.ENGINE_INIT_GLOBALS, function()
    local block_info = wrappers.find_block(G_engine_table, "fish")
    G_fish_sounds = block_info.sounds
end)


scripting_light_block_input_register(scripting_current_light_registry, current_block, "entity_tick",
    ---@param layer Layer
    ---@param entity BlockEntity
    function(layer, entity, value)
        local vel = { x = entity.velocity_x, y = entity.velocity_y }

        -- entity.velocity_x = 0.95 * vel.x
        -- entity.velocity_y = 0.95 * vel.y

        if math.abs(vel.x) < 1 and math.abs(vel.y) < 1 then
            -- entity:remove()
            entity.rotation = entity.rotation + 45
            return
        end
    end
)

scripting_light_block_input_register(scripting_current_light_registry, current_block, "entity_collision",
    ---@param layer Layer
    ---@param entity BlockEntity
    function(layer, entity, hit_x, hit_y, hit_id, hit_pos_x, hit_pos_y)
        local vel = { x = entity.velocity_x, y = entity.velocity_y }
        local pos = { x = entity.position_x, y = entity.position_y }

        if hit_pos_x == 0 and hit_pos_y == 0 then
            return
        end

        local block_size = G_block_size
        local half_size = block_size * 0.5

        local new_x = pos.x
        local new_y = pos.y

        if math.abs(vel.x) > math.abs(vel.y) then
            if vel.x > 0 then
                new_x = hit_x * block_size - half_size - 0.01
            else
                new_x = hit_x * block_size + block_size + half_size + 0.01
            end
        else
            if vel.y > 0 then
                new_y = hit_y * block_size - half_size - 0.01
            else
                new_y = hit_y * block_size + block_size + half_size + 0.01
            end
        end

        entity.position_x = new_x
        entity.position_y = new_y
        entity.velocity_x = 0
        entity.velocity_y = 0
        ---@type Sound
        local sel_sound = G_fish_sounds[math.random(1, #G_fish_sounds)].sound
        sel_sound:play()
    end
)
