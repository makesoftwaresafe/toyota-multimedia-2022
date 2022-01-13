#include "lua.h"
#include "lauxlib.h"
#include "llocal.h"

LUA_API int luaopen_local(lua_State *L)
{
#include "llocal_lua.c"
    return 0;
}

