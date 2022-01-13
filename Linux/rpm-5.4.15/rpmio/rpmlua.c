/*@-moduncon -mustmod -realcompare -sizeoftype @*/
#include "system.h"

#ifdef	WITH_LUA
#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmmacro.h>
#include <rpmlog.h>
#include <rpmurl.h>
#include <rpmhook.h>
#include <rpmcb.h>
#include <argv.h>
#include <popt.h>		/* XXX poptSaneFile test */

#ifdef __cplusplus
extern "C" {
#endif

#include <lauxlib.h>
#include <lualib.h>

#ifdef	WITH_SYCK
LUALIB_API int luaopen_syck(lua_State *L)
	/*@modifies L @*/;
#endif	/* WITH_SYCK */
#ifdef WITH_LUA_INTERNAL
#include <llocal.h>
#include <lposix.h>
#include <lrexlib.h>
#include <luuid.h>
#include <lwrs.h>
#ifdef	USE_LUA_CRYPTO		/* XXX external lua modules instead. */
#include <lcrypto.h>
#include <lxplib.h>
#endif
#ifdef	USE_LUA_SOCKET		/* XXX external lua modules instead. */
#include <luasocket.h>
#endif
#endif

#ifdef __cplusplus
}
#endif

#define _RPMLUA_INTERNAL
#include "rpmlua.h"

#include "debug.h"

/*@access rpmiob @*/

#else /* WITH_LUA */
#include <rpmio.h>
#endif

/*@unchecked@*/
int _rpmlua_debug = 0;

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmluaPool = NULL;

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmluavPool = NULL;

#ifdef	WITH_LUA

/* XXX lua-5.2.0 retrofit destruction area. */
#if LUA_VERSION_NUM >= 502
LUALIB_API int luaopen_posix_c (lua_State *L);
#endif

#if !defined(HAVE_VSNPRINTF)
static inline int vsnprintf(char * buf, /*@unused@*/ size_t nb,
			    const char * fmt, va_list ap)
{
    return vsprintf(buf, fmt, ap);
}
#endif

#define INITSTATE(_lua, lua) \
    rpmlua lua = _lua ? _lua : \
	    (globalLuaState ? globalLuaState : \
			/*@-mods@*/ \
			(globalLuaState = rpmluaNew()) \
			/*@=mods@*/ \
	    )

/*@only@*/ /*@unchecked@*/ /*@relnull@*/
static rpmlua globalLuaState;

static int luaopen_rpm(lua_State *L)
	/*@modifies L @*/;
static int rpm_print(lua_State *L)
	/*@globals fileSystem @*/
	/*@modifies L, fileSystem @*/;

/*@unchecked@*/ /*@observer@*/
const char * rpmluaFiles = RPMLUAFILES;

/*@unchecked@*/ /*@observer@*/
const char * rpmluaPath = "%{?_rpmhome}%{!?_rpmhome:" USRLIBRPM "}/lua/?.lua";

rpmlua rpmluaGetGlobalState(void)
{
/*@-globstate@*/
    return globalLuaState;
/*@=globstate@*/
}

static void rpmluaFini(void * _lua)
	/*@globals globalLuaState @*/
	/*@modifies globalLuaState @*/
{
    rpmlua lua = (rpmlua) _lua;

    if (lua->L) lua_close(lua->L);
    lua->L = NULL;
    lua->printbuf = _free(lua->printbuf);
}

static rpmlua rpmluaGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmluaPool, fileSystem @*/
        /*@modifies pool, _rpmluaPool, fileSystem @*/
{
    rpmlua lua;

    if (_rpmluaPool == NULL) {
        _rpmluaPool = rpmioNewPool("lua", sizeof(*lua), -1, _rpmlua_debug,
                        NULL, NULL, rpmluaFini);
        pool = _rpmluaPool;
    }
    return (rpmlua) rpmioGetPool(pool, sizeof(*lua));
}

void *rpmluaFree(rpmlua lua)
{
    if (lua == NULL) lua = globalLuaState;
    (void)rpmioFreePoolItem((rpmioItem)lua, __FUNCTION__, __FILE__, __LINE__);
    if (lua == globalLuaState) globalLuaState = NULL;
    return NULL;
}

