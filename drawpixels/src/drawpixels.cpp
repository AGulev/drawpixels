// Extension lib defines
#define LIB_NAME "DrawPixels"
#define MODULE_NAME "drawpixels"

#define DLIB_LOG_DOMAIN LIB_NAME
#define M_PI 3.14159265358979323846

#include <dmsdk/sdk.h>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

struct BufferInfo
{
  dmBuffer::HBuffer buffer;
  int width;
  int height;
  int channels;
  uint8_t* bytes;
  uint32_t src_size;
};

BufferInfo buffer_info;

struct Point
{
  int x;
  int y;
};

static int in_buffer(int x, int y) {
  return (x >= 0) && (y >= 0) && (x < buffer_info.width) && (y < buffer_info.height);
}

static int xytoi(int x, int y) {
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x >= buffer_info.width) x = buffer_info.width - 1;
  if (y >= buffer_info.height) y = buffer_info.height - 1;
  return (y * buffer_info.width * buffer_info.channels) + (x * buffer_info.channels);
}

static void putpixel(int x, int y, int r, int g,int b, int a) {
  // ignore the pixel if it's outside the area of the buffer
  if (!in_buffer(x, y)) {
    return;
  }

  int i = xytoi(x, y);
  buffer_info.bytes[i] = r;
  buffer_info.bytes[i + 1] = g;
  buffer_info.bytes[i + 2] = b;
  if (buffer_info.channels == 4) {
    buffer_info.bytes[i + 3] = a;
  }
}

