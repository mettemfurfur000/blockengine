-- always defined: input_file, output_file

-- tests gamma correction

local image = ie.load(input_file)

local iterations = 8
local gamma_start = 0.25
local gamma_step = 0.125

local width, height = image:size()

local results = ie.create(width + width * iterations, height)

results:overlay(image, 0, 0) -- place original image

for i = 1, iterations do
    -- place gamma corrected image
    results:overlay(image:copy():gamma_correction(gamma_start + gamma_step * i), i * width, 0)
end

results:save(output_file)
