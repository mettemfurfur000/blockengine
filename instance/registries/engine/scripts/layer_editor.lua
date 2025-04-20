-- UI for testing
function layer_names_get()
    local layer_names = {}
    local layer_indexes = {}

    for k, v in pairs(g_menu) do
        if v.is_ui ~= true then
            table.insert(layer_names, v.name)
            table.insert(layer_indexes, v.index)
        end
    end

    return layer_indexes, layer_names
end

function place_ui_init()
    local width, height = render_rules.get_size(g_render_rules)

    

end