/*@-globs -mods@*/	/* XXX hide rpmGlobalMacroContext mods for now. */
rpmlua rpmluaNew(void)
{
    rpmlua lua = rpmluaGetPool(_rpmluaPool);
    lua_State *L = luaL_newstate();
    /*@-readonlytrans -nullassign @*/
    /*@observer@*/ /*@unchecked@*/
    static const luaL_Reg lualibs[] = {
#ifdef	WITH_SYCK
	{"lsyck", luaopen_syck},
#endif	/* WITH_SYCK */
	/* local LUA libraries (RPM only) */
#ifdef WITH_LUA_INTERNAL
	{"posix", luaopen_posix_c},
	{"rex_posix", luaopen_rex_posix},
	{"rex_pcre", luaopen_rex_pcre},
	{"uuid", luaopen_uuid},
#ifdef	DYING	/* XXX not currently internal */
	{"wrs", luaopen_wrs},
#endif
#ifdef	USE_LUA_CRYPTO		/* XXX external lua modules instead. */
	{"crypto", luaopen_crypto},
	{"lxp", luaopen_lxp},
#endif
#ifdef	USE_LUA_SOCKET		/* XXX external lua modules instead. */
	{"socket", luaopen_socket_core},
#endif
	{"local", luaopen_local},
#endif
	{"rpm", luaopen_rpm},
	{NULL, NULL},
    };
    /*@=readonlytrans =nullassign @*/
    /*@observer@*/ /*@unchecked@*/
    const luaL_Reg *lib = lualibs;
    char *path_buf;
    char *path_next;
    char *path;

    lua->L = L;
    lua->pushsize = 0;
    lua->storeprint = 0;
    /* XXX TODO: use an rpmiob here. */
    lua->printbufsize = 0;
    lua->printbufused = 0;
    lua->printbuf = NULL;

    luaL_openlibs(L);

    for (; lib->name; lib++) {
	luaL_requiref(L, lib->name, lib->func, 1);
    }

    {	const char * _lua_path = rpmGetPath(rpmluaPath, NULL);
 	if (_lua_path != NULL) {
	    lua_pushliteral(L, "LUA_PATH");
	    lua_pushstring(L, _lua_path);
	    _lua_path = _free(_lua_path);
	}
    }

#if defined(LUA_GLOBALSINDEX)
    lua_rawset(L, LUA_GLOBALSINDEX);
#else
    lua_pushglobaltable(L);
#endif
    lua_pushliteral(L, "print");
    lua_pushcfunction(L, rpm_print);

#if defined(LUA_GLOBALSINDEX)
    lua_rawset(L, LUA_GLOBALSINDEX);
#else
    lua_pushglobaltable(L);
#endif
    rpmluaSetData(lua, "lua", lua);

    /* load all standard RPM Lua script files */
    path_buf = xstrdup(rpmluaFiles);
    for (path = path_buf; path != NULL && *path != '\0'; path = path_next) {
        const char **av;
        struct stat st;
        int ac, i;

        /* locate start of next path element */
        path_next = strchr(path, ':');
        if (path_next != NULL && *path_next == ':')
            *path_next++ = '\0';
        else
            path_next = path + strlen(path);

        /* glob-expand the path element */
        ac = 0;
        av = NULL;
        if ((i = rpmGlob(path, &ac, &av)) != 0)
            continue;

        /* work-off each resulting file from the path element */
        for (i = 0; i < ac; i++) {
            const char *fn = av[i];
            if (fn[0] == '@' /* attention */) {
                fn++;
#if defined(RPM_VENDOR_OPENPKG) /* stick-with-rpm-file-sanity-checking */ || \
    !defined(POPT_ERROR_BADCONFIG)	/* XXX POPT 1.15 retrofit */
		if (!rpmSecuritySaneFile(fn))
#else
		if (!poptSaneFile(fn))
#endif
		{
                    rpmlog(RPMLOG_WARNING, "existing RPM Lua script file \"%s\" considered INSECURE -- not loaded\n", fn);
                    /*@innercontinue@*/ continue;
                }
            }
            if (Stat(fn, &st) != -1)
                (void)rpmluaRunScriptFile(lua, fn);
            av[i] = _free(av[i]);
        }
        av = _free(av);
    }
    path_buf = _free(path_buf);

    return ((rpmlua)rpmioLinkPoolItem((rpmioItem)lua, __FUNCTION__, __FILE__, __LINE__));
}
/*@=globs =mods@*/

