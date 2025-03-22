local constants = require("registries.engine.scripts.constants")

local test_level = le.create_level("test")

local status = test_level:load_registry("engine")
if status == false then
    print("error loading registry")
    os.exit()
end

local menu_room = test_level:new_room("menu", 20, 8)

-- new layer creation: select a registry to get blocks from, bytes per block, and additional flags
local flags = 1 -- magic flag to enable layer variables
local floor_l = menu_room:new_layer("engine", 1, flags)

print("layer created")

local skibidi = "toilet"

floor_l:paste_block(4, 4, 1) -- x, y, id
floor_l:paste_block(4, 6, 2) -- x, y, id
floor_l:bprint(1, 1, 1, 16, "test string, 16 chars\n".. "magic word: " .. skibidi)

print("blocks pasted")

local render_order = {0}

local width, height = render_rules.get_size(g_render_rules)

render_rules.set_order(g_render_rules, render_order)
render_rules.set_slice(g_render_rules, 0, floor_l, 0, 0, width, height, 1)
