local vec = require("registries.engine.scripts.vector_additions")
local wrappers = require("registries.engine.scripts.wrappers")
local blockengine = require("registries.engine.scripts.definitions.blockengine")
local camera_utils = require("registries.engine.scripts.camera_utils")
local block_utils = require("registries.engine.scripts.block_utils")
local game_data = require("registries.engine.scripts.game_data")

local current_block = scripting_current_block_id

G_shop_open = false

local SHOP_LEFT = 35
local SHOP_TOP = 4
local SHOP_COLS = 14
local SHOP_ROWS = 6

local menu_items = {
    { name = "fuel cell", price = 20, give = "fuel_cell" },
    { name = "energy upgrade", price = 40, give = "energy_upgrade", dynamic = true },
    { name = "stack upgrade", price = 60, give = "stack_upgrade" },
    { name = "scrap collector", price = 100, give = "collector_bot" },
    { name = "scrap drill", price = 200, give = "drill" },
    { name = "assembler", price = 400, give = "assembler" },
}

local function keeper_vars(layer, x, y)
    local vars = layer:get_vars(x, y)
    if not vars then return nil end
    if vars:get_u8("h") == nil then vars:set_u8("h", 0) end
    return vars
end

local function row_price(kvars, idx)
    local item = menu_items[idx]
    if item.dynamic then
        local h = kvars:get_u8("h") or 0
        return 40 + 20 * h
    end
    return item.price
end

local function icon_cell(idx)
    local col = (idx - 1) % 3
    local brow = math.floor((idx - 1) / 3)
    return {
        x = SHOP_LEFT + 3 + col * 4,
        y = SHOP_TOP + 1 + brow * 2
    }
end

local function draw_menu(kvars)
    local pallete = G_view_menu.pallete.layer
    local text = G_view_menu.text.layer
    local panel_id = game_data.id("panel")

    for j = SHOP_TOP, SHOP_TOP + SHOP_ROWS - 1 do
        for i = SHOP_LEFT, SHOP_LEFT + SHOP_COLS - 1 do
            pallete:paste_block(i, j, panel_id)
        end
    end

    wrappers.world_print(SHOP_LEFT + 5, SHOP_TOP, 4, "SHOP")

    for i = 1, #menu_items do
        local cell = icon_cell(i)
        local item = menu_items[i]
        local give_id = game_data.id(item.give)
        text:paste_block(cell.x, cell.y, give_id)
        wrappers.world_print(SHOP_LEFT + 1 + ((i - 1) % 3) * 4, cell.y + 1, 4, tostring(row_price(kvars, i)))
    end

    local credits = 0
    if G_self_bot and G_self_bot.vars then
        credits = G_self_bot.vars:get_u16("c") or 0
    end
    wrappers.world_print(SHOP_LEFT + 1, SHOP_TOP + SHOP_ROWS - 1, 12, "credits: " .. credits)
end

local function clear_menu()
    local pallete = G_view_menu.pallete.layer
    local text = G_view_menu.text.layer

    for j = SHOP_TOP, SHOP_TOP + SHOP_ROWS - 1 do
        for i = SHOP_LEFT, SHOP_LEFT + SHOP_COLS - 1 do
            pallete:paste_block(i, j, 0)
            text:paste_block(i, j, 0)
        end
    end
end

local function grant_item(kvars, idx, layer, x, y)
    if not G_self_bot or not G_self_bot.vars then return end
    local item = menu_items[idx]
    local price = row_price(kvars, idx)
    local credits = G_self_bot.vars:get_u16("c") or 0
    if credits < price then return end

    G_self_bot.vars:set_u16("c", credits - price)
    print("bought " .. item.name .. " for " .. price .. ", credits now " .. (credits - price))
    if item.dynamic then
        kvars:set_u8("h", (kvars:get_u8("h") or 0) + 1)
    end

    local give_id = game_data.id(item.give)
    if give_id == 0 then return end

    local items_str = G_self_bot.vars:get_string("I")
    local cap = game_data.stack_capacity(G_self_bot.vars)
    if game_data.stack_total(items_str) < cap then
        G_self_bot.vars:set_string("I", game_data.stack_push(items_str, give_id))
    else
        local bx, by = nil, nil
        local front = vec.add(G_self_bot.pos, vec.delta(G_self_bot.vars:get_u8("t") or 1))
        if G_view_menu.items.layer:get_id(front.x, front.y) == 0 then
            bx, by = front.x, front.y
        elseif bx == nil then
            for dy = -1, 1 do
                for dx = -1, 1 do
                    if (dx ~= 0 or dy ~= 0) and G_view_menu.items.layer:get_id(front.x + dx, front.y + dy) == 0 then
                        bx, by = front.x + dx, front.y + dy
                    end
                end
            end
        end
        if bx then
            game_data.place_item(G_view_menu.items.layer, bx, by, give_id, x, y)
        end
    end

    draw_menu(kvars)
end

scripting_light_block_input_register(scripting_current_light_registry, current_block, "tick",
    function(layer, x, y, value)
        local kvars = keeper_vars(layer, x, y)
        if not kvars then return end

        if G_shop_open then
            draw_menu(kvars)
        end

        local near = false
        if G_bot_pos then
            local dx = G_bot_pos.x - x
            local dy = G_bot_pos.y - y
            near = math.abs(dx) + math.abs(dy) <= 2
        end
        kvars:set_u8("t", near and 1 or 0)
    end
)

scripting_light_block_input_register(scripting_current_light_registry, current_block, "click",
    function(layer, x, y, input_value)
        G_shop_open = not G_shop_open
        if G_shop_open then
            print("shop open")
            draw_menu(keeper_vars(layer, x, y))
            camera_utils.set_target(vec.mult({ x = x, y = y }, G_block_size))
        else
            print("shop closed")
            clear_menu()
        end
    end
)

blockengine.register_handler(events.SDL_MOUSEBUTTONDOWN, function(sx, sy, state, clicks, button)
    if not G_shop_open then return end
    local rect = block_utils.pixel_to_layer_blocks(G_view_menu.pallete.index, { x = sx, y = sy })
    if rect.x < SHOP_LEFT or rect.x > SHOP_LEFT + 11 then return end
    if rect.y < SHOP_TOP + 1 or rect.y > SHOP_TOP + 4 then return end

    local col = math.floor((rect.x - SHOP_LEFT) / 4)
    local brow = math.floor((rect.y - SHOP_TOP - 1) / 2)
    local row = brow * 3 + col + 1
    if row < 1 or row > #menu_items then return end

    local kvars = nil
    G_view_menu.objects.layer:for_each(current_block, function(kx, ky)
        if kvars == nil then kvars = keeper_vars(G_view_menu.objects.layer, kx, ky) end
    end)
    if kvars then
        local price = row_price(kvars, row)
        if not G_self_bot or not G_self_bot.vars or (G_self_bot.vars:get_u16("c") or 0) >= price then
            grant_item(kvars, row, nil, 0, 0)
        end
    end
end)