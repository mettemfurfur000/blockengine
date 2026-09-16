local vec = require("registries.engine.scripts.vector_additions")
local wrappers = require("registries.engine.scripts.wrappers")
local blockengine = require("registries.engine.scripts.definitions.blockengine")
local camera_utils = require("registries.engine.scripts.camera_utils")
local game_data = require("registries.engine.scripts.game_data")

local current_block = scripting_current_block_id
local movement = scripting_current_block_api.movement
local wasd = scripting_current_block_api.wasd
local move_interval_ms = movement.interp_takes

local bots_reg = {}
local self_bot_uuid = 0

local function update_camera_interp()
    if G_camera ~= nil then
        G_camera:set_interp_takes(move_interval_ms)
    end
end

local function bot_lookup(uuid)
    for _, v in ipairs(bots_reg) do
        if v.uuid == uuid then
            return v
        end
    end
    return nil
end

local function bot_register(vars, x, y)
    ::register_again::
    local uuid_new = math.random(1, (1 << 16) - 1)
    if bot_lookup(uuid_new) then
        goto register_again
    end
    vars:set_u32("@", uuid_new)
    local newbot = { uuid = uuid_new, items = {}, var_handle = vars, pos = { x, y } }
    if self_bot_uuid == 0 then
        self_bot_uuid = uuid_new
    end
    table.insert(bots_reg, newbot)
end

local function claim_existing_player()
    self_bot_uuid = 0
    bots_reg = {}
    G_bot_pos = nil
    G_self_bot = nil
    G_game_over = false

    if G_view_menu == nil or G_view_menu.objects == nil then
        return
    end

    G_view_menu.objects.layer:for_each(current_block, function(x, y)
        local vars = G_view_menu.objects.layer:get_vars(x, y)
        if not vars then return end

        local uuid = vars:get_u32("@")
        if uuid == nil or uuid == 0 then
            bot_register(vars, x, y)
            uuid = vars:get_u32("@")
        else
            table.insert(bots_reg, { uuid = uuid, items = {}, var_handle = vars, pos = { x, y } })
        end

        if self_bot_uuid == 0 then
            self_bot_uuid = uuid
            print("claimed player at " .. x .. "," .. y)

            -- SDL ticks restart between runs, so movement interpolation state from
            -- the saved level must not throttle the first move after loading.
            movement.reset(vars)
        end
    end)
end

local function clear_hud_row(y, w)
    w = w or 30
    wrappers.text_fill(G_ui_width, y, G_ui_width + w, 0)
end

local held_icon_cells = {}
local held_icon_items = {}
local held_icon_pos = nil

local function update_held_icon(vars)
    local icon_layer = G_view_menu.held_items.layer

    if G_camera == nil or G_bot_pos == nil then
        for _, cell in pairs(held_icon_cells) do
            if cell then
                icon_layer:paste_block(cell.x, cell.y, 0)
            end
        end
        held_icon_cells = {}
        held_icon_items = {}
        held_icon_pos = nil
        return
    end

    local items = game_data.stack_items(vars:get_string("I"))
    local hx = G_bot_pos.x
    local hy = G_bot_pos.y

    if held_icon_pos ~= nil then
        local dx = hx - held_icon_pos.x
        local dy = hy - held_icon_pos.y
        if dx ~= 0 or dy ~= 0 then
            local first = dy > 0 and 1 or #held_icon_cells
            local last = dy > 0 and #held_icon_cells or 1
            local step = dy > 0 and 1 or -1
            for index = first, last, step do
                local cell = held_icon_cells[index]
                if cell then
                    local next_x = cell.x + dx
                    local next_y = cell.y + dy
                    if next_x < 0 or next_x >= G_width_blocks or next_y < 0 or next_y >= G_height_blocks then
                        icon_layer:paste_block(cell.x, cell.y, 0)
                        held_icon_cells[index] = nil
                    elseif icon_layer:move_block(cell.x, cell.y, dx, dy) then
                        -- move 1 past to edit the vars of the new cell, since the old cell is now empty
                        cell.x = next_x
                        cell.y = next_y

                        movement.copy_movement(icon_layer, next_x, next_y, vars)
                    end
                end
            end
        end
    end

    for index, item_id in ipairs(items) do
        local icon_y = hy - index + 1
        local cell = held_icon_cells[index]
        if cell == nil then
            if hx >= 0 and hx < G_width_blocks and icon_y >= 0 and icon_y < G_height_blocks then
                icon_layer:paste_block(hx, icon_y, item_id)
                movement.copy_movement(icon_layer, hx, icon_y, vars)
                held_icon_cells[index] = { x = hx, y = icon_y }
            end
        elseif held_icon_items[index] ~= item_id then
            icon_layer:paste_block(cell.x, cell.y, item_id)
            movement.copy_movement(icon_layer, cell.x, cell.y, vars)
        end
    end

    for index = #held_icon_cells, #items + 1, -1 do
        local cell = held_icon_cells[index]
        if cell then
            icon_layer:paste_block(cell.x, cell.y, 0)
            held_icon_cells[index] = nil
        end
    end

    held_icon_items = items
    held_icon_pos = { x = hx, y = hy }
