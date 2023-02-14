# Draw Pixels

[![Build Status](https://github.com/AGulev/drawpixels/workflows/Build%20with%20bob/badge.svg)](https://github.com/AGulev/drawpixels/actions)

[Defold engine](http://www.defold.com) native extension for drawing pixels and simple geometry into texture buffer.

Feel free to contribute!

>Please, pay attension!
The size of the image you manipulate with DrawPixels must match that of the atlas, not the sprite image. Otherwise you need to know where the sprite is the atlas and update that region of the atlas.

## Installation
To use this library in your Defold project, add the needed version URL to your `game.project` dependencies from [Releases](https://github.com/AGulev/drawpixels/releases)

<img width="401" alt="image" src="https://user-images.githubusercontent.com/2209596/202223571-c77f0304-5202-4314-869d-7a90bbeec5ec.png">


## Example
![screenshot](https://user-images.githubusercontent.com/2209596/45321616-7f24e100-b54e-11e8-9821-00b1b2e36a97.jpg)

Main code example is [here](https://github.com/AGulev/drawpixels/blob/master/example/canvas.script)

## Lua API
First of all you need to create a table with a buffer information that contain next fields:

`buffer` - buffer<br />
`width` - buffer width, same as your texture width<br />
`height` - buffer height, same as your texture height<br />
`channels` - 3 for rgb, 4 for rgba<br />
`premultiply_alpha` - alpha value will be premultiplied into the RGB color values. Optional parameter. Default is false.<br />

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
    channels = 4,
    premultiply_alpha = true
  }
```
Then when you have a buffer info, you can use next methods:

#### drawpixels.circle(buffer_info, pox_x, pox_y, diameter, red, green, blue, alpha, antialiasing, width)
Method for drawing circle:

`buffer_info` - buffer information<br />
`pox_x` - x position center of the circle<br />
`pox_y` - y position center of the circle<br />
`diameter` - diameter of the circle<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />
`antialiasing` - adds anti-aliasing. Only for 4 channels. Optional parameter.<br />
`width` - indicates the circle width. Only for circle with anti-aliasing. Optional parameter.<br />

#### drawpixels.filled_circle(buffer_info, pos_x, pos_y, diameter, red, green, blue, alpha, antialiasing)
Method for drawing filled circle:

`buffer_info` - buffer information<br />
`pox_x` - x position center of the circle<br />
`pox_y` - y position center of the circle<br />
`diameter` - diameter of the circle<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />
`antialiasing` - adds anti-aliasing. Only for 4 channels. Optional parameter.<br />

#### drawpixels.rect(buffer_info, pos_x, pos_y, rect_width, rect_height, red, green, blue, alpha)
Method for drawing rectangle:

`buffer_info` - buffer information<br />
`pox_x` - x position center of the rectangle<br />
`pox_y` - y position center of the rectangle<br />
`rect_width` - rectangle width<br />
`rect_height` - rectangle height<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

#### drawpixels.filled_rect(buffer_info, pos_x, pos_y, rect_width, rect_height, red, green, blue, alpha, angle)
Method for drawing filled rectangle:

`buffer_info` - buffer information<br />
`pox_x` - x position center of the rectangle<br />
`pox_y` - y position center of the rectangle<br />
`rect_width` - rectangle width<br />
`rect_height` - rectangle height<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />
`angle` - rotation angle in degrees. Optional.<br />

#### drawpixels.fill(buffer_info, red, green, blue, alpha)
Fill buffer with the color:

`buffer_info` - buffer information<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

#### drawpixels.line(buffer_info, x0, y0, x1, y1, red, green, blue, alpha, antialiasing, width)
Draw a line between two points:

`buffer_info` - buffer information<br />
`x0` - x position of one end of the line<br />
`y0` - y position of one end of the line<br />
`x1` - x position of the other end of the line<br />
`y1` - y position of the other end of the line<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />
`antialiasing` - adds anti-aliasing. Only for 4 channels. Optional parameter.<br />
`width` - indicates the line width. Only for line with anti-aliasing. Optional parameter.<br />

#### drawpixels.gradient_line(buffer_info, x0, y0, x1, y1, red1, green1, blue1, red2, green2, blue2, alpha, antialiasing, width)
Draw a gradient line between two points:

`buffer_info` - buffer information<br />
`x0` - x position of one end of the line<br />
`y0` - y position of one end of the line<br />
`x1` - x position of the other end of the line<br />
`y1` - y position of the other end of the line<br />
`red1` - first red channel of the color 0..255<br />
`green1` - first green channel of the color 0..255<br />
`blue1` - first blue channel of the color 0..255<br />
`red2` - second red channel of the color 0..255<br />
`green2` - second green channel of the color 0..255<br />
`blue2` - second blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />
`antialiasing` - adds anti-aliasing. Only for 4 channels. Optional parameter.<br />
`width` - indicates the line width. Only for line with anti-aliasing. Optional parameter.<br />

#### drawpixels.arc(buffer_info, x, y, radius, from, to, red, green, blue, alpha)
Draw a arc between two corners. If from < to the arc is drawn counterclockwise. If from > to the arc is drawn clockwise. Only for 4 channels:

`buffer_info` - buffer information<br />
`x` - x position center of the circle<br />
`y` - y position center of the circle<br />
`radius` - radius of the circle<br />
`from` - first arc angle in radians. May be negative<br />
`to` - second arc angle in radians. May be negative<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />

#### drawpixels.filled_arc(buffer_info, x, y, radius, from, to, red, green, blue, alpha)
Draw a filled arc between two corners. If from < to the arc is drawn counterclockwise. If from > to the arc is drawn clockwise. Only for 4 channels:

`buffer_info` - buffer information<br />
`x` - x position center of the circle<br />
`y` - y position center of the circle<br />
`radius` - radius of the circle<br />
`from` - first arc angle in radians. May be negative<br />
`to` - second arc angle in radians. May be negative<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />

#### drawpixels.gradient_arc(buffer_info, x, y, radius, from, to, red1, green1, blue1, red2, green2, blue2, alpha)
Draw a gradient arc between two corners. If from < to the arc is drawn counterclockwise. If from > to the arc is drawn clockwise. Only for 4 channels:

`buffer_info` - buffer information<br />
`x` - x position center of the circle<br />
`y` - y position center of the circle<br />
`radius` - radius of the circle<br />
`from` - first arc angle in radians. May be negative<br />
`to` - second arc angle in radians. May be negative<br />
`red1` - first red channel of the color 0..255<br />
`green1` - first green channel of the color 0..255<br />
`blue1` - first blue channel of the color 0..255<br />
`red2` - second red channel of the color 0..255<br />
`green2` - second green channel of the color 0..255<br />
`blue2` - second blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />

#### drawpixels.pixel(buffer_info, x, y, red, green, blue, alpha)
Draw a pixel:

`buffer_info` - buffer information<br />
`x` - x position of pixel<br />
`y` - y position of pixel<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

#### drawpixels.color(buffer_info, x, y)
Read color from a position in the buffer:

`buffer_info` - buffer information<br />
`x` - x position to get color from<br />
`y` - y position to get color from<br />

RETURNS:

`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

#### drawpixels.bezier(buffer_info, x0, y0, xc, yc, x1, y1, red, green, blue, alpha)
Draw a bezier line between two points and one control point:

`buffer_info` - buffer information<br />
`x0` - x position of the first point<br />
`y0` - y position of the first point<br />
`xc` - x position of the control point<br />
`yc` - y position of the control point<br />
`x1` - x position of the second point<br />
`y1` - y position of the second point<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255. Optional parameter for rgba textures<br />

You can fill in a specific area. To do this, you need to identify the borders with `start_fill` method and call the `fill_area` method to fill the area. Borders work with: lines, gradient lines, circles, filled circles with anti-aliasing, pixels, arcs. <b>NOT WORK WITH FILLED ARCS AND GRADIENT ARCS.</b> The arcs themselves use this method, so the fill may not be predictable. Do not create arcs until you are done with the fill. Be sure to call the `end_fill` method when you stop working with the fill.

#### drawpixels.start_fill()
Indicates the start of border preservation.

#### drawpixels.end_fill()
Indicates the stop of border preservation.

#### drawpixels.fill_area(buffer_info, x, y, red, green, blue, alpha)
Fills an area at specified boundaries. Only for 4 channels:

`buffer_info` - buffer information<br />
`x` - x position of the start point<br />
`y` - y position of the start point<br />
`red` - red channel of the color 0..255<br />
`green` - green channel of the color 0..255<br />
`blue` - blue channel of the color 0..255<br />
`alpha` - alpha channel 0..255<br />

In order to draw this into a sprite, you need a separate atlas with power of two texture. Then we can use `resource.set_texture`:

```lua
resource.set_texture(go.get("#sprite", "texture0"), self.header, self.buffer_info.buffer)
```

In order to render this in gui, we just need to create a box and create a new texture.
We can use `gui.new_texture`. Then you need to set this texture to the box using `gui.set_texture`:

```lua
local data = buffer.get_bytes(self.buffer_info.buffer, hash("rgba"))
gui.new_texture("name", width, height, image.TYPE_RGBA, data)
gui.set_texture(gui.get_node("box"), "name")
gui.set_size(gui.get_node("box"), vmath.vector3(width, height, 0))
```

--------

If you have any questions or suggestions contact me: me@agulev.com
