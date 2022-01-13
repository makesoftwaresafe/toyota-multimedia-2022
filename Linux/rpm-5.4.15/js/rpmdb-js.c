/** \ingroup js_c
 * \file js/rpmdb-js.c
 */

#include "system.h"

#define	_RPMDB_JS_INTERNAL
#include "rpmdb-js.h"

#include "rpmdbc-js.h"
#include "rpmdbe-js.h"
#include "rpmmpf-js.h"
#include "rpmtxn-js.h"
#include "rpmjs-debug.h"

#include <argv.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmdb_addprop		JS_PropertyStub
#define	rpmdb_delprop		JS_PropertyStub
#define	rpmdb_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmdb_getobjectops	NULL
#define	rpmdb_checkaccess	NULL
#define	rpmdb_call		rpmdb_call
#define	rpmdb_construct		rpmdb_ctor
#define	rpmdb_xdrobject		NULL
#define	rpmdb_hasinstance	NULL
#define	rpmdb_mark		NULL
#define	rpmdb_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmdb_equality		NULL
#define rpmdb_outerobject	NULL
#define rpmdb_innerobject	NULL
#define rpmdb_iteratorobject	NULL
#define rpmdb_wrappedobject	NULL

/* --- helpers */

#ifdef	UNUSED
static int
rpmdb_DBTcompare(DB * db, const DBT * A, const DBT * B)
{
    size_t nb = (A->size < B->size ? A->size : B->size);
    int ret = (nb > 0 ? memcmp(A->data, B->data, nb) : 0);
    if (ret)
	return ret;
    return (int)((long)A->size - (long)B->size);
}
#endif

static void
rpmdb_feedback(DB * db, int opcode, int percent)
{
#ifdef	NOTNOW
    JSObject * o = (db ? db->app_private : NULL);
if (_debug)
fprintf(stderr, "==> %s(%p, %d, %d) o %p\n", __FUNCTION__, db, opcode, percent, o);
#endif
}

int
rpmdb_v2dbt(JSContext *cx, jsval v, _RPMDBT * p)
{
    int rc = 0;

    if (v == JSVAL_NULL || v == JSVAL_VOID) {
	p->dbt.size = 0;
	p->dbt.data = NULL;
    } else
    if (JSVAL_IS_INT(v)) {
	JS_ValueToInt32(cx, v, &p->_u._i);
	p->dbt.size = sizeof(p->_u._i);
	p->dbt.data = &p->_u._i;
    } else
    if (JSVAL_IS_DOUBLE(v)) {
	jsdouble d = 0;
	JS_ValueToNumber(cx, v, &d);
	p->_u._ll = d;
	p->dbt.size = sizeof(p->_u._ll);
	p->dbt.data = &p->_u._ll;
    } else
    if (JSVAL_IS_STRING(v)) {
	p->dbt.data = JS_EncodeString(cx, JSVAL_TO_STRING(v));
	p->dbt.size = strlen(p->dbt.data) + 1;
    } else
    if (JSVAL_IS_OBJECT(v)) {
#ifdef	NOTYET
        JSObject *o = JSVAL_TO_OBJECT(v);
        if (OBJ_IS_FOO(cx, o))
            foo = JS_GetInstancePrivate(cx, o, &rpmfooClass, NULL);
#else
	p->dbt.size = 0;
	p->dbt.data = NULL;
#endif
    } else
    {
#ifdef	NOTNOW
if (_debug)
fprintf(stderr, "*** %s(%p, 0x%x, %p)  FIXME\n", __FUNCTION__, cx, (unsigned)v, p);
#endif
	rc = 0;
    }
    return rc;
}

static int
rpmdb_Acallback(DB * db, const DBT * _k, const DBT * _d, DBT * _r)
{
if (_debug)
fprintf(stderr, "==> %s(%p, %p, %p, %p) k %p[%u] d %p[%u]\n", __FUNCTION__, db, _k, _d, _r, _k->data, (unsigned)_k->size, _d->data, (unsigned)_d->size);
    _r->data = _d->data;
    _r->size = _d->size;
    return 0;
}

static int
rpmdb_AFcallback(DB * db, const DBT * _k, DBT * _d, const DBT * _fk,
		int * _changed)
{
#ifdef	NOTNOW
if (_debug)
fprintf(stderr, "==> %s(%p, %p, %p, %p, %p) k %p[%u] fk %p[%u] changed %d\n", __FUNCTION__, db, _k, _d, _fk, _changed, _k->data, (unsigned)_k->size, _fk->data, (unsigned)_fk->size, *_changed);
#endif
    return 0;
}

/* --- Object methods */