end

local function refresh_hud(vars, x, y)
    local energy = vars:get_u16("e") or 0
    local max_energy = vars:get_u16("m") or 60
    local credits = vars:get_u16("c") or 0
    local items = game_data.stack_items(vars:get_string("I"))

    clear_hud_row(0)
    wrappers.text_print(G_ui_width, 0, 30, "ENERGY " .. energy .. "/" .. max_energy .. "  CREDITS " .. credits)

    clear_hud_row(1)
    if #items == 0 then
        wrappers.text_print(G_ui_width, 1, 30, "HOLD: nothing  [E grab/drop, click use]")
    else
        local name = game_data.name_of_id(items[1]) or ("#" .. items[1])
        wrappers.text_print(G_ui_width, 1, 30,
            "HOLD: " .. name .. " (" .. #items .. "/" .. game_data.stack_capacity(vars) .. ")  [E grab/drop, click use]")
    end

    clear_hud_row(2)
    if energy <= 0 then
        wrappers.text_print(G_ui_width, 2, 30, "*** GAME OVER ***")
    end

    update_held_icon(vars)
end

local frames = {
    stand = 0,
    walk_0 = 1,
    walk_1 = 2,
    grab_attempt = 3,
    has_items_stand = 4,
    has_items_walk_0 = 5,
    has_items_walk_1 = 6,
    has_items_grab_attempt = 7
}

G_bot_pos = nil
G_self_bot = nil
G_game_over = false

local function maybe_drain_energy(vars, e)
    local action_count = vars:get_u8("n") or 0
    if action_count >= 10 then
        vars:set_u8("n", 0)
        e = e - 1
    end
    return e
end

blockengine.register_handler(events.ENGINE_INIT, claim_existing_player)

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    --- @param layer Layer
    function(layer, x, y, value)
        if G_game_over then return end

        local vars = layer:get_vars(x, y)
        if not vars then return end

        if movement.moved_this_tick(vars) then return end

        local uuid = vars:get_u32("@")
        if uuid == nil or uuid == 0 then
            bot_register(vars, x, y)
            uuid = vars:get_u32("@")
        elseif self_bot_uuid == 0 then
            local newbot = { uuid = uuid, items = {}, var_handle = vars, pos = { x, y } }
            self_bot_uuid = uuid
            table.insert(bots_reg, newbot)
            print("claimed saved player at " .. x .. "," .. y)
        end
        if uuid ~= self_bot_uuid then return end

        G_self_bot = { vars = vars, pos = { x = x, y = y } }
        G_bot_pos = { x = x, y = y }

        local energy = vars:get_u16("e") or 0
        local max_energy = vars:get_u16("m") or 60
        if energy <= 0 then
            G_game_over = true
            refresh_hud(vars, x, y)
            return
        end

        local last_drain = vars:get_u32("d") or 0
        if last_drain == 0 then
            vars:set_u32("d", G_sdl_tick)
        elseif G_sdl_tick and G_sdl_tick - last_drain >= 10000 then
            energy = energy - 1
            vars:set_u32("d", G_sdl_tick)
        end

        local pos = { x = x, y = y }
        local delta = wasd.delta()
        local dir = vars:get_u8("t")

        local items_str = vars:get_string("I")
        local items = game_data.stack_items(items_str)
        local is_grabbing = wasd.consume_press('e')
        local is_standing = delta.x == 0 and delta.y == 0
        local frame_base = #items > 0 and frames.has_items_stand or frames.stand

        if is_grabbing then
            local front = vec.add(pos, vec.delta(dir))

            if #items > 0 then
                local held = items[1]
                local front_item = G_view_menu.items.layer:get_id(front.x, front.y)

                if front_item ~= 0 and not game_data.is_machine(front_item) and #items < game_data.stack_capacity(vars) then
                    G_view_menu.items.layer:paste_block(front.x, front.y, 0)
                    vars:set_string("I", game_data.stack_push(items_str, front_item))
                    vars:set_u8("n", (vars:get_u8("n") or 0) + 1)
                    energy = maybe_drain_energy(vars, energy)
                elseif game_data.is_machine(held) then
                    if G_view_menu.objects.layer:get_id(front.x, front.y) == 0 then
                        G_view_menu.objects.layer:paste_block(front.x, front.y, held)
                        vars:set_string("I", game_data.stack_pop(items_str))
                        vars:set_u8("n", (vars:get_u8("n") or 0) + 1)
                        energy = maybe_drain_energy(vars, energy)
                    end
                elseif game_data.is_scrap(held) and G_view_menu.floor.layer:get_id(front.x, front.y) == game_data.id("sell_pad") then
                    local price = game_data.price_of_id(held)
                    vars:set_string("I", game_data.stack_pop(items_str))
                    vars:set_u16("c", (vars:get_u16("c") or 0) + price)
                    vars:set_u8("n", (vars:get_u8("n") or 0) + 1)
                    energy = maybe_drain_energy(vars, energy)
                elseif front_item == 0 then
                    if not game_data.is_machine(held) then
                        game_data.place_item(G_view_menu.items.layer, front.x, front.y, held, x, y)
                        vars:set_string("I", game_data.stack_pop(items_str))
                        vars:set_u8("n", (vars:get_u8("n") or 0) + 1)
                        energy = maybe_drain_energy(vars, energy)
                    else
                        -- machines can only be placed on the objects layer
                    end
                end
            else
                local grab_id = G_view_menu.items.layer:get_id(front.x, front.y)
                local cap = game_data.stack_capacity(vars)
                if grab_id ~= 0 and #items < cap then
                    G_view_menu.items.layer:paste_block(front.x, front.y, 0)
                    vars:set_string("I", game_data.stack_push(items_str, grab_id))
                    vars:set_u8("n", (vars:get_u8("n") or 0) + 1)
                    energy = maybe_drain_energy(vars, energy)
                end
            end

            vars:set_u16("e", energy)
            vars:set_u8("v", frame_base + frames.grab_attempt)
            refresh_hud(vars, x, y)
            camera_utils.set_target(vec.mult(pos, G_block_size))
            return
        end

        if is_standing then
            vars:set_u16("e", energy)
            vars:set_u8("v", frame_base)
            refresh_hud(vars, x, y)
            return
        end

        local now = G_sdl_tick or 0
        if movement.last_move_tick(vars) ~= 0 and now > 0 and now >= movement.last_move_tick(vars) and
            now - movement.last_move_tick(vars) < move_interval_ms then
            vars:set_u16("e", energy)
            vars:set_u8("v", frame_base)
            refresh_hud(vars, x, y)
            return
        end

        vars:set_u8("v", frame_base + frames.walk_0 + G_tick % 2)
        vars:set_u8("t", vec.direction(delta.x, delta.y))

        local next_pos = vec.add(pos, delta)
        update_camera_interp()
        local id = G_view_menu.objects.layer:get_id(next_pos.x, next_pos.y)
        if id == 0 then
            if layer:move_block(pos.x, pos.y, delta.x, delta.y) then
                movement.begin(vars, delta.x, delta.y)
                vars:set_u8("n", (vars:get_u8("n") or 0) + 1)
                energy = maybe_drain_energy(vars, energy)
                G_bot_pos = next_pos
                G_self_bot.pos = next_pos
                camera_utils.set_target(vec.mult(next_pos, G_block_size))
                print("move to " .. next_pos.x .. "," .. next_pos.y .. " t=" .. (G_sdl_tick or 0))
            end
        end

        vars:set_u16("e", energy)
        refresh_hud(vars, x, y)
    end
)

