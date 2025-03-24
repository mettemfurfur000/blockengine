---
-- Generates a connected texture mapping from a 16x12 pixel map.
-- 
-- The texture mapping is divided into several sections:
-- - 4 upper 4x4 squares for block corners, handling different corner connection scenarios
-- - 2 4x8 side stripes for handling block side connections
-- - 1 8x8 center section
--
-- Requires predefined input_file and output_file variables for processing.
local image = ie.load(input_file)

local width, height = image:size()

if width ~= 16 and height ~= 12 then
    error("image size must be 16x12")
    os.exit(1)
end

local function swap(a, b)
    return b, a
end

local function t_copy(t)
    local result = {}
    for i, v in ipairs(t) do
        result[i] = v
    end
    return result
end
-- first generate/write all possible blocks with all possible neighbors
local function generate_neighbour_map(number)
    local result = {}
    for i = 1, 8 do
        result[i] = number % 2
        number = math.floor(number / 2)
    end
    return result
end

local function rotate_once(map)
    local r = t_copy(map)

    local tmp = r[1]
    r[1] = r[6]
    r[6] = r[8]
    r[8] = r[3]
    r[3] = tmp

    tmp = r[2]
    r[2] = r[4]
    r[4] = r[7]
    r[7] = r[5]
    r[5] = tmp

    return r
end

local function rotate_n_times(map, n)
    local result = t_copy(map)
    for i = 1, n do
        result = rotate_once(result)
    end
    return result
end

local function maps_match(map1, map2)
    for i = 1, 8 do
        if map1[i] ~= map2[i] then
            return false
        end
    end
    return true
end

local function vertical_flip(map)
    local result = t_copy(map)

    result[1], result[6] = swap(result[1], result[6])
    result[2], result[7] = swap(result[2], result[7])
    result[3], result[8] = swap(result[3], result[8])

    return result
end

local function horizontal_flip(map)
    local result = t_copy(map)

    result[1], result[3] = swap(result[1], result[3])
    result[4], result[5] = swap(result[4], result[5])
    result[6], result[8] = swap(result[6], result[8])

    return result
end

local function print_table(t)
    for i, v in ipairs(t) do
        if type(v) == "table" then
            print_table(v)
        end
        print(v)
    end
end

local function print_map(t)
    print(t[1], t[2], t[3])
    print(t[4], 0, t[5])
    print(t[6], t[7], t[8])
end

local variants_to_generate = 256

-- assembling our image components

local corner_000 = image:crop(0, 0, 4, 4)
local corner_001 = image:crop(4, 0, 4, 4)
local corner_101 = image:crop(8, 0, 4, 4)
local corner_111 = image:crop(12, 0, 4, 4)

local side_0 = image:crop(0, 4, 4, 8)
local side_1 = image:crop(4, 4, 4, 8)

local center = image:crop(8, 4, 8, 8)

local function assemble_texture(map, dest_image, x_offset, y_offset)

    -- non-similar corner variants
    if map[2] == 0 and map[4] == 0 then
        dest_image:overlay(corner_000, x_offset, y_offset)
    end

    if map[2] == 0 and map[5] == 0 then
        dest_image:overlay(corner_000:rotate(1), x_offset + 12, y_offset)
    end

    if map[5] == 0 and map[7] == 0 then
        dest_image:overlay(corner_000:rotate(2), x_offset + 12, y_offset + 12)
    end

    if map[4] == 0 and map[7] == 0 then
        dest_image:overlay(corner_000:rotate(3), x_offset, y_offset + 12)
    end

    -- similar corner variants
    -- 2 each
    if map[2] == 0 and map[4] == 1 then
        dest_image:overlay(corner_001:flip_vertical():rotate(2), x_offset, y_offset)
    end
    if map[2] == 1 and map[4] == 0 then
        dest_image:overlay(corner_001:rotate(3), x_offset, y_offset)
    end

    if map[2] == 1 and map[5] == 0 then
        dest_image:overlay(corner_001:rotate(1):flip_vertical(), x_offset + 12, y_offset)
    end
    if map[2] == 0 and map[5] == 1 then
        dest_image:overlay(corner_001, x_offset + 12, y_offset)
    end

    if map[5] == 1 and map[7] == 0 then
        dest_image:overlay(corner_001:rotate(2):flip_horizontal(), x_offset + 12, y_offset + 12)
    end
    if map[5] == 0 and map[7] == 1 then
        dest_image:overlay(corner_001:rotate(1), x_offset + 12, y_offset + 12)
    end

    if map[4] == 1 and map[7] == 0 then
        dest_image:overlay(corner_001:rotate(2), x_offset, y_offset + 12)
    end
    if map[4] == 0 and map[7] == 1 then
        dest_image:overlay(corner_001:rotate(3), x_offset, y_offset + 12)
    end

    -- 2 similar neighbours but 1 different block on the corner
    if map[1] == 0 and map[2] == 1 and map[4] == 1 then
        dest_image:overlay(corner_101, x_offset, y_offset)
    end

    if map[3] == 0 and map[2] == 1 and map[5] == 1 then
        dest_image:overlay(corner_101:rotate(1), x_offset + 12, y_offset)
    end

    if map[8] == 0 and map[5] == 1 and map[7] == 1 then
        dest_image:overlay(corner_101:rotate(2), x_offset + 12, y_offset + 12)
    end

    if map[6] == 0 and map[4] == 1 and map[7] == 1 then
        dest_image:overlay(corner_101:rotate(3), x_offset, y_offset + 12)
    end

    -- all three
    if map[1] == 1 and map[2] == 1 and map[4] == 1 then
        dest_image:overlay(corner_111, x_offset, y_offset)
    end

    if map[3] == 1 and map[2] == 1 and map[5] == 1 then
        dest_image:overlay(corner_111:rotate(1), x_offset + 12, y_offset)
    end

    if map[8] == 1 and map[5] == 1 and map[7] == 1 then
        dest_image:overlay(corner_111:rotate(2), x_offset + 12, y_offset + 12)
    end

    if map[6] == 1 and map[4] == 1 and map[7] == 1 then
        dest_image:overlay(corner_111:rotate(3), x_offset, y_offset + 12)
    end
    -- sides

    if map[4] == 0 then
        dest_image:overlay(side_0, x_offset, y_offset + 4)
    else
        dest_image:overlay(side_1, x_offset, y_offset + 4)
    end

    if map[2] == 0 then
        dest_image:overlay(side_0:rotate(1), x_offset + 4, y_offset)
    else
        dest_image:overlay(side_1:rotate(1), x_offset + 4, y_offset)
    end

    if map[5] == 0 then
        dest_image:overlay(side_0:rotate(2), x_offset + 12, y_offset + 4)
    else
        dest_image:overlay(side_1:rotate(2), x_offset + 12, y_offset + 4)
    end

    if map[7] == 0 then
        dest_image:overlay(side_0:rotate(3), x_offset + 4, y_offset + 12)
    else
        dest_image:overlay(side_1:rotate(3), x_offset + 4, y_offset + 12)
    end

    -- center
    dest_image:overlay(center, x_offset + 4, y_offset + 4)
