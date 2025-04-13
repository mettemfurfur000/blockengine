local constants = require("registries.engine.scripts.constants")

local current_block_id = scripting_current_block_id

blockengine.register_handler(engine_events.ENGINE_INIT, function(code)
    g_menu.objects.layer:paste_block(1, 1, current_block_id) -- x, y, id

    register_input_direct(current_block_id, "click", function(block_id, input_value)
        
    end)
end)

