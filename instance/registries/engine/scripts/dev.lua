local vec = require("registries.engine.scripts.vector_additions")
local sdl = require("registries.engine.scripts.definitions.sdl")
local wrappers = require("registries.engine.scripts.wrappers")
local blockengine = require("registries.engine.scripts.definitions.blockengine")
local camera_utils = require("registries.engine.scripts.camera_utils")

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

blockengine.register_handler(events.SDL_KEYDOWN, function(keysym, mod, state, rep)
    if rep ~= nil and rep <= 1 and state ~= 0 then
        -- keystate[string.char(keysym)] = state
        wrappers.try(function()
            keystate[string.char(keysym)] = state
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

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    ---@param layer Layer
    function(layer, x, y, value)
        local vars = layer:get_vars(x, y)
        if not vars then
            error("error getting vars for the dev, nuking it")
            layer:set_id(x, y, 0)
            return
        end

        local moved_on_tick = vars:get_u32("T")
        -- print("move tick is " .. moved_on_tick .. ", current is " .. G_sdl_tick)
        if moved_on_tick == G_sdl_tick then -- already moved at this tick
            -- print("moved already!")
            return
        end

        local pos = {
            x = x,
            y = y
        }

        local delta = input_delta()
        if delta.x == 0 and delta.y == 0 then
            local dir = vars:get_u8("t")
            local move_delta = vec.delta(dir)

            if keystate['f'] == 1 then
                local fish_id = wrappers.find_block(G_engine_table, "fish").id

                -- spawn the real maximum amount of fish
                for i = 1, 65535, 1 do
                    local ent = layer:new_entity(fish_id,
                        math.random(0, G_screen_width),
                        math.random(0, G_screen_height - 1
                        )
                    )

                    if not ent then
                        print("no fish found")
                    else
                        local fish_launch_velocity = vec.mult(move_delta, 160);

                        ent.velocity_x = fish_launch_velocity.x + math.random() * 20
                        ent.velocity_y = fish_launch_velocity.y + math.random() * 20
                    end
                end
            end
            if keystate[' '] == 1 then
                vars:set_u8("v", 3) -- bonk
                local next_pos = vec.add(pos, move_delta)
                layer:paste_block(next_pos.x, next_pos.y, 0)
            else
                vars:set_u8("v", 0)
            end
            return
        end

        vars:set_u8("v", 1 + G_tick % 2)

        local dir = vec.direction(delta.x, delta.y)

        vars:set_u8("t", dir)
        local next_pos = vec.add(pos, delta)
        local id = layer:get_id(next_pos.x, next_pos.y)

        if id == 0 then -- only advance on empty blocks
            local status = layer:move_block(pos.x, pos.y, delta.x, delta.y)
            if status == false then
                print("failed to move dev to " .. next_pos.x .. ":" .. next_pos.y)
            end

            vars:set_i16("x", -delta.x * G_block_width_pixels)
            vars:set_i16("y", -delta.y * G_block_width_pixels)

            vars:set_u32("T", G_sdl_tick)

            camera_utils.set_target(vec.mult(next_pos, G_block_width_pixels))
        end
    end
)
