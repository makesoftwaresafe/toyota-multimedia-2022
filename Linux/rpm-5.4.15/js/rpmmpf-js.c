/** \ingroup js_c
 * \file js/rpmmpf-js.c
 */

#include "system.h"

#include "rpmmpf-js.h"
#include "rpmtxn-js.h"
#include "rpmjs-debug.h"

#include <argv.h>

#include <db.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmmpf_addprop		JS_PropertyStub
#define	rpmmpf_delprop		JS_PropertyStub
#define	rpmmpf_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmmpf_getobjectops	NULL
#define	rpmmpf_checkaccess	NULL
#define	rpmmpf_call		rpmmpf_call
#define	rpmmpf_construct		rpmmpf_ctor
#define	rpmmpf_xdrobject		NULL
#define	rpmmpf_hasinstance	NULL
#define	rpmmpf_mark		NULL
#define	rpmmpf_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmmpf_equality		NULL
#define rpmmpf_outerobject	NULL
#define rpmmpf_innerobject	NULL
#define rpmmpf_iteratorobject	NULL
#define rpmmpf_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

#define	OBJ_IS_RPMTXN(_cx, _o)	(JS_GET_CLASS(_cx, _o) == &rpmtxnClass)

#define	_RPMMPF_PAGESIZE	(4 * BUFSIZ)

static JSBool
rpmmpf_Close(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);
    DB_MPOOLFILE * mpf = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (mpf == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    {	int ret = mpf->close(mpf, _flags);
	if (ret)
	    fprintf(stderr, "DB_MPOOLFILE->close: %s\n", db_strerror(ret));
	else
	    *vp = JSVAL_TRUE;
	mpf = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmmpf_Get(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);
    DB_MPOOLFILE * mpf = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (mpf == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &o, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);

    {	db_pgno_t _pgno = 0;
	void * _page = NULL;
	int ret = mpf->get(mpf, &_pgno, _txn, _flags, &_page);
	switch (ret) {
	default:
	    fprintf(stderr, "DB_MPOOLFILE->get: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmmpf_Open(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);
    DB_MPOOLFILE * mpf = ptr;
    char * _file = NULL;
    uint32_t _flags = DB_CREATE;
    int _mode = 0640;
    uint32_t _pagesize = _RPMMPF_PAGESIZE;

    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (mpf == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/suiu", &_file, &_flags, &_mode, &_pagesize)))
	goto exit;

    {	size_t pagesize = _pagesize;
	int ret = mpf->open(mpf, _file, _flags, _mode, pagesize);
	switch (ret) {
	default:
	    fprintf(stderr, "DB_MPOOLFILE->open: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmmpf_Put(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);
    DB_MPOOLFILE * mpf = ptr;
    DB_CACHE_PRIORITY _priority = DB_PRIORITY_UNCHANGED;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (mpf == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_priority)))
	goto exit;

    {	void * _pgaddr = NULL;
	uint32_t _flags = 0;
	int ret = mpf->put(mpf, _pgaddr, _priority, _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DB_MPOOLFILE->put: %s\n", db_strerror(ret));
	    *vp = JSVAL_FALSE;
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmmpf_Sync(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);
    DB_MPOOLFILE * mpf = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (mpf == NULL) goto exit;
    *vp = JSVAL_FALSE;

    {	int ret = mpf->sync(mpf);
	if (ret)
	    fprintf(stderr, "DB_MPOOLFILE->sync: %s\n", db_strerror(ret));
	*vp = (!ret ? JSVAL_TRUE : JSVAL_FALSE);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSFunctionSpec rpmmpf_funcs[] = {
    JS_FS("close",	rpmmpf_Close,		0,0),
    JS_FS("get",	rpmmpf_Get,		0,0),
    JS_FS("open",	rpmmpf_Open,		0,0),
    JS_FS("put",	rpmmpf_Put,		0,0),
    JS_FS("sync",	rpmmpf_Sync,		0,0),
    JS_FS_END
};

/* --- Object properties */

#define	_TABLE(_v)	#_v, _##_v, JSPROP_ENUMERATE, NULL, NULL

enum rpmmpf_tinyid {
    _DEBUG	= -2,
    _CLEARLEN	= -3,
    _FILEID	= -4,
    _FLAGS	= -5,
    _FTYPE	= -6,
    _LSNOFFSET	= -7,
    _MAXSIZE	= -8,
    _PGCOOKIE	= -9,
    _PRIORITY	= -10,
};

static JSPropertySpec rpmmpf_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"clear_len",_CLEARLEN,	JSPROP_ENUMERATE,	NULL,	NULL},
#ifdef	NOTNEEDED
    {"fileid",	_FILEID,	JSPROP_ENUMERATE,	NULL,	NULL},
#endif
    {"flags",	_FLAGS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"ftype",	_FTYPE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"lsn_offset",_LSNOFFSET,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"maxsize",	_MAXSIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"pgcookie",_PGCOOKIE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"priority",_PRIORITY,	JSPROP_ENUMERATE,	NULL,	NULL},

    {NULL, 0, 0, NULL, NULL}
};

#define	_RET_B(_bool)	((_bool) > 0 ? JSVAL_TRUE: JSVAL_FALSE)
#define	_RET_S(_str)	\
    ((_str) ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (_str))) : JSVAL_NULL)
#define	_GET_S(_test)	((_test) ? _RET_S(_s) : JSVAL_VOID)
#define	_GET_U(_test)	((_test) ? INT_TO_JSVAL(_u) : JSVAL_VOID)
#define	_GET_I(_test)	((_test) ? INT_TO_JSVAL(_i) : JSVAL_VOID)
#define	_GET_B(_test)	((_test) ? _RET_B(_i) : JSVAL_VOID)

static JSBool
rpmmpf_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);
    DB_MPOOLFILE * mpf = ptr;
    int32_t _i = 0;
    uint32_t _u = 0;
    uint32_t _gb = 0;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    case _CLEARLEN:	*vp = _GET_U(!mpf->get_clear_len(mpf, &_u)); break;
    case _FLAGS:	*vp = _GET_U(!mpf->get_flags(mpf, &_u)); break;
    case _FTYPE:	*vp = _GET_I(!mpf->get_ftype(mpf, &_i)); break;
    case _LSNOFFSET:	*vp = _GET_I(!mpf->get_lsn_offset(mpf, &_i)); break;
    case _MAXSIZE:	*vp = _GET_U(!mpf->get_maxsize(mpf, &_gb, &_u)); break;
    case _PGCOOKIE:	*vp = JSVAL_VOID; break;	/* XXX FIXME */
    case _PRIORITY:	*vp = _GET_U(!mpf->get_priority(mpf, (DB_CACHE_PRIORITY *)&_u)); break;

    default:
	break;
    }

    return JS_TRUE;
}

#define	_PUT_S(_put)	(JSVAL_IS_STRING(*vp) && !(_put) \
	? JSVAL_TRUE : JSVAL_FALSE)
#define	_PUT_U(_put)	(JSVAL_IS_NUMBER(*vp) && !(_put) \
	? JSVAL_TRUE : JSVAL_FALSE)
#define	_PUT_I(_put)	(JSVAL_IS_NUMBER(*vp) && !(_put) \
	? JSVAL_TRUE : JSVAL_FALSE)

