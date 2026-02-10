---@class Image
---@field crop fun(self:Image, x:integer, y:integer, width:integer, height:integer):Image
---@field rotate fun(self:Image, angle:integer):Image
---@field size fun(self:Image):integer, integer
---@field flip_horizontal fun(self:Image):Image
---@field flip_vertical fun(self:Image):Image
---@field copy fun(self:Image):Image
---@field save fun(self:Image, path:string):nil
---@field add_brightness fun(self:Image, amount:integer):Image
---@field add_color fun(self:Image, r:integer, g:integer, b:integer, a:integer):Image
---@field get_avg_color fun(self:Image):integer, integer, integer
---@field image_gamma_correction fun(self:Image, gamma:number):Image
---@field overlay fun(self:Image, other_image:Image):Image
---@field clear fun(self:Image):Image
---@field set_pixel fun(self:Image, x:integer, y:integer, r:integer, g:integer, b:integer, a:integer):Image
---@field get_pixel fun(self:Image, x:integer, y:integer):integer, integer, integer, integer
---@field fill fun(self:Image, r:integer, g:integer, b:integer, a:integer):Image
---@field quantize fun(self:Image, n:integer):nil
---@field dither fun(self:Image, n:integer):nil
local image = {}

local m = {}

---@param path string
---@return Image
function m.load(path)
    return ie.load(path)
end

---@param width integer
---@param height integer
---@return Image
function m.create(width, height)
    return ie.create(width, height)
end

return m