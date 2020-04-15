// Extension lib defines
#define LIB_NAME "DrawPixels"
#define MODULE_NAME "drawpixels"

#define DLIB_LOG_DOMAIN LIB_NAME
#define M_PI 3.14159265358979323846
#define M_PI_2 M_PI * 2

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

static void recordmixpixel(int i, float r, float g, float b, float a)
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
  if (buffer_info.channels == 3 || buffer_info.bytes[i + 3] == 0 || a == 255)
  {
    putpixel(x, y, r, g, b, a);
    return;
  }
  if (a == 0)
  {
    return;
  }

  recordmixpixel(i, r, g, b, a);
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
    recordmixpixel(i, r, g, b, a);
  }
}

static bool is_contain(int x, int y)
{
  return points[y * buffer_info.width + x] == 1;
}

static void set_border_pixels(int x, int y, int &left, int &right)
{
  // for (Point point : points)
  // {
  //   if (point.y == y)
  //   {
  //     if (point.x <= x && point.x <= left)
  //     {
  //       left = point.x;
  //     }
  //     if (point.x >= x && point.x >= right)
  //     {
  //       right = point.x;
  //     }
  //   }
  // }
}

static bool is_new(int i, int r, int g, int b)
{
  return buffer_info.bytes[i] == r && buffer_info.bytes[i + 1] == g && buffer_info.bytes[i + 2] == b;
}

static void find_seed_pixel(std::stack<Point> &top, Point pixel, int x_right, int r, int g, int b, int a)
{
  int MAX = 1000000;
  int count = 0;
  while (pixel.x <= x_right && count <= MAX)
  {
    // printf("count3: %d\n", count);
    count++;
    int flag = 0;
    while (pixel.x < x_right && count <= MAX && !is_contain(pixel.x, pixel.y))
    {
      count++;
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
        // printf("push x == right x: %d y: %d \n", new_pixel.x, new_pixel.y);
        top.push(new_pixel);
      }
      else
      {
        Point new_pixel;
        new_pixel.x = pixel.x - 1;
        new_pixel.y = pixel.y;
        // printf("push x: %d y: %d \n", new_pixel.x, new_pixel.y);
        top.push(new_pixel);
      }
      flag = 0;
    }

    int x_in = pixel.x;
    while (pixel.x < x_right && count <= MAX && is_contain(pixel.x, pixel.y))
    {
      mixpixel(pixel.x, pixel.y, r, g, b, a);
      count++;
      pixel.x += 1;
    }
    if (pixel.x == x_in)
    {
      pixel.x += 1;
    }
  }
}

