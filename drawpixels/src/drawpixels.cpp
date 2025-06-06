// Extension lib defines
#define LIB_NAME "DrawPixels"
#define MODULE_NAME "drawpixels"
#ifndef DLIB_LOG_DOMAIN
#define DLIB_LOG_DOMAIN LIB_NAME
#endif
#define M_PI 3.14159265358979323846

#include <dmsdk/sdk.h>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <string>
#include <stack>

struct BufferInfo
{
    dmBuffer::HBuffer buffer;
    int width;
    int height;
    int channels;
    bool premultiply_alpha;
    uint8_t *bytes;
    uint32_t src_size;
};

BufferInfo buffer_info;

struct Point
{
    int x;
    int y;
};

struct Color
{
    int r;
    int g;
    int b;
};

static float PI_2 = M_PI * 2;
static bool is_record_point = false;
static int *points = nullptr;

static void clear_point()
{
    if (points != nullptr)
    {
        delete[] points;
        points = nullptr;
    }
}

static void start_record_points()
{
    clear_point();
    is_record_point = true;
    points = new int[buffer_info.width * buffer_info.height];
    for (int i = 0; i < buffer_info.width * buffer_info.height; i++)
    {
        points[i] = 0;
    }
}

static void stop_record_points()
{
    is_record_point = false;
    clear_point();
}

static int in_buffer(int x, int y)
{
    return (x >= 0) && (y >= 0) && (x < buffer_info.width) && (y < buffer_info.height);
}

static int xytoi(int x, int y)
{
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x >= buffer_info.width)
        x = buffer_info.width - 1;
    if (y >= buffer_info.height)
        y = buffer_info.height - 1;
    return (y * buffer_info.width * buffer_info.channels) + (x * buffer_info.channels);
}

static void recordtobuffer(int i, float v)
{
    buffer_info.bytes[i] = v > 1 ? 255 : v * 255.0;
}

static float getmixrgb(float dc, float da, float sc, float sa, float oa)
{
    if (oa == 0)
    {
        return 0.0;
    }
    return (sc * sa + dc * da * (1 - sa)) / oa;
}

static void putpixel(int x, int y, int r, int g, int b, int a)
{
    // ignore the pixel if it's outside the area of the buffer
    if (!in_buffer(x, y))
    {
        return;
    }
    int i = xytoi(x, y);
    buffer_info.bytes[i] = r;
    buffer_info.bytes[i + 1] = g;
    buffer_info.bytes[i + 2] = b;
    if (buffer_info.channels == 4)
    {
        buffer_info.bytes[i + 3] = a;
    }
}

static void record_mix_pixel(int i, float r, float g, float b, float a)
{
    float ba = buffer_info.bytes[i + 3] / 255.0;
    a = a / 255.0;
    float oa = a + ba * (1 - a);
    r = getmixrgb(buffer_info.bytes[i] / 255.0, ba, r / 255.0, a, oa);
    g = getmixrgb(buffer_info.bytes[i + 1] / 255.0, ba, g / 255.0, a, oa);
    b = getmixrgb(buffer_info.bytes[i + 2] / 255.0, ba, b / 255.0, a, oa);
    recordtobuffer(i, r);
    recordtobuffer(i + 1, g);
    recordtobuffer(i + 2, b);
    recordtobuffer(i + 3, oa);
}

static void record_premul_alpha_pixel(int i, unsigned short r, unsigned short g, unsigned short b, unsigned short a)
{
    buffer_info.bytes[i] = r * a >> 8;
    buffer_info.bytes[i + 1] = g * a >> 8;
    buffer_info.bytes[i + 2] = b * a >> 8;
    buffer_info.bytes[i + 3] = a;
}

static void add_point(int x, int y)
{
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x >= buffer_info.width)
        x = buffer_info.width - 1;
    if (y >= buffer_info.height)
        y = buffer_info.height - 1;
    points[y * buffer_info.width + x] = 1;
}

static void record_pixel(int i, int r, int g, int b, int a)
{
    buffer_info.bytes[i] = r;
    buffer_info.bytes[i + 1] = g;
    buffer_info.bytes[i + 2] = b;
    if (buffer_info.channels == 4)
    {
        buffer_info.bytes[i + 3] = a;
    }
}

static void mixpixel(int x, int y, float r, float g, float b, float a)
{
    if (!in_buffer(x, y))
    {
        return;
    }
    if (is_record_point && a != 0)
    {
        add_point(x, y);
    }

    int i = xytoi(x, y);
    if (buffer_info.channels == 3 || a == 255 || (!buffer_info.premultiply_alpha && buffer_info.bytes[i + 3] == 0))
    {
        record_pixel(i, r, g, b, a);
        return;
    }
    if (a == 0)
    {
        return;
    }
    if (buffer_info.premultiply_alpha && buffer_info.bytes[i + 3] == 0)
    {
        record_premul_alpha_pixel(i, r, g, b, a);
    }

    record_mix_pixel(i, r, g, b, a);
}

static void fill_mixed_line(int fromx, int tox, int y, int r, int g, int b, int a)
{
    if (fromx > tox)
    {
        int temp = fromx;
        fromx = tox;
        tox = temp;
    }
    fromx = fmax(0, fromx);
    tox = fmin(buffer_info.width - 1, tox);
    int start = xytoi(fromx, y);
    int end = xytoi(tox, y);
    for (int i = start; i < end; i += 4)
    {
        record_mix_pixel(i, r, g, b, a);
    }
}

static Color lerp_color(Point center, Point pixel, float fd_2, Color c1, Color c2)
{
    Color nc;
    float coef = (float)((center.x - pixel.x) * (center.x - pixel.x) + (center.y - pixel.y) * (center.y - pixel.y)) / fd_2;
    if (coef <= 0)
    {
        return c1;
    }
    if (coef >= 1)
    {
        return c2;
    }
    nc.r = c1.r + (c2.r - c1.r) * coef;
    nc.g = c1.g + (c2.g - c1.g) * coef;
    nc.b = c1.b + (c2.b - c1.b) * coef;
    return nc;
}

static void lerp_pixel(Point center, Point pixel, int fd_2, Color c1, Color c2, int a)
{
    Color nc = lerp_color(center, pixel, fd_2, c1, c2);
    mixpixel(pixel.x, pixel.y, nc.r, nc.g, nc.b, a);
}

static void fill_line(int fromx, int tox, int y, int r, int g, int b, int a)
{
    if (fromx > tox)
    {
        int temp = fromx;
        fromx = tox;
        tox = temp;
    }
    fromx = fmax(0, fromx);
    tox = fmin(buffer_info.width - 1, tox);
    int size = 10;
    //prepare line for memcpy
    int line_size = 10 * buffer_info.channels;
    uint8_t *line = new uint8_t[line_size];
    for (int i = 0; i < line_size; i += buffer_info.channels)
    {
        line[i] = r;
        line[i + 1] = g;
        line[i + 2] = b;
        if (buffer_info.channels == 4)
        {
            line[i + 3] = a;
        }
    }
    int start = xytoi(fromx, y);
    int end = xytoi(tox, y);
    int width = (end - start + buffer_info.channels);
    for (int i = start; i < end; i += line_size)
    {
        if (width >= line_size)
        {
            memcpy(&buffer_info.bytes[i], line, line_size);
        }
        else
        {
            memcpy(&buffer_info.bytes[i], line, width);
        }
        width = width - line_size;
    }
    delete[] line;
}

static bool is_contain(int x, int y)
{
    return points[y * buffer_info.width + x] == 1;
}

static bool is_new(int i, int r, int g, int b)
{
    return buffer_info.bytes[i] == r && buffer_info.bytes[i + 1] == g && buffer_info.bytes[i + 2] == b;
}