void rpmluaSetData(rpmlua _lua, const char *key, const void *data)
{
    INITSTATE(_lua, lua);
    lua_State *L = lua->L;
    lua_pushliteral(L, "rpm_");
    lua_pushstring(L, key);
    lua_concat(L, 2);
    if (data == NULL)
	lua_pushnil(L);
    else
	lua_pushlightuserdata(L, (void *)data);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

/*@null@*/
static void *getdata(lua_State *L, const char *key)
	/*@modifies L @*/
{
    void *ret = NULL;
    lua_pushliteral(L, "rpm_");
    lua_pushstring(L, key);
    lua_concat(L, 2);
    lua_rawget(L, LUA_REGISTRYINDEX);
    if (lua_islightuserdata(L, -1))
	ret = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ret;
}

void *rpmluaGetData(rpmlua _lua, const char *key)
{
    INITSTATE(_lua, lua);
    return getdata(lua->L, key);
}

void rpmluaSetPrintBuffer(rpmlua _lua, int flag)
{
    INITSTATE(_lua, lua);
    lua->storeprint = flag;
    lua->printbuf = _free(lua->printbuf);
    lua->printbufsize = 0;
    lua->printbufused = 0;
}

const char *rpmluaGetPrintBuffer(rpmlua _lua)
{
    INITSTATE(_lua, lua);
    return lua->printbuf;
}

static int pushvar(lua_State *L, rpmluavType type, void *value)
	/*@modifies L @*/
{
    int ret = 0;
    switch (type) {
	case RPMLUAV_NIL:
	    lua_pushnil(L);
	    break;
	case RPMLUAV_STRING:
	    lua_pushstring(L, *((char **)value));
	    break;
	case RPMLUAV_NUMBER:
	    lua_pushnumber(L, *((double *)value));
	    break;
	default:
	    ret = -1;
	    break;
    }
    return ret;
}

void rpmluaSetVar(rpmlua _lua, rpmluav var)
{
    INITSTATE(_lua, lua);
    lua_State *L = lua->L;
    if (var->listmode && lua->pushsize > 0) {
	if (var->keyType != RPMLUAV_NUMBER || var->key.num == (double)0) {
	    var->keyType = RPMLUAV_NUMBER;
	    var->key.num = (double) lua_rawlen(L, -1);
	}
	var->key.num++;
    }
    if (!var->listmode || lua->pushsize > 0) {
#if defined(LUA_GLOBALSINDEX)
	if (lua->pushsize == 0)
	    lua_pushvalue(L, LUA_GLOBALSINDEX);
#endif
	if (pushvar(L, var->keyType, &var->key) != -1) {
	    if (pushvar(L, var->valueType, &var->value) != -1)
		lua_rawset(L, -3);
	    else
		lua_pop(L, 1);
	}
	if (lua->pushsize == 0)
	    lua_pop(L, 1);
    }
}

static void popvar(lua_State *L, rpmluavType *type, void *value)
	/*@modifies L, *type, *value @*/
{
    switch (lua_type(L, -1)) {
    case LUA_TSTRING:
	*type = RPMLUAV_STRING;
/*@-observertrans -dependenttrans @*/
	*((const char **)value) = lua_tostring(L, -1);
/*@=observertrans =dependenttrans @*/
	break;
    case LUA_TNUMBER:
	*type = RPMLUAV_NUMBER;
	*((double *)value) = lua_tonumber(L, -1);
	break;
    default:
	*type = RPMLUAV_NIL;
	*((void **)value) = NULL;
	break;
    }
    lua_pop(L, 1);
}

void rpmluaGetVar(rpmlua _lua, rpmluav var)
{
    INITSTATE(_lua, lua);
    lua_State *L = lua->L;
    if (!var->listmode) {
#if defined(LUA_GLOBALSINDEX)
	if (lua->pushsize == 0)
	    lua_pushvalue(L, LUA_GLOBALSINDEX);
#else
	if (lua->pushsize == 0)
	    lua_pushglobaltable(L);
#endif
	if (pushvar(L, var->keyType, &var->key) != -1) {
	    lua_rawget(L, -2);
	    popvar(L, &var->valueType, &var->value);
	}
	if (lua->pushsize == 0)
	    lua_pop(L, 1);
    } else if (lua->pushsize > 0) {
	(void) pushvar(L, var->keyType, &var->key);
	if (lua_next(L, -2) != 0)
	    popvar(L, &var->valueType, &var->value);
    }
}

#define FINDKEY_RETURN 0
#define FINDKEY_CREATE 1
#define FINDKEY_REMOVE 2
static int findkey(lua_State *L, int oper, const char *key, va_list va)
	/*@modifies L @*/
{
    char buf[BUFSIZ];
    const char *s, *e;
    int ret = 0;
    (void) vsnprintf(buf, sizeof(buf), key, va);
    s = e = buf;
#if defined(LUA_GLOBALSINDEX)
    lua_pushvalue(L, LUA_GLOBALSINDEX);
#else
    lua_pushglobaltable(L);
#endif
    for (;;) {
	if (*e == '\0' || *e == '.') {
	    if (e != s) {
		lua_pushlstring(L, s, e-s);
		switch (oper) {
		case FINDKEY_REMOVE:
		    if (*e == '\0') {
			lua_pushnil(L);
			lua_rawset(L, -3);
			lua_pop(L, 1);
			/*@switchbreak@*/ break;
		    }
		    /*@fallthrough@*/
		case FINDKEY_RETURN:
		    lua_rawget(L, -2);
		    lua_remove(L, -2);
		    /*@switchbreak@*/ break;
		case FINDKEY_CREATE:
		    lua_rawget(L, -2);
		    if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			lua_newtable(L);
			lua_pushlstring(L, s, e-s);
			lua_pushvalue(L, -2);
			lua_rawset(L, -4);
		    }
		    lua_remove(L, -2);
		    /*@switchbreak@*/ break;
		}
	    }
	    if (*e == '\0')
		break;
	    if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		ret = -1;
		break;
	    }
	    s = e+1;
	}
	e++;
    }

    return ret;
}