end

local function generate_all_possible()
    local variants = {}
    for i = 1, variants_to_generate do
        local map = generate_neighbour_map(i - 1)
        local is_dup = false
        local last_dup_found_index = 0

        for j = 1, #variants do
            if i == 1 then
                break
            end -- ignore the first one
            -- flip, rotate a map to see if it matches any maps we already have
            -- if yes, mark one that we have with said operation to replecate a duplicate

            -- simple check for side duplicate
            if variants[j][2] == map[2] and variants[j][4] == map[4] and variants[j][5] == map[5] and variants[j][7] ==
                map[7] then
                last_dup_found_index = j
                is_dup = true
            end

            if maps_match(variants[j], rotate_n_times(map, 1)) == true then
                variants[j].rotate_dup_1 = true
                last_dup_found_index = j
                is_dup = true
            end

            if maps_match(variants[j], rotate_n_times(map, 2)) == true then
                variants[j].rotate_dup_2 = true
                last_dup_found_index = j
                is_dup = true
            end

            if maps_match(variants[j], rotate_n_times(map, 3)) == true then
                variants[j].rotate_dup_3 = true
                last_dup_found_index = j
                is_dup = true
            end

            if maps_match(variants[j], vertical_flip(map)) == true then
                variants[j].vertical_flip = true
                last_dup_found_index = j
                is_dup = true
            end

            if maps_match(variants[j], horizontal_flip(map)) == true then
                variants[j].horizontal_flip = true
                last_dup_found_index = j
                is_dup = true
            end
        end

        if is_dup == false then
            table.insert(variants, map)
        else
            print("duplicate found for " .. last_dup_found_index)
        end
    end

    return variants
end

local function generate_simple_variants()
    -- only generates 6 types of blocks, ignoring the corner ones and just assuming the sides are the same
    local variants = {}
    table.insert(variants, {0, 0, 0, 0, 0, 0, 0, 0})
    table.insert(variants, {0, 1, 0, 0, 0, 0, 0, 0})
    table.insert(variants, {0, 1, 1, 0, 1, 0, 0, 0})
    table.insert(variants, {0, 0, 0, 1, 1, 0, 0, 0})
    table.insert(variants, {1, 1, 1, 1, 1, 0, 0, 0})
    table.insert(variants, {1, 1, 1, 1, 1, 1, 1, 1})
    return variants
end

local function generate_simple_variants_assume_corner_diff()
    -- only generates 6 types of blocks, ignoring the corner ones and just assuming the sides are the same
    local variants = {}
    table.insert(variants, {0, 0, 0, 0, 0, 0, 0, 0})
    table.insert(variants, {0, 1, 0, 0, 0, 0, 0, 0})
    table.insert(variants, {0, 1, 0, 0, 1, 0, 0, 0})
    table.insert(variants, {0, 0, 0, 1, 1, 0, 0, 0})
    table.insert(variants, {0, 1, 0, 1, 1, 0, 0, 0})
    table.insert(variants, {0, 1, 0, 1, 1, 0, 1, 0})
    return variants
end

-- local variants = generate_simple_variants()
-- local variants = generate_simple_variants_assume_corner_diff()
local variants = generate_all_possible()

local variants_count = #variants
local textures_per_row = 6
local columns = math.ceil(variants_count / textures_per_row)

local out_image = ie.create(textures_per_row * 16, columns * 16)

for i = 1, variants_count do
    local map = variants[i]

    print("generating " .. i .. " / " .. variants_count)

    assemble_texture(map, out_image, math.floor((i - 1) % textures_per_row) * 16,
        math.floor((i - 1) / textures_per_row) * 16)
end

out_image:save(output_file)