static void find_seed_pixel(std::stack<Point> &top, Point pixel, int x_right, int r, int g, int b, int a)
{
    while (pixel.x <= x_right)
    {
        int flag = 0;
        while (pixel.x < x_right && !is_contain(pixel.x, pixel.y))
        {
            if (flag == 0)
            {
                flag = 1;
            }
            pixel.x += 1;
        }
        if (flag == 1)
        {
            if (pixel.x == x_right && !is_contain(pixel.x, pixel.y))
            {
                Point new_pixel;
                new_pixel.x = pixel.x;
                new_pixel.y = pixel.y;
                if (in_buffer(new_pixel.x, new_pixel.y))
                {
                    top.push(new_pixel);
                }
            }
            else
            {
                Point new_pixel;
                new_pixel.x = pixel.x - 1;
                new_pixel.y = pixel.y;
                if (in_buffer(new_pixel.x, new_pixel.y))
                {
                    top.push(new_pixel);
                }
            }
            flag = 0;
        }

        int x_in = pixel.x;
        while (pixel.x < x_right && is_contain(pixel.x, pixel.y))
        {
            mixpixel(pixel.x, pixel.y, r, g, b, a);
            pixel.x += 1;
        }
        if (pixel.x == x_in)
        {
            pixel.x += 1;
        }
    }
}

// Seed-by-line filling algorithm
static void fill_area(int x, int y, int r, int g, int b, int a)
{
    if (!in_buffer(x, y))
    {
        return;
    }
    Point _pixel;
    _pixel.x = x;
    _pixel.y = y;
    std::stack<Point> top;
    top.push(_pixel);

    while (!top.empty())
    {
        Point pixel = top.top();
        top.pop();
        mixpixel(pixel.x, pixel.y, r, g, b, a);
        int temp_x = pixel.x;
        pixel.x += 1;
        while (pixel.x < buffer_info.width - 1 && !is_contain(pixel.x, pixel.y))
        {
            mixpixel(pixel.x, pixel.y, r, g, b, a);
            pixel.x += 1;
        }
        mixpixel(pixel.x, pixel.y, r, g, b, a);
        int x_right = pixel.x >= buffer_info.width ? buffer_info.width - 1 : pixel.x;
        pixel.x = temp_x;
        pixel.x -= 1;
        while (pixel.x >= 1 && !is_contain(pixel.x, pixel.y))
        {
            mixpixel(pixel.x, pixel.y, r, g, b, a);
            pixel.x -= 1;
        }
        mixpixel(pixel.x, pixel.y, r, g, b, a);
        pixel.x++;
        int x_left = pixel.x;
        pixel.y += 1;
        if (pixel.y >= buffer_info.height)
        {
            continue;
        }
        find_seed_pixel(top, pixel, x_right, r, g, b, a);
        pixel.y -= 2;
        if (pixel.y < 0)
        {
            continue;
        }
        pixel.x = x_left;
        find_seed_pixel(top, pixel, x_right, r, g, b, a);
    }
}

static void gradient_find_seed_pixel(std::stack<Point> &top, Point pixel, int x_right, Point center, int fd_2, Color c1, Color c2, int a)
{
    while (pixel.x <= x_right)
    {
        int flag = 0;
        while (pixel.x < x_right && !is_contain(pixel.x, pixel.y))
        {
            if (flag == 0)
            {
                flag = 1;
            }
            pixel.x += 1;
        }
        if (flag == 1)
        {
            if (pixel.x == x_right && !is_contain(pixel.x, pixel.y))
            {
                Point new_pixel;
                new_pixel.x = pixel.x;
                new_pixel.y = pixel.y;
                if (in_buffer(new_pixel.x, new_pixel.y))
                {
                    top.push(new_pixel);
                }
            }
            else
            {
                Point new_pixel;
                new_pixel.x = pixel.x - 1;
                new_pixel.y = pixel.y;
                if (in_buffer(new_pixel.x, new_pixel.y))
                {
                    top.push(new_pixel);
                }
            }
            flag = 0;
        }

        int x_in = pixel.x;
        while (pixel.x < x_right && is_contain(pixel.x, pixel.y))
        {
            lerp_pixel(center, pixel, fd_2, c1, c2, a);
            pixel.x += 1;
        }
        if (pixel.x == x_in)
        {
            pixel.x += 1;
        }
    }
}

static void gradient_fill_area(int x, int y, Point center, int fd_2, Color c1, Color c2, int a)
{
    if (!in_buffer(x, y))
    {
        return;
    }
    Point _pixel;
    _pixel.x = x;
    _pixel.y = y;
    std::stack<Point> top;
    top.push(_pixel);

    while (!top.empty())
    {
        Point pixel = top.top();
        top.pop();
        lerp_pixel(center, pixel, fd_2, c1, c2, a);
        int temp_x = pixel.x;
        pixel.x += 1;
        while (pixel.x < buffer_info.width - 1 && !is_contain(pixel.x, pixel.y))
        {
            lerp_pixel(center, pixel, fd_2, c1, c2, a);
            pixel.x += 1;
        }
        lerp_pixel(center, pixel, fd_2, c1, c2, a);
        int x_right = pixel.x >= buffer_info.width ? buffer_info.width - 1 : pixel.x;
        pixel.x = temp_x;
        pixel.x -= 1;
        while (pixel.x >= 1 && !is_contain(pixel.x, pixel.y))
        {
            lerp_pixel(center, pixel, fd_2, c1, c2, a);
            pixel.x -= 1;
        }
        lerp_pixel(center, pixel, fd_2, c1, c2, a);
        pixel.x++;
        int x_left = pixel.x;
        pixel.y += 1;
        if (pixel.y >= buffer_info.height)
        {
            continue;
        }
        gradient_find_seed_pixel(top, pixel, x_right, center, fd_2, c1, c2, a);
        pixel.y -= 2;
        if (pixel.y < 0)
        {
            continue;
        }
        pixel.x = x_left;
        gradient_find_seed_pixel(top, pixel, x_right, center, fd_2, c1, c2, a);
    }
}

//http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
static void fill_bottom_flat_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a)
{
    float invslope1 = float(x2 - x1) / float(y2 - y1);
    float invslope2 = float(x3 - x1) / float(y3 - y1);
    float curx1 = x1;
    float curx2 = x1;
    for (int scanlineY = y1; scanlineY <= y2; scanlineY++)
    {
        fill_line(curx1, curx2, scanlineY, r, g, b, a);
        curx1 += invslope1;
        curx2 += invslope2;
    }
}

static void fill_top_flat_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a)
{
    float invslope1 = float(x3 - x1) / float(y3 - y1);
    float invslope2 = float(x3 - x2) / float(y3 - y2);
    float curx1 = x3;
    float curx2 = x3;
    for (int scanlineY = y3; scanlineY > y1; scanlineY--)
    {
        fill_line(curx1, curx2, scanlineY, r, g, b, a);
        curx1 -= invslope1;
        curx2 -= invslope2;
    }
}

static void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a)
{
    /* values should be sorted by y */
    /* here we know that y1 <= y2 <= y3 */
    /* check for trivial case of bottom-flat triangle */
    if (y2 == y3)
    {
        fill_bottom_flat_triangle(x1, y1, x2, y2, x3, y3, r, g, b, a);
    }
    /* check for trivial case of top-flat triangle */
    else if (y1 == y2)
    {
        fill_top_flat_triangle(x1, y1, x2, y2, x3, y3, r, g, b, a);
    }
    else
    {
        /* general case - split the triangle in a topflat and bottom-flat one */
        int x4 = x1 + ((float(y2 - y1) / float(y3 - y1)) * (x3 - x1));
        int y4 = y2;
        fill_bottom_flat_triangle(x1, y1, x2, y2, x4, y4, r, g, b, a);
        fill_top_flat_triangle(x2, y2, x4, y4, x3, y3, r, g, b, a);
    }
}

static void read_and_validate_buffer_info(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    lua_getfield(L, index, "buffer");
    lua_getfield(L, index, "width");
    lua_getfield(L, index, "height");
    lua_getfield(L, index, "channels");
    lua_getfield(L, index, "premultiply_alpha");
    dmScript::LuaHBuffer *lua_buffer = dmScript::CheckBuffer(L, -5);
    buffer_info.buffer = lua_buffer->m_Buffer;

    if (!dmBuffer::IsBufferValid(buffer_info.buffer))
    {
        luaL_error(L, "Buffer is invalid");
    }

    dmBuffer::Result result = dmBuffer::GetBytes(buffer_info.buffer, (void **)&buffer_info.bytes, &buffer_info.src_size);
    if (result != dmBuffer::RESULT_OK)
    {
        luaL_error(L, "Buffer is invalid");
    }

    buffer_info.width = lua_tointeger(L, -4);
    if (buffer_info.width == 0)
    {
        luaL_error(L, "'width' of the buffer should be an integer and > 0");
    }

    buffer_info.height = lua_tointeger(L, -3);
    if (buffer_info.height == 0)
    {
        luaL_error(L, "'height' of the buffer should be an integer and > 0");
    }

    buffer_info.channels = lua_tointeger(L, -2);
    if (buffer_info.channels == 0)
    {
        luaL_error(L, "'channels' of should be an integer and 3 or 4");
    }

    if (lua_isboolean(L, -1) == 1)
    {
        buffer_info.premultiply_alpha = lua_toboolean(L, -1);
    }
    else
    {
        buffer_info.premultiply_alpha = false;
    }
}

