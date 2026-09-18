local wrappers = require("registries.engine.scripts.wrappers")

local fuel_key = "F"
local fuel_max_key = "M"
local fuel_cfg_max_entry = "cfg_fuel_max"

trait.add_var(fuel_key, 1)
trait.add_var(fuel_max_key, 1)

local max_fuel = trait.get_resource_field(fuel_cfg_max_entry)

if not max_fuel then
    wrappers.log_warning(fuel_cfg_max_entry ..
        " is not set for block \'" ..
        trait.get_resource_field("source_filename") ..
        "\', setting to 20 by default")
    max_fuel = "20"
end

trait.set_u8(fuel_max_key, tonumber(max_fuel))

local api = {}

function api.spend_fuel(layer, x, y, amount)
    amount = amount or 1
    local vars = layer:get_vars(x, y)
    if not vars then return false end

    local fuel = vars:get_u8(fuel_key) or 0
    if fuel <= 0 then return false end

    fuel = fuel - amount
    vars:set_u8(fuel_key, fuel)

    return true
end

function api.get_fuel(layer, x, y)
    local vars = layer:get_vars(x, y)
    if not vars then return 0 end

    return vars:get_u8(fuel_key) or 0
end

function api.set_fuel(layer, x, y, amount)
    local vars = layer:get_vars(x, y)
    if not vars then return false end

    vars:set_u8(fuel_key, amount)

    return true
end

function api.get_max_fuel(layer, x, y)
    local vars = layer:get_vars(x, y)
    if not vars then return 0 end

    return vars:get_u8(fuel_max_key) or 0
end

function api.set_max_fuel(layer, x, y, amount)
    local vars = layer:get_vars(x, y)
    if not vars then return false end

    vars:set_u8(fuel_max_key, amount)

    return true
end

-- returns overflow amount if fuel exceeds max fuel_trait
function api.add_fuel(layer, x, y, amount)
    local vars = layer:get_vars(x, y)
    if not vars then return false, 0 end

    local fuel = vars:get_u8(fuel_key) or 0
    local max_fuel = vars:get_u8(fuel_max_key) or 0

    local overflow = math.max(0, fuel + amount - max_fuel)

    fuel = math.min(fuel + amount, max_fuel)
    vars:set_u8(fuel_key, fuel)

    return true, overflow
end

trait.register_api("refuelable", api)
