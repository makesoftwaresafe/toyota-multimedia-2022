/** \ingroup js_c
 * \file js/rpmts-js.c
 */

#include "system.h"

#include "rpmts-js.h"
#include "rpmte-js.h"
#include "rpmmi-js.h"
#include "rpmjs-debug.h"

#include <rpmlog.h>
#include <rpmcb.h>
#include <argv.h>
#include <mire.h>

#include <rpmdb.h>

#define	_RPMTS_INTERNAL
#include <rpmts.h>
#include <rpmte.h>

#ifdef	NOTYET
/* XXX FIXME: deadlock if used. ts holds implcit reference currently. */
#define rpmteLink(_te)    \
    ((rpmte)rpmioLinkPoolItem((rpmioItem)(_te), __FUNCTION__, __FILE__, __LINE__))
#define rpmteUnlink(_te)    \
    ((rpmte)rpmioUnlinkPoolItem((rpmioItem)(_te), __FUNCTION__, __FILE__, __LINE__))
#else
#define	rpmteLink(_te)		(_te)
#define	rpmteUnlink(_te)	(_te)
#endif

#include <rpmcli.h>	/* XXX rpmcliInstall{Check,Order,Run} */

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	rpmts_addprop	JS_PropertyStub
#define	rpmts_delprop	JS_PropertyStub
#define	rpmts_convert	JS_ConvertStub

/* --- helpers */

#ifdef	DYING
static JSObject *
rpmtsLoadNVRA(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    JSObject * NVRA = JS_NewArrayObject(cx, 0, NULL);
    ARGV_t keys = NULL;
    int nkeys;
    int xx;
    int i;

    if (ts->rdb == NULL)
	(void) rpmtsOpenDB(ts, O_RDONLY);

    xx = rpmdbMireApply(rpmtsGetRdb(ts), RPMTAG_NVRA,
		RPMMIRE_STRCMP, NULL, &keys);
    nkeys = argvCount(keys);

    if (keys)
    for (i = 0; i < nkeys; i++) {
	JSString * valstr = JS_NewStringCopyZ(cx, keys[i]);
	jsval id = STRING_TO_JSVAL(valstr);
	JS_SetElement(cx, NVRA, i, &id);
    }

    JS_DefineProperty(cx, obj, "NVRA", OBJECT_TO_JSVAL(NVRA),
				NULL, NULL, JSPROP_ENUMERATE);

if (_debug)
fprintf(stderr, "==> %s(%p,%p) ptr %p NVRA %p\n", __FUNCTION__, cx, obj, ptr, NVRA);

    return NVRA;
}
#endif

/* --- Object methods */
static JSBool
rpmts_add(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    char * pkgN = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &pkgN)))
        goto exit;

    if (pkgN != NULL) {
	rpmmi mi;
	Header h;
	int upgrade = 0;
	int xx;

	switch (*pkgN) {
	case '-':	pkgN++;		upgrade = -1;	break;
	case '+':	pkgN++;		upgrade = 1;	break;
	default:			upgrade = 1;	break;
	}
	mi = rpmtsInitIterator(ts, RPMTAG_NVRA, pkgN, 0);
	while ((h = rpmmiNext(mi)) != NULL) {
	    xx = (upgrade >= 0)
	        ? rpmtsAddInstallElement(ts, h, (fnpyKey)pkgN, upgrade, NULL)
		: rpmtsAddEraseElement(ts, h, rpmmiInstance(mi));
	    break;
	}
	mi = rpmmiFree(mi);
    }

    ok = JS_TRUE;
exit:
    *vp = BOOLEAN_TO_JSVAL(ok);	/* XXX return error */
    return ok;
}

static JSBool
rpmts_check(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (rpmtsNElements(ts) > 0)
	(void) rpmcliInstallCheck(ts);	/* XXX print ps for now */
    ok = JS_TRUE;

    *vp = BOOLEAN_TO_JSVAL(ok);	/* XXX return error */
    return ok;
}

static JSBool
rpmts_order(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (rpmtsNElements(ts) > 0)
	(void) rpmcliInstallOrder(ts);	/* XXX print ps for now */
    ok = JS_TRUE;

    *vp = BOOLEAN_TO_JSVAL(ok);	/* XXX return error */
    return ok;
}

static JSBool
rpmts_run(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

#ifdef	NOTYET
    /* XXX force --test instead. */
    if (rpmtsNElements(ts) > 0)
	(void) rpmcliInstallRun(ts, NULL, 0);
#else
    rpmtsEmpty(ts);
#endif
    ok = JS_TRUE;

    *vp = BOOLEAN_TO_JSVAL(ok);	/* XXX return error */
    return ok;
}