static void draw_line(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
    // https://gist.github.com/bert/1085538#file-plot_line-c
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

    for (;;)
    { /* loop */
        mixpixel(x0, y0, r, g, b, a);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        } /* e_xy+e_x > 0 */
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        } /* e_xy+e_y < 0 */
    }
}

static void draw_gradient_line(int x0, int y0, int x1, int y1, Color c1, Color c2, int a)
{
    // https://gist.github.com/bert/1085538#file-plot_line-c
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */
    int fd_2 = (x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1);

    Point center;
    center.x = x0;
    center.y = y0;
    Point pixel;

    for (;;)
    { /* loop */
        pixel.x = x0;
        pixel.y = y0;
        lerp_pixel(center, pixel, fd_2, c1, c2, a);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        } /* e_xy+e_x > 0 */
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        } /* e_xy+e_y < 0 */
    }
}

static int IPart(float x)
{
    return (int)x;
}

static float FPart(float x)
{
    while (x >= 0)
        x--;
    x++;
    return x;
}

// http://grafika.me/node/38
static void draw_line_vu(int x0, int y0, int x1, int y1, int r, int g, int b, int a, float w)
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int w_min = ceilf(w / 2) - 1;
    int w_max = w / 2 + 1;
    if (dx == 0)
    {
        for (int i = -w_min; i < w_max; i++)
        {
            draw_line(x0 + i, y0, x1 + i, y1, r, g, b, a);
        }
        return;
    }
    if (dy == 0)
    {
        for (int i = -w_min; i < w_max; i++)
        {
            draw_line(x0, y0 + i, x1, y1 + i, r, g, b, a);
        }
        return;
    }
    float na = (float)a / 255;

    if (dy < dx)
    {
        if (x1 < x0)
        {
            x1 += x0;
            x0 = x1 - x0;
            x1 -= x0;
            y1 += y0;
            y0 = y1 - y0;
            y1 -= y0;
        }
        float grad = (float)dy / dx;
        if (y1 < y0)
            grad = -grad;
        float intery = y0 + grad;
        for (int i = -w_min; i < w_max; i++)
        {
            mixpixel(x0, y0 + i, r, g, b, a);
        }
        float intens = 0;
        int ipart = 0;
        for (int x = x0 + 1; x < x1; x++)
        {
            intens = FPart(intery) * 255;
            ipart = IPart(intery);
            mixpixel(x, ipart - w_min, r, g, b, (255 - intens) * na);
            for (int i = -w_min + 1; i < w_max; i++)
            {
                mixpixel(x, ipart + i, r, g, b, a);
            }
            mixpixel(x, ipart + w_max, r, g, b, intens * na);
            intery += grad;
        }
        for (int i = -w_min; i < w_max; i++)
        {
            mixpixel(x1, y1 + i, r, g, b, a);
        }
    }
    else
    {
        if (y1 < y0)
        {
            x1 += x0;
            x0 = x1 - x0;
            x1 -= x0;
            y1 += y0;
            y0 = y1 - y0;
            y1 -= y0;
        }
        float grad = (float)dx / dy;
        if (x1 < x0)
            grad = -grad;
        float interx = x0 + grad;
        for (int i = -w_min; i < w_max; i++)
        {
            mixpixel(x0 + i, y0, r, g, b, a);
        }
        float intens = 0;
        int ipart = 0;
        for (int y = y0 + 1; y < y1; y++)
        {
            intens = FPart(interx) * 255;
            ipart = IPart(interx);
            mixpixel(ipart - w_min, y, r, g, b, (255 - intens) * na);
            for (int i = -w_min + 1; i < w_max; i++)
            {
                mixpixel(ipart + i, y, r, g, b, a);
            }
            mixpixel(ipart + w_max, y, r, g, b, intens * na);
            interx += grad;
        }
        for (int i = -w_min; i < w_max; i++)
        {
            mixpixel(x1 + i, y1, r, g, b, a);
        }
    }
}

static void draw_gradient_line_vu(int x0, int y0, int x1, int y1, Color c1, Color c2, int a, float w)
{
    Point center;
    center.x = x0;
    center.y = y0;
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int fd_2 = (x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1);
    int w_min = ceilf(w / 2) - 1;
    int w_max = w / 2 + 1;
    if (dx == 0)
    {
        for (int i = -w_min; i < w_max; i++)
        {
            draw_gradient_line(x0 + i, y0, x1 + i, y1, c1, c2, a);
        }
        return;
    }
    if (dy == 0)
    {
        for (int i = -w_min; i < w_max; i++)
        {
            draw_gradient_line(x0, y0 + i, x1, y1 + i, c1, c2, a);
        }
        return;
    }
    float na = (float)a / 255;
    if (dy < dx)
    {
        if (x1 < x0)
        {
            x1 += x0;
            x0 = x1 - x0;
            x1 -= x0;
            y1 += y0;
            y0 = y1 - y0;
            y1 -= y0;
        }
        float grad = (float)dy / dx;
        if (y1 < y0)
            grad = -grad;
        float intery = y0 + grad;
        for (int i = -w_min; i < w_max; i++)
        {
            mixpixel(x0, y0 + i, c1.r, c1.g, c1.b, a);
        }
        Point pixel;
        float intens = 0;
        int ipart = 0;
        for (int x = x0 + 1; x < x1; x++)
        {
            intens = FPart(intery) * 255;
            ipart = IPart(intery);
            pixel.x = x;
            pixel.y = ipart - w_min;

            lerp_pixel(center, pixel, fd_2, c1, c2, (255 - intens) * na);
            for (int i = -w_min + 1; i < w_max; i++)
            {
                pixel.y = ipart + i;
                lerp_pixel(center, pixel, fd_2, c1, c2, a);
            }
            pixel.y = ipart + w_max;
            lerp_pixel(center, pixel, fd_2, c1, c2, intens * na);
            intery += grad;
        }
        for (int i = -w_min; i < w_max; i++)
        {
            mixpixel(x1, y1 + i, c2.r, c2.g, c2.b, a);
        }
    }
    else
    {
        if (y1 < y0)
        {
            x1 += x0;
            x0 = x1 - x0;
            x1 -= x0;
            y1 += y0;
            y0 = y1 - y0;
            y1 -= y0;
        }
        float grad = (float)dx / dy;
        if (x1 < x0)
            grad = -grad;
        float interx = x0 + grad;
        for (int i = -w_min; i < w_max; i++)
        {
            mixpixel(x0 + i, y0, c1.r, c1.g, c1.b, a);
        }
        Point pixel;
        float intens = 0;
        int ipart = 0;
        for (int y = y0 + 1; y < y1; y++)
        {
            intens = FPart(interx) * 255;
            ipart = IPart(interx);
            pixel.x = ipart - w_min;
            pixel.y = y;

            lerp_pixel(center, pixel, fd_2, c1, c2, (255 - intens) * na);
            for (int i = -w_min + 1; i < w_max; i++)
            {
                pixel.x = ipart + i;
                lerp_pixel(center, pixel, fd_2, c1, c2, a);
            }
            pixel.x = ipart + w_max;
            lerp_pixel(center, pixel, fd_2, c1, c2, intens * na);
            interx += grad;
        }
        for (int i = -w_min; i < w_max; i++)
        {
            mixpixel(x1 + i, y1, c2.r, c2.g, c2.b, a);
        }
    }
}

