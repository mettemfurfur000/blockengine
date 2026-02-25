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

local function to_hex(n)
    return string.format("%02X", n)
end

local function hex(r, g, b)
    return "#" .. to_hex(r) .. to_hex(g) .. to_hex(b)
end

local function to_binary(n)
    local result = ""
    for i = 7, 0, -1 do
        result = ((n & 1) == 1 and "1" or "0") .. result
        n = n >> 1
    end
    return result
end


---@param image Image
local function get_pixel_neighbour_map(image, offset_x, offset_y)
    local ret = 0
    for y = 0, 2 do
        for x = 0, 2 do
            -- for y = 2, 0, -1 do
            --     for x = 2, 0, -1 do
            if not (x == 1 and y == 1) then
                local acc_x = offset_x + 4 + (x * 4)
                local acc_y = offset_y + 4 + (y * 4)

                local r, g, b, a = image:get_pixel(
                    acc_x,
                    acc_y
                )

                -- print(acc_x .. " " .. acc_y .. " = " .. hex(r, g, b))

                if r ~= 0xff or g ~= 0xff or b ~= 0xff then
                    ret = ret | 1
                    image:set_pixel(acc_x, acc_y, 11, 128, 11, 255)
                else
                    image:set_pixel(acc_x, acc_y, 128, 11, 11, 255)
                end
                ret = ret << 1
            end
        end
    end
    ret = ret >> 1
    return ret
end

local args = parse_c_args()

local input_file = args.input or "input.png"

local image = ie.load(input_file)

local w, h = image:size()
local block_width = 16

local blocks_w, blocks_h = w / block_width, h / block_width
print(blocks_w, blocks_h)

for y = 0, blocks_h - 1 do
    for x = 0, blocks_w - 1 do
        local state = get_pixel_neighbour_map(image, x * block_width, y * block_width)

        print(y * blocks_w + x, to_binary(state))
    end
end

-- image:get_pixel()

-- image:dither(2)

image:save("output.png")

print("Done!")
