log_error = function(msg)
    log_msg(2, msg)
end

log_message = function(msg)
    log_msg(1, msg)
end

function try(f, catch_f)
    local status, exception = pcall(f)
    if not status then
        catch_f(exception)
    end
end

function print_table(t, indent)
    indent = indent or 0
    for k, v in pairs(t) do
        if type(v) == "table" then
            print(string.rep("  ", indent) .. k .. " = {")
            print_table(v, indent + 1)
            print(string.rep("  ", indent) .. "}")
        else
            print(string.rep("  ", indent) .. k .. " = " .. tostring(v))
        end
    end
end

function safe_registry_load(level, name)
    local status, reg_table = level:load_registry(name)

    if status == false then
        log_error("error loading engine registry")
        os.exit()
    end

    return reg_table
end

function safe_menu_create(level, name, width, height)
    local menu = level:new_room(name, width, height)

    if menu == nil then
        log_error("error creating room")
        os.exit()
    end

    return menu
end

function safe_layer_create(room, registry_name, block_width, var_width)
    if block_width <= 0 or var_width < 0 or registry_name == nil or room == nil then
        log_error("invalid arguments for layer creation")
        os.exit()
    end

    local enable_vars = 0

    if var_width > 0 then
        enable_vars = 1
    end

    local layer = room:new_layer(registry_name, block_width, var_width, enable_vars)

    if layer == nil then
        log_error("error creating a floor")
        os.exit()
    end

    return layer
end

function find_block(reg_table, filename)
    print("searching for " .. filename)
    for k, v in pairs(reg_table) do
        print(v)
        if v.all_fields ~= nil then
            local file_src = v.all_fields.source_filename
            local match = string.gmatch(file_src, "/(%w+).blk$")()
            -- print_table(v.all_fields)
            -- print(filename, file_src, match)
            if match == filename then
                -- print("match!")
                return v
            end
        end
    end
end
