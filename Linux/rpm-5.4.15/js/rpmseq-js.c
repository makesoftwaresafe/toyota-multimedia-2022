/** \ingroup js_c
 * \file js/rpmseq-js.c
 */

#include "system.h"

#define	_RPMDB_JS_INTERNAL
#include "rpmdb-js.h"
#include "rpmseq-js.h"
#include "rpmtxn-js.h"
#include "rpmjs-debug.h"

#include <argv.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmseq_addprop		JS_PropertyStub
#define	rpmseq_delprop		JS_PropertyStub
#define	rpmseq_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmseq_getobjectops	NULL
#define	rpmseq_checkaccess	NULL
#define	rpmseq_call		rpmseq_call
#define	rpmseq_construct		rpmseq_ctor
#define	rpmseq_xdrobject		NULL
#define	rpmseq_hasinstance	NULL
#define	rpmseq_mark		NULL
#define	rpmseq_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmseq_equality		NULL
#define rpmseq_outerobject	NULL
#define rpmseq_innerobject	NULL
#define rpmseq_iteratorobject	NULL
#define rpmseq_wrappedobject	NULL

/* --- helpers */

/* --- Object methods */

static JSBool
rpmseq_Close(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (seq == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    {	int ret = seq->close(seq, _flags);
	if (ret)
	    fprintf(stderr, "DB_SEQUENCE->close: %s\n", db_strerror(ret));
	else
	    *vp = JSVAL_TRUE;
	seq = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmseq_Get(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    int32_t _delta = 1;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (seq == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/iu", &o, &_delta, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
    if (_delta <= 0)
	goto exit;

    {	jsdouble d = 0;
	db_seq_t _seqno = 0;
	int ret = seq->get(seq, _txn, _delta, &_seqno, _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DB_SEQUENCE->get: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    d = _seqno;
	    if (!JS_NewNumberValue(cx, d, vp))
		*vp = JSVAL_FALSE;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmseq_Open(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    uint32_t _flags = DB_CREATE;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (seq == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ov/u", &o, &_kv, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;

    {	int ret = seq->open(seq, _txn, _RPMDBT_PTR(_k), _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DB_SEQUENCE->open: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    seq->api_internal = obj;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmseq_Remove(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (seq == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &o, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);

    {	int ret = seq->remove(seq, _txn, _flags);
	switch (ret) {
	default:
	    fprintf(stderr, "DB_SEQUENCE->remove: %s\n", db_strerror(ret));
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	}
	seq = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmseq_Stat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (seq == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    {	DB_SEQUENCE_STAT * sp = NULL;
	int ret = seq->stat(seq, &sp, _flags);
	if (ret)
	    fprintf(stderr, "DB_SEQUENCE->stat: %s\n", db_strerror(ret));
	else
	    *vp = JSVAL_TRUE;
	sp = _free(sp);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmseq_StatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (seq == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    {	int ret = seq->stat_print(seq, _flags);
	if (ret)
	    fprintf(stderr, "DB_SEQUENCE->stat_print: %s\n", db_strerror(ret));
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSFunctionSpec rpmseq_funcs[] = {
    JS_FS("close",		rpmseq_Close,		0,0),
    JS_FS("get",		rpmseq_Get,		0,0),
    JS_FS("open",		rpmseq_Open,		0,0),
    JS_FS("remove",		rpmseq_Remove,		0,0),
    JS_FS("stat",		rpmseq_Stat,		0,0),
    JS_FS("stat_print",		rpmseq_StatPrint,	0,0),
    JS_FS_END
};

/* --- Object properties */

#define	_TABLE(_v)	#_v, _##_v, JSPROP_ENUMERATE, NULL, NULL

enum rpmseq_tinyid {
    _DEBUG	= -2,
    _DB		= -3,
    _KEY	= -4,
    _CACHESIZE	= -5,
    _FLAGS	= -6,
    _INITVAL	= -7,
    _RANGEMIN	= -8,
    _RANGEMAX	= -9,

    _ST_WAIT		= -10,
    _ST_NOWAIT		= -11,
    _ST_CURRENT		= -12,
    _ST_VALUE		= -13,
    _ST_LAST_VALUE	= -14,
    _ST_MIN		= -15,
    _ST_MAX		= -16,
    _ST_CACHESIZE	= -17,
    _ST_FLAGS		= -18,
};

static JSPropertySpec rpmseq_props[] = {
    {"debug",		_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"db",		_DB,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"key",		_KEY,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"cachesize",	_CACHESIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"flags",		_FLAGS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"initval",		_INITVAL,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"rangemin",	_RANGEMIN,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"rangemax",	_RANGEMAX,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_wait",		_ST_WAIT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_nowait",	_ST_NOWAIT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_current",	_ST_CURRENT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_value",	_ST_VALUE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_last_value",	_ST_LAST_VALUE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_min",		_ST_MIN,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_max",		_ST_MAX,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_cachesize",	_ST_CACHESIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_flags",	_ST_FLAGS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmseq_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;
    jsint tiny = JSVAL_TO_INT(id);
    jsdouble d = 0;
    int ret;

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;
    case _DB:
    {	DB * _db = NULL;
	if ((ret = seq->get_db(seq, &_db)) != 0) {
	    *vp = JSVAL_VOID;
	    break;
	}
	*vp = _db->app_private ? OBJECT_TO_JSVAL(_db->app_private) : JSVAL_NULL;
    }	break;
    case _KEY:
    {	DBT _k = {0};
	if ((ret = seq->get_key(seq, &_k)) != 0) {
	    *vp = JSVAL_VOID;
	    break;
	}
	*vp = _k.data
		? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _k.data)) : JSVAL_NULL;
    }	break;
    case _CACHESIZE:
    {	int32_t _i = 0;
	if ((ret = seq->get_cachesize(seq, &_i)) != 0) {
	    *vp = JSVAL_VOID;
	    break;
	}
	d = _i;
	if (!JS_NewNumberValue(cx, d, vp))
	    *vp = JSVAL_FALSE;
    }	break;
    case _FLAGS:
    {	uint32_t _u = 0;
	if ((ret = seq->get_flags(seq, &_u)) != 0) {
	    *vp = JSVAL_VOID;
	    break;
	}
	d = _u;
	if (!JS_NewNumberValue(cx, d, vp))
	    *vp = JSVAL_FALSE;
    }	break;
    case _RANGEMIN:
    case _RANGEMAX:
    {	db_seq_t _min = 0;
	db_seq_t _max = 0;

	if ((ret = seq->get_range(seq, &_min, &_max)) != 0) {
	    *vp = JSVAL_VOID;
	    break;
	}
	switch (tiny) {
	case _RANGEMIN:	d = _min;	break;
	case _RANGEMAX:	d = _max;	break;
	}
	if (!JS_NewNumberValue(cx, d, vp))
	    *vp = JSVAL_FALSE;
    }	break;
    case _ST_WAIT:
    case _ST_NOWAIT:
    case _ST_CURRENT:
    case _ST_VALUE:
    case _ST_LAST_VALUE:
    case _ST_MIN:
    case _ST_MAX:
    case _ST_CACHESIZE:
    case _ST_FLAGS:
    {	DB_SEQUENCE_STAT * sp = NULL;
	uint32_t _flags = 0;

	if ((ret = seq->stat(seq, &sp, _flags)) != 0) {
	    *vp = JSVAL_VOID;
	    break;
	}
	switch (tiny) {
	case _ST_WAIT:		d = sp->st_wait;	break;
	case _ST_NOWAIT:	d = sp->st_nowait;	break;
	case _ST_CURRENT:	d = sp->st_current;	break;
	case _ST_VALUE:		d = sp->st_value;	break;
	case _ST_LAST_VALUE:	d = sp->st_last_value;	break;
	case _ST_MIN:		d = sp->st_min;		break;
	case _ST_MAX:		d = sp->st_max;		break;
	case _ST_CACHESIZE:	d = sp->st_cache_size;	break;
	case _ST_FLAGS:		d = sp->st_flags;	break;
	}
	sp = _free(sp);
	if (!JS_NewNumberValue(cx, d, vp))
	    *vp = JSVAL_FALSE;
    }	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmseq_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;
    jsint tiny = JSVAL_TO_INT(id);
    jsdouble d = 0;
    int ret;

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
    case _CACHESIZE:
    {	int32_t _i = 0;
	if (seq->api_internal != NULL || !JS_ValueToNumber(cx, *vp, &d)) {
	    *vp = JSVAL_FALSE;
	    break;
	}
	_i = d;
	ret = seq->set_cachesize(seq, _i);
	*vp = (!ret ? JSVAL_TRUE : JSVAL_FALSE);
    }	break;
    case _FLAGS:
    {	uint32_t _u = 0;
	if (seq->api_internal != NULL || !JS_ValueToNumber(cx, *vp, &d)) {
	    *vp = JSVAL_FALSE;
	    break;
	}
	_u = d;
	ret = seq->set_flags(seq, _u);
	*vp = (!ret ? JSVAL_TRUE : JSVAL_FALSE);
    }	break;
    case _INITVAL:
    {	db_seq_t _ival = 0;

	if (seq->api_internal != NULL || !JS_ValueToNumber(cx, *vp, &d)) {
	    *vp = JSVAL_FALSE;
	    break;
	}
	_ival = d;
	ret = seq->initial_value(seq, _ival);
	*vp = (!ret ? JSVAL_TRUE : JSVAL_FALSE);
    }	break;
    case _RANGEMIN:
    case _RANGEMAX:
    {	db_seq_t _min = 0;
	db_seq_t _max = 0;

	if (seq->api_internal != NULL || !JS_ValueToNumber(cx, *vp, &d)) {
	    *vp = JSVAL_FALSE;
	    break;
	}
	if ((ret = seq->get_range(seq, &_min, &_max)) != 0) {
	    *vp = JSVAL_VOID;
	    break;
	}
	switch (tiny) {
	case _RANGEMIN:	_min = d;	break;
	case _RANGEMAX:	_max = d;	break;
	}
	ret = seq->set_range(seq, _min, _max);
	*vp = (!ret ? JSVAL_TRUE : JSVAL_FALSE);
    }	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmseq_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);

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
rpmseq_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static DB_SEQUENCE *
rpmseq_init(JSContext *cx, JSObject *obj, DB * db, uint32_t flags)
{
    DB_SEQUENCE * seq = NULL;
    int ret = db_sequence_create(&seq, db, flags);

    if (ret || seq == NULL || !JS_SetPrivate(cx, obj, (void *)seq)) {
	if (db)
	    db->err(db, ret, "db_sequence_create");
        else
            fprintf(stderr, "db_sequence_create: %s\n", db_strerror(ret));
	if (seq)
	    ret = seq->close(seq, 0);
	seq = NULL;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) seq %p\n", __FUNCTION__, cx, obj, seq);

    return seq;
}

static void
rpmseq_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmseqClass, NULL);
    DB_SEQUENCE * seq = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    if (seq)
	(void) seq->close(seq, 0);
}

static JSBool
rpmseq_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSObject * o = NULL;
    DB * _db = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &o, &_flags)))
	goto exit;

    if (OBJ_IS_RPMDB(cx, o))
	_db = JS_GetInstancePrivate(cx, o, &rpmdbClass, NULL);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmseq_init(cx, obj, _db, _flags);
    } else {
	if ((obj = JS_NewObject(cx, &rpmseqClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmseq_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmseqClass, NULL);
#ifdef	NOTYET
    DB_SEQUENCE * seq = ptr;
    const char *_fn = NULL;
    const char * _con = NULL;
#endif
    JSBool ok = JS_FALSE;

#ifdef	NOTYET
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_fn)))
        goto exit;

    *vp = (seq && _fn && (_con = rpmseqLgetfilecon(seq, _fn)) != NULL)
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
JSClass rpmseqClass = {
    "Seq", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmseq_addprop,   rpmseq_delprop, rpmseq_getprop, rpmseq_setprop,
    (JSEnumerateOp)rpmseq_enumerate, (JSResolveOp)rpmseq_resolve,
    rpmseq_convert,	rpmseq_dtor,

    rpmseq_getobjectops,	rpmseq_checkaccess,
    rpmseq_call,		rpmseq_construct,
    rpmseq_xdrobject,	rpmseq_hasinstance,
    rpmseq_mark,		rpmseq_reserveslots,
};

JSObject *
rpmjs_InitSeqClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmseqClass, rpmseq_ctor, 1,
		rpmseq_props, rpmseq_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewSeqObject(JSContext *cx, void * _db, uint32_t _flags)
{
    JSObject *obj;
    DB * db = _db;
    DB_SEQUENCE * seq;

    if ((obj = JS_NewObject(cx, &rpmseqClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((seq = rpmseq_init(cx, obj, db, _flags)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmseq, Seq, JS_TRUE)
