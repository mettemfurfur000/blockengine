m = {}

--- @param name string
--- @return BlockLevel
function m.create_level(name) return le.create_level(name) end

--- @param name string
--- @return BlockLevel | nil error
function m.load_level(name) return le.load_level(name) end

--- @param name string
--- @param level BlockLevel
--- @return boolean
function m.save_level(level, name) return le.save_level(level, name) end

return m