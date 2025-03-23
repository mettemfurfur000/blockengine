local constants = require("registries.engine.scripts.constants")

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
    last_delta = {
        x = 0,
        y = 0
    },
    state = player_states.dead
}

local function util_block_move(layer, pos, delta) -- TODO: implement move functions from C space
    local dest_pos = { x = pos.x + delta.x, y =pos.y + delta.y }
    local status, dest_id = layer:get_id(dest_pos.x, dest_pos.y)
    local status2, src_id = layer:get_id(pos.x, pos.y)

    if status ~= 0 or status2 ~= 0 then
        return false
    end

    if dest_id == 0 then
        layer:set_id(dest_pos.x, dest_pos.y, src_id)
        layer:set_id(pos.x, pos.y, 0)
    end
end

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

    render_rules.set_slice(g_render_rules, 0, slice)
end

local function update_player()
    player.tick = player.tick + 1

    local delta = {
        x = 0,
        y = 0
    }

    if player.keystate[SDL_SCANCODE.SDL_SCANCODE_W] then
        delta.y = -1
    end
    if player.keystate[SDL_SCANCODE.SDL_SCANCODE_S] then
        delta.y = 1
    end
    if player.keystate[SDL_SCANCODE.SDL_SCANCODE_A] then
        delta.x = -1
    end
    if player.keystate[SDL_SCANCODE.SDL_SCANCODE_D] then
        delta.x = 1
    end

    if delta.x == 0 and delta.y == 0 then -- standing actions
        player.state = player_states.idle

        if player.keystates[SDL_SCANCODE.SDL_SCANCODE_SPACE] then -- on spacebar press we attack
            frame = 3
            player.state = player_states.attack
            player.player.pos = move_block_r(player.player.pos, delta)
        else
            player.player.pos = move_block_g(player.player.pos, delta)
        end

        return
    end

    player.state = player_states.walk

    local frame = walk_frame(delta) -- aka specific frame of the animation
    local type = walk_type(delta) -- aka direction

    camera_set_target(player.player.pos)

    util_block_move(player.home_layer, player.player.pos, delta)

    -- blockengine.blob_set_number(player.player.block, "v", frame)
    -- blockengine.blob_set_number(player.player.block, "r", math.floor(math.fmod(player.tick, 360)))

    if type ~= nil then
        -- blockengine.blob_set_number(player.player.block, "t", type)
    end
end

-- hook zone

blockengine.register_handler(EVENT_IDS.ENGINE_BLOCK_CREATE, function(room, layer, new_id, old_id, x, y)
    if new_id ~= scripting_current_block_id then
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

    camera_set_target(pos)

    print("initialized and found a player at " .. pos.x .. ", " .. pos.y .. ", room named " .. room:get_name())
end)

blockengine.register_handler(EVENT_IDS.SDL_KEYDOWN, function(keysym, mod, state, rep)
    if rep > 0 then
        return
    end
    keystate[keysym] = state
end)

blockengine.register_handler(EVENT_IDS.SDL_KEYUP, function(keysym, mod, state, rep)
    if rep > 0 then
        return
    end
    keystate[keysym] = state
end)
