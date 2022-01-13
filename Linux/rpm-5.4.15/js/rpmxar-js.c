/** \ingroup js_c
 * \file js/rpmxar-js.c
 */

#include "system.h"

#include "rpmxar-js.h"
#include "rpmst-js.h"
#include "rpmjs-debug.h"

#define	_RPMXAR_INTERNAL
#include <rpmxar.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmxar_addprop		JS_PropertyStub
#define	rpmxar_delprop		JS_PropertyStub
#define	rpmxar_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmxar_getobjectops	NULL
#define	rpmxar_checkaccess	NULL
#define	rpmxar_call		NULL
#define	rpmxar_construct		rpmxar_ctor
#define	rpmxar_xdrobject		NULL
#define	rpmxar_hasinstance	NULL
#define	rpmxar_mark		NULL
#define	rpmxar_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmxar_equality		NULL
#define rpmxar_outerobject	NULL
#define rpmxar_innerobject	NULL
#define rpmxar_iteratorobject	NULL
#define rpmxar_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSFunctionSpec rpmxar_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmxar_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmxar_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmxar_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmxarClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmxar_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmxarClass, NULL);
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
rpmxar_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmxarClass, NULL);

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
rpmxar_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmxarClass, NULL);
    rpmxar xar = ptr;
    int ix = 0;

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	if (idp)
	    *idp = JSVAL_ZERO;
	*statep = INT_TO_JSVAL(ix);
if (_debug)
fprintf(stderr, "\tINIT xar %p\n", xar);
        break;
    case JSENUMERATE_NEXT:
	ix = JSVAL_TO_INT(*statep);
	if (!rpmxarNext(xar)) {
	    const char * path = rpmxarPath(xar);
	    struct stat * st = xmalloc(sizeof(*st));
	    JSObject * arr = JS_NewArrayObject(cx, 0, NULL);
	    JSBool ok = JS_AddRoot(cx, &arr);
	    JSObject * o;
	    jsval v;

	    v = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, path));
	    ok = JS_SetElement(cx, arr, 0, &v);
	    if (!rpmxarStat(xar, st)
	     && (o = JS_NewObject(cx, &rpmstClass, NULL, NULL)) != NULL
             && JS_SetPrivate(cx, o, (void *)st))
		v = OBJECT_TO_JSVAL(o);
	    else {
		st = _free(st);
		v = JSVAL_NULL;
	    }
	    ok = JS_SetElement(cx, arr, 1, &v);

	    v = OBJECT_TO_JSVAL(arr);
	    ok = JS_DefineElement(cx, obj, ix, v, NULL, NULL, JSPROP_ENUMERATE);

	    (void) JS_RemoveRoot(cx, &arr);
if (_debug)
fprintf(stderr, "\tNEXT xar %p[%u] \"%s\"\n", xar, ix, path);
	    path = _free(path);
	    JS_ValueToId(cx, *statep, idp);
	    *statep = INT_TO_JSVAL(ix+1);
	} else
	    *idp = JSVAL_VOID;
	if (!JSID_IS_VOID(*idp))
            break;
        /*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	ix = JSVAL_TO_INT(*statep);
	(void) JS_DefineProperty(cx, obj, "length", INT_TO_JSVAL(ix),
			NULL, NULL, JSPROP_ENUMERATE);
if (_debug)
fprintf(stderr, "\tFINI xar %p[%u]\n", xar, ix);
	*statep = JSVAL_NULL;
        break;
    }
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static rpmxar
rpmxar_init(JSContext *cx, JSObject *obj, const char * _fn, const char * _fmode)
{
    rpmxar xar = NULL;

    if (_fn) {
	if (_fmode == NULL) _fmode = "r";
	xar = rpmxarNew(_fn, _fmode);
    }

    if (!JS_SetPrivate(cx, obj, (void *)xar)) {
	/* XXX error msg */
	return NULL;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) xar(%s,%s) %p\n", __FUNCTION__, cx, obj, _fn, _fmode, xar);
    return xar;
}

static void
rpmxar_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmxarClass, NULL);
    rpmxar xar = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    (void) rpmxarFree(xar, __FUNCTION__);
}

static JSBool
rpmxar_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    const char * _fn = "popt-1.14.xar";
    const char * _fmode = "r";

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/ss", &_fn, &_fmode)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmxar_init(cx, obj, _fn, _fmode);
    } else {
	if ((obj = JS_NewObject(cx, &rpmxarClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmxarClass = {
    "Xar", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmxar_addprop,   rpmxar_delprop, rpmxar_getprop, rpmxar_setprop,
    (JSEnumerateOp)rpmxar_enumerate, (JSResolveOp)rpmxar_resolve,
    rpmxar_convert,	rpmxar_dtor,

    rpmxar_getobjectops,	rpmxar_checkaccess,
    rpmxar_call,		rpmxar_construct,
    rpmxar_xdrobject,	rpmxar_hasinstance,
    rpmxar_mark,		rpmxar_reserveslots,
};

JSObject *
rpmjs_InitXarClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmxarClass, rpmxar_ctor, 1,
		rpmxar_props, rpmxar_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewXarObject(JSContext *cx, const char * _fn, const char *_fmode)
{
    JSObject *obj;
    rpmxar xar;

    if ((obj = JS_NewObject(cx, &rpmxarClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((xar = rpmxar_init(cx, obj, _fn, _fmode)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmxar, Xar, JS_TRUE)