static JSBool
rpmts_mi(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    jsval tagid = JSVAL_VOID;
    jsval kv = JSVAL_VOID;
    rpmTag tag = RPMDBI_PACKAGES;
    JSObject *mio;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/vv", &tagid, &kv)))
        goto exit;

    if (!JSVAL_IS_VOID(tagid)) {
	const char * s = JS_EncodeString(cx, JS_ValueToString(cx, tagid));
	/* XXX TODO: make sure both tag and key were specified. */
	tag = JSVAL_IS_INT(tagid)
		? (rpmTag) JSVAL_TO_INT(tagid)
		: tagValue(s);
	s = _free(s);
    }

    if ((mio = rpmjs_NewMiObject(cx, ts, tag, kv)) == NULL)
	goto exit;

    *vp = OBJECT_TO_JSVAL(mio);
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmts_dbrebuild(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    /* XXX rebuild requires root. */
    if (getuid())
	*vp = JSVAL_VOID;
    else
	*vp = (ts && !rpmtsRebuildDB(ts)) ? JSVAL_TRUE : JSVAL_FALSE;
    ok = JS_TRUE;

    return ok;
}

static JSBool
rpmts_dbkeys(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    jsval tagid = JSVAL_VOID;
    jsval v = JSVAL_VOID;
    rpmTag tag = RPMTAG_NVRA;
    rpmMireMode _mode = RPMMIRE_PCRE;
    const char * _pat = NULL;
    ARGV_t _av = NULL;
    JSBool ok = JS_FALSE;
    int xx;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/vvu", &v, &tagid, &v, &_mode)))
        goto exit;

    if (!JSVAL_IS_VOID(tagid)) {
	const char * s = JS_EncodeString(cx, JS_ValueToString(cx, tagid));
	/* XXX TODO: make sure both tag and key were specified. */
	tag = JSVAL_IS_INT(tagid)
		? (rpmTag) JSVAL_TO_INT(tagid)
		: tagValue(s);
	s = _free(s);
    }

    if (JSVAL_IS_VOID(v))
	_pat = xstrdup("^.*$");
    else if (JSVAL_IS_NULL(v))
	_pat = NULL;
    else if (JSVAL_IS_STRING(v))
	_pat = JS_EncodeString(cx, JS_ValueToString(cx, v));
#ifdef	NOTYET
    else if (JSVAL_IS_NUMBER(v)) {
	uint32_t _u = 0;
	if (!JS_ValueToECMAUint32(cx, v, &_u)) {
	    *vp = JSVAL_VOID;
	    goto exit;
	}
    }
#endif
    else
	_pat = xstrdup("^a.*$");

    switch (_mode) {
    default:
	*vp = JSVAL_VOID;
	goto exit;
	break;
    case RPMMIRE_DEFAULT:
    case RPMMIRE_STRCMP:
    case RPMMIRE_REGEX:
    case RPMMIRE_GLOB:
    case RPMMIRE_PCRE:
	break;
    }

    if (rpmtsGetRdb(ts) == NULL)
	xx = rpmtsOpenDB(ts, O_RDONLY);

    if (rpmdbMireApply(rpmtsGetRdb(ts), tag, _mode, _pat, &_av))
	*vp = JSVAL_VOID;
    else if (_av == NULL || _av[0] == NULL)
	*vp = JSVAL_NULL;
    else {
	int _ac = argvCount(_av);
	int i;
	JSObject * arr = JS_NewArrayObject(cx, 0, NULL);
	*vp = OBJECT_TO_JSVAL(arr);

	for (i = 0; i < _ac; i++) {
	    v = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _av[i]));
	    ok = JS_SetElement(cx, arr, i, &v);
	}
    }

exit:
    ok = JS_TRUE;
    _av = argvFree(_av);
    _pat = _free(_pat);
    return ok;
}

