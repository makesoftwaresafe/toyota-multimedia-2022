/** \ingroup js_c
 * \file js/syck-js.c
 */

#include "system.h"

#include "rpmjs-debug.h"

#include <rpmiotypes.h>

#if defined(WITH_SYCK)
#include <syck.h>
#include "syck-js.h"

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	syck_addprop	JS_PropertyStub
#define	syck_delprop	JS_PropertyStub
#define	syck_convert	JS_ConvertStub

typedef struct syck_s {
    void * P;
} * JSSyck;

struct emitter_xtra {
    void *L;
    rpmiob iob;
    int id;
};

struct parser_xtra {
    void *L;
};

static SYMID
js_syck_parser_handler(SyckParser *p, SyckNode *n)
{
#ifdef	NOTYET
    struct parser_xtra *bonus = (struct parser_xtra *)p->bonus;
    int o2;
    int o3 = -1;
    int i;
#endif
    int o = 0;
    SYMID oid;

    switch (n->kind) {
    case syck_str_kind:
#ifdef	NOTYET
	if (n->type_id == NULL || strcmp(n->type_id, "str") == 0) {
	    lua_pushlstring(bonus->L, n->data.str->ptr, n->data.str->len);
	    o = lua_gettop(bonus->L);
	} else if (strcmp(n->type_id, "null") == 0) {
	    lua_pushnil(bonus->L);
	    o = lua_gettop(bonus->L);
	} else if (strcmp(n->type_id, "bool#yes") == 0) {
	    lua_pushboolean(bonus->L, 1);
	    o = lua_gettop(bonus->L);
	} else if (strcmp(n->type_id, "bool#no") == 0) {
	    lua_pushboolean(bonus->L, 0);
	    o = lua_gettop(bonus->L);
	} else if (strcmp(n->type_id, "int#hex") == 0) {
	    long intVal = strtol(n->data.str->ptr, NULL, 16);
	    lua_pushnumber(bonus->L, intVal);
	    o = lua_gettop(bonus->L);
	} else if (strcmp(n->type_id, "int") == 0) {
	    long intVal = strtol(n->data.str->ptr, NULL, 10);
	    lua_pushnumber(bonus->L, intVal);
	    o = lua_gettop(bonus->L);
	} else {
	    lua_pushlstring(bonus->L, n->data.str->ptr, n->data.str->len);
	    o = lua_gettop(bonus->L);
	}
#endif
	break;
    case syck_seq_kind:
#ifdef	NOTYET
	lua_newtable(bonus->L);
	o = lua_gettop(bonus->L);
	for (i = 0; i < n->data.list->idx; i++) {
	    oid = syck_seq_read(n, i);
	    syck_lookup_sym(p, oid, (char **)&o2);
	    lua_pushvalue(bonus->L, o2);
	    lua_rawseti(bonus->L, o, i+1);
	}
#endif
	break;
    case syck_map_kind:
#ifdef	NOTYET
	lua_newtable(bonus->L);
	o = lua_gettop(bonus->L);
	for (i = 0; i < n->data.pairs->idx; i++) {
	    oid = syck_map_read(n, map_key, i);
	    syck_lookup_sym(p, oid, (char **)&o2);
	    oid = syck_map_read(n, map_value, i);
	    syck_lookup_sym(p, oid, (char **)&o3);

	    lua_pushvalue(bonus->L, o2);
	    lua_pushvalue(bonus->L, o3);
	    lua_settable(bonus->L, o);
	}
#endif
	break;
    }
    oid = syck_add_sym(p, (char *)((long)o));
    return oid;
}

