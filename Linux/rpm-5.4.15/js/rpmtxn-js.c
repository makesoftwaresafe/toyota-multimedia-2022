/** \ingroup js_c
 * \file js/rpmtxn-js.c
 */

#include "system.h"

#include "rpmtxn-js.h"
#include "rpmjs-debug.h"

#include <argv.h>

#include <db.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmtxn_addprop		JS_PropertyStub
#define	rpmtxn_delprop		JS_PropertyStub
#define	rpmtxn_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmtxn_getobjectops	NULL
#define	rpmtxn_checkaccess	NULL
#define	rpmtxn_call		rpmtxn_call
#define	rpmtxn_construct		rpmtxn_ctor
#define	rpmtxn_xdrobject		NULL
#define	rpmtxn_hasinstance	NULL
#define	rpmtxn_mark		NULL
#define	rpmtxn_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmtxn_equality		NULL
#define rpmtxn_outerobject	NULL
#define rpmtxn_innerobject	NULL
#define rpmtxn_iteratorobject	NULL
#define rpmtxn_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSBool
rpmtxn_Abort(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtxnClass, NULL);
    DB_TXN * txn = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (txn == NULL) goto exit;
    *vp = JSVAL_FALSE;

    {	int ret = txn->abort(txn);
        if (ret)
	    fprintf(stderr, "DB_TXN->abort: %s", db_strerror(ret));
        else
            *vp = JSVAL_TRUE;
	txn = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmtxn_Commit(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtxnClass, NULL);
    DB_TXN * txn = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (txn == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    {	int ret = txn->commit(txn, _flags);
        if (ret)
	    fprintf(stderr, "DB_TXN->commit: %s", db_strerror(ret));
        else
            *vp = JSVAL_TRUE;
	txn = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmtxn_Discard(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtxnClass, NULL);
    DB_TXN * txn = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (txn == NULL) goto exit;
    *vp = JSVAL_FALSE;

    {	int ret = txn->discard(txn, _flags);
        if (ret)
	    fprintf(stderr, "DB_TXN->discard: %s", db_strerror(ret));
        else
            *vp = JSVAL_TRUE;
	txn = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmtxn_Prepare(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtxnClass, NULL);
    DB_TXN * txn = ptr;
#if !defined(DB_GID_SIZE)
#define	DB_GID_SIZE	128
#endif
    uint8_t _gid[DB_GID_SIZE] = {0};
    const char * _s = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (txn == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_s)))
	goto exit;
	/* XXX todo: hex convert for _gid string? */
    (void) strncpy((char *)_gid, _s, sizeof(_gid));
    
    {	int ret = txn->prepare(txn, _gid);
        if (ret)
	    fprintf(stderr, "DB_TXN->prepare: %s", db_strerror(ret));
        else
            *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSFunctionSpec rpmtxn_funcs[] = {
    JS_FS("abort",	rpmtxn_Abort,		0,0),
    JS_FS("commit",	rpmtxn_Commit,		0,0),
    JS_FS("discard",	rpmtxn_Discard,		0,0),
    JS_FS("prepare",	rpmtxn_Prepare,		0,0),
    JS_FS_END
};

/* --- Object properties */

#define	_TABLE(_v)	#_v, _##_v, JSPROP_ENUMERATE, NULL, NULL

enum rpmtxn_tinyid {
    _DEBUG			= -2,
    _NAME			= -3,
    _ID				= -4,
    _DB_SET_LOCK_TIMEOUT	= -5,
    _DB_SET_TXN_TIMEOUT		= -6,
};

static JSPropertySpec rpmtxn_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"name",	_NAME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"id",	_ID,		JSPROP_ENUMERATE,	NULL,	NULL},
    { _TABLE(DB_SET_LOCK_TIMEOUT) },
    { _TABLE(DB_SET_TXN_TIMEOUT) },

    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmtxn_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtxnClass, NULL);
    DB_TXN * txn = ptr;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_NewNumberValue(cx, (jsdouble)_debug, vp))
	    *vp = JSVAL_VOID;
	break;
    case _NAME:
    {	const char * _s = NULL;
	int ret = txn->get_name(txn, &_s);
	*vp = !ret ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _s)) : JSVAL_VOID;
    }   break;
    case _ID:
    {	uint32_t _u = txn->id(txn);
	if (!JS_NewNumberValue(cx, (jsdouble)_u, vp))
            *vp = JSVAL_VOID;
    }   break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmtxn_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtxnClass, NULL);
    DB_TXN * txn = ptr;
    jsint tiny = JSVAL_TO_INT(id);
    uint32_t _flags = 0;

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    *vp = JSVAL_VOID;
	break;
    case _NAME:
    {	const char * _s = JS_EncodeString(cx, JS_ValueToString(cx, *vp));
	*vp = !txn->set_name(txn, _s) ? JSVAL_TRUE : JSVAL_FALSE;
	_s = _free(_s);
    }	break;