static JSFunctionSpec rpmts_funcs[] = {
    JS_FS("add",	rpmts_add,		0,0),
    JS_FS("check",	rpmts_check,		0,0),
    JS_FS("order",	rpmts_order,		0,0),
    JS_FS("run",	rpmts_run,		0,0),
    JS_FS("mi",		rpmts_mi,		0,0),
    JS_FS("dbrebuild",	rpmts_dbrebuild,	0,0),
    JS_FS("dbkeys",	rpmts_dbkeys,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmts_tinyid {
    _DEBUG	= -2,
    _LENGTH	= -3,
    _VSFLAGS	= -4,
    _TYPE	= -5,
    _ARBGOAL	= -6,
    _ROOTDIR	= -7,
    _CURRDIR	= -8,
    _SELINUX	= -9,
    _CHROOTDONE	= -10,
    _TID	= -11,
    _NELEMENTS	= -12,
    _PROBFILTER	= -13,
    _FLAGS	= -14,
    _DFLAGS	= -15,
    _GOAL	= -16,
    _DBMODE	= -17,
    _COLOR	= -18,
    _PREFCOLOR	= -19,
};

static JSPropertySpec rpmts_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"length",	_LENGTH,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"vsflags",	_VSFLAGS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"type",	_TYPE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"arbgoal",	_ARBGOAL,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"rootdir",	_ROOTDIR,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"currdir",	_CURRDIR,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"selinux",	_SELINUX,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"chrootdone",_CHROOTDONE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"tid",	_TID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"nelements",_NELEMENTS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"probfilter",_PROBFILTER,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"flags",	_FLAGS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"dflags",	_DFLAGS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"goal",	_GOAL,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"dbmode",	_DBMODE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"color",	_COLOR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"prefcolor",_PREFCOLOR,	JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmts_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
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
	*vp = INT_TO_JSVAL(rpmtsNElements(ts));
	break;
    case _VSFLAGS:
	*vp = INT_TO_JSVAL((jsint)rpmtsVSFlags(ts));
	break;
    case _TYPE:
	*vp = INT_TO_JSVAL((jsint)rpmtsType(ts));
	break;
    case _ARBGOAL:
	*vp = INT_TO_JSVAL((jsint)rpmtsARBGoal(ts));
	break;
    case _ROOTDIR:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmtsRootDir(ts)));
	break;
    case _CURRDIR:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmtsCurrDir(ts)));
	break;
    case _SELINUX:
	*vp = INT_TO_JSVAL((jsint)rpmtsSELinuxEnabled(ts));
	break;
    case _CHROOTDONE:
	*vp = INT_TO_JSVAL((jsint)rpmtsChrootDone(ts));
	break;
    case _TID:
	*vp = INT_TO_JSVAL((jsint)rpmtsGetTid(ts));
	break;
    case _NELEMENTS:
	*vp = INT_TO_JSVAL((jsint)rpmtsNElements(ts));
	break;
    case _PROBFILTER:
	*vp = INT_TO_JSVAL((jsint)rpmtsFilterFlags(ts));
	break;
    case _FLAGS:
	*vp = INT_TO_JSVAL((jsint)rpmtsFlags(ts));
	break;
    case _DFLAGS:
	*vp = INT_TO_JSVAL((jsint)rpmtsDFlags(ts));
	break;
    case _GOAL:
	*vp = INT_TO_JSVAL((jsint)rpmtsGoal(ts));
	break;
    case _DBMODE:
	*vp = INT_TO_JSVAL((jsint)rpmtsDBMode(ts));
	break;
    case _COLOR:
	*vp = INT_TO_JSVAL((jsint)rpmtsColor(ts));
	break;
    case _PREFCOLOR:
	*vp = INT_TO_JSVAL((jsint)rpmtsPrefColor(ts));
	break;
    default:
	if (JSVAL_IS_INT(id)) {
	    int oc = JSVAL_TO_INT(id);
	    JSObject *teo = NULL;
	    rpmte te = NULL;
	    /* XXX rpmteLink/rpmteUnlink are no-ops */
	    if ((te = rpmtsElement(ts, oc)) != NULL
	     && (teo = JS_NewObject(cx, &rpmteClass, NULL, NULL)) != NULL
	     && JS_SetPrivate(cx, teo, rpmteLink(te)))
	    {
		*vp = OBJECT_TO_JSVAL(teo);
	    }
	    break;
	}
#ifdef	DYING
	if (JSVAL_IS_STRING(id)) {
	    JSString * str = JS_ValueToString(cx, id);
	    const char * name = JS_GetStringBytes(str);
	    if (!strcmp(name, "NVRA")) {
		JSObject * NVRA = rpmtsLoadNVRA(cx, obj);
		*vp = OBJECT_TO_JSVAL(NVRA);
	    }
	    break;
	}
#endif
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmts_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = (rpmts)ptr;
    jsint tiny = JSVAL_TO_INT(id);
    const char * s = NULL;
    int myint;

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
    case _VSFLAGS:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetVSFlags(ts, (rpmVSFlags)myint);
	break;
    case _TYPE:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetType(ts, (rpmTSType)myint);
	break;
    case _ARBGOAL:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetARBGoal(ts, (uint32_t)myint);
	break;
    case _ROOTDIR:
	s = JS_EncodeString(cx, JS_ValueToString(cx, *vp));
	(void) rpmtsSetRootDir(ts, s);
	s = _free(s);
	break;
    case _CURRDIR:
	s = JS_EncodeString(cx, JS_ValueToString(cx, *vp));
	(void) rpmtsSetCurrDir(ts, s);
	s = _free(s);
	break;
    case _CHROOTDONE:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetChrootDone(ts, myint);
	break;
    case _TID:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetTid(ts, (uint32_t)myint);
	break;
    case _FLAGS:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetFlags(ts, myint);
	break;
    case _DFLAGS:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetDFlags(ts, (rpmdepFlags)myint);
	break;
    case _GOAL:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetGoal(ts, (tsmStage)myint);
	break;
    case _DBMODE:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetDBMode(ts, myint);
	break;
    case _COLOR:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmtsSetColor(ts, (uint32_t)(myint & 0x0f));
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmts_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    JSBool ok = JS_FALSE;
    int oc;

