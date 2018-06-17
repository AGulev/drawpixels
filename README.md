# Draw Pixels

[Defold engine](http://www.defold.com) native extension for drawing pixels and simple geometry into texture buffer.

Feel free to contribute!

## Installation

You can use the Draw Pixels extension in your own project by adding this project as a [Defold library dependency](http://www.defold.com/manuals/libraries/).
Open your game.project file and in the dependencies field under project add:

>https://github.com/AGulev/drawpixels/archive/master.zip

## Example
![screenshot](https://user-images.githubusercontent.com/2209596/38828895-1dc0802e-41c0-11e8-87c1-20fd65cb017f.jpg)

Main code example is [here](https://github.com/AGulev/drawpixels/blob/master/example/canvas.script)

## Lua API
First of all you need to create a table with a buffer information that contain next fields:

`buffer` - buffer

`width` - buffer width, same as your texture width

`height` - buffer height, same as your texture height

`channels` - 3 for rgb, 4 for rgba

For example:
```lua
local buffer_info = {
    buffer = buffer.create(1024 * 2048, -- size of the buffer width*height
    {{
      name = hash("my_buffer"),
      type = buffer.VALUE_TYPE_UINT8,
      count = 4 -- same as channels
    }}),
    width = 1024,
    height = 2048,
    channels = 4
  }
```
Then when you have a buffer info, you can use next methods:

#### drawpixels.circle(buffer_info, pox_x, pox_y, diameter, red, green, blue, alpha)
Method for drawing circle:

`buffer_info` - buffer information

`pox_x` - x position center of the circle

`pox_y` - y position center of the circle

`diameter` - diameter of the circle

`red` - red channel of the color 0..255

`green` - green channel of the color 0..255

`blue` - blue channel of the color 0..255

`alpha` - alpha channel 0..255. Optional parameter for rgba textures

#### drawpixels.filled_circle(buffer_info, pos_x, pos_y, diameter, red, green, blue, alpha)
Method for drawing filled circle:

`buffer_info` - buffer information

`pox_x` - x position center of the circle

`pox_y` - y position center of the circle

`diameter` - diameter of the circle

`red` - red channel of the color 0..255

`green` - green channel of the color 0..255

`blue` - blue channel of the color 0..255

`alpha` - alpha channel 0..255. Optional parameter for rgba textures

#### drawpixels.rect(buffer_info, pos_x, pos_y, rect_width, rect_height, red, green, blue, alpha)
Method for drawing rectangle:

`buffer_info` - buffer information

`pox_x` - x position center of the rectangle

`pox_y` - y position center of the rectangle

`rect_width` - rectangle width

`rect_height` - rectangle height

`red` - red channel of the color 0..255

`green` - green channel of the color 0..255

`blue` - blue channel of the color 0..255

`alpha` - alpha channel 0..255. Optional parameter for rgba textures

#### drawpixels.filled_rect(buffer_info, pos_x, pos_y, rect_width, rect_height, red, green, blue, alpha, angle)
Method for drawing filled rectangle:

`buffer_info` - buffer information

`pox_x` - x position center of the rectangle

`pox_y` - y position center of the rectangle

`rect_width` - rectangle width

`rect_height` - rectangle height

`red` - red channel of the color 0..255

`green` - green channel of the color 0..255

`blue` - blue channel of the color 0..255

`alpha` - alpha channel 0..255. Optional parameter for rgba textures

`angle` - rotation angle in degrees. Optional.

#### drawpixels.fill(buffer_info, red, green, blue, alpha)
Fill buffer with the color:

`buffer_info` - buffer information

`red` - red channel of the color 0..255

`green` - green channel of the color 0..255

`blue` - blue channel of the color 0..255

`alpha` - alpha channel 0..255. Optional parameter for rgba textures

#### drawpixels.line(buffer_info, x0, y0, x1, y1, red, green, blue, alpha)
Draw a line between two points:

`buffer_info` - buffer information

`x0` - x position of one end of the line

`y0` - y position of one end of the line

`x1` - x position of the other end of the line

`y1` - y position of the other end of the line

`red` - red channel of the color 0..255

`green` - green channel of the color 0..255

`blue` - blue channel of the color 0..255

`alpha` - alpha channel 0..255. Optional parameter for rgba textures

#### drawpixels.pixel(buffer_info, x, y, red, green, blue, alpha)
Draw a pixel:

`buffer_info` - buffer information

`x` - x position of pixel

`y` - y position of pixel

`red` - red channel of the color 0..255

`green` - green channel of the color 0..255

`blue` - blue channel of the color 0..255

`alpha` - alpha channel 0..255. Optional parameter for rgba textures

#### drawpixels.color(buffer_info, x, y)
Read color from a position in the buffer:

`buffer_info` - buffer information

`x` - x position to get color from

`y` - y position to get color from

RETURNS:

`red` - red channel of the color 0..255

`green` - green channel of the color 0..255

`blue` - blue channel of the color 0..255

`alpha` - alpha channel 0..255. Optional parameter for rgba textures

--------

If you have any questions or suggestions contact me: me@agulev.com
