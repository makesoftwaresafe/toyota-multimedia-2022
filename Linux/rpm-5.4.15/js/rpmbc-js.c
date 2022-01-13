/** \ingroup js_c
 * \file js/rpmbc-js.c
 */

#include "system.h"

#include "rpmbc-js.h"
#include "rpmjs-debug.h"

#define	_RPMBC_INTERNAL
#include <rpmbc.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmbc_addprop		JS_PropertyStub
#define	rpmbc_delprop		JS_PropertyStub
#define	rpmbc_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmbc_getobjectops	NULL
#define	rpmbc_checkaccess	NULL
#define	rpmbc_call		NULL
#define	rpmbc_construct		rpmbc_ctor
#define	rpmbc_xdrobject		NULL
#define	rpmbc_hasinstance	NULL
#define	rpmbc_mark		NULL
#define	rpmbc_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmbc_equality		NULL
#define rpmbc_outerobject	NULL
#define rpmbc_innerobject	NULL
#define rpmbc_iteratorobject	NULL
#define rpmbc_wrappedobject	NULL

#define	rpmbcNew()	xcalloc(1, sizeof(struct rpmbc_s))
#define	rpmbcFree(_bc)	_free(_bc)

/* --- helpers */

/* --- Object methods */

static JSFunctionSpec rpmbc_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmbc_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmbc_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmbc_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmbcClass, NULL);
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
rpmbc_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmbcClass, NULL);
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
rpmbc_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmbcClass, NULL);

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
rpmbc_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static rpmbc
rpmbc_init(JSContext *cx, JSObject *obj)
{
    rpmbc bc = rpmbcNew();

if (_debug)
fprintf(stderr, "==> %s(%p,%p) bc %p\n", __FUNCTION__, cx, obj, bc);

    if (!JS_SetPrivate(cx, obj, (void *)bc)) {
	/* XXX error msg */
	return NULL;
    }
    return bc;
}

static void
rpmbc_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmbcClass, NULL);
    rpmbc bc = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    bc = rpmbcFree(bc);
}

static JSBool
rpmbc_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmbc_init(cx, obj);
    } else {
	if ((obj = JS_NewObject(cx, &rpmbcClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmbcClass = {
    /* XXX class should be "BeeCrypt" eventually, avoid name conflicts for now */
    "Bc", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmbc_addprop,   rpmbc_delprop, rpmbc_getprop, rpmbc_setprop,
    (JSEnumerateOp)rpmbc_enumerate, (JSResolveOp)rpmbc_resolve,
    rpmbc_convert,	rpmbc_dtor,

    rpmbc_getobjectops,	rpmbc_checkaccess,
    rpmbc_call,		rpmbc_construct,
    rpmbc_xdrobject,	rpmbc_hasinstance,
    rpmbc_mark,		rpmbc_reserveslots,
};

JSObject *
rpmjs_InitBcClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmbcClass, rpmbc_ctor, 1,
		rpmbc_props, rpmbc_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewBcObject(JSContext *cx)
{
    JSObject *obj;
    rpmbc bc;

    if ((obj = JS_NewObject(cx, &rpmbcClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((bc = rpmbc_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmbc, Bc, JS_TRUE)
