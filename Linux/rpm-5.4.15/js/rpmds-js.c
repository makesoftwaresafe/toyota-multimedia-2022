/** \ingroup js_c
 * \file js/rpmds-js.c
 */

#include "system.h"

#include "rpmds-js.h"
#include "rpmhdr-js.h"
#include "rpmjs-debug.h"

#include <argv.h>
#include <mire.h>

#include <rpmdb.h>

#include <rpmds.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	rpmds_addprop	JS_PropertyStub
#define	rpmds_delprop	JS_PropertyStub
#define	rpmds_convert	JS_ConvertStub

/* --- helpers */

/* --- Object methods */
static JSFunctionSpec rpmds_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmds_tinyid {
    _DEBUG	= -2,
    _LENGTH	= -3,
    _TYPE	= -4,
    _IX		= -5,
    _A		= -6,
    _BUILDTIME	= -7,
    _COLOR	= -8,
    _NOPROMOTE	= -9,
    _N		= -11,
    _EVR	= -12,
    _F		= -13,
    _DNEVR	= -14,
    _NS		= -15,
    _REFS	= -16,
    _RESULT	= -17,
};

static JSPropertySpec rpmds_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"length",	_LENGTH,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"type",	_TYPE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"ix",	_IX,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"buildtime",_BUILDTIME,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"color",	_COLOR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"nopromote",_NOPROMOTE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"A",	_A,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"N",	_N,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"EVR",	_EVR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"F",	_F,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"DNEVR",	_DNEVR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"NS",	_NS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"refs",	_REFS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"result",	_RESULT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmds_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = ptr;
    jsint tiny = JSVAL_TO_INT(id);
    int ix;

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    case _LENGTH:
	*vp = INT_TO_JSVAL(rpmdsCount(ds));
	break;
    case _TYPE:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsType(ds)));
        break;
    case _IX:
	*vp = INT_TO_JSVAL(rpmdsIx(ds));
        break;
    case _A:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsA(ds)));
        break;
    case _BUILDTIME:
	*vp = INT_TO_JSVAL(rpmdsBT(ds));
        break;
    case _COLOR:
	*vp = INT_TO_JSVAL(rpmdsColor(ds));
        break;
    case _NOPROMOTE:
	*vp = INT_TO_JSVAL(rpmdsNoPromote(ds));
        break;
    case _N:
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsN(ds)));
        break;
    case _EVR:
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsEVR(ds)));
        break;
    case _F:
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    *vp = INT_TO_JSVAL(rpmdsFlags(ds));
        break;
    case _DNEVR:
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsDNEVR(ds)));
        break;
    case _NS:
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    *vp = INT_TO_JSVAL(rpmdsNSType(ds));
        break;
    case _REFS:
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    *vp = INT_TO_JSVAL(rpmdsRefs(ds));
        break;
    case _RESULT:
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    *vp = INT_TO_JSVAL(rpmdsResult(ds));
        break;
    default:
	if (!(tiny >= 0 && tiny < rpmdsCount(ds)))
	    break;
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmds_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = (rpmds)ptr;
    jsint tiny = JSVAL_TO_INT(id);
    int myint;
    int ix;

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
    case _IX:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	if (myint < 0 || myint >= rpmdsCount(ds))
	    (void)rpmdsInit(ds);
	else {
if (_debug < 0)
fprintf(stderr, "\trpmdsSetIx(%p, %d)\n", ds, myint);
	    if ((myint-1) != rpmdsIx(ds))
		(void) rpmdsSetIx(ds, myint-1);
	    /* XXX flush and recreate N and DNEVR with a rpmdsNext() step */
	    (void) rpmdsNext(ds);
	}
        break;
    case _BUILDTIME:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmdsSetBT(ds, myint);
        break;
    case _COLOR:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmdsSetColor(ds, myint);
        break;
    case _NOPROMOTE:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmdsSetNoPromote(ds, myint);
        break;
    case _REFS:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    (void) rpmdsSetRefs(ds, myint);
        break;
    case _RESULT:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	if ((ix = rpmdsIx(ds)) >= 0 && ix < rpmdsCount(ds))
	    (void) rpmdsSetResult(ds, myint);
        break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmds_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = ptr;
    jsint ix;
    JSBool ok = JS_FALSE;