// modified http://www.opita.net/node/699
static void draw_circle_vu(int _x, int _y, int radius, int r, int g, int b, int a, float w)
{
    if (radius < 1)
    {
        return;
    }
    int w_min = ceilf(w / 2) - 1;
    int w_max = w / 2 + 1;
    float iy = 0;
    int intens = 0;
    int inv_intens = 0;
    int iiy = 0;
    int cx = 0;
    int cy = 0;
    for (int x = 0; x <= (radius + w) * cos(M_PI / 4); x++)
    {
        iy = (float)sqrt(radius * radius - x * x);
        intens = (int)((FPart(iy) * 255.0) * a / 255);
        inv_intens = (255 - (int)(FPart(iy) * 255)) * a / 255;
        iiy = IPart(iy);
        cx = _x - x;
        cy = _y + iiy;
        mixpixel(cx, cy - w_min, r, g, b, inv_intens);
        mixpixel(cx, cy + w_max, r, g, b, intens);
        cy = _y - iiy - 1;
        mixpixel(cx, cy - w_min, r, g, b, intens);
        mixpixel(cx, cy + w_max, r, g, b, inv_intens);
        cx = _x + x;
        cy = _y + iiy;
        mixpixel(cx, cy - w_min, r, g, b, inv_intens);
        mixpixel(cx, cy + w_max, r, g, b, intens);
        cy = _y - iiy - 1;
        mixpixel(cx, cy - w_min, r, g, b, intens);
        mixpixel(cx, cy + w_max, r, g, b, inv_intens);

        cx = _x + iiy;
        cy = _y + x;
        mixpixel(cx - w_min, cy, r, g, b, inv_intens);
        mixpixel(cx + w_max, cy, r, g, b, intens);
        cy = _y - x;
        mixpixel(cx - w_min, cy, r, g, b, inv_intens);
        mixpixel(cx + w_max, cy, r, g, b, intens);
        cx = _x - iiy - 1;
        cy = _y + x;
        mixpixel(cx - w_min, cy, r, g, b, intens);
        mixpixel(cx + w_max, cy, r, g, b, inv_intens);
        cy = _y - x;
        mixpixel(cx - w_min, cy, r, g, b, intens);
        mixpixel(cx + w_max, cy, r, g, b, inv_intens);
        for (int i = -w_min + 1; i < w_max; i++)
        {
            cx = _x - x;
            cy = _y + iiy;
            mixpixel(cx, cy + i, r, g, b, a);
            cy = _y - iiy - 1;
            mixpixel(cx, cy + i, r, g, b, a);
            cx = _x + x;
            cy = _y + iiy;
            mixpixel(cx, cy + i, r, g, b, a);
            cy = _y - iiy - 1;
            mixpixel(cx, cy + i, r, g, b, a);

            cx = _x + iiy;
            cy = _y + x;
            mixpixel(cx + i, cy, r, g, b, a);
            cy = _y - x;
            mixpixel(cx + i, cy, r, g, b, a);
            cx = _x - iiy - 1;
            cy = _y + x;
            mixpixel(cx + i, cy, r, g, b, a);
            cy = _y - x;
            mixpixel(cx + i, cy, r, g, b, a);
        }
    }
}

static bool sectorcont(float x, float y, int radius, float from, float to)
{
    float atan = atan2(y, x) - M_PI / 2;
    if (atan < 0)
    {
        atan = M_PI * 2 + atan;
    }
    if (from <= to)
    {
        return from <= atan && atan <= to;
    }
    else
    {
        return to >= atan || atan >= from;
    }
}

static void draw_gradient_arc_lines(int _x, int _y, int radius, float from, float to, Color c1, Color c2, int a)
{
    float fx = radius * cos(from + M_PI / 2);
    float fy = radius * sin(from + M_PI / 2);
    if (from == to)
    {
        draw_gradient_line_vu(_x, _y, fx + _x, fy + _y, c1, c2, a, 1);
        return;
    }

    float tx = radius * cos(to + M_PI / 2);
    float ty = radius * sin(to + M_PI / 2);
    draw_gradient_line_vu(_x, _y, fx + _x, fy + _y, c1, c2, a, 1);
    draw_gradient_line_vu(_x, _y, tx + _x, ty + _y, c1, c2, a, 1);
}

static void draw_arc_lines(int _x, int _y, int radius, float from, float to, int r, int g, int b, int a)
{
    float fx = radius * cos(from + M_PI / 2);
    float fy = radius * sin(from + M_PI / 2);
    if (from == to)
    {
        draw_line_vu(_x, _y, fx + _x, fy + _y, r, g, b, a, 1);
        return;
    }

    float tx = radius * cos(to + M_PI / 2);
    float ty = radius * sin(to + M_PI / 2);
    draw_line_vu(_x, _y, fx + _x, fy + _y, r, g, b, a, 1);
    draw_line_vu(_x, _y, tx + _x, ty + _y, r, g, b, a, 1);
}

static void draw_arc_vu(int _x, int _y, int radius, float from, float to, int r, int g, int b, int a)
{
    if (from == to)
    {
        return;
    }
    float iy = 0;
    float shift = M_PI * 0.0007;
    from = from - shift;
    to = to + shift;
    if (sectorcont(radius, 0, radius, from, to))
    {
        mixpixel(_x + radius, _y, r, g, b, a);
    }
    if (sectorcont(0, radius, radius, from, to))
    {
        mixpixel(_x, _y + radius, r, g, b, a);
    }
    if (sectorcont(-radius + 1, 0, radius, from, to))
    {
        mixpixel(_x - radius + 1, _y, r, g, b, a);
    }
    if (sectorcont(0, -radius + 1, radius, from, to))
    {
        mixpixel(_x, _y - radius + 1, r, g, b, a);
    }

    for (int x = 0; x <= radius * cos(M_PI / 4) + 1; x++)
    {
        iy = (float)sqrt(radius * radius - x * x);
        int intens = (int)((FPart(iy) * 255.0) * a / 255);
        int inv_intens = (255 - (int)(FPart(iy) * 255)) * a / 255;
        int iiy = IPart(iy);
        int cx = _x - x;
        int cy = _y + iiy;
        if (sectorcont(cx - _x, cy - _y, radius, from, to))
        {
            mixpixel(cx, cy, r, g, b, inv_intens);
            mixpixel(cx, cy + 1, r, g, b, intens);
        }
        cx = _x + x;
        cy = _y + iiy;
        if (sectorcont(cx - _x, cy - _y, radius, from, to))
        {
            mixpixel(cx, cy, r, g, b, inv_intens);
            mixpixel(cx, cy + 1, r, g, b, intens);
        }
        cx = _x + iiy;
        cy = _y + x;
        if (sectorcont(cx - _x, cy - _y, radius, from, to))
        {
            mixpixel(cx, cy, r, g, b, inv_intens);
            mixpixel(cx + 1, cy, r, g, b, intens);
        }
        cx = _x + iiy;
        cy = _y - x;
        if (sectorcont(cx - _x, cy - _y, radius, from, to))
        {
            mixpixel(cx, cy, r, g, b, inv_intens);
            mixpixel(cx + 1, cy, r, g, b, intens);
        }
        x++;
        cx = _x + x;
        cy = _y - iiy;
        if (sectorcont(cx - _x, cy - _y, radius, from, to))
        {
            mixpixel(cx, cy, r, g, b, intens);
            mixpixel(cx, cy + 1, r, g, b, inv_intens);
        }
        cx = _x - x;
        cy = _y - iiy;
        if (sectorcont(cx - _x, cy - _y, radius, from, to))
        {
            mixpixel(cx, cy, r, g, b, intens);
            mixpixel(cx, cy + 1, r, g, b, inv_intens);
        }
        cx = _x - iiy;
        cy = _y - x;
        if (sectorcont(cx - _x, cy - _y, radius, from, to))
        {
            mixpixel(cx, cy, r, g, b, intens);
            mixpixel(cx + 1, cy, r, g, b, inv_intens);
        }
        cx = _x - iiy;
        cy = _y + x;
        if (sectorcont(cx - _x, cy - _y, radius, from, to))
        {
            mixpixel(cx, cy, r, g, b, intens);
            mixpixel(cx + 1, cy, r, g, b, inv_intens);
        }
        x--;
    }
}

