require("resources.scripts.blockengine")

wrap_register(EVENT_IDS.INIT, function(sym, mod, state, rep)
    print("yeller initialized")
end)

wrap_register(EVENT_IDS.ENGINE_BLOCK_ERASED,
    function(target, x, y, layer, prev_id, new_id)
        local reg_fields = blockengine.read_registry_entry(g_reg, new_id)

        local block_name = ""

        for k, v in pairs(reg_fields) do
            if k == "source_filename" then
                block_name = v
            end
        end

        print("frickin \"" .. block_name .. "\" overwrote me!")
    end
)
