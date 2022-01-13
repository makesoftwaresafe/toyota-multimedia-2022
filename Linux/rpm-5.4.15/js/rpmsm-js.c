/** \ingroup js_c
 * \file js/rpmsm-js.c
 */

#include "system.h"

#include "rpmsm-js.h"
#include "rpmjs-debug.h"

#if defined(WITH_SEMANAGE)
#include <semanage/semanage.h>
#endif
#define	_RPMSM_INTERNAL
#include <rpmsm.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmsm_addprop		JS_PropertyStub
#define	rpmsm_delprop		JS_PropertyStub
#define	rpmsm_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmsm_getobjectops	NULL
#define	rpmsm_checkaccess	NULL
#define	rpmsm_call		rpmsm_call
#define	rpmsm_construct		rpmsm_ctor
#define	rpmsm_xdrobject		NULL
#define	rpmsm_hasinstance	NULL
#define	rpmsm_mark		NULL
#define	rpmsm_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmsm_equality		NULL
#define rpmsm_outerobject	NULL
#define rpmsm_innerobject	NULL
#define rpmsm_iteratorobject	NULL
#define rpmsm_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSFunctionSpec rpmsm_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmsm_tinyid {
    _DEBUG	= -2,
    _FLAGS	= -3,
    _STATE	= -4,
    _ACCESS	= -5,
    _STORE	= -6,
};

static JSPropertySpec rpmsm_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"flags",	_FLAGS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"state",	_STATE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"access",	_ACCESS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"store",	_STORE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

#define	_GET_STR(_str)	\
    ((_str) ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (_str))) : JSVAL_VOID)
#define	_GET_CON(_test)	((_test) ? _GET_STR(con) : JSVAL_VOID)

static JSBool
rpmsm_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsmClass, NULL);
    rpmsm sm = ptr;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    case _FLAGS:	*vp = INT_TO_JSVAL(sm->flags);			break;
    case _STATE:	*vp = INT_TO_JSVAL(sm->state);			break;
    case _ACCESS:	*vp = INT_TO_JSVAL(sm->access);			break;
    case _STORE:	*vp = _GET_STR(sm->fn);				break;
    default:
	break;
    }

    return JS_TRUE;
}

#define	_PUT_CON(_put)	(JSVAL_IS_STRING(*vp) && !(_put) \
	? JSVAL_TRUE : JSVAL_FALSE)
#define	_PUT_INT(_put)	(JSVAL_IS_INT(*vp) && !(_put) \
	? JSVAL_TRUE : JSVAL_FALSE)

static JSBool
rpmsm_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsmClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmsm_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsmClass, NULL);

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
rpmsm_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static rpmsm
rpmsm_init(JSContext *cx, JSObject *obj, const char * _fn, unsigned int _flags)
{
    rpmsm sm = rpmsmNew(_fn, _flags);

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%s,0x%x) sm %p\n", __FUNCTION__, cx, obj, _fn, _flags, sm);

    if (!JS_SetPrivate(cx, obj, (void *)sm)) {
	/* XXX error msg */
	return NULL;
    }
    return sm;
}

static void
rpmsm_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsmClass, NULL);
    rpmsm sm = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    (void) rpmsmFree(sm);
}

static JSBool
rpmsm_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    const char * _fn = NULL;
    unsigned int _flags = 0;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/su", &_fn, &_flags)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmsm_init(cx, obj, _fn, _flags);
    } else {
	if ((obj = JS_NewObject(cx, &rpmsmClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmsm_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmsmClass, NULL);
    JSBool ok = JS_FALSE;
    rpmsm sm = ptr;
    const char *_cmd = NULL;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_cmd)))
        goto exit;

    if (sm && _cmd) {
	const char * av[2] = { _cmd, NULL };
	const char * result = NULL;
	/* XXX FIXME: pin down rc value. list returns nitems? */
	int rc = rpmsmRun(sm, (char **)av, &result);

	if (rc < 0)
	    *vp = JSVAL_FALSE;
	else if (rc == 0 || result == NULL)
	    *vp = JSVAL_TRUE;
	else
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, result));
    } else
	*vp = JSVAL_FALSE;

    ok = JS_TRUE;

exit:
if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);

    return ok;
}

/* --- Class initialization */
JSClass rpmsmClass = {
    "Sm", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmsm_addprop,   rpmsm_delprop, rpmsm_getprop, rpmsm_setprop,
    (JSEnumerateOp)rpmsm_enumerate, (JSResolveOp)rpmsm_resolve,
    rpmsm_convert,	rpmsm_dtor,

    rpmsm_getobjectops,	rpmsm_checkaccess,
    rpmsm_call,		rpmsm_construct,
    rpmsm_xdrobject,	rpmsm_hasinstance,
    rpmsm_mark,		rpmsm_reserveslots,
};

JSObject *
rpmjs_InitSmClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmsmClass, rpmsm_ctor, 1,
		rpmsm_props, rpmsm_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewSmObject(JSContext *cx, const char * _fn, unsigned int _flags)
{
    JSObject *obj;
    rpmsm sm;

    if ((obj = JS_NewObject(cx, &rpmsmClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((sm = rpmsm_init(cx, obj, _fn, _flags)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmsm, Sm, JS_TRUE)