static void draw_normalized_arc(int32_t posx, int32_t posy, int32_t radius, float &from, float &to, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    if (abs(from - to) >= PI_2)
    {
        float fx = radius * cos(from);
        float fy = radius * sin(from);
        draw_circle_vu(posx, posy, radius, r, g, b, a, 1);
    }
    else
    {
        from -= M_PI / 2;
        to -= M_PI / 2;
        from = from >= PI_2 ? from - (((int)(from / PI_2)) * PI_2) : from;
        to = to >= PI_2 ? to - (((int)(to / PI_2)) * PI_2) : to;
        if (to < from && to < 0 && from < 0)
        {
            float temp = to;
            to = from;
            from = temp;
        }
        from = from < 0 ? (from - PI_2 * (int)(from / PI_2)) + PI_2 : from;
        to = to < 0 ? (to - PI_2 * (int)(to / PI_2)) + PI_2 : to;
        draw_arc_vu(posx, posy, radius, from, to, r, g, b, a);
    }
}

//https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
static void draw_circle(int32_t posx, int32_t posy, int32_t diameter, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    if (diameter > 0)
    {
        int radius = diameter / 2;
        int x = radius - 1;
        int y = 0;
        int dx = 1;
        int dy = 1;
        int err = dx - (radius << 1);

        while (x >= y)
        {
            mixpixel(posx + x, posy + y, r, g, b, a);
            mixpixel(posx + y, posy + x, r, g, b, a);
            mixpixel(posx - y, posy + x, r, g, b, a);
            mixpixel(posx - x, posy + y, r, g, b, a);
            mixpixel(posx - x, posy - y, r, g, b, a);
            mixpixel(posx - y, posy - x, r, g, b, a);
            mixpixel(posx + y, posy - x, r, g, b, a);
            mixpixel(posx + x, posy - y, r, g, b, a);

            if (err <= 0)
            {
                y++;
                err += dy;
                dy += 2;
            }

            if (err > 0)
            {
                x--;
                dx += 2;
                err += dx - (radius << 1);
            }
        }
    }
}

static void draw_filled_circle(int32_t posx, int32_t posy, int32_t diameter, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    if (diameter > 0)
    {
        int radius = diameter / 2;
        int x = radius - 1;
        int y = 0;
        int dx = 1;
        int dy = 1;
        int err = dx - (radius << 1);
        while (x >= y)
        {
            fill_line(posx - x, posx + x, posy + y, r, g, b, a);
            fill_line(posx - y, posx + y, posy + x, r, g, b, a);
            fill_line(posx - x, posx + x, posy - y, r, g, b, a);
            fill_line(posx - y, posx + y, posy - x, r, g, b, a);

            if (err <= 0)
            {
                y++;
                err += dy;
                dy += 2;
            }

            if (err > 0)
            {
                x--;
                dx += 2;
                err += dx - (radius << 1);
            }
        }
    }
}

static void draw_filled_circle_vu(int _x, int _y, int radius, int r, int g, int b, int a)
{
    if (radius < 1)
    {
        return;
    }

    float iy = 0;
    int intens = 0;
    int inv_intens = 0;
    int iiy = 0;
    int cx = 0;
    int cy = 0;
    int last_iiy1 = 0;
    int last_iiy2 = 0;
    int last_iiy3 = 0;
    int last_iiy4 = 0;
    int first_x = (radius + 1) * cos(M_PI / 4);
    int first_y = sqrt(radius * radius - first_x * first_x);
    for (int x = first_x; x >= 0; x--)
    {
        iy = (float)sqrt(radius * radius - x * x);
        intens = (int)((FPart(iy) * 255.0) * a / 255);
        inv_intens = (255 - (int)(FPart(iy) * 255)) * a / 255;
        iiy = IPart(iy);
        cx = _x - x;
        cy = _y + iiy;
        mixpixel(cx, cy, r, g, b, inv_intens);
        mixpixel(cx, cy + 1, r, g, b, intens);
        cy = _y - iiy - 1;
        mixpixel(cx, cy, r, g, b, intens);
        mixpixel(cx, cy + 1, r, g, b, inv_intens);
        cx = _x + x;
        cy = _y + iiy;
        mixpixel(cx, cy, r, g, b, inv_intens);
        mixpixel(cx, cy + 1, r, g, b, intens);
        cy = _y - iiy - 1;
        mixpixel(cx, cy, r, g, b, intens);
        mixpixel(cx, cy + 1, r, g, b, inv_intens);
        cx = _x + iiy;
        cy = _y + x;
        mixpixel(cx, cy, r, g, b, inv_intens);
        mixpixel(cx + 1, cy, r, g, b, intens);
        cy = _y - x;
        mixpixel(cx, cy, r, g, b, inv_intens);
        mixpixel(cx + 1, cy, r, g, b, intens);
        cx = _x - iiy - 1;
        cy = _y + x;
        mixpixel(cx, cy, r, g, b, intens);
        mixpixel(cx + 1, cy, r, g, b, inv_intens);
        cy = _y - x;
        mixpixel(cx, cy, r, g, b, intens);
        mixpixel(cx + 1, cy, r, g, b, inv_intens);
        // Fill circle
        if (_y + iiy != last_iiy1)
        {
            fill_mixed_line(_x - x, _x + x + 1, _y + iiy, r, g, b, a);
            fill_mixed_line(_x - x, _x + x + 1, _y - iiy, r, g, b, a);
            last_iiy1 = _y + iiy;
        }
        if (_y - x > _y - first_y && _y - x != last_iiy3)
        {
            last_iiy3 = _y - x;
            fill_mixed_line(_x - iiy, _x + iiy + 1, _y - x, r, g, b, a);
        }
        if (x != 0 && _y + x < _y + first_y && _y + x != last_iiy4)
        {
            last_iiy4 = _y + x;
            fill_mixed_line(_x - iiy, _x + iiy + 1, _y + x, r, g, b, a);
        }
    }
}

static int start_record_points_lua(lua_State *L)
{
    int top = lua_gettop(L);

    start_record_points();

    assert(top == lua_gettop(L));
    return 0;
}

static int stop_record_points_lua(lua_State *L)
{
    int top = lua_gettop(L);

    stop_record_points();

    assert(top == lua_gettop(L));
    return 0;
}