// Построчный алгоритм заполнения с затравкой
static void fill_area(int x, int y, int r, int g, int b, int a)
{
  Point _pixel;
  _pixel.x = x;
  _pixel.y = y;
  std::stack<Point> top;
  top.push(_pixel);

  int MAX = 100000;
  int count = 0;

  while (!top.empty() && count <= MAX)
  {
    count++;
    Point pixel = top.top();
    top.pop();
    mixpixel(pixel.x, pixel.y, r, g, b, a);
    // printf("count: %d x: %d y: %d \n", count, pixel.x, pixel.y);
    int temp_x = pixel.x;
    pixel.x += 1;
    while (pixel.x < buffer_info.width && !is_contain(pixel.x, pixel.y) && count <= MAX)
    {
      // count++;
      mixpixel(pixel.x, pixel.y, r, g, b, a);
      pixel.x += 1;
    }
    mixpixel(pixel.x, pixel.y, r, g, b, a);
    int x_right = pixel.x;
    pixel.x = temp_x;
    pixel.x -= 1;
    while (pixel.x > -1 && !is_contain(pixel.x, pixel.y) && count <= MAX)
    {
      // count++;
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
    if (pixel.y <= 0)
    {
      continue;
    }

    pixel.x = x_left;
    find_seed_pixel(top, pixel, x_right, r, g, b, a);
    // printf("count: %d\n", count);
  }
}

static Color lerp_color(Point center, Point pixel, float fd_2, Color c1, Color c2)
{
  Color nc;
  float coef = (float)((center.x - pixel.x) * (center.x - pixel.x) + (center.y - pixel.y) * (center.y - pixel.y)) / fd_2;
  nc.r = c1.r + (c2.r - c1.r) * coef;
  nc.g = c1.g + (c2.g - c1.g) * coef;
  nc.b = c1.b + (c2.b - c1.b) * coef;
  return nc;
}

static void lerppixel(Point center, Point pixel, int fd_2, Color c1, Color c2, int a)
{
  Color nc = lerp_color(center, pixel, fd_2, c1, c2);
  mixpixel(pixel.x, pixel.y, nc.r, nc.g, nc.b, a);
}

static void gradient_find_seed_pixel(std::stack<Point> &top, Point pixel, int x_right, Point center, int fd_2, Color c1, Color c2, int a)
{
  int MAX = 1000000;
  int count = 0;
  while (pixel.x <= x_right && count <= MAX)
  {
    // printf("count3: %d\n", count);
    count++;
    int flag = 0;
    while (pixel.x < x_right && count <= MAX && !is_contain(pixel.x, pixel.y))
    {
      count++;
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
        // printf("push x == right x: %d y: %d \n", new_pixel.x, new_pixel.y);
        top.push(new_pixel);
      }
      else
      {
        Point new_pixel;
        new_pixel.x = pixel.x - 1;
        new_pixel.y = pixel.y;
        // printf("push x: %d y: %d \n", new_pixel.x, new_pixel.y);
        top.push(new_pixel);
      }
      flag = 0;
    }

    int x_in = pixel.x;
    while (pixel.x < x_right && count <= MAX && is_contain(pixel.x, pixel.y))
    {
      lerppixel(center, pixel, fd_2, c1, c2, a);
      count++;
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

  Point _pixel;
  _pixel.x = x;
  _pixel.y = y;
  std::stack<Point> top;
  top.push(_pixel);

  int MAX = 100000;
  int count = 0;

  while (!top.empty() && count <= MAX)
  {
    count++;
    Point pixel = top.top();
    top.pop();
    lerppixel(center, pixel, fd_2, c1, c2, a);
    // printf("count: %d x: %d y: %d \n", count, pixel.x, pixel.y);
    int temp_x = pixel.x;
    pixel.x += 1;
    while (pixel.x < buffer_info.width && !is_contain(pixel.x, pixel.y) && count <= MAX)
    {
      // count++;
      lerppixel(center, pixel, fd_2, c1, c2, a);
      pixel.x += 1;
    }
    lerppixel(center, pixel, fd_2, c1, c2, a);
    int x_right = pixel.x;
    pixel.x = temp_x;
    pixel.x -= 1;
    while (pixel.x > -1 && !is_contain(pixel.x, pixel.y) && count <= MAX)
    {
      // count++;
      lerppixel(center, pixel, fd_2, c1, c2, a);
      pixel.x -= 1;
    }
    lerppixel(center, pixel, fd_2, c1, c2, a);
    pixel.x++;
    int x_left = pixel.x;
    pixel.y += 1;
    if (pixel.y >= buffer_info.height)
    {
      continue;
    }
    gradient_find_seed_pixel(top, pixel, x_right, center, fd_2, c1, c2, a);
    pixel.y -= 2;
    if (pixel.y <= 0)
    {
      continue;
    }

    pixel.x = x_left;
    gradient_find_seed_pixel(top, pixel, x_right, center, fd_2, c1, c2, a);
    // printf("count: %d\n", count);
  }
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

//http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
static void fillBottomFlatTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a)
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

static void fillTopFlatTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a)
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

static void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a)
{
  /* values should be sorted by y */
  /* here we know that y1 <= y2 <= y3 */
  /* check for trivial case of bottom-flat triangle */
  if (y2 == y3)
  {
    fillBottomFlatTriangle(x1, y1, x2, y2, x3, y3, r, g, b, a);
  }
  /* check for trivial case of top-flat triangle */
  else if (y1 == y2)
  {
    fillTopFlatTriangle(x1, y1, x2, y2, x3, y3, r, g, b, a);
  }
  else
  {
    /* general case - split the triangle in a topflat and bottom-flat one */
    int x4 = x1 + ((float(y2 - y1) / float(y3 - y1)) * (x3 - x1));
    int y4 = y2;
    fillBottomFlatTriangle(x1, y1, x2, y2, x4, y4, r, g, b, a);
    fillTopFlatTriangle(x2, y2, x4, y4, x3, y3, r, g, b, a);
  }
}

