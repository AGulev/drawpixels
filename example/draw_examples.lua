local M = {}

local function draw_cross(buffer_info, x, y, r, g, b, a)
    drawpixels.pixel(buffer_info, x, y, r, g, b, a)
    drawpixels.pixel(buffer_info, x - 2, y, r, g, b, a)
    drawpixels.pixel(buffer_info, x + 2, y, r, g, b, a)
    drawpixels.pixel(buffer_info, x, y - 2, r, g, b, a)
    drawpixels.pixel(buffer_info, x, y + 2, r, g, b, a)
end

local function draw_fill_area(buffer_info, x, y)
    local w = 120
    local h = 80
    local left = x - w / 2
    local right = x + w / 2
    local top = y + h / 2
    local bottom = y - h / 2

    drawpixels.start_fill()
    drawpixels.line(buffer_info, left, bottom, right, bottom, 0, 0, 0, 255, true, 3)
    drawpixels.line(buffer_info, right, bottom, right, top, 0, 0, 0, 255, true, 3)
    drawpixels.line(buffer_info, right, top, left, top, 0, 0, 0, 255, true, 3)
    drawpixels.line(buffer_info, left, top, left, bottom, 0, 0, 0, 255, true, 3)
    drawpixels.fill_area(buffer_info, x, y, 0, 150, 60, 255)
    drawpixels.end_fill()
end

local function positions(buffer_info)
    local w = buffer_info.width
    local h = buffer_info.height
    local visible_w = 512
    local visible_h = 1024
    -- The 1024x2048 texture is centered at (512,1024), so the visible window
    -- maps to the bottom-left quarter of the buffer.
    local visible_left = 0
    local visible_right = visible_left + visible_w
    local visible_bottom = 0
    local visible_top = visible_bottom + visible_h
    local margin_top = 64
    local margin_bottom = 200
    local start_y = visible_top - margin_top
    local row_gap = 120
    local col_gap = visible_w / 2
    local x1 = visible_left + col_gap / 2
    local x2 = x1 + col_gap

    local slots = {}
    local y = start_y
    while y > visible_bottom + margin_bottom do
        slots[#slots + 1] = { x = x1, y = y }
        slots[#slots + 1] = { x = x2, y = y }
        y = y - row_gap
    end

    return {
        slots = slots,
        height = h,
        visible_left = visible_left,
        visible_right = visible_right,
        visible_bottom = visible_bottom,
        visible_top = visible_top,
    }
end

local steps = {
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[1]
        draw_cross(buffer_info, s.x, s.y, 0, 0, 0, 255)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[2]
        drawpixels.line(buffer_info, s.x - 40, s.y - 40, s.x + 40, s.y + 40, 0, 0, 255, 255, true, 3)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[3]
        drawpixels.gradient_line(buffer_info, s.x - 50, s.y, s.x + 50, s.y, 255, 0, 0, 0, 0, 255, 255, true, 3)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[4]
        drawpixels.circle(buffer_info, s.x, s.y, 90, 0, 0, 0, 255)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[5]
        drawpixels.filled_circle(buffer_info, s.x, s.y, 90, 0, 160, 0, 255)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[6]
        drawpixels.rect(buffer_info, s.x, s.y, 90, 60, 0, 0, 0, 255)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[7]
        drawpixels.filled_rect(buffer_info, s.x, s.y, 90, 60, 200, 120, 0, 255)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[8]
        drawpixels.filled_rect(buffer_info, s.x, s.y, 90, 60, 0, 120, 200, 255, 25)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[9]
        local arc_start = 10 / 6 * math.pi
        local arc_finish = 5 / 2 * math.pi
        drawpixels.arc(buffer_info, s.x, s.y, 55, arc_start, arc_finish, 0, 0, 0, 255)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[10]
        local arc_start = 10 / 6 * math.pi
        local arc_finish = 5 / 2 * math.pi
        drawpixels.filled_arc(buffer_info, s.x, s.y, 55, arc_start, arc_finish, 0, 0, 255, 255)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[11]
        local arc_start = 10 / 6 * math.pi
        local arc_finish = 5 / 2 * math.pi
        drawpixels.gradient_arc(buffer_info, s.x, s.y, 70, arc_start, arc_finish, 255, 0, 0, 0, 0, 255, 255)
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[13]
        if s then
            draw_fill_area(buffer_info, s.x, s.y)
        end
    end,
    function(buffer_info)
        local p = positions(buffer_info)
        local s = p.slots[12]
        drawpixels.bezier(buffer_info, s.x - 40, s.y - 30, s.x, s.y + 40, s.x + 40, s.y - 30, 120, 0, 200, 255)
    end,
}

function M.count()
    return #steps
end

function M.draw_step(buffer_info, index)
    if index == 1 then
        drawpixels.fill(buffer_info, 0, 0, 0, 0)
    end
    local step = steps[index]
    if step then
        step(buffer_info)
    end
end

return M
