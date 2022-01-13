/** \ingroup js_c
 * \file js/rpmdbe-js.c
 */

#include "system.h"
#if defined(HAVE_FTOK)
#include <sys/ipc.h>		/* XXX ftok(3) */
#endif

#define	_RPMDB_JS_INTERNAL
#include "rpmdb-js.h"
#include "rpmdbe-js.h"
#include "rpmtxn-js.h"
#include "rpmjs-debug.h"

#include <argv.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmdbe_addprop		JS_PropertyStub
#define	rpmdbe_delprop		JS_PropertyStub
#define	rpmdbe_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmdbe_getobjectops	NULL
#define	rpmdbe_checkaccess	NULL
#define	rpmdbe_call		rpmdbe_call
#define	rpmdbe_construct	rpmdbe_ctor
#define	rpmdbe_xdrobject	NULL
#define	rpmdbe_hasinstance	NULL
#define	rpmdbe_mark		NULL
#define	rpmdbe_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmdbe_equality		NULL
#define rpmdbe_outerobject	NULL
#define rpmdbe_innerobject	NULL
#define rpmdbe_iteratorobject	NULL
#define rpmdbe_wrappedobject	NULL

/* --- helpers */
#define	_TABLE(_v)	{ #_v, DB_EVENT_##_v }
static struct _events_s {
    const char * n;
    uint32_t v;
} _events[] = {
    _TABLE(NO_SUCH_EVENT),	/*  0 */
    _TABLE(PANIC),		/*  1 */
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
    _TABLE(REG_ALIVE),		/*  2 */
    _TABLE(REG_PANIC),		/*  3 */
#else
    _TABLE(NO_SUCH_EVENT),	/*  2 */
    _TABLE(NO_SUCH_EVENT),	/*  3 */
#endif
    _TABLE(REP_CLIENT),		/*  4 */
    _TABLE(REP_ELECTED),	/*  5 */
    _TABLE(REP_MASTER),		/*  6 */
    _TABLE(REP_NEWMASTER),	/*  7 */
    _TABLE(REP_PERM_FAILED),	/*  8 */
    _TABLE(REP_STARTUPDONE),	/*  9 */
    _TABLE(WRITE_FAILED),	/* 10 */
    _TABLE(NO_SUCH_EVENT),	/* 11 */
    _TABLE(NO_SUCH_EVENT),	/* 12 */
    _TABLE(NO_SUCH_EVENT),	/* 13 */
    _TABLE(NO_SUCH_EVENT),	/* 14 */
    _TABLE(NO_SUCH_EVENT),	/* 15 */
};
#undef	_TABLE

static void
rpmdbe_event_notify(DB_ENV * dbenv, u_int32_t event, void * event_info)
{
    JSObject * o = (dbenv ? dbenv->app_private : NULL);
fprintf(stderr, "==> %s(%p, %s(%u), %p) o %p\n", __FUNCTION__, dbenv, _events[event & 0xf].n, event, event_info, o);
}

static void
rpmdbe_feedback(DB_ENV * dbenv, int opcode, int percent)
{
    JSObject * o = (dbenv ? dbenv->app_private : NULL);
fprintf(stderr, "==> %s(%p, %d, %d) o %p\n", __FUNCTION__, dbenv, opcode, percent, o);
}

#define	_TABLE(_v)	{ #_v, DB_TXN_##_v }
static struct _appops_s {
    const char * n;
    uint32_t v;
} _appops[] = {
    _TABLE(ABORT),		/*  0 */
    _TABLE(APPLY),		/*  1 */
    { "UNKNOWN", 2 },
    _TABLE(BACKWARD_ROLL),	/*  3 */
    _TABLE(FORWARD_ROLL),	/*  4 */
    _TABLE(OPENFILES),		/*  5 */
    _TABLE(POPENFILES),		/*  6 */
    _TABLE(PRINT),		/*  7 */
};
#undef	_TABLE

static int
rpmdbe_app_dispatch(DB_ENV * dbenv, DBT * log_rec, DB_LSN * lsn, db_recops op)
{
    JSObject * o = (dbenv ? dbenv->app_private : NULL);
fprintf(stderr, "==> %s(%p, %p, %p, %s(%d)) o %p\n", __FUNCTION__, dbenv, log_rec, lsn, _appops[op & 0x7].n, op, o);
    return 0;
}

static void
rpmdbe_errcall(const DB_ENV * dbenv, const char * errpfx, const char * msg)
{
    JSObject * o = (dbenv ? dbenv->app_private : NULL);
fprintf(stderr, "==> %s(%p, %s, %s) o %p\n", __FUNCTION__, dbenv, errpfx, msg, o);
}

static void
rpmdbe_msgcall(const DB_ENV * dbenv, const char * msg)
{
    JSObject * o = (dbenv ? dbenv->app_private : NULL);
fprintf(stderr, "==> %s(%p, %s) o %p\n", __FUNCTION__, dbenv, msg, o);
}

static int
rpmdbe_isalive(DB_ENV *dbenv, pid_t pid, db_threadid_t tid, u_int32_t flags)
	/*@*/
{
    int alive = 1;	/* assume all processes are alive */

    switch (flags) {
    case 0:
    default:
	/* XXX FIXME: check thread ID's */
	/*@fallthrough@*/
    case DB_MUTEX_PROCESS_ONLY:
	alive = (!(kill(pid, 0) < 0 && errno == ESRCH));
	break;
    }
    return alive;
}

/* --- Object methods */

