/** \ingroup js_c
 * \file js/rpmdc-js.c
 */

#include "system.h"

#include "rpmdc-js.h"
#include "rpmjs-debug.h"

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmdc_addprop		JS_PropertyStub
#define	rpmdc_delprop		JS_PropertyStub
#define	rpmdc_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmdc_getobjectops	NULL
#define	rpmdc_checkaccess	NULL
#define	rpmdc_call		rpmdc_call
#define	rpmdc_construct		rpmdc_ctor
#define	rpmdc_xdrobject		NULL
#define	rpmdc_hasinstance	NULL
#define	rpmdc_mark		NULL
#define	rpmdc_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmdc_equality		NULL
#define rpmdc_outerobject	NULL
#define rpmdc_innerobject	NULL
#define rpmdc_iteratorobject	NULL
#define rpmdc_wrappedobject	NULL

typedef	DIGEST_CTX rpmdc;	/* XXX use rpmdc from tools/digest.c instead */

/*@uncehcked@*/
static pgpHashAlgo _dalgo_default = PGPHASHALGO_MD5;

/* --- helpers */

/* --- Object methods */
static JSBool
rpmdc_Init(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdcClass, NULL);
    rpmdc dc = ptr;
    JSBool ok = JS_FALSE;
    unsigned int _dalgo = PGPHASHALGO_NONE;
    unsigned int _flags = RPMDIGEST_NONE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/uu", &_dalgo, &_flags)))
        goto exit;

    if (dc) {
	if (_dalgo == PGPHASHALGO_NONE) _dalgo = rpmDigestAlgo(dc);
	(void) rpmDigestFinal(dc, NULL, NULL, 0);
	(void) JS_SetPrivate(cx, obj, (void *)NULL);
    }
    if (_dalgo == PGPHASHALGO_NONE) _dalgo = _dalgo_default;
    dc = rpmDigestInit(_dalgo, _flags);
    (void) JS_SetPrivate(cx, obj, (void *)dc);

    *vp = JSVAL_TRUE;
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmdc_Update(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdcClass, NULL);
    rpmdc dc = ptr;
    JSBool ok = JS_FALSE;
    const char * s = NULL;
    size_t ns = 0;

_METHOD_DEBUG_ENTRY(_debug);

    *vp = JSVAL_FALSE;
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &s)))
        goto exit;
    if (dc == NULL)
	goto exit;

    if (s && (ns = strlen(s)) > 0)
	(void) rpmDigestUpdate(dc, s, ns);
    *vp = JSVAL_TRUE;

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmdc_Fini(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdcClass, NULL);
    rpmdc dc = ptr;
    JSBool ok = JS_FALSE;
    const char * s = NULL;
    size_t ns = 0;

_METHOD_DEBUG_ENTRY(_debug);

    *vp = JSVAL_FALSE;
    if (dc == NULL)
	goto exit;

    (void) rpmDigestFinal(dc, &s, &ns, 1);
    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, s));

    (void) JS_SetPrivate(cx, obj, (void *)NULL);

    ok = JS_TRUE;
exit:
    s = _free(s);
    return ok;
}

static JSFunctionSpec rpmdc_funcs[] = {
    JS_FS("init",	rpmdc_Init,		0,0),
    JS_FS("update",	rpmdc_Update,		0,0),
    JS_FS("fini",	rpmdc_Fini,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmdc_tinyid {
    _DEBUG	= -2,
    _ALGO	= -3,
    _ASN1	= -4,
    _NAME	= -5,
};

static JSPropertySpec rpmdc_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"algo",	_ALGO,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"asn1",	_ASN1,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"name",	_NAME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

/* XXX rpm digest getters handle NULL already */
#define	_GET_I(_p, _f)   ((_p) ? INT_TO_JSVAL((int)(_f)) : JSVAL_VOID)
#define	_GET_S(_p, _f) \
    ((_p) ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (_f))) : JSVAL_VOID)

static JSBool
rpmdc_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdcClass, NULL);
    rpmdc dc = ptr;
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:	*vp = INT_TO_JSVAL(_debug);		break;
    case _ALGO:		*vp = _GET_I(dc, rpmDigestAlgo(dc));	break;
    case _ASN1:		*vp = _GET_S(dc, rpmDigestASN1(dc));	break;
    case _NAME:		*vp = _GET_S(dc, rpmDigestName(dc));	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmdc_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdcClass, NULL);
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

