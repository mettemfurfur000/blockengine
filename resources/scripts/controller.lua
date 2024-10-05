require("resources.scripts.blockengine")

player = { pos = { x = 0, y = 0 }, block = nil }
g_states = {}

wrap_register(EVENT_IDS.INIT, function(sym, mod, state, rep)
    local pos    = find_block(16, 16, 5)
    player.pos   = pos
    player.block = p

    print("initialized and found a player at " .. pos.x .. "," .. pos.y)
end)

wrap_register(EVENT_IDS.TICK, function(sym, mod, state, rep)
    delta = { x = 0, y = 0 }

    if g_states[SDL_SCANCODE.SDL_SCANCODE_W] then delta.y = -1 end
    if g_states[SDL_SCANCODE.SDL_SCANCODE_S] then delta.y = 1 end
    if g_states[SDL_SCANCODE.SDL_SCANCODE_A] then delta.x = -1 end
    if g_states[SDL_SCANCODE.SDL_SCANCODE_D] then delta.x = 1 end

    if g_states[SDL_SCANCODE.SDL_SCANCODE_SPACE] then
        move_block_r(player.pos, delta)
    else
        move_block_g(player.pos, delta)
    end
end)


wrap_register(EVENT_IDS.SDL_KEYDOWN, function(sym, mod, state, rep) blockengine.get_keyboard_state(g_states) end)

wrap_register(EVENT_IDS.SDL_KEYUP, function(sym, mod, state, rep) blockengine.get_keyboard_state(g_states) end)
