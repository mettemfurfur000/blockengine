require("registries.engine.scripts.constants")

local current_block = scripting_current_block_id

---@param layer Layer
scripting_light_block_input_register(scripting_current_light_registry, current_block, "click", function(layer, x, y, input_value)
    local status, vars = layer:get_vars(x, y)
    if status == false then
        return
    end
    vars:set_i16("r", (vars:get_i16("r") or 0) + 45)
end)