void rpmluaDelVar(rpmlua _lua, const char *key, ...)
{
    INITSTATE(_lua, lua);
    va_list va;
    va_start(va, key);
    (void) findkey(lua->L, FINDKEY_REMOVE, key, va);
    va_end(va);
}

int rpmluaVarExists(rpmlua _lua, const char *key, ...)
{
    INITSTATE(_lua, lua);
    lua_State *L = lua->L;
    int ret = 0;
    va_list va;
    va_start(va, key);
    if (findkey(L, FINDKEY_RETURN, key, va) == 0) {
	if (!lua_isnil(L, -1))
	    ret = 1;
	lua_pop(L, 1);
    }
    va_end(va);
    return ret;
}

void rpmluaPushTable(rpmlua _lua, const char *key, ...)
{
    INITSTATE(_lua, lua);
    va_list va;
    va_start(va, key);
    (void) findkey(lua->L, FINDKEY_CREATE, key, va);
    lua->pushsize++;
    va_end(va);
}

void rpmluaPop(rpmlua _lua)
{
    INITSTATE(_lua, lua);
    assert(lua->pushsize > 0);
    lua->pushsize--;
    lua_pop(lua->L, 1);
}

void *rpmluavFree(rpmluav var)
{
    (void)rpmioFreePoolItem((rpmioItem)var, __FUNCTION__, __FILE__, __LINE__);
    return NULL;
}

static rpmluav rpmluavGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmluavPool, fileSystem @*/
        /*@modifies pool, _rpmluavPool, fileSystem @*/
{
    rpmluav luav;

    if (_rpmluavPool == NULL) {
        _rpmluavPool = rpmioNewPool("luav", sizeof(*luav), -1, _rpmlua_debug,
                        NULL, NULL, NULL);
        pool = _rpmluavPool;
    }
    return (rpmluav) rpmioGetPool(pool, sizeof(*luav));
}

rpmluav rpmluavNew(void)
{
    rpmluav var = rpmluavGetPool(_rpmluavPool);
    var->keyType = RPMLUAV_NIL;
    var->valueType = RPMLUAV_NIL;
    var->key.ptr = NULL;
    var->value.ptr = NULL;
    var->listmode = 0;
    return ((rpmluav)rpmioLinkPoolItem((rpmioItem)var, __FUNCTION__, __FILE__, __LINE__));
}

void rpmluavSetListMode(rpmluav var, int flag)
{
    var->listmode = flag;
    var->keyType = RPMLUAV_NIL;
}

void rpmluavSetKey(rpmluav var, rpmluavType type, const void *value)
{
    var->keyType = type;
/*@-assignexpose -temptrans @*/
    switch (type) {
	case RPMLUAV_NUMBER:
	    var->key.num = *((double *)value);
	    break;
	case RPMLUAV_STRING:
	    var->key.str = (char *)value;
	    break;
	default:
	    break;
    }
/*@=assignexpose =temptrans @*/
}

void rpmluavSetValue(rpmluav var, rpmluavType type, const void *value)
{
    var->valueType = type;
/*@-assignexpose -temptrans @*/
    switch (type) {
	case RPMLUAV_NUMBER:
	    var->value.num = *((const double *)value);
	    break;
	case RPMLUAV_STRING:
	    var->value.str = (const char *)value;
	    break;
	default:
	    break;
    }
/*@=assignexpose =temptrans @*/
}

void rpmluavGetKey(rpmluav var, rpmluavType *type, void **value)
{
    *type = var->keyType;
/*@-onlytrans@*/
    switch (var->keyType) {
	case RPMLUAV_NUMBER:
	    *((double **)value) = &var->key.num;
	    break;
	case RPMLUAV_STRING:
	    *((const char **)value) = var->key.str;
	    break;
	default:
	    break;
    }
/*@=onlytrans@*/
}

void rpmluavGetValue(rpmluav var, rpmluavType *type, void **value)
{
    *type = var->valueType;
/*@-onlytrans@*/
    switch (var->valueType) {
	case RPMLUAV_NUMBER:
	    *((double **)value) = &var->value.num;
	    break;
	case RPMLUAV_STRING:
	    *((const char **)value) = var->value.str;
	    break;
	default:
	    break;
    }
/*@=onlytrans@*/
}

void rpmluavSetKeyNum(rpmluav var, double value)
{
    rpmluavSetKey(var, RPMLUAV_NUMBER, &value);
}

void rpmluavSetValueNum(rpmluav var, double value)
{
    rpmluavSetValue(var, RPMLUAV_NUMBER, &value);
}

double rpmluavGetKeyNum(rpmluav var)
{
    rpmluavType type;
    void *value;
    rpmluavGetKey(var, &type, &value);
    if (type == RPMLUAV_NUMBER)
	return *((double *)value);
    return (double) 0;
}

