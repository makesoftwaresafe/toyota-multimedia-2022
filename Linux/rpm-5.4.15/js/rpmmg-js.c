/** \ingroup js_c
 * \file js/rpmmg-js.c
 */

#include "system.h"

#include "rpmmg-js.h"
#include "rpmjs-debug.h"

#include <rpmmg.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmmg_addprop		JS_PropertyStub
#define	rpmmg_delprop		JS_PropertyStub
#define	rpmmg_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmmg_getobjectops	NULL
#define	rpmmg_checkaccess	NULL
#define	rpmmg_call		rpmmg_call
#define	rpmmg_construct	rpmmg_ctor
#define	rpmmg_xdrobject	NULL
#define	rpmmg_hasinstance	NULL
#define	rpmmg_mark		NULL
#define	rpmmg_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmmg_equality		NULL
#define rpmmg_outerobject	NULL
#define rpmmg_innerobject	NULL
#define rpmmg_iteratorobject	NULL
#define rpmmg_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSFunctionSpec rpmmg_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmmg_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmmg_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmmg_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmgClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:	*vp = INT_TO_JSVAL(_debug);		break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmmg_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmgClass, NULL);
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
rpmmg_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
		JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmgClass, NULL);

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
rpmmg_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
#ifdef	NOTYET
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmgClass, NULL);
    rpmmg mg = ptr;
    struct dirent * dp;
    unsigned int ix = 0;
#endif

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

#ifdef	NOTYET
    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
        if (idp)
            *idp = JSVAL_ZERO;
	*statep = INT_TO_JSVAL(ix);
if (_debug)
fprintf(stderr, "\tINIT mg %p\n", mg);
        break;
    case JSENUMERATE_NEXT:
	ix = JSVAL_TO_INT(*statep);
	if ((dp = Readdir(dir)) != NULL) {
	    (void) JS_DefineElement(cx, obj,
			ix, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dp->d_name)),
			NULL, NULL, JSPROP_ENUMERATE);
	    JS_ValueToId(cx, *statep, idp);
if (_debug)
fprintf(stderr, "\tNEXT mg %p[%u] dirent %p \"%s\"\n", mg, ix, dp, dp->d_name);
	    *statep = INT_TO_JSVAL(ix+1);
	} else
	    *idp = JSVAL_VOID;
	if (!JSID_IS_VOID(*idp))
            break;
        /*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	ix = JSVAL_TO_INT(*statep);
	(void) JS_DefineProperty(cx, obj, "length", INT_TO_JSVAL(ix),
			NULL, NULL, JSPROP_ENUMERATE);
if (_debug)
fprintf(stderr, "\tFINI mg %p[%u]\n", mg, ix);
	*statep = JSVAL_NULL;
        break;
    }
#else
    *statep = JSVAL_NULL;
#endif
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static rpmmg
rpmmg_init(JSContext *cx, JSObject *obj, const char * _magicfile, int _flags)
{
    rpmmg mg = rpmmgNew(_magicfile, _flags);

    /* XXX error msg */
    if (!JS_SetPrivate(cx, obj, (void *)mg)) {
	/* XXX error msg */
	if (mg)
	    mg = rpmmgFree(mg);
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p,\"%s\",0x%x) mg %p\n", __FUNCTION__, cx, obj, _magicfile, _flags, mg);

    return mg;
}

static void
rpmmg_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmgClass, NULL);
    rpmmg mg = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    (void) rpmmgFree(mg);
}

static JSBool
rpmmg_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    const char * _magicfile = NULL;
    int _flags = 0;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/su", &_magicfile, &_flags)))
        goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmmg_init(cx, obj, _magicfile, _flags);
    } else {
	if ((obj = JS_NewObject(cx, &rpmmgClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmmg_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmmgClass, NULL);
    rpmmg mg = ptr;
    JSBool ok = JS_FALSE;
    const char * _fn = NULL;
    const char * s = NULL;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/s", &_fn)))
        goto exit;

    if (mg)
	s = rpmmgFile(mg, _fn);
    *vp = (s ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, s)) : JSVAL_NULL);

    ok = JS_TRUE;

exit:
if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, mg);
    s = _free(s);
    return ok;
}

/* --- Class initialization */
JSClass rpmmgClass = {
    /* XXX class should be "Magic" eventually, avoid name conflicts for now */
    "Mg", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmmg_addprop,   rpmmg_delprop, rpmmg_getprop, rpmmg_setprop,
    (JSEnumerateOp)rpmmg_enumerate, (JSResolveOp)rpmmg_resolve,
    rpmmg_convert,	rpmmg_dtor,

    rpmmg_getobjectops,	rpmmg_checkaccess,
    rpmmg_call,		rpmmg_construct,
    rpmmg_xdrobject,	rpmmg_hasinstance,
    rpmmg_mark,		rpmmg_reserveslots,
};

JSObject *
rpmjs_InitMgClass(JSContext *cx, JSObject* obj)
{
    JSObject * proto = JS_InitClass(cx, obj, NULL, &rpmmgClass, rpmmg_ctor, 1,
		rpmmg_props, rpmmg_funcs, NULL, NULL);

if (_debug)
fprintf(stderr, "<== %s(%p,%p) proto %p\n", __FUNCTION__, cx, obj, proto);

assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewMgObject(JSContext *cx, const char * _magicfile, int _flags)
{
    JSObject *obj;
    rpmmg mg;

    if ((obj = JS_NewObject(cx, &rpmmgClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((mg = rpmmg_init(cx, obj, _magicfile, _flags)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmmg, Mg, JS_TRUE)
