/** \ingroup js_c
 * \file js/rpmcudf-js.c
 */

#include "system.h"

#include "rpmcudf-js.h"
#include "rpmjs-debug.h"

#define	_RPMCUDF_INTERNAL
#include <rpmcudf.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmcudf_addprop		JS_PropertyStub
#define	rpmcudf_delprop		JS_PropertyStub
#define	rpmcudf_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmcudf_getobjectops	NULL
#define	rpmcudf_checkaccess	NULL
#define	rpmcudf_call		NULL
#define	rpmcudf_construct		rpmcudf_ctor
#define	rpmcudf_xdrobject		NULL
#define	rpmcudf_hasinstance	NULL
#define	rpmcudf_mark		NULL
#define	rpmcudf_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmcudf_equality		NULL
#define rpmcudf_outerobject	NULL
#define rpmcudf_innerobject	NULL
#define rpmcudf_iteratorobject	NULL
#define rpmcudf_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */
static JSBool
rpmcudf_issolution(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmcudfClass, NULL);
    rpmcudf cudf = ptr;
    JSObject *fno = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o", &fno)))
	goto exit;

    if (fno && JSVAL_IS_STRING(OBJECT_TO_JSVAL(fno))) {
	const char * _fn =
		JS_EncodeString(cx, JS_ValueToString(cx, OBJECT_TO_JSVAL(fno)));
	int _flags = RPMCUDV_CUDF;	/* XXX FIXME */
	const char * _av[] = { _fn, NULL };
	rpmcudf Y = rpmcudfNew((char **)_av, _flags);
	*vp = (rpmcudfIsSolution(cudf, Y) ? JSVAL_TRUE : JSVAL_FALSE);
	Y = rpmcudfFree(Y);
	_fn = _free(_fn);
    } else
	*vp = JSVAL_VOID;
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmcudf_print(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmcudfClass, NULL);
    rpmcudf cudf = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (cudf && cudf->V.typ == RPMCUDV_CUDFDOC) {
	rpmcudfPrintPreamble(cudf);
	rpmcudfPrintRequest(cudf);
	rpmcudfPrintUniverse(cudf);
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmiobStr(cudf->iob)));
	(void) rpmiobEmpty(cudf->iob);
    } else
	*vp = JSVAL_VOID;
    ok = JS_TRUE;

    return ok;
}