double rpmluavGetValueNum(rpmluav var)
{
    rpmluavType type;
    void *value;
    rpmluavGetValue(var, &type, &value);
    if (type == RPMLUAV_NUMBER)
	return *((double *)value);
    return (double) 0;
}

int rpmluavKeyIsNum(rpmluav var)
{
    return (var->keyType == RPMLUAV_NUMBER) ? 1 : 0;
}

int rpmluavValueIsNum(rpmluav var)
{
    return (var->valueType == RPMLUAV_NUMBER) ? 1 : 0;
}

int rpmluaCheckScript(rpmlua _lua, const char *script, const char *name)
{
    INITSTATE(_lua, lua);
    lua_State *L = lua->L;
    int ret = 0;
    if (name == NULL)
	name = "<lua>";
    if (luaL_loadbuffer(L, script, strlen(script), name) != 0) {
	rpmlog(RPMLOG_ERR,
		_("invalid syntax in Lua scriptlet: %s\n"),
		  lua_tostring(L, -1));
	ret = -1;
    }
    lua_pop(L, 1); /* Error or chunk. */
    return ret;
}

int rpmluaRunScript(rpmlua _lua, const char *script, const char *name)
{
    INITSTATE(_lua, lua);
    lua_State *L = lua->L;
    int ret = 0;
    if (name == NULL)
	name = "<lua>";
    if (luaL_loadbuffer(L, script, strlen(script), name) != 0) {
	rpmlog(RPMLOG_ERR, _("invalid syntax in Lua script: %s\n"),
		 lua_tostring(L, -1));
	lua_pop(L, 1);
	ret = -1;
    } else if (lua_pcall(L, 0, 0, 0) != 0) {
	rpmlog(RPMLOG_ERR, _("Lua script failed: %s\n"),
		 lua_tostring(L, -1));
	lua_pop(L, 1);
	ret = -1;
    }
    return ret;
}

int rpmluaRunScriptFile(rpmlua _lua, const char *filename)
{
    INITSTATE(_lua, lua);
    lua_State *L = lua->L;
    int ret = 0;
    if (luaL_loadfile(L, filename) != 0) {
	rpmlog(RPMLOG_ERR, _("invalid syntax in Lua file: %s\n"),
		 lua_tostring(L, -1));
	lua_pop(L, 1);
	ret = -1;
    } else if (lua_pcall(L, 0, 0, 0) != 0) {
	rpmlog(RPMLOG_ERR, _("Lua script failed: %s\n"),
		 lua_tostring(L, -1));
	lua_pop(L, 1);
	ret = -1;
    }
    return ret;
}

/* From lua.c */
static int rpmluaReadline(lua_State *L, const char *prompt)
	/*@globals fileSystem @*/
	/*@modifies L, fileSystem @*/
{
   static char buffer[1024];
   if (prompt) {
      (void) fputs(prompt, stdout);
      (void) fflush(stdout);
   }
   if (fgets(buffer, (int)sizeof(buffer), stdin) == NULL) {
      return 0;  /* read fails */
   } else {
      lua_pushstring(L, buffer);
      return 1;
   }
}

/* Based on lua.c */
static void _rpmluaInteractive(lua_State *L)
	/*@globals fileSystem @*/
	/*@modifies L, fileSystem @*/
{
   (void) fputs("\n", stdout);
   printf("RPM Interactive %s Interpreter\n", LUA_VERSION);
   for (;;) {
      int rc = 0;

      if (rpmluaReadline(L, "> ") == 0)
	 break;
      if (lua_tostring(L, -1)[0] == '=') {
/*@-evalorder@*/
	 (void) lua_pushfstring(L, "print(%s)", lua_tostring(L, -1)+1);
/*@=evalorder@*/
	 lua_remove(L, -2);
      }
      for (;;) {
/*@-evalorder@*/
	 rc = luaL_loadbuffer(L, lua_tostring(L, -1),
			      lua_rawlen(L, -1), "<lua>");
/*@=evalorder@*/
	 if (rc == LUA_ERRSYNTAX &&
	     strstr(lua_tostring(L, -1), "near `<eof>'") != NULL) {
	    if (rpmluaReadline(L, ">> ") == 0)
	       /*@innerbreak@*/ break;
	    lua_remove(L, -2); /* Remove error */
	    lua_concat(L, 2);
	    /*@innercontinue@*/ continue;
	 }
	 /*@innerbreak@*/ break;
      }
      if (rc == 0)
	 rc = lua_pcall(L, 0, 0, 0);
      if (rc != 0) {
/*@-evalorderuncon@*/
	 fprintf(stderr, "%s\n", lua_tostring(L, -1));
/*@=evalorderuncon@*/
	 lua_pop(L, 1);
      }
      lua_pop(L, 1); /* Remove line */
   }
   (void) fputs("\n", stdout);
}