_RESOLVE_DEBUG_ENTRY(_debug < 0);

    if ((flags & JSRESOLVE_ASSIGNING)
     || (ds == NULL)) {	/* don't resolve to parent prototypes objects. */
	*objp = NULL;
	ok = JS_TRUE;
	goto exit;
    }

    if (JSVAL_IS_INT(id)
     && (ix = JSVAL_TO_INT(id)) >= 0 && ix < rpmdsCount(ds))
    {
	JSObject * arr = JS_NewArrayObject(cx, 3, NULL);
	JSString *valstr;
	const char *N = rpmdsN(ds);
	const char *EVR = rpmdsEVR(ds);
	unsigned Flags = rpmdsFlags(ds);

	if (ix != rpmdsIx(ds)) {
if (_debug < 0)
fprintf(stderr, "\trpmdsSetIx(%p, %d)\n", ds, ix);
	    (void) rpmdsSetIx(ds, ix-1);
	    (void) rpmdsNext(ds);
	}

	if (!JS_DefineElement(cx, obj, ix, OBJECT_TO_JSVAL(arr),
			NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
	if ((valstr = JS_NewStringCopyZ(cx, N)) == NULL
         || !JS_DefineElement(cx, arr, 0, STRING_TO_JSVAL(valstr),
                        NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
 	if ((valstr = JS_NewStringCopyZ(cx, EVR)) == NULL
         || !JS_DefineElement(cx, arr, 1, STRING_TO_JSVAL(valstr),
                        NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
        if (!JS_DefineElement(cx, arr, 2, INT_TO_JSVAL(Flags),
                        NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
	*objp = obj;
    } else
	*objp = NULL;

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmds_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = (rpmds)ptr;
    int ix;

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	*statep = JSVAL_VOID;
	(void) rpmdsInit(ds);
        if (idp)
            *idp = JSVAL_ZERO;
        break;
    case JSENUMERATE_NEXT:
	*statep = JSVAL_VOID;		/* XXX needed? */
	if ((ix = rpmdsNext(ds)) >= 0) {
	    JS_ValueToId(cx, INT_TO_JSVAL(ix), idp);
	} else
	    *idp = JSVAL_VOID;
	if (!JSID_IS_VOID(*idp))
            break;
        /*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	/* XXX Allow our iterator object to be GC'd. */
	*statep = JSVAL_NULL;
        break;
    }
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static rpmds
rpmds_init(JSContext *cx, JSObject *obj, jsval v, uint32_t _tagN)
{
    rpmds ds = NULL;
    JSObject * o = (JSVAL_IS_OBJECT(v) ? JSVAL_TO_OBJECT(v) : NULL);
    int xx;

    if (o && OBJ_IS_RPMHDR(cx, o)) {
	Header h = JS_GetPrivate(cx, o);
	int flags = 0;
	if (_tagN == RPMTAG_NAME) {
	    flags = RPMSENSE_EQUAL;
	    _tagN = RPMTAG_PROVIDENAME;
	    ds = rpmdsThis(h, _tagN, flags);
if (_debug)
fprintf(stderr, "\trpmdsThis(%p, %s(%d), 0x%x) ds %p\n", h, tagName(_tagN), _tagN, flags, ds);
	} else {
	    ds = rpmdsNew(h, _tagN, flags);
if (_debug)
fprintf(stderr, "\trpmdsNew(%p, %s(%d), 0x%x) ds %p\n", h, tagName(_tagN), _tagN, flags, ds);
	}
	if (ds == NULL)
	    return NULL;
    } else
    if (JSVAL_IS_STRING(v)) {
	const char * s = JS_EncodeString(cx, JS_ValueToString(cx, v));
	if (!strcmp(s, "cpuinfo")) {
	    xx = rpmdsCpuinfo(&ds, NULL);
if (_debug)
fprintf(stderr, "\trpmdsCpuinfo() ret %d ds %p\n", xx, ds);
	} else
	if (!strcmp(s, "rpmlib")) {
	    xx = rpmdsRpmlib(&ds, NULL);
if (_debug)
fprintf(stderr, "\trpmdsRpmlib() ret %d ds %p\n", xx, ds);
	} else
	if (!strcmp(s, "getconf")) {
	    xx = rpmdsGetconf(&ds, NULL);
if (_debug)
fprintf(stderr, "\trpmdsGetconf() ret %d ds %p\n", xx, ds);
	} else
	if (!strcmp(s, "uname")) {
	    xx = rpmdsUname(&ds, NULL);
if (_debug)
fprintf(stderr, "\trpmdsUname() ret %d ds %p\n", xx, ds);
	} else
	{
if (_debug)
fprintf(stderr, "\tstring \"%s\" is unknown. ds %p\n", s, ds);
	    s = _free(s);
	    return NULL;
	}
	s = _free(s);
    } else
    if (o && JS_IsArrayObject(cx, o)) {
	jsuint length = 0;
	JSBool ok = JS_GetArrayLength(cx, o, &length);
	const char * N = NULL;
	const char * EVR = NULL;
	uint32_t F = 0;
	jsuint i;

	if (!ok)
	    return NULL;
	if (length != 3)
	    return NULL;
	for (i = 0; i < length; i++) {
	    jsval v;
	    if (!(ok = JS_GetElement(cx, o, (jsint)i, &v)))
		return NULL;
	    switch (i) {
	    default:
		return NULL;
		/*@notreached@*/ break;
	    case 0:
		N = JS_EncodeString(cx, JSVAL_TO_STRING(v));
		break;
	    case 1:
		EVR = JS_EncodeString(cx, JSVAL_TO_STRING(v));
		break;
	    case 2:
		F = JSVAL_TO_INT(v);
		break;
	    }
	}
	ds = rpmdsSingle(_tagN, N, EVR, F);
if (_debug)
fprintf(stderr, "\trpmdsSingle(%s(%d), %s, %s, 0x%x) ds %p\n", tagName(_tagN), _tagN, N, EVR, F, ds);
	EVR = _free(EVR);
	N = _free(N);
	return NULL;
    } else {
if (_debug)
fprintf(stderr, "\tobject class %p is unknown. ds %p\n", JS_GET_CLASS(cx, o), ds);
	return NULL;
    }
    if (!JS_SetPrivate(cx, obj, (void *)ds)) {
	/* XXX error msg */
	(void) rpmdsFree(ds);
	return NULL;
    }
    return ds;
}

static void
rpmds_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    (void) rpmdsFree(ds);
}

static JSBool
rpmds_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    jsval v = JSVAL_VOID;
    uint32_t tagN = RPMTAG_REQUIRENAME;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "v/u", &v, &tagN)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	if (rpmds_init(cx, obj, v, tagN) == NULL)
	    goto exit;
    } else {
	if ((obj = JS_NewObject(cx, &rpmdsClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmdsClass = {
    "Ds", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmds_addprop,   rpmds_delprop, rpmds_getprop, rpmds_setprop,
    (JSEnumerateOp)rpmds_enumerate, (JSResolveOp)rpmds_resolve,
    rpmds_convert,	rpmds_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitDsClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &rpmdsClass, rpmds_ctor, 1,
		rpmds_props, rpmds_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

JSObject *
rpmjs_NewDsObject(JSContext *cx, jsval v, uint32_t _tagN)
{
    JSObject *obj;
    rpmds ds;

    if ((obj = JS_NewObject(cx, &rpmdsClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((ds = rpmds_init(cx, obj, v, _tagN)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmds, Ds, JS_TRUE)
