---@meta

---@alias char string

---@class VarsHandle
---@field is_valid fun(self:VarsHandle):boolean
---@field release fun(self:VarsHandle):boolean
---@field get_length fun(self:VarsHandle):integer
---@field get_string fun(self:VarsHandle, key:char):string, nil
---@field set_string fun(self:VarsHandle, key:char, str:string):boolean
---@field add fun(self:VarsHandle, key:char, length:integer):boolean
---@field parse fun(self:VarsHandle, input:string):boolean
---@field set_i8 fun(self:VarsHandle, key:char, num:integer):boolean
---@field set_i16 fun(self:VarsHandle, key:char, num:integer):boolean
---@field set_i32 fun(self:VarsHandle, key:char, num:integer):boolean
---@field set_i64 fun(self:VarsHandle, key:char, num:integer):boolean
---@field set_u8 fun(self:VarsHandle, key:char, num:integer):boolean
---@field set_u16 fun(self:VarsHandle, key:char, num:integer):boolean
---@field set_u32 fun(self:VarsHandle, key:char, num:integer):boolean
---@field set_u64 fun(self:VarsHandle, key:char, num:integer):boolean
---@field get_i8 fun(self:VarsHandle, key:char):integer|nil
---@field get_i16 fun(self:VarsHandle, key:char):integer|nil
---@field get_i32 fun(self:VarsHandle, key:char):integer|nil
---@field get_i64 fun(self:VarsHandle, key:char):integer|nil
---@field get_u8 fun(self:VarsHandle, key:char):integer|nil
---@field get_u16 fun(self:VarsHandle, key:char):integer|nil
---@field get_u32 fun(self:VarsHandle, key:char):integer|nil
---@field get_u64 fun(self:VarsHandle, key:char):integer|nil
---@field get_size fun(self:VarsHandle, key:char):integer|nil
---@field __tostring fun(self:VarsHandle):string
---@field get_raw fun(self:VarsHandle):integer,integer|nil
---@field remove fun(self:VarsHandle, key:char):boolean
---@field resize fun(self:VarsHandle, key:char,new_size:integer):boolean
---@field rename fun(self:VarsHandle, key_old:char, key_new:char):boolean
---@field ensure fun(self:VarsHandle, key:char, needed:integer):nil|integer

---@class Layer
---@field get_size fun(self:Layer):integer, integer, integer, integer
---@field for_each fun(self:Layer, filter_id:integer, callback:function): nil
---@field set_id fun(self:Layer, x:integer, y:integer, id:integer):nil
---@field get_id fun(self:Layer, x:integer, y:integer):integer
---@field move_block fun(self:Layer, x:integer, y:integer, d_x:integer, d_y:integer):boolean
---@field paste_block fun(self:Layer, x:integer, y:integer, id:integer):boolean
---@field get_input_handler fun(self:Layer, x:integer, y:integer, name:string):function|nil
---@field set_static fun(self:Layer, val:integer):nil
---@field get_vars fun(self:Layer, x:integer, y:integer):boolean, VarsHandle
---@field set_vars fun(self:Layer, x:integer, y:integer, vars:VarsHandle):boolean
---@field bprint fun(self:Layer, char_id:integer, orig_x:integer, orig_y:integer, limit:integer, format:string):nil
---@field tick fun(self:Layer, value:integer):nil
---@field uuid fun(self:Layer):integer

---@class Room
---@field get_name fun(self:Room):string
---@field get_size fun(self:Room):integer, integer
---@field get_layer fun(self:Room, index:integer):Layer
---@field get_layer_count fun(self:Room):integer
---@field new_layer fun(self:Room, reg_name:string, block_width:integer, unused:integer, flags:integer):Layer
---@field uuid fun(self:Room):integer
local room = {}

---@class Sound
---@field play fun(self:Sound):integer
---@field set_volume fun(self:Sound, volume:integer):integer
local sound = {}

---@class BlockRegistry
---@field get_name fun(self:BlockRegistry):string
---@field to_table fun(self:BlockRegistry):table
---@field register_input fun(self:BlockRegistry, id:integer, name:string, func:function):boolean
---@field uuid fun(self:BlockRegistry):integer
local registry = {}

---@class BlockLevel
---@field load_registry fun(self:BlockLevel, name:string):BlockRegistry
---@field get_registries fun(self:BlockLevel):BlockRegistry[]
---@field get_room_count fun(self:BlockLevel):integer
---@field find_room fun(self:BlockLevel, name:string):Room|nil
---@field get_name fun(self:BlockLevel):string
---@field get_room fun(self:BlockLevel, index:integer):Room
---@field new_room fun(self:BlockLevel, name:string, width:integer, height:integer):Room
---@field uuid fun(self:BlockLevel):integer
local local_level = {}
