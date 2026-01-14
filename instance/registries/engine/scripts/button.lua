local current_block = scripting_current_block_id

print("registering an input")

---@param layer Layer
scripting_light_block_input_register(scripting_current_light_registry, current_block, "click", function(layer, x, y, input_value)
        local status, vars = layer:get_vars(x, y)
        if status == false then
            error("error getting vars for de button (really weird!!)")
            return
        end

        local val = vars:get_u8("a")

        -- print("val: " .. val)
        -- print("button blip set to " ..val)

        vars:set_u8("a", 1 - val)
    end)

-- blockengine.register_handler(engine_events.ENGINE_INIT, function(code)
--     g_menu.objects.layer:paste_block(1, 1, current_block) -- x, y, id
--     log_message("placed a button")
-- end)

