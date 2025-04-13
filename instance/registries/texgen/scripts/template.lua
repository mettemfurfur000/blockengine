local image = ie.load(input_file)

local width, height = image:size()

-- if width ~= 16 and height ~= 12 then
--     error("image size must be 16x12")
--     os.exit(1)
-- end

r, g, b = image:get_avg_color()

print(r, g, b)