/*@-mods@*/
void rpmluaInteractive(rpmlua _lua)
{
    INITSTATE(_lua, lua);
    _rpmluaInteractive(lua->L);
}
/*@=mods@*/

/* ------------------------------------------------------------------ */
/* Lua API */

static int rpm_macros(lua_State *L)
	/*@modifies L @*/
{
    const char ** av = NULL;
    int ac = 0;
    int i;

/*@-modunconnomods@*/
    lua_newtable(L);
/*@=modunconnomods@*/

/*@-globs@*/
    ac = rpmGetMacroEntries(NULL, NULL, -1, &av);
/*@=globs@*/

    if (av != NULL)
    for (i = 0; i < ac; i++) {
	char *n, *o, *b;

	/* Parse out "%name(opts)\tbody" into n/o/b strings. */
	n = (char *) av[i];
	b = strchr(n, '\t');
assert(b != NULL);
	o = ((b > n && b[-1] == ')') ? strchr(n, '(') : NULL);
	if (*n == '%')	n++;
	if (o != NULL && *o == '(') {
	    b[-1] = '\0';
	    o++;
            o[-1] = '\0';
	}
        else
            b[0] = '\0';
	b++;

/*@-modunconnomods@*/
        lua_pushstring(L, n);
        lua_newtable(L);
        if (o) {
            lua_pushstring(L, "opts");
            lua_pushstring(L, o);
            lua_settable(L, -3);
        }
        if (b) {
            lua_pushstring(L, "body");
            lua_pushstring(L, b);
            lua_settable(L, -3);
        }
        lua_settable(L, -3);
/*@=modunconnomods@*/
    }
    av = argvFree(av);
    return 1;
}

static int rpm_expand(lua_State *L)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies L, rpmGlobalMacroContext, internalState @*/
{
    const char *str = luaL_checkstring(L, 1);
    lua_pushstring(L, rpmExpand(str, NULL));
    return 1;
}

static int rpm_define(lua_State *L)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies L, rpmGlobalMacroContext, internalState @*/
{
    const char *str = luaL_checkstring(L, 1);
    (void) rpmDefineMacro(NULL, str, 0);
    return 0;
}

static int rpm_undefine(lua_State *L)
	/*@globals rpmGlobalMacroContext, internalState @*/
	/*@modifies L, rpmGlobalMacroContext, internalState @*/
{
    const char *str = luaL_checkstring(L, 1);
    (void) rpmUndefineMacro(NULL, str);
    return 0;
}

static int rpm_interactive(lua_State *L)
	/*@globals fileSystem @*/
	/*@modifies L, fileSystem @*/
{
    _rpmluaInteractive(L);
    return 0;
}

typedef struct rpmluaHookData_s {
/*@shared@*/
    lua_State *L;
    int funcRef;
    int dataRef;
} * rpmluaHookData;

static int rpmluaHookWrapper(rpmhookArgs args, void *data)
	/*@*/
{
    rpmluaHookData hookdata = (rpmluaHookData)data;
    lua_State *L = hookdata->L;
    int ret = 0;
    int i;
    lua_rawgeti(L, LUA_REGISTRYINDEX, hookdata->funcRef);
    lua_newtable(L);
    for (i = 0; i != args->argc; i++) {
	switch (args->argt[i]) {
	    case 's':
		lua_pushstring(L, args->argv[i].s);
		lua_rawseti(L, -2, i+1);
		/*@switchbreak@*/ break;
	    case 'i':
		lua_pushnumber(L, (lua_Number)args->argv[i].i);
		lua_rawseti(L, -2, i+1);
		/*@switchbreak@*/ break;
	    case 'f':
		lua_pushnumber(L, (lua_Number)args->argv[i].f);
		lua_rawseti(L, -2, i+1);
		/*@switchbreak@*/ break;
	    case 'p':
		lua_pushlightuserdata(L, args->argv[i].p);
		lua_rawseti(L, -2, i+1);
		/*@switchbreak@*/ break;
	    default:
                (void) luaL_error(L, "unsupported type '%c' as "
                              "a hook argument\n", args->argt[i]);
		/*@switchbreak@*/ break;
	}
    }
    if (lua_pcall(L, 1, 1, 0) != 0) {
	rpmlog(RPMLOG_ERR, _("lua hook failed: %s\n"),
		 lua_tostring(L, -1));
	lua_pop(L, 1);
    } else {
	if (lua_isnumber(L, -1))
	    ret = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
    }
    return ret;
}

