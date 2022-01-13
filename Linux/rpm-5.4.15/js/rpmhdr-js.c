/** \ingroup js_c
 * \file js/rpmhdr-js.c
 */

#include "system.h"

#include "rpmhdr-js.h"
#include "rpmds-js.h"
#include "rpmfi-js.h"
#include "rpmjs-debug.h"

#include <rpmcli.h>	/* XXX rpmHeaderFormats */

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	rpmhdr_addprop	JS_PropertyStub
#define	rpmhdr_delprop	JS_PropertyStub
#define	rpmhdr_convert	JS_ConvertStub

/* --- helpers */
static JSObject *
rpmhdrLoadTag(JSContext *cx, JSObject *obj, Header h, rpmTag tag, jsval *vp)
{
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    JSObject * arr;
    jsval v;
    JSObject * retobj = NULL;
    JSBool ok;
const char * name = tagName(tag);
    int i;

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%p,%s)\n", __FUNCTION__, cx, obj, h, name);

    he->tag = tag;
    if (headerGet(h, he, 0)) {
if (_debug < 0)
fprintf(stderr, "\t%s(%u) %u %p[%u]\n", name, (unsigned)he->tag, (unsigned)he->t, he->p.ptr, (unsigned)he->c);
	switch (he->t) {
	default:
	    goto exit;
	    /*@notreached@*/ break;
	case RPM_BIN_TYPE:	/* XXX return as array of octets for now. */
	case RPM_UINT8_TYPE:
	    arr = JS_NewArrayObject(cx, 0, NULL);
	    ok = JS_AddObjectRoot(cx, &arr);
	    for (i = 0; i < (int)he->c; i++) {
		v = INT_TO_JSVAL(he->p.ui8p[i]);
		ok = JS_SetElement(cx, arr, i, &v);
	    }
	    ok = JS_DefineProperty(cx, obj, name, (v=OBJECT_TO_JSVAL(arr)),
				NULL, NULL, JSPROP_ENUMERATE);
	    (void) JS_RemoveObjectRoot(cx, &arr);
	    if (!ok)
		goto exit;
	    retobj = obj;
	    if (vp) *vp = v;
	    break;
	case RPM_UINT16_TYPE:
	    arr = JS_NewArrayObject(cx, 0, NULL);
	    ok = JS_AddObjectRoot(cx, &arr);
	    for (i = 0; i < (int)he->c; i++) {
		v = INT_TO_JSVAL(he->p.ui16p[i]);
		ok = JS_SetElement(cx, arr, i, &v);
	    }
	    ok = JS_DefineProperty(cx, obj, name, (v=OBJECT_TO_JSVAL(arr)),
				NULL, NULL, JSPROP_ENUMERATE);
	    (void) JS_RemoveObjectRoot(cx, &arr);
	    if (!ok)
		goto exit;
	    retobj = obj;
	    if (vp) *vp = v;
	    break;
	case RPM_UINT32_TYPE:
	    arr = JS_NewArrayObject(cx, 0, NULL);
	    ok = JS_AddObjectRoot(cx, &arr);
	    for (i = 0; i < (int)he->c; i++) {
		if (!JS_NewNumberValue(cx, he->p.ui32p[i], &v))
		    v = JSVAL_VOID;
		ok = JS_SetElement(cx, arr, i, &v);
	    }
	    ok = JS_DefineProperty(cx, obj, name, (v=OBJECT_TO_JSVAL(arr)),
				NULL, NULL, JSPROP_ENUMERATE);
	    (void) JS_RemoveObjectRoot(cx, &arr);
	    if (!ok)
		goto exit;
	    retobj = obj;
	    if (vp) *vp = v;
	    break;
	case RPM_UINT64_TYPE:
	    arr = JS_NewArrayObject(cx, 0, NULL);
	    ok = JS_AddObjectRoot(cx, &arr);
	    for (i = 0; i < (int)he->c; i++) {
		if (!JS_NewNumberValue(cx, he->p.ui64p[i], &v))
		    v = JSVAL_VOID;
		ok = JS_SetElement(cx, arr, i, &v);
	    }
	    ok = JS_DefineProperty(cx, obj, name, (v=OBJECT_TO_JSVAL(arr)),
				NULL, NULL, JSPROP_ENUMERATE);
	    (void) JS_RemoveObjectRoot(cx, &arr);
	    if (!ok)
		goto exit;
	    retobj = obj;
	    if (vp) *vp = v;
	    break;
	case RPM_STRING_ARRAY_TYPE:
	    arr = JS_NewArrayObject(cx, 0, NULL);
	    ok = JS_AddObjectRoot(cx, &arr);
	    for (i = 0; i < (int)he->c; i++) {
		v = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, he->p.argv[i]));
		ok = JS_SetElement(cx, arr, i, &v);
	    }
	    ok = JS_DefineProperty(cx, obj, name, (v=OBJECT_TO_JSVAL(arr)),
				NULL, NULL, JSPROP_ENUMERATE);
	    (void) JS_RemoveObjectRoot(cx, &arr);
	    if (!ok)
		goto exit;
	    retobj = obj;
	    if (vp) *vp = v;
	    break;
	case RPM_I18NSTRING_TYPE:	/* XXX FIXME: is this ever seen? */