static void js_syck_emitter_handler(SyckEmitter *e, st_data_t data)
{
    struct emitter_xtra *bonus = (struct emitter_xtra *)e->bonus;
#ifdef	NOTYET
    int type = lua_type(bonus->L, -1);
    char buf[30];		/* find a better way, if possible */
	
    switch (type) {
    case LUA_TBOOLEAN:
	if (lua_toboolean(bonus->L, -1))
		strcpy(buf, "true");
	else
		strcpy(buf, "false");
	syck_emit_scalar(e, "boolean", scalar_none, 0, 0, 0, (char *)buf, strlen(buf));
	break;
    case LUA_TSTRING:
	syck_emit_scalar(e, "string", scalar_none, 0, 0, 0, (char *)lua_tostring(bonus->L, -1), lua_strlen(bonus->L, -1));
	break;
    case LUA_TNUMBER:
	/* should handle floats as well */
	snprintf(buf, sizeof(buf), "%i", (int)lua_tonumber(bonus->L, -1));
	syck_emit_scalar(e, "number", scalar_none, 0, 0, 0, buf, strlen(buf));
	break;
    case LUA_TTABLE:
	if (luaL_getn(bonus->L, -1) > 0) {	/* treat it as an array */
	    syck_emit_seq(e, "table", seq_none);
	    lua_pushnil(bonus->L);  /* first key */
	    while (lua_next(bonus->L, -2) != 0) {
		/* `key' is at index -2 and `value' at index -1 */
		syck_emit_item(e, (st_data_t)bonus->id++);
		lua_pop(bonus->L, 1);  /* removes `value'; keeps `key' for next iteration */
	    }
	    syck_emit_end(e);
	} else {	/* treat it as a map */
	    syck_emit_map(e, "table", map_none);
	    lua_pushnil(bonus->L);
	    while (lua_next(bonus->L, -2) != 0) {
		lua_pushvalue(bonus->L, -2);
		syck_emit_item(e, (st_data_t)bonus->id++);
		lua_pop(bonus->L, 1);
		syck_emit_item(e, (st_data_t)bonus->id++);
		lua_pop(bonus->L, 1);
	    }
	    syck_emit_end(e);
	}
	break;
    }
#endif
    bonus->id++;
}

static void js_syck_mark_emitter(SyckEmitter *e, int idx)
{
#ifdef	NOTYET
    struct emitter_xtra *bonus = (struct emitter_xtra *)e->bonus;
    int type = lua_type(bonus->L, idx);

    switch (type) {
    case LUA_TTABLE:
	lua_pushnil(bonus->L);  /* first key */
	while (lua_next(bonus->L, -2) != 0) {
	    /* `key' is at index -2 and `value' at index -1 */
	    syck_emitter_mark_node(e, (st_data_t)bonus->id++);
	    lua_syck_mark_emitter(e, -1);
	    lua_pop(bonus->L, 1);
	}
	break;
    default:
	syck_emitter_mark_node(e, (st_data_t)bonus->id++);
	break;
    }
#endif
}

static void js_syck_output_handler(SyckEmitter *e, char *str, long len)
{
    struct emitter_xtra *bonus = (struct emitter_xtra *)e->bonus;
    (void) rpmiobAppend(bonus->iob, str, 0);
}

/* --- Object methods */

static JSBool
syck_load(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    SyckParser *parser = syck_new_parser();
    struct parser_xtra *bonus = xcalloc(1, sizeof(*bonus));
    SYMID v;
    int sobj;
    char * s = NULL;
    JSBool ok = JS_FALSE;

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%p[%u],%p)\n", __FUNCTION__, cx, obj, argv, (unsigned)argc, vp);

    ok = JS_ConvertArguments(cx, argc, argv, "s", &s);
    if (!ok)
	goto exit;

#ifdef	NOTYET
    bonus->L = lua_newthread(L);
#endif
    parser->bonus = bonus;

    syck_parser_str(parser, s, strlen(s), NULL);
    syck_parser_handler(parser, js_syck_parser_handler);

    v = syck_parse(parser);
    syck_lookup_sym(parser, v, (char **)&sobj);

#ifdef	NOTYET
    lua_xmove(bonus->L, L, 1);
#endif

    ok = JS_TRUE;

exit:
    syck_free_parser(parser);
    return ok;
}

