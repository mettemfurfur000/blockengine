local wrappers = require("registries.engine.scripts.wrappers")
local vec = require("registries.engine.scripts.vector_additions")

local current_block = scripting_current_block_id

local ticks = 0
local wait_time = 50

-- for debugging purposes

-- local sounds = {}

-- blockengine.register_handler(events.ENGINE_TICK, function(code)
--     ticks = ticks + 1
--     if ticks % wait_time == 0 then
--         local sel = math.random(1, 4)
--         local sel_sound = sounds[sel]
--         sel_sound.sound:play()
--         wait_time = math.random(45, 65);
--     end
-- end)

-- blockengine.register_handler(events.ENGINE_INIT_GLOBALS, function()
--     local block_info = wrappers.find_block(G_engine_table, "fish")
--     sounds = block_info.sounds
--     G_view_menu.objects.layer:paste_block(7, 7, current_block) -- x, y, id
-- end)


scripting_light_block_input_register(scripting_current_light_registry, current_block, "entity_tick",
    ---@param layer Layer
    ---@param entity BlockEntity
    function(layer, entity, value)
        local vel = { x = entity.velocity_x, y = entity.velocity_y }

        if math.abs(vel.x) < 1 and math.abs(vel.y) < 1 then
            entity:remove()
            return
        end

        entity.velocity_x = 0.5 * vel.x
        entity.velocity_y = 0.5 * vel.y
    end
)