fprintf(stderr, "==> FIXME: %s(%d) t %d %p[%u]\n", tagName(he->tag), he->tag, he->t, he->p.ptr, he->c);
	    /*@fallthrough@*/
	case RPM_STRING_TYPE:
	     ok = JS_DefineProperty(cx, obj, name,
			(v=STRING_TO_JSVAL(JS_NewStringCopyZ(cx, he->p.str))),
			NULL, NULL, JSPROP_ENUMERATE);
	    if (!ok)
		goto exit;
	    retobj = obj;
	    if (vp) *vp = v;
	    break;
	}
    }

exit:
if (_debug < 0)
fprintf(stderr, "\tretobj %p vp %p *vp 0x%lx\n", retobj, vp, (unsigned long)(vp ? *vp : 0));
    return retobj;
}

/* --- Object methods */
static JSBool
rpmhdr_ds(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    rpmTag tagN = RPMTAG_NAME;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &tagN)))
        goto exit;
    *vp = OBJECT_TO_JSVAL(rpmjs_NewDsObject(cx, OBJECT_TO_JSVAL(obj), tagN));
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmhdr_fi(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    Header h = ptr;
    rpmTag tagN = RPMTAG_BASENAMES;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &tagN)))
        goto exit;
    *vp = OBJECT_TO_JSVAL(rpmjs_NewFiObject(cx, h, tagN));
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmhdr_sprintf(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    Header h = ptr;
    char * qfmt = NULL;
    const char * s = NULL;
    const char * errstr = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &qfmt)))
        goto exit;

    if ((s = headerSprintf(h, qfmt, NULL, rpmHeaderFormats, &errstr)) == NULL)
	s = errstr; 	/* XXX FIXME: returning errstr in-band. */
    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, s));
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmhdr_getorigin(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    Header h = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, headerGetOrigin(h)));
    ok = JS_TRUE;
    return ok;
}

static JSBool
rpmhdr_setorigin(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    Header h = ptr;
    char * s = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &s)))
        goto exit;

    (void) headerSetOrigin(h, s);
    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, headerGetOrigin(h)));
    ok = JS_TRUE;
exit:
    return ok;
}

static JSFunctionSpec rpmhdr_funcs[] = {
    JS_FS("ds",		rpmhdr_ds,		0,0),
    JS_FS("fi",		rpmhdr_fi,		0,0),
    JS_FS("sprintf",	rpmhdr_sprintf,		0,0),
    JS_FS("getorigin",	rpmhdr_getorigin,	0,0),
    JS_FS("setorigin",	rpmhdr_setorigin,	0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmhdr_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmhdr_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmhdr_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    Header h = ptr;
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    default: {
	const char * s = JS_EncodeString(cx, JS_ValueToString(cx, id));
	rpmTag tag = JSVAL_IS_INT(id)
		? (rpmTag) JSVAL_TO_INT(id)
		: tagValue(s);
	s = _free(s);
	if (rpmhdrLoadTag(cx, obj, h, tag, vp) == NULL)
	    break;
      } break;
    }

    return JS_TRUE;
}

static JSBool
rpmhdr_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
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
rpmhdr_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    Header h = ptr;
    JSBool ok = JS_FALSE;

