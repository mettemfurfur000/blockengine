local M = {}

function M.add(a, b)
    return {
        x = a.x + b.x,
        y = a.y + b.y
    }
end

function M.delta(dir)
    return {
        x = dir == 1 and 1 or (dir == 3 and -1 or 0),
        y = dir == 2 and 1 or (dir == 0 and -1 or 0)
    }
end

function M.direction(dx, dy)
    local magnitude = math.sqrt(dx * dx + dy * dy)
    dx = dx / magnitude
    dy = dy / magnitude

    if dx < 0 then
        return 3 -- West
    elseif dx > 0 then
        return 1 -- East
    elseif dy < 0 then
        return 0 -- North
    elseif dy > 0 then
        return 2 -- South
    end

    error("invalid direction " .. dx .. ", " .. dy)
end

function M.advance(pos, dir)
    return M.vec2d_add(pos, M.delta_direction(pos, dir))
end

function M.rotate(dir, times)
    return ((dir or 0) + times) % 4
end

return M