static JSBool
rpmdbe_CdsgroupBegin(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (dbenv->app_private != NULL) {
	JSObject * _o = NULL;
	DB_TXN * _txn = NULL;
	int ret = dbenv->cdsgroup_begin(dbenv, &_txn);
	switch (ret) {
	default:
	    dbenv->err(dbenv, ret, "DB_ENV->cdsgroup_begin");
	    goto exit;
	    break;
	case 0:
	    if ((_o = JS_NewObject(cx, &rpmtxnClass, NULL, NULL)) == NULL
	     || !JS_SetPrivate(cx, _o, (void *)_txn))
	    {
		if (_txn)       _txn->discard(_txn, 0);
		/* XXX error msg */
		goto exit;
	    }
	    *vp = OBJECT_TO_JSVAL(_o);
	    break;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_Close(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    {	int ret = dbenv->close(dbenv, _flags);
        if (ret)
	    fprintf(stderr, "DB_ENV->close: %s", db_strerror(ret));
        else
            *vp = JSVAL_TRUE;
	dbenv = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, ptr);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_Dbremove(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    const char * _file = NULL;
    const char * _database = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s/s", &_file, &_database)))
	goto exit;

    if (dbenv) {
	DB_TXN * _txnid = NULL;
	uint32_t _flags = 0;
	int ret = dbenv->dbremove(dbenv, _txnid, _file, _database, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->dbremove(%s,%s)", _file, _database);
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_Dbrename(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    const char * _file = NULL;
    const char * _database = NULL;
    const char * _newname = NULL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "sss", &_file, &_database, &_newname)))
	goto exit;

    if (dbenv) {
	DB_TXN * _txnid = NULL;
	uint32_t _flags = 0;
	int ret = dbenv->dbrename(dbenv, _txnid, _file, _database, _newname, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->dbrename(%s,%s,%s)", _file, _database, _newname);
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_Failchk(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_VOID;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->failchk(dbenv, _flags);
	switch (ret) {
	default:
	    dbenv->err(dbenv, ret, "DB_ENV->failchk");
	    break;
	case DB_RUNRECOVERY:
	    *vp = JSVAL_FALSE;
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
rpmdbe_FileidReset(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    const char * _file = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s/u", &_file, &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->fileid_reset(dbenv, _file, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->fileid_reset");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LockDetect(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LockGet(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LockId(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LockIdFree(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LockPut(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LockStat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LockStatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LockVec(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LogArchive(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	const char ** _av = NULL;
	int ret = dbenv->log_archive(dbenv, (char ***) &_av, _flags);

	switch (ret) {
	default:
	    dbenv->err(dbenv, ret, "DB_ENV->log_archive");
	    break;
	case DB_RUNRECOVERY:
	    *vp = JSVAL_FALSE;
	    break;
	case 0:
argvPrint("log_archive", _av, NULL);
	    if (_av == NULL) {
		*vp = JSVAL_NULL;
	    } else {
		int _ac = argvCount(_av);
		JSObject * o = JS_NewArrayObject(cx, 0, NULL);
		int i;
		*vp = OBJECT_TO_JSVAL(o);
		for (i = 0; i < _ac; i++) {
		    jsval v = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _av[i]));
		    (void) JS_SetElement(cx, o, i, &v);
		}
	    }
	    break;
	}
    }

    ok = JS_TRUE;

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LogCursor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LogFile(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LogFlush(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LogPrintf(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LogPut(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LogStat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LogStatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_LsnReset(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    const char * _file = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s/u", &_file, &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->lsn_reset(dbenv, _file, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->lsn_reset");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MempFcreate(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MempRegister(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MempStat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	DB_MPOOL_STAT ** _gsp = NULL;	/* XXX todo++ */
#ifdef	NOTYET
	DB_MPOOL_FSTAT *(*_fsp)[] = NULL;
#endif

	int ret = dbenv->memp_stat(dbenv, _gsp, NULL, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->memp_stat");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MempStatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->memp_stat_print(dbenv, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->memp_stat_print");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MempSync(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (dbenv->app_private != NULL) {
	DB_LSN * _lsn = NULL;
	int ret = dbenv->memp_sync(dbenv, _lsn);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->memp_sync");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MempTrickle(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    int _percent = 20;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_VOID;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/i", &_percent)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int nwrote = 0;
	int ret = dbenv->memp_trickle(dbenv, _percent, &nwrote);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->memp_sync");
	else
	    *vp = INT_TO_JSVAL(nwrote);
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MutexAlloc(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MutexFree(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MutexLock(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MutexStat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	DB_MUTEX_STAT * _stat = NULL;
	int ret = dbenv->mutex_stat(dbenv, &_stat, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->mutex_stat");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MutexStatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->mutex_stat_print(dbenv, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->mutex_stat_print");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_MutexUnlock(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_Open(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    const char * _home = NULL;
    uint32_t _eflags = 0;
    int _mode = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "su/i", &_home, &_eflags, &_mode)))
	goto exit;

    if (dbenv->app_private == NULL) {
	int ret = dbenv->open(dbenv, _home, _eflags, _mode);

	if (ret) {
	    dbenv->err(dbenv, ret, "DB_ENV->open: %s", _home);
	    goto exit;
	} else {
	    dbenv->app_private = obj;
	    ret = dbenv->set_event_notify(dbenv, rpmdbe_event_notify);
	    if (ret) dbenv->err(dbenv, ret, "DB_ENV->set_event_notify");
	    /* XXX only DB_RECOVER is currently implemented */
	    ret = dbenv->set_feedback(dbenv, rpmdbe_feedback);
	    if (ret) dbenv->err(dbenv, ret, "DB_ENV->set_feedback");
	    *vp = JSVAL_TRUE;
	}
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_Remove(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    const char * _home = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s/u", &_home, &_flags)))
	goto exit;

    if (dbenv->app_private == NULL) {
	int ret = dbenv->remove(dbenv, _home, _flags);
	if (ret) {
	    dbenv->err(dbenv, ret, "DB_ENV->remove: %s", _home);
	    goto exit;
	} else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepElect(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _nsites = 0;
    uint32_t _nvotes = 0;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/uu", &_nsites, &_nvotes)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->rep_elect(dbenv, _nsites, _nvotes, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->rep_elect");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepProcessMessage(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepStart(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepStat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	DB_REP_STAT * _stat = NULL;
	int ret = dbenv->rep_stat(dbenv, &_stat, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->rep_stat");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepStatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->rep_stat_print(dbenv, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->rep_stat_print");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepSync(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->rep_sync(dbenv, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->rep_sync");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepmgrStart(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

	/* FIXME todo++ */

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepmgrStat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	DB_REPMGR_STAT * _stat = NULL;
	int ret = dbenv->repmgr_stat(dbenv, &_stat, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->repmgr_stat");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_RepmgrStatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->repmgr_stat_print(dbenv, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->repmgr_stat_print");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_StatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->stat_print(dbenv, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->stat_print");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

#define OBJ_IS_RPMTXN(_cx, _o)  (JS_GET_CLASS(_cx, _o) == &rpmtxnClass)

static JSBool
rpmdbe_TxnBegin(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSObject * o = NULL;
    DB_TXN * _parent = NULL;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &o, &_flags)))
	goto exit;

    if (o && OBJ_IS_RPMTXN(cx, o))
	_parent = JS_GetInstancePrivate(cx, o, &rpmtxnClass, NULL);

    if (dbenv->app_private != NULL) {
	DB_TXN * _txn = NULL;
	int ret = dbenv->txn_begin(dbenv, _parent, &_txn, _flags);
	switch (ret) {
	default:
	    dbenv->err(dbenv, ret, "DB_ENV->txn_begin");
	    goto exit;
	    break;
	case 0:
	    if ((o = JS_NewObject(cx, &rpmtxnClass, NULL, NULL)) == NULL
	     || !JS_SetPrivate(cx, o, (void *)_txn))
	    {
		if (_txn)       _txn->discard(_txn, 0);
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
rpmdbe_TxnCheckpoint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _kb = 0;
    uint32_t _minutes = 0;
    uint32_t _flags = 0;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/uuu", &_kb, &_minutes, &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->txn_checkpoint(dbenv, _kb, _minutes, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->txn_checkpoint");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_TxnRecover(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

#ifdef	NOTYET
    if (dbenv->app_private != NULL) {
	DB_PREPLIST _preplist[32];
	long _count = (sizeof(_preplist) / sizeof(_preplist[0]));
	long _got = 0;
	uint32_t _flags = DB_FIRST;
	int ret = 0;
	int i;

	while (1) {
	    ret = dbenv->txn_recover(dbenv, _preplist, _count, &_count, _flags);
	    if (ret) {
		dbenv->err(dbenv, ret, "DB_ENV->txn_recover");
		goto exit;
	    }
	    if (_got == 0)
		break;
	    for (i = 0; i < _got; i++) {
		DB_TXN * _txn = _preplist[i].txn;
		uint32_t _tflags = 0;
		(void) _txn->discard(_txn, _tflags);
	    }
	    _flags = DB_NEXT;
	}
	*vp = JSVAL_TRUE;
    }
#endif

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_TxnStat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	DB_TXN_STAT * _stat = NULL;
	int ret = dbenv->txn_stat(dbenv, &_stat, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->txn_stat");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_TxnStatPrint(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = DB_STAT_ALL;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (dbenv == NULL) goto exit;
    *vp = JSVAL_FALSE;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_flags)))
	goto exit;

    if (dbenv->app_private != NULL) {
	int ret = dbenv->txn_stat_print(dbenv, _flags);
	if (ret)
	    dbenv->err(dbenv, ret, "DB_ENV->txn_stat_print");
	else
	    *vp = JSVAL_TRUE;
    }

    ok = JS_TRUE;

exit:
    return ok;
}

static JSFunctionSpec rpmdbe_funcs[] = {
    JS_FS("cdsgroup_begin",	rpmdbe_CdsgroupBegin,		0,0),
    JS_FS("close",		rpmdbe_Close,			0,0),
    JS_FS("dbremove",		rpmdbe_Dbremove,		0,0),
    JS_FS("dbrename",		rpmdbe_Dbrename,		0,0),
    JS_FS("failchk",		rpmdbe_Failchk,			0,0),
    JS_FS("fileid_reset",	rpmdbe_FileidReset,		0,0),
    JS_FS("lock_detect",	rpmdbe_LockDetect,		0,0),
    JS_FS("lock_get",		rpmdbe_LockGet,			0,0),
    JS_FS("lock_id",		rpmdbe_LockId,			0,0),
    JS_FS("lock_id_free",	rpmdbe_LockIdFree,		0,0),
    JS_FS("lock_put",		rpmdbe_LockPut,			0,0),
    JS_FS("lock_stat",		rpmdbe_LockStat,		0,0),
    JS_FS("lock_stat_print",	rpmdbe_LockStatPrint,		0,0),
    JS_FS("lock_vec",		rpmdbe_LockVec,			0,0),
    JS_FS("log_archive",	rpmdbe_LogArchive,		0,0),
    JS_FS("log_cursor",		rpmdbe_LogCursor,		0,0),
    JS_FS("log_file",		rpmdbe_LogFile,			0,0),
    JS_FS("log_flush",		rpmdbe_LogFlush,		0,0),
    JS_FS("log_printf",		rpmdbe_LogPrintf,		0,0),
    JS_FS("log_put",		rpmdbe_LogPut,			0,0),
    JS_FS("log_stat",		rpmdbe_LogStat,			0,0),
    JS_FS("log_stat_print",	rpmdbe_LogStatPrint,		0,0),
    JS_FS("lsn_reset",		rpmdbe_LsnReset,		0,0),
    JS_FS("memp_fcreate",	rpmdbe_MempFcreate,		0,0),
    JS_FS("memp_register",	rpmdbe_MempRegister,		0,0),
    JS_FS("memp_stat",		rpmdbe_MempStat,		0,0),
    JS_FS("memp_stat_print",	rpmdbe_MempStatPrint,		0,0),
    JS_FS("memp_sync",		rpmdbe_MempSync,		0,0),
    JS_FS("memp_trickle",	rpmdbe_MempTrickle,		0,0),
    JS_FS("mutex_alloc",	rpmdbe_MutexAlloc,		0,0),
    JS_FS("mutex_free",		rpmdbe_MutexFree,		0,0),
    JS_FS("mutex_lock",		rpmdbe_MutexLock,		0,0),
    JS_FS("mutex_stat",		rpmdbe_MutexStat,		0,0),
    JS_FS("mutex_stat_print",	rpmdbe_MutexStatPrint,		0,0),
    JS_FS("mutex_unlock",	rpmdbe_MutexUnlock,		0,0),
    JS_FS("open",		rpmdbe_Open,			0,0),
    JS_FS("remove",		rpmdbe_Remove,			0,0),
    JS_FS("rep_elect",		rpmdbe_RepElect,		0,0),
    JS_FS("rep_process_message",rpmdbe_RepProcessMessage,	0,0),
    JS_FS("rep_start",		rpmdbe_RepStart,		0,0),
    JS_FS("rep_stat",		rpmdbe_RepStat,			0,0),
    JS_FS("rep_stat_print",	rpmdbe_RepStatPrint,		0,0),
    JS_FS("rep_sync",		rpmdbe_RepSync,			0,0),
    JS_FS("repmgr_start",	rpmdbe_RepmgrStart,		0,0),
    JS_FS("repmgr_stat",	rpmdbe_RepmgrStat,		0,0),
    JS_FS("repmgr_stat_print",	rpmdbe_RepmgrStatPrint,		0,0),
    JS_FS("stat_print",		rpmdbe_StatPrint,		0,0),
    JS_FS("txn_begin",		rpmdbe_TxnBegin,		0,0),
    JS_FS("txn_checkpoint",	rpmdbe_TxnCheckpoint,		0,0),
    JS_FS("txn_recover",	rpmdbe_TxnRecover,		0,0),
    JS_FS("txn_stat",		rpmdbe_TxnStat,			0,0),
    JS_FS("txn_stat_print",	rpmdbe_TxnStatPrint,		0,0),
    JS_FS_END
};

/* --- Object properties */

#define	_TABLE(_v)	#_v, _##_v, JSPROP_ENUMERATE, NULL, NULL

enum rpmdbe_tinyid {
    _DEBUG		= -2,
    _HOME		= -3,
    _OPEN_FLAGS		= -4,
    _DATADIRS		= -5,
    _CREATE_DIR		= -6,
    _ENCRYPT		= -7,
    _ERRFILE		= -8,
    _ERRPFX		= -9,
    _FLAGS		= -10,
    _IDIRMODE		= -11,
    _MSGFILE		= -12,
    _SHMKEY		= -13,
    _THREADCNT		= -14,

    _DB_SET_LOCK_TIMEOUT	= -15,

    _TMPDIR		= -16,
    _VERBOSE		= -17,
    _LKCONFLICTS	= -18,
    _LKDETECT		= -19,
    _LKMAXLOCKERS	= -20,
    _LKMAXLOCKS		= -21,
    _LKMAXOBJS		= -22,
    _LKPARTITIONS	= -23,

    _LOGDIRECT		= -24,
    _LOGDSYNC		= -25,
    _LOGAUTORM		= -26,
    _LOGINMEM		= -27,
    _LOGZERO		= -28,

    _LGBSIZE		= -29,
    _LGDIR		= -30,
    _LGFILEMODE		= -31,
    _LGMAX		= -32,
    _LGREGIONMAX	= -33,

    _DB_SET_TXN_TIMEOUT		= -34,

    _VERSION		= -35,
    _MAJOR		= -36,
    _MINOR		= -37,
    _PATCH		= -38,

    _CACHESIZE		= -39,
    _NCACHES		= -40,
    _REGTIMEOUT		= -41,
    _CACHEMAX		= -42,
    _MAXOPENFD		= -43,
    _MMAPSIZE		= -44,

    _MXALIGN		= -45,
    _MXINC		= -46,
    _MXMAX		= -47,
    _MXSPINS		= -48,

    _TXMAX		= -49,
    _TXTSTAMP		= -50,

    _DB_REP_CONF_BULK		= -51,
    _DB_REP_CONF_DELAYCLIENT	= -52,
    _DB_REP_CONF_INMEM		= -53,
    _DB_REP_CONF_LEASE		= -54,
    _DB_REP_CONF_NOAUTOINIT	= -55,
    _DB_REP_CONF_NOWAIT		= -56,
    _DB_REPMGR_CONF_2SITE_STRICT= -57,

    _REPLIMIT		= -58,
    _REPNSITES		= -59,
    _REPPRIORITY	= -60,

    _DB_REP_ACK_TIMEOUT		= -61,
    _DB_REP_CHECKPOINT_DELAY	= -62,
    _DB_REP_CONNECTION_RETRY	= -63,
    _DB_REP_ELECTION_TIMEOUT	= -64,
    _DB_REP_ELECTION_RETRY	= -65,
    _DB_REP_FULL_ELECTION_TIMEOUT = -66,
    _DB_REP_HEARTBEAT_MONITOR	= -67,
    _DB_REP_HEARTBEAT_SEND	= -68,
    _DB_REP_LEASE_TIMEOUT	= -69,
};

static JSPropertySpec rpmdbe_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},

    {"version",	_VERSION,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"major",	_MAJOR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"minor",	_MINOR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"patch",	_PATCH,		JSPROP_ENUMERATE,	NULL,	NULL},

    {"home",	_HOME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"open_flags", _OPEN_FLAGS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"data_dirs", _DATADIRS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"create_dir", _CREATE_DIR,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"encrypt",	_ENCRYPT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"errfile",	_ERRFILE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"errpfx",	_ERRPFX,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"flags",	_FLAGS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"idirmode", _IDIRMODE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"msgfile", _MSGFILE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"shm_key", _SHMKEY,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"thread_count", _THREADCNT, JSPROP_ENUMERATE,	NULL,	NULL},

    {"tmp_dir", _TMPDIR,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"verbose", _VERBOSE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"lk_conflicts", _LKCONFLICTS, JSPROP_ENUMERATE,	NULL,	NULL},
    {"lk_detect", _LKDETECT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"lk_max_lockers", _LKMAXLOCKERS, JSPROP_ENUMERATE,	NULL,	NULL},
    {"lk_max_locks", _LKMAXLOCKS, JSPROP_ENUMERATE,	NULL,	NULL},
    {"lk_max_objects", _LKMAXOBJS, JSPROP_ENUMERATE,	NULL,	NULL},
    {"lk_partitions", _LKPARTITIONS, JSPROP_ENUMERATE,	NULL,	NULL},

    {"log_direct", _LOGDIRECT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"log_dsync", _LOGDSYNC,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"log_autorm", _LOGAUTORM,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"log_inmemory", _LOGINMEM, JSPROP_ENUMERATE,	NULL,	NULL},
    {"log_zero", _LOGZERO,	JSPROP_ENUMERATE,	NULL,	NULL},

    {"lg_bsize", _LGBSIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"lg_dir", _LGDIR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"lg_filemode", _LGFILEMODE, JSPROP_ENUMERATE,	NULL,	NULL},
    {"lg_max", _LGMAX,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"lg_regionmax", _LGREGIONMAX, JSPROP_ENUMERATE,	NULL,	NULL},

    { _TABLE(DB_SET_LOCK_TIMEOUT) },
    { _TABLE(DB_SET_TXN_TIMEOUT) },
    {"reg_timeout", _REGTIMEOUT, JSPROP_ENUMERATE,	NULL,	NULL},

    {"cachemax", _CACHEMAX,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"cachesize", _CACHESIZE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"ncaches",	_NCACHES,	JSPROP_ENUMERATE,	NULL,	NULL},

    {"max_openfd", _MAXOPENFD,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"mmapsize", _MMAPSIZE,	JSPROP_ENUMERATE,	NULL,	NULL},

    {"mutex_align", _MXALIGN,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"mutex_inc", _MXINC,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"mutex_max", _MXMAX,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"mutex_spins", _MXSPINS,	JSPROP_ENUMERATE,	NULL,	NULL},

    {"tx_max",	_TXMAX,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"tx_timestamp", _TXTSTAMP,	JSPROP_ENUMERATE,	NULL,	NULL},

    { _TABLE(DB_REP_CONF_BULK) },
    { _TABLE(DB_REP_CONF_DELAYCLIENT) },
    { _TABLE(DB_REP_CONF_INMEM) },
    { _TABLE(DB_REP_CONF_LEASE) },
    { _TABLE(DB_REP_CONF_NOAUTOINIT) },
    { _TABLE(DB_REP_CONF_NOWAIT) },
    { _TABLE(DB_REPMGR_CONF_2SITE_STRICT) },

    {"rep_limit", _REPLIMIT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"rep_nsites", _REPNSITES,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"rep_priority", _REPPRIORITY, JSPROP_ENUMERATE,	NULL,	NULL},

    { _TABLE(DB_REP_ACK_TIMEOUT) },
    { _TABLE(DB_REP_CHECKPOINT_DELAY) },
    { _TABLE(DB_REP_CONNECTION_RETRY) },
    { _TABLE(DB_REP_ELECTION_TIMEOUT) },
    { _TABLE(DB_REP_ELECTION_RETRY) },
    { _TABLE(DB_REP_FULL_ELECTION_TIMEOUT) },
    { _TABLE(DB_REP_HEARTBEAT_MONITOR) },
    { _TABLE(DB_REP_HEARTBEAT_SEND) },
    { _TABLE(DB_REP_LEASE_TIMEOUT) },

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
rpmdbe_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    const char ** _av = NULL;
    int _ac = 0;
    const char * _s = NULL;
    uint32_t _gb = 0;
    uint32_t _u = 0;
    int _i = 0;
    long _l = 0;
    FILE * _fp = NULL;
    jsint tiny = JSVAL_TO_INT(id);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	break;

    case _VERSION:	*vp = _RET_S(DB_VERSION_STRING);		break;
    case _MAJOR:	*vp = INT_TO_JSVAL(DB_VERSION_MAJOR);		break;
    case _MINOR:	*vp = INT_TO_JSVAL(DB_VERSION_MINOR);		break;
    case _PATCH:	*vp = INT_TO_JSVAL(DB_VERSION_PATCH);		break;

    case _HOME:		*vp = _GET_S(!dbenv->get_home(dbenv, &_s));	break;
    case _OPEN_FLAGS:	*vp = _GET_U(!dbenv->get_open_flags(dbenv, &_u)); break;
    case _DATADIRS:
	if (!dbenv->get_data_dirs(dbenv, &_av)
	 && (_ac = argvCount(_av)) > 0)
	{
	    JSObject * o = JS_NewArrayObject(cx, 0, NULL);
	    int i;

	    *vp = OBJECT_TO_JSVAL(o);
	    for (i = 0; i < _ac; i++) {
		jsval v = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, _av[i]));
		(void) JS_SetElement(cx, o, i, &v);
	    }
	} else
	    *vp = JSVAL_VOID;
	break;
    case _CREATE_DIR:
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
	*vp = _GET_S(!dbenv->get_create_dir(dbenv, &_s));
#else
	*vp = JSVAL_VOID;
#endif
	break;
    case _ENCRYPT:	*vp = _GET_U(!dbenv->get_encrypt_flags(dbenv, &_u)); break;
    case _ERRFILE:
	dbenv->get_errfile(dbenv, &_fp);
	_s = (_fp ? "other" : NULL);
	if (_fp == stdin)	_s = "stdin";
	if (_fp == stdout)	_s = "stdout";
	if (_fp == stderr)	_s = "stderr";
	*vp = _RET_S(_s);
	break;
    case _ERRPFX:
	dbenv->get_errpfx(dbenv, &_s);
	*vp = _RET_S(_s);
	break;
    case _FLAGS:	*vp = _GET_U(!dbenv->get_flags(dbenv, &_u)); break;
    case _IDIRMODE:	*vp = _GET_S(!dbenv->get_intermediate_dir_mode(dbenv, &_s)); break;
    case _MSGFILE:
	dbenv->get_msgfile(dbenv, &_fp);
	_s = (_fp ? "other" : NULL);
	if (_fp == stdin)	_s = "stdin";
	if (_fp == stdout)	_s = "stdout";
	if (_fp == stderr)	_s = "stderr";
	*vp = _RET_S(_s);
	break;
    case _SHMKEY:
	if (!dbenv->get_shm_key(dbenv, &_l)) {
	    jsdouble d = _l;
            if (!JS_NewNumberValue(cx, d, vp))
                *vp = JSVAL_VOID;
	} else
	    *vp = JSVAL_VOID;
	break;
    case _THREADCNT:	*vp = _GET_U(!dbenv->get_thread_count(dbenv, &_u)); break;

	/* XXX FIXME assumes typedef uint32_t db_timeout_t; */
#define	_JUMP(_v, _lbl)	_##_v:	_u = _v;	goto _lbl
    case _JUMP(DB_SET_LOCK_TIMEOUT,		_get_timeout);
    case _JUMP(DB_SET_TXN_TIMEOUT,		_get_timeout);
#undef	_JUMP
    _get_timeout:
	*vp = _GET_U(!dbenv->get_timeout(dbenv, (db_timeout_t *)&_u, _u));
	break;

    case _REGTIMEOUT:	*vp = JSVAL_NULL;	break;	/* XXX FIXME */

    case _TMPDIR:	*vp = _GET_S(!dbenv->get_tmp_dir(dbenv, &_s));	break;
    case _VERBOSE:	*vp = JSVAL_VOID;	break;	/* XXX FIXME enum */
    case _LKCONFLICTS:	*vp = JSVAL_VOID;	break;	/* XXX FIXME */
    case _LKDETECT:	*vp = _GET_U(!dbenv->get_lk_detect(dbenv, &_u)); break;
    case _LKMAXLOCKERS:	*vp = _GET_U(!dbenv->get_lk_max_lockers(dbenv, &_u)); break;
    case _LKMAXLOCKS:	*vp = _GET_U(!dbenv->get_lk_max_locks(dbenv, &_u)); break;
    case _LKMAXOBJS:	*vp = _GET_U(!dbenv->get_lk_max_objects(dbenv, &_u)); break;
    case _LKPARTITIONS:	*vp = _GET_U(!dbenv->get_lk_partitions(dbenv, &_u)); break;

    case _LOGDIRECT:	*vp = _GET_B(!dbenv->log_get_config(dbenv, DB_LOG_DIRECT, &_i)); break;
    case _LOGDSYNC:	*vp = _GET_B(!dbenv->log_get_config(dbenv, DB_LOG_DSYNC, &_i)); break;
    case _LOGAUTORM:	*vp = _GET_B(!dbenv->log_get_config(dbenv, DB_LOG_AUTO_REMOVE, &_i)); break;
    case _LOGINMEM:	*vp = _GET_B(!dbenv->log_get_config(dbenv, DB_LOG_IN_MEMORY, &_i)); break;
    case _LOGZERO:	*vp = _GET_B(!dbenv->log_get_config(dbenv, DB_LOG_ZERO, &_i)); break;

    case _LGBSIZE:	*vp = _GET_U(!dbenv->get_lg_bsize(dbenv, &_u));	break;
    case _LGDIR:	*vp = _GET_S(!dbenv->get_lg_dir(dbenv, &_s)); break;
    case _LGFILEMODE:	*vp = _GET_I(!dbenv->get_lg_filemode(dbenv, &_i)); break;
    case _LGMAX:	*vp = _GET_U(!dbenv->get_lg_max(dbenv, &_u));	break;
    case _LGREGIONMAX:	*vp = _GET_U(!dbenv->get_lg_regionmax(dbenv, &_u)); break;

	/* XXX FIXME: return uint64_t */
    case _CACHEMAX:	*vp = _GET_U(!dbenv->get_cache_max(dbenv, &_gb, &_u)); break;
    case _CACHESIZE:	*vp = _GET_U(!dbenv->get_cachesize(dbenv, &_gb, &_u, &_i)); break;
    case _NCACHES:	*vp = _GET_I(!dbenv->get_cachesize(dbenv, &_gb, &_u, &_i)); break;
    case _MAXOPENFD:	*vp = _GET_I(!dbenv->get_mp_max_openfd(dbenv, &_i)); break;
	/* XXX FIXME dbenv->get_mp_max_write(dbenv, &maxwrite, &timeout); */
    case _MMAPSIZE:
    {	size_t _sz = 0;
	*vp = (!dbenv->get_mp_mmapsize(dbenv, &_sz)
		? INT_TO_JSVAL((int)_sz) : JSVAL_VOID);
    }	break;
    case _MXALIGN:	*vp = _GET_U(!dbenv->mutex_get_align(dbenv, &_u)); break;
    case _MXINC:	*vp = _GET_U(!dbenv->mutex_get_increment(dbenv, &_u)); break;
    case _MXMAX:	*vp = _GET_U(!dbenv->mutex_get_increment(dbenv, &_u)); break;
    case _MXSPINS:	*vp = _GET_U(!dbenv->mutex_get_tas_spins(dbenv, &_u)); break;
	/* XXX FIXME: dbenv->rep_get_clockskew(dbenv, u &fast, u &slow) */
#define	_JUMP(_v, _lbl)	_##_v:	_i = _v;	goto _lbl
    case _JUMP(DB_REP_CONF_BULK,		_get_config);
    case _JUMP(DB_REP_CONF_DELAYCLIENT,		_get_config);
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
    case _JUMP(DB_REP_CONF_INMEM,		_get_config);
#endif
    case _JUMP(DB_REP_CONF_LEASE,		_get_config);
#if defined(DB_REP_CONF_NOAUTOINIT)	/* XXX db version? */
    case _JUMP(DB_REP_CONF_NOAUTOINIT,		_get_config);
#endif
    case _JUMP(DB_REP_CONF_NOWAIT,		_get_config);
    case _JUMP(DB_REPMGR_CONF_2SITE_STRICT,	_get_config);
#undef	_JUMP
    _get_config:
	*vp = _GET_B(!dbenv->rep_get_config(dbenv, _i, &_i));
	break;
    case _REPLIMIT:	_GET_U(!dbenv->rep_get_limit(dbenv, &_gb, &_u)); break;
    case _REPNSITES:	_GET_U(!dbenv->rep_get_nsites(dbenv, &_u)); break;
    case _REPPRIORITY:	_GET_U(!dbenv->rep_get_priority(dbenv, &_u)); break;
	/* XXX FIXME: dbenv->rep_get_request(dbenv, u &min, u &max) */

#define	_JUMP(_v, _lbl)	_##_v:	_i = _v;	goto _lbl
    case _JUMP(DB_REP_ACK_TIMEOUT,		_rep_get_timeout);
    case _JUMP(DB_REP_CHECKPOINT_DELAY,		_rep_get_timeout);
    case _JUMP(DB_REP_CONNECTION_RETRY,		_rep_get_timeout);
    case _JUMP(DB_REP_ELECTION_TIMEOUT,		_rep_get_timeout);
    case _JUMP(DB_REP_ELECTION_RETRY,		_rep_get_timeout);
    case _JUMP(DB_REP_FULL_ELECTION_TIMEOUT,	_rep_get_timeout);
    case _JUMP(DB_REP_HEARTBEAT_MONITOR,	_rep_get_timeout);
    case _JUMP(DB_REP_HEARTBEAT_SEND,		_rep_get_timeout);
    case _JUMP(DB_REP_LEASE_TIMEOUT,		_rep_get_timeout);
#undef	_JUMP
    _rep_get_timeout:
	*vp = _GET_U(!dbenv->rep_get_timeout(dbenv, _i, &_u));
	break;

	/* XXX FIXME: dbenv->repmgr_get_ack_policy(dbenv, &_i) */

    case _TXMAX:	*vp = _GET_U(!dbenv->get_tx_max(dbenv, &_u)); break;
    case _TXTSTAMP:
    {	time_t tstamp = 0;
	if (!dbenv->get_tx_timestamp(dbenv, &tstamp)) {
	    _u = tstamp;
	    *vp = INT_TO_JSVAL(_u);	/* XXX FIXME */
	} else
	    *vp = JSVAL_VOID;
    }	break;

    default:
	break;
    }

    return JS_TRUE;
}

#define	_PUT_S(_put)	(JSVAL_IS_STRING(*vp) && !(_put) \
	? JSVAL_TRUE : JSVAL_FALSE)
#define	_PUT_U(_put)	(JSVAL_IS_INT(*vp) && !(_put) \
	? JSVAL_TRUE : JSVAL_FALSE)
#define	_PUT_I(_put)	(JSVAL_IS_INT(*vp) && !(_put) \
	? JSVAL_TRUE : JSVAL_FALSE)

static JSBool
rpmdbe_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
#ifdef	NOTYET
    const char ** _av = NULL;
    int _ac = 0;
#endif
    const char * _s = NULL;
    uint32_t _gb = 0;
    uint32_t _b = 0;
    uint32_t _u = 0;
    int _i = 0;
    int _nc = 0;
    long _l = 0;
    FILE * _fp = NULL;
    jsint tiny = JSVAL_TO_INT(id);
    jsdouble d;

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    if (JSVAL_IS_STRING(*vp))
	_s = JS_EncodeString(cx, JS_ValueToString(cx, *vp));
    if (JSVAL_IS_NUMBER(*vp)) {
	(void) JS_ValueToNumber(cx, *vp, &d);
	_u = _i = _l = d;
    }

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
    /* dbenv->add_data_dir() */
    case _DATADIRS:
	/* XXX duplicates? */
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
	*vp = _PUT_S(dbenv->add_data_dir(dbenv, _s));
#else
	*vp = JSVAL_VOID;
#endif
	break;
    case _CREATE_DIR:
	/* XXX check datadirs to prevent failure? */
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
	*vp = _PUT_S(dbenv->set_create_dir(dbenv, _s));
#else
	*vp = JSVAL_VOID;
#endif
	break;
    case _ENCRYPT:
	*vp = _PUT_S(dbenv->set_encrypt(dbenv, _s, DB_ENCRYPT_AES));
	break;
    case _ERRFILE:
	/* XXX FIXME: cleaner typing */
	_fp = NULL;
	if (!strcmp(_s, "stdout")) _fp = stdout;
	if (!strcmp(_s, "stderr")) _fp = stderr;
	dbenv->set_errfile(dbenv, _fp);
	*vp = JSVAL_TRUE;
	break;
    case _ERRPFX:
	dbenv->set_errpfx(dbenv, _s);
	*vp = JSVAL_TRUE;
	break;
    case _FLAGS:	break;	/* XXX FIXME */
    case _IDIRMODE:	*vp = _PUT_S(dbenv->set_intermediate_dir_mode(dbenv, _s));	break;
    case _MSGFILE:
	/* XXX FIXME: cleaner typing */
	_fp = NULL;
	if (_s != NULL) {
	    if (!strcmp(_s, "stdout")) _fp = stdout;
	    else if (!strcmp(_s, "stderr")) _fp = stderr;
	    /* XXX FIXME: fp = fopen(_s, O_RDWR); with error checking */
	}
	dbenv->set_errfile(dbenv, _fp);
	*vp = JSVAL_TRUE;
	break;
    case _SHMKEY:
#if defined(HAVE_FTOK)
	if (JSVAL_IS_STRING(*vp) && _s != NULL)
	    _l = ftok(_s, 0);
#endif
	
	*vp = (!dbenv->set_shm_key(dbenv, _l) ? JSVAL_TRUE : JSVAL_FALSE);
	break;
    case _THREADCNT:
	if (JSVAL_IS_INT(*vp) && _u >= 8 && !dbenv->set_thread_count(dbenv, _u))
	{
	    int ret = dbenv->set_isalive(dbenv, (_u ? rpmdbe_isalive : NULL));
	    *vp = (!ret ? JSVAL_TRUE : JSVAL_FALSE);
	} else
	    *vp = JSVAL_FALSE;
	break;

#define	_JUMP(_v, _lbl)	_##_v:	_nc = _v;	goto _lbl
    case _JUMP(DB_SET_LOCK_TIMEOUT,		_set_timeout);
    case _JUMP(DB_SET_TXN_TIMEOUT,		_set_timeout);
#undef	_JUMP
    _set_timeout:
	*vp = _PUT_U(dbenv->set_timeout(dbenv, (db_timeout_t)_u, _nc));
	break;
    case _REGTIMEOUT:
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
	*vp = _PUT_U(dbenv->set_timeout(dbenv, (db_timeout_t)_u, DB_SET_REG_TIMEOUT));
#endif
	break;

    case _TMPDIR:	*vp = _PUT_S(dbenv->set_tmp_dir(dbenv, _s));	break;
    case _VERBOSE:	break;	/* XXX FIXME */
    case _LKCONFLICTS:	break;	/* XXX FIXME */
	/* XXX FIXME: explicit enum check needed */
    case _LKDETECT:	*vp = _PUT_U(dbenv->set_lk_detect(dbenv, _u)); break;
    case _LKMAXLOCKERS:	*vp = _PUT_U(dbenv->set_lk_max_lockers(dbenv, _u)); break;
    case _LKMAXLOCKS:	*vp = _PUT_U(dbenv->set_lk_max_locks(dbenv, _u)); break;
    case _LKMAXOBJS:	*vp = _PUT_U(dbenv->set_lk_max_objects(dbenv, _u)); break;
    case _LKPARTITIONS:	*vp = _PUT_U(dbenv->set_lk_partitions(dbenv, _u)); break;

    case _LOGDIRECT:	*vp = _PUT_I(dbenv->log_set_config(dbenv, DB_LOG_DIRECT, (_i ? 1 : 0)));	break;
    case _LOGDSYNC:	*vp = _PUT_I(dbenv->log_set_config(dbenv, DB_LOG_DSYNC, (_i ? 1 : 0)));	break;
    case _LOGAUTORM:	*vp = _PUT_I(dbenv->log_set_config(dbenv, DB_LOG_AUTO_REMOVE, (_i ? 1 : 0)));	break;
    case _LOGINMEM:	*vp = _PUT_I(dbenv->log_set_config(dbenv, DB_LOG_IN_MEMORY, (_i ? 1 : 0)));	break;
    case _LOGZERO:	*vp = _PUT_I(dbenv->log_set_config(dbenv, DB_LOG_ZERO, (_i ? 1 : 0)));	break;


    case _LGBSIZE:	*vp = _PUT_U(dbenv->set_lg_bsize(dbenv, _u));	break;
    case _LGDIR:	*vp = _PUT_S(dbenv->set_lg_dir(dbenv, _s));	break;
    case _LGFILEMODE:	*vp = _PUT_I(dbenv->set_lg_filemode(dbenv, _i)); break;
    case _LGMAX:	*vp = _PUT_U(dbenv->set_lg_max(dbenv, _u));	break;
    case _LGREGIONMAX:	*vp = _PUT_U(dbenv->set_lg_regionmax(dbenv, _u)); break;

    case _CACHEMAX:
	if (!dbenv->get_cache_max(dbenv, &_gb, &_b)) {
	    _b = _u;
	    *vp = !dbenv->set_cache_max(dbenv, _gb, _b)
		? JSVAL_TRUE : JSVAL_FALSE;
	} else
	    *vp = JSVAL_FALSE;
	break;
    case _CACHESIZE:
	if (!dbenv->get_cachesize(dbenv, &_gb, &_b, &_nc)) {
	    _b = _u;
	    *vp = !dbenv->set_cachesize(dbenv, _gb, _b, _nc)
		? JSVAL_TRUE : JSVAL_FALSE;
	} else
	    *vp = JSVAL_FALSE;
	break;
    case _NCACHES:
	if (!dbenv->get_cachesize(dbenv, &_gb, &_b, &_nc)) {
	    _nc = _i;
	    *vp = !dbenv->set_cachesize(dbenv, _gb, _b, _nc)
		? JSVAL_TRUE : JSVAL_FALSE;
	} else
	    *vp = JSVAL_FALSE;
	break;
    case _MAXOPENFD: 	*vp = _PUT_I(!dbenv->set_mp_max_openfd(dbenv, _i)); break;
	/* XXX FIXME dbenv->set_mp_max_write(dbenv, maxwrite, timeout); */
    case _MMAPSIZE:	*vp = _PUT_U(dbenv->set_mp_mmapsize(dbenv, _u)); break;
    case _MXALIGN:	break;	/* XXX FIXME */
    case _MXINC:	break;	/* XXX FIXME */
    case _MXMAX:	break;	/* XXX FIXME */
    case _MXSPINS:	break;	/* XXX FIXME */
	/* XXX FIXME: dbenv->rep_set_clockskew(dbenv, u fast, u slow) */
    case _TXMAX:	*vp = _PUT_U(!dbenv->set_tx_max(dbenv, _u)); break;
    case _TXTSTAMP:
    {	time_t tstamp = _u;
	*vp = (JSVAL_IS_INT(*vp) && !dbenv->set_tx_timestamp(dbenv, &tstamp)
		? JSVAL_TRUE : JSVAL_FALSE);
    }	break;

#define	_JUMP(_v, _lbl)	_##_v:	_i = _v;	goto _lbl
    case _JUMP(DB_REP_CONF_BULK,		_set_config);
    case _JUMP(DB_REP_CONF_DELAYCLIENT,		_set_config);
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8)
    case _JUMP(DB_REP_CONF_INMEM,		_set_config);
#endif
    case _JUMP(DB_REP_CONF_LEASE,		_set_config);
#if defined(DB_REP_CONF_NOAUTOINIT)	/* XXX db version? */
    case _JUMP(DB_REP_CONF_NOAUTOINIT,		_set_config);
#endif
    case _JUMP(DB_REP_CONF_NOWAIT,		_set_config);
    case _JUMP(DB_REPMGR_CONF_2SITE_STRICT,	_set_config);
#undef	_JUMP
    _set_config:
	*vp = _PUT_I(!dbenv->rep_set_config(dbenv, _i, (_u ? 1 : 0)));
	break;
	/* XXX FIXME _gb always 0 */

    case _REPLIMIT:	_PUT_U(!dbenv->rep_set_limit(dbenv, _gb, _u)); break;
    case _REPNSITES:	_PUT_U(!dbenv->rep_set_nsites(dbenv, _u)); break;
    case _REPPRIORITY:	_PUT_U(!dbenv->rep_set_priority(dbenv, _u)); break;
	/* XXX FIXME: dbenv->rep_set_request(dbenv, u min, u max) */

#define	_JUMP(_v, _lbl)	_##_v:	_i = _v;	goto _lbl
    case _JUMP(DB_REP_ACK_TIMEOUT,		_rep_set_timeout);
    case _JUMP(DB_REP_CHECKPOINT_DELAY,		_rep_set_timeout);
    case _JUMP(DB_REP_CONNECTION_RETRY,		_rep_set_timeout);
    case _JUMP(DB_REP_ELECTION_TIMEOUT,		_rep_set_timeout);
    case _JUMP(DB_REP_ELECTION_RETRY,		_rep_set_timeout);
    case _JUMP(DB_REP_FULL_ELECTION_TIMEOUT,	_rep_set_timeout);
    case _JUMP(DB_REP_HEARTBEAT_MONITOR,	_rep_set_timeout);
    case _JUMP(DB_REP_HEARTBEAT_SEND,		_rep_set_timeout);
    case _JUMP(DB_REP_LEASE_TIMEOUT,		_rep_set_timeout);
#undef	_JUMP
    _rep_set_timeout:
	*vp = _PUT_U(!dbenv->rep_set_timeout(dbenv, _i, _u));
	break;

	/* XXX FIXME: dbenv->repmgr_set_ack_policy(dbenv, _i) */

    default:
	break;
    }

    _s = _free(_s);

    return JS_TRUE;
}

static JSBool
rpmdbe_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);

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
rpmdbe_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
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
static DB_ENV *
rpmdbe_init(JSContext *cx, JSObject *obj)
{
    DB_ENV * dbenv = NULL;
    uint32_t _flags = 0;
    int ret = db_env_create(&dbenv, _flags);

    if (ret || dbenv == NULL || !JS_SetPrivate(cx, obj, (void *)dbenv)) {
	if (dbenv)
	    ret = dbenv->close(dbenv, _flags);

	/* XXX error msg */
	dbenv = NULL;
    } else {
	dbenv->set_errcall(dbenv, rpmdbe_errcall);
	dbenv->set_msgcall(dbenv, rpmdbe_msgcall);
	ret = dbenv->set_app_dispatch(dbenv, rpmdbe_app_dispatch);
	if (ret) dbenv->err(dbenv, ret, "DB_ENV->set_app_dispatch");
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) dbenv %p\n", __FUNCTION__, cx, obj, dbenv);

    return dbenv;
}

static void
rpmdbe_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdbeClass, NULL);
    DB_ENV * dbenv = ptr;
    uint32_t _flags = 0;

_DTOR_DEBUG_ENTRY(_debug);

    if (dbenv)
	(void) dbenv->close(dbenv, _flags);

}

static JSBool
rpmdbe_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmdbe_init(cx, obj);
    } else {
	if ((obj = JS_NewObject(cx, &rpmdbeClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmdbe_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmdbeClass, NULL);
#ifdef	NOTYET
    DB_ENV * dbenv = ptr;
    const char *_fn = NULL;
    const char * _con = NULL;
#endif
    JSBool ok = JS_FALSE;

#ifdef	NOTYET
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_fn)))
        goto exit;

    *vp = (db && _fn && (_con = rpmdbeLgetfilecon(db, _fn)) != NULL)
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
JSClass rpmdbeClass = {
    "Dbe", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmdbe_addprop,   rpmdbe_delprop, rpmdbe_getprop, rpmdbe_setprop,
    (JSEnumerateOp)rpmdbe_enumerate, (JSResolveOp)rpmdbe_resolve,
    rpmdbe_convert,	rpmdbe_dtor,

    rpmdbe_getobjectops,	rpmdbe_checkaccess,
    rpmdbe_call,		rpmdbe_construct,
    rpmdbe_xdrobject,	rpmdbe_hasinstance,
    rpmdbe_mark,		rpmdbe_reserveslots,
};

JSObject *
rpmjs_InitDbeClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    proto = JS_InitClass(cx, obj, NULL, &rpmdbeClass, rpmdbe_ctor, 1,
		rpmdbe_props, rpmdbe_funcs, NULL, NULL);
assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewDbeObject(JSContext *cx)
{
    JSObject *obj;
    DB_ENV * dbenv;

    if ((obj = JS_NewObject(cx, &rpmdbeClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((dbenv = rpmdbe_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmdbe, Dbe, JS_TRUE)
