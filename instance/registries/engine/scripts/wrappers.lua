local M = {}

function M.log_error(msg)
    log_msg(2, msg)
end

function M.log_message(msg)
    log_msg(1, msg)
end

function M.try(f, catch_f)
    local status, exception = pcall(f)
    if not status then
        catch_f(exception)
    end
end

function M.world_fill(x, y, w, h, id)
    id = id or 0

    for j = y, h do
        for i = x, w do
            G_view_menu.text.layer:paste_block(i, j, id)
        end
    end
end

function M.world_print(x, y, width, msg)
    if type(msg) ~= "string" then
        msg = tostring(msg)
    end

    if msg == " " then
        for i = 0, width do
            G_view_menu.text.layer:paste_block(x + i, y, 0)
        end
    end

    local spaces = width - #msg
    local spaces_str = ""
    for i = 1, spaces do
        spaces_str = spaces_str .. " "
    end

    G_view_menu.text.layer:bprint(G_character_id, x, y, width, msg .. spaces_str)
end

function M.tablelength(T)
    local count = 0
    for _ in pairs(T) do
        count = count + 1
    end
    return count
end

function M.print_table(t, indent)
    indent = indent or 0
    for k, v in pairs(t) do
        if type(v) == "table" then
            print(string.rep("  ", indent) .. k .. " = {")
            M.print_table(v, indent + 1)
            print(string.rep("  ", indent) .. "}")
        else
            print(string.rep("  ", indent) .. k .. " = " .. tostring(v))
        end
    end
end

function M.safe_registry_load(level, name)
    local registry = level:deserialize_registry(name)

    if registry == nil then
        M.log_error("error deserializing engine registry, reading from folders instead")

        registry = level:load_registry(name)

        if registry == nil then
            M.log_error("error loading engine registry")
            os.exit()
        end
    end

    M.log_message("loaded registry " .. name)

    return registry
end

function M.safe_room_create(level, name, width, height)
    print("creating room " .. name .. " size " .. width .. "x" .. height)
    local menu = level:new_room(name, width, height)

    if menu == nil then
        M.log_error("error creating room")
        os.exit()
    end

    return menu
end

function M.safe_layer_create(room, registry_name, block_width, enable_vars)
    if block_width <= 0 or registry_name == nil or room == nil then
        M.log_error("invalid arguments for layer creation")
        os.exit()
    end

    local layer = room:new_layer(registry_name, block_width, enable_vars and 1 or 0)

    if layer == nil then
        M.log_error("error creating a floor")
        os.exit()
    end

    return layer
end

function M.find_block(reg_table, filename)
    for k, v in pairs(reg_table) do
        if v.all_fields ~= nil then
            local file_src = v.all_fields.source_filename
            local match = string.gmatch(file_src, "/(%w+).blk$")()
            if match == filename then
                return v
            end
        end
    end
end

return M