static int fill_area_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;
    read_and_validate_buffer_info(L, 1);
    int32_t x = luaL_checknumber(L, 2);
    int32_t y = luaL_checknumber(L, 3);
    int32_t r = luaL_checknumber(L, 4);
    int32_t g = luaL_checknumber(L, 5);
    int32_t b = luaL_checknumber(L, 6);
    int32_t a = 0;
    if (lua_isnumber(L, 7) == 1)
    {
        a = luaL_checknumber(L, 7);
    }
    if (a == 0 && buffer_info.channels == 4)
    {
        assert(top == lua_gettop(L));
        return 0;
    }

    fill_area(x, y, r, g, b, a);

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_triangle_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t x0 = luaL_checknumber(L, 2);
    int32_t y0 = luaL_checknumber(L, 3);
    int32_t x1 = luaL_checknumber(L, 4);
    int32_t y1 = luaL_checknumber(L, 5);
    int32_t x2 = luaL_checknumber(L, 6);
    int32_t y2 = luaL_checknumber(L, 7);
    uint32_t r = luaL_checknumber(L, 8);
    uint32_t g = luaL_checknumber(L, 9);
    uint32_t b = luaL_checknumber(L, 10);
    uint32_t a = 0;
    if (lua_isnumber(L, 11) == 1)
    {
        a = luaL_checknumber(L, 11);
    }

    draw_triangle(x0, y0, x1, y1, x2, y2, r, g, b, a);
    draw_line_vu(x0, y0, x1, y1, r, g, b, a, 1);
    draw_line_vu(x1, y1, x2, y2, r, g, b, a, 1);
    draw_line_vu(x2 - 1, y2, x0 - 1, y0, r, g, b, a, 1);

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_line_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t x0 = luaL_checknumber(L, 2);
    int32_t y0 = luaL_checknumber(L, 3);
    int32_t x1 = luaL_checknumber(L, 4);
    int32_t y1 = luaL_checknumber(L, 5);
    uint32_t r = luaL_checknumber(L, 6);
    uint32_t g = luaL_checknumber(L, 7);
    uint32_t b = luaL_checknumber(L, 8);
    uint32_t a = 0;
    int w = 1;
    if (lua_isnumber(L, 9) == 1)
    {
        a = luaL_checknumber(L, 9);
    }
    bool antialiasing = false;
    if (lua_isboolean(L, 10) == 1)
    {
        antialiasing = lua_toboolean(L, 10);
        if (lua_isnumber(L, 11) == 1)
        {
            w = luaL_checknumber(L, 11);
            if (w < 1)
            {
                w = 1;
            }
        }
    }

    if (antialiasing && buffer_info.channels == 4)
    {
        draw_line_vu(x0, y0, x1, y1, r, g, b, a, w);
    }
    else
    {
        draw_line(x0, y0, x1, y1, r, g, b, a);
    }

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_gradient_line_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t x0 = luaL_checknumber(L, 2);
    int32_t y0 = luaL_checknumber(L, 3);
    int32_t x1 = luaL_checknumber(L, 4);
    int32_t y1 = luaL_checknumber(L, 5);
    uint32_t r1 = luaL_checknumber(L, 6);
    uint32_t g1 = luaL_checknumber(L, 7);
    uint32_t b1 = luaL_checknumber(L, 8);
    uint32_t r2 = luaL_checknumber(L, 9);
    uint32_t g2 = luaL_checknumber(L, 10);
    uint32_t b2 = luaL_checknumber(L, 11);
    uint32_t a = 0;
    int w = 1;
    if (lua_isnumber(L, 12) == 1)
    {
        a = luaL_checknumber(L, 12);
    }
    bool antialiasing = false;
    if (lua_isboolean(L, 13) == 1)
    {
        antialiasing = lua_toboolean(L, 13);
        if (lua_isnumber(L, 14) == 1)
        {
            w = luaL_checknumber(L, 14);
            if (w < 1)
            {
                w = 1;
            }
        }
    }

    Color c1;
    c1.r = r1;
    c1.g = g1;
    c1.b = b1;
    Color c2;
    c2.r = r2;
    c2.g = g2;
    c2.b = b2;

    if (antialiasing && buffer_info.channels == 4)
    {
        draw_gradient_line_vu(x0, y0, x1, y1, c1, c2, a, w);
    }
    else
    {
        draw_gradient_line(x0, y0, x1, y1, c1, c2, a);
    }

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_arc_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t posx = luaL_checknumber(L, 2);
    int32_t posy = luaL_checknumber(L, 3);
    int32_t radius = luaL_checknumber(L, 4);
    float from = luaL_checknumber(L, 5);
    float to = luaL_checknumber(L, 6);
    uint32_t r = luaL_checknumber(L, 7);
    uint32_t g = luaL_checknumber(L, 8);
    uint32_t b = luaL_checknumber(L, 9);
    uint32_t a = 0;
    if (lua_isnumber(L, 10) == 1)
    {
        a = luaL_checknumber(L, 10);
    }

    draw_normalized_arc(posx, posy, radius, from, to, r, g, b, a);
    draw_arc_lines(posx, posy, radius, from, to, r, g, b, a);

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_filled_arc_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t posx = luaL_checknumber(L, 2);
    int32_t posy = luaL_checknumber(L, 3);
    int32_t radius = luaL_checknumber(L, 4);
    float from = luaL_checknumber(L, 5);
    float to = luaL_checknumber(L, 6);
    uint32_t r = luaL_checknumber(L, 7);
    uint32_t g = luaL_checknumber(L, 8);
    uint32_t b = luaL_checknumber(L, 9);
    uint32_t a = 0;
    if (lua_isnumber(L, 10) == 1)
    {
        a = luaL_checknumber(L, 10);
    }
    start_record_points();
    draw_normalized_arc(posx, posy, radius, from, to, r, g, b, a);
    draw_arc_lines(posx, posy, radius, from, to, r, g, b, a);

    float center = (from + to) / 2;
    center = from > to ? center - M_PI / 2 : center + M_PI / 2;

    fill_area(posx + radius * 2 / 3 * cos(center), posy + radius * 2 / 3 * sin(center), r, g, b, a);
    stop_record_points();
    draw_line_vu(posx, posy, posx + radius * 2 / 3 * cos(center), posy + radius * 2 / 3 * sin(center), r, g, b, a, 2);

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_gradient_arc_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t posx = luaL_checknumber(L, 2);
    int32_t posy = luaL_checknumber(L, 3);
    int32_t radius = luaL_checknumber(L, 4);
    float from = luaL_checknumber(L, 5);
    float to = luaL_checknumber(L, 6);
    uint32_t r1 = luaL_checknumber(L, 7);
    uint32_t g1 = luaL_checknumber(L, 8);
    uint32_t b1 = luaL_checknumber(L, 9);
    uint32_t r2 = luaL_checknumber(L, 10);
    uint32_t g2 = luaL_checknumber(L, 11);
    uint32_t b2 = luaL_checknumber(L, 12);
    uint32_t a = 0;
    if (lua_isnumber(L, 13) == 1)
    {
        a = luaL_checknumber(L, 13);
    }

    Color c1;
    c1.r = r1;
    c1.g = g1;
    c1.b = b1;
    Color c2;
    c2.r = r2;
    c2.g = g2;
    c2.b = b2;
    start_record_points();
    draw_normalized_arc(posx, posy, radius, from, to, r2, g2, b2, a);
    draw_gradient_arc_lines(posx, posy, radius, from, to, c1, c2, a);

    float center = (from + to) / 2;
    center = from > to ? center - M_PI / 2 : center + M_PI / 2;

    Point center_point;
    center_point.x = posx;
    center_point.y = posy;

    gradient_fill_area(posx + radius * 2 / 3 * cos(center), posy + radius * 2 / 3 * sin(center), center_point, radius * radius, c1, c2, a);
    stop_record_points();
    draw_gradient_line_vu(posx, posy, posx + radius * 2 / 2.05 * cos(center), posy + radius * 2 / 2.05 * sin(center), c1, c2, a, 1);

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_circle_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t posx = luaL_checknumber(L, 2);
    int32_t posy = luaL_checknumber(L, 3);
    int32_t diameter = luaL_checknumber(L, 4);
    uint32_t r = luaL_checknumber(L, 5);
    uint32_t g = luaL_checknumber(L, 6);
    uint32_t b = luaL_checknumber(L, 7);
    uint32_t a = 0;
    int w = 1;
    if (lua_isnumber(L, 8) == 1)
    {
        a = luaL_checknumber(L, 8);
    }
    bool antialiasing = false;
    if (lua_isboolean(L, 9) == 1)
    {
        antialiasing = lua_toboolean(L, 9);
        if (lua_isnumber(L, 10) == 1)
        {
            w = luaL_checknumber(L, 10);
            if (w < 1)
            {
                w = 1;
            }
        }
    }

    if (antialiasing && buffer_info.channels == 4)
    {
        draw_circle_vu(posx, posy, diameter / 2, r, g, b, a, w);
    }
    else
    {
        draw_circle(posx, posy, diameter, r, g, b, a);
    }

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_filled_circle_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t posx = luaL_checknumber(L, 2);
    int32_t posy = luaL_checknumber(L, 3);
    int32_t diameter = luaL_checknumber(L, 4);
    uint32_t r = luaL_checknumber(L, 5);
    uint32_t g = luaL_checknumber(L, 6);
    uint32_t b = luaL_checknumber(L, 7);
    uint32_t a = 0;
    if (lua_isnumber(L, 8) == 1)
    {
        a = luaL_checknumber(L, 8);
    }
    bool antialiasing = false;
    if (lua_isboolean(L, 9) == 1)
    {
        antialiasing = lua_toboolean(L, 9);
    }

    if (antialiasing && buffer_info.channels == 4)
    {
        draw_filled_circle_vu(posx, posy, diameter / 2, r, g, b, a);
    }
    else
    {
        draw_filled_circle(posx, posy, diameter, r, g, b, a);
    }

    assert(top == lua_gettop(L));
    return 0;
}

static int fill_texture_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    uint8_t r = luaL_checknumber(L, 2);
    uint8_t g = luaL_checknumber(L, 3);
    uint8_t b = luaL_checknumber(L, 4);
    uint8_t a = 0;
    if (lua_isnumber(L, 5) == 1)
    {
        a = luaL_checknumber(L, 5);
    }
    int line_size = buffer_info.width * buffer_info.channels;
    uint8_t *line = new uint8_t[line_size];
    for (int i = 0; i < line_size; i += buffer_info.channels)
    {
        line[i] = r;
        line[i + 1] = g;
        line[i + 2] = b;
        if (buffer_info.channels == 4)
        {
            line[i + 3] = a;
        }
    }
    for (int i = 0; i < buffer_info.src_size; i += line_size)
    {
        memcpy(&buffer_info.bytes[i], line, line_size);
    }
    delete[] line;
    assert(top == lua_gettop(L));
    return 0;
}

