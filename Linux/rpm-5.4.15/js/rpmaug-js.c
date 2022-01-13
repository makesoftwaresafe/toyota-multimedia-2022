/** \ingroup js_c
 * \file js/rpmaug-js.c
 */

#include "system.h"

#include "rpmaug-js.h"
#include "rpmjs-debug.h"

#include <rpmmacro.h>
#include <rpmaug.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	rpmaug_addprop	JS_PropertyStub
#define	rpmaug_delprop	JS_PropertyStub
#define	rpmaug_convert	JS_ConvertStub

#define	AUGEAS_META_TREE	"/augeas"
/*@unchecked@*/
static const char _defvar[] = AUGEAS_META_TREE "/version/defvar";

/* --- helpers */

/* --- Object methods */
/* XXX does aug_defnode() need binding? */
/* XXX unclear whether aug.defvar("foo", "bar") or aug.foo = "bar" is better */
static JSBool
rpmaug_defvar(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;
    const char * _name = NULL;
    const char * _expr = NULL;

_METHOD_DEBUG_ENTRY(_debug);

    /* XXX note optional EXPR. If EXPR is NULL, then NAME is deleted. */
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s/s", &_name, &_expr)))
        goto exit;

    switch (rpmaugDefvar(aug, _name, _expr)) {
    default:
    case 0:	/* failure (but success if EXPR was NULL?) */
    case 1:	/* success */
	/* XXX return NAME or EXPR on success?  or bool for success/failure? */
	/* XXX hmmm, bool and string mixed returns. */
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _name));
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmaug_get(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;
    const char * _path = NULL;
    const char * _value = NULL;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_path)))
        goto exit;

    switch (rpmaugGet(aug, _path, &_value)) {
    case 1:	/* found */
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _value));
#ifdef	NOTYET
	_value = _free(_value);
#endif
	break;
    default:
    case 0:	/* not found */
    case -1:	/* multiply defined */
	*vp = JSVAL_VOID;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmaug_set(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;
    const char * _path = NULL;
    const char * _value = NULL;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ss", &_path, &_value)))
        goto exit;

    switch (rpmaugSet(aug, _path, _value)) {
    case 0:	/* found */
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _value));
	break;
    default:
    case -1:	/* multiply defined */
	*vp = JSVAL_VOID;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmaug_insert(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;
    const char * _path = NULL;
    const char * _label = NULL;
    int _before;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ssi", &_path, &_label, &_before)))
        goto exit;

    switch (rpmaugInsert(aug, _path, _label, _before)) {
    case 0:	/* success */
	*vp = JSVAL_TRUE;
	break;
    default:
    case -1:	/* failure */
	*vp = JSVAL_FALSE;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmaug_rm(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;
    const char * _path = NULL;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_path)))
        goto exit;

    /* XXX rpmaug_rm() returns number of deleted nodes. */
    *vp = INT_TO_JSVAL(rpmaugRm(aug, _path));
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmaug_mv(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;
    const char * _src = NULL;
    const char * _dst = NULL;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ss", &_src, &_dst)))
        goto exit;

    switch (rpmaugMv(aug, _src, _dst)) {
    case 0:	/* success */
	*vp = JSVAL_TRUE;
	break;
    default:
    case -1:	/* failure */
	*vp = JSVAL_FALSE;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmaug_match(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;
    const char * _path = NULL;
    char ** _matches = NULL;
    int nmatches;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_path)))
        goto exit;

    nmatches = rpmaugMatch(aug, _path, &_matches);
    if (nmatches <= 0) {	/* not found */
	*vp = JSVAL_VOID;
	goto exit;
    } else {
	JSObject *o;
	jsval v;
	int i;
	*vp = OBJECT_TO_JSVAL(o=JS_NewArrayObject(cx, 0, NULL));
	for (i = 0; i < nmatches; i++) {
	    v = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _matches[i]));
	    ok = JS_SetElement(cx, o, i, &v);
	    _matches[i] = _free(_matches[i]);
	}
	_matches = _free(_matches);
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmaug_save(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    switch (rpmaugSave(aug)) {
    case 0:	/* success */
	*vp = JSVAL_TRUE;
	break;
    default:
    case -1:	/* failure */
	*vp = JSVAL_FALSE;
	break;
    }
    ok = JS_TRUE;
    return ok;
}

static JSBool
rpmaug_load(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    switch (rpmaugLoad(aug)) {
    case 0:	/* success */
	*vp = JSVAL_TRUE;
	break;
    default:
    case -1:	/* failure */
	*vp = JSVAL_FALSE;
	break;
    }
    ok = JS_TRUE;
    return ok;
}

