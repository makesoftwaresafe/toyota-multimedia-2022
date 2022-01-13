#ifndef LREXLIB_H
#define LREXLIB_H

int luaopen_rex_pcre(lua_State *L)
	/*@modifies L @*/;
int luaopen_rex_posix(lua_State *L)
	/*@modifies L @*/;

#endif
