local constants = require("registries.engine.scripts.constants")

local player_block_id = scripting_current_block_id

print("loading a controller")

local player_states = {
    dead = 0,
    idle = 1,
    walk = 2,
    attack = 3
}

local player = {
    pos = {
        x = 0,
        y = 0
    },
    tick = 0,
    keystate = {},
    home_layer = nil,
    home_room = nil,
    vars = nil,
    last_delta = {
        x = 0,
        y = 0
    },
    state = player_states.dead
}

local function lerp(a, b)
    return a + (b - a) * 0.05
end

local function walk_type(delta)
    if delta.x > 0 then
        return 0
    end
    if delta.x < 0 then
        return 2
    end

    if delta.y < 0 then
        return 1
    end
    if delta.y > 0 then
        return 3
    end
    return nil
end

local function walk_frame(delta)
    if delta.x == 0 and delta.y == 0 then
        return 0
    end

    return 1 + math.fmod(player.tick, 2)
end

local function camera_set_target(pos)
    local slice = render_rules.get_slice(g_render_rules, 0)

    local actual_block_width = slice.zoom * g_block_size
    slice.x = (pos.x + 0.5) * actual_block_width - slice.w / 2
    slice.y = (pos.y + 0.5) * actual_block_width - slice.h / 2

    slice.x = math.max(slice.x, 0)
    slice.y = math.max(slice.y, 0)

    render_rules.set_slice(g_render_rules, 0, slice)
end

local function camera_move_debug()
    local slice = render_rules.get_slice(g_render_rules, 0)

    slice.x = slice.x + 1

    render_rules.set_slice(g_render_rules, 0, slice)
end

local function update_player()
    player.tick = player.tick + 1

    local delta = {
        x = 0,
        y = 0
    }

    delta.x = (player.keystate['d'] or 0) - (player.keystate['a'] or 0)
    delta.y = (player.keystate['s'] or 0) - (player.keystate['w'] or 0)

    local frame = walk_frame(delta) -- aka specific frame of the animation
    local type = walk_type(delta) -- aka direction

    if delta.x == 0 and delta.y == 0 then -- standing actions
        player.state = player_states.idle
        frame = 0

        if player.keystate[' '] == 1 then -- on spacebar press we attack
            frame = 3
            player.state = player_states.attack
        end
    else
        player.state = player_states.walk

        local status = player.home_layer:move_block(player.pos.x, player.pos.y, delta.x, delta.y)
        if status == true then
            player.pos.x = player.pos.x + delta.x
            player.pos.y = player.pos.y + delta.y
            -- print("moving, new pos", player.pos.x, player.pos.y)
        end

        camera_set_target(player.pos)
    end

    player.vars:set_number("v", frame, 1, 0)

    if type ~= nil then
        player.vars:set_number("t", type, 1, 0)
    end
end

-- hook zone

blockengine.register_handler(EVENT_IDS.ENGINE_BLOCK_CREATE, function(room, layer, new_id, old_id, x, y)
    if new_id ~= player_block_id or layer ~= g_floor  then
        print("ignored a spawn event for " .. x .. "," .. y .. "block " .. new_id)
        return
    end

    pos = {
        x = x,
        y = y
    }

    player.pos = pos
    player.state = player_states.idle
    player.home_layer = layer
    player.home_room = room

    local status, vars = player.home_layer:get_vars(player.pos.x, player.pos.y)
    if status == false then
        log_error("error getting vars")
        return
    end

    player.vars = vars

    print("initialized and found a player at " .. pos.x .. ", " .. pos.y .. ", room named " .. room:get_name())

    print("vars", vars:__tostring())
end)

blockengine.register_handler(EVENT_IDS.SDL_KEYDOWN, function(keysym, mod, state, rep)
    if rep == nil or rep > 0 then
        return
    end

    if state == 0 then
        return
    end

    local char = string.char(keysym)

    player.keystate[char] = state
end)

blockengine.register_handler(EVENT_IDS.SDL_KEYUP, function(keysym, mod, state, rep)
    if rep == nil or rep > 0 then
        return
    end

    if state == 1 then
        return
    end

    local char = string.char(keysym)

    player.keystate[char] = state
end)

blockengine.register_handler(EVENT_IDS.ENGINE_TICK, function(code)
    update_player()
end)