#define	_JUMP(_v, _lbl)	_##_v:	_flags = _v;	goto _lbl
    case _JUMP(DB_SET_LOCK_TIMEOUT,		_set_timeout);
    case _JUMP(DB_SET_TXN_TIMEOUT,		_set_timeout);
#undef	_JUMP
    _set_timeout:
    {	uint32_t _u = 0;
        if (JS_ValueToECMAUint32(cx, *vp, &_u)) {
	    db_timeout_t _timeout = _u;
	    *vp = !txn->set_timeout(txn, _timeout, _flags)
			? JSVAL_TRUE : JSVAL_FALSE;
	} else
	    *vp = JSVAL_VOID;
    }	break;

    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmtxn_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtxnClass, NULL);

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
rpmtxn_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static DB_TXN *
rpmtxn_init(JSContext *cx, JSObject *obj)
{
    DB_TXN * txn = NULL;
#ifdef	NOTYET
    uint32_t _flags = 0;

    if (rpmtxn_env_create(&db, _flags) || txn == NULL
     || !JS_SetPrivate(cx, obj, (void *)txn))
#else
    if (!JS_SetPrivate(cx, obj, (void *)txn))
#endif
    {
#ifdef	NOTYET
	if (txn)
	    (void) txn->close(txn, _flags);
#endif

	/* XXX error msg */
	txn = NULL;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) txn %p\n", __FUNCTION__, cx, obj, txn);

    return txn;
}

static void
rpmtxn_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmtxnClass, NULL);
#ifdef	NOTYET
    DB_TXN * txn = ptr;
    uint32_t _flags = 0;
#endif

_DTOR_DEBUG_ENTRY(_debug);

#ifdef	NOTYET
    if (db) {
	(void) db->close(db, _flags);
    }
#endif
}

static JSBool
rpmtxn_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmtxn_init(cx, obj);
    } else {
	if ((obj = JS_NewObject(cx, &rpmtxnClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmtxn_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
#ifdef	NOTYET
    DB_TXN * txn = ptr;
    const char *_fn = NULL;
    const char * _con = NULL;
#endif
    JSBool ok = JS_FALSE;

#ifdef	NOTYET
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_fn)))
        goto exit;

    *vp = (txn && _fn && (_con = rpmtxnLgetfilecon(txn, _fn)) != NULL)
	? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _con)) : JSVAL_VOID;
    _con = _free(_con);

    ok = JS_TRUE;

exit:
#endif

if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);

    return ok;
}

/* --- Class initialization */
JSClass rpmtxnClass = {
    "Txn", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmtxn_addprop,   rpmtxn_delprop, rpmtxn_getprop, rpmtxn_setprop,
    (JSEnumerateOp)rpmtxn_enumerate, (JSResolveOp)rpmtxn_resolve,
    rpmtxn_convert,	rpmtxn_dtor,

    rpmtxn_getobjectops,	rpmtxn_checkaccess,
    rpmtxn_call,		rpmtxn_construct,
    rpmtxn_xdrobject,	rpmtxn_hasinstance,
    rpmtxn_mark,		rpmtxn_reserveslots,
};

JSObject *
rpmjs_InitTxnClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmtxnClass, rpmtxn_ctor, 1,
		rpmtxn_props, rpmtxn_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewTxnObject(JSContext *cx)
{
    JSObject *obj;
    DB_TXN * txn;

    if ((obj = JS_NewObject(cx, &rpmtxnClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((txn = rpmtxn_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmtxn, Txn, JS_TRUE)