static int rpm_register(lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
    if (!lua_isstring(L, 1)) {
	(void) luaL_argerror(L, 1, "hook name expected");
    } else if (!lua_isfunction(L, 2)) {
	(void) luaL_argerror(L, 2, "function expected");
    } else {
	rpmluaHookData hookdata = (rpmluaHookData)
	    lua_newuserdata(L, sizeof(struct rpmluaHookData_s));
	lua_pushvalue(L, -1);
	hookdata->dataRef = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 2);
	hookdata->funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
/*@-temptrans@*/
	hookdata->L = L;
/*@=temptrans@*/
	rpmhookRegister(lua_tostring(L, 1), rpmluaHookWrapper, hookdata);
	return 1;
    }
    return 0;
}

static int rpm_unregister(lua_State *L)
	/*@modifies L @*/
{
    if (!lua_isstring(L, 1)) {
	(void) luaL_argerror(L, 1, "hook name expected");
    } else if (!lua_islightuserdata(L, 2)) {
	(void) luaL_argerror(L, 2, "hook information expected");
    } else {
	rpmluaHookData hookdata = (rpmluaHookData)lua_touserdata(L, 2);
	luaL_unref(L, LUA_REGISTRYINDEX, hookdata->funcRef);
	luaL_unref(L, LUA_REGISTRYINDEX, hookdata->dataRef);
	rpmhookUnregister(lua_tostring(L, 1), rpmluaHookWrapper, hookdata);
    }
    return 0;
}

static int rpm_call(lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
    if (!lua_isstring(L, 1)) {
	(void) luaL_argerror(L, 1, "hook name expected");
    } else {
	rpmhookArgs args = rpmhookArgsNew(lua_gettop(L)-1);
	const char *name = lua_tostring(L, 1);
	char *argt = (char *)xmalloc(args->argc+1);
	int i;
	for (i = 0; i != args->argc; i++) {
	    switch (lua_type(L, i+1)) {
		case LUA_TNIL:
		    argt[i] = 'p';
		    args->argv[i].p = NULL;
		    /*@switchbreak@*/ break;
		case LUA_TNUMBER: {
		    float f = (float)lua_tonumber(L, i+1);
/*@+relaxtypes@*/
		    if (f == (int)f) {
			argt[i] = 'i';
			args->argv[i].i = (int)f;
		    } else {
			argt[i] = 'f';
			args->argv[i].f = f;
		    }
/*@=relaxtypes@*/
		}   /*@switchbreak@*/ break;
		case LUA_TSTRING:
		    argt[i] = 's';
		    args->argv[i].s = lua_tostring(L, i+1);
		    /*@switchbreak@*/ break;
		case LUA_TUSERDATA:
		case LUA_TLIGHTUSERDATA:
		    argt[i] = 'p';
		    args->argv[i].p = lua_touserdata(L, i+1);
		    /*@switchbreak@*/ break;
		default:
		    (void) luaL_error(L, "unsupported Lua type passed to hook");
		    argt[i] = 'p';
		    args->argv[i].p = NULL;
		    /*@switchbreak@*/ break;
	    }
	}
/*@-compdef -kepttrans -usereleased @*/
	args->argt = argt;
	rpmhookCallArgs(name, args);
	argt = _free(argt);
	(void) rpmhookArgsFree(args);
/*@=compdef =kepttrans =usereleased @*/
    }
    return 0;
}

/* Based on luaB_print. */
static int rpm_print (lua_State *L)
	/*@globals fileSystem @*/
	/*@modifies L, fileSystem @*/
{
    rpmlua lua = (rpmlua)getdata(L, "lua");
    int n = lua_gettop(L);  /* number of arguments */
    int i;
    if (!lua) return 0;
    lua_getglobal(L, "tostring");
    for (i = 1; i <= n; i++) {
	const char *s;
	lua_pushvalue(L, -1);  /* function to be called */
	lua_pushvalue(L, i);   /* value to print */
	lua_call(L, 1, 1);
	s = lua_tostring(L, -1);  /* get result */
	if (s == NULL)
	    return luaL_error(L, "`tostring' must return a string to `print'");
	if (lua->storeprint) {
	    size_t sl = lua_rawlen(L, -1);
	    if ((size_t)(lua->printbufused+sl+1) > lua->printbufsize) {
		lua->printbufsize += sl+512;
		lua->printbuf = (char *) xrealloc(lua->printbuf, lua->printbufsize);
	    }
	    if (i > 1)
		lua->printbuf[lua->printbufused++] = '\t';
	    memcpy(lua->printbuf+lua->printbufused, s, sl+1);
	    lua->printbufused += sl;
	} else {
	    if (i > 1)
		(void) fputs("\t", stdout);
	    (void) fputs(s, stdout);
	}
	lua_pop(L, 1);  /* pop result */
    }
    if (!lua->storeprint) {
	(void) fputs("\n", stdout);
    } else {
	if ((size_t)(lua->printbufused+1) > lua->printbufsize) {
	    lua->printbufsize += 512;
	    lua->printbuf = (char *) xrealloc(lua->printbuf, lua->printbufsize);
	}
	lua->printbuf[lua->printbufused] = '\0';
    }
    return 0;
}

