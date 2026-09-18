local wrappers = require("registries.engine.scripts.wrappers")
local blockengine = require("registries.engine.scripts.definitions.blockengine")

local keystate = {}
local presses = {}

local api = {}

function api.delta()
    return {
        x = (keystate['d'] or 0) - (keystate['a'] or 0),
        y = (keystate['s'] or 0) - (keystate['w'] or 0)
    }
end

function api.down(ch)
    return keystate[ch] == 1
end

function api.consume_press(ch)
    if presses[ch] then
        presses[ch] = nil
        return true
    end
    return false
end

blockengine.register_handler(events.SDL_KEYDOWN, function(keysym, mod, state, rep)
    if rep ~= nil and rep <= 1 and state ~= 0 then
        wrappers.try(function()
            local ch = string.char(keysym)
            local prev = keystate[ch] or 0
            keystate[ch] = state
            if state == 1 and prev ~= 1 then
                presses[ch] = true
            end
        end, function(e)
        end)
    end
end)

blockengine.register_handler(events.SDL_KEYUP, function(keysym, mod, state, rep)
    if rep ~= nil and rep <= 0 and state ~= 1 then
        wrappers.try(function()
            keystate[string.char(keysym)] = state
        end, function(e)
        end)
    end
end)

trait.register_api("wasd", api)