static JSBool
rpmmpf_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);
    DB_MPOOLFILE * mpf = ptr;
    int32_t _i = 0;
    uint32_t _u = 0;
    uint32_t _gb = 0;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    if (JSVAL_IS_INT(*vp))
	_i = _u = JSVAL_TO_INT(*vp);

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
    case _CLEARLEN:	*vp = _PUT_U(!mpf->set_clear_len(mpf, _u)); break;
    case _FLAGS:	*vp = JSVAL_FALSE; break;	/* XXX FIXME */
    case _FTYPE:	*vp = _PUT_I(!mpf->set_ftype(mpf, _i)); break;
    case _LSNOFFSET:	*vp = _PUT_I(!mpf->set_lsn_offset(mpf, _i)); break;
    case _MAXSIZE:	*vp = _PUT_U(!mpf->set_maxsize(mpf, _gb, _u)); break;
    case _PGCOOKIE:	*vp = JSVAL_FALSE; break;	/* XXX FIXME */
    case _PRIORITY:	*vp = _PUT_U(mpf->set_priority(mpf, (DB_CACHE_PRIORITY)_u)); break;

    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmmpf_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);

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
rpmmpf_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static DB_MPOOLFILE *
rpmmpf_init(JSContext *cx, JSObject *obj)
{
    DB_MPOOLFILE * mpf = NULL;
#ifdef	NOTYET
    uint32_t _flags = 0;

    if (rpmmpf_env_create(&db, _flags) || mpf == NULL
     || !JS_SetPrivate(cx, obj, (void *)mpf))
#else
    if (!JS_SetPrivate(cx, obj, (void *)mpf))
#endif
    {
	if (mpf)
	    (void) mpf->close(mpf, 0);
	/* XXX error msg */
	mpf = NULL;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) mpf %p\n", __FUNCTION__, cx, obj, mpf);

    return mpf;
}

static void
rpmmpf_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmmpfClass, NULL);
#ifdef	NOTYET	/* XXX let BDB handle DB_MPOOLFILE ptrs */
    DB_MPOOLFILE * mpf = ptr;
#endif

_DTOR_DEBUG_ENTRY(_debug);

#ifdef	NOTYET	/* XXX let BDB handle DB_MPOOLFILE ptrs */
    if (mpf)
	(void) mpf->close(mpf, 0);
#endif
}

static JSBool
rpmmpf_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmmpf_init(cx, obj);
    } else {
	if ((obj = JS_NewObject(cx, &rpmmpfClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmmpf_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmmpfClass, NULL);
#ifdef	NOTYET
    DB_MPOOLFILE * mpf = ptr;
    const char *_fn = NULL;
    const char * _con = NULL;
#endif
    JSBool ok = JS_FALSE;

#ifdef	NOTYET
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_fn)))
        goto exit;

    *vp = (mpf && _fn && (_con = rpmmpfLgetfilecon(mpf, _fn)) != NULL)
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
JSClass rpmmpfClass = {
    "Mpf", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmmpf_addprop,   rpmmpf_delprop, rpmmpf_getprop, rpmmpf_setprop,
    (JSEnumerateOp)rpmmpf_enumerate, (JSResolveOp)rpmmpf_resolve,
    rpmmpf_convert,	rpmmpf_dtor,

    rpmmpf_getobjectops,	rpmmpf_checkaccess,
    rpmmpf_call,		rpmmpf_construct,
    rpmmpf_xdrobject,	rpmmpf_hasinstance,
    rpmmpf_mark,		rpmmpf_reserveslots,
};

JSObject *
rpmjs_InitMpfClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmmpfClass, rpmmpf_ctor, 1,
		rpmmpf_props, rpmmpf_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewMpfObject(JSContext *cx)
{
    JSObject *obj;
    DB_MPOOLFILE * mpf;

    if ((obj = JS_NewObject(cx, &rpmmpfClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((mpf = rpmmpf_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmmpf, Mpf, JS_TRUE)
