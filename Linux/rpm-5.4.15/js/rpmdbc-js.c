/** \ingroup js_c
 * \file js/rpmdbc-js.c
 */

#include "system.h"

#define	_RPMDB_JS_INTERNAL
#include "rpmdb-js.h"
#include "rpmdbc-js.h"
#include "rpmjs-debug.h"

#include <argv.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmdbc_addprop		JS_PropertyStub
#define	rpmdbc_delprop		JS_PropertyStub
#define	rpmdbc_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmdbc_getobjectops	NULL
#define	rpmdbc_checkaccess	NULL
#define	rpmdbc_call		rpmdbc_call
#define	rpmdbc_construct		rpmdbc_ctor
#define	rpmdbc_xdrobject		NULL
#define	rpmdbc_hasinstance	NULL
#define	rpmdbc_mark		NULL
#define	rpmdbc_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmdbc_equality		NULL
#define rpmdbc_outerobject	NULL
#define rpmdbc_innerobject	NULL
#define rpmdbc_iteratorobject	NULL
#define rpmdbc_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSBool
rpmdbc_Close(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbc == NULL) goto exit;
    *vp = JSVAL_FALSE;

    {	int ret = dbc->close(dbc);
	if (ret)
	    fprintf(stderr, "DBC->close: %s\n", db_strerror(ret));
	else
	    *vp = JSVAL_TRUE;
	dbc = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmdbc_Cmp(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    JSObject * _o = NULL;
    DBC * _otherdbc = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbc == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o", &_o))
     ||	(_otherdbc = JS_GetInstancePrivate(cx, _o, &rpmdbcClass, NULL)) == NULL)
	goto exit;

    {	int _result = 0;
	uint32_t _flags = 0;
	int ret = ENOTSUP;
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
	ret = dbc->cmp(dbc, _otherdbc, &_result, _flags);
#endif
	switch (ret) {
	default:
	    fprintf(stderr, "DBC->cmp: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    *vp = (!_result ? JSVAL_TRUE : JSVAL_FALSE);
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbc_Count(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbc == NULL) goto exit;
    *vp = JSVAL_FALSE;

    {	db_recno_t _count = 0;
	uint32_t _flags = 0;
	int ret = dbc->count(dbc, &_count, _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DBC->count: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    if (!JS_NewNumberValue(cx, (jsdouble)_count, vp))
		*vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbc_Del(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbc == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    {	int ret = dbc->del(dbc, _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DBC->del: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbc_Dup(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbc == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    {	JSObject * _o = NULL;
	DBC * _cursor = NULL;
	int ret = dbc->dup(dbc, &_cursor, _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DBC->dup: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    if ((_o = JS_NewObject(cx, &rpmdbcClass, NULL, NULL)) == NULL
	     || !JS_SetPrivate(cx, _o, (void *)_cursor))
	    {
		if (_cursor)	_cursor->close(_cursor);
		/* XXX error msg */
		goto exit;
	    }
	    *vp = OBJECT_TO_JSVAL(_o);
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbc_Get(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    jsval _dv = JSVAL_NULL;
    _RPMDBT _d = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbc == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "v/vu", &_kv, &_dv, &_flags)))
	goto exit;
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;
    if (rpmdb_v2dbt(cx, _dv, &_d))
	goto exit;

    {	int ret = dbc->get(dbc, _RPMDBT_PTR(_k), _RPMDBT_PTR(_d), _flags);
#ifdef	NOTNOW
fprintf(stderr, "==> %s: ret %d = dbc->get(%p, %p[%u], %p[%u], 0x%x)\n",
__FUNCTION__, ret, dbc,
_RPMDBT_PTR(_k)->data, (unsigned)_RPMDBT_PTR(_k)->size,
_RPMDBT_PTR(_d)->data, (unsigned)_RPMDBT_PTR(_d)->size,
_flags);
#endif
	switch (ret) {
	default:
	    fprintf(stderr, "DBC->get: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, _RPMDBT_PTR(_d)->data, _RPMDBT_PTR(_d)->size));
	    break;
	case DB_NOTFOUND:
	    *vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbc_Pget(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    jsval _dv = JSVAL_NULL;
    _RPMDBT _d = _RPMDBT_INIT;
    _RPMDBT _p = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbc == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "v/vu", &_kv, &_dv, &_flags)))
	goto exit;
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;
    if (rpmdb_v2dbt(cx, _dv, &_d))
	goto exit;

    {	int ret = dbc->pget(dbc, _RPMDBT_PTR(_k), _RPMDBT_PTR(_p), _RPMDBT_PTR(_d), _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DBC->pget: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _RPMDBT_PTR(_d)->data));
	    break;
	case DB_NOTFOUND:
	    *vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbc_Put(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    jsval _dv = JSVAL_NULL;
    _RPMDBT _d = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbc == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "vv/u", &_kv, &_dv, &_flags)))
	goto exit;
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;
    if (rpmdb_v2dbt(cx, _dv, &_d))
	goto exit;

    {	int ret = dbc->put(dbc, _RPMDBT_PTR(_k), _RPMDBT_PTR(_d), _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DBC->put: %s\n", db_strerror(ret));
	    *vp = JSVAL_FALSE;
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	case DB_NOTFOUND:
	    *vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSFunctionSpec rpmdbc_funcs[] = {
    JS_FS("close",	rpmdbc_Close,		0,0),
    JS_FS("cmp",	rpmdbc_Cmp,		0,0),
    JS_FS("count",	rpmdbc_Count,		0,0),
    JS_FS("del",	rpmdbc_Del,		0,0),
    JS_FS("dup",	rpmdbc_Dup,		0,0),
    JS_FS("get",	rpmdbc_Get,		0,0),
    JS_FS("pget",	rpmdbc_Pget,		0,0),
    JS_FS("put",	rpmdbc_Put,		0,0),
    JS_FS_END
};

/* --- Object properties */

#define	_TABLE(_v)	#_v, _##_v, JSPROP_ENUMERATE, NULL, NULL

enum rpmdbc_tinyid {
    _DEBUG	= -2,
    _LENGTH	= -3,
    _PRIORITY	= -4,
};

static JSPropertySpec rpmdbc_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"length",	_LENGTH,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"priority",_PRIORITY,	JSPROP_ENUMERATE,	NULL,	NULL},

    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmdbc_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_NewNumberValue(cx, (jsdouble)_debug, vp))
	    *vp = JSVAL_VOID;
	break;
    case _LENGTH:
    {	db_recno_t _count = 0;
	if (!dbc->count(dbc, &_count, 0)) {
	    if (!JS_NewNumberValue(cx, (jsdouble)_count, vp))
		*vp = JSVAL_VOID;
	} else
	    *vp = JSVAL_VOID;
    }	break;
    case _PRIORITY:
    {	DB_CACHE_PRIORITY _priority = DB_PRIORITY_UNCHANGED;
	if (!dbc->get_priority(dbc, &_priority)) {
	    if (!JS_NewNumberValue(cx, (jsdouble)_priority, vp))
		*vp = JSVAL_VOID;
	} else
	    *vp = JSVAL_VOID;
    }	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmdbc_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    *vp = JSVAL_VOID;
	break;
    case _PRIORITY:
    {	uint32_t _u = 0;
	if (JS_ValueToECMAUint32(cx, *vp, &_u)) {
	    DB_CACHE_PRIORITY _priority = _u;
	    *vp = !dbc->set_priority(dbc, _priority)
		? JSVAL_TRUE : JSVAL_FALSE;
	} else
	    *vp = JSVAL_VOID;
    }	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmdbc_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);

_RESOLVE_DEBUG_ENTRY(_debug < 0);

    if ((flags & JSRESOLVE_ASSIGNING)
     || (ptr == NULL)) { /* don't resolve to parent prototypes objects. */
	*objp = NULL;
	goto exit;
    }

    *objp = obj;	/* XXX always resolve in this object. */

exit:
    return JS_TRUE;
}

static JSBool
rpmdbc_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	*statep = JSVAL_ZERO;
        if (idp) {
	    db_recno_t _count = 0;
	    uint32_t _flags = 0;
	    int ret = dbc->count(dbc, &_count, _flags);
	    switch (ret) {
	    default:
		*idp = JSVAL_ZERO;
		break;
	    case 0:
		*idp = INT_TO_JSVAL(_count);
		break;
	    }
	}
        break;
    case JSENUMERATE_NEXT:
    {	DBT _k = {0};
	DBT _d = {0};
	uint32_t _flags = 0;
	int ret = dbc->get(dbc, &_k, &_d, _flags);
	int ix = JSVAL_TO_INT(*statep);
	*statep = INT_TO_JSVAL(ix + 1);
#ifdef  NOTNOW
fprintf(stderr, "==> %s: ret %d = dbc->get(%p, %p[%u], %p[%u], 0x%x)\n",
__FUNCTION__, ret, dbc,
_k.data, (unsigned)_k.size,
_d.data, (unsigned)_d.size,
_flags);
#endif
	switch (ret) {
	default:
	    *idp = JSVAL_VOID;
	    break;
	case 0:
	    *idp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, _d.data, _d.size));
	    break;
	case DB_NOTFOUND:
	    *idp = JSVAL_VOID;
	    break;
	}
    }
	if (!JSID_IS_VOID(*idp))
            break;
        /*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	*statep = JSVAL_NULL;
        break;
    }
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static DBC *
rpmdbc_init(JSContext *cx, JSObject *obj)
{
    DBC * dbc = NULL;
#ifdef	NOTYET
    uint32_t _flags = 0;

    if (rpmdbc_env_create(&db, _flags) || dbc == NULL
     || !JS_SetPrivate(cx, obj, (void *)dbc))
#else
    if (!JS_SetPrivate(cx, obj, (void *)dbc))
#endif
    {
	if (dbc)
	    (void) dbc->close(dbc);
	/* XXX error msg */
	dbc = NULL;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) dbc %p\n", __FUNCTION__, cx, obj, dbc);

    return dbc;
}

static void
rpmdbc_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbcClass, NULL);
    DBC * dbc = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    if (dbc)
	(void) dbc->close(dbc);
}

static JSBool
rpmdbc_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmdbc_init(cx, obj);
    } else {
	if ((obj = JS_NewObject(cx, &rpmdbcClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbc_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmdbcClass, NULL);
#ifdef	NOTYET
    DBC * dbc = ptr;
    const char *_fn = NULL;
    const char * _con = NULL;
#endif
    JSBool ok = JS_FALSE;

#ifdef	NOTYET
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_fn)))
        goto exit;

    *vp = (dbc && _fn && (_con = rpmdbcLgetfilecon(dbc, _fn)) != NULL)
	? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _con)) : JSVAL_VOID;
    _con = _free(_con);

    ok = JS_TRUE;

exit:
#endif

if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);

    return ok;
}

/* --- Class initialization */
JSClass rpmdbcClass = {
    "Dbc", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmdbc_addprop,   rpmdbc_delprop, rpmdbc_getprop, rpmdbc_setprop,
    (JSEnumerateOp)rpmdbc_enumerate, (JSResolveOp)rpmdbc_resolve,
    rpmdbc_convert,	rpmdbc_dtor,

    rpmdbc_getobjectops,	rpmdbc_checkaccess,
    rpmdbc_call,		rpmdbc_construct,
    rpmdbc_xdrobject,	rpmdbc_hasinstance,
    rpmdbc_mark,		rpmdbc_reserveslots,
};

JSObject *
rpmjs_InitDbcClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmdbcClass, rpmdbc_ctor, 1,
		rpmdbc_props, rpmdbc_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewDbcObject(JSContext *cx)
{
    JSObject *obj;
    DBC * dbc;

    if ((obj = JS_NewObject(cx, &rpmdbcClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((dbc = rpmdbc_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmdbc, Dbc, JS_TRUE)
