local current_block = scripting_current_block_id

local ticks = 0
local wait_time = 50

-- local sounds = {}

-- blockengine.register_handler(engine_events.ENGINE_TICK, function(code)
--     ticks = ticks + 1
--     if ticks % wait_time == 0 then
--         local sel = math.random(1, 4)
--         local sel_sound = sounds[sel]
--         sel_sound.sound:play()
--         wait_time = math.random(45, 65);
--     end
-- end)

-- blockengine.register_handler(engine_events.ENGINE_INIT, function(code)
--     local block_info = find_block(g_engine_table, "fish")
--     sounds = block_info.sounds
--     g_menu.objects.layer:paste_block(7, 7, current_block) -- x, y, id
-- end)

