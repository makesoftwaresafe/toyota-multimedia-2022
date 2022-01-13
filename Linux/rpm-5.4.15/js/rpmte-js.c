/** \ingroup js_c
 * \file js/rpmte-js.c
 */

#include "system.h"

#include "rpmts-js.h"
#include "rpmte-js.h"
#include "rpmds-js.h"
#include "rpmfi-js.h"
#include "rpmhdr-js.h"
#include "rpmjs-debug.h"

#include <argv.h>
#include <mire.h>

#include <rpmdb.h>

#include <rpmds.h>
#include <rpmfi.h>
#include <rpmal.h>
#include <rpmts.h>
#define	_RPMTE_INTERNAL		/* XXX for rpmteNew/rpmteFree */
#include <rpmte.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	rpmte_addprop	JS_PropertyStub
#define	rpmte_delprop	JS_PropertyStub
#define	rpmte_convert	JS_ConvertStub

/* --- helpers */

/* --- Object methods */
static JSBool
rpmte_ds(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmteClass, NULL);
    rpmte te = ptr;
    rpmTag tagN = RPMTAG_NAME;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/i", &tagN)))
        goto exit;
    {	rpmds ds = NULL;
	JSObject *dso = NULL;
	if ((ds = rpmteDS(te, tagN)) == NULL)
	    *vp = JSVAL_NULL;
	else
	if ((dso = JS_NewObject(cx, &rpmdsClass, NULL, NULL)) != NULL
	 && JS_SetPrivate(cx, dso, rpmdsLink(ds, __FUNCTION__)))
	    *vp = OBJECT_TO_JSVAL(dso);
	else {
	    ds = rpmdsFree(ds);
	    *vp = JSVAL_VOID;
	}
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmte_fi(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmteClass, NULL);
    rpmte te = ptr;
    rpmTag tagN = RPMTAG_BASENAMES;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/i", &tagN)))
        goto exit;
    {	rpmfi fi = NULL;
	JSObject *fio = NULL;
	if ((fi = rpmteFI(te, tagN)) == NULL)
	    *vp = JSVAL_NULL;
	else
	if ((fio = JS_NewObject(cx, &rpmfiClass, NULL, NULL)) != NULL
	 && JS_SetPrivate(cx, fio, rpmfiLink(fi, __FUNCTION__)))
	    *vp = OBJECT_TO_JSVAL(fio);
	else {
	    fi = rpmfiFree(fi);
	    *vp = JSVAL_VOID;
	}
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSFunctionSpec rpmte_funcs[] = {
    JS_FS("ds",		rpmte_ds,		0,0),
    JS_FS("fi",		rpmte_fi,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmte_tinyid {
    _DEBUG	= -2,
    _TYPE	= -3,
    _N		= -4,
    _E		= -5,
    _V		= -6,
    _R		= -7,
    _A		= -8,
    _O		= -9,
    _NEVR	= -10,
    _NEVRA	= -11,
    _PKGID	= -12,
    _HDRID	= -13,
    _COLOR	= -14,
    _PKGFSIZE	= -15,
    _BREADTH	= -16,
    _DEPTH	= -17,
    _NPREDS	= -18,
    _DEGREE	= -19,
    _PARENT	= -20,
    _TREE	= -21,
    _ADDEDKEY	= -22,
    _DBOFFSET	= -23,
    _KEY	= -24,
    _SOURCERPM	= -25,
};

static JSPropertySpec rpmte_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"type",	_TYPE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"N",	_N,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"E",	_E,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"V",	_V,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"R",	_R,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"A",	_A,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"O",	_O,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"NEVR",	_NEVR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"NEVRA",	_NEVRA,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"pkgid",	_PKGID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"hdrid",	_HDRID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"color",	_COLOR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"pkgfsize",_PKGFSIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"breadth",	_BREADTH,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"depth",	_DEPTH,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"npreds",	_NPREDS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"degree",	_DEGREE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"parent",	_PARENT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"tree",	_TREE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"addedkey",_ADDEDKEY,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"dboffset",_DBOFFSET,	JSPROP_ENUMERATE,	NULL,	NULL},
#ifdef	NOTYET
    {"key",	_KEY,		JSPROP_ENUMERATE,	NULL,	NULL},
#endif
    {"sourcerpm",_SOURCERPM,	JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmte_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmteClass, NULL);
    rpmte te = ptr;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    case _TYPE:
	*vp = INT_TO_JSVAL(rpmteType(te));	/* XXX should be string */
	break;
    case _N:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteN(te)));
	break;
    case _E:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteE(te)));
	break;
    case _V:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteV(te)));
	break;
    case _R:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteR(te)));
	break;
    case _A:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteA(te)));
	break;
    case _O:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteO(te)));
	break;
    case _NEVR:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteNEVR(te)));
	break;
    case _NEVRA:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteNEVRA(te)));
	break;
    case _PKGID:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmtePkgid(te)));
	break;
    case _HDRID:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteHdrid(te)));
	break;
    case _COLOR:
	*vp = INT_TO_JSVAL(rpmteColor(te));
	break;
    case _PKGFSIZE:
	*vp = INT_TO_JSVAL(rpmtePkgFileSize(te));
	break;
    case _BREADTH:
	*vp = INT_TO_JSVAL(rpmteBreadth(te));
	break;
    case _DEPTH:
	*vp = INT_TO_JSVAL(rpmteDepth(te));
	break;
    case _NPREDS:
	*vp = INT_TO_JSVAL(rpmteNpreds(te));
	break;
    case _DEGREE:
	*vp = INT_TO_JSVAL(rpmteDegree(te));
	break;
    case _PARENT:
