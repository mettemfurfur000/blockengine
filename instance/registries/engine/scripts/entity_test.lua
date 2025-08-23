-- -- Simple test script demonstrating Entity Lua API
-- -- Note: run this from the engine's scripting environment

-- -- create a level and a room
-- local lvl = le.create_level("testlvl")
-- local room = lvl:new_room("r1", 16, 16)

-- -- create a simple layer with solid blocks to form something for physics (skipped here)
-- -- assume room has at least one layer populated; in real test we'd fill a layer

-- -- create a full entity using the room as both body and parent
-- local h = entity.create_full("ent1", room, 0, 0)
-- local ent = entity.wrap(h)

-- print("entity valid:", ent:is_valid())
-- print("entity uuid:", ent:uuid())
-- print("entity name:", ent:name())

-- local x,y = ent:get_position()
-- print("initial pos:", x, y)

-- ent:apply_force(10, 0)

-- -- step physics a few times via room:step_physics
-- for i=1,10 do
--   room:step_physics(1/60)
-- end

-- local vx,vy = ent:get_velocity()
-- print("velocity after force:", vx, vy)

-- local nx,ny = ent:get_position()
-- print("new pos:", nx, ny)

-- -- cleanup
-- ent:release()

-- print("test finished")
