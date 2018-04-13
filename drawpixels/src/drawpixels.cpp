// Extension lib defines
#define LIB_NAME "DrawPixels"
#define MODULE_NAME "drawpixels"

#define DLIB_LOG_DOMAIN LIB_NAME

#include <dmsdk/sdk.h>
#include <math.h>

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

static int xytoi(int x, int y) {
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x >= buffer_info.width) x = buffer_info.width - 1;
  if (y >= buffer_info.height) y = buffer_info.height - 1;
  return (y * buffer_info.width * buffer_info.channels) + (x * buffer_info.channels);
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

static void putpixel(int x, int y, int r, int g,int b, int a){
  int i = xytoi(x, y);
  buffer_info.bytes[i] = r;
  buffer_info.bytes[i + 1] = g;
  buffer_info.bytes[i + 2] = b;
  if (buffer_info.channels == 4) {
    buffer_info.bytes[i + 3] = a;
  }
}

static void fill_line(int from, int to, int r, int g, int b, int a){
  for (int i = from; i <= to; i += buffer_info.channels) {
    buffer_info.bytes[i] = r;
    buffer_info.bytes[i + 1] = g;
    buffer_info.bytes[i + 2] = b;
    if (buffer_info.channels == 4) {
      buffer_info.bytes[i + 3] = a;
    }
  }
}

//https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
static int draw_circle(lua_State* L) {
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  uint32_t posx = luaL_checknumber(L, 2);
  uint32_t posy = luaL_checknumber(L, 3);
  uint32_t radius = luaL_checknumber(L, 4);
  uint32_t r = luaL_checknumber(L, 5);
  uint32_t g = luaL_checknumber(L, 6);
  uint32_t b = luaL_checknumber(L, 7);
  uint32_t a = 0;
  if (lua_isnumber(L, 8) == 1)
  {
    a = luaL_checknumber(L, 8);
  }
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

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_filled_circle(lua_State* L) {
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  uint32_t posx = luaL_checknumber(L, 2);
  uint32_t posy = luaL_checknumber(L, 3);
  uint32_t diameter = luaL_checknumber(L, 4);
  uint32_t r = luaL_checknumber(L, 5);
  uint32_t g = luaL_checknumber(L, 6);
  uint32_t b = luaL_checknumber(L, 7);
  uint32_t a = 0;
  if (lua_isnumber(L, 8) == 1)
  {
    a = luaL_checknumber(L, 8);
  }

  int radius = diameter/2;
  int x = radius-1;
  int y = 0;
  int dx = 1;
  int dy = 1;
  int err = dx - (radius << 1);
  while (x >= y)
  {
    fill_line(xytoi(posx - x, posy + y), xytoi(posx + x, posy + y), r, g, b, a);
    fill_line(xytoi(posx - y, posy + x), xytoi(posx + y, posy + x), r, g, b, a);
    fill_line(xytoi(posx - x, posy - y), xytoi(posx + x, posy - y), r, g, b, a);
    fill_line(xytoi(posx - y, posy - x), xytoi(posx + y, posy - x), r, g, b, a);

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

  assert(top == lua_gettop(L));
  return 0;
}

static int fill_texture(lua_State* L) {
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  uint32_t r = luaL_checknumber(L, 2);
  uint32_t g = luaL_checknumber(L, 3);
  uint32_t b = luaL_checknumber(L, 4);
  uint32_t a = 0;
  if (lua_isnumber(L, 5) == 1)
  {
    a = luaL_checknumber(L, 5);
  }
  for(int i = 0; i < buffer_info.src_size; i += buffer_info.channels) {
    buffer_info.bytes[i] = r;
    buffer_info.bytes[i + 1] = g;
    buffer_info.bytes[i + 2] = b;
    if (buffer_info.channels == 4) {
      buffer_info.bytes[i + 3] = a;
    }
  }

  assert(top == lua_gettop(L));
  return 0;
}

static int draw_rect(lua_State* L) {
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  uint32_t posx = luaL_checknumber(L, 2);
  uint32_t posy = luaL_checknumber(L, 3);
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
        int i = xytoi(newposx, newposy);
        buffer_info.bytes[i] = r;
        buffer_info.bytes[i + 1] = g;
        buffer_info.bytes[i + 2] = b;
        if (buffer_info.channels == 4) {
          buffer_info.bytes[i + 3] = a;
        }
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

static int draw_filled_rect(lua_State* L) {
  int top = lua_gettop(L) + 4;

  read_and_validate_buffer_info(L, 1);
  uint32_t posx = luaL_checknumber(L, 2);
  uint32_t posy = luaL_checknumber(L, 3);
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
  for(int x = -half_size_x; x < half_size_x; x++) {
    for(int y = -half_size_y; y < half_size_y; y++) {
      newposx = x + posx;
      newposy = y + posy;
      int i = xytoi(newposx, newposy);
      buffer_info.bytes[i] = r;
      buffer_info.bytes[i + 1] = g;
      buffer_info.bytes[i + 2] = b;
      if (buffer_info.channels == 4) {
        buffer_info.bytes[i + 3] = a;
      }
    }
  }

  assert(top == lua_gettop(L));
  return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
  {"circle", draw_circle},
  {"filled_circle", draw_filled_circle},
  {"fill", fill_texture},
  {"rect", draw_rect},
  {"filled_rect", draw_filled_rect},
  {0, 0}
};

static void LuaInit(lua_State* L) {
  int top = lua_gettop(L);
  luaL_register(L, MODULE_NAME, Module_methods);
  lua_pop(L, 1);
  assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializeImpExtension(dmExtension::AppParams* params) {
  return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeImpExtension(dmExtension::Params* params) {
  LuaInit(params->m_L);
  printf("Registered %s Extension\n", MODULE_NAME);
  return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeImpExtension(dmExtension::AppParams* params) {
  return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeImpExtension(dmExtension::Params* params) {
  return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(DrawPixels, LIB_NAME, AppInitializeImpExtension, AppFinalizeImpExtension, InitializeImpExtension, 0, 0, FinalizeImpExtension)