_RESOLVE_DEBUG_ENTRY(_debug);

    if (flags & JSRESOLVE_ASSIGNING) {
	ok = JS_TRUE;
	goto exit;
    }
    if (JSVAL_IS_INT(id)
     && (oc = JSVAL_TO_INT(id)) >= 0 && oc < rpmtsNElements(ts))
    {
	JSObject *teo = NULL; 
	rpmte te = NULL;
	/* XXX rpmteLink/rpmteUnlink are no-ops */
	if ((te = rpmtsElement(ts, oc)) == NULL
	 || (teo = JS_NewObject(cx, &rpmteClass, NULL, NULL)) == NULL
	 || !JS_SetPrivate(cx, teo, rpmteLink(te))
         || !JS_DefineElement(cx, obj, oc, OBJECT_TO_JSVAL(teo),
                        NULL, NULL, JSPROP_ENUMERATE))
	{
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
rpmtsi_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsiClass, NULL);
    rpmtsi tsi = ptr;

if (_debug)
fprintf(stderr, "==> %s(%p,%p) ptr %p\n", __FUNCTION__, cx, obj, ptr);

    tsi = rpmtsiFree(tsi);
}

JSClass rpmtsiClass = {
    "Tsi",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,  JS_ConvertStub,  rpmtsi_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
rpmts_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;
    rpmtsi tsi;
    JSObject *tsio = NULL;
    JSBool ok = JS_FALSE;

_ENUMERATE_DEBUG_ENTRY(_debug);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	if ((tsio = JS_NewObject(cx, &rpmtsiClass, NULL, obj)) == NULL)
	    goto exit;
	if ((tsi = rpmtsiInit(ts)) == NULL)
	    goto exit;
	if (!JS_SetPrivate(cx, tsio, (void *)tsi)) {
	    tsi = rpmtsiFree(tsi);
	    goto exit;
	}
	*statep = OBJECT_TO_JSVAL(tsio);
	if (idp)
	    *idp = JSVAL_ZERO;
if (_debug)
fprintf(stderr, "\tINIT tsio %p tsi %p\n", tsio, tsi);
	break;
    case JSENUMERATE_NEXT:
	tsio = (JSObject *) JSVAL_TO_OBJECT(*statep);
	tsi = JS_GetInstancePrivate(cx, tsio, &rpmtsiClass, NULL);
	if (rpmtsiNext(tsi, 0) != NULL) {
	    int oc = rpmtsiOc(tsi);
if (_debug)
fprintf(stderr, "\tNEXT tsio %p tsi %p[%d]\n", tsio, tsi, oc);
	    JS_ValueToId(cx, INT_TO_JSVAL(oc), idp);
	} else
	    *idp = JSVAL_VOID;
	if (!JSID_IS_VOID(*idp))
	    break;
	/*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	tsio = (JSObject *) JSVAL_TO_OBJECT(*statep);
	tsi = JS_GetInstancePrivate(cx, tsio, &rpmtsiClass, NULL);
if (_debug)
fprintf(stderr, "\tFINI tsio %p tsi %p\n", tsio, tsi);
	/* Allow our iterator object to be GC'd. */
	*statep = JSVAL_NULL;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

/* --- Object ctors/dtors */
static rpmts
rpmts_init(JSContext *cx, JSObject *obj)
{
    rpmts ts;

    if ((ts = rpmtsCreate()) == NULL)
	return NULL;
    if (!JS_SetPrivate(cx, obj, (void *)ts)) {
	/* XXX error msg */
	(void) rpmtsFree(ts);
	return NULL;
    }
    return ts;
}

static void
rpmts_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtsClass, NULL);
    rpmts ts = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    (void) rpmtsFree(ts);
}

static JSBool
rpmts_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	if (rpmts_init(cx, obj) == NULL)
	    goto exit;
    } else {
	if ((obj = JS_NewObject(cx, &rpmtsClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmtsClass = {
    "Ts", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmts_addprop,   rpmts_delprop, rpmts_getprop, rpmts_setprop,
    (JSEnumerateOp)rpmts_enumerate, (JSResolveOp)rpmts_resolve,
    rpmts_convert,	rpmts_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitTsClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &rpmtsClass, rpmts_ctor, 1,
		rpmts_props, rpmts_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

JSObject *
rpmjs_NewTsObject(JSContext *cx)
{
    JSObject *obj;
    rpmts ts;

    if ((obj = JS_NewObject(cx, &rpmtsClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((ts = rpmts_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmts, Ts, JS_TRUE)