static void read_and_validate_buffer_info(lua_State *L, int index)
{
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, "buffer");
  lua_getfield(L, index, "width");
  lua_getfield(L, index, "height");
  lua_getfield(L, index, "channels");
  dmScript::LuaHBuffer *lua_buffer = dmScript::CheckBuffer(L, -4);
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

  buffer_info.width = lua_tointeger(L, -3);
  if (buffer_info.width == 0)
  {
    luaL_error(L, "'width' of the buffer should be an integer and > 0");
  }

  buffer_info.height = lua_tointeger(L, -2);
  if (buffer_info.height == 0)
  {
    luaL_error(L, "'height' of the buffer should be an integer and > 0");
  }

  buffer_info.channels = lua_tointeger(L, -1);
  if (buffer_info.channels == 0)
  {
    luaL_error(L, "'channels' of should be an integer and 3 or 4");
  }
}

// static void draw_simple_line(int x0, int y0.int  x1,int  y1, int r, int g, int b, int a) {
// }

static void DrawLine(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
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

static int IPart(float x) //Целая часть числа
{
  return (int)x;
}

static float FPart(float x) //дробная часть числа
{
  while (x >= 0)
    x--;
  x++;
  return x;
}

static void plotLineWidth(int x0, int y0, int x1, int y1, float wd, int r, int g, int b, int a)
{
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx - dy, e2, x2, y2; /* error value e_xy */
  float ed = dx + dy == 0 ? 1 : sqrt((float)dx * dx + (float)dy * dy);

  for (wd = (wd + 1) / 2;;)
  { /* pixel loop */
    mixpixel(x0, y0, r, g, b, a - fmax(0, 255 * (abs(err - dx + dy) / ed - wd + 1)));
    e2 = err;
    x2 = x0;
    if (2 * e2 >= -dx)
    { /* x step */
      for (e2 += dy, y2 = y0; e2 < ed * wd && (y1 != y2 || dx > dy); e2 += dx)
        mixpixel(x0, y2 += sy, r, g, b, a - fmax(0, 255 * (abs(e2) / ed - wd + 1)));
      if (x0 == x1)
        break;
      e2 = err;
      err -= dy;
      x0 += sx;
    }
    if (2 * e2 <= dy)
    { /* y step */
      for (e2 = dx - e2; e2 < ed * wd && (x1 != x2 || dx < dy); e2 += dy)
        mixpixel(x2 += sx, y0, r, g, b, a - fmax(0, 255 * (abs(e2) / ed - wd + 1)));
      if (y0 == y1)
        break;
      err += dx;
      y0 += sy;
    }
  }
}

// http://grafika.me/node/38
static void DrawLineVU(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
  int w = 4;
  //Вычисление изменения координат
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
  //Если линия параллельна одной из осей, рисуем обычную линию - заполняем все пикселы в ряд
  if (dx == 0 || dy == 0)
  {
    DrawLine(x0, y0, x1, y1, r, g, b, a);
    return;
  }

  //Для Х-линии (коэффициент наклона < 1)
  if (dy < dx)
  {
    //Первая точка должна иметь меньшую координату Х
    if (x1 < x0)
    {
      x1 += x0;
      x0 = x1 - x0;
      x1 -= x0;
      y1 += y0;
      y0 = y1 - y0;
      y1 -= y0;
    }
    //Относительное изменение координаты Y
    float grad = (float)dy / dx;
    if (y1 < y0)
      grad = -grad;
    //Промежуточная переменная для Y
    float intery = y0 + grad;
    //Первая точка
    mixpixel(x0, y0, r, g, b, a);

    for (int x = x0 + 1; x < x1; x++)
    {
      //Верхняя точка
      mixpixel(x, IPart(intery), r, g, b, (int)(255 - FPart(intery) * 255) * a / 255);
      //Нижняя точка
      mixpixel(x, IPart(intery) + 1, r, g, b, (int)(FPart(intery) * 255) * a / 255);
      //Изменение координаты Y
      intery += grad;
    }
    //Последняя точка
    mixpixel(x1, y1, r, g, b, a);
  }
  //Для Y-линии (коэффициент наклона > 1)
  else
  {
    //Первая точка должна иметь меньшую координату Y
    if (y1 < y0)
    {
      x1 += x0;
      x0 = x1 - x0;
      x1 -= x0;
      y1 += y0;
      y0 = y1 - y0;
      y1 -= y0;
    }
    //Относительное изменение координаты X
    float grad = (float)dx / dy;
    if (x1 < x0)
      grad = -grad;
    //Промежуточная переменная для X
    float interx = x0 + grad;
    //Первая точка
    mixpixel(x0, y0, r, g, b, a);

    for (int y = y0 + 1; y < y1; y++)
    {
      int intens = (int)(FPart(interx) * 255);
      //Верхняя точка
      mixpixel(IPart(interx), y, r, g, b, (255 - intens) * a / 255);
      //Нижняя точка
      mixpixel(IPart(interx) + 1, y, r, g, b, intens * a / 255);
      //Изменение координаты X
      interx += grad;
    }
    //Последняя точка
    mixpixel(x1, y1, r, g, b, a);
  }
}

static void DrawGradientLineVU(int x0, int y0, int x1, int y1, Color c1, Color c2, int a)
{
  Point center;
  center.x = x0;
  center.y = y0;
  int fd_2 = (x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1);
  int w = 4;
  //Вычисление изменения координат
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
  //Если линия параллельна одной из осей, рисуем обычную линию - заполняем все пикселы в ряд
  // if (dx == 0 || dy == 0)
  // {
  //   DrawLine(x0, y0, x1, y1, r, g, b, a);
  //   return;
  // }

  //Для Х-линии (коэффициент наклона < 1)
  if (dy < dx)
  {
    //Первая точка должна иметь меньшую координату Х
    if (x1 < x0)
    {
      x1 += x0;
      x0 = x1 - x0;
      x1 -= x0;
      y1 += y0;
      y0 = y1 - y0;
      y1 -= y0;
    }
    //Относительное изменение координаты Y
    float grad = (float)dy / dx;
    if (y1 < y0)
      grad = -grad;
    //Промежуточная переменная для Y
    float intery = y0 + grad;
    //Первая точка
    mixpixel(x0, y0, c1.r, c1.g, c1.b, a);

    for (int x = x0 + 1; x < x1; x++)
    {
      //Верхняя точка
      Point pixel;
      pixel.x = x;
      pixel.y = IPart(intery);
      lerppixel(center, pixel, fd_2, c1, c2, (int)(255 - FPart(intery) * 255) * a / 255);
      //Нижняя точка
      pixel.y += 1;
      lerppixel(center, pixel, fd_2, c1, c2, (int)(FPart(intery) * 255) * a / 255);
      //Изменение координаты Y
      intery += grad;
    }
    //Последняя точка
    mixpixel(x1, y1, c2.r, c2.g, c2.b, a);
  }
  //Для Y-линии (коэффициент наклона > 1)
  else
  {
    //Первая точка должна иметь меньшую координату Y
    if (y1 < y0)
    {
      x1 += x0;
      x0 = x1 - x0;
      x1 -= x0;
      y1 += y0;
      y0 = y1 - y0;
      y1 -= y0;
    }
    //Относительное изменение координаты X
    float grad = (float)dx / dy;
    if (x1 < x0)
      grad = -grad;
    //Промежуточная переменная для X
    float interx = x0 + grad;
    //Первая точка
    mixpixel(x0, y0, c1.r, c1.g, c1.b, a);

    for (int y = y0 + 1; y < y1; y++)
    {
      int intens = (int)(FPart(interx) * 255);
      //Верхняя точка
      Point pixel;
      pixel.x = IPart(interx);
      pixel.y = y;
      lerppixel(center, pixel, fd_2, c1, c2, (255 - intens) * a / 255);
      //Нижняя точка
      pixel.x += 1;
      lerppixel(center, pixel, fd_2, c1, c2, intens * a / 255);
      //Изменение координаты X
      interx += grad;
    }
    //Последняя точка
    mixpixel(x1, y1, c2.r, c2.g, c2.b, a);
  }
}

static int draw_pixel(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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

  putpixel(x, y, r, g, b, a);

  assert(top == lua_gettop(L));
  return 0;
}

static int read_color(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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

// http://www.opita.net/node/699
static void DrawWuCircle(int _x, int _y, int radius, int r, int g, int b, int a)
{
  //Установка пикселов, лежащих на осях системы координат с началом в центре
  // mixpixel(_x + radius, _y, r, g, b, a);
  // mixpixel(_x, _y + radius, r, g, b, a);
  mixpixel(_x - radius + 1, _y, r, g, b, a);
  mixpixel(_x, _y - radius + 1, r, g, b, a);

  float iy = 0;
  for (int x = 0; x <= radius * cos(M_PI / 4); x++)
  {
    //Вычисление точного значения координаты Y
    iy = (float)sqrt(radius * radius - x * x);
    int intens = (int)((FPart(iy) * 255.0) * a / 255);
    int inv_intens = (255 - (int)(FPart(iy) * 255)) * a / 255;
    int iiy = IPart(iy);

    //IV квадрант, Y
    mixpixel(_x - x - 1, _y + iiy, r, g, b, inv_intens);
    mixpixel(_x - x - 1, _y + iiy + 1, r, g, b, intens);
    //I квадрант, Y
    mixpixel(_x + x, _y + iiy, r, g, b, inv_intens);
    mixpixel(_x + x, _y + iiy + 1, r, g, b, intens);

    //I квадрант, X
    mixpixel(_x + iiy, _y + x, r, g, b, inv_intens);
    mixpixel(_x + iiy + 1, _y + x, r, g, b, intens);
    //II квадрант, X
    mixpixel(_x + iiy, _y - x - 1, r, g, b, inv_intens);
    mixpixel(_x + iiy + 1, _y - x - 1, r, g, b, intens);

    //С помощью инкремента устраняется ошибка смещения на 1 пиксел
    x++;
    //II квадрант, Y
    mixpixel(_x + x, _y - iiy, r, g, b, intens);
    mixpixel(_x + x, _y - iiy + 1, r, g, b, inv_intens);
    //III квадрант, Y
    mixpixel(_x - x, _y - iiy, r, g, b, intens);
    mixpixel(_x - x, _y - iiy + 1, r, g, b, inv_intens);
    //III квадрант, X
    mixpixel(_x - iiy, _y - x, r, g, b, intens);
    mixpixel(_x - iiy + 1, _y - x, r, g, b, inv_intens);
    //IV квадрант, X
    mixpixel(_x - iiy, _y + x, r, g, b, intens);
    mixpixel(_x - iiy + 1, _y + x, r, g, b, inv_intens);
    //Возврат значения
    x--;
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
    // printf("count: %f %f %f\n", from, to, atan);
    return to >= atan || atan >= from;
  }
  // return from <= to && from <= atan && atan <= to || from > to && to <= atan && atan <= from;
}

static void DrawArcLines(int _x, int _y, int radius, float from, float to, int r, int g, int b, int a)
{
  float fx = radius * cos(from + M_PI / 2);
  float fy = radius * sin(from + M_PI / 2);
  if (from == to)
  {
    DrawLineVU(_x, _y, fx + _x, fy + _y, r, g, b, a);
    return;
  }

  float tx = radius * cos(to + M_PI / 2);
  float ty = radius * sin(to + M_PI / 2);
  DrawLineVU(_x, _y, fx + _x, fy + _y, r, g, b, a);
  DrawLineVU(_x, _y, tx + _x, ty + _y, r, g, b, a);
  mixpixel(fx + _x, fy + _y, r, g, b, a);
  mixpixel(tx + _x, ty + _y, r, g, b, a);
}

static void DrawGradientArcLines(int _x, int _y, int radius, float from, float to, Color c1, Color c2, int a)
{
  float fx = radius * cos(from + M_PI / 2);
  float fy = radius * sin(from + M_PI / 2);
  if (from == to)
  {
    DrawGradientLineVU(_x, _y, fx + _x, fy + _y, c1, c2, a);
    return;
  }

  float tx = radius * cos(to + M_PI / 2);
  float ty = radius * sin(to + M_PI / 2);
  DrawGradientLineVU(_x, _y, fx + _x, fy + _y, c1, c2, a);
  DrawGradientLineVU(_x, _y, tx + _x, ty + _y, c1, c2, a);
  mixpixel(fx + _x, fy + _y, c1.r, c1.g, c1.b, a);
  mixpixel(tx + _x, ty + _y, c2.r, c2.g, c2.b, a);
}

static void DrawArcAA(int _x, int _y, int radius, float from, float to, int r, int g, int b, int a)
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
    //Вычисление точного значения координаты Y
    iy = (float)sqrt(radius * radius - x * x);
    int intens = (int)((FPart(iy) * 255.0) * a / 255);
    int inv_intens = (255 - (int)(FPart(iy) * 255)) * a / 255;
    int iiy = IPart(iy);

    //IV квадрант, Y
    int cx = _x - x;
    int cy = _y + iiy;
    if (sectorcont(cx - _x, cy - _y, radius, from, to))
    {
      mixpixel(cx, cy, r, g, b, inv_intens);
      mixpixel(cx, cy + 1, r, g, b, intens);
    }

    // //I квадрант, Y
    cx = _x + x;
    cy = _y + iiy;
    if (sectorcont(cx - _x, cy - _y, radius, from, to))
    {
      mixpixel(cx, cy, r, g, b, inv_intens);
      mixpixel(cx, cy + 1, r, g, b, intens);
    }

    // //I квадрант, X
    cx = _x + iiy;
    cy = _y + x;
    if (sectorcont(cx - _x, cy - _y, radius, from, to))
    {
      mixpixel(cx, cy, r, g, b, inv_intens);
      mixpixel(cx + 1, cy, r, g, b, intens);
    }
    // //II квадрант, X
    cx = _x + iiy;
    cy = _y - x;
    if (sectorcont(cx - _x, cy - _y, radius, from, to))
    {
      mixpixel(cx, cy, r, g, b, inv_intens);
      mixpixel(cx + 1, cy, r, g, b, intens);
    }

    // //С помощью инкремента устраняется ошибка смещения на 1 пиксел
    x++;
    // //II квадрант, Y
    cx = _x + x;
    cy = _y - iiy;
    if (sectorcont(cx - _x, cy - _y, radius, from, to))
    {
      mixpixel(cx, cy, r, g, b, intens);
      mixpixel(cx, cy + 1, r, g, b, inv_intens);
    }
    // //III квадрант, Y
    cx = _x - x;
    cy = _y - iiy;
    if (sectorcont(cx - _x, cy - _y, radius, from, to))
    {
      mixpixel(cx, cy, r, g, b, intens);
      mixpixel(cx, cy + 1, r, g, b, inv_intens);
    }
    // //III квадрант, X
    cx = _x - iiy;
    cy = _y - x;
    if (sectorcont(cx - _x, cy - _y, radius, from, to))
    {
      mixpixel(cx, cy, r, g, b, intens);
      mixpixel(cx + 1, cy, r, g, b, inv_intens);
    }
    // //IV квадрант, X
    cx = _x - iiy;
    cy = _y + x;
    if (sectorcont(cx - _x, cy - _y, radius, from, to))
    {
      mixpixel(cx, cy, r, g, b, intens);
      mixpixel(cx + 1, cy, r, g, b, inv_intens);
    }
    // //Возврат значения
    x--;
  }
}

