local wrappers = require("registries.engine.scripts.wrappers")

local M = {}

M.price = {
    item_nut = 2,
    item_sheet = 5,
    item_pipe = 5,
    item_gear = 10,
    item_spring = 25,
    item_camera = 50,
    item_engine = 75,
    item_cpu = 100,
}

M.machines = { "collector_bot", "drill", "assembler", "collector_beacon" }
M.usables = { "fuel_cell", "energy_upgrade", "stack_upgrade" }

M.robot_default_max_fuel = 60

M.fuel_value = 20
M.craft_cost = 2
M.mine_cost = 1

M.machine_max_fuel = 100

local id_cache = {}
local name_cache = {}

function M.id(name)
    if id_cache[name] ~= nil then
        return id_cache[name]
    end
    local b = wrappers.find_block(G_engine_table, name)
    if not b then
        return 0
    end
    id_cache[name] = b.id
    return b.id
end

function M.place_item(layer, x, y, id, from_x, from_y)
    layer:paste_block(x, y, id)
    if id == 0 then return end

    local vars = layer:get_vars(x, y)
    if not vars then return end

    local previous_x = from_x or x
    local previous_y = from_y or y

    local movement = trait.get_block_api(id).movement
    if movement then
        movement.begin(vars, x - previous_x, y - previous_y)
    end
end

function M.name_of_id(id)
    if id == nil then
        return nil
    end
    if name_cache[id] ~= nil then
        return name_cache[id]
    end
    for k, v in pairs(G_engine_table) do
        if v.id == id then
            local sf = v.all_fields and v.all_fields.source_filename
            if sf then
                local m = string.match(sf, "[^\\/]*%.blk$")
                if m then
                    name_cache[id] = string.sub(m, 1, -5)
                    return name_cache[id]
                end
            end
        end
    end
    name_cache[id] = false
    return nil
end

function M.price_of_id(id)
    local name = M.name_of_id(id)
    if not name then
        return 0
    end
    return M.price[name] or 0
end

function M.is_machine(id)
    local name = M.name_of_id(id)
    for i = 1, #M.machines do
        if M.machines[i] == name then
            return true
        end
    end
    return false
end

function M.is_usable(id)
    local name = M.name_of_id(id)
    for i = 1, #M.usables do
        if M.usables[i] == name then
            return true
        end
    end
    return false
end

function M.is_scrap(id)
    return M.price_of_id(id) > 0
end

local scrap_ids = {}

function M.random_scrap_id()
    if #scrap_ids == 0 then
        for name, _ in pairs(M.price) do
            local id = M.id(name)
            if id ~= 0 then
                scrap_ids[#scrap_ids + 1] = id
            end
        end
    end
    if #scrap_ids == 0 then
        return 0
    end
    return scrap_ids[math.random(1, #scrap_ids)]
end

function M.scrap_ids()
    if #scrap_ids == 0 then
        for name, _ in pairs(M.price) do
            local id = M.id(name)
            if id ~= 0 then
                scrap_ids[#scrap_ids + 1] = id
            end
        end
    end
    return scrap_ids
end

function M.stack_init()
    return string.rep(string.char(0x01), 15)
end

function M.stack_items(item_str)
    local ret = {}
    for i = 1, #item_str do
        local id = string.byte(item_str, i) - 1
        if id == 0 then break end
        ret[#ret + 1] = id
    end
    return ret
end

function M.stack_total(item_str)
    local n = 0
    for i = 1, #item_str do
        if string.byte(item_str, i) == 0x01 then break end
        n = n + 1
    end
    return n
end

function M.stack_capacity(vars)
    return 1 + (vars:get_u8("p") or 0)
end

local function stack_build(items)
    local s = ""
    for _, v in ipairs(items) do
        s = s .. string.char(v + 1)
    end
    while #s < 15 do
        s = s .. string.char(0x01)
    end
    return s
end

function M.stack_push(item_str, id)
    local items = M.stack_items(item_str)
    items[#items + 1] = id
    return stack_build(items)
end

function M.stack_prepend(item_str, id)
    local items = M.stack_items(item_str)
    table.insert(items, 1, id)
    return stack_build(items)
end

function M.stack_pop(item_str)
    local items = M.stack_items(item_str)
    local popped = table.remove(items, 1)
    return stack_build(items), popped
end

return M