static int draw_rect_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t posx = luaL_checknumber(L, 2);
    int32_t posy = luaL_checknumber(L, 3);
    uint32_t sizex = luaL_checknumber(L, 4);
    uint32_t sizey = luaL_checknumber(L, 5);
    uint32_t r = luaL_checknumber(L, 6);
    uint32_t g = luaL_checknumber(L, 7);
    uint32_t b = luaL_checknumber(L, 8);
    uint32_t a = 0;
    if (lua_isnumber(L, 9) == 1)
    {
        a = luaL_checknumber(L, 9);
    }

    int half_size_x = sizex / 2;
    int half_size_y = sizey / 2;
    int newposx = 0;
    int newposy = 0;
    for (int y = -half_size_y; y < half_size_y; y++)
    {
        if (y == -half_size_y || y == half_size_y - 1)
        {
            for (int x = -half_size_x; x < half_size_x; x++)
            {
                newposx = x + posx;
                newposy = y + posy;
                putpixel(newposx, newposy, r, g, b, a);
            }
        }
        else
        {
            putpixel(-half_size_x + posx, y + posy, r, g, b, a);
            putpixel(half_size_x - 1 + posx, y + posy, r, g, b, a);
        }
    }

    assert(top == lua_gettop(L));
    return 0;
}

bool sort_coords(const Point &p1, const Point &p2)
{
    if (p1.y == p2.y)
        return p1.x < p2.x;
    return p1.y < p2.y;
}

static int draw_filled_rect_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t posx = luaL_checknumber(L, 2);
    int32_t posy = luaL_checknumber(L, 3);
    uint32_t sizex = luaL_checknumber(L, 4);
    uint32_t sizey = luaL_checknumber(L, 5);
    uint32_t r = luaL_checknumber(L, 6);
    uint32_t g = luaL_checknumber(L, 7);
    uint32_t b = luaL_checknumber(L, 8);
    uint32_t a = 0;
    if (lua_isnumber(L, 9) == 1)
    {
        a = luaL_checknumber(L, 9);
    }
    int angle = 0;
    if (lua_isnumber(L, 10) == 1)
    {
        angle = luaL_checknumber(L, 10);
    }

    int half_size_x = sizex / 2;
    int half_size_y = sizey / 2;
    if (angle == 0)
    {
        int newposx = 0;
        int newposy = 0;
        for (int y = -half_size_y; y < half_size_y; y++)
        {
            newposy = y + posy;
            fill_line(posx - half_size_x, posx + half_size_x, newposy, r, g, b, a);
        }
    }
    else
    {
        std::vector<Point> vec(4);
        float rd = M_PI / 180 * angle;
        float cs = cos(rd);
        float sn = sin(rd);
        vec[0].x = (-half_size_x) * cs - (-half_size_y) * sn;
        vec[0].y = (-half_size_x) * sn + (-half_size_y) * cs;
        vec[1].x = half_size_x * cs - half_size_y * sn;
        vec[1].y = half_size_x * sn + half_size_y * cs;
        vec[2].x = (-half_size_x) * cs - half_size_y * sn;
        vec[2].y = (-half_size_x) * sn + half_size_y * cs;
        vec[3].x = half_size_x * cs - (-half_size_y) * sn;
        vec[3].y = half_size_x * sn + (-half_size_y) * cs;
        for (int i = 0; i < 4; i++)
        {
            vec[i].x += posx;
            vec[i].y += posy;
        }
        sort(vec.begin(), vec.end(), sort_coords);
        draw_triangle(vec[0].x, vec[0].y, vec[1].x, vec[1].y, vec[2].x, vec[2].y, r, g, b, a);
        draw_triangle(vec[1].x, vec[1].y, vec[2].x, vec[2].y, vec[3].x, vec[3].y, r, g, b, a);
    }

    assert(top == lua_gettop(L));
    return 0;
}

static int draw_pixel_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t x = luaL_checknumber(L, 2);
    int32_t y = luaL_checknumber(L, 3);
    uint32_t r = luaL_checknumber(L, 4);
    uint32_t g = luaL_checknumber(L, 5);
    uint32_t b = luaL_checknumber(L, 6);
    uint32_t a = 0;
    if (lua_isnumber(L, 7) == 1)
    {
        a = luaL_checknumber(L, 7);
    }

    mixpixel(x, y, r, g, b, a);

    assert(top == lua_gettop(L));
    return 0;
}

static int read_color_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t x = luaL_checknumber(L, 2);
    int32_t y = luaL_checknumber(L, 3);

    if (!in_buffer(x, y))
    {
        assert(top == lua_gettop(L));
        return 0;
    }

    int i = xytoi(x, y);
    lua_pushnumber(L, buffer_info.bytes[i]);
    lua_pushnumber(L, buffer_info.bytes[i + 1]);
    lua_pushnumber(L, buffer_info.bytes[i + 2]);
    if (buffer_info.channels == 4)
    {
        lua_pushnumber(L, buffer_info.bytes[i + 3]);
    }
    assert(top + buffer_info.channels == lua_gettop(L));
    return buffer_info.channels;
}

static int draw_bezier_lua(lua_State *L)
{
    int top = lua_gettop(L) + 5;

    read_and_validate_buffer_info(L, 1);
    int32_t x0 = luaL_checknumber(L, 2);
    int32_t y0 = luaL_checknumber(L, 3);
    int32_t xc = luaL_checknumber(L, 4);
    int32_t yc = luaL_checknumber(L, 5);
    int32_t x1 = luaL_checknumber(L, 6);
    int32_t y1 = luaL_checknumber(L, 7);
    uint32_t r = luaL_checknumber(L, 8);
    uint32_t g = luaL_checknumber(L, 9);
    uint32_t b = luaL_checknumber(L, 10);
    uint32_t a = 0;
    if (lua_isnumber(L, 11) == 1)
    {
        a = luaL_checknumber(L, 11);
    }

    int max_dx = fmax(fmax(x0, xc), x1) - fmin(fmin(x0, xc), x1);
    int max_dy = fmax(fmax(y0, yc), y1) - fmin(fmin(y0, yc), y1);

    double max_d = fmax(max_dx, max_dy) * 2;
    double dt = 1.0 / max_d;

    for (double t = 0; t < 1; t += dt)
    {
        double opt1 = pow((1 - t), 2);
        double opt2 = 2 * t * (1 - t);
        double opt3 = pow(t, 2);
        int xt = (opt1 * x0) + (opt2 * xc) + (opt3 * x1);
        int yt = (opt1 * y0) + (opt2 * yc) + (opt3 * y1);
        putpixel(xt, yt, r, g, b, a);
    }

    assert(top == lua_gettop(L));
    return 0;
}

