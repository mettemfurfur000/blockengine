local vec = require("registries.engine.scripts.vector_additions")
require("registries.engine.scripts.camera_utils")

local sdl = require("registries.engine.scripts.definitions.sdl")
local blockengine = require("registries.engine.scripts.definitions.blockengine")

local current_block = scripting_current_block_id

print("loading a controller block id " .. current_block)

local keystate = {}

local player_states = {
    dead = 0,
    idle = 1,
    walk = 2,
    attack = 3
}

local function input_delta()
    return {
        x = (keystate['d'] or 0) - (keystate['a'] or 0),
        y = (keystate['s'] or 0) - (keystate['w'] or 0)
    }
end

blockengine.register_handler(sdl_events.SDL_KEYDOWN, function(keysym, mod, state, rep)
    if rep ~= nil and rep <= 1 and state ~= 0 then
        -- keystate[string.char(keysym)] = state
        try(function()
            keystate[string.char(keysym)] = state
        end, function(e)
        end)
    end
end)

blockengine.register_handler(sdl_events.SDL_KEYUP, function(keysym, mod, state, rep)
    if rep ~= nil and rep <= 0 and state ~= 1 then
        -- keystate[string.char(keysym)] = state
        try(function()
            keystate[string.char(keysym)] = state
        end, function(e)
        end)
    end
end)

---@param layer Layer
scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick", function(layer, x, y, value)
        -- if g_tick % tick_skip ~= 0 then -- tick cap
        --     return
        -- end

        local status, vars = layer:get_vars(x, y)
        if status == false then
            error("error getting vars for the dev")
            return
        end

        if value == 0 then
            vars:set_u8("f", 1) -- ready to perform a move
            return
        end

        if vars:get_u8("f") == 0 then -- if 0 that means that it already moved
            return
        end

        local pos = {
            x = x,
            y = y
        }

        local delta = input_delta()
        if delta.x == 0 and delta.y == 0 then
            if keystate[' '] == 1 then
                vars:set_u8("v", 3) -- bonk
                local dir = vars:get_u8("t")
                delta = vec.delta(dir)
                local next_pos = vec.add(pos, delta)
                layer:paste_block(next_pos.x, next_pos.y, 0)
            else
                vars:set_u8("v", 0)
            end
            return
        end

        vars:set_u8("v", 1 + g_tick % 2)

        local dir = vec.direction(delta.x, delta.y)

        vars:set_u8("t", dir)
        local next_pos = vec.add(pos, delta)
        local id = layer:get_id(next_pos.x, next_pos.y)

        if id == 0 then -- only advance on empty blocks
            local status = layer:move_block(pos.x, pos.y, delta.x, delta.y)
            if status == false then
                print("failed to move dev to " .. next_pos.x .. ":" .. next_pos.y)
            end

            vars:set_i16("x", -delta.x * g_block_width_pixels)
            vars:set_i16("y", -delta.y * g_block_width_pixels)

            vars:set_u32("T", sdl.get_ticks())

            camera_set_target(vec.mult(next_pos, g_block_width_pixels))

            vars:set_u8("f", 0) -- moved
        end
    end)