#ifdef	DYING
	*vp = INT_TO_JSVAL(rpmteColor(te));
#else
	*vp = JSVAL_VOID;
#endif
	break;
    case _TREE:
	*vp = INT_TO_JSVAL(rpmteTree(te));
	break;
    case _ADDEDKEY:
	/* XXX FIXME */
	*vp = INT_TO_JSVAL((int)((long)rpmteAddedKey(te)));
	break;
    case _DBOFFSET:
	*vp = INT_TO_JSVAL(rpmteDBOffset(te));
	break;
#ifdef	NOTYET
    case _KEY:
	*vp = INT_TO_JSVAL(rpmteColor(te));
	break;
#endif
    case _SOURCERPM:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmteSourcerpm(te)));
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmte_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmteClass, NULL);
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
rpmte_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmteClass, NULL);

_RESOLVE_DEBUG_ENTRY(_debug);

    if (flags & JSRESOLVE_ASSIGNING) {
	*objp = NULL;
	goto exit;
    }

    *objp = obj;        /* XXX always resolve in this object. */

exit:
    return JS_TRUE;
}

static JSBool
rpmte_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{

_ENUMERATE_DEBUG_ENTRY(_debug);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	*statep = JSVAL_VOID;
	if (idp)
	    *idp = JSVAL_ZERO;
	break;
    case JSENUMERATE_NEXT:
	*statep = JSVAL_VOID;
	    *idp = JSVAL_VOID;	/* XXX immediate terminate */
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
static rpmte
rpmte_init(JSContext *cx, JSObject *obj, rpmts ts, JSObject *hdro)
{
    rpmte te = NULL;
    rpmElementType etype = TR_ADDED;
    fnpyKey key = NULL;
    rpmRelocation relocs = NULL;
    uint32_t dboffset = 0;
    alKey pkgKey = NULL;

    if (hdro != NULL) {
	Header h = JS_GetInstancePrivate(cx, hdro, &rpmhdrClass, NULL);
	if ((te = rpmteNew(ts, h, etype, key, relocs, dboffset, pkgKey)) == NULL)
	    return NULL;
    }
    if (!JS_SetPrivate(cx, obj, (void *)te)) {
	/* XXX error msg */
	(void) rpmteFree(te);
	return NULL;
    }
    return te;
}

static void
rpmte_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmteClass, NULL);

_DTOR_DEBUG_ENTRY(_debug);

#ifdef	BUGGY	/* XXX the ts object holds an implicit reference currently. */
    {	rpmte te = ptr;
	if (te != NULL)
	    (void) rpmteFree(te);
    }
#endif
}

static JSBool
rpmte_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    JSObject *tso = NULL;
    JSObject *hdro = NULL;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/o", &tso, &hdro)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	rpmts ts = JS_GetInstancePrivate(cx, tso, &rpmtsClass, NULL);
	if (rpmte_init(cx, obj, ts, hdro) == NULL)
	    goto exit;
    } else {
	if ((obj = JS_NewObject(cx, &rpmteClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmteClass = {
    "Te", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmte_addprop,   rpmte_delprop, rpmte_getprop, rpmte_setprop,
    (JSEnumerateOp)rpmte_enumerate, (JSResolveOp)rpmte_resolve,
    rpmte_convert,	rpmte_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitTeClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &rpmteClass, rpmte_ctor, 1,
		rpmte_props, rpmte_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

JSObject *
rpmjs_NewTeObject(JSContext *cx, void * _ts, void * _hdro)
{
    JSObject *obj;
    rpmte te;

    if ((obj = JS_NewObject(cx, &rpmteClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((te = rpmte_init(cx, obj, _ts, _hdro)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmte, Te, JS_TRUE)