static JSBool
syck_dump(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    SyckEmitter *emitter = syck_new_emitter();
    struct emitter_xtra * bonus = xcalloc(1, sizeof(*bonus));
    JSBool ok = JS_FALSE;

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%p[%u],%p)\n", __FUNCTION__, cx, obj, argv, (unsigned)argc, vp);

#ifdef	NOTYET
    ok = JS_ConvertArguments(cx, argc, argv, "",
		&version, &syck_ns_str, &data);
#endif
    if (!ok)
	goto exit;

    bonus->iob = rpmiobNew(0);
#ifdef	NOTYET
    bonus->L = lua_newthread(L);
    luaL_buffinit(L, &bonus->output);
#endif
    emitter->bonus = bonus;

    syck_emitter_handler(emitter, js_syck_emitter_handler);
    syck_output_handler(emitter, js_syck_output_handler);

#ifdef	NOTYET
    lua_pushvalue(L, -2);
    lua_xmove(L, bonus->L, 1);
#endif

    bonus->id = 1;
    js_syck_mark_emitter(emitter, bonus->id);

    bonus->id = 1;
    syck_emit(emitter, (st_data_t)((long)bonus->id));
    syck_emitter_flush(emitter, 0);

#ifdef	NOTYET
    luaL_pushresult(&bonus->output);
#endif

    ok = JS_TRUE;

exit:
    bonus->iob = rpmiobFree(bonus->iob);
    syck_free_emitter(emitter);
    return ok;
}

static JSFunctionSpec syck_funcs[] = {
    JS_FS("load",	syck_load,		0,0),
    JS_FS("dump",	syck_dump,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum syck_tinyid {
    _DEBUG		= -2,
};

static JSPropertySpec syck_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
syck_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &syckClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    if (JSVAL_IS_STRING(id)) {
	char * str = JS_EncodeString(cx, JSVAL_TO_STRING(id));
	const JSFunctionSpec *fsp;
	for (fsp = syck_funcs; fsp->name != NULL; fsp++) {
	    if (strcmp(fsp->name, str))
		continue;
	    break;
	}
	str = _free(str);
	goto exit;
    }

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    default:
	break;
    }
exit:
    return JS_TRUE;
}

static JSBool
syck_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &syckClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
syck_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    JSObject *iterator;
    JSBool ok = JS_FALSE;

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%d,%p,%p)\n", __FUNCTION__, cx, obj, op, statep, idp);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	if ((iterator = JS_NewPropertyIterator(cx, obj)) == NULL)
	    goto exit;
	*statep = OBJECT_TO_JSVAL(iterator);
	if (idp)
	    *idp = JSVAL_ZERO;
	break;
    case JSENUMERATE_NEXT:
	iterator = (JSObject *) JSVAL_TO_OBJECT(*statep);
	if (!JS_NextProperty(cx, iterator, idp))
	    goto exit;
	if (!JSID_IS_VOID(*idp))
	    break;
	/*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	/* Allow our iterator object to be GC'd. */
	*statep = JSVAL_NULL;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
syck_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &syckClass, NULL);

_RESOLVE_DEBUG_ENTRY(_debug);

    return JS_TRUE;
}

/* --- Object ctors/dtors */
static void
syck_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &syckClass, NULL);

_DTOR_DEBUG_ENTRY(_debug);

    ptr = _free(ptr);
}

static JSBool
syck_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	JSSyck ptr = xcalloc(0, sizeof(*ptr));

	if (ptr == NULL || !JS_SetPrivate(cx, obj, ptr)) {
	    ptr = _free(ptr);
	    goto exit;
	}
    } else {
	if ((obj = JS_NewObject(cx, &syckClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass syckClass = {
    "Syck", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    syck_addprop,   syck_delprop, syck_getprop, syck_setprop,
    (JSEnumerateOp)syck_enumerate, (JSResolveOp)syck_resolve,
    syck_convert,	syck_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitSyckClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &syckClass, syck_ctor, 1,
		syck_props, syck_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

GPSEE_MODULE_WRAP(syck, Syck, JS_TRUE)

#endif	/* WITH_SYCK */