static JSBool
rpmdb_Associate(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o_txn = NULL;
    DB_TXN * _txn = NULL;
    JSObject * o_db = NULL;
    DB * _secondary = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "oo/u", &o_txn, &o_db, &_flags)))
	goto exit;

    if (o_db && OBJ_IS_RPMDB(cx, o_db))
	_secondary = JS_GetInstancePrivate(cx, o_db, &rpmdbClass, NULL);
    else goto exit;
    if (o_txn && OBJ_IS_RPMTXN(cx, o_txn))
	_txn = JS_GetInstancePrivate(cx, o_txn, &rpmtxnClass, NULL);

    if (db->app_private != NULL) {
	/* XXX todo++ */
	int (*_callback) (DB *, const DBT *, const DBT *, DBT *)
		= rpmdb_Acallback;
	int ret = db->associate(db, _txn, _secondary, _callback, _flags);
#ifdef	NOTNOW
if (_debug)
fprintf(stderr, "==> %s: ret %d = db->associate(%p, %p, %p %p, 0x%x)\n", __FUNCTION__, ret, db, _txn, _secondary, _callback, _flags);
#endif
	switch (ret) {
	default:
	    db->err(db, ret, "DB->associate");
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
rpmdb_AssociateForeign(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o_db = NULL;
    DB * _secondary = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &o_db, &_flags)))
	goto exit;

    if (o_db && OBJ_IS_RPMDB(cx, o_db))
	_secondary = JS_GetInstancePrivate(cx, o_db, &rpmdbClass, NULL);
    else goto exit;

    if (db->app_private != NULL) {
	/* XXX todo++ */
	int (*_callback) (DB *, const DBT *, DBT *, const DBT *, int *)
		= (_flags & (DB_FOREIGN_ABORT | DB_FOREIGN_CASCADE))
			? NULL : rpmdb_AFcallback;
	int ret = db->associate_foreign(db, _secondary, _callback, _flags);
#ifdef	NOTNOW
if (_debug)
fprintf(stderr, "==> %s: ret %d = db->associate_foreign(%p, %p, %p, 0x%x)\n", __FUNCTION__, ret, db, _secondary, _callback, _flags);
#endif
	switch (ret) {
	default:
	    db->err(db, ret, "DB->associate_foreign");
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
rpmdb_Close(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    {	int ret = db->close(db, _flags);
	if (ret)
	    db->err(db, ret, "DB->close");
	else
	    *vp = JSVAL_TRUE;
	db = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmdb_Compact(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &o, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);

    if (db->app_private != NULL) {
	DBT * _start = NULL;		/* XXX todo++ */
	DBT * _stop = NULL;		/* XXX todo++ */
	DB_COMPACT * _c_data = NULL;	/* XXX todo++ */
	DBT * _end = NULL;		/* XXX todo++ */
	int ret = db->compact(db, _txn, _start, _stop, _c_data, _flags, _end);
	if (ret)
	    db->err(db, ret, "DB->compact");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Cursor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/ou", &o, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);

    if (db->app_private != NULL) {
	DBC * _dbc = NULL;
	int ret = db->cursor(db, _txn, &_dbc, _flags);
	switch (ret) {
	default:
	    db->err(db, ret, "DB->cursor");
	    goto exit;
	    break;
	case 0:
	    if ((o = JS_NewObject(cx, &rpmdbcClass, NULL, NULL)) == NULL
	     || !JS_SetPrivate(cx, o, (void *)_dbc))
	    {
		if (_dbc)	_dbc->close(_dbc);
		/* XXX error msg */
		goto exit;
	    }
	    *vp = OBJECT_TO_JSVAL(o);
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Del(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ov/u", &o, &_kv, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;

    if (db->app_private != NULL) {
	int ret = db->del(db, _txn, _RPMDBT_PTR(_k), _flags);
	switch (ret) {
	default:
	    db->err(db, ret, "DB->del");
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	case DB_NOTFOUND:
	    *vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Exists(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ov/u", &o, &_kv, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;

    if (db->app_private != NULL) {
	int ret = db->exists(db, _txn, _RPMDBT_PTR(_k), _flags);
	switch (ret) {
	default:
	    *vp = JSVAL_FALSE;
	    db->err(db, ret, "DB->exists");
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	case DB_NOTFOUND:
	    *vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Get(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    jsval _dv = JSVAL_NULL;
    _RPMDBT _d = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ov/vu", &o, &_kv, &_dv, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;
    if (rpmdb_v2dbt(cx, _dv, &_d))
	goto exit;

    if (db->app_private != NULL) {
	int ret = db->get(db, _txn, _RPMDBT_PTR(_k), _RPMDBT_PTR(_d), _flags);
#ifdef	NOTNOW
if (_debug)
fprintf(stderr, "==> %s: ret %d = db->get(%p, %p, %p[%u], %p[%u], 0x%x)\n",
__FUNCTION__, ret, db, _txn,
_RPMDBT_PTR(_k)->data, (unsigned)_RPMDBT_PTR(_k)->size,
_RPMDBT_PTR(_d)->data, (unsigned)_RPMDBT_PTR(_d)->size,
_flags);
#endif
	switch (ret) {
	default:
	    *vp = JSVAL_FALSE;
	    db->err(db, ret, "DB->get");
	    goto exit;
	    break;
	case 0:
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, _RPMDBT_PTR(_d)->data, _RPMDBT_PTR(_d)->size));
	    break;
	case DB_NOTFOUND:
	    *vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Join(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DBC * _dbc = NULL;
    size_t nb = (argc + 1) * sizeof(_dbc);
    DBC ** _list = memset(alloca(nb), 0, nb);
    uint32_t _flags = 0;
    uint32_t i;
    jsval v;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    /* Build the cursor list. */
    for (i = 0; i < argc; i++) {
	v = argv[i];
	if (!JSVAL_IS_OBJECT(v))
	    break;
	o = JSVAL_TO_OBJECT(v);
	if (!(o && OBJ_IS_RPMDBC(cx, o)))
	    break;
	if ((_list[i] = JS_GetInstancePrivate(cx, o, &rpmdbcClass, NULL)) == NULL)
	    break;
    }
    _list[i] = NULL;

    /* Set _flags from optional last int in args */
    if (i+1 == argc && JSVAL_IS_INT(argv[i]))
	_flags = JSVAL_TO_INT(argv[i++]);
    if (i < argc)
	goto exit;

    if (db->app_private != NULL) {
	int ret = db->join(db, _list, &_dbc, _flags);
#ifdef	NOTNOW
if (_debug)
fprintf(stderr, "==> %s: ret %d = db->join(%p, %p, %p 0x%x) dbc %p\n", __FUNCTION__, ret, db, _list, &_dbc, _flags, _dbc);
#endif
	switch (ret) {
	default:
	    db->err(db, ret, "DB->join");
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    if ((o = JS_NewObject(cx, &rpmdbcClass, NULL, NULL)) == NULL
	     || !JS_SetPrivate(cx, o, (void *)_dbc))
	    {
		if (_dbc)	_dbc->close(_dbc);
		goto exit;
	    }
	    *vp = OBJECT_TO_JSVAL(o);
	    break;
	}
    }

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmdb_KeyRange(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ov/u", &o, &_kv, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;

    if (db->app_private != NULL) {
	DB_KEY_RANGE _key_range = {0};
	int ret = db->key_range(db, _txn, _RPMDBT_PTR(_k), &_key_range, _flags);
#ifdef	NOTNOW
if (_debug)
fprintf(stderr, "==> %s: ret %d = db->key_range(%p, %p, %p, %p, 0x%x) <%g =%g >%g\n", __FUNCTION__, ret, db, _txn, _RPMDBT_PTR(_k), &_key_range, _flags, _key_range.less, _key_range.equal, _key_range.greater);
#endif
	switch (ret) {
	default:
	    db->err(db, ret, "DB->key_range");
	    goto exit;
	    break;
	case 0:
	  { void * mark = NULL;
	    *vp = OBJECT_TO_JSVAL(JS_NewArrayObject(cx, 3,
			JS_PushArguments(cx, &mark, "ddd",
				_key_range.less,
				_key_range.equal,
				_key_range.greater)));
	    JS_PopArguments(cx, mark);
	  } break;
	}
    }

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmdb_Open(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    const char * _file = NULL;
    const char * _database = NULL;
    DBTYPE _type = DB_HASH;
    uint32_t _oflags = DB_CREATE;
    int _mode = 0644;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "os/suui", &o, &_file, &_database, &_type, &_oflags, &_mode)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);

    /* XXX hmm, "null" is getting passed somehow. */
    if (_database && (*_database == '\0' || !strcmp(_database, "null")))
	_database = NULL;
    if (_type == DB_QUEUE)
	_database = NULL;

    if (db->app_private == NULL) {
	int ret = db->open(db, _txn, _file, _database, _type, _oflags, _mode);
#ifdef	NOTNOW
if (_debug)
fprintf(stderr, "==> %s: ret %d = db->open(%p, %p, \"%s\", \"%s\", %d, 0x%x, 0%o)\n", __FUNCTION__, ret, db, _txn, _file, _database, _type, _oflags, _mode);
#endif
	if (ret) {
	    db->err(db, ret, "DB->open");
	    goto exit;
	} else {
	    db->app_private = obj;
            /* XXX only DB_UPGRADE/DB_VERIFY are currently implemented */
            ret = db->set_feedback(db, rpmdb_feedback);
            if (ret) db->err(db, ret, "DB->set_feedback");
	    *vp = JSVAL_TRUE;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Pget(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    jsval _dv = JSVAL_NULL;
    _RPMDBT _d = _RPMDBT_INIT;
    _RPMDBT _p = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ov/vu", &o, &_kv, &_dv, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;
    if (rpmdb_v2dbt(cx, _dv, &_d))
	goto exit;

    if (db->app_private != NULL) {
	int ret = db->pget(db, _txn, _RPMDBT_PTR(_k), _RPMDBT_PTR(_p), _RPMDBT_PTR(_d), _flags);
	switch (ret) {
	default:
	    *vp = JSVAL_FALSE;
	    db->err(db, ret, "DB->pget");
	    goto exit;
	    break;
	case 0:
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, _RPMDBT_PTR(_d)->data, _RPMDBT_PTR(_d)->size));
	    break;
	case DB_NOTFOUND:
	    *vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Put(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    jsval _kv = JSVAL_NULL;
    _RPMDBT _k = _RPMDBT_INIT;
    jsval _dv = JSVAL_NULL;
    _RPMDBT _d = _RPMDBT_INIT;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ovv/u", &o, &_kv, &_dv, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);
    if (rpmdb_v2dbt(cx, _kv, &_k))
	goto exit;
    if (rpmdb_v2dbt(cx, _dv, &_d))
	goto exit;

    if (db->app_private != NULL) {
	int ret = db->put(db, _txn, _RPMDBT_PTR(_k), _RPMDBT_PTR(_d), _flags);
	switch (ret) {
	default:
	    *vp = JSVAL_FALSE;
	    db->err(db, ret, "DB->put");
	    goto exit;
	    break;
	case 0:
	    *vp = JSVAL_TRUE;
	    break;
	case DB_NOTFOUND:
	    *vp = JSVAL_VOID;
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Remove(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    const char * _file = NULL;
    const char * _database = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s/s", &_file, &_database)))
	goto exit;

    if (db->app_private == NULL) {
	uint32_t _flags = 0;
	int ret = db->remove(db, _file, _database, _flags);
	if (ret)
	    fprintf(stderr, "db->remove: %s\n", db_strerror(ret));
	else
	    *vp = JSVAL_TRUE;
	db = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Rename(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    const char * _file = NULL;
    const char * _database = NULL;
    const char * _newname = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "sss", &_file, &_database, &_newname)))
	goto exit;

    /* XXX lazy reopen in order to rename? */

    if (db->app_private == NULL) {
	uint32_t _flags = 0;
	int ret = db->rename(db, _file, _database, _newname, _flags);
	if (ret)
	    db->err(db, ret, "DB->rename");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Stat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    uint32_t _flags = DB_STAT_ALL;	/* XXX DB_FAST_STAT is saner default */
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &o, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);

    if (db->app_private != NULL) {
	void * _sp = NULL;
	int ret = db->stat(db, _txn, &_sp, _flags);
	if (ret)
	    db->err(db, ret, "DB->stat");
	else
	    *vp = JSVAL_TRUE;
	/* XXX FIXME: format for return. ugh */
	_sp = _free(_sp);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_StatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    uint32_t _flags = DB_STAT_ALL;	/* XXX DB_FAST_STAT is saner default */
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (db->app_private != NULL) {
	int ret = db->stat_print(db, _flags);
	if (ret)
	    db->err(db, ret, "DB->stat_print");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Sync(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    /* XXX check argc? */

    {	uint32_t _flags = 0;
	int ret = db->sync(db, _flags);
	if (ret)
	    db->err(db, ret, "DB->sync");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Truncate(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    JSObject * o = NULL;
    DB_TXN * _txn = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    /* XXX check argc? */
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &o, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_txn = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);

    {	uint32_t _count = 0;
	int ret = db->truncate(db, _txn, &_count, _flags);
	if (ret) {
	    db->err(db, ret, "DB->truncate");
	    goto exit;
	}
	*vp = INT_TO_JSVAL(_count);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Upgrade(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    const char * _file = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_file)))
	goto exit;

    /* XXX check db->lorder, same endianness is required. */
    {	uint32_t _flags = 0;	/* XXX DB_DUPSORT prior to db-3.1 */
	int ret = db->upgrade(db, _file, _flags);
	if (ret) {
	    db->err(db, ret, "DB->upgrade");
	    goto exit;
	}
	*vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_Verify(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    const char * _file = NULL;
    const char * _database = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (db == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "ssu", &_file, &_database, &_flags)))
	goto exit;

    if (db->app_private == NULL) {
	FILE * _outfile = NULL;		/* XXX optional key/val dump */
	int ret = db->verify(db, _file, _database, _outfile, _flags);
	if (ret)
	    fprintf(stderr, "db->verify: %s\n", db_strerror(ret));
	else
	    *vp = JSVAL_TRUE;
	db = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSFunctionSpec rpmdb_funcs[] = {
    JS_FS("associate",	rpmdb_Associate,	0,0),
    JS_FS("associate_foreign",	rpmdb_AssociateForeign,	0,0),
    JS_FS("close",	rpmdb_Close,		0,0),
    JS_FS("compact",	rpmdb_Compact,		0,0),
    JS_FS("cursor",	rpmdb_Cursor,		0,0),
    JS_FS("del",	rpmdb_Del,		0,0),
    JS_FS("exists",	rpmdb_Exists,		0,0),
    JS_FS("get",	rpmdb_Get,		0,0),
    JS_FS("join",	rpmdb_Join,		0,0),
    JS_FS("key_range",	rpmdb_KeyRange,		0,0),
    JS_FS("open",	rpmdb_Open,		0,0),
    JS_FS("pget",	rpmdb_Pget,		0,0),
    JS_FS("put",	rpmdb_Put,		0,0),
    JS_FS("remove",	rpmdb_Remove,		0,0),
    JS_FS("rename",	rpmdb_Rename,		0,0),
    JS_FS("stat",	rpmdb_Stat,		0,0),
    JS_FS("stat_print",	rpmdb_StatPrint,	0,0),
    JS_FS("sync",	rpmdb_Sync,		0,0),
    JS_FS("truncate",	rpmdb_Truncate,		0,0),
    JS_FS("upgrade",	rpmdb_Upgrade,		0,0),
    JS_FS("verify",	rpmdb_Verify,		0,0),
    JS_FS_END
};

/* --- Object properties */

#define	_TABLE(_v)	#_v, _##_v, JSPROP_ENUMERATE, NULL, NULL

enum rpmdb_tinyid {
    _DEBUG		= -2,
    _BYTESWAPPED	= -3,
    _DBFILE		= -4,
    _DBNAME		= -5,
    _MULTIPLE		= -6,
    _OPEN_FLAGS		= -7,
    _TYPE		= -8,
    _BT_MINKEY		= -9,
    _CACHESIZE		= -10,
    _CREATE_DIR		= -11,
    _ENCRYPT		= -12,
    _ERRFILE		= -13,
    _ERRPFX		= -14,
    _FLAGS		= -15,
    _H_FFACTOR		= -16,
    _H_NELEM		= -17,
    _LORDER		= -18,
    _MPF		= -19,
    _MSGFILE		= -20,
    _PAGESIZE		= -21,
    _PARTITION_DIRS	= -22,
    _PRIORITY		= -23,
    _Q_EXTENTSIZE	= -24,
    _RE_DELIM		= -25,
    _RE_LEN		= -26,
    _RE_PAD		= -27,
    _RE_SOURCE		= -28,

    _ST_BUCKETS		= -29,
    _ST_CUR_RECNO	= -30,
    _ST_EXTENTSIZE	= -31,
    _ST_FFACTOR		= -32,
    _ST_FIRST_RECNO	= -33,
    _ST_MAGIC		= -34,
    _ST_MINKEY		= -35,
    _ST_NDATA		= -36,
    _ST_NKEYS		= -37,
    _ST_PAGECNT		= -38,
    _ST_PAGESIZE	= -39,
    _ST_RE_LEN		= -40,
    _ST_RE_PAD		= -41,
    _ST_VERSION		= -42,

#ifndef	NOTFAST
    _ST_BFREE		= -43,
    _ST_BIG_BFREE	= -44,
    _ST_BIGPAGES	= -45,
    _ST_BT_DUP_PG	= -46,
    _ST_BT_DUP_PGFREE	= -47,
    _ST_BT_EMPTY_PG	= -48,
    _ST_BT_FREE		= -49,
    _ST_BT_INT_PG	= -50,
    _ST_BT_INT_PGFREE	= -51,
    _ST_BT_LEAF_PG	= -52,
    _ST_BT_LEAF_PGFREE	= -53,
    _ST_BT_LEVELS	= -54,
    _ST_BT_OVER_PG	= -55,
    _ST_BT_OVER_PGFREE	= -56,
    _ST_DUP		= -57,
    _ST_DUP_FREE	= -58,
    _ST_FREE		= -59,
    _ST_OVERFLOWS	= -60,
    _ST_OVFL_FREE	= -61,
    _ST_PAGES		= -62,
    _ST_PGFREE		= -63,
#endif
};

static JSPropertySpec rpmdb_props[] = {
    {"debug",		_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"byteswapped",	_BYTESWAPPED,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"dbfile",		_DBFILE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"dbname",		_DBNAME,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"multiple",	_MULTIPLE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"open_flags",	_OPEN_FLAGS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"type",		_TYPE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"bt_minkey",	_BT_MINKEY,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"cachesize",	_CACHESIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"create_dir",	_CREATE_DIR,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"encrypt",		_ENCRYPT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"errfile",		_ERRFILE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"errpfx",		_ERRPFX,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"flags",		_FLAGS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"h_ffactor",	_H_FFACTOR,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"h_nelem",		_H_NELEM,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"lorder",		_LORDER,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"mpf",		_MPF,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"msgfile",		_MSGFILE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"pagesize",	_PAGESIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"partition_dirs",	_PARTITION_DIRS, JSPROP_ENUMERATE,	NULL,	NULL},
    {"priority",	_PRIORITY,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"q_extentsize",	_Q_EXTENTSIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"re_delim",	_RE_DELIM,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"re_len",		_RE_LEN,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"re_pad",		_RE_PAD,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"re_source",	_RE_SOURCE,	JSPROP_ENUMERATE,	NULL,	NULL},

    {"st_buckets",	_ST_BUCKETS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_cur_recno",	_ST_CUR_RECNO,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_extentsize",	_ST_EXTENTSIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_ffactor",	_ST_FFACTOR,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_first_recno",	_ST_FIRST_RECNO, JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_magic",	_ST_MAGIC,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_minkey",	_ST_MINKEY,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_ndata",	_ST_NDATA,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_nkeys",	_ST_NKEYS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_pagcnt",	_ST_PAGECNT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_pagesize",	_ST_PAGESIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_re_len",	_ST_RE_LEN,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_re_pad",	_ST_RE_PAD,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_version",	_ST_VERSION,	JSPROP_ENUMERATE,	NULL,	NULL},

#ifndef	NOTFAST
    {"st_bfree",	_ST_BFREE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_big_bfree",	_ST_BIG_BFREE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bigpages",	_ST_BIGPAGES,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_dup_pg",	_ST_BT_DUP_PG,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_dup_pgfree",_ST_BT_DUP_PGFREE, JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_empty_pg",	_ST_BT_EMPTY_PG, JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_free",	_ST_BT_FREE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_int_pg",	_ST_BT_INT_PG,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_int_pgfree",_ST_BT_INT_PGFREE, JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_leaf_pg",	_ST_BT_LEAF_PG,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_leaf_pgfree", _ST_BT_LEAF_PGFREE, JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_levels",	_ST_BT_LEVELS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_over_pg",	_ST_BT_OVER_PG,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_bt_over_pgfree", _ST_BT_OVER_PGFREE, JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_dup",		_ST_DUP,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_dup_free",	_ST_DUP_FREE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_free",		_ST_FREE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_overflows",	_ST_OVERFLOWS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_ovfl_free",	_ST_OVFL_FREE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_pages",	_ST_PAGES,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"st_pgfree",	_ST_PGFREE,	JSPROP_ENUMERATE,	NULL,	NULL},
#endif
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmdb_getFastStats(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
		void * _txn, uint32_t flags)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    jsint tiny = JSVAL_TO_INT(id);

    DBTYPE _type = DB_UNKNOWN;
    DB_TXN * txn = _txn;
    void * _sp = NULL;
    int ret = db->stat(db, txn, &_sp, flags);
    DB_BTREE_STAT *bsp = _sp;
    DB_HASH_STAT *hsp = _sp;
    DB_QUEUE_STAT *qsp = _sp;
    jsdouble dorig = 0.0123456789;
    jsdouble d = dorig;

    ret = db->get_type(db, &_type);
    switch (_type) {
    case DB_RECNO:
    case DB_BTREE:
      switch (tiny) {
	case _ST_MAGIC:			d = bsp->bt_magic;	break;
	case _ST_VERSION:		d = bsp->bt_version;	break;
	case _ST_NKEYS:			d = bsp->bt_nkeys;	break;
	case _ST_NDATA:			d = bsp->bt_ndata;	break;
	case _ST_MINKEY:		d = bsp->bt_minkey;	break;
	case _ST_RE_LEN:		d = bsp->bt_re_len;	break;
	case _ST_RE_PAD:		d = bsp->bt_re_pad;	break;
	case _ST_PAGESIZE:		d = bsp->bt_pagesize;	break;
	case _ST_PAGECNT:		d = bsp->bt_pagecnt;	break;
#ifndef	NOTFAST
	case _ST_BT_LEVELS:		d = bsp->bt_levels;	break;
	case _ST_BT_INT_PG:		d = bsp->bt_int_pg;	break;
	case _ST_BT_LEAF_PG:		d = bsp->bt_leaf_pg;	break;
	case _ST_BT_DUP_PG:		d = bsp->bt_dup_pg;	break;
	case _ST_BT_OVER_PG:		d = bsp->bt_over_pg;	break;
	case _ST_BT_EMPTY_PG:		d = bsp->bt_empty_pg;	break;
	case _ST_BT_FREE:		d = bsp->bt_free;	break;
	case _ST_BT_INT_PGFREE:		d = bsp->bt_int_pgfree;	break;
	case _ST_BT_LEAF_PGFREE:	d = bsp->bt_leaf_pgfree; break;
	case _ST_BT_DUP_PGFREE:		d = bsp->bt_dup_pgfree;	break;
	case _ST_BT_OVER_PGFREE:	d = bsp->bt_over_pgfree; break;
#endif
      }	break;
    case DB_HASH:
      switch (tiny) {
	case _ST_MAGIC:			d = hsp->hash_magic;	break;
	case _ST_VERSION:		d = hsp->hash_version;	break;
	case _ST_PAGESIZE:		d = hsp->hash_pagesize;	break;
	case _ST_PAGECNT:		d = hsp->hash_pagecnt;	break;
	case _ST_NKEYS:			d = hsp->hash_nkeys;	break;
	case _ST_NDATA:			d = hsp->hash_ndata;	break;
	case _ST_FFACTOR:		d = hsp->hash_ffactor;	break;
	case _ST_BUCKETS:		d = hsp->hash_buckets;	break;
#ifndef	NOTFAST
	case _ST_FREE:			d = hsp->hash_free;	break;
	case _ST_BFREE:			d = hsp->hash_bfree;	break;
	case _ST_BIGPAGES:		d = hsp->hash_bigpages;	break;
	case _ST_BIG_BFREE:		d = hsp->hash_big_bfree; break;
	case _ST_OVERFLOWS:		d = hsp->hash_overflows; break;
	case _ST_OVFL_FREE:		d = hsp->hash_ovfl_free; break;
	case _ST_DUP:			d = hsp->hash_dup;	break;
	case _ST_DUP_FREE:		d = hsp->hash_dup_free;	break;
#endif
      }	break;
    case DB_QUEUE:
      switch (tiny) {
	case _ST_MAGIC:			d = qsp->qs_magic;	break;
	case _ST_VERSION:		d = qsp->qs_version;	break;
	case _ST_PAGESIZE:		d = qsp->qs_pagesize;	break;
	case _ST_EXTENTSIZE:		d = qsp->qs_extentsize;	break;
	case _ST_NKEYS:			d = qsp->qs_nkeys;	break;
	case _ST_NDATA:			d = qsp->qs_ndata;	break;
	case _ST_RE_LEN:		d = qsp->qs_re_len;	break;
	case _ST_RE_PAD:		d = qsp->qs_re_pad;	break;
	case _ST_FIRST_RECNO:		d = qsp->qs_first_recno; break;
	case _ST_CUR_RECNO:		d = qsp->qs_cur_recno;	break;
#ifndef	NOTFAST
	case _ST_PAGES:			d = qsp->qs_pages;	break;
	case _ST_PGFREE:		d = qsp->qs_pgfree;	break;
#endif
      }	break;
    case DB_UNKNOWN:
	break;
    }

    _sp = _free(_sp);

    if (d != dorig) {
        if (!JS_NewNumberValue(cx, d, vp))
            *vp = JSVAL_FALSE;
    } else
	*vp = JSVAL_VOID;

    return JS_TRUE;
}

#define	_RET_B(_bool)	((_bool) > 0 ? JSVAL_TRUE: JSVAL_FALSE)
#define	_RET_S(_str)	\
    ((_str) ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (_str))) : JSVAL_NULL)
#define	_GET_S(_test)	((_test) ? _RET_S(_s) : JSVAL_VOID)
#define	_GET_U(_test)	((_test) ? INT_TO_JSVAL(_u) : JSVAL_VOID)
#define	_GET_I(_test)	((_test) ? INT_TO_JSVAL(_i) : JSVAL_VOID)
#define	_GET_B(_test)	((_test) ? _RET_B(_i) : JSVAL_VOID)

static JSBool
rpmdb_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    const char ** _av = NULL;
    int _ac = 0;
    const char * _s = NULL;
    const char * _t = NULL;
    uint32_t _gb = 0;
    uint32_t _u = 0;
    int _i = 0;
    FILE * _fp = NULL;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;

    case _BYTESWAPPED:	*vp = _GET_I(!db->get_byteswapped(db, &_i));	break;
    case _DBFILE:	*vp = _GET_S(!db->get_dbname(db, &_s, &_t));	break;
    case _DBNAME:	*vp = _GET_S(!db->get_dbname(db, &_t, &_s));	break;
    case _MULTIPLE:	*vp = _RET_B(db->get_multiple(db));		break;
    case _OPEN_FLAGS:	*vp = _GET_U(!db->get_open_flags(db, &_u));	break;
    case _TYPE:		*vp = _GET_U(!db->get_type(db, &_u));	break;
    case _BT_MINKEY:	*vp = _GET_U(!db->get_bt_minkey(db, &_u));	break;
    case _CACHESIZE:	*vp = _GET_U(!db->get_cachesize(db, &_gb, &_u, &_i)); break;
    case _CREATE_DIR:
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
	*vp = _GET_S(!db->get_create_dir(db, &_s));
#else
	*vp = JSVAL_VOID;
#endif
	break;
    case _ENCRYPT:	*vp = _GET_U(!db->get_encrypt_flags(db, &_u));	break;
    case _ERRFILE:
	db->get_errfile(db, &_fp);
	_s = (_fp ? "other" : NULL);
	if (_fp == stdin)	_s = "stdin";
	if (_fp == stdout)	_s = "stdout";
	if (_fp == stderr)	_s = "stderr";
	*vp = _RET_S(_s);
	break;
    case _ERRPFX:
	db->get_errpfx(db, &_s);
	*vp = _RET_S(_s);
	break;
    case _FLAGS:	*vp = _GET_U(!db->get_flags(db, &_u));		break;
    case _H_FFACTOR:	*vp = _GET_U(!db->get_h_ffactor(db, &_u));	break;
    case _H_NELEM:	*vp = _GET_U(!db->get_h_nelem(db, &_u));	break;
    case _LORDER:	*vp = _GET_I(!db->get_lorder(db, &_i));		break;
    case _MPF:
    {	DB_MPOOLFILE * _mpf = db->get_mpf(db);
	if (_mpf) {
	    JSObject * o = JS_NewObject(cx, &rpmmpfClass, NULL, NULL);
	    *vp = (o && JS_SetPrivate(cx, o, (void *)_mpf)
		? OBJECT_TO_JSVAL(o) : JSVAL_VOID);
	} else
	    *vp = JSVAL_NULL;
    }	break;
    case _MSGFILE:
	db->get_msgfile(db, &_fp);
	_s = (_fp ? "other" : NULL);
	if (_fp == stdin)	_s = "stdin";
	if (_fp == stdout)	_s = "stdout";
	if (_fp == stderr)	_s = "stderr";
	*vp = _RET_S(_s);
	break;
    case _PAGESIZE:	*vp = _GET_U(!db->get_pagesize(db, &_u));	break;
    case _PARTITION_DIRS:	break;
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
	if (!db->get_partition_dirs(db, &_av)
	 && (_ac = argvCount(_av)) > 0)
	{
	    _s = _av[0];	/* XXX FIXME: return array */
	    *vp = _RET_S(_s);
	} else
#endif
	    *vp = JSVAL_VOID;
	break;
    case _PRIORITY:	*vp = _GET_U(!db->get_priority(db, &_u));	break;
    case _Q_EXTENTSIZE:	*vp = _GET_U(!db->get_q_extentsize(db, &_u));	break;
    case _RE_DELIM:	*vp = _GET_I(!db->get_re_delim(db, &_i));	break;
    case _RE_LEN:	*vp = _GET_U(!db->get_re_len(db, &_u));		break;
    case _RE_PAD:	*vp = _GET_I(!db->get_re_pad(db, &_i));		break;
    case _RE_SOURCE:	*vp = _GET_S(!db->get_re_source(db, &_s));	break;
    case _ST_BUCKETS:
    case _ST_CUR_RECNO:
    case _ST_EXTENTSIZE:
    case _ST_FFACTOR:
    case _ST_FIRST_RECNO:
    case _ST_MAGIC:
    case _ST_MINKEY:
    case _ST_NDATA:
    case _ST_NKEYS:
    case _ST_PAGECNT:
    case _ST_PAGESIZE:
    case _ST_RE_LEN:
    case _ST_RE_PAD:
    case _ST_VERSION:
    {	void * _txn = NULL;
	uint32_t _flags = DB_READ_UNCOMMITTED | DB_FAST_STAT;
	(void) rpmdb_getFastStats(cx, obj, id, vp, _txn, _flags);
    }	break;

#ifndef	NOTFAST
    case _ST_BFREE:
    case _ST_BIG_BFREE:
    case _ST_BIGPAGES:
    case _ST_BT_DUP_PG:
    case _ST_BT_DUP_PGFREE:
    case _ST_BT_EMPTY_PG:
    case _ST_BT_FREE:
    case _ST_BT_INT_PG:
    case _ST_BT_INT_PGFREE:
    case _ST_BT_LEAF_PG:
    case _ST_BT_LEAF_PGFREE:
    case _ST_BT_LEVELS:
    case _ST_BT_OVER_PG:
    case _ST_BT_OVER_PGFREE:
    case _ST_DUP:
    case _ST_DUP_FREE:
    case _ST_FREE:
    case _ST_OVERFLOWS:
    case _ST_OVFL_FREE:
    case _ST_PAGES:
    case _ST_PGFREE:
    {	void * _txn = NULL;
	uint32_t _flags = DB_READ_UNCOMMITTED;
	(void) rpmdb_getFastStats(cx, obj, id, vp, _txn, _flags);
    }	break;
#endif

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
rpmdb_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    const char * _s = NULL;
    uint32_t _gb = 0;
    uint32_t _b = 0;
    uint32_t _u = 0;
    int _i = 0;
    int _nc = 0;
    FILE * _fp = NULL;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    if (JSVAL_IS_STRING(*vp))
	_s = JS_EncodeString(cx, JS_ValueToString(cx, *vp));
    if (JSVAL_IS_INT(*vp))
	_u = _i = JSVAL_TO_INT(*vp);

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;

#ifdef	NOTYET
    case _DBFILE:	break;
    case _DBNAME:	break;
#endif

    case _BT_MINKEY:	*vp = _PUT_U(db->set_bt_minkey(db, _u));	break;
    case _CACHESIZE:
	if (!db->get_cachesize(db, &_gb, &_b, &_nc)) {
	    _b = _u;
	    *vp = !db->set_cachesize(db, _gb, _b, _nc)
		? JSVAL_TRUE : JSVAL_FALSE;
	} else
	    *vp = JSVAL_FALSE;
	break;
    case _CREATE_DIR:
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
	*vp = _PUT_S(db->set_create_dir(db, _s));
#else
	*vp = JSVAL_VOID;
#endif
	break;
    case _ENCRYPT:	*vp = _PUT_S(db->set_encrypt(db, _s, DB_ENCRYPT_AES)); break;
    case _ERRFILE:
	/* XXX FIXME: cleaner typing */
	_fp = NULL;
	if (!strcmp(_s, "stdout")) _fp = stdout;
	if (!strcmp(_s, "stderr")) _fp = stderr;
	db->set_errfile(db, _fp);
	*vp = JSVAL_TRUE;
	break;
    case _ERRPFX:
	db->set_errpfx(db, _s);
	*vp = JSVAL_TRUE;
	break;
    case _FLAGS:	*vp = _PUT_U(db->set_flags(db, _u));	break;
    case _H_FFACTOR:	*vp = _PUT_U(db->set_h_ffactor(db, _u));	break;
    case _H_NELEM:	*vp = _PUT_U(db->set_h_nelem(db, _u));		break;
    case _LORDER:	*vp = _PUT_I(db->set_h_nelem(db, _i));		break;
    case _MSGFILE:
	/* XXX FIXME: cleaner typing */
	_fp = NULL;
	if (_s != NULL) {
	    if (!strcmp(_s, "stdout")) _fp = stdout;
	    else if (!strcmp(_s, "stderr")) _fp = stderr;
	    /* XXX FIXME: fp = fopen(_s, O_RDWR); with error checking */
	}
	db->set_errfile(db, _fp);
	*vp = JSVAL_TRUE;
	break;
    case _PAGESIZE:	*vp = _PUT_U(db->set_pagesize(db, _u));		break;
    case _PARTITION_DIRS:	break;	/* XXX FIXME */
    case _PRIORITY:	*vp = _PUT_U(db->set_priority(db, _u));		break;
    case _Q_EXTENTSIZE:	*vp = _PUT_U(db->set_q_extentsize(db, _u));	break;
    case _RE_DELIM:	*vp = _PUT_I(db->set_re_delim(db, _i));		break;
    case _RE_LEN:	*vp = _PUT_U(db->set_re_len(db, _u));	break;
    case _RE_PAD:	*vp = _PUT_I(db->set_re_pad(db, _i));		break;
    case _RE_SOURCE:	*vp = _PUT_S(db->set_re_source(db, _s));	break;

    default:
	break;
    }

    _s = _free(_s);

    return JS_TRUE;
}

static JSBool
rpmdb_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);

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
rpmdb_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static DB *
rpmdb_init(JSContext *cx, JSObject *obj, DB_ENV * dbenv, uint32_t flags)
{
    DB * db = NULL;
    int ret = db_create(&db, dbenv, flags);

    if (ret || db == NULL || !JS_SetPrivate(cx, obj, (void *)db)) {
	if (dbenv)
	    dbenv->err(dbenv, ret, "db_create");
	else
	    fprintf(stderr, "db_create: %s\n", db_strerror(ret));

	if (db)
	    ret = db->close(db, 0);
	db = NULL;
    } else {
	FILE * _errfile = stderr;
	if (dbenv)
	    dbenv->get_errfile(dbenv, &_errfile);
	if (_errfile)
	    db->set_errfile(db, _errfile);
	db->app_private = NULL;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) db %p\n", __FUNCTION__, cx, obj, db);
    return db;
}

static void
rpmdb_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbClass, NULL);
    DB * db = ptr;
    uint32_t _flags = 0;

_DTOR_DEBUG_ENTRY(_debug);

    if (db)
	(void) db->close(db, _flags);
}

static JSBool
rpmdb_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSObject * o = NULL;
    DB_ENV * _dbenv = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/ou", &o, &_flags)))
	goto exit;

    if (OBJ_IS_RPMDBE(cx, o))
	_dbenv = JS_GetInstancePrivate(cx, o, &rpmdbeClass, NULL);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmdb_init(cx, obj, _dbenv, _flags);
    } else {
	if ((obj = JS_NewObject(cx, &rpmdbClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdb_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmdbClass, NULL);
#ifdef	NOTYET
    DB * db = ptr;
    const char *_fn = NULL;
    const char * _con = NULL;
#endif
    JSBool ok = JS_FALSE;

#ifdef	NOTYET
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_fn)))
        goto exit;

    *vp = (db && _fn && (_con = rpmdbLgetfilecon(db, _fn)) != NULL)
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
JSClass rpmdbClass = {
    "Db", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmdb_addprop,   rpmdb_delprop, rpmdb_getprop, rpmdb_setprop,
    (JSEnumerateOp)rpmdb_enumerate, (JSResolveOp)rpmdb_resolve,
    rpmdb_convert,	rpmdb_dtor,

    rpmdb_getobjectops,	rpmdb_checkaccess,
    rpmdb_call,		rpmdb_construct,
    rpmdb_xdrobject,	rpmdb_hasinstance,
    rpmdb_mark,		rpmdb_reserveslots,
};

JSObject *
rpmjs_InitDbClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmdbClass, rpmdb_ctor, 1,
		rpmdb_props, rpmdb_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewDbObject(JSContext *cx, void * _dbenv, uint32_t _flags)
{
    JSObject *obj;
    DB_ENV * dbenv = _dbenv;
    DB * db;

    if ((obj = JS_NewObject(cx, &rpmdbClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((db = rpmdb_init(cx, obj, dbenv, _flags)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmdb, Db, JS_TRUE)
