table.insert(event_handlers[768],
    function(sym, mod, state, rep)
        print("don press \"" .. sym .. "\" ever again")
    end
)


table.insert(event_handlers[769],
    function(sym, mod, state, rep)
        print("good...")
    end
)
