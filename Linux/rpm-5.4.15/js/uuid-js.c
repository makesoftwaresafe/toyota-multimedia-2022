/** \ingroup js_c
 * \file js/uuid-js.c
 */

#if defined(__APPLE__)
/* workaround for "uuid_t" type conflict, between <unistd.h> and "uuid.h" */
#define _UUID_T
#define uuid_t __darwin_uuid_t
#include <unistd.h>
#undef uuid_t
#undef _UUID_T
#endif

#include "system.h"

#include "rpmjs-debug.h"

#if defined(WITH_UUID)
#include <uuid.h>
#include "uuid-js.h"

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#define	uuid_addprop	JS_PropertyStub
#define	uuid_delprop	JS_PropertyStub
#define	uuid_convert	JS_ConvertStub

typedef struct uuid_s {
    const char * ns_str;
    uuid_t  * ns;
} * JSUuid;

/* --- helpers */

/* --- Object methods */

static JSBool
uuid_generate(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &uuidClass, NULL);
    int32 version = 0;
    const char *uuid_ns_str = NULL;
    const char *data = NULL;
    uuid_t *uuid = NULL;
    uuid_t *uuid_ns = NULL;
    uuid_rc_t rc;
    char buf[UUID_LEN_STR+1];
    char *b = buf;
    size_t blen = sizeof(buf);
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    ok = JS_ConvertArguments(cx, argc, argv, "i/ss",
		&version, &uuid_ns_str, &data);
    if (!ok)
	goto exit;

    switch (version) {
    default:	goto exit;	/*@notreached@*/ break;
    case 1:
    case 4:
	break;
    case 3:
    case 5:
	if (uuid_ns_str == NULL || data == NULL
	 || (rc = uuid_create(&uuid_ns)) != UUID_RC_OK)
	    goto exit;
        if ((rc = uuid_load(uuid_ns, uuid_ns_str)) != UUID_RC_OK
	 && (rc = uuid_import(uuid_ns, UUID_FMT_STR, uuid_ns_str, strlen(uuid_ns_str))) != UUID_RC_OK)
	    goto exit;
	break;
    }

    if ((rc = uuid_create(&uuid)) != UUID_RC_OK)
	goto exit;

    switch (version) {
    default:	goto exit;		/*@notreached@*/break;
    case 1:	rc = uuid_make(uuid, UUID_MAKE_V1);	break;
    case 3:	rc = uuid_make(uuid, UUID_MAKE_V3, uuid_ns, data);	break;
    case 4:	rc = uuid_make(uuid, UUID_MAKE_V4);	break;
    case 5:	rc = uuid_make(uuid, UUID_MAKE_V5, uuid_ns, data);	break;
    }
    if (rc != UUID_RC_OK)
	goto exit;

    if ((rc = uuid_export(uuid, UUID_FMT_STR, &b, &blen)) == UUID_RC_OK) {
	JSString *str;
	if ((str = JS_NewStringCopyZ(cx, b)) != NULL) {
	    *vp = STRING_TO_JSVAL(str);
	    ok = JS_TRUE;
	}
    }

exit:
    if (uuid != NULL)
	uuid_destroy(uuid);
    if (uuid_ns != NULL)
	uuid_destroy(uuid_ns);
    return ok;
}

static JSBool
uuid_describe(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &uuidClass, NULL);
    uuid_t *uuid = NULL;
    const char *uuid_str = NULL;
    uuid_rc_t rc;
    char *b = NULL;
    size_t blen = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    ok = JS_ConvertArguments(cx, argc, argv, "s", &uuid_str);
    if (!ok)
	goto exit;

    if ((rc = uuid_create(&uuid)) == UUID_RC_OK
     && (rc = uuid_import(uuid, UUID_FMT_STR, uuid_str, strlen(uuid_str))) == UUID_RC_OK
     && (rc = uuid_export(uuid, UUID_FMT_TXT, &b, &blen)) == UUID_RC_OK)
    {	JSString *str;
	if ((str = JS_NewStringCopyZ(cx, b)) != NULL) {
	    *vp = STRING_TO_JSVAL(str);
	    ok = JS_TRUE;
	}
    }

exit:
    b = _free(b);
    if (uuid != NULL)
	uuid_destroy(uuid);
    return ok;
}

static JSFunctionSpec uuid_funcs[] = {
    JS_FS("generate",	uuid_generate,		0,0),
    JS_FS("describe",	uuid_describe,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum uuid_tinyid {
    _DEBUG		= -2,
};

static JSPropertySpec uuid_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
uuid_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &uuidClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    if (JSVAL_IS_STRING(id)) {
	char * str = JS_EncodeString(cx, JSVAL_TO_STRING(id));
	const JSFunctionSpec *fsp;
	for (fsp = uuid_funcs; fsp->name != NULL; fsp++) {
	    if (strcmp(fsp->name, str))
		continue;
	    break;
	}
	str = _free(str);
	goto exit;
    }

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    default:
	break;
    }
exit:
    return JS_TRUE;
}

static JSBool
uuid_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &uuidClass, NULL);
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
uuid_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &uuidClass, NULL);

_RESOLVE_DEBUG_ENTRY(_debug);

    if ((flags & JSRESOLVE_ASSIGNING)
     || (ptr == NULL)) { /* don't resolve to parent prototypes objects. */
	*objp = NULL;
	goto exit;
    }

    *objp = obj;        /* XXX always resolve in this object. */

exit:
    return JS_TRUE;
}

static JSBool
uuid_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    JSObject *iterator;
    JSBool ok = JS_FALSE;

_ENUMERATE_DEBUG_ENTRY(_debug);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	if ((iterator = JS_NewPropertyIterator(cx, obj)) == NULL)
	    goto exit;
	*statep = OBJECT_TO_JSVAL(iterator);
	if (idp)
	    *idp = JSVAL_ZERO;
	break;
    case JSENUMERATE_NEXT:
	iterator = (JSObject *) JSVAL_TO_OBJECT(*statep);
	if (!JS_NextProperty(cx, iterator, idp))
	    goto exit;
	if (!JSID_IS_VOID(*idp))
	    break;
	/*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	/* Allow our iterator object to be GC'd. */
	*statep = JSVAL_NULL;
	break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

/* --- Object ctors/dtors */
static void
uuid_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &uuidClass, NULL);

_DTOR_DEBUG_ENTRY(_debug);

    ptr = _free(ptr);
}

static JSBool
uuid_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	JSUuid ptr = xcalloc(0, sizeof(*ptr));

	if (ptr == NULL || !JS_SetPrivate(cx, obj, ptr)) {
	    ptr = _free(ptr);
	    goto exit;
	}
    } else {
	if ((obj = JS_NewObject(cx, &uuidClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass uuidClass = {
    "Uuid", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    uuid_addprop,   uuid_delprop, uuid_getprop, uuid_setprop,
    (JSEnumerateOp)uuid_enumerate, (JSResolveOp)uuid_resolve,
    uuid_convert,	uuid_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitUuidClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &uuidClass, uuid_ctor, 1,
		uuid_props, uuid_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

GPSEE_MODULE_WRAP(uuid, Uuid, JS_TRUE)

#endif	/* WITH_UUID */
