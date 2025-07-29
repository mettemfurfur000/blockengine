local current_block = scripting_current_block_id

local instances = {}

local function vec2d_add(a, b)
    return {
        x = a.x + b.x,
        y = a.y + b.y
    }
end

-- dir is a numba from 0 to 3
-- 0 means north and goes clockwis til 3, wher 3 means west 
local function delta_direction(pos, dir)
    return {
        x = dir == 1 and 1 or (dir == 3 and -1 or 0),
        y = dir == 2 and 1 or (dir == 0 and -1 or 0)
    }
end

-- local function interpolate(pos, )
--     -- TODO
-- end

local function advance_direction(pos, dir)
    return vec2d_add(v.pos, delta_direction(pos, dir))
end

local function rotate(dir, times)
    return (dir + times) % 4
end

local function spawn_instance(layer, x, y)
    local status, vars = layer:get_vars(x, y)

    if status == false then
        log_error("error getting vars for an instance at " .. x .. ", " .. y)
        return
    end

    table.insert(instances, {
        pos = {
            x = x,
            y = y
        },
        old_pos = {
            x = x,
            y = y
        },
        vars = vars,
        wish_direction = math.random(0, 3)
    })
end

blockengine.register_handler(engine_events.ENGINE_INIT, function(code) -- collect all existing jumpers
    g_menu.objects.layer:for_each(current_block, function(x, y)
        spawn_instance(g_menu.objects.layer, x, y)
    end)
end)

blockengine.register_handler(engine_events.ENGINE_BLOCK_CREATE, function(room, layer, new_id, old_id, x, y)
    if layer:uuid() ~= g_menu.objects.layer:uuid() then -- objects only
        return
    end

    -- local w, h, id_range, var_range = layer:lua_layer_get_size()
    -- if var_range == 0 then -- layer have to support variables, maybe...
    --     return
    -- end

    if old_id == current_block then -- current block was erased, clear the instance
        for k, v in pairs(instances) do
            if v.pos.x == x and v.pos.y == y then
                instances[k] = nil
                return
            end
        end
        return
    end

    if new_id == current_block then
        spawn_instance(layer, x, y)
    end
end)

local tick = 0

blockengine.register_handler(engine_events.ENGINE_TICK, function(code) -- tick over all existing jumpers
    tick = tick + 1

    if tick % 5 ~= 0 then
        return
    end

    for k, v in pairs(instances) do
        local delta = delta_direction(v.pos, v.wish_direction)
        local next_pos = vec2d_add(v.pos, delta)
        local id = g_menu.objects.layer:get_id(next_pos.x, next_pos.y)

        if id == 0 then -- only advance on empty blocks
            local status = g_menu.objects.layer:move_block(v.pos.x, v.pos.y, delta.x, delta.y)
            if status == true then
                v.old_pos = v.pos
                v.pos = next_pos
            else
                print("failed to advance even though it should be fine at " .. next_pos.x .. ":" .. next_pos.y)
            end
        else -- in case of a nil or a non-zero block, just rotate
            v.wish_direction = rotate(v.wish_direction, 1)
        end
    end
end)

