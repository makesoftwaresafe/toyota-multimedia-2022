/** \ingroup js_c
 * \file js/rpmmc-js.c
 */

#include "system.h"

#include "rpmmc-js.h"
#include "rpmjs-debug.h"

#define	_MACRO_INTERNAL
#include <rpmmacro.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	rpmmc_addprop	JS_PropertyStub
#define	rpmmc_delprop	JS_PropertyStub
#define	rpmmc_convert	JS_ConvertStub

typedef	MacroContext rpmmc;

/* --- helpers */

/* --- Object methods */
static JSBool
rpmmc_add(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmcClass, NULL);
    rpmmc mc = ptr;
    char * s = NULL;
    int lvl = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &s)))
        goto exit;

    (void) rpmDefineMacro(mc, s, lvl);
    ok = JS_TRUE;
exit:
    *vp = BOOLEAN_TO_JSVAL(ok);
    return ok;
}

static JSBool
rpmmc_del(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmcClass, NULL);
    rpmmc mc = ptr;
    char * s = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &s)))
        goto exit;

    (void) rpmUndefineMacro(mc, s);
    ok = JS_TRUE;
exit:
    *vp = BOOLEAN_TO_JSVAL(ok);
    return ok;
}

static JSBool
rpmmc_list(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmcClass, NULL);
    rpmmc mc = ptr;
    void * _mire = NULL;
    int used = -1;
    const char ** av = NULL;
    int ac = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

/*@-globs@*/
    ac = rpmGetMacroEntries(mc, _mire, used, &av);
/*@=globs@*/
    if (ac > 0 && av != NULL && av[0] != NULL) {
	jsval *vec = xmalloc(ac * sizeof(*vec));
	JSString *valstr;
	int i;
	for (i = 0; i < ac; i++) {
	    /* XXX lua splits into {name,opts,body} triple. */
	    if ((valstr = JS_NewStringCopyZ(cx, av[i])) == NULL)
		goto exit;
	    vec[i] = STRING_TO_JSVAL(valstr);
	}
	*vp = OBJECT_TO_JSVAL(JS_NewArrayObject(cx, ac, vec));
	vec = _free(vec);
    } else
	*vp = JSVAL_NULL;	/* XXX JSVAL_VOID? */

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmmc_expand(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmcClass, NULL);
    rpmmc mc = ptr;
    char * s;
    char * t;
    JSString *valstr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &s)))
        goto exit;

    t = rpmMCExpand(mc, s, NULL);

    if ((valstr = JS_NewStringCopyZ(cx, t)) == NULL)
	goto exit;
    t = _free(t);
    *vp = STRING_TO_JSVAL(valstr);

    ok = JS_TRUE;
exit:
    return ok;
}

static JSFunctionSpec rpmmc_funcs[] = {
    JS_FS("add",	rpmmc_add,		0,0),
    JS_FS("del",	rpmmc_del,		0,0),
    JS_FS("list",	rpmmc_list,		0,0),
    JS_FS("expand",	rpmmc_expand,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmmc_tinyid {
    _DEBUG	= -2,
};

static JSPropertySpec rpmmc_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmmc_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmcClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmmc_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmcClass, NULL);
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
rpmmc_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmcClass, NULL);

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
rpmmc_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static rpmmc
rpmmc_init(JSContext *cx, JSObject *obj, jsval v)
{
    rpmmc mc = NULL;	/* XXX FIXME: only global context for now. */
    JSObject * o = (JSVAL_IS_OBJECT(v) ? JSVAL_TO_OBJECT(v) : NULL);

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%p) mc %p\n", __FUNCTION__, cx, obj, o, mc);

    if (JSVAL_IS_STRING(v)) {
	const char * s = JS_EncodeString(cx, JS_ValueToString(cx, v));
        if (!strcmp(s, "global"))
            mc = rpmGlobalMacroContext;
	else if (!strcmp(s, "cli"))
            mc = rpmCLIMacroContext;
	else {
	    mc = xcalloc(1, sizeof(*mc));
	    if (s && *s)
		rpmInitMacros(mc, s);
	}
if (_debug)
fprintf(stderr, "\tinitMacros(\"%s\") mc %p\n", s, mc);
	s = _free(s);
    } else
    if (o == NULL) {
if (_debug)
fprintf(stderr, "\tinitMacros() mc %p\n", mc);
    }

    if (!JS_SetPrivate(cx, obj, (void *)mc)) {
	/* XXX error msg */
	return NULL;
    }
    return mc;
}

static void
rpmmc_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmcClass, NULL);
    rpmmc mc = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    if (!(mc == rpmGlobalMacroContext || mc == rpmCLIMacroContext)) {
	rpmFreeMacros(mc);
	mc = _free(mc);
    }
}

static JSBool
rpmmc_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    jsval v = JSVAL_VOID;
    JSObject *o = NULL;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/v", &v)))
        goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmmc_init(cx, obj, v);
    } else {
	if ((obj = JS_NewObject(cx, &rpmmcClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmmcClass = {
    "Mc", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmmc_addprop,   rpmmc_delprop, rpmmc_getprop, rpmmc_setprop,
    (JSEnumerateOp)rpmmc_enumerate, (JSResolveOp)rpmmc_resolve,
    rpmmc_convert,	rpmmc_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitMcClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmmcClass, rpmmc_ctor, 1,
		rpmmc_props, rpmmc_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewMcObject(JSContext *cx, jsval v)
{
    JSObject *obj;
    rpmmc mc;

    if ((obj = JS_NewObject(cx, &rpmmcClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((mc = rpmmc_init(cx, obj, v)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmmc, Mc, JS_TRUE)
