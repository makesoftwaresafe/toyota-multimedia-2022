/** \ingroup js_c
 * \file js/rpmdir-js.c
 */

#include "system.h"

#include "rpmdir-js.h"
#include "rpmjs-debug.h"
#include <rpmdir.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmdir_addprop		JS_PropertyStub
#define	rpmdir_delprop		JS_PropertyStub
#define	rpmdir_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmdir_getobjectops	NULL
#define	rpmdir_checkaccess	NULL
#define	rpmdir_call		rpmdir_call
#define	rpmdir_construct	rpmdir_ctor
#define	rpmdir_xdrobject	NULL
#define	rpmdir_hasinstance	NULL
#define	rpmdir_mark		NULL
#define	rpmdir_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmdir_equality		NULL
#define rpmdir_outerobject	NULL
#define rpmdir_innerobject	NULL
#define rpmdir_iteratorobject	NULL
#define rpmdir_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSFunctionSpec rpmdir_funcs[] = {
    JS_FS_END
};

/* --- Object properties */
enum rpmdir_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmdir_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmdir_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdirClass, NULL);
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
rpmdir_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdirClass, NULL);
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
rpmdir_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
		JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdirClass, NULL);

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
rpmdir_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdirClass, NULL);
    DIR * dir = ptr;
    struct dirent * dp;
    unsigned int ix = 0;

    /* XXX VG: JS_Enumerate (jsobj.c:4211) doesn't initialize some fields. */
_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	if (idp)
	    *idp = JSVAL_ZERO;
	*statep = INT_TO_JSVAL(ix);
if (_debug)
fprintf(stderr, "\tINIT dir %p\n", dir);
        break;
    case JSENUMERATE_NEXT:
	ix = JSVAL_TO_INT(*statep);
	if ((dp = Readdir(dir)) != NULL) {
	    (void) JS_DefineElement(cx, obj,
			ix, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dp->d_name)),
			NULL, NULL, JSPROP_ENUMERATE);
	    JS_ValueToId(cx, *statep, idp);
if (_debug)
fprintf(stderr, "\tNEXT dir %p[%u] dirent %p \"%s\"\n", dir, ix, dp, dp->d_name);
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
fprintf(stderr, "\tFINI dir %p[%u]\n", dir, ix);
	*statep = JSVAL_NULL;
        break;
    }
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static DIR *
rpmdir_init(JSContext *cx, JSObject *obj, const char * _dn)
{
    DIR * dir = NULL;

    if (_dn) {
	dir = Opendir(_dn);
	/* XXX error msg */
	if (!JS_SetPrivate(cx, obj, (void *)dir)) {
	    /* XXX error msg */
	    if (dir) {
		(void) Closedir(dir);
		/* XXX error msg */
	    }
	    dir = NULL;
	}
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p,\"%s\") dir %p\n", __FUNCTION__, cx, obj, _dn, dir);

    return dir;
}

static void
rpmdir_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdirClass, NULL);
    DIR * dir = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    if (dir)
	(void) Closedir(dir);
}

static JSBool
rpmdir_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    const char * _dn = NULL;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/s", &_dn)))
        goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmdir_init(cx, obj, _dn);
    } else {
	if ((obj = JS_NewObject(cx, &rpmdirClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdir_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmdirClass, NULL);
    DIR * dir = ptr;
    JSBool ok = JS_FALSE;
    const char * _dn = NULL;

if (_debug)
fprintf(stderr, "==> %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/s", &_dn)))
        goto exit;

    if (dir) {
	(void) Closedir(dir);
	/* XXX error msg */
	dir = ptr = NULL;
	(void) JS_SetPrivate(cx, o, (void *)dir);
    }

    dir = ptr = rpmdir_init(cx, o, _dn);

    *vp = OBJECT_TO_JSVAL(o);

    ok = JS_TRUE;

exit:
if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);

    return ok;
}

/* --- Class initialization */
JSClass rpmdirClass = {
    "Dir", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmdir_addprop,   rpmdir_delprop, rpmdir_getprop, rpmdir_setprop,
    (JSEnumerateOp)rpmdir_enumerate, (JSResolveOp)rpmdir_resolve,
    rpmdir_convert,	rpmdir_dtor,

    rpmdir_getobjectops,rpmdir_checkaccess,
    rpmdir_call,	rpmdir_construct,
    rpmdir_xdrobject,	rpmdir_hasinstance,
    rpmdir_mark,	rpmdir_reserveslots,
};

JSObject *
rpmjs_InitDirClass(JSContext *cx, JSObject* obj)
{
    JSObject * proto = JS_InitClass(cx, obj, NULL, &rpmdirClass, rpmdir_ctor, 1,
		rpmdir_props, rpmdir_funcs, NULL, NULL);

if (_debug)
fprintf(stderr, "<== %s(%p,%p) proto %p\n", __FUNCTION__, cx, obj, proto);

assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewDirObject(JSContext *cx, const char * _dn)
{
    JSObject *obj;
    DIR * dir;

    if ((obj = JS_NewObject(cx, &rpmdirClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((dir = rpmdir_init(cx, obj, _dn)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmdir, Dir, JS_TRUE)
