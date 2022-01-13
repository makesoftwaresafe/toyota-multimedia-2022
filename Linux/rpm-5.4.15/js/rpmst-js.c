/** \ingroup js_c
 * \file js/rpmst-js.c
 */

#include "system.h"

#include "rpmst-js.h"
#include "rpmjs-debug.h"

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmst_addprop	JS_PropertyStub
#define	rpmst_delprop	JS_PropertyStub
#define	rpmst_convert	JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmst_getobjectops	NULL
#define	rpmst_checkaccess	NULL
#define	rpmst_call		rpmst_call
#define	rpmst_construct		rpmst_ctor
#define	rpmst_xdrobject		NULL
#define	rpmst_hasinstance	NULL
#define	rpmst_mark		NULL
#define	rpmst_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmst_equality		NULL
#define rpmst_outerobject	NULL
#define rpmst_innerobject	NULL
#define rpmst_iteratorobject	NULL
#define rpmst_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */
static JSFunctionSpec rpmst_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmst_tinyid {
    _DEBUG	= -2,
    _DEV	= -3,
    _INO	= -4,
    _MODE	= -5,
    _NLINK	= -6,
    _UID	= -7,
    _GID	= -8,
    _RDEV	= -9,
    _SIZE	= -10,
    _BLKSIZE	= -11,
    _BLOCKS	= -12,
    _ATIME	= -13,
    _MTIME	= -14,
    _CTIME	= -15,
};

static JSPropertySpec rpmst_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"dev",	_DEV,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"ino",	_INO,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"mode",	_MODE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"nlink",	_NLINK,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"uid",	_UID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"gid",	_GID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"rdev",	_RDEV,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"size",	_SIZE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"blksize",	_BLKSIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"blocks",	_BLOCKS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"atime",	_ATIME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"mtime",	_MTIME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"ctime",	_CTIME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

#define	_GET_I(_p, _f)   ((_p) ? INT_TO_JSVAL((int)(_p)->_f) : JSVAL_VOID)

static JSBool
rpmst_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmstClass, NULL);
    struct stat * st = ptr;
    jsint tiny = JSVAL_TO_INT(id);
    time_t mytime = (time_t)0xffffffff;

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:	*vp = INT_TO_JSVAL(_debug);		break;
    case _DEV:		*vp = _GET_I(st, st_dev);		break;
    case _INO:		*vp = _GET_I(st, st_ino);		break;
    case _MODE:		*vp = _GET_I(st, st_mode);	break;
    case _NLINK:	*vp = _GET_I(st, st_nlink);	break;
    case _UID:		*vp = _GET_I(st, st_uid);		break;
    case _GID:		*vp = _GET_I(st, st_gid);		break;
    case _RDEV:		*vp = _GET_I(st, st_rdev);	break;
    case _SIZE:		*vp = _GET_I(st, st_size);	break;
    case _BLKSIZE:	*vp = _GET_I(st, st_blksize);	break;
    case _BLOCKS:	*vp = _GET_I(st, st_blocks);	break;
    case _ATIME:	if (st) mytime = st->st_atime;	break;
    case _MTIME:	if (st) mytime = st->st_mtime;	break;
    case _CTIME:	if (st) mytime = st->st_ctime;	break;
    default:
	break;
    }

    if (mytime != (time_t)0xffffffff) {
	struct tm *tm = gmtime(&mytime);
	*vp = OBJECT_TO_JSVAL(js_NewDateObject(cx,
					tm->tm_year + 1900,
					tm->tm_mon,
					tm->tm_mday,
					tm->tm_hour,
					tm->tm_min,
					tm->tm_sec));
    }

    return JS_TRUE;
}

static JSBool
rpmst_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmstClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

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
rpmst_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmstClass, NULL);

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
rpmst_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    JSObject * iter;
    JSBool ok = JS_FALSE;

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	if ((iter = JS_NewPropertyIterator(cx, obj)) == NULL)
	    goto exit;
	*statep = OBJECT_TO_JSVAL(iter);
        if (idp)
            *idp = JSVAL_ZERO;
        break;
    case JSENUMERATE_NEXT:
	iter = (JSObject*)JSVAL_TO_OBJECT(*statep);
	if (!JS_NextProperty(cx, iter, idp))
	    goto exit;
	if (!JSID_IS_VOID(*idp))
            break;
        /*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	*statep = JSVAL_NULL;
        break;
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Object ctors/dtors */
static struct stat *
rpmst_init(JSContext *cx, JSObject *obj, jsval fnv)
{
    struct stat * st = xcalloc(1, sizeof(*st));

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%u) st %p\n", __FUNCTION__, cx, obj, (unsigned)fnv, st);

    if (JSVAL_IS_STRING(fnv)) {
	const char * fn = JS_EncodeString(cx, JS_ValueToString(cx, fnv));
	if (Stat(fn, st) < 0) {
	    /* XXX error msg */
	    st = _free(st);
	}
	fn = _free(fn);
    } else {
	st = _free(st);
    }

    if (!JS_SetPrivate(cx, obj, (void *)st)) {
	/* XXX error msg */
	st = _free(st);
    }
    return st;
}

static void
rpmst_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmstClass, NULL);
    struct stat * st = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    st = _free(st);
}

static JSBool
rpmst_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    jsval fnv = JSVAL_VOID;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/v", &fnv)))
        goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmst_init(cx, obj, fnv);
    } else {
	if ((obj = JS_NewObject(cx, &rpmstClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmst_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmstClass, NULL);
    struct stat * st = ptr;
    JSBool ok = JS_FALSE;
    jsval fnv = JSVAL_VOID;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/v", &fnv)))
        goto exit;

    if (st) {
	st = ptr = _free(st);
	(void) JS_SetPrivate(cx, o, (void *)st);
    }

    st = ptr = rpmst_init(cx, o, fnv);

    *vp = OBJECT_TO_JSVAL(o);

    ok = JS_TRUE;

exit:
if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);

    return ok;
}

/* --- Class initialization */
JSClass rpmstClass = {
    /* XXX class should be "Stat" eventually, avoid name conflicts for now */
    "St", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmst_addprop,   rpmst_delprop, rpmst_getprop, rpmst_setprop,
    (JSEnumerateOp)rpmst_enumerate, (JSResolveOp)rpmst_resolve,
    rpmst_convert,	rpmst_dtor,

    rpmst_getobjectops,	rpmst_checkaccess,
    rpmst_call,		rpmst_construct,
    rpmst_xdrobject,	rpmst_hasinstance,
    rpmst_mark,		rpmst_reserveslots,
};

JSObject *
rpmjs_InitStClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmstClass, rpmst_ctor, 1,
		rpmst_props, rpmst_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewStObject(JSContext *cx, jsval fnv)
{
    JSObject *obj;
    struct stat * st;

    if ((obj = JS_NewObject(cx, &rpmstClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((st = rpmst_init(cx, obj, fnv)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmst, St, JS_TRUE)