static void fill_line(int fromx, int tox, int y, int r, int g, int b, int a){
  if (fromx > tox) {
    int temp = fromx;
    fromx = tox;
    tox = temp;
  }
  fromx = fmax(0, fromx);
  tox = fmin(buffer_info.width-1, tox);
  int size = 10;
  //prepare line for memcpy
  int line_size = 10*buffer_info.channels;
  uint8_t* line = new uint8_t[line_size];
  for (int i=0; i<line_size; i +=buffer_info.channels){
    line[i] = r;
    line[i + 1] = g;
    line[i + 2] = b;
    if (buffer_info.channels == 4) {
      line[i + 3] = a;
    }
  }
  int start = xytoi(fromx, y);
  int end = xytoi(tox, y);
  int width = (end - start+buffer_info.channels);
  for(int i = start; i < end; i +=line_size) {
    if(width >= line_size){
      memcpy(&buffer_info.bytes[i], line, line_size);
    }else{
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
  for (int scanlineY = y1; scanlineY <= y2; scanlineY++) {
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
  for (int scanlineY = y3; scanlineY > y1; scanlineY--) {
    fill_line(curx1, curx2, scanlineY, r, g, b, a);
    curx1 -= invslope1;
    curx2 -= invslope2;
  }
}

static void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a){
  /* values should be sorted by y */
  /* here we know that y1 <= y2 <= y3 */
  /* check for trivial case of bottom-flat triangle */
  if (y2 == y3) {
    fillBottomFlatTriangle(x1, y1, x2, y2, x3, y3, r, g, b, a);
  }
  /* check for trivial case of top-flat triangle */
  else if (y1 == y2) {
    fillTopFlatTriangle(x1, y1, x2, y2, x3, y3, r, g, b, a);
  }
  else {
    /* general case - split the triangle in a topflat and bottom-flat one */
    int x4 = x1 + ((float(y2 - y1) / float(y3 - y1)) * (x3 - x1));
    int y4 = y2;
    fillBottomFlatTriangle(x1, y1, x2, y2, x4, y4, r, g, b, a);
    fillTopFlatTriangle(x2, y2, x4, y4, x3, y3, r, g, b, a);
  }
}

static void read_and_validate_buffer_info(lua_State* L, int index) {
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, "buffer");
  lua_getfield(L, index, "width");
  lua_getfield(L, index, "height");
  lua_getfield(L, index, "channels");
  dmScript::LuaHBuffer *lua_buffer = dmScript::CheckBuffer(L, -4);
  buffer_info.buffer = lua_buffer->m_Buffer;

  if (!dmBuffer::IsBufferValid(buffer_info.buffer)) {
    luaL_error(L, "Buffer is invalid");
  }

  dmBuffer::Result result = dmBuffer::GetBytes(buffer_info.buffer, (void**)&buffer_info.bytes, &buffer_info.src_size);
  if (result != dmBuffer::RESULT_OK) {
    luaL_error(L, "Buffer is invalid");
  }

  buffer_info.width = lua_tointeger(L, -3);
  if (buffer_info.width == 0) {
    luaL_error(L, "'width' of the buffer should be an integer and > 0");
  }

  buffer_info.height = lua_tointeger(L, -2);
  if (buffer_info.height == 0) {
    luaL_error(L, "'height' of the buffer should be an integer and > 0");
  }

  buffer_info.channels = lua_tointeger(L, -1);
  if (buffer_info.channels == 0) {
    luaL_error(L, "'channels' of should be an integer and 3 or 4");
  }
}

static int draw_line(lua_State* L) {
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

  // https://gist.github.com/bert/1085538#file-plot_line-c
  int dx =  abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1; 
  int err = dx + dy, e2; /* error value e_xy */

  for (;;){  /* loop */
    putpixel(x0, y0, r, g, b, a);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
    if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
  }

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_pixel(lua_State* L) {
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

static int read_color(lua_State* L) {
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  int32_t x = luaL_checknumber(L, 2);
  int32_t y = luaL_checknumber(L, 3);

  if (!in_buffer(x, y)) {
    assert(top == lua_gettop(L));
    return 0;
  }

  int i = xytoi(x, y);
  lua_pushnumber(L, buffer_info.bytes[i]);
  lua_pushnumber(L, buffer_info.bytes[i + 1]);
  lua_pushnumber(L, buffer_info.bytes[i + 2]);
  if (buffer_info.channels == 4) {
    lua_pushnumber(L, buffer_info.bytes[i + 3]);
  }
  assert(top + buffer_info.channels == lua_gettop(L));
  return buffer_info.channels;
}

//https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
static int draw_circle(lua_State* L) {
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

  if (diameter > 0) {
    int radius = diameter/2;
    int x = radius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    while (x >= y)
    {
      putpixel(posx + x, posy + y, r, g, b, a);
      putpixel(posx + y, posy + x, r, g, b, a);
      putpixel(posx - y, posy + x, r, g, b, a);
      putpixel(posx - x, posy + y, r, g, b, a);
      putpixel(posx - x, posy - y, r, g, b, a);
      putpixel(posx - y, posy - x, r, g, b, a);
      putpixel(posx + y, posy - x, r, g, b, a);
      putpixel(posx + x, posy - y, r, g, b, a);

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

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_filled_circle(lua_State* L) {
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

  if(diameter > 0) {
    int radius = diameter/2;
    int x = radius-1;
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

  assert(top == lua_gettop(L));
  return 0;
}

static int fill_texture(lua_State* L) {
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
  int line_size = buffer_info.width*buffer_info.channels;
  uint8_t* line = new uint8_t[line_size];
  for (int i=0; i<line_size; i +=buffer_info.channels){
    line[i] = r;
    line[i + 1] = g;
    line[i + 2] = b;
    if (buffer_info.channels == 4) {
      line[i + 3] = a;
    }
  }
  for(int i = 0; i < buffer_info.src_size; i +=line_size) {
    memcpy(&buffer_info.bytes[i], line, line_size);
  }
  delete[] line;
  assert(top == lua_gettop(L));
  return 0;
}

static int draw_rect(lua_State* L) {
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

  int half_size_x = sizex/2;
  int half_size_y = sizey/2;
  int newposx = 0;
  int newposy = 0;
  for(int y = -half_size_y; y < half_size_y; y++) {
    if (y == -half_size_y || y == half_size_y - 1 ) {
      for(int x = -half_size_x; x < half_size_x; x++) {
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

bool sortByY(const Point &f, const Point &t){
  return (f.y < t.y);
}

static int draw_filled_rect(lua_State* L) {
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
  uint32_t angle = 0;
  if (lua_isnumber(L, 10) == 1)
  {
    angle = luaL_checknumber(L, 10);
  }

  int half_size_x = sizex/2;
  int half_size_y = sizey/2;
  if (angle == 0) {
    int newposx = 0;
    int newposy = 0;
    for(int x = -half_size_x; x < half_size_x; x++) {
      for(int y = -half_size_y; y < half_size_y; y++) {
        newposx = x + posx;
        newposy = y + posy;
        putpixel(newposx, newposy, r, g, b, a);
      }
    }
  }
  else{
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
    for (int i = 0; i < 4; i++) {
      vec[i].x += posx;
      vec[i].y += posy;
    }
    sort(vec.begin(),vec.end(), sortByY);
    drawTriangle(vec[0].x, vec[0].y, vec[1].x, vec[1].y, vec[2].x, vec[2].y, r, g, b, a);
    drawTriangle(vec[1].x, vec[1].y, vec[2].x, vec[2].y, vec[3].x, vec[3].y, r, g, b, a);
  }

  assert(top == lua_gettop(L));
  return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
  {"line", draw_line},
  {"circle", draw_circle},
  {"filled_circle", draw_filled_circle},
  {"fill", fill_texture},
  {"rect", draw_rect},
  {"filled_rect", draw_filled_rect},
  {"pixel", draw_pixel},
  {"color", read_color},
  {0, 0}
};

static void LuaInit(lua_State* L) {
  int top = lua_gettop(L);
  luaL_register(L, MODULE_NAME, Module_methods);
  lua_pop(L, 1);
  assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializeDrawPixelsExtension(dmExtension::AppParams* params) {
  return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeDrawPixelsExtension(dmExtension::Params* params) {
  LuaInit(params->m_L);
  printf("Registered %s Extension\n", MODULE_NAME);
  return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeDrawPixelsExtension(dmExtension::AppParams* params) {
  return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeDrawPixelsExtension(dmExtension::Params* params) {
  return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(DrawPixels, LIB_NAME, AppInitializeDrawPixelsExtension, AppFinalizeDrawPixelsExtension, InitializeDrawPixelsExtension, 0, 0, FinalizeDrawPixelsExtension)