#ifdef	NOTYET
static JSBool
rpmdc_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdcClass, NULL);

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
rpmdc_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
#else
#define	rpmdc_resolve	JS_ResolveStub
#define	rpmdc_enumerate	JS_EnumerateStub
#endif

/* --- Object ctors/dtors */
static rpmdc
rpmdc_init(JSContext *cx, JSObject *obj, unsigned int _dalgo,
		unsigned int _flags)
{
    rpmdc dc = (_dalgo > 0 ? rpmDigestInit(_dalgo, _flags) : NULL);

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%u,%u) dc %p\n", __FUNCTION__, cx, obj, _dalgo, _flags, dc);

    if (!JS_SetPrivate(cx, obj, (void *)dc)) {
	/* XXX error msg */
	return NULL;
    }
    return dc;
}

static void
rpmdc_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdcClass, NULL);
    rpmdc dc = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    if (dc)
	(void) rpmDigestFinal(dc, NULL, NULL, 0);
}

static JSBool
rpmdc_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    unsigned int _dalgo = PGPHASHALGO_NONE;
    unsigned int _flags = RPMDIGEST_NONE;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/uu", &_dalgo, &_flags)))
        goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmdc_init(cx, obj, _dalgo, _flags);
    } else {
	if ((obj = JS_NewObject(cx, &rpmdcClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdc_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmdcClass, NULL);
    rpmdc dc = ptr;
    JSBool ok = JS_FALSE;
    const char * s = NULL;
    unsigned int _dalgo = PGPHASHALGO_NONE;
    unsigned int _flags = RPMDIGEST_NONE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/s", &s)))
        goto exit;

    if (dc) {
	/* XXX retrieve current settings for persistence. */
	_dalgo = rpmDigestAlgo(dc);
	_flags = rpmDigestF(dc);
	(void) rpmDigestFinal(dc, NULL, NULL, 0);
	/* XXX error msg */
	dc = ptr = NULL;
	(void) JS_SetPrivate(cx, o, (void *)dc);
    }

    if (_dalgo == PGPHASHALGO_NONE) _dalgo = _dalgo_default;
    dc = ptr = rpmdc_init(cx, o, _dalgo, _flags);

    if (dc && s != NULL) {
	size_t ns = strlen(s);
	(void) rpmDigestUpdate(dc, s, ns);
	s = NULL;
	ns = 0;
	if (!rpmDigestFinal(dc, &s, &ns, 1)) {
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, s));
	} else
	    *vp = JSVAL_VOID;
	s = _free(s);

	/* XXX reinitialize so that _dalgo is persistent */
	dc = ptr = NULL;
	(void) JS_SetPrivate(cx, o, (void *)dc);
	dc = ptr = rpmdc_init(cx, o, _dalgo, _flags);
    } else
	*vp = JSVAL_VOID;

    ok = JS_TRUE;

exit:
if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);

    return ok;
}

/* --- Class initialization */
JSClass rpmdcClass = {
    /* XXX class should be "Digest" eventually, avoid name conflicts for now */
#ifdef	NOTYET
    "Dc", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
#else
    "Dc", JSCLASS_HAS_PRIVATE,
#endif
    rpmdc_addprop,   rpmdc_delprop, rpmdc_getprop, rpmdc_setprop,
    (JSEnumerateOp)rpmdc_enumerate, (JSResolveOp)rpmdc_resolve,
    rpmdc_convert,	rpmdc_dtor,

    rpmdc_getobjectops,	rpmdc_checkaccess,
    rpmdc_call,		rpmdc_construct,
    rpmdc_xdrobject,	rpmdc_hasinstance,
    rpmdc_mark,		rpmdc_reserveslots,
};

JSObject *
rpmjs_InitDcClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmdcClass, rpmdc_ctor, 1,
		rpmdc_props, rpmdc_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewDcObject(JSContext *cx, unsigned int _dalgo, unsigned int _flags)
{
    JSObject *obj;
    rpmdc dc;

    if ((obj = JS_NewObject(cx, &rpmdcClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((dc = rpmdc_init(cx, obj, _dalgo, _flags)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmdc, Dc, JS_TRUE)
