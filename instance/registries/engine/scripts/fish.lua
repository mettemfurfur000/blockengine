local wrappers = require("registries.engine.scripts.wrappers")

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