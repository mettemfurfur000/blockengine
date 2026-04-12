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
        local pos = { x = entity.position_x, y = entity.position_y }
        local target = { x = G_screen_width / 2, y = G_screen_height / 2 }
        local diff = vec.sub(target, pos)

        local add_vel = vec.mult(diff, 0.5)

        entity.velocity_x = add_vel.x + 0.99 * entity.velocity_x + math.random() * 80
        entity.velocity_y = add_vel.y + 0.99 * entity.velocity_y + math.random() * 80
    end
)
