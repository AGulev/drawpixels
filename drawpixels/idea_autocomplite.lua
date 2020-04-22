--AUTOCOMPLETE FOR IDEA DO NOT REMOVE OR REQUIRE
---@class DRAW_PIXELS
drawpixels = {}

---Indicates border should be maintained:
function drawpixels.start_fill() end
---Stop border recording:
function drawpixels.stop_fill() end
---Fills the area considering the boundaries:
function drawpixels.fill_area(buffer_info, x,y, red, green, blue, alpha) end
---Method for drawing circle:
function drawpixels.circle(buffer_info, pox_x, pox_y, diameter, red, green, blue, alpha, antialiasing) end
---Method for drawing filled circle:
function drawpixels.filled_circle(buffer_info, pos_x, pos_y, diameter, red, green, blue, alpha, antialiasing) end
---Method for drawing rectangle:
function drawpixels.rect(buffer_info, pos_x, pos_y, rect_width, rect_height, red, green, blue, alpha) end
---Method for drawing filled rectangle:
function drawpixels.filled_rect(buffer_info, pos_x, pos_y, rect_width, rect_height, red, green, blue, alpha, angle) end
---Fill buffer with the color:
function drawpixels.fill(buffer_info, red, green, blue, alpha) end
---Draw a line between two points:
function drawpixels.line(buffer_info, x0, y0, x1, y1, red, green, blue, alpha, antialiasing, width) end
---Draw a gradient line between two points:
function drawpixels.gradient_line(buffer_info, x0, y0, x1, y1, red1, green1, blue1, red2, green2, blue2, alpha, width) end
---Draw a arc between two angles:
function drawpixels.arc(buffer_info, x, y, radius, from, to, red, green, blue, alpha) end
---Draw a filled arc between two angles:
function drawpixels.filled_arc(buffer_info, x, y, radius, from, to, red, green, blue, alpha) end
---Draw a gradient arc between two angles:
function drawpixels.gradient_arc(buffer_info, x, y, radius, from, to, red1, green1, blue1, red2, green2, blue2, alpha) end
---Draw a pixel:
function drawpixels.pixel(buffer_info, x, y, red, green, blue, alpha) end
---Read color from a position in the buffer:
function drawpixels.color(buffer_info, x, y) end
---Draw a bezier line between two points and one control point:
function drawpixels.bezier(buffer_info, x0, y0, xc, yc, x1, y1, red, green, blue, alpha) end

return
