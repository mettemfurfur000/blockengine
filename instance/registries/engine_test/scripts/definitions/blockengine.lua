---@meta

local m = {}

--- @param func function
--- @param event_id integer
--- @return nil
function m.register_handler(event_id, func) return blockengine.register_handler(event_id, func) end

return m