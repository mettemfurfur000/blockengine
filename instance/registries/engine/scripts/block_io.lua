g_inputs = {}

-- callback format: function(layer, x, y, input_value)

function register_input_direct(layer, block_id, input_name, callback)
    -- g_inputs[layer:uuid()][block_id][input_name] = callback

    -- try(function()
    --     g_inputs[layer:uuid()][block_id][input_name] = callback
    -- end, function(e)
    --     g_inputs[layer:uuid()] = {}
    --     g_inputs[layer:uuid()][block_id] = {}
    --     g_inputs[layer:uuid()][block_id][input_name] = callback
    -- end)

    if g_inputs[layer:uuid()] == nil then
        g_inputs[layer:uuid()] = {}
    end

    if g_inputs[layer:uuid()][block_id] == nil then
        g_inputs[layer:uuid()][block_id] = {}
    end

    g_inputs[layer:uuid()][block_id][input_name] = callback

    print("registered input '" .. input_name .. "' for block id " .. block_id)
end

function invoke_input(layer, x, y, input_name, input_value)
    local success, block_id = layer:get_id(x, y)
    if success == false then return end

    -- local callback = g_inputs[layer:uuid()][block_id][input_name]

    if callback then
        callback(layer, x, y, input_value)
    else
        print("callback not found on " .. input_name " for block " .. block_id .. " at " .. x .. ", " .. y .. ", layer " .. layer:uuid())
    end
end
