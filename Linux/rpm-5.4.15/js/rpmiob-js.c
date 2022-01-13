/** \ingroup js_c
 * \file js/rpmiob-js.c
 */

#include "system.h"

#define	_RPMIOB_INTERNAL
#include "rpmiob-js.h"
#include "rpmjs-debug.h"

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmiob_addprop		JS_PropertyStub
#define	rpmiob_delprop		JS_PropertyStub
#define	rpmiob_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmiob_getobjectops	NULL
#define	rpmiob_checkaccess	NULL
#define	rpmiob_call		NULL
#define	rpmiob_construct	rpmiob_ctor
#define	rpmiob_xdrobject	NULL
#define	rpmiob_hasinstance	NULL
#define	rpmiob_mark		NULL
#define	rpmiob_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmiob_equality		NULL
#define rpmiob_outerobject	NULL
#define rpmiob_innerobject	NULL
#define rpmiob_iteratorobject	NULL
#define rpmiob_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSFunctionSpec rpmiob_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmiob_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmiob_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmiob_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmiobClass, NULL);
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
rpmiob_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmiobClass, NULL);
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
rpmiob_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmiobClass, NULL);

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
rpmiob_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static rpmiob
rpmiob_init(JSContext *cx, JSObject *obj)
{
    rpmiob iob = rpmiobNew(0);

if (_debug)
fprintf(stderr, "==> %s(%p,%p) iob %p\n", __FUNCTION__, cx, obj, iob);

    if (!JS_SetPrivate(cx, obj, (void *)iob)) {
	/* XXX error msg */
	return NULL;
    }
    return iob;
}

static void
rpmiob_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmiobClass, NULL);
    rpmiob iob = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    if (iob)
	(void) rpmiobFree(iob);
}

static JSBool
rpmiob_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmiob_init(cx, obj);
    } else {
	if ((obj = JS_NewObject(cx, &rpmiobClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmiobClass = {
    /* XXX class should be "Buf" eventually, avoid name conflicts for now */
    "Iob", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmiob_addprop,   rpmiob_delprop, rpmiob_getprop, rpmiob_setprop,
    (JSEnumerateOp)rpmiob_enumerate, (JSResolveOp)rpmiob_resolve,
    rpmiob_convert,	rpmiob_dtor,

    rpmiob_getobjectops,	rpmiob_checkaccess,
    rpmiob_call,		rpmiob_construct,
    rpmiob_xdrobject,	rpmiob_hasinstance,
    rpmiob_mark,		rpmiob_reserveslots,
};

JSObject *
rpmjs_InitIobClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmiobClass, rpmiob_ctor, 1,
		rpmiob_props, rpmiob_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewIobObject(JSContext *cx)
{
    JSObject *obj;
    rpmiob iob;

    if ((obj = JS_NewObject(cx, &rpmiobClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((iob = rpmiob_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmiob, Iob, JS_TRUE)
