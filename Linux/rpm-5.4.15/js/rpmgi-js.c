/** \ingroup js_c
 * \file js/rpmgi-js.c
 */

#include "system.h"

#include "rpmgi-js.h"
#include "rpmjs-debug.h"

#include <argv.h>
#define	_RPMGI_INTERNAL
#include <rpmgi.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmgi_addprop		JS_PropertyStub
#define	rpmgi_delprop		JS_PropertyStub
#define	rpmgi_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmgi_getobjectops	NULL
#define	rpmgi_checkaccess	NULL
#define	rpmgi_call		NULL
#define	rpmgi_construct		rpmgi_ctor
#define	rpmgi_xdrobject	NULL
#define	rpmgi_hasinstance	NULL
#define	rpmgi_mark		NULL
#define	rpmgi_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmgi_equality		NULL
#define rpmgi_outerobject	NULL
#define rpmgi_innerobject	NULL
#define rpmgi_iteratorobject	NULL
#define rpmgi_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSFunctionSpec rpmgi_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmgi_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmgi_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmgi_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmgiClass, NULL);
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
rpmgi_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmgiClass, NULL);
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
rpmgi_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmgiClass, NULL);

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
rpmgi_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	*statep = JSVAL_VOID;
        if (idp)
            *idp = JSVAL_ZERO;
        break;
    case JSENUMERATE_NEXT:
	*statep = JSVAL_VOID;
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
static rpmgi
rpmgi_init(JSContext *cx, JSObject *obj)
{
    void * _ts = NULL;
    int _tag = 0;
    const void * _keyp = NULL;
    size_t _keylen = 0;
    rpmgi gi = rpmgiNew(_ts, _tag, _keyp, _keylen);

if (_debug)
fprintf(stderr, "==> %s(%p,%p) gi %p\n", __FUNCTION__, cx, obj, gi);

    if (!JS_SetPrivate(cx, obj, (void *)gi)) {
	/* XXX error msg */
	return NULL;
    }
    return gi;
}

static void
rpmgi_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmgiClass, NULL);
    rpmgi gi = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    if (gi)
	(void) rpmgiFree(gi);
}

static JSBool
rpmgi_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmgi_init(cx, obj);
    } else {
	if ((obj = JS_NewObject(cx, &rpmgiClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmgiClass = {
    "Gi", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmgi_addprop,   rpmgi_delprop, rpmgi_getprop, rpmgi_setprop,
    (JSEnumerateOp)rpmgi_enumerate, (JSResolveOp)rpmgi_resolve,
    rpmgi_convert,	rpmgi_dtor,

    rpmgi_getobjectops,	rpmgi_checkaccess,
    rpmgi_call,		rpmgi_construct,
    rpmgi_xdrobject,	rpmgi_hasinstance,
    rpmgi_mark,		rpmgi_reserveslots,
};

JSObject *
rpmjs_InitGiClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmgiClass, rpmgi_ctor, 1,
		rpmgi_props, rpmgi_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewGiObject(JSContext *cx)
{
    JSObject *obj;
    rpmgi gi;

    if ((obj = JS_NewObject(cx, &rpmgiClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((gi = rpmgi_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmgi, Gi, JS_TRUE)