_RESOLVE_DEBUG_ENTRY(_debug);

    if ((flags & JSRESOLVE_ASSIGNING)
     || (h == NULL)) {	/* don't resolve to parent prototypes objects. */
	*objp = NULL;
	ok = JS_TRUE;
	goto exit;
    }

    if (JSVAL_IS_INT(id) || JSVAL_IS_STRING(id)) {
	const char * s = JS_EncodeString(cx, JS_ValueToString(cx, id));
	rpmTag tag = JSVAL_IS_INT(id)
		? (rpmTag) JSVAL_TO_INT(id)
		: tagValue(s);
	JSObject * arr = rpmhdrLoadTag(cx, obj, h, tag, NULL);

	s = _free(s);
	if (!JS_DefineElement(cx, obj, tag, OBJECT_TO_JSVAL(arr),
			NULL, NULL, JSPROP_ENUMERATE)) {
	    *objp = NULL;
	    goto exit;
	}
	*objp = obj;
    } else
	*objp = NULL;
    ok = JS_TRUE;
exit:
    return ok;
}

static void
rpmhi_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhiClass, NULL);
    HeaderIterator hi = ptr;

if (_debug)
fprintf(stderr, "==> %s(%p,%p) ptr %p\n", __FUNCTION__, cx, obj, ptr);

    hi = headerFini(hi);
}

JSClass rpmhiClass = {
    "Hi",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub,  JS_ConvertStub,  rpmhi_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
rpmhdr_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    Header h = ptr;
    HeaderIterator hi = NULL;
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    JSObject *ho = NULL;
    JSBool ok = JS_FALSE;

_ENUMERATE_DEBUG_ENTRY(_debug);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	if ((ho = JS_NewObject(cx, &rpmhiClass, NULL, obj)) == NULL)
	    goto exit;
	if ((hi = headerInit(h)) == NULL)
	    goto exit;
	if (!JS_SetPrivate(cx, ho, (void *)hi)) {
	    hi = headerFini(hi);
	    goto exit;
	}
	*statep = OBJECT_TO_JSVAL(ho);
	if (idp)
	    *idp = JSVAL_ZERO;
if (_debug)
fprintf(stderr, "\tINIT ho %p hi %p\n", ho, hi);
	break;
    case JSENUMERATE_NEXT:
	ho = (JSObject *) JSVAL_TO_OBJECT(*statep);
	hi = JS_GetInstancePrivate(cx, ho, &rpmhiClass, NULL);
if (_debug)
fprintf(stderr, "\tNEXT ho %p hi %p\n", ho, hi);
	if (headerNext(hi, he, 0)) {
	    JS_ValueToId(cx, INT_TO_JSVAL(he->tag), idp);
	    he->p.ptr = _free(he->p.ptr);
	} else
	    *idp = JSVAL_VOID;
	if (!JSID_IS_VOID(*idp))
	    break;
	/*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	ho = (JSObject *) JSVAL_TO_OBJECT(*statep);
	hi = JS_GetInstancePrivate(cx, ho, &rpmhiClass, NULL);
if (_debug)
fprintf(stderr, "\tFINI ho %p hi %p\n", ho, hi);
	/* Allow our iterator object to be GC'd. */
	*statep = JSVAL_NULL;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

/* --- Object ctors/dtors */
static Header
rpmhdr_init(JSContext *cx, JSObject *obj, void * _h)
{
    Header h = (_h ? _h : headerNew());

    if (h == NULL)
	return NULL;
    if (!JS_SetPrivate(cx, obj, (void *)h)) {
	/* XXX error msg */
	h = headerFree(h);
	return NULL;
    }
    return h;
}

static void
rpmhdr_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmhdrClass, NULL);
    Header h = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    (void) headerFree(h);
}

static JSBool
rpmhdr_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    JSObject *tso = NULL;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/o", &tso)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	if (rpmhdr_init(cx, obj, NULL))
	    goto exit;
    } else {
	if ((obj = JS_NewObject(cx, &rpmhdrClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmhdrClass = {
    "Hdr", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmhdr_addprop, rpmhdr_delprop, rpmhdr_getprop, rpmhdr_setprop,
    (JSEnumerateOp)rpmhdr_enumerate, (JSResolveOp)rpmhdr_resolve,
    rpmhdr_convert,	rpmhdr_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitHdrClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &rpmhdrClass, rpmhdr_ctor, 1,
		rpmhdr_props, rpmhdr_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

JSObject *
rpmjs_NewHdrObject(JSContext *cx, void * _h)
{
    JSObject *obj;
    Header h;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, _h);

    if ((obj = JS_NewObject(cx, &rpmhdrClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((h = rpmhdr_init(cx, obj, _h)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmhdr, Hdr, JS_TRUE)
