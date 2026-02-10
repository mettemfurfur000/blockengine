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

image:dither(2)

image:save("output.png")

print("Done!")