scripting_light_block_input_register(scripting_current_light_registry, current_block, "click",
    --- @param layer Layer
    function(layer, x, y, input_value)
        if G_game_over then return end
        local vars = layer:get_vars(x, y)
        if not vars then return end

        local uuid = vars:get_u32("@")
        if uuid ~= self_bot_uuid then return end

        local items_str = vars:get_string("I")
        local items = game_data.stack_items(items_str)
        if #items == 0 then return end

        local held = items[1]

        if held == game_data.id("fuel_cell") then
            vars:set_string("I", game_data.stack_pop(items_str))
            vars:set_u16("e", vars:get_u16("m") or 60)
            refresh_hud(vars, x, y)
            return
        end

        if held == game_data.id("energy_upgrade") then
            vars:set_string("I", game_data.stack_pop(items_str))
            vars:set_u16("m", (vars:get_u16("m") or 60) + 10)
            vars:set_u16("e", (vars:get_u16("e") or 0) + 10)
            refresh_hud(vars, x, y)
            return
        end

        if held == game_data.id("stack_upgrade") then
            local remaining_items = game_data.stack_pop(items_str)
            local upgrades = vars:get_u8("p") or 0
            vars:set_string("I", remaining_items)
            vars:set_u8("p", upgrades + 1)
            refresh_hud(vars, x, y)
            return
        end
    end
)
