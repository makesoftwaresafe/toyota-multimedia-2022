/** \ingroup js_c
 * \file js/rpmmi-js.c
 */

#include "system.h"

#include "rpmts-js.h"
#include "rpmmi-js.h"
#include "rpmhdr-js.h"
#include "rpmjs-debug.h"

#include <rpmdb.h>
#include <rpmts.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	rpmmi_addprop	JS_PropertyStub
#define	rpmmi_delprop	JS_PropertyStub
#define	rpmmi_convert	JS_ConvertStub

/* --- helpers */

/* --- Object methods */
static JSBool
rpmmi_pattern(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
    rpmmi mi = ptr;
    jsval tagid = JSVAL_VOID;
    int tag = RPMTAG_NAME;
    rpmMireMode type = RPMMIRE_PCRE;
    char * pattern = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "vs/u", &tagid, &pattern, &type)))
	goto exit;

    switch (type) {
    default:
	ok = JS_FALSE;
	break;
    case RPMMIRE_DEFAULT:
    case RPMMIRE_STRCMP:
    case RPMMIRE_REGEX:
    case RPMMIRE_GLOB:
    case RPMMIRE_PCRE:
	if (!JSVAL_IS_VOID(tagid)) {
	    const char * s = JS_EncodeString(cx, JS_ValueToString(cx, tagid));
	    /* XXX TODO: make sure both tag and key were specified. */
	    tag = JSVAL_IS_INT(tagid)
		? (rpmTag) JSVAL_TO_INT(tagid)
		: tagValue(s);
	    s = _free(s);
	}
	rpmmiAddPattern(mi, tag, type, pattern);
	ok = JS_TRUE;
	break;
    }
    *vp = BOOLEAN_TO_JSVAL(ok);

exit:
    return ok;
}

static JSBool
rpmmi_prune(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
    rpmmi mi = ptr;
    uint32_t _u = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "u", &_u)))
	goto exit;
    /* XXX handle arrays */
    if (!rpmmiPrune(mi, &_u, 1, 1))
	ok = JS_TRUE;
    *vp = BOOLEAN_TO_JSVAL(ok);

exit:
    return ok;
}

static JSBool
rpmmi_grow(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
    rpmmi mi = ptr;
    uint32_t _u = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "u", &_u)))
	goto exit;
    /* XXX handle arrays */
    if (!rpmmiGrow(mi, &_u, 1))
	ok = JS_TRUE;
    *vp = BOOLEAN_TO_JSVAL(ok);

exit:
    return ok;
}

static JSBool
rpmmi_growbn(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
    rpmmi mi = ptr;
    const char * _bn = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_bn)))
	goto exit;
    /* XXX handle arrays */
    if (!rpmmiGrowBasename(mi, _bn))
	ok = JS_TRUE;
    *vp = BOOLEAN_TO_JSVAL(ok);

exit:
    return ok;
}

static JSFunctionSpec rpmmi_funcs[] = {
    JS_FS("pattern",	rpmmi_pattern,		0,0),
    JS_FS("prune",	rpmmi_prune,		0,0),
    JS_FS("grow",	rpmmi_grow,		0,0),
    JS_FS("growbn",	rpmmi_growbn,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmmi_tinyid {
    _DEBUG	= -2,
    _LENGTH	= -3,
    _COUNT	= -4,	/* XXX is _LENGTH enuf? */
    _INSTANCE	= -5,
};

static JSPropertySpec rpmmi_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"length",	_LENGTH,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"count",	_COUNT,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"instance",_INSTANCE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmmi_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
    rpmmi mi = ptr;
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    case _LENGTH:
    case _COUNT:	/* XXX is _LENGTH enuf? */
	*vp = INT_TO_JSVAL(rpmmiCount(mi));
        break;
    case _INSTANCE:
	*vp = INT_TO_JSVAL(rpmmiInstance(mi));
        break;
    default:
      {	JSObject *o = (JSVAL_IS_OBJECT(id) ? JSVAL_TO_OBJECT(id) : NULL);
	Header h = JS_GetInstancePrivate(cx, o, &rpmhdrClass, NULL);
	rpmuint32_t ix = headerGetInstance(h);
	if (ix != 0) {
	    *vp = id;
if (_debug)
fprintf(stderr, "\tGET  %p[%d] h %p\n", mi, ix, h);
	}
      }	break;
    }

    return JS_TRUE;
}

static JSBool
rpmmi_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
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
rpmmi_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
    rpmmi mi = ptr;
    JSObject *o = (JSVAL_IS_OBJECT(id) ? JSVAL_TO_OBJECT(id) : NULL);
    JSClass *c = (o ? JS_GET_CLASS(cx, o) : NULL);