static void DrawWuFilledCircle(int _x, int _y, int radius, int r, int g, int b, int a)
{
  //Установка пикселов, лежащих на осях системы координат с началом в центре
  // mixpixel(_x + radius, _y, r, g, b, a);
  // mixpixel(_x, _y + radius, r, g, b, a);
  mixpixel(_x - radius + 1, _y, r, g, b, a);
  mixpixel(_x, _y - radius + 1, r, g, b, a);

  float iy = 0;
  int last_iiy1 = 0;
  int last_iiy2 = 0;
  int last_iiy3 = 0;
  int last_iiy4 = 0;
  int first_x = radius * cos(M_PI / 4);
  int first_y = sqrt(radius * radius - first_x * first_x);
  for (int x = first_x; x >= 0; x--)
  {
    //Вычисление точного значения координаты Y
    iy = (float)sqrt(radius * radius - x * x);
    int intens = (int)((FPart(iy) * 255.0) * a / 255.0);
    int inv_intens = (255 - (int)(FPart(iy) * 255)) * a / 255;
    int iiy = IPart(iy);

    //IV квадрант, Y
    mixpixel(_x - x - 1, _y + iiy, r, g, b, inv_intens);
    mixpixel(_x - x - 1, _y + iiy + 1, r, g, b, intens);
    //I квадрант, Y
    mixpixel(_x + x, _y + iiy, r, g, b, inv_intens);
    mixpixel(_x + x, _y + iiy + 1, r, g, b, intens);
    if (_y + iiy != last_iiy1)
    {
      fill_mixed_line(_x - x, _x + x, _y + iiy, r, g, b, a);
    }

    // //I квадрант, X
    mixpixel(_x + iiy, _y + x, r, g, b, inv_intens);
    mixpixel(_x + iiy + 1, _y + x, r, g, b, intens);
    // //II квадрант, X
    mixpixel(_x + iiy, _y - x - 1, r, g, b, inv_intens);
    mixpixel(_x + iiy + 1, _y - x - 1, r, g, b, intens);

    //С помощью инкремента устраняется ошибка смещения на 1 пиксел
    x++;
    //II квадрант, Y
    mixpixel(_x + x, _y - iiy, r, g, b, intens);
    mixpixel(_x + x, _y - iiy + 1, r, g, b, inv_intens);
    //III квадрант, Y
    mixpixel(_x - x, _y - iiy, r, g, b, intens);
    mixpixel(_x - x, _y - iiy + 1, r, g, b, inv_intens);
    if (_y - iiy + 1 != last_iiy2)
    {
      fill_mixed_line(_x + x + 1, _x - x + 1, _y - iiy + 1, r, g, b, a);
    }
    //III квадрант, X
    mixpixel(_x - iiy, _y - x, r, g, b, intens);
    mixpixel(_x - iiy + 1, _y - x, r, g, b, inv_intens);
    if (_y - x > _y - first_y + 1 && _y - x != last_iiy3)
    {
      fill_mixed_line(_x - iiy + 1, _x + iiy + 1, _y - x, r, g, b, a);
    }
    // //IV квадрант, X
    mixpixel(_x - iiy, _y + x, r, g, b, intens);
    mixpixel(_x - iiy + 1, _y + x, r, g, b, inv_intens);
    if (_y + iiy > _y + first_y && _y + x != last_iiy4)
    {
      fill_mixed_line(_x - iiy + 1, _x + iiy + 1, _y + x - 1, r, g, b, a);
    }
    //Возврат значения
    x--;
    last_iiy1 = _y + iiy;
    last_iiy2 = _y - iiy + 1;
    last_iiy3 = _y - x + 1;
    last_iiy4 = _y + x - 1;
  }
}