static int rpm_source(lua_State *L)
	/*@globals fileSystem, internalState @*/
	/*@modifies L, fileSystem, internalState @*/
{
    if (!lua_isstring(L, 1)) {
	(void)luaL_argerror(L, 1, "filename expected");
    } else {
        rpmlua lua = (rpmlua)getdata(L, "lua");
	const char *filename = lua_tostring(L, 1);
	(void)rpmluaRunScriptFile(lua, filename);
    }
    return 0;
}

extern int _max_load_depth;	/* Maximum load nesting depth. */
static int rpm_load(lua_State *L)
	/*@globals rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies L, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    if (!lua_isstring(L, 1)) {
	(void)luaL_argerror(L, 1, "filename expected");
    } else {
	const char *filename = lua_tostring(L, 1);
/*@-globs@*/
	(void)rpmLoadMacroFile(NULL, filename, _max_load_depth);
/*@=globs@*/
    }
    return 0;
}

static int rpm_verbose(lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
    lua_pushboolean(L, rpmIsVerbose());
    return 1;
}

static int rpm_debug(lua_State *L)
	/*@globals internalState @*/
	/*@modifies L, internalState @*/
{
    lua_pushboolean(L, rpmIsDebug());
    return 1;
}

static int rpm_slurp(lua_State *L)
	/*@globals fileSystem, internalState @*/
	/*@modifies L, fileSystem, internalState @*/
{
    rpmiob iob = NULL;
    const char *fn;
    int rc;

    if (lua_isstring(L, 1))
        fn = lua_tostring(L, 1);
    else {
        (void)luaL_argerror(L, 1, "filename");
        return 0;
    }
/*@-globs@*/
    rc = rpmiobSlurp(fn, &iob);
/*@=globs@*/
    if (rc || iob == NULL) {
        (void)luaL_error(L, "failed to slurp data");
        return 0;
    }
    lua_pushlstring(L, (const char *)rpmiobStr(iob), rpmiobLen(iob));
    iob = rpmiobFree(iob);
    return 1;
}

static int rpm_sleep(lua_State *L)
	/*@globals fileSystem, internalState @*/
	/*@modifies L, fileSystem, internalState @*/
{
    unsigned sec;

    if (lua_isnumber(L, 1))
        sec = (unsigned) lua_tonumber(L, 1);
    else {
        (void)luaL_argerror(L, 1, "seconds");
        return 0;
    }
    (void) sleep(sec);
    return 0;
}

static int rpm_realpath(lua_State *L)
    /*@globals fileSystem, internalState @*/
    /*@modifies L, fileSystem, internalState @*/
{
    const char *pn;
    char rp_buf[PATH_MAX];
    char *rp = (char *) "";

    if (lua_isstring(L, 1))
        pn = lua_tostring(L, 1);
    else {
        (void)luaL_argerror(L, 1, "pathname");
        return 0;
    }
    if ((rp = Realpath(pn, rp_buf)) == NULL) {
        (void)luaL_error(L, "failed to resolve path via realpath(3): %s", strerror(errno));
        return 0;
    }
    lua_pushstring(L, (const char *)rp);
    return 1;
}

static int rpm_hostname(lua_State *L)
	/*@globals h_errno, internalState @*/
	/*@modifies L, h_errno, internalState @*/
{
    char hostname[1024];
    struct hostent *hbn;
    char *h;

/*@-multithreaded@*/
    (void)gethostname(hostname, sizeof(hostname));
    if ((hbn = gethostbyname(hostname)) != NULL)
        h = hbn->h_name;
    else
        h = hostname;
/*@=multithreaded@*/
    lua_pushstring(L, (const char *)h);
    return 1;
}

/*@-readonlytrans -nullassign @*/
/*@observer@*/ /*@unchecked@*/
static const luaL_Reg rpmlib[] = {
    {"macros", rpm_macros},
    {"expand", rpm_expand},
    {"define", rpm_define},
    {"undefine", rpm_undefine},
    {"register", rpm_register},
    {"unregister", rpm_unregister},
    {"call", rpm_call},
    {"interactive", rpm_interactive},
    {"source", rpm_source},
    {"load", rpm_load},
    {"verbose", rpm_verbose},
    {"debug", rpm_debug},
    {"slurp", rpm_slurp},
    {"sleep", rpm_sleep},
    {"realpath", rpm_realpath},
    {"hostname", rpm_hostname},
    {NULL, NULL}
};
/*@=readonlytrans =nullassign @*/

static int luaopen_rpm(lua_State *L)
	/*@modifies L @*/
{
#if defined(LUA_GLOBALSINDEX)
    lua_pushvalue(L, LUA_GLOBALSINDEX);
#else
    lua_pushglobaltable(L);
#endif
    luaL_newlib(L, rpmlib);
    return 1;
}
#endif	/* WITH_LUA */

/*@=moduncon =mustmod =realcompare =sizeoftype @*/