static JSFunctionSpec rpmcudf_funcs[] = {
    JS_FS("issolution",	rpmcudf_issolution,		0,0),
    JS_FS("print",	rpmcudf_print,			0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmcudf_tinyid {
    _DEBUG		= -2,
    _HASPREAMBLE	= -3,
    _HASREQUEST		= -4,
    _ISCONSISTENT	= -5,
    _INSTALLEDSIZE	= -6,
    _UNIVERSESIZE	= -7,
    _PREAMBLE		= -8,
    _REQUEST		= -9,
    _UNIVERSE		= -10,
};

static JSPropertySpec rpmcudf_props[] = {
    {"debug",		_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"haspreamble",	_HASPREAMBLE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"hasrequest",	_HASREQUEST,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"isconsistent",	_ISCONSISTENT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"installedsize",	_INSTALLEDSIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"universesize",	_UNIVERSESIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"preamble",	_PREAMBLE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"request",		_REQUEST,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"universe",	_UNIVERSE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmcudf_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmcudfClass, NULL);
    rpmcudf cudf = ptr;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    case _HASPREAMBLE:
	*vp = (cudf && rpmcudfHasPreamble(cudf) ? JSVAL_TRUE : JSVAL_FALSE);
	break;
    case _HASREQUEST:
	*vp = (cudf && rpmcudfHasRequest(cudf) ? JSVAL_TRUE : JSVAL_FALSE);
	break;
    case _ISCONSISTENT:
	*vp = (cudf && rpmcudfIsConsistent(cudf) ? JSVAL_TRUE : JSVAL_FALSE);
	break;
    case _INSTALLEDSIZE:
	*vp = (cudf ? INT_TO_JSVAL(rpmcudfInstalledSize(cudf)) : JSVAL_VOID);
	break;
    case _UNIVERSESIZE:
	*vp = (cudf ? INT_TO_JSVAL(rpmcudfUniverseSize(cudf)) : JSVAL_VOID);
	break;
    case _PREAMBLE:
	if (cudf && cudf->V.typ == RPMCUDV_CUDFDOC) {
	    rpmcudfPrintPreamble(cudf);
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmiobStr(cudf->iob)));
	    (void) rpmiobEmpty(cudf->iob);
	} else
	    *vp = JSVAL_VOID;
	break;
    case _REQUEST:
	if (cudf && cudf->V.typ == RPMCUDV_CUDFDOC) {
	    rpmcudfPrintRequest(cudf);
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmiobStr(cudf->iob)));
	    (void) rpmiobEmpty(cudf->iob);
	} else
	    *vp = JSVAL_VOID;
	break;
    case _UNIVERSE:
	if (cudf && cudf->V.typ == RPMCUDV_CUDFDOC) {
	    rpmcudfPrintUniverse(cudf);
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmiobStr(cudf->iob)));
	    (void) rpmiobEmpty(cudf->iob);
	} else
	    *vp = JSVAL_VOID;
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmcudf_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmcudfClass, NULL);
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
rpmcudf_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmcudfClass, NULL);

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
rpmcudf_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static rpmcudf
rpmcudf_init(JSContext *cx, JSObject *obj, JSObject *fno, int _flags)
{
    const char * fn = NULL;
    rpmcudf cudf = NULL;

    if (fno && JSVAL_IS_STRING(OBJECT_TO_JSVAL(fno)))
	fn = JS_EncodeString(cx, JS_ValueToString(cx, OBJECT_TO_JSVAL(fno)));

    switch (_flags) {
    default:	_flags = RPMCUDV_CUDF;	break;
    case RPMCUDV_CUDFDOC:
    case RPMCUDV_CUDF:
	break;
    }

    {	const char *_av[] = { fn, NULL };
	cudf = rpmcudfNew((char **)_av, _flags);
    }

    if (!JS_SetPrivate(cx, obj, (void *)cudf)) {
	/* XXX error msg */
	cudf = rpmcudfFree(cudf);
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p, %p, %d) cudf %p fn %s\n", __FUNCTION__, cx, obj, fno, _flags, cudf, fn);

    fn = _free(fn);

    return cudf;
}

static void
rpmcudf_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmcudfClass, NULL);
    rpmcudf cudf = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    cudf = rpmcudfFree(cudf);
}

static JSBool
rpmcudf_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    JSObject *fno = NULL;
    int _flags = 0;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/oi", &fno, &_flags)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmcudf_init(cx, obj, fno, _flags);
    } else {
	if ((obj = JS_NewObject(cx, &rpmcudfClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmcudfClass = {
    "Cudf", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmcudf_addprop,   rpmcudf_delprop, rpmcudf_getprop, rpmcudf_setprop,
    (JSEnumerateOp)rpmcudf_enumerate, (JSResolveOp)rpmcudf_resolve,
    rpmcudf_convert,	rpmcudf_dtor,

    rpmcudf_getobjectops,	rpmcudf_checkaccess,
    rpmcudf_call,		rpmcudf_construct,
    rpmcudf_xdrobject,	rpmcudf_hasinstance,
    rpmcudf_mark,		rpmcudf_reserveslots,
};

JSObject *
rpmjs_InitCudfClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmcudfClass, rpmcudf_ctor, 1,
		rpmcudf_props, rpmcudf_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewCudfObject(JSContext *cx, JSObject *fno, int _flags)
{
    JSObject *obj;
    rpmcudf cudf;

    if ((obj = JS_NewObject(cx, &rpmcudfClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((cudf = rpmcudf_init(cx, obj, fno, _flags)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmcudf, Cudf, JS_TRUE)