//https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
static void DrawCircle(int32_t posx, int32_t posy, int32_t diameter, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
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

static void DrawFilledCircle(int32_t posx, int32_t posy, int32_t diameter, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
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

static int fill_texture(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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

static int draw_rect(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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

bool sortCoords(const Point &p1, const Point &p2)
{
  if (p1.y == p2.y)
    return p1.x < p2.x;
  return p1.y < p2.y;
}

static int draw_filled_rect(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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
    sort(vec.begin(), vec.end(), sortCoords);
    drawTriangle(vec[0].x, vec[0].y, vec[1].x, vec[1].y, vec[2].x, vec[2].y, r, g, b, a);
    drawTriangle(vec[1].x, vec[1].y, vec[2].x, vec[2].y, vec[3].x, vec[3].y, r, g, b, a);
  }

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_bezier(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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

static int draw_circle(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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
    DrawWuCircle(posx, posy, diameter / 2, r, g, b, a);
  }
  else
  {
    DrawCircle(posx, posy, diameter, r, g, b, a);
  }

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_filled_circle(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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
    DrawWuFilledCircle(posx, posy, diameter / 2, r, g, b, a);
  }
  else
  {
    DrawFilledCircle(posx, posy, diameter, r, g, b, a);
  }

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_thick_line(lua_State *L)
{
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  int32_t x0 = luaL_checknumber(L, 2);
  int32_t y0 = luaL_checknumber(L, 3);
  int32_t x1 = luaL_checknumber(L, 4);
  int32_t y1 = luaL_checknumber(L, 5);
  float w = luaL_checknumber(L, 6);
  uint32_t r = luaL_checknumber(L, 7);
  uint32_t g = luaL_checknumber(L, 8);
  uint32_t b = luaL_checknumber(L, 9);
  uint32_t a = 0;
  if (lua_isnumber(L, 10) == 1)
  {
    a = luaL_checknumber(L, 10);
  }

  plotLineWidth(x0, y0, x1, y1, w, r, g, b, a);

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_triangle(lua_State *L)
{
  int top = lua_gettop(L) + 4;

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

  drawTriangle(x0, y0, x1, y1, x2, y2, r, g, b, a);
  DrawLineVU(x0, y0, x1, y1, r, g, b, a);
  DrawLineVU(x1, y1, x2, y2, r, g, b, a);
  DrawLineVU(x2 - 1, y2, x0 - 1, y0, r, g, b, a);

  assert(top == lua_gettop(L));
  return 0;
}

static void DrawFixedArc(int32_t posx, int32_t posy, int32_t diameter, float &from, float &to, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
  float radius = diameter / 2;
  if (abs(from - to) >= M_PI_2)
  {
    float fx = radius * cos(from);
    float fy = radius * sin(from);
    // DrawLineVU(posx, posy, fx + posx, fy + posy, r, g, b, a);
    DrawWuCircle(posx, posy, radius, r, g, b, a);
  }
  else
  {
    from -= M_PI / 2;
    to -= M_PI / 2;
    from = from >= M_PI_2 ? from - (((int)(from / M_PI_2)) * M_PI_2) : from;
    to = to >= M_PI_2 ? to - (((int)(to / M_PI_2)) * M_PI_2) : to;
    if (to < from && to < 0 && from < 0)
    {
      float temp = to;
      to = from;
      from = temp;
    }

    from = from < 0 ? from + M_PI_2 : from;
    to = to < 0 ? to + M_PI_2 : to;
    DrawArcAA(posx, posy, radius, from, to, r, g, b, a);
  }
}

static int draw_arc(lua_State *L)
{
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  int32_t posx = luaL_checknumber(L, 2);
  int32_t posy = luaL_checknumber(L, 3);
  int32_t diameter = luaL_checknumber(L, 4);
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

  DrawFixedArc(posx, posy, diameter, from, to, r, g, b, a);
  DrawArcLines(posx, posy, diameter / 2, from, to, r, g, b, a);

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_filled_arc(lua_State *L)
{
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  int32_t posx = luaL_checknumber(L, 2);
  int32_t posy = luaL_checknumber(L, 3);
  int32_t diameter = luaL_checknumber(L, 4);
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
  DrawFixedArc(posx, posy, diameter, from, to, r, g, b, a);
  DrawArcLines(posx, posy, diameter / 2, from, to, r, g, b, a);

  float center = (from + to) / 2;
  center = from > to ? center - M_PI / 2 : center + M_PI / 2;

  fill_area(posx + diameter / 4 * cos(center), posy + diameter / 4 * sin(center), r, g, b, a);
  stop_record_points();

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_gradient_arc(lua_State *L)
{
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  int32_t posx = luaL_checknumber(L, 2);
  int32_t posy = luaL_checknumber(L, 3);
  int32_t diameter = luaL_checknumber(L, 4);
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
  DrawFixedArc(posx, posy, diameter, from, to, r2, g2, b2, a);
  DrawGradientArcLines(posx, posy, diameter / 2, from, to, c1, c2, a);

  float center = (from + to) / 2;
  center = from > to ? center - M_PI / 2 : center + M_PI / 2;

  Point center_point;
  center_point.x = posx;
  center_point.y = posy;

  gradient_fill_area(posx + diameter / 4 * cos(center), posy + diameter / 4 * sin(center), center_point, (diameter / 2) * (diameter / 2), c1, c2, a);
  stop_record_points();

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_line(lua_State *L)
{
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  int32_t x0 = luaL_checknumber(L, 2);
  int32_t y0 = luaL_checknumber(L, 3);
  int32_t x1 = luaL_checknumber(L, 4);
  int32_t y1 = luaL_checknumber(L, 5);
  uint32_t r = luaL_checknumber(L, 6);
  uint32_t g = luaL_checknumber(L, 7);
  uint32_t b = luaL_checknumber(L, 8);
  uint32_t a = 0;
  if (lua_isnumber(L, 9) == 1)
  {
    a = luaL_checknumber(L, 9);
  }
  bool antialiasing = false;
  if (lua_isboolean(L, 10) == 1)
  {
    antialiasing = lua_toboolean(L, 10);
  }

  if (antialiasing && buffer_info.channels == 4)
  {
    DrawLineVU(x0, y0, x1, y1, r, g, b, a);
  }
  else
  {
    DrawLine(x0, y0, x1, y1, r, g, b, a);
  }

  assert(top == lua_gettop(L));
  return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
    {"gradient_arc", draw_gradient_arc},
    {"triangle", draw_triangle},
    {"line", draw_line},
    {"line_thick", draw_thick_line},
    {"filled_arc", draw_filled_arc},
    {"arc", draw_arc},
    {"circle", draw_circle},
    {"filled_circle", draw_filled_circle},
    {"fill", fill_texture},
    {"rect", draw_rect},
    {"filled_rect", draw_filled_rect},
    {"pixel", draw_pixel},
    {"color", read_color},
    {"bezier", draw_bezier},
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