/* XXX print is uselss method because of FILE * in Augeas API. */
static JSBool
rpmaug_print(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    JSBool ok = JS_FALSE;
    const char * _path = NULL;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_path)))
        goto exit;

    switch (rpmaugPrint(aug, NULL, _path)) {
    case 0:	/* success */
	*vp = JSVAL_TRUE;
	break;
    default:
    case -1:	/* failure */
	*vp = JSVAL_FALSE;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSFunctionSpec rpmaug_funcs[] = {
    JS_FS("defvar",	rpmaug_defvar,		0,0),
    JS_FS("get",	rpmaug_get,		0,0),
    JS_FS("set",	rpmaug_set,		0,0),
    JS_FS("insert",	rpmaug_insert,		0,0),
    JS_FS("rm",		rpmaug_rm,		0,0),
    JS_FS("remove",	rpmaug_rm,		0,0),
    JS_FS("mv",		rpmaug_mv,		0,0),
    JS_FS("move",	rpmaug_mv,		0,0),
    JS_FS("match",	rpmaug_match,		0,0),
    JS_FS("save",	rpmaug_save,		0,0),
    JS_FS("load",	rpmaug_load,		0,0),
    JS_FS("print",	rpmaug_print,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmaug_tinyid {
    _DEBUG	= -2,
    _LENGTH	= -3,
};

static JSPropertySpec rpmaug_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmaug_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    default:
        if (JSVAL_IS_STRING(id) && *vp == JSVAL_VOID) {
	    const char * name = JS_EncodeString(cx, JS_ValueToString(cx, id));
	    const char * _path = rpmGetPath(_defvar, "/", name, NULL);
	    const char * _value = NULL;
	    if (rpmaugGet(aug, _path, &_value) >= 0 && _value != NULL)
		*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _value));
	    _value = _free(_value);
	    _path = _free(_path);
	    name = _free(name);
            break;
        }
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmaug_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;
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
	/* XXX expr = undefined same as deleting? */
        if (JSVAL_IS_STRING(id)) {
            const char * name = JS_EncodeString(cx, JS_ValueToString(cx, id));
            const char * expr = JS_EncodeString(cx, JS_ValueToString(cx, *vp));
	    /* XXX should *setprop be permitted to delete NAME?!? */
	    /* XXX return is no. nodes in EXPR match. */
	    (void) rpmaugDefvar(aug, name, expr);
	    expr = _free(expr);
	    name = _free(name);
            break;
        }
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmaug_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;

_RESOLVE_DEBUG_ENTRY(_debug);

    if (ptr == NULL) {	/* don't resolve to parent prototypes objects. */
	*objp = NULL;
	goto exit;
    }

    /* Lazily resolve new strings, with duplication to Augeas defvar too. */
    if ((flags & JSRESOLVE_ASSIGNING) && JSVAL_IS_STRING(id)) {
        const char *name = JS_EncodeString(cx, JS_ValueToString(cx, id));
	const char * _path;
	const char * _value;
	int xx;
	JSFunctionSpec *fsp;

	/* XXX avoid "aug.print" method namess duped into defvar space? */
	for (fsp = rpmaug_funcs; fsp->name != NULL; fsp++) {
	    if (!strcmp(fsp->name, name)) {
		*objp = obj;	/* XXX always resolve in this object. */
		goto exit;
	    }
	}

	/* See if NAME exists in DEFVAR path. */
	_path = rpmGetPath(_defvar, "/", name, NULL);
	_value = NULL;

	switch (rpmaugGet(aug, _path, &_value)) {
	/* XXX For now, force all defvar's to be single valued strings. */
	case -1:/* multiply defined */
	case 0:	/* not found */
	    /* Create an empty Augeas defvar for NAME */
	    xx = rpmaugDefvar(aug, name, "");
	    /*@fallthrough@*/
	case 1:	/* found */
	    /* Reflect the Augeas defvar NAME into JS Aug properties. */
	    if (JS_DefineProperty(cx, obj, name,
			(_value != NULL
			    ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _value))
			    : JSVAL_VOID),
                        NULL, NULL, JSPROP_ENUMERATE))
		break;
	    /*@fallthrough@*/
	default:
assert(0);
	    break;
	}

	_path = _free(_path);
	_value = _free(_value);
	name = _free(name);
    }
    *objp = obj;	/* XXX always resolve in this object. */

exit:
    return JS_TRUE;
}

static JSBool
rpmaug_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static rpmaug
rpmaug_init(JSContext *cx, JSObject *obj, const char *_root,
		const char *_loadpath, unsigned int _flags)
{
    rpmaug aug;

    if ((aug = rpmaugNew(_root, _loadpath, _flags)) == NULL)
	return NULL;
    if (!JS_SetPrivate(cx, obj, aug)) {
	/* XXX error msg */
	(void) rpmaugFree(aug);
	return NULL;
    }
    return aug;
}

static void
rpmaug_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmaugClass, NULL);
    rpmaug aug = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    aug = rpmaugFree(aug);
}

static JSBool
rpmaug_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    const char * _root = _rpmaugRoot;
    const char * _loadpath = _rpmaugLoadpath;
    unsigned int _flags = _rpmaugFlags;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/ssu", &_root, &_loadpath, &_flags)))
	goto exit;

    if (JS_IsConstructing(cx, vp)) {
	if (rpmaug_init(cx, obj, _root, _loadpath, _flags) == NULL)
	    goto exit;
    } else {
	if ((obj = JS_NewObject(cx, &rpmaugClass, NULL, NULL)) == NULL)
	    goto exit;
        JS_SET_RVAL( cx , vp , OBJECT_TO_JSVAL( obj ) );
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmaugClass = {
    /* XXX class should be "Augeas" eventually, avoid name conflicts for now */
    "Aug", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmaug_addprop,   rpmaug_delprop, rpmaug_getprop, rpmaug_setprop,
    (JSEnumerateOp)rpmaug_enumerate, (JSResolveOp)rpmaug_resolve,
    rpmaug_convert,	rpmaug_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitAugClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &rpmaugClass, rpmaug_ctor, 1,
		rpmaug_props, rpmaug_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

JSObject *
rpmjs_NewAugObject(JSContext *cx, const char * _root,
		const char * _loadpath, unsigned int _flags)
{
    JSObject *obj;
    rpmaug aug;
    if( ! obj ) {
	JS_ReportError( cx , "Failed to create 'this' object" );
	return JS_FALSE;
    }

    if ((obj = JS_NewObject(cx, &rpmaugClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((aug = rpmaug_init(cx, obj, _root, _loadpath, _flags)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmaug, Aug, JS_TRUE)
