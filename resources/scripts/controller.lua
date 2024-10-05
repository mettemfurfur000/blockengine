require("resources.scripts.blockengine")

controller = {
    player = { pos = { x = 0, y = 0 }, block = nil },
    states = {},
    tick = 0,
    last_walk_frame = 0
}

local function util_get_block(pos)
    return blockengine.access_block(g_world, 1, pos.x, pos.y)
end

local function get_walking_type(delta)
    if delta.x > 0 then return 0 end
    if delta.x < 0 then return 2 end

    if delta.y < 0 then return 1 end
    if delta.y > 0 then return 3 end
    return nil
end

local function get_walking_frame(delta)
    if delta.x == 0 and delta.y == 0 then return 0 end

    local tick_fraction = math.floor(controller.tick / 100)

    return 1 + math.fmod(tick_fraction, 2)
end

wrap_register(EVENT_IDS.INIT, function(sym, mod, state, rep)
    local pos, p            = find_block(16, 16, scripting_current_block_id)
    controller.player.pos   = pos
    controller.player.block = p

    print("initialized and found a player at " .. pos.x .. "," .. pos.y)
end)

wrap_register(EVENT_IDS.TICK, function(sym, mod, state, rep)
    controller.tick = controller.tick + 1

    local delta = { x = 0, y = 0 }

    if controller.states[SDL_SCANCODE.SDL_SCANCODE_W] then delta.y = -1 end
    if controller.states[SDL_SCANCODE.SDL_SCANCODE_S] then delta.y = 1 end
    if controller.states[SDL_SCANCODE.SDL_SCANCODE_A] then delta.x = -1 end
    if controller.states[SDL_SCANCODE.SDL_SCANCODE_D] then delta.x = 1 end

    if delta.x == 0 and delta.y == 0 then return end

    local frame = get_walking_frame(delta) -- aka specific frame of the animation
    local type = get_walking_type(delta)   -- aka direction

    print(frame .. "," .. (type or "nil"))

    if controller.states[SDL_SCANCODE.SDL_SCANCODE_SPACE] then
        frame = 3
        controller.player.pos = move_block_r(controller.player.pos, delta)
    else
        controller.player.pos = move_block_g(controller.player.pos, delta)
    end

    controller.player.block = util_get_block(controller.player.pos)

    blockengine.blob_set_number(controller.player.block, "v", frame)

    if type ~= nil then
        blockengine.blob_set_number(controller.player.block, "t", type)
    end
end)


wrap_register(EVENT_IDS.SDL_KEYDOWN,
    function(sym, mod, state, rep) blockengine.get_keyboard_state(controller.states) end
)

wrap_register(EVENT_IDS.SDL_KEYUP,
    function(sym, mod, state, rep) blockengine.get_keyboard_state(controller.states) end
)