static int copy_pixels_lua(lua_State* L) {
    int top = lua_gettop(L) + 10;  // account for pushed fields from two buffer tables
    // 1. Read and validate destination buffer
    read_and_validate_buffer_info(L, 1);
    BufferInfo dest = buffer_info;  // store dest info locally
    // 2. Read and validate source buffer (will overwrite global buffer_info)
    read_and_validate_buffer_info(L, 2);
    BufferInfo src = buffer_info;   // store source info
    // (Optionally, restore dest buffer_info if we plan to use buffer_info in helpers for dest)

    // 3. Get rectangle and destination coordinates from Lua arguments
    int32_t rect_x      = luaL_checkinteger(L, 3);
    int32_t rect_y      = luaL_checkinteger(L, 4);
    int32_t rect_width  = luaL_checkinteger(L, 5);
    int32_t rect_height = luaL_checkinteger(L, 6);
    int32_t dest_x      = luaL_checkinteger(L, 7);
    int32_t dest_y      = luaL_checkinteger(L, 8);
    bool merge_alpha = false;
    if (lua_gettop(L) >= 9 && lua_isboolean(L, 9)) {
        merge_alpha = lua_toboolean(L, 9);
    }

    // 4. Clamp and adjust the copy region to source and dest bounds
    if (rect_x < 0) { 
        rect_width += rect_x;  // reduce width
        dest_x += rect_x;      // move dest start left
        rect_x = 0;
    }
    if (rect_y < 0) {
        rect_height += rect_y;
        dest_y += rect_y;
        rect_y = 0;
    }
    if (rect_x + rect_width > src.width) {
        rect_width = src.width - rect_x;
    }
    if (rect_y + rect_height > src.height) {
        rect_height = src.height - rect_y;
    }
    // Clamp to dest
    if (dest_x < 0) {
        // skip pixels off left of dest
        rect_x -= dest_x;
        rect_width += dest_x;
        dest_x = 0;
    }
    if (dest_y < 0) {
        rect_y -= dest_y;
        rect_height += dest_y;
        dest_y = 0;
    }
    if (dest_x + rect_width > dest.width) {
        rect_width = dest.width - dest_x;
    }
    if (dest_y + rect_height > dest.height) {
        rect_height = dest.height - dest_y;
    }
    if (rect_width <= 0 || rect_height <= 0) {
        // Nothing to copy
        assert(top == lua_gettop(L));
        return 0;
    }

    // 5. Perform the copy operation
    uint8_t* destBytes = dest.bytes;
    uint8_t* srcBytes  = src.bytes;
    int channels = dest.channels;   // dest channels (assume dest and src match for simplicity)
    int src_channels = src.channels;
    // If no alpha merge, copy directly
    if (!merge_alpha) {
        if (dest.channels == src.channels) {
            // Optimized path: copy row by row using memcpy
            for (int y = 0; y < rect_height; ++y) {
                uint32_t src_offset  = ((rect_y + y) * src.width  + rect_x) * src_channels;
                uint32_t dest_offset = ((dest_y + y) * dest.width + dest_x) * channels;
                memcpy(&destBytes[dest_offset], &srcBytes[src_offset], rect_width * channels);
            }
        } else {
            // Channels differ (e.g., RGB -> RGBA or vice versa)
            for (int y = 0; y < rect_height; ++y) {
                for (int x = 0; x < rect_width; ++x) {
                    uint32_t src_index  = ((rect_y + y) * src.width  + (rect_x + x)) * src_channels;
                    uint32_t dest_index = ((dest_y + y) * dest.width + (dest_x + x)) * channels;
                    // Copy RGB
                    destBytes[dest_index]     = srcBytes[src_index];
                    destBytes[dest_index + 1] = srcBytes[src_index + 1];
                    destBytes[dest_index + 2] = srcBytes[src_index + 2];
                    if (channels == 4) {
                        // dest has alpha channel
                        if (src_channels == 4) {
                            destBytes[dest_index + 3] = srcBytes[src_index + 3];  // copy alpha from src
                        } else {
                            destBytes[dest_index + 3] = 255;  // no alpha in source, make fully opaque
                        }
                    }
                    // if dest is RGB and src has alpha, we simply drop the alpha (or could premultiply color, but since no dest alpha, we consider dest fully opaque).
                }
            }
        }
    } else {
        // merge_alpha == true, perform per-pixel alpha blending
        for (int y = 0; y < rect_height; ++y) {
            for (int x = 0; x < rect_width; ++x) {
                uint32_t src_index  = ((rect_y + y) * src.width  + (rect_x + x)) * src_channels;
                uint32_t dest_index = ((dest_y + y) * dest.width + (dest_x + x)) * channels;
                // Read source pixel
                uint8_t sr = srcBytes[src_index];
                uint8_t sg = srcBytes[src_index + 1];
                uint8_t sb = srcBytes[src_index + 2];
                uint8_t sa = (src_channels == 4 ? srcBytes[src_index + 3] : 255);
                if (sa == 0) {
                    continue;  // fully transparent source pixel, skip
                }
                if (channels == 3) {
                    // Dest has no alpha: blend onto an opaque background
                    float alpha = sa / 255.0f;
                    // Get dest pixel
                    uint8_t dr = destBytes[dest_index];
                    uint8_t dg = destBytes[dest_index + 1];
                    uint8_t db = destBytes[dest_index + 2];
                    // Blend RGB (dest alpha is implicitly 1)
                    destBytes[dest_index]     = (uint8_t)(dr * (1 - alpha) + sr * alpha);
                    destBytes[dest_index + 1] = (uint8_t)(dg * (1 - alpha) + sg * alpha);
                    destBytes[dest_index + 2] = (uint8_t)(db * (1 - alpha) + sb * alpha);
                } else {
                    // Dest has 4 channels (RGBA)
                    uint8_t dr = destBytes[dest_index];
                    uint8_t dg = destBytes[dest_index + 1];
                    uint8_t db = destBytes[dest_index + 2];
                    uint8_t da = destBytes[dest_index + 3];
                    if (sa == 255 || (!dest.premultiply_alpha && da == 0)) {
                        // Opaque source or empty dest (non-premultiplied): copy directly
                        destBytes[dest_index]     = sr;
                        destBytes[dest_index + 1] = sg;
                        destBytes[dest_index + 2] = sb;
                        destBytes[dest_index + 3] = sa;
                    } else if (dest.premultiply_alpha && da == 0) {
                        // Dest uses premultiplied alpha and dest pixel is transparent -> just premultiply source into dest
                        destBytes[dest_index]     = (uint8_t)((sr * sa) / 255);
                        destBytes[dest_index + 1] = (uint8_t)((sg * sa) / 255);
                        destBytes[dest_index + 2] = (uint8_t)((sb * sa) / 255);
                        destBytes[dest_index + 3] = sa;
                    } else {
                        // General blending case
                        float srcA = sa / 255.0f;
                        float destA = da / 255.0f;
                        float outA = srcA + destA * (1 - srcA);
                        // Compute output RGB (in float [0,1])
                        float outR = (sr/255.0f * srcA + dr/255.0f * destA * (1 - srcA));
                        float outG = (sg/255.0f * srcA + dg/255.0f * destA * (1 - srcA));
                        float outB = (sb/255.0f * srcA + db/255.0f * destA * (1 - srcA));
                        if (outA > 0) {
                            // convert premultiplied result to straight alpha (if needed)
                            outR = outR / outA;
                            outG = outG / outA;
                            outB = outB / outA;
                        }
                        // Write back to dest (clamp to [0,255])
                        destBytes[dest_index]     = (uint8_t) (outR * 255);
                        destBytes[dest_index + 1] = (uint8_t) (outG * 255);
                        destBytes[dest_index + 2] = (uint8_t) (outB * 255);
                        destBytes[dest_index + 3] = (uint8_t) (outA * 255);
                    }
                }
            }
        }
    }

    assert(top == lua_gettop(L));
    return 0;
}


// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
        {"start_fill", start_record_points_lua},
        {"end_fill", stop_record_points_lua},
        {"fill_area", fill_area_lua},
        // {"triangle", draw_triangle_lua},
        {"line", draw_line_lua},
        {"gradient_line", draw_gradient_line_lua},
        {"arc", draw_arc_lua},
        {"filled_arc", draw_filled_arc_lua},
        {"gradient_arc", draw_gradient_arc_lua},
        {"circle", draw_circle_lua},
        {"filled_circle", draw_filled_circle_lua},
        {"fill", fill_texture_lua},
        {"rect", draw_rect_lua},
        {"filled_rect", draw_filled_rect_lua},
        {"pixel", draw_pixel_lua},
        {"color", read_color_lua},
        {"bezier", draw_bezier_lua},
        {"copy_pixels", copy_pixels_lua},
        {0, 0}};

static void LuaInit(lua_State *L)
{
    int top = lua_gettop(L);
    luaL_register(L, MODULE_NAME, Module_methods);
    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializeDrawPixelsExtension(dmExtension::AppParams *params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeDrawPixelsExtension(dmExtension::Params *params)
{
    LuaInit(params->m_L);
    printf("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeDrawPixelsExtension(dmExtension::AppParams *params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeDrawPixelsExtension(dmExtension::Params *params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(DrawPixels, LIB_NAME, AppInitializeDrawPixelsExtension, AppFinalizeDrawPixelsExtension, InitializeDrawPixelsExtension, 0, 0, FinalizeDrawPixelsExtension)
