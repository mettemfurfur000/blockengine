local vec = require("registries.engine.scripts.vector_additions")
local sdl = require("registries.engine.scripts.definitions.sdl")
local wrappers = require("registries.engine.scripts.wrappers")
local blockengine = require("registries.engine.scripts.definitions.blockengine")
local camera_utils = require("registries.engine.scripts.camera_utils")

local current_block = scripting_current_block_id

print("loading a player bot block id " .. current_block)

-- TODO: on level load, scan for existing bots to register
local bots_reg = {}
local self_bot_uuid = 0

local function bot_lookup(uuid)
    for index, value in ipairs(bots_reg) do
        if value.uuid == uuid then
            return value
        end
    end
    return nil
end

--- @param vars VarsHandle
local function bot_reigster(vars, x, y)
    ::register_again::
    local uuid_new = math.random(0, 1 << 16)
    local bot = bot_lookup(uuid_new)

    if bot ~= nil then
        goto register_again
    end

    vars:set_u32("@", uuid_new)

    local newbot = {
        uuid = uuid_new,
        items = {},
        var_handle = vars,
        pos = { x, y }
    }

    if self_bot_uuid == 0 then
        self_bot_uuid = uuid_new
    end

    table.insert(bots_reg, newbot)
end

-- takes an uninitialized vars of things and fills it with `0x01`-s
--- @param vars VarsHandle
local function ils_init(vars, key)
    local empty_str = string.rep(string.char(0x01), 15) -- 15 because the 16th is reserved for the terminator `0x00`
    vars:set_string(key, empty_str)
end

-- takes a string of `0x01`-s and turns it into a table of item ids, also returns the total number of items
local function ils_parse(item_str)
    local ret = {}
    local total = 0

    for i = 1, #item_str do
        local item_id = string.byte(item_str, i) - 1

        if item_id == 0 then
            break
        end

        table.insert(ret, item_id)
        total = total + 1
    end

    return ret, total
end

local keystate = {}

local types = {
    face_up = 0,
    face_right = 1,
    face_down = 2,
    face_left = 3,
}

local frames = {
    stand = 0,
    walk_0 = 1,
    walk_1 = 2,
    grab_attempt = 3,
    has_items_stand = 4,
    has_items_walk_0 = 5,
    has_items_walk_1 = 6,
    has_items_grab_attempt = 7,
}

local function input_delta()
    return {
        x = (keystate['d'] or 0) - (keystate['a'] or 0),
        y = (keystate['s'] or 0) - (keystate['w'] or 0)
    }
end

blockengine.register_handler(events.SDL_KEYDOWN, function(keysym, mod, state, rep)
    if rep ~= nil and rep <= 1 and state ~= 0 then
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
        if moved_on_tick == G_sdl_tick then
            return
        end

        local uuid = vars:get_u32("@")

        if uuid == 0 then -- register new bot
            bot_reigster(vars, x, y)
            -- also run any init code in here
            ils_init(vars, "I")
        end

        if uuid ~= self_bot_uuid then
            return
        end

        local pos = {
            x = x,
            y = y
        }

        local delta = input_delta()

        local items_str = vars:get_string("I")

        -- TODO: store items in a special storage layer!
        -- alongside with marker-blocks to store which bot it belongs to by uuid

        local items, items_total = ils_parse(items_str)
        local is_standing = delta.x == 0 and delta.y == 0
        local is_grabbing = keystate['e'] == 1

        local frame_base = items_total > 0 and frames.has_items_stand or frames.stand
        -- render state management
        if is_standing then
            if is_grabbing then                                    -- attempts to get an item
                vars:set_u8("v", frame_base + frames.grab_attempt) -- bonk
                local dir = vars:get_u8("t")
                delta = vec.delta(dir)
                local target_pos = vec.add(pos, delta)
                -- layer:paste_block(target_pos.x, target_pos.y, 0)
                if layer:get_id(target_pos.x, target_pos.y) ~= 0 then
                    -- TODO: implement item grabbing logic
                end

            else
                vars:set_u8("v", frame_base)
            end
            return
        end

        vars:set_u8("v", frame_base + frames.walk_0 + G_tick % 2)

        local dir = vec.direction(delta.x, delta.y)
        vars:set_u8("t", dir)

        local next_pos = vec.add(pos, delta)
        local id = layer:get_id(next_pos.x, next_pos.y)

        if id == 0 then -- only advance on empty blocks
            if layer:move_block(pos.x, pos.y, delta.x, delta.y) == false then
                print("failed to move bot to " .. next_pos.x .. ":" .. next_pos.y)
            end

            vars:set_i16("x", -delta.x * G_block_width_pixels)
            vars:set_i16("y", -delta.y * G_block_width_pixels)

            vars:set_u32("T", G_sdl_tick)

            camera_utils.set_target(vec.mult(next_pos, G_block_size))
        end
    end
)
