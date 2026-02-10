-- This is a template for a texture generator script. It loads an image, processes it, and prints the average color.

local ie = require("registries.texgen.scripts.definitions.image_editor")

local function parse_c_args()
    local args = {}
    for i, v in ipairs(c_args) do
        if v:find("--") == 1 then
            local key, value = v:match("%-%-(%w+)=(.+)")
            if key and value then
                args[key] = value
            end
        else
            table.insert(args, v)
        end
    end
    return args
end

local args = parse_c_args()

local input_file = args.input or "input.png"

local image = ie.load(input_file)

r, g, b, a = image:get_avg_color()

print(string.format("#%02x%02x%02x%02x", r, g, b, a))
