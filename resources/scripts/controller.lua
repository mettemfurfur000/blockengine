function find_block(range_x, range_y, id_to_find)
    for y = 0, range_y do
        for x = 0, range_x do
            local b = blockengine.access_block(g_world, 1, x, y)
            if b == nil then goto continue end

            local id, datalen = blockengine.block_unpack(b)

            if id == id_to_find then return x, y, b end

            ::continue::
        end
    end
end

table.insert(event_handlers[768],
    function(sym, mod, state, rep)
        if rep < 0 then return end

        local player_x, player_y, player = find_block(64, 64, 5)

        if sym == 119 or sym == 97 or sym == 115 or sym == 100 then
            local vx = 0
            local vy = 0

            if sym == 119 then vy = -1 end -- w
            if sym == 115 then vy = 1 end  -- s

            if sym == 97 then vx = -1 end  -- a
            if sym == 100 then vx = 1 end  -- d

            print(player_x, player_y, vx, vy)

            print(g_world)

            print(player)

            if blockengine.move_block_rough(g_world, 1, player_x, player_y, vx, vy) then
                player_x = player_x + vx
                player_y = player_y + vy
            end
        end
    end
)


table.insert(event_handlers[769],
    function(sym, mod, state, rep)
        --print("good...")
    end
)
