local M = {}

M.up = 0
M.right = 1
M.down = 2
M.left = 3

function M.new(x, y)
    return {
        x = x,
        y = y
    }
end

function M.add(a, b)
    return {
        x = a.x + b.x,
        y = a.y + b.y
    }
end

function M.sub(a, b)
    return {
        x = a.x - b.x,
        y = a.y - b.y
    }
end

function M.mult(a, numbuh)
    return {
        x = a.x * numbuh,
        y = a.y * numbuh
    }
end

function M.delta(dir)
    if dir == M.up then return { x = 0, y = -1 } end
    if dir == M.down then return { x = 0, y = 1 } end
    if dir == M.right then return { x = 1, y = 0 } end
    if dir == M.left then return { x = -1, y = 0 } end
end

function M.direction(dx, dy)
    local absX = math.abs(dx);
    local absY = math.abs(dy);

    if absX > absY then
        return dx > 0 and M.right or M.left;
    else
        return dy > 0 and M.down or M.up;
    end
end

function M.rotate(dir, times)
    return ((dir or 0) + times) % 4
end

return M