_RESOLVE_DEBUG_ENTRY(_debug);

    if ((flags & JSRESOLVE_ASSIGNING)
     || (mi == NULL)) {	/* don't resolve to parent prototypes objects. */
	*objp = NULL;
	goto exit;
    }

    if (c == &rpmhdrClass) {
	Header h = JS_GetInstancePrivate(cx, o, &rpmhdrClass, NULL);
	rpmuint32_t ix = headerGetInstance(h);
	if (ix == 0
	 || !JS_DefineElement(cx, obj, ix, id, NULL, NULL, JSPROP_ENUMERATE))
	{
	    *objp = NULL;
            goto exit;
	}
if (_debug)
fprintf(stderr, "\tRESOLVE %p[%d] h %p\n", mi, ix, h);
	*objp = obj;
    } else
	*objp = NULL;

exit:
    return JS_TRUE;
}

static JSBool
rpmmi_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
    rpmmi mi = ptr;
    Header h;

_ENUMERATE_DEBUG_ENTRY(_debug);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	*statep = JSVAL_VOID;
	if (idp)
	    *idp = JSVAL_ZERO;
if (_debug)
fprintf(stderr, "\tINIT mi %p\n", mi);
	break;
    case JSENUMERATE_NEXT:
	*statep = JSVAL_VOID;		/* XXX needed? */
	if ((h = rpmmiNext(mi)) != NULL) {
	    h = headerLink(h);
	    JS_ValueToId(cx, OBJECT_TO_JSVAL(rpmjs_NewHdrObject(cx, h)), idp);
if (_debug)
fprintf(stderr, "\tNEXT mi %p h# %u %p\n", mi, (unsigned)headerGetInstance(h), h);
	} else
	    *idp = JSVAL_VOID;
	if (!JSID_IS_VOID(*idp))
	    break;
	/*@fallthrough@*/
    case JSENUMERATE_DESTROY:
if (_debug)
fprintf(stderr, "\tFINI mi %p\n", mi);
	/* XXX Allow our iterator object to be GC'd. */
	*statep = JSVAL_NULL;
	break;
    }
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static rpmmi
rpmmi_init(JSContext *cx, JSObject *obj, rpmts ts, int _tag, jsval v)
{
    rpmmi mi = NULL;
    uint32_t _u = 0;
    const char * s = NULL;
    void * _key = NULL;
    int _keylen = 0;

    if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v)) {
	_keylen = 0;
	_key = NULL;
    } else
    if (JSVAL_IS_NUMBER(v)) {
	if (!JS_ValueToECMAUint32(cx, v, &_u))
	    goto exit;
	_keylen = sizeof(_u);
	_key = (void *) &_u;
    } else
    if (JSVAL_IS_STRING(v)) {
	s = JS_EncodeString(cx, JS_ValueToString(cx, v));
	_keylen = strlen(s);
	_key = (void *)s;
    } else
	/* XXX TODO: handle key object as binary octet string. */
	goto exit;

    if ((mi = rpmtsInitIterator(ts, _tag, _key, _keylen)) == NULL)
	goto exit;
    if (!JS_SetPrivate(cx, obj, (void *)mi)) {
	/* XXX error msg */
	mi = rpmmiFree(mi);
	mi = NULL;
	goto exit;
    }

exit:
    s = _free(s);
    return mi;
}

static void
rpmmi_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmiClass, NULL);
    rpmmi mi = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    (void) rpmmiFree(mi);
}

static JSBool
rpmmi_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSObject *tso = NULL;
    jsval tagid = JSVAL_VOID;
    jsval kv = JSVAL_VOID;
    rpmTag tag = RPMDBI_PACKAGES;
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/vv", &tso, &tagid, &kv)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	rpmts ts = JS_GetInstancePrivate(cx, tso, &rpmtsClass, NULL);

	if (!JSVAL_IS_VOID(tagid)) {
	    const char * s = JS_EncodeString(cx, JS_ValueToString(cx, tagid));
	    /* XXX TODO: make sure both tag and key were specified. */
	    tag = JSVAL_IS_INT(tagid)
		? (rpmTag) JSVAL_TO_INT(tagid)
		: tagValue(s);
	    s = _free(s);
	}

	if (ts == NULL || rpmmi_init(cx, obj, ts, tag, kv))
	    goto exit;		/* XXX error msg */
    } else {
	if ((obj = JS_NewObject(cx, &rpmmiClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmmiClass = {
    "Mi", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmmi_addprop,   rpmmi_delprop, rpmmi_getprop, rpmmi_setprop,
    (JSEnumerateOp)rpmmi_enumerate, (JSResolveOp)rpmmi_resolve,
    rpmmi_convert,	rpmmi_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitMiClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &rpmmiClass, rpmmi_ctor, 1,
		rpmmi_props, rpmmi_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

JSObject *
rpmjs_NewMiObject(JSContext *cx, void * _ts, int _tag, jsval kv)
{
    JSObject *obj;
    rpmmi mi;

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%u(%s),%u(%s))\n", __FUNCTION__, cx, _ts, (unsigned)_tag, tagName(_tag), (unsigned)kv, v2s(cx, kv));

    if ((obj = JS_NewObject(cx, &rpmmiClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((mi = rpmmi_init(cx, obj, _ts, _tag, kv)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmmi, Mi, JS_TRUE)
