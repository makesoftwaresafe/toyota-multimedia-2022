/*@-type@*/ /* FIX: annotate db3 methods */
/** \ingroup db3
 * \file rpmdb/db3.c
 */

/*@unchecked@*/
static int _debug = 1;	/* XXX if < 0 debugging, > 0 unusual error returns */

#include "system.h"

#if defined(HAVE_FTOK) && defined(HAVE_SYS_IPC_H)
#include <sys/ipc.h>
#endif

#include <rpmlog.h>
#include <rpmmacro.h>
#include <rpmbf.h>
#include <rpmpgp.h>		/* XXX pgpExtractPubkeyFingerprint */
#include <rpmurl.h>		/* XXX urlPath proto */

#define	_RPMTAG_INTERNAL
#include <rpmtag.h>

#define _RPMEVR_INTERNAL	/* XXX isInstallPrereq */
#include <rpmevr.h>

#define	_RPMDB_INTERNAL
#include <rpmdb.h>

#include "debug.h"

#ifdef	NOTYET	/* XXX syscall ACID needs --with-db=internal */
extern int logio_dispatch(DB_ENV * dbenv, DBT * dbt, DB_LSN * lsn, db_recops op)
	/*@*/;
#endif

#define	DBIDEBUG(_dbi, _list)	if ((_dbi)->dbi_debug) fprintf _list

/*@access rpmdb @*/
/*@access dbiIndex @*/
/*@access dbiIndexSet @*/

/*@-redef@*/
union _dbswap {
    uint64_t ul;
    uint32_t ui;
    uint16_t us;
    uint8_t uc[8];
};
/*@=redef@*/
/*@unchecked@*/
static union _dbswap _endian = { .ui = 0x11223344 };

static inline uint64_t _ntoh_ul(uint64_t ul)
	/*@*/
{
    union _dbswap _a;
    _a.ul = ul;
    if (_endian.uc[0] == 0x44) {
	uint8_t _b, *_c = _a.uc; \
	_b = _c[7]; _c[7] = _c[0]; _c[0] = _b; \
	_b = _c[6]; _c[6] = _c[1]; _c[1] = _b; \
	_b = _c[5]; _c[5] = _c[2]; _c[2] = _b; \
	_b = _c[4]; _c[4] = _c[3]; _c[3] = _b; \
    }
    return _a.ul;
}
static inline uint64_t _hton_ul(uint64_t ul)
	/*@*/
{
    return _ntoh_ul(ul);
}

static inline uint32_t _ntoh_ui(uint32_t ui)
	/*@*/
{
    union _dbswap _a;
    _a.ui = ui;
    if (_endian.uc[0] == 0x44) {
	uint8_t _b, *_c = _a.uc; \
	_b = _c[3]; _c[3] = _c[0]; _c[0] = _b; \
	_b = _c[2]; _c[2] = _c[1]; _c[1] = _b; \
    }
    return _a.ui;
}
static inline uint32_t _hton_ui(uint32_t ui)
	/*@*/
{
    return _ntoh_ui(ui);
}

static inline uint16_t _ntoh_us(uint16_t us)
	/*@*/
{
    union _dbswap _a;
    _a.us = us;
    if (_endian.uc[0] == 0x44) {
	uint8_t _b, *_c = _a.uc; \
	_b = _c[1]; _c[1] = _c[0]; _c[0] = _b; \
    }
    return _a.us;
}
static inline uint16_t _hton_us(uint16_t us)
	/*@*/
{
    return _ntoh_us(us);
}

#ifdef	NOTNOW
static const char * bfstring(unsigned int x, const char * xbf)
{
    const char * s = xbf;
    static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    static char buf[BUFSIZ];
    char * t, * te;
    unsigned radix;
    unsigned c, i, k;

    radix = (s != NULL ? *s++ : 16);

    if (radix <= 1 || radix >= 32)
	radix = 16;

    t = buf;
    switch (radix) {
    case 8:	*t++ = '0';	break;
    case 16:	*t++ = '0';	*t++ = 'x';	break;
    }

    i = 0;
    k = x;
    do { i++; k /= radix; } while (k);

    te = t + i;

    k = x;
    do { --i; t[i] = digits[k % radix]; k /= radix; } while (k);

    t = te;
    i = '<';
    if (s != NULL)
    while ((c = *s++) != '\0') {
	if (c > ' ') continue;

	k = (1 << (c - 1));
	if (!(x & k)) continue;

	if (t == te) *t++ = '=';

	*t++ = i;
	i = ',';
	while (*s > ' ')
	    *t++ = *s++;
    }
    if (t > te)	*t++ = '>';
    *t = '\0';
    return buf;
}

/* XXX checked with db-4.5.20 */
static const char * dbtFlags =
	"\20\1APPMALLOC\2ISSET\3MALLOC\4PARTIAL\5REALLOC\6USERMEM\7DUPOK";

static const char * dbenvOpenFlags =
	"\20\1CREATE\2DURABLE_UNKNOWN\3FORCE\4MULTIVERSION\5NOMMAP\6RDONLY\7RECOVER\10THREAD\11TRUNCATE\12TXN_NOSYNC\13TXN_NOT_DURABLEi\14TXN_WRITE_NOSYNC\15USE_ENVIRON\16USE_ENVIRON_ROOT\17CDB\20LOCK\21LOG\22MPOOL\23REP\24TXN\25LOCKDOWN\26PRIVATE\27RECOVER_FATAL\30REGISTER\31SYSTEM_MEM";

static const char * dbOpenFlags =
	"\20\1CREATE\2DURABLE_UNKNOWN\3FORCE\4MULTIVERSION\5NOMMAP\6RDONLY\7RECOVER\10THREAD\11TRUNCATE\12TXN_NOSYNC\13TXN_NOT_DURABLEi\14TXN_WRITE_NOSYNC\15USE_ENVIRON\16USE_ENVIRON_ROOT\17EXCL\20FCNTL_LOCKING\21NO_AUTO_COMMIT\22RDWRMASTER\23WRITEOPEN";

static const char * dbenvSetFlags =
	"\20\1CREATE\2DURABLE_UNKNOWN\3FORCE\4MULTIVERSION\5NOMMAP\6RDONLY\7RECOVER\10THREAD\11TRUNCATE\12TXN_NOSYNC\13TXN_NOT_DURABLEi\14TXN_WRITE_NOSYNC\15USE_ENVIRON\16USE_ENVIRON_ROOT\17CDB_ALLDB\20DIRECT_DB\21DIRECT_LOG\22DSYNC_DB\23DSYNC_LOG\24LOG_AUTOREMOVE\25LOG_INMEMORY\26NOLOCKING\27NOPANIC\30OVERWRITE\31PANIC_ENV\36REGION_INIT\37TIME_NOTGRANTED\40YIELDCPU";

static const char * dbSetFlags =
	"\20\1CREATE\2DURABLE_UNKNOWN\3FORCE\4MULTIVERSION\5NOMMAP\6RDONLY\7RECOVER\10THREAD\11TRUNCATE\12TXN_NOSYNC\13TXN_NOT_DURABLEi\14TXN_WRITE_NOSYNC\15USE_ENVIRON\16USE_ENVIRON_ROOT\17CHKSUM\20DUP\21DUPSORT\22ENCRYPT\23INORDER\24RECNUM\25RENUMBER\26REVSPLITOFF\27SNAPSHOT";

static const char * dbiModeFlags =
	"\20\1WRONLY\2RDWR\7CREAT\10EXCL\11NOCTTY\12TRUNC\13APPEND\14NONBLOCK\15SYNC\16ASYNC\17DIRECT\20LARGEFILE\21DIRECTORY\22NOFOLLOW";
#endif	/* NOTNOW */

/*@-redef@*/
typedef struct key_s {
    uint32_t	v;
/*@observer@*/
    const char *n;
} KEY;
/*@=redef@*/

/*@observer@*/
static const char * tblName(uint32_t v, KEY * tbl, size_t ntbl)
	/*@*/
{
    const char * n = NULL;
    static char buf[32];
    size_t i;

    for (i = 0; i < ntbl; i++) {
	if (v != tbl[i].v)
	    continue;
	n = tbl[i].n;
	break;
    }
    if (n == NULL) {
	(void) snprintf(buf, sizeof(buf), "0x%x", (unsigned)v);
	n = buf;
    }
    return n;
}

static const char * fmtBits(uint32_t flags, KEY tbl[], size_t ntbl, char *t)
	/*@modifies t @*/
{
    char pre = '<';
    char * te = t;
    int i;

    sprintf(t, "0x%x", (unsigned)flags);
    te = t;
    te += strlen(te);
    for (i = 0; i < 32; i++) {
	uint32_t mask = (1 << i);
	const char * name;

	if (!(flags & mask))
	    continue;

	name = tblName(mask, tbl, ntbl);
	*te++ = pre;
	pre = ',';
	te = stpcpy(te, name);
    }
    if (pre == ',') *te++ = '>';
    *te = '\0';
    return t;
}

#define _ENTRY(_v)      { DB_##_v, #_v, }

/*@unchecked@*/ /*@observer@*/
static KEY DBeflags[] = {
    _ENTRY(INIT_CDB),
    _ENTRY(INIT_LOCK),
    _ENTRY(INIT_LOG),
    _ENTRY(INIT_MPOOL),
#if defined(DB_INIT_MUTEX)	/* XXX db-6.1.19 breaks "make distcheck" */
    _ENTRY(INIT_MUTEX),		/* XXX not in DBENV->open() doco */
#endif
    _ENTRY(INIT_REP),
    _ENTRY(INIT_TXN),
    _ENTRY(RECOVER),
    _ENTRY(RECOVER_FATAL),
    _ENTRY(USE_ENVIRON),
    _ENTRY(USE_ENVIRON_ROOT),
    _ENTRY(CREATE),
    _ENTRY(LOCKDOWN),
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8) || (DB_VERSION_MAJOR >= 5)
    _ENTRY(FAILCHK),
#endif
    _ENTRY(PRIVATE),
    _ENTRY(REGISTER),
    _ENTRY(SYSTEM_MEM),
    _ENTRY(THREAD),
};
/*@unchecked@*/
static size_t nDBeflags = sizeof(DBeflags) / sizeof(DBeflags[0]);
/*@observer@*/
static const char * fmtDBeflags(uint32_t flags)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;
    te = stpcpy(te, "\n\tflags: ");
    (void) fmtBits(flags, DBeflags, nDBeflags, te);
    return buf;
}
#define	_EFLAGS(_eflags)	fmtDBeflags(_eflags)

/*@unchecked@*/ /*@observer@*/
static KEY DBoflags[] = {
    _ENTRY(AUTO_COMMIT),
    _ENTRY(CREATE),
    _ENTRY(EXCL),
    _ENTRY(MULTIVERSION),
    _ENTRY(NOMMAP),
    _ENTRY(RDONLY),
    _ENTRY(READ_UNCOMMITTED),
    _ENTRY(THREAD),
    _ENTRY(TRUNCATE),
};
/*@unchecked@*/
static size_t nDBoflags = sizeof(DBoflags) / sizeof(DBoflags[0]);
/*@observer@*/
static const char * fmtDBoflags(uint32_t flags)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;
    te = stpcpy(te, "\n\tflags: ");
    (void) fmtBits(flags, DBoflags, nDBoflags, te);
    return buf;
}
#define	_OFLAGS(_oflags)	fmtDBoflags(_oflags)

/*@unchecked@*/ /*@observer@*/
static KEY DBaflags[] = {
    _ENTRY(CREATE),
    _ENTRY(IMMUTABLE_KEY),
};
/*@unchecked@*/
static size_t nDBaflags = sizeof(DBaflags) / sizeof(DBaflags[0]);
/*@observer@*/
static const char * fmtDBaflags(uint32_t flags)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;
    te = stpcpy(te, "\n\tflags: ");
    (void) fmtBits(flags, DBaflags, nDBaflags, te);
    return buf;
}
#define	_AFLAGS(_aflags)	fmtDBaflags(_aflags)

/*@unchecked@*/ /*@observer@*/
static KEY DBafflags[] = {
    _ENTRY(FOREIGN_ABORT),
    _ENTRY(FOREIGN_CASCADE),
    _ENTRY(FOREIGN_NULLIFY),
};
/*@unchecked@*/
static size_t nDBafflags = sizeof(DBafflags) / sizeof(DBafflags[0]);
/*@observer@*/
static const char * fmtDBafflags(uint32_t flags)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;
    te = stpcpy(te, "\n\tflags: ");
    (void) fmtBits(flags, DBafflags, nDBafflags, te);
    return buf;
}
#define	_AFFLAGS(_afflags)	fmtDBafflags(_afflags)

/*@unchecked@*/ /*@observer@*/
static KEY DBCoflags[] = {
	/* XXX DB->cursor() doco for db-5.1.19 lists, undef'd */
#if defined(DB_CURSOR_BULK)
    _ENTRY(CURSOR_BULK),
#endif
    _ENTRY(READ_COMMITTED),
    _ENTRY(READ_UNCOMMITTED),
    _ENTRY(WRITECURSOR),
    _ENTRY(TXN_SNAPSHOT),
};
/*@unchecked@*/
static size_t nDBCoflags = sizeof(DBCoflags) / sizeof(DBCoflags[0]);
/*@observer@*/
static const char * fmtDBCoflags(uint32_t flags)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;
    (void) fmtBits(flags, DBCoflags, nDBCoflags, te);
    return buf;
}
#define	_DBCOFLAGS(_coflags)	fmtDBCoflags(_coflags)

/*@unchecked@*/ /*@observer@*/
static KEY DBCflags[] = {
    _ENTRY(AFTER),		/* Dbc.put */
    _ENTRY(APPEND),		/* Db.put */
    _ENTRY(BEFORE),		/* Dbc.put */
    _ENTRY(CONSUME),		/* Db.get, Dbc.del */
    _ENTRY(CONSUME_WAIT),	/* Db.get */
    _ENTRY(CURRENT),		/* Dbc.get, Dbc.put, DbLogc.get */
    _ENTRY(FIRST),		/* Dbc.get, DbLogc->get */
    _ENTRY(GET_BOTH),		/* Db.get, Dbc.get */
    _ENTRY(GET_BOTHC),		/* Dbc.get (internal) */
    _ENTRY(GET_BOTH_RANGE),	/* Db.get, Dbc.get */
    _ENTRY(GET_RECNO),		/* Dbc.get */
    _ENTRY(JOIN_ITEM),		/* Dbc.get; don't do primary lookup */
    _ENTRY(KEYFIRST),		/* Dbc.put */
    _ENTRY(KEYLAST),		/* Dbc.put */
    _ENTRY(LAST),		/* Dbc.get, DbLogc->get */
    _ENTRY(NEXT),		/* Dbc.get, DbLogc->get */
    _ENTRY(NEXT_DUP),		/* Dbc.get */
    _ENTRY(NEXT_NODUP),		/* Dbc.get */
    _ENTRY(NODUPDATA),		/* Db.put, Dbc.put */
    _ENTRY(NOOVERWRITE),	/* Db.put */
    _ENTRY(NOSYNC),		/* Db.close */
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8) || (DB_VERSION_MAJOR >= 5)
    _ENTRY(OVERWRITE_DUP),	/* Dbc.put, Db.put; no DB_KEYEXIST */
#endif
    _ENTRY(POSITION),		/* Dbc.dup */
    _ENTRY(PREV),		/* Dbc.get, DbLogc->get */
    _ENTRY(PREV_DUP),		/* Dbc.get */
    _ENTRY(PREV_NODUP),		/* Dbc.get */
    _ENTRY(SET),		/* Dbc.get, DbLogc->get */
    _ENTRY(SET_RANGE),		/* Dbc.get */
    _ENTRY(SET_RECNO),		/* Db.get, Dbc.get */
    _ENTRY(UPDATE_SECONDARY),	/* Dbc.get, Dbc.del (internal) */
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8) || (DB_VERSION_MAJOR >= 5)
    _ENTRY(SET_LTE),		/* Dbc.get (internal) */
    _ENTRY(GET_BOTH_LTE),	/* Dbc.get (internal) */
#endif

    _ENTRY(IGNORE_LEASE),
    _ENTRY(READ_COMMITTED),
    _ENTRY(READ_UNCOMMITTED),
    _ENTRY(MULTIPLE),
    _ENTRY(MULTIPLE_KEY),
    _ENTRY(RMW),
};
#undef	_ENTRY
/*@unchecked@*/
static size_t nDBCflags = sizeof(DBCflags) / sizeof(DBCflags[0]);

/*@observer@*/
static const char * fmtDBCflags(uint32_t flags)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;
    uint32_t op = (flags & DB_OPFLAGS_MASK);
    flags &= ~DB_OPFLAGS_MASK;

    te = stpcpy(te, "\n\tflags: ");
    if (op) {
	te = stpcpy( stpcpy(te, "DB_"), tblName(op, DBCflags, nDBCflags));
	*te++ = ' ';
	*te = '\0';
    }
    if (flags)
	(void) fmtBits(flags, DBCflags, nDBCflags, te);
    return buf;
}
#define	_DBCFLAGS(_flags)	fmtDBCflags(_flags)

#define _DBT_ENTRY(_v)      { DB_DBT_##_v, #_v, }
/*@unchecked@*/ /*@observer@*/
static KEY DBTflags[] = {
    _DBT_ENTRY(MALLOC),
    _DBT_ENTRY(REALLOC),
    _DBT_ENTRY(USERMEM),
    _DBT_ENTRY(PARTIAL),
    _DBT_ENTRY(APPMALLOC),
    _DBT_ENTRY(MULTIPLE),
#if defined(DB_DBT_READONLY)	/* XXX db-5.2.28 */
    _DBT_ENTRY(READONLY),
#endif
#if defined(DB_DBT_ISSET)
    _DBT_ENTRY(ISSET),
#endif
#if defined(DB_DBT_USERCOPY)
    _DBT_ENTRY(USERCOPY),
#endif
#if defined(DB_DBT_USERMEM)
    _DBT_ENTRY(USERMEM),
#endif
#if defined(DB_DBT_BLOB)
    _DBT_ENTRY(BLOB),
#endif
#if defined(DB_DBT_BLOB_REC)	/* internal */
    _DBT_ENTRY(BLOB_REC),
#endif
#if defined(DB_DBT_STREAMING)	/* internal */
    _DBT_ENTRY(STREAMING),
#endif
#if defined(DB_DBT_BULK)	/* internal */
    _DBT_ENTRY(BULK),
#endif
#if defined(DB_DBT_DUPOK)	/* internal */
    _DBT_ENTRY(DUPOK),
#endif
};
#undef	_DBT_ENTRY
/*@unchecked@*/
static size_t nDBTflags = sizeof(DBTflags) / sizeof(DBTflags[0]);

/*@observer@*/
static char * fmtDBT(const DBT * K, char * te)
	/*@modifies te @*/
{
    static size_t keymax = 35;
    int unprintable;
    uint32_t i;

    sprintf(te, "%p[%u]\t", K->data, (unsigned)K->size);
    te += strlen(te);
    (void) fmtBits(K->flags, DBTflags, nDBTflags, te);
    te += strlen(te);
    if (K->data && K->size > 0) {
 	uint8_t * _u;
	size_t _nu;

	/* Grab the key data/size. */
	if (K->flags & DB_DBT_MULTIPLE) {
	    DBT * _K = K->data;
	    _u = _K->data;
	    _nu = _K->size;
	} else {
	    _u = K->data;
	    _nu = K->size;
	}
	/* Verify if data is a string. */
	unprintable = 0;
	for (i = 0; i < _nu; i++)
	    unprintable |= !xisprint(_u[i]);

	/* Display the data. */
	if (!unprintable) {
	    size_t nb = (_nu < keymax ? _nu : keymax);
	    char * ellipsis = (_nu < keymax ? "" : "...");
	    sprintf(te, "\t\"%.*s%s\"", (int)nb, (char *)_u, ellipsis);
	} else {
	    switch (_nu) {
	    default: break;
	    case 4:	sprintf(te, "\t0x%08x", (unsigned)*(uint32_t *)_u); break;
	    }
	}

	te += strlen(te);
	*te = '\0';
    }
    return te;
}
/*@observer@*/
static const char * fmtKDR(const DBT * K, const DBT * P, const DBT * D, const DBT * R)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;

    if (K) {
	te = stpcpy(te, "\n\t  key: ");
	te = fmtDBT(K, te);
    }
    if (P) {
	te = stpcpy(te, "\n\t pkey: ");
	te = fmtDBT(P, te);
    }
    if (D) {
	te = stpcpy(te, "\n\t data: ");
	te = fmtDBT(D, te);
    }
    if (R) {
	te = stpcpy(te, "\n\t  res: ");
	te = fmtDBT(R, te);
    }
    *te = '\0';
    
    return buf;
}
#define	_KEYDATA(_K, _P, _D, _R)	fmtKDR(_K, _P, _D, _R)

/*@-globuse -mustmod @*/	/* FIX: rpmError not annotated yet. */
static int Xcvtdberr(/*@unused@*/ dbiIndex dbi, const char * msg,
		int error, int printit,
		const char * func, const char * fn, unsigned ln)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int rc = error;

    if (printit && rc) {
/*@-moduncon@*/ /* FIX: annotate db3 methods */
	rpmlog(RPMLOG_ERR, "%s:%s:%u: %s(%d): %s\n",
		func, fn, ln, msg, rc, db_strerror(error));
/*@=moduncon@*/
    }

    return rc;
}
/*@=globuse =mustmod @*/
#define	cvtdberr(_dbi, _msg, _error, _printit)	\
    Xcvtdberr(_dbi, _msg, _error, _printit, __FUNCTION__, __FILE__, __LINE__)

/**
 * Return (possibly renamed) tagName. Handles arbitrary tags.
 * @param rpmdb		rpm database
 * @param dbi		rpm database index
 * @return		tag string
 */
/*@observer@*/
static const char * mapTagName(rpmdb rpmdb, dbiIndex dbi)
	/*@*/
{
    tagStore_t dbiTags = rpmdb->db_tags;
    size_t dbix = 0;

    if (dbiTags != NULL)
    while (dbix < rpmdb->db_ndbi) {
	if (dbi->dbi_rpmtag == dbiTags->tag)
	    return dbiTags->str;
	dbiTags++;
	dbix++;
    }
    /* XXX should never reach here */
    return tagName(dbi->dbi_rpmtag);
}

static int db_fini(dbiIndex dbi, const char * dbhome,
		/*@null@*/ const char * dbfile,
		/*@unused@*/ /*@null@*/ const char * dbsubfile)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    rpmdb rpmdb = dbi->dbi_rpmdb;
    DB_ENV * dbenv = rpmdb->db_dbenv;
    int rc;

DBIDEBUG(dbi, (stderr, "--> %s(%p,%s,%s,%s)\n", __FUNCTION__, dbi, dbhome, dbfile, dbsubfile));

    if (dbenv == NULL)
	return 0;

    rc = dbenv->close(dbenv, 0);
    rc = cvtdberr(dbi, "dbenv->close", rc, _debug);
    rpmdb->db_dbenv = NULL;

    if (dbfile)
	rpmlog(RPMLOG_DEBUG, D_("closed   db environment %s/%s\n"),
			dbhome, dbfile);

    if (rpmdb->db_remove_env) {
	int xx;

	/*@-moduncon@*/ /* FIX: annotate db3 methods */
	xx = db_env_create(&dbenv, 0);
	/*@=moduncon@*/
	if (!xx && dbenv != NULL) {
	    xx = cvtdberr(dbi, "db_env_create", xx, _debug);
	    xx = dbenv->remove(dbenv, dbhome, DB_FORCE);
	    xx = cvtdberr(dbi, "dbenv->remove", xx, _debug);

	    if (dbfile)
		rpmlog(RPMLOG_DEBUG, D_("removed  db environment %s/%s\n"),
			dbhome, dbfile);
	}

    }
    return rc;
}

static int db3_fsync_disable(/*@unused@*/ int fd)
	/*@*/
{
    return 0;
}

#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 5) || (DB_VERSION_MAJOR >= 5)
/**
 * Is process/thread still alive?
 * @param dbenv		db environment
 * @param pid		process id
 * @param tid		thread id
 * @param flags		0 or DB_MUTEX_PROCESS_ONLY
 * @return		
 */
static int db3is_alive(/*@unused@*/ DB_ENV *dbenv, pid_t pid,
		/*@unused@*/ db_threadid_t tid,
		rpmuint32_t flags)
	/*@*/
{
    int is_alive = 1;	/* assume all processes are alive */

    switch (flags) {
    case DB_MUTEX_PROCESS_ONLY:
    case 0:
    default:
	is_alive = (!(kill(pid, 0) < 0 && errno == ESRCH));
	break;
    }
    return is_alive;
}
#endif

/*==============================================================*/

/* HAVE_SYS_SYSCTL_H */
#if defined(HAVE_PHYSMEM_SYSCTL) || defined(HAVE_NCPU_SYSCTL)
#include <sys/sysctl.h>
#endif

static uint64_t physmem(void)
	/*@*/
{
    static uint64_t _physmem = 0;
    static int oneshot = 0;

    if (!oneshot) {
#if defined(HAVE_PHYSMEM_SYSCONF)
	const long _pagesize = sysconf(_SC_PAGESIZE);
	const long _pages = sysconf(_SC_PHYS_PAGES);
	if (_pagesize != -1 || _pages != -1)
	    _physmem = (uint64_t)(_pagesize) * (uint64_t)(_pages);
#elif defined(HAVE_PHYSMEM_SYSCTL)
	int name[2] = { CTL_HW, HW_PHYSMEM };
	unsigned long mem;
	size_t mem_ptr_size = sizeof(mem);
	if (!sysctl(name, 2, &mem, &mem_ptr_size, NULL, 0)) {
	    if (mem_ptr_size != sizeof(mem)) {
		if (mem_ptr_size == sizeof(unsigned int))
		    _physmem = *(unsigned int *)(&mem);
	    } else {
		_physmem = mem;
	    }
	}
#endif
	oneshot++;
    }
    return _physmem;
}

static size_t ncores(void)
	/*@*/
{
    static size_t _ncores = 1;
    static int oneshot = 0;

    if (!oneshot) {
#if defined(HAVE_NCPU_SYSCONF)
	const long cpus = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(HAVE_NCPU_SYSCTL)
	int name[2] = { CTL_HW, HW_NCPU };
	int cpus = 0;
	size_t cpus_size = sizeof(cpus);
	if (sysctl(name, 2, &cpus, &cpus_size, NULL, 0)
	 || cpus_size != sizeof(cpus))
	    cpus = 0;
#endif
	if (cpus > (int)_ncores)
	    _ncores = (size_t)(cpus);
	oneshot++;
    }
    return _ncores;
}

/*==============================================================*/
#define	_TABLE(_v)	{ #_v, DB_EVENT_##_v }
static struct _events_s {
    const char * n;
    uint32_t v;
} _events[] = {
#if (DB_VERSION_MAJOR == 6 && DB_VERSION_MINOR >= 1)
    _TABLE(PANIC),		/*  0 */
    _TABLE(REG_ALIVE),		/*  1 */
    _TABLE(REG_PANIC),		/*  2 */
    _TABLE(REP_AUTOTAKEOVER_FAILED), /*  3 */
    _TABLE(REP_CLIENT),		/*  4 */
    _TABLE(REP_CONNECT_BROKEN),	/*  5 */
    _TABLE(REP_CONNECT_ESTD),	/*  6 */
    _TABLE(REP_CONNECT_TRY_FAILED), /*  7 */
    _TABLE(REP_DUPMASTER),	/*  8 */
    _TABLE(REP_ELECTED),	/*  9 */
    _TABLE(REP_ELECTION_FAILED),/* 10 */
    _TABLE(REP_INQUEUE_FULL),	/* 11 */
    _TABLE(REP_INIT_DONE),	/* 12 */
    _TABLE(REP_JOIN_FAILURE),	/* 13 */
    _TABLE(REP_LOCAL_SITE_REMOVED), /* 14 */
    _TABLE(REP_MASTER),		/* 15 */
    _TABLE(REP_MASTER_FAILURE),	/* 16 */
    _TABLE(REP_NEWMASTER),	/* 17 */
    _TABLE(REP_PERM_FAILED),	/* 18 */
    _TABLE(REP_SITE_ADDED),	/* 19 */
    _TABLE(REP_SITE_REMOVED),	/* 20 */
    _TABLE(REP_STARTUPDONE),	/* 21 */
    _TABLE(REP_WOULD_ROLLBACK),	/* 22 */
    _TABLE(WRITE_FAILED),	/* 23 */
    _TABLE(MUTEX_DIED),		/* 24 */
    _TABLE(FAILCHK_PANIC),	/* 25 */
    _TABLE(NO_SUCH_EVENT),	/* 26 */
    _TABLE(NO_SUCH_EVENT),	/* 27 */
    _TABLE(NO_SUCH_EVENT),	/* 28 */
    _TABLE(NO_SUCH_EVENT),	/* 29 */
    _TABLE(NO_SUCH_EVENT),	/* 30 */
    _TABLE(NO_SUCH_EVENT),	/* 31 */
#elif (DB_VERSION_MAJOR == 6 && DB_VERSION_MINOR >= 0)
    _TABLE(PANIC),		/*  0 */
    _TABLE(REG_ALIVE),		/*  1 */
    _TABLE(REG_PANIC),		/*  2 */
    _TABLE(REP_AUTOTAKEOVER_FAILED), /*  3 */
    _TABLE(REP_CLIENT),		/*  4 */
    _TABLE(REP_CONNECT_BROKEN),	/*  5 */
    _TABLE(REP_CONNECT_ESTD),	/*  6 */
    _TABLE(REP_CONNECT_TRY_FAILED), /*  7 */
    _TABLE(REP_DUPMASTER),	/*  8 */
    _TABLE(REP_ELECTED),	/*  9 */
    _TABLE(REP_ELECTION_FAILED),/* 10 */
    _TABLE(REP_INIT_DONE),	/* 11 */
    _TABLE(REP_JOIN_FAILURE),	/* 12 */
    _TABLE(REP_LOCAL_SITE_REMOVED), /* 13 */
    _TABLE(REP_MASTER),		/* 14 */
    _TABLE(REP_MASTER_FAILURE),	/* 15 */
    _TABLE(REP_NEWMASTER),	/* 16 */
    _TABLE(REP_PERM_FAILED),	/* 17 */
    _TABLE(REP_SITE_ADDED),	/* 18 */
    _TABLE(REP_SITE_REMOVED),	/* 19 */
    _TABLE(REP_STARTUPDONE),	/* 20 */
    _TABLE(REP_WOULD_ROLLBACK),	/* 21 */
    _TABLE(WRITE_FAILED),	/* 22 */
    _TABLE(NO_SUCH_EVENT),	/* 23 */
    _TABLE(NO_SUCH_EVENT),	/* 24 */
    _TABLE(NO_SUCH_EVENT),	/* 25 */
    _TABLE(NO_SUCH_EVENT),	/* 26 */
    _TABLE(NO_SUCH_EVENT),	/* 27 */
    _TABLE(NO_SUCH_EVENT),	/* 28 */
    _TABLE(NO_SUCH_EVENT),	/* 29 */
    _TABLE(NO_SUCH_EVENT),	/* 30 */
    _TABLE(NO_SUCH_EVENT),	/* 31 */
#elif (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2)
    _TABLE(PANIC),		/*  0 */
    _TABLE(REG_ALIVE),		/*  1 */
    _TABLE(REG_PANIC),		/*  2 */
    _TABLE(REP_CLIENT),		/*  3 */
    _TABLE(REP_CONNECT_BROKEN),	/*  4 */
    _TABLE(REP_CONNECT_ESTD),	/*  5 */
    _TABLE(REP_CONNECT_TRY_FAILED), /*  6 */
    _TABLE(REP_DUPMASTER),	/*  7 */
    _TABLE(REP_ELECTED),	/*  8 */
    _TABLE(REP_ELECTION_FAILED),/*  9 */
    _TABLE(REP_INIT_DONE),	/* 10 */
    _TABLE(REP_JOIN_FAILURE),	/* 11 */
    _TABLE(REP_LOCAL_SITE_REMOVED), /* 12 */
    _TABLE(REP_MASTER),		/* 13 */
    _TABLE(REP_MASTER_FAILURE),	/* 14 */
    _TABLE(REP_NEWMASTER),	/* 15 */
    _TABLE(REP_PERM_FAILED),	/* 16 */
    _TABLE(REP_SITE_ADDED),	/* 17 */
    _TABLE(REP_SITE_REMOVED),	/* 18 */
    _TABLE(REP_STARTUPDONE),	/* 19 */
    _TABLE(REP_WOULD_ROLLBACK),	/* 20 */
    _TABLE(WRITE_FAILED),	/* 21 */
    _TABLE(NO_SUCH_EVENT),	/* 22 */
    _TABLE(NO_SUCH_EVENT),	/* 23 */
    _TABLE(NO_SUCH_EVENT),	/* 24 */
    _TABLE(NO_SUCH_EVENT),	/* 25 */
    _TABLE(NO_SUCH_EVENT),	/* 26 */
    _TABLE(NO_SUCH_EVENT),	/* 27 */
    _TABLE(NO_SUCH_EVENT),	/* 28 */
    _TABLE(NO_SUCH_EVENT),	/* 29 */
    _TABLE(NO_SUCH_EVENT),	/* 30 */
    _TABLE(NO_SUCH_EVENT),	/* 31 */
#elif (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR < 2) || (DB_VERSION_MAJOR >= 6)
	/* XXX numbered from db-5.1.19, older versions are different. */
    _TABLE(PANIC),		/*  0 */
    _TABLE(REG_ALIVE),		/*  1 */
    _TABLE(REG_PANIC),		/*  2 */
    _TABLE(REP_CLIENT),		/*  3 */
    _TABLE(REP_DUPMASTER),	/*  4 */
    _TABLE(REP_ELECTED),	/*  5 */
    _TABLE(REP_ELECTION_FAILED),/*  6 */
    _TABLE(REP_JOIN_FAILURE),	/*  7 */
    _TABLE(REP_MASTER),		/*  8 */
    _TABLE(REP_MASTER_FAILURE),	/*  9 */
    _TABLE(REP_NEWMASTER),	/* 10 */
    _TABLE(REP_PERM_FAILED),	/* 11 */
    _TABLE(REP_STARTUPDONE),	/* 12 */
    _TABLE(WRITE_FAILED),	/* 13 */
    _TABLE(NO_SUCH_EVENT),	/* 14 */
    _TABLE(NO_SUCH_EVENT),	/* 15 */
    _TABLE(NO_SUCH_EVENT),	/* 16 */
    _TABLE(NO_SUCH_EVENT),	/* 17 */
    _TABLE(NO_SUCH_EVENT),	/* 18 */
    _TABLE(NO_SUCH_EVENT),	/* 19 */
    _TABLE(NO_SUCH_EVENT),	/* 20 */
    _TABLE(NO_SUCH_EVENT),	/* 21 */
    _TABLE(NO_SUCH_EVENT),	/* 22 */
    _TABLE(NO_SUCH_EVENT),	/* 23 */
    _TABLE(NO_SUCH_EVENT),	/* 24 */
    _TABLE(NO_SUCH_EVENT),	/* 25 */
    _TABLE(NO_SUCH_EVENT),	/* 26 */
    _TABLE(NO_SUCH_EVENT),	/* 27 */
    _TABLE(NO_SUCH_EVENT),	/* 28 */
    _TABLE(NO_SUCH_EVENT),	/* 29 */
    _TABLE(NO_SUCH_EVENT),	/* 30 */
    _TABLE(NO_SUCH_EVENT),	/* 31 */
#else
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
    _TABLE(NO_SUCH_EVENT),	/* 16 */
    _TABLE(NO_SUCH_EVENT),	/* 17 */
    _TABLE(NO_SUCH_EVENT),	/* 18 */
    _TABLE(NO_SUCH_EVENT),	/* 19 */
    _TABLE(NO_SUCH_EVENT),	/* 20 */
    _TABLE(NO_SUCH_EVENT),	/* 21 */
    _TABLE(NO_SUCH_EVENT),	/* 22 */
    _TABLE(NO_SUCH_EVENT),	/* 23 */
    _TABLE(NO_SUCH_EVENT),	/* 24 */
    _TABLE(NO_SUCH_EVENT),	/* 25 */
    _TABLE(NO_SUCH_EVENT),	/* 26 */
    _TABLE(NO_SUCH_EVENT),	/* 27 */
    _TABLE(NO_SUCH_EVENT),	/* 28 */
    _TABLE(NO_SUCH_EVENT),	/* 29 */
    _TABLE(NO_SUCH_EVENT),	/* 30 */
    _TABLE(NO_SUCH_EVENT),	/* 31 */
#endif
};
#undef	_TABLE

static void
rpmdbe_event_notify(DB_ENV * dbenv, u_int32_t event, void * event_info)
{
    void * o = (dbenv ? dbenv->app_private : NULL);
fprintf(stderr, "==> %s(%p, %s(%u), %p) app_private %p\n", __FUNCTION__, dbenv, _events[event & 0x1f].n, event, event_info, o);
}

static void
rpmdbe_feedback(DB_ENV * dbenv, int opcode, int percent)
	/*@*/
{
    dbenv = NULL;
    dbenv = dbenv;
    switch (opcode) {
    case DB_RECOVER:
	fprintf(stderr, "\rrecovery %d%% complete", percent);
	(void)fflush(stderr);	/* XXX unnecessary? */
	/*@fallthrough@*/
    default:
	break;
    }
}

/*@-moduncon@*/ /* FIX: annotate db3 methods */
static int db_init(dbiIndex dbi, const char * dbhome,
		/*@null@*/ const char * dbfile,
		/*@unused@*/ /*@null@*/ const char * dbsubfile,
		/*@out@*/ DB_ENV ** dbenvp)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies dbi, *dbenvp, fileSystem, internalState @*/
{
    static int oneshot = 0;
    uint64_t _physmem = physmem();
    size_t _ncores = ncores();
    rpmdb rpmdb = dbi->dbi_rpmdb;
    DB_ENV *dbenv = NULL;
    int eflags;
    int rc;
    int xx;

    if (!oneshot) {
	rpmlog(RPMLOG_DEBUG, D_("rpmdb: cpus %u physmem %uMb\n"),
		(unsigned)_ncores, (unsigned)(_physmem/(1024 * 1024)));
	xx = db_env_set_func_open((int (*)(const char *, int, ...))Open);
	xx = cvtdberr(dbi, "db_env_set_func_open", xx, _debug);
	oneshot++;
    }

    if (dbenvp == NULL)
	return 1;

    /* XXX HACK */
    /*@-assignexpose@*/
    if (rpmdb->db_errfile == NULL)
	rpmdb->db_errfile = stderr;
    /*@=assignexpose@*/

    eflags = (dbi->dbi_oeflags | dbi->dbi_eflags);
    /* Try to join, rather than create, the environment. */
    /* XXX DB_JOINENV is defined to 0 in db-4.5.20 */
    if (eflags & DB_JOINENV) eflags &= DB_JOINENV;
    /* XXX DB_RECOVER needs automagic */
    if (!(eflags & DB_INIT_TXN)) eflags &= ~DB_RECOVER;

    if (dbfile)
	rpmlog(RPMLOG_DEBUG, D_("opening  db environment %s/%s %s\n"),
		dbhome, dbfile, prDbiOpenFlags(eflags, 1));

    /* XXX Can't do RPC w/o host. */
#if defined(DB_RPCCLIENT)
    if (dbi->dbi_host == NULL)
	dbi->dbi_ecflags &= ~DB_RPCCLIENT;
#endif

	/* XXX DB_THREAD from dbi->dbi_oeflags? */
    rc = db_env_create(&dbenv, dbi->dbi_ecflags);
    rc = cvtdberr(dbi, "db_env_create", rc, _debug);
    if (dbenv == NULL || rc)
	goto errxit;

/*@-noeffectuncon@*/
/*@-castfcnptr@*/
    dbenv->set_errcall(dbenv, (void *)rpmdb->db_errcall);
/*@=castfcnptr@*/
    dbenv->set_errfile(dbenv, rpmdb->db_errfile);
    dbenv->set_errpfx(dbenv, rpmdb->db_errpfx);
/*@=noeffectuncon@*/

 /* 4.1: dbenv->set_alloc(???) */
 /* 4.1: dbenv->set_data_dir(???) */
 /* 4.1: dbenv->set_encrypt(???) */

 /* 5.3: dbenv->backup() */
 /* 5.3: dbenv->dbackup() */
 /* 5.3: dbenv->set_backup_callbacks() */
 /* 5.3: dbenv->set_backup_config() */

 /* 5.3: dbenv->set_metadata_dir() */

    xx = dbenv->set_feedback(dbenv, rpmdbe_feedback);
    xx = cvtdberr(dbi, "dbenv->set_feedback", xx, _debug);
    xx = dbenv->set_event_notify(dbenv, rpmdbe_event_notify);
    xx = cvtdberr(dbi, "dbenv->set_event_notify", xx, _debug);

 /* 4.1: dbenv->set_flags(???) */

 /* dbenv->set_paniccall(???) */

#if defined(DB_RPCCLIENT)
    if ((dbi->dbi_ecflags & DB_RPCCLIENT) && dbi->dbi_host) {
	const char * home;
	int retry = 0;

	if ((home = strrchr(dbhome, '/')) != NULL)
	    dbhome = ++home;

	while (retry++ < 5) {
/* XXX 3.3.4 change. */
	    xx = dbenv->set_rpc_server(dbenv, NULL, dbi->dbi_host,
		dbi->dbi_cl_timeout, dbi->dbi_sv_timeout, 0);
	    xx = cvtdberr(dbi, "dbenv->set_server", xx, _debug);
	    if (!xx)
		break;
	    (void) sleep(15);
	}
    } else
#endif
    {

	{   size_t _lo =  16 * 1024 * 1024;
	    size_t _hi = 512 * 1024 * 1024;
	    size_t _mp_mmapsize = _physmem;	/* XXX default value? */
	    if (_mp_mmapsize < _lo) _mp_mmapsize = _lo;
	    if (_mp_mmapsize > _hi) _mp_mmapsize = _hi;
	    xx = dbenv->set_mp_mmapsize(dbenv, _mp_mmapsize);
	    xx = cvtdberr(dbi, "dbenv->set_mp_mmapsize", xx, _debug);
	}

	if (dbi->dbi_tmpdir) {
	    const char * root;
	    const char * tmpdir;

	    root = (dbi->dbi_root ? dbi->dbi_root : rpmdb->db_root);
	    if ((root[0] == '/' && root[1] == '\0') || rpmdb->db_chrootDone)
		root = NULL;
/*@-mods@*/
	    tmpdir = rpmGenPath(root, dbi->dbi_tmpdir, NULL);
/*@=mods@*/
	    xx = dbenv->set_tmp_dir(dbenv, tmpdir);
	    xx = cvtdberr(dbi, "dbenv->set_tmp_dir", xx, _debug);
	    tmpdir = _free(tmpdir);
	}
    }

/* ==== Locking: */
#define	_RPMDB_NLOCKS	16384
    if (eflags & DB_INIT_LOCK) {
	uint32_t _lk_max_lockers = _RPMDB_NLOCKS;
	uint32_t _lk_max_locks = _RPMDB_NLOCKS;
	uint32_t _lk_max_objects = _RPMDB_NLOCKS;

	xx = dbenv->set_lk_max_lockers(dbenv, _lk_max_lockers);
	xx = cvtdberr(dbi, "dbenv->set_lk_max_lockers", xx, _debug);
	xx = dbenv->set_lk_max_locks(dbenv, _lk_max_locks);
	xx = cvtdberr(dbi, "dbenv->set_lk_max_locks", xx, _debug);
	xx = dbenv->set_lk_max_objects(dbenv, _lk_max_objects);
	xx = cvtdberr(dbi, "dbenv->set_lk_max_objects", xx, _debug);

      {	uint32_t _max = 10 * _RPMDB_NLOCKS;
	xx = dbenv->mutex_set_max(dbenv, _max);
	xx = cvtdberr(dbi, "dbenv->mutex_set_max", xx, _debug);
      }

    }

/* ==== Logging: */
    const char *logdir;

    logdir = rpmGetPath(dbhome, "/", "log", NULL);
    /*
     * Create the /var/lib/rpm/log directory if it doesn't exist (root only).
     */
    rpmioMkpath(logdir, 0755, getuid(), getgid());

    xx = dbenv->set_lg_dir(dbenv, logdir);
    xx = cvtdberr(dbi, "dbenv->set_lg_dir", xx, _debug);

    _free(logdir);

/* ==== Memory pool: */
    if (eflags & DB_INIT_MPOOL) {
	uint32_t _lo =  16 * 1024 * 1024;
	uint32_t _hi = 512 * 1024 * 1024;
	uint32_t _gb = 0;
	uint32_t _bytes	= _physmem;		/* XXX default value? */
	int _ncache = 4;
	if (_bytes < _lo) _bytes = _lo;
	if (_bytes > _hi) _bytes = _hi;
	xx = dbenv->set_cache_max(dbenv, _gb, _hi);
	xx = cvtdberr(dbi, "dbenv->set_cache_max", xx, _debug);
	if (_ncache > 0)
	    _bytes /= _ncache;
	xx = dbenv->set_cachesize(dbenv, _gb, _bytes, _ncache);
	xx = cvtdberr(dbi, "dbenv->set_cachesize", xx, _debug);
    }

/* ==== Mutexes: */
/* ==== Replication: */
/* ==== Sequences: */
/* ==== Transactions: */
#ifdef	NOTYET	/* XXX syscall ACID needs --with-db=internal */
    if (eflags & DB_INIT_TXN) {
	xx = dbenv->set_app_dispatch(dbenv, logio_dispatch);
	xx = cvtdberr(dbi, "dbenv->set_app_dispatch", xx, _debug);
    }
#endif

/* ==== Other: */
    if (dbi->dbi_no_fsync) {
	xx = db_env_set_func_fsync(db3_fsync_disable);
	xx = cvtdberr(dbi, "db_env_set_func_fsync", xx, _debug);
    }

    /* XXX Set a default shm_key. */
    if ((eflags & DB_SYSTEM_MEM) && dbi->dbi_shmkey == 0) {
#if defined(HAVE_FTOK)
	dbi->dbi_shmkey = ftok(dbhome, 0);
#else
	dbi->dbi_shmkey = 0x44631380;
#endif
    }
    if (dbi->dbi_shmkey) {
	xx = dbenv->set_shm_key(dbenv, dbi->dbi_shmkey);
	xx = cvtdberr(dbi, "dbenv->set_shm_key", xx, _debug);
    }

#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 5) || (DB_VERSION_MAJOR >= 5)
    /* XXX capture dbenv->falchk output on stderr. */
/*@-noeffectuncon@*/
    dbenv->set_msgfile(dbenv, rpmdb->db_errfile);
/*@=noeffectuncon@*/
    if (dbi->dbi_thread_count >= 8) {
	xx = dbenv->set_thread_count(dbenv, dbi->dbi_thread_count);
	xx = cvtdberr(dbi, "dbenv->set_thread_count", xx, _debug);
    }
#endif

    /* XXX Attempt db_recover -ev (i.e. dbenv w DB_INIT_LOCK) */
    if (eflags & DB_RECOVER) {
	eflags |= DB_CREATE;
	xx = dbenv->set_verbose(dbenv, DB_VERB_RECOVERY, 1);
	xx = cvtdberr(dbi, "dbenv->set_verbose", xx, _debug);
    }

    rc = (dbenv->open)(dbenv, dbhome, eflags, dbi->dbi_perms);
    xx = _debug;
#if defined(DB_VERSION_MISMATCH)
    if (rc == DB_VERSION_MISMATCH) xx = 0;
#endif
    if (rc == EINVAL) xx = 0;
    rc = cvtdberr(dbi, "dbenv->open", rc, xx);
    if (rc)
	goto errxit;

#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 5) || (DB_VERSION_MAJOR >= 5)
    if (dbi->dbi_thread_count >= 8) {
	/* XXX Set pid/tid is_alive probe. */
	xx = dbenv->set_isalive(dbenv, db3is_alive);
	xx = cvtdberr(dbi, "dbenv->set_isalive", xx, _debug);
	/* XXX Clean out stale shared read locks. */
	xx = dbenv->failchk(dbenv, 0);
	xx = cvtdberr(dbi, "dbenv->failchk", xx, _debug);
	if (xx == DB_RUNRECOVERY) {
	    rc = xx;
	    goto errxit;
	}
    }
#endif

    *dbenvp = dbenv;

DBIDEBUG(dbi, (stderr, "<-- %s(%p(%s),%s,%s,%s,%p) dbenv %p %s\n", __FUNCTION__, dbi, tagName(dbi->dbi_rpmtag), dbhome, dbfile, dbsubfile, dbenvp, dbenv, _EFLAGS(eflags)));

    return 0;

errxit:
    if (dbenv) {
	xx = dbenv->close(dbenv, 0);
	xx = cvtdberr(dbi, "dbenv->close", xx, _debug);
    }
    return rc;
}
/*@=moduncon@*/

#ifdef	NOTYET
/*@-mustmod@*/
static int db3remove(dbiIndex dbi, /*@null@*/ const char * dbfile,
		/*@unused@*/ /*@null@*/ const char * dbsubfile,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    int rc;

assert(db != NULL);
    rc = db->remove(db, dbfile, dbsubfile, flags);
    rc = cvtdberr(dbi, "db->remove", rc, _debug);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%s,%s,0x%x) rc %d\n", __FUNCTION__, dbi, dbfile, dbsubfile, flags, rc));

    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int db3rename(dbiIndex dbi, /*@null@*/ const char * dbfile,
		/*@unused@*/ /*@null@*/ const char * dbsubfile,
		/*@unused@*/ /*@null@*/ const char * newname,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    int rc;

assert(db != NULL);
    rc = db->rename(db, dbfile, dbsubfile, newname, flags);
    rc = cvtdberr(dbi, "db->rename", rc, _debug);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%s,%s,%s,0x%x) rc %d %s\n", __FUNCTION__, dbi, dbfile, dbsubfile, newname, flags, rc, _DBCFLAGS(flags)));

    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int db3truncate(dbiIndex dbi, unsigned int * countp, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *countp, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DB_TXN * _txnid = dbiTxnid(dbi);
    int rc;

assert(db != NULL);
    rc = db->truncate(db, _txnid, countp, flags);
    rc = cvtdberr(dbi, "db->truncate", rc, _debug);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,0x%x) rc %d\n", __FUNCTION__, dbi, countp, flags, rc));

    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int db3upgrade(dbiIndex dbi, /*@null@*/ const char * dbfile,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    int rc;

assert(db != NULL);
    rc = db->upgrade(db, dbfile, flags);
    rc = cvtdberr(dbi, "db->upgrade", rc, _debug);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%s,0x%x) rc %d\n", __FUNCTION__, dbi, dbfile, flags, rc));

    return rc;
}
/*@=mustmod@*/
#endif	/* NOTYET */

static int db3sync(dbiIndex dbi, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    int rc = 0;
    int _printit;

    if (db != NULL)
	rc = db->sync(db, flags);
    _printit = _debug;
    rc = cvtdberr(dbi, "db->sync", rc, _printit);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,0x%x) rc %d\n", __FUNCTION__, dbi, flags, rc));

    return rc;
}

/*@-mustmod@*/
static int db3exists(dbiIndex dbi, DBT * key, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DB_TXN * _txnid = dbiTxnid(dbi);
    int _printit;
    int rc;

assert(db != NULL);
    rc = db->exists(db, _txnid, key, flags);
    /* XXX DB_NOTFOUND can be returned */
    _printit = (rc == DB_NOTFOUND ? 0 : _debug);
    rc = cvtdberr(dbi, "db->exists", rc, _printit);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,0x%x) rc %d %s\n", __FUNCTION__, dbi, key, flags, rc, _KEYDATA(key, NULL, NULL, NULL)));

    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int db3seqno(dbiIndex dbi, int64_t * seqnop, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *seqnop, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DB_TXN * _txnid = dbiTxnid(dbi);
    DB_SEQUENCE * seq = (DB_SEQUENCE *) dbi->dbi_seq;
    int32_t _delta = 1;
    db_seq_t seqno = 0;
    int rc;

assert(db != NULL);
assert(seq != NULL);

    if (seqnop && *seqnop)
	_delta = *seqnop;

    rc = seq->get(seq, _txnid, _delta, &seqno, 0);  
    rc = cvtdberr(dbi, "seq->get", rc, _debug);
    if (rc) goto exit;

    if (seqnop)
	*seqnop = seqno;

exit:
DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,0x%x) seqno %lld rc %d\n", __FUNCTION__, dbi, seqnop, flags, (long long)seqno, rc));

    return rc;
}
/*@=mustmod@*/

static int db3cdup(dbiIndex dbi, DBC * dbcursor, DBC ** dbcp,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcp, fileSystem @*/
{
    int rc;

    if (dbcp) *dbcp = NULL;
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 6) || (DB_VERSION_MAJOR >= 5)
    rc = dbcursor->dup(dbcursor, dbcp, flags);
    rc = cvtdberr(dbi, "dbcursor->dup", rc, _debug);
#else
    rc = dbcursor->c_dup(dbcursor, dbcp, flags);
    rc = cvtdberr(dbi, "dbcursor->c_dup", rc, _debug);
#endif

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,0x%x) rc %d\n", __FUNCTION__, dbi, dbcursor, dbcp, flags, rc));

    return rc;
}

/*@-mustmod@*/
static int db3cclose(dbiIndex dbi, /*@only@*/ /*@null@*/ DBC * dbcursor,
		/*@unused@*/ unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    int rc = -2;

    /* XXX db3copen error pathways come through here. */
    if (dbcursor != NULL) {
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 6) || (DB_VERSION_MAJOR >= 5)
	rc = dbcursor->close(dbcursor);
	rc = cvtdberr(dbi, "dbcursor->close", rc, _debug);
#else
	rc = dbcursor->c_close(dbcursor);
	rc = cvtdberr(dbi, "dbcursor->c_close", rc, _debug);
#endif
    }

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,0x%x) rc %d\n", __FUNCTION__, dbi, dbcursor, flags, rc));

    return rc;
}
/*@=mustmod@*/

static int db3copen(dbiIndex dbi, DB_TXN * txnid,
		/*@null@*/ /*@out@*/ DBC ** dbcp, unsigned int dbiflags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcp, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DBC * dbcursor = NULL;
    int flags;
    int rc;

   /* XXX DB_WRITECURSOR cannot be used with sunrpc dbenv. */
    assert(db != NULL);
    if ((dbiflags & DB_WRITECURSOR) &&
	(dbi->dbi_eflags & DB_INIT_CDB) && !(dbi->dbi_oflags & DB_RDONLY))
    {
	flags = DB_WRITECURSOR;
    } else
	flags = 0;

    rc = db->cursor(db, txnid, &dbcursor, flags);
    rc = cvtdberr(dbi, "db->cursor", rc, _debug);

    if (dbcp)
	*dbcp = dbcursor;
    else
	(void) db3cclose(dbi, dbcursor, 0);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,0x%x) dbc %p %s rc %d\n", __FUNCTION__, dbi, txnid, dbcp, dbiflags, dbcursor, _DBCOFLAGS(flags), rc));
    return rc;
}

static int db3cput(dbiIndex dbi, DBC * dbcursor, DBT * key, DBT * data,
		/*@unused@*/ unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DB_TXN * _txnid = dbiTxnid(dbi);
    int rc;

    assert(db != NULL);
    if (dbcursor == NULL) {
flags = 0;
	rc = db->put(db, _txnid, key, data, flags);
	rc = cvtdberr(dbi, "db->put", rc, _debug);
    } else {
flags = DB_KEYLAST;
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 6) || (DB_VERSION_MAJOR >= 5)
	rc = dbcursor->put(dbcursor, key, data, flags);
	rc = cvtdberr(dbi, "dbcursor->put", rc, _debug);
#else
	rc = dbcursor->c_put(dbcursor, key, data, flags);
	rc = cvtdberr(dbi, "dbcursor->c_put", rc, _debug);
#endif
    }

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,%p,0x%x) rc %d %s%s\n", __FUNCTION__, dbi, dbcursor, key, data, flags, rc, _DBCFLAGS(flags), _KEYDATA(key, NULL, data, NULL)));
    return rc;
}

/*@-mustmod@*/
static int db3cget(dbiIndex dbi, DBC * dbcursor, DBT * key, DBT * data,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, *key, *data, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DB_TXN * _txnid = dbiTxnid(dbi);
    int _printit;
    int rc;

assert(db != NULL);
    if (dbcursor == NULL) {
	/* XXX duplicates require cursors. */
	rc = db->get(db, _txnid, key, data, flags);
	/* XXX DB_NOTFOUND can be returned */
	_printit = (rc == DB_NOTFOUND ? 0 : _debug);
	rc = cvtdberr(dbi, "db->get", rc, _printit);
    } else {
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 6) || (DB_VERSION_MAJOR >= 5)
	/* XXX db3 does DB_FIRST on uninitialized cursor */
	rc = dbcursor->get(dbcursor, key, data, flags);
	/* XXX DB_NOTFOUND can be returned */
	_printit = (rc == DB_NOTFOUND ? 0 : _debug);
	/* XXX Permit DB_BUFFER_SMALL to be returned (more restrictive?) */
	_printit = (rc == DB_BUFFER_SMALL ? 0 : _printit);
	rc = cvtdberr(dbi, "dbcursor->get", rc, _printit);
#else
	/* XXX db3 does DB_FIRST on uninitialized cursor */
	rc = dbcursor->c_get(dbcursor, key, data, flags);
	/* XXX DB_NOTFOUND can be returned */
	_printit = (rc == DB_NOTFOUND ? 0 : _debug);
	rc = cvtdberr(dbi, "dbcursor->c_get", rc, _printit);
#endif
    }

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,%p,0x%x) rc %d %s%s\n", __FUNCTION__, dbi, dbcursor, key, data, flags, rc, _DBCFLAGS(flags), _KEYDATA(key, NULL, data, NULL)));
    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int db3cpget(dbiIndex dbi, DBC * dbcursor, DBT * key, DBT * pkey,
		DBT * data, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, *key, *data, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DB_TXN * _txnid = dbiTxnid(dbi);
    int _printit;
    int rc;

assert(db != NULL);
    if (dbcursor == NULL) {
	/* XXX duplicates require cursors. */
	rc = db->pget(db, _txnid, key, pkey, data, flags);
	/* XXX DB_NOTFOUND can be returned */
	_printit = (rc == DB_NOTFOUND ? 0 : _debug);
	rc = cvtdberr(dbi, "db->pget", rc, _printit);
    } else {
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 6) || (DB_VERSION_MAJOR >= 5)
	/* XXX db3 does DB_FIRST on uninitialized cursor */
	rc = dbcursor->pget(dbcursor, key, pkey, data, flags);
	/* XXX DB_NOTFOUND can be returned */
	_printit = (rc == DB_NOTFOUND ? 0 : _debug);
	rc = cvtdberr(dbi, "dbcursor->pget", rc, _printit);
#else
	/* XXX db3 does DB_FIRST on uninitialized cursor */
	rc = dbcursor->c_pget(dbcursor, key, pkey, data, flags);
	/* XXX DB_NOTFOUND can be returned */
	_printit = (rc == DB_NOTFOUND ? 0 : _debug);
	rc = cvtdberr(dbi, "dbcursor->c_pget", rc, _printit);
#endif
    }

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,%p,%p,0x%x) rc %d %s%s\n", __FUNCTION__, dbi, dbcursor, key, pkey, data, flags, rc, _DBCFLAGS(flags), _KEYDATA(key, pkey, data, NULL)));
    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int db3cdel(dbiIndex dbi, DBC * dbcursor, DBT * key, DBT * data,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DB_TXN * _txnid = dbiTxnid(dbi);
    int rc;

assert(db != NULL);
    if (dbcursor == NULL) {
	rc = db->del(db, _txnid, key, flags);
	rc = cvtdberr(dbi, "db->del", rc, _debug);
    } else {

	/* XXX TODO: insure that cursor is positioned with duplicates */
	rc = db3cget(dbi, dbcursor, key, data, DB_SET);

	if (rc == 0) {
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 6) || (DB_VERSION_MAJOR >= 5)
	    rc = dbcursor->del(dbcursor, flags);
	    rc = cvtdberr(dbi, "dbcursor->del", rc, _debug);
#else
	    rc = dbcursor->c_del(dbcursor, flags);
	    rc = cvtdberr(dbi, "dbcursor->c_del", rc, _debug);
#endif
	}
    }

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,%p,0x%x) rc %d %s%s\n", __FUNCTION__, dbi, dbcursor, key, data, flags, rc, _DBCFLAGS(flags), _KEYDATA(key, NULL, data, NULL)));
    return rc;
}
/*@=mustmod@*/

static int db3ccount(dbiIndex dbi, DBC * dbcursor,
		/*@null@*/ /*@out@*/ unsigned int * countp,
		/*@unused@*/ unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *countp, fileSystem @*/
{
    db_recno_t count = 0;
    int rc = 0;

    flags = 0;
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 6) || (DB_VERSION_MAJOR >= 5)
    rc = dbcursor->count(dbcursor, &count, flags);
    rc = cvtdberr(dbi, "dbcursor->count", rc, _debug);
#else
    rc = dbcursor->c_count(dbcursor, &count, flags);
    rc = cvtdberr(dbi, "dbcursor->c_count", rc, _debug);
#endif
    if (countp) *countp = (!rc ? count : 0);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,0x%x) count %d\n", __FUNCTION__, dbi, dbcursor, countp, flags, count));

    return rc;
}

static int db3byteswapped(dbiIndex dbi)	/*@*/
{
    DB * db = (DB *) dbi->dbi_db;
    int rc = 0;

    if (db != NULL) {
	int isswapped = 0;
	rc = db->get_byteswapped(db, &isswapped);
	if (rc == 0)
	    rc = isswapped;
    }

    return rc;
}

static int db3stat(dbiIndex dbi, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 3) || (DB_VERSION_MAJOR >= 5)
    DB_TXN * _txnid = dbiTxnid(dbi);
#endif
    int rc = 0;

    assert(db != NULL);
#if defined(DB_FAST_STAT)
    if (flags)
	flags = DB_FAST_STAT;
    else
#endif
	flags = 0;
    dbi->dbi_stats = _free(dbi->dbi_stats);
/* XXX 3.3.4 change. */
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 3) || (DB_VERSION_MAJOR >= 5)
    rc = db->stat(db, _txnid, &dbi->dbi_stats, flags);
#else
    rc = db->stat(db, &dbi->dbi_stats, flags);
#endif
    rc = cvtdberr(dbi, "db->stat", rc, _debug);

DBIDEBUG(dbi, (stderr, "<-- %s(%p,0x%x) rc %d\n", __FUNCTION__, dbi, flags, rc));

    return rc;
}

/*@-mustmod@*/
static int db3associate(dbiIndex dbi, dbiIndex dbisecondary,
		int (*callback)(DB *, const DBT *, const DBT *, DBT *),
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DB * secondary = (DB *) dbisecondary->dbi_db;
    DB_TXN * _txnid = dbiTxnid(dbi);
    int rc;

assert(db != NULL);

/*@-moduncon@*/ /* FIX: annotate db3 methods */
    rc = db->associate(db, _txnid, secondary, callback, flags);
/*@=moduncon@*/
    rc = cvtdberr(dbi, "db->associate", rc, _debug);

    if (dbi->dbi_debug || dbisecondary->dbi_debug) {
    	const char * tag2 = xstrdup(tagName(dbisecondary->dbi_rpmtag));
fprintf(stderr, "<-- %s(%p(%s),%p(%s),%p,0x%x) rc %d %s\n", __FUNCTION__, dbi, tagName(dbi->dbi_rpmtag), dbisecondary, tag2, (void *)callback, flags, rc, _AFLAGS(flags));
	tag2 = _free(tag2);
    }

    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int db3associate_foreign(dbiIndex dbi, dbiIndex dbisecondary,
		int (*callback)(DB *, const DBT *, DBT *, const DBT *, int *),
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    int rc = ENOTSUP;;

#if !defined(__LCLINT__)
/*@-moduncon@*/ /* FIX: annotate db3 methods */
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8) || (DB_VERSION_MAJOR >= 5)
    DB * db = (DB *) dbi->dbi_db;
    DB * secondary = (DB *) dbisecondary->dbi_db;
assert(db != NULL);
    rc = db->associate_foreign(db, secondary, callback, flags);
#endif
/*@=moduncon@*/
#endif	/* !defined(__LCLINT__) */
    rc = cvtdberr(dbi, "db->associate_foreign", rc, _debug);

    if (dbi->dbi_debug || dbisecondary->dbi_debug) {
    	const char * tag2 = xstrdup(tagName(dbisecondary->dbi_rpmtag));
fprintf(stderr, "<-- %s(%p(%s),%p(%s),%p,0x%x) rc %d %s\n", __FUNCTION__, dbi, tagName(dbi->dbi_rpmtag), dbisecondary, tag2, callback, flags, rc, _AFFLAGS(flags));
	tag2 = _free(tag2);
    }

    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int db3join(dbiIndex dbi, DBC ** curslist, DBC ** dbcp,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    DB * db = (DB *) dbi->dbi_db;
    int rc;

DBIDEBUG(dbi, (stderr, "--> %s(%p,%p,%p,0x%x)\n", __FUNCTION__, dbi, curslist, dbcp, flags));
assert(db != NULL);
/*@-moduncon@*/ /* FIX: annotate db3 methods */
    rc = db->join(db, curslist, dbcp, flags);
/*@=moduncon@*/
    rc = cvtdberr(dbi, "db->join", rc, _debug);
    return rc;
}
/*@=mustmod@*/

/*@-moduncon@*/ /* FIX: annotate db3 methods */
static int db3close(/*@only@*/ dbiIndex dbi, /*@unused@*/ unsigned int flags)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies dbi, fileSystem, internalState @*/
{
    rpmdb rpmdb = dbi->dbi_rpmdb;
    const char * urlfn = NULL;
    const char * root;
    const char * home;
    const char * dbhome;
    const char * dbfile;
    const char * dbsubfile;
    DB * db = (DB *) dbi->dbi_db;
    DB_SEQUENCE * seq = (DB_SEQUENCE *) dbi->dbi_seq;
    const char * dbiBN = mapTagName(rpmdb, dbi);
    int _printit;
    int rc = 0, xx;

    flags = 0;	/* XXX unused */

    /*
     * Get the prefix/root component and directory path.
     */
    root = (dbi->dbi_root ? dbi->dbi_root : rpmdb->db_root);
    if ((root[0] == '/' && root[1] == '\0') || rpmdb->db_chrootDone)
	root = NULL;
    home = (dbi->dbi_home ? dbi->dbi_home : rpmdb->db_home);

    /*
     * Either the root or directory components may be a URL. Concatenate,
     * convert the URL to a path, and add the name of the file.
     */
    /*@-mods@*/
    urlfn = rpmGenPath(root, home, NULL);
    /*@=mods@*/
    (void) urlPath(urlfn, &dbhome);
    if (dbi->dbi_temporary) {
	dbfile = NULL;
	dbsubfile = NULL;
    } else {
#ifdef	HACK	/* XXX necessary to support dbsubfile */
	dbfile = (dbi->dbi_file ? dbi->dbi_file : db3basename);
	dbsubfile = (dbi->dbi_subfile ? dbi->dbi_subfile : dbiBN);
#else
	dbfile = (dbi->dbi_file ? dbi->dbi_file : dbiBN);
	dbsubfile = NULL;
#endif
    }

    if (seq) {
	rc = seq->close(seq, 0);  
	rc = cvtdberr(dbi, "seq->close", rc, _debug);
	seq = dbi->dbi_seq = NULL;

	rpmlog(RPMLOG_DEBUG, D_("closed   db seqno       %s/%s\n"),
		dbhome, (dbfile ? dbfile : dbiBN));

    }
    if (db) {
	rc = db->close(db, 0);
	/* XXX ignore not found error messages. */
	_printit = (rc == ENOENT ? 0 : _debug);
	rc = cvtdberr(dbi, "db->close", rc, _printit);
	db = dbi->dbi_db = NULL;

	rpmlog(RPMLOG_DEBUG, D_("closed   db index       %s/%s\n"),
		dbhome, (dbfile ? dbfile : dbiBN));

    }

    /* XXX avoid non-root EPERM ACID PANIC with temp Depcache close. */
    if (rpmdb->db_dbenv != NULL && dbi->dbi_use_dbenv && !dbi->dbi_temporary) {
	if (rpmdb->db_opens == 1) {
	    /*@-nullstate@*/
	    xx = db_fini(dbi, (dbhome ? dbhome : ""), dbfile, dbsubfile);
	    /*@=nullstate@*/
	    rpmdb->db_dbenv = NULL;
	}
	rpmdb->db_opens--;
    }

DBIDEBUG(dbi, (stderr, "<-- %s(%p,0x%x) rc %d\n", __FUNCTION__, dbi, flags, rc));

    dbi->dbi_db = NULL;

    urlfn = _free(urlfn);

    dbi = db3Free(dbi);

    return rc;
}
/*@=moduncon@*/

/**
 * Convert hex to binary nibble.
 * @param c		hex character
 * @return		binary nibble
 */
static inline unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (unsigned char)(c - '0');
    if (c >= 'A' && c <= 'F')
	return (unsigned char)((int)(c - 'A') + 10);
    if (c >= 'a' && c <= 'f')
	return (unsigned char)((int)(c - 'a') + 10);
    return '\0';
}

static int loadDBT(DBT * _r, rpmTag tag, const void * _s, size_t ns)
	/*@modifies *_r @*/
{
    const char * s = _s;
    void * data = NULL;
    size_t size = 0;
    uint8_t * t = NULL;
    uint32_t i;
    int xx;

    if (ns == 0) ns = strlen(s);
    switch (tag) {
    case RPMTAG_FILEDIGESTS:
	/* Convert hex to binary, filter out odd hex strings. */
	if (ns > 0 && !(ns & 1)) {
	    ns /= 2;
	    data = t = xmalloc(ns);
	    for (i = 0; i < ns; i++, t++, s += 2) {
		if (!(isxdigit(s[0]) && isxdigit(s[1])))
		    /*@loopbreak@*/ break;
		*t = (uint8_t) (nibble(s[0]) << 4) | nibble(s[1]);
	    }
	    if (i == ns)
		size = ns;
	    else
		data = _free(data);
	}
	break;
    case RPMTAG_PUBKEYS:
	/* Extract pubkey id from the base64 blob. */
	t = xmalloc(32);
	if ((xx = pgpExtractPubkeyFingerprint(s, t)) > 0) {
	    data = t;
	    size = xx;
	} else
	    t = _free(t);
	break;
    default:
	data = (void *) memcpy(xmalloc(ns), _s, ns);
	size = ns;
	break;
    }
    if ((_r->data = data) != NULL) _r->flags |= DB_DBT_APPMALLOC;
    return (_r->size = size);
}

static int uint32Cmp(const void * _a, const void * _b)
	/*@*/
{
    const uint32_t * a = _a;
    const uint32_t * b = _b;
    return ((*a < *b) ? -1 :
	   ((*a > *b) ?  1 : 0));
}

static int uint64Cmp(const void * _a, const void * _b)
	/*@*/
{
    const uint64_t * a = _a;
    const uint64_t * b = _b;
    return ((*a < *b) ? -1 :
	   ((*a > *b) ?  1 : 0));
}

static int
db3Acallback(DB * db, const DBT * key, const DBT * data, DBT * _r)
	/*@globals internalState @*/
	/*@modifies *_r, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    HE_t Fhe = (HE_t) memset(alloca(sizeof(*Fhe)), 0, sizeof(*Fhe));
#ifdef	NOTYET
    HE_t FMhe = (HE_t) memset(alloca(sizeof(*FMhe)), 0, sizeof(*FMhe));
#endif
    dbiIndex dbi = db->app_private;
    rpmdb rpmdb = NULL;
    Header h = NULL;
    uint32_t hdrNum;
    DBT * A = NULL;
    const char * s = NULL;
    size_t ns = 0;
    int rc = DB_DONOTINDEX;	/* assume no-op */
    uint32_t i;
    int xx;

assert(key->size == sizeof(hdrNum));
    memcpy(&hdrNum, key->data, key->size);
    hdrNum = _ntoh_ui(hdrNum);

    /* XXX Don't index the header instance counter at record 0. */
    if (hdrNum == 0)
	goto exit;

assert(dbi);
    rpmdb = dbi->dbi_rpmdb;
assert(rpmdb);

    /* XXX Track the maximum primary key value. */
    if (hdrNum > rpmdb->db_maxkey)
	rpmdb->db_maxkey = hdrNum;

    h = headerLink(rpmdb->db_h);
    if (h == NULL) {
	/* XXX needs PROT_READ somewhen. */
	h = headerLoad(data->data);
	if (h == NULL) {
	    rpmlog(RPMLOG_ERR,
		_("db3: header #%u cannot be loaded -- skipping.\n"),
		(unsigned)hdrNum);
	    goto exit;
	}
    }

    memset(_r, 0, sizeof(*_r));

    he->tag = dbi->dbi_rpmtag;
    if (!headerGet(h, he, 0))
	goto exit;

    /* XXX catch busted headerGet() rc on RPMTAG_FILEPATHS w empty list. */
assert(he->p.ptr != NULL && he->c > 0);

    /* Retrieve other tags needed for filtering decisions. */
    switch (he->tag) {
    default:
	break;
#ifdef	NOTYET
    case RPMTAG_BASENAMES:
    case RPMTAG_FILEPATHS:
	/* XXX Add the pesky trailing '/' to directories. */
	FMhe->tag = RPMTAG_FILEMODES;
	(void) headerGet(h, FMhe, 0);
	break;
#endif
    case RPMTAG_REQUIREYAMLENTRY:
    case RPMTAG_REQUIRENAME:
	/* The Requires: F is needed to filter install context dependencies. */
	Fhe->tag = RPMTAG_REQUIREFLAGS;
	(void) headerGet(h, Fhe, 0);
	break;
    }

    switch (he->t) {
    default:
assert(0);
	/*@notreached@*/ break;
    case RPM_UINT8_TYPE:	/* XXX coerce to uint32_t */
    {	uint8_t * _u = (uint8_t *) he->p.ui8p;
	he->p.ui32p = xmalloc(he->c * sizeof(*he->p.ui32p));
	for (i = 0; i < he->c; i++)
	    he->p.ui32p[i] = _u[i];
	_u = _free(_u);
	goto _ifill;
    }	/*@notreached@*/ break;
    case RPM_UINT16_TYPE:	/* XXX coerce to uint32_t */
    {	uint16_t * _u = (uint16_t *) he->p.ui16p;
	he->p.ui32p = xmalloc(he->c * sizeof(*he->p.ui32p));
	for (i = 0; i < he->c; i++)
	    he->p.ui32p[i] = _u[i];
	_u = _free(_u);
	goto _ifill;
    }	/*@notreached@*/ break;
    case RPM_UINT32_TYPE:
_ifill:
    {	uint32_t * _u = (uint32_t *) he->p.ui32p;
	size_t _ulen = sizeof(*_u);
	uint32_t _ube;			/* XXX network order integer keys */

	/* Drop the transaction id usecs field (if present) when indexing. */
	switch (he->tag) {
	case RPMTAG_INSTALLTID:
	case RPMTAG_REMOVETID:
	    he->c = 1;
	    /*@innerbreak@*/ break;
	default:
	    /*@innerbreak@*/ break;
	}
	if (he->c == 1) {
	    _ube = _hton_ui(*_u);	/* XXX network order integer keys */
	    /* XXX is it worth avoiding the realloc here? */
	    xx = loadDBT(_r, he->tag, &_ube, _ulen);
	    break;
	}
	_r->flags = DB_DBT_MULTIPLE | DB_DBT_APPMALLOC;
	_r->data = A = xcalloc(he->c, sizeof(*A));
	_r->size = 0;
	if (he->c > 1)
	    qsort(_u, he->c, _ulen, uint32Cmp);
	for (i = 0; i < he->c; i++, _u++) {
	    /* Don't add identical (key,val) item to secondary. */
	    if (i > 0 && _u[-1] == _u[0])
		continue;
	    _ube = _hton_ui(*_u);	/* XXX network order integer keys */
	    if (!loadDBT(A, he->tag, &_ube, _ulen))
		continue;
	    A++;
	    _r->size++;
	}
    }	break;
    case RPM_UINT64_TYPE:
    {   uint64_t * _u = (uint64_t *) he->p.ui64p;
	size_t _ulen = sizeof(*_u);
	uint64_t _ube;			/* XXX network order integer keys */

	if (he->c == 1) {
	    _ube = _hton_ul(*_u);	/* XXX network order integer keys */
	    /* XXX is it worth avoiding the realloc here? */
	    xx = loadDBT(_r, he->tag, _u, _ulen);
	    break;
	}
	_r->flags = DB_DBT_MULTIPLE | DB_DBT_APPMALLOC;
	_r->data = A = (DBT *) xcalloc(he->c, sizeof(*A));
	_r->size = 0;
	if (he->c > 1)
	    qsort(_u, he->c, _ulen, uint64Cmp);
	for (i = 0; i < he->c; i++, _u++) {
	    /* Don't add identical (key,val) item to secondary. */
	    if (i > 0 && _u[-1] == _u[0])
		continue;
	    _ube = _hton_ul(*_u);	/* XXX network order integer keys */
	    if (!loadDBT(A, he->tag, &_ube, _ulen))
		continue;
	    A++;
	    _r->size++;
	}
    }	break;
    case RPM_BIN_TYPE:
	s = (char *) he->p.ptr; ns = he->c;
	/* XXX is it worth avoiding the realloc here? */
	if (ns > 0)			/* No "" empty keys please. */
	    xx = loadDBT(_r, he->tag, s, ns);
	break;
    case RPM_I18NSTRING_TYPE:		/* XXX never occurs */
    case RPM_STRING_TYPE:
	s = he->p.str; ns = strlen(s);
	/* XXX is it worth avoiding the realloc here? */
	if (ns > 0)			/* No "" empty keys please. */
	    xx = loadDBT(_r, he->tag, s, ns);
	break;
    case RPM_STRING_ARRAY_TYPE:
	if (he->c == 1) {
	    s = he->p.argv[0]; ns = strlen(s);
	    if (ns > 0)			/* No "" empty keys please. */
		xx = loadDBT(_r, he->tag, s, ns);
	} else {
	    static double e = 1.0e-5;
	    static size_t nmin = 16;
	    size_t n = 2 * (he->c > nmin ? he->c : nmin);
	    size_t m = 0;
	    size_t k = 0;
	    rpmbf bf;
	    rpmbfParams(n, e, &m, &k);
	    bf = rpmbfNew(m, k, 0);

	    _r->flags = DB_DBT_MULTIPLE | DB_DBT_APPMALLOC;
	    _r->data = A = (DBT *) xcalloc(he->c, sizeof(*A));
	    _r->size = 0;
	    for (i = 0; i < he->c; i++) {
		s = he->p.argv[i]; ns = strlen(s);

		/* XXX Skip YAML "- ..." lead-in mark up if present. */
		if (s[0] == '-' && s[1] == ' ') {
		    s += 2, ns -= 2;
		}

#ifdef	NOTYET
		/* Add the pesky trailing '/' to directories. */
		if (FMhe->p.ui16p && !S_ISREG((mode_t)FMhe->p.ui16p[i])) {
		    continue;
		}
#endif

		if (ns == 0)		/* No "" empty keys please. */
		    continue;

		/* Filter install context dependencies. */
		if (Fhe->p.ui32p && isInstallPreReq(Fhe->p.ui32p[i]))
		    continue;

		/* Don't add identical (key,val) item to secondary. */
		if (rpmbfChk(bf, s, ns) > 0)
		    continue;
		xx = rpmbfAdd(bf, s, ns);
assert(xx == 0);

		if (!loadDBT(A, he->tag, s, ns))
		    continue;
		A++;
		_r->size++;
	    }
	    bf = rpmbfFree(bf);
	}
	break;
    }
    if (_r->data && _r->size > 0)
	rc = 0;
    else if (_r->flags & DB_DBT_APPMALLOC) {
	_r->data = _free(_r->data);
	memset(_r, 0, sizeof(*_r));
    }

exit:
    if (!dbi->dbi_no_dbsync && rc != DB_DONOTINDEX)
	xx = dbiSync(dbi, 0);
#ifdef	NOTYET
    FMhe->p.ptr = _free(FMhe->p.ptr);
#endif
    Fhe->p.ptr = _free(Fhe->p.ptr);
    he->p.ptr = _free(he->p.ptr);
    h = headerFree(h);

DBIDEBUG(dbi, (stderr, "<-- %s(%p, %p, %p, %p) rc %d\n\tdbi %p(%s) rpmdb %p h %p %s\n", __FUNCTION__, db, key, data, _r, rc, dbi, tagName(dbi->dbi_rpmtag), rpmdb, h, _KEYDATA(key, NULL, data, _r)));

    return rc;
}

static int seqid_init(dbiIndex dbi, const char * keyp, size_t keylen,
		DB_SEQUENCE ** seqp)
	/*@modifies *seqp @*/
{
    DB * db = (DB *) dbi->dbi_db;
    DBT k = {0};
    DB_TXN * _txnid = dbiTxnid(dbi);
    DB_SEQUENCE * seq = NULL;
    db_seq_t _rangemin = -922337203685477600LL;
    db_seq_t _rangemax =  922337203685477600LL;
    db_seq_t _value = 0;
    int32_t _cachesize = 0;
    uint32_t _flags = DB_SEQ_INC;
    uint32_t _oflags = DB_CREATE;
    int rc;

assert(db != NULL);
    if (seqp) *seqp = NULL;

/*@-moduncon@*/
    rc = db_sequence_create(&seq, db, 0);
/*@=moduncon@*/
    rc = cvtdberr(dbi, "db_sequence_create", rc, _debug);
    if (rc) goto exit;
assert(seq != NULL);

    if (dbi->dbi_seq_cachesize) {
	_cachesize = dbi->dbi_seq_cachesize;
	rc = seq->set_cachesize(seq, _cachesize);
	rc = cvtdberr(dbi, "seq->set_cachesize", rc, _debug);
	if (rc) goto exit;
    }

    if (dbi->dbi_seq_initial)
	_value = dbi->dbi_seq_initial;
    if (_value <= 0) _value = 1;
    rc = seq->initial_value(seq, _value);
    rc = cvtdberr(dbi, "seq->initial_value", rc, _debug);
    if (rc) goto exit;

    if (dbi->dbi_seq_min)
	_rangemin = dbi->dbi_seq_min;
    if (dbi->dbi_seq_max)
	_rangemax = dbi->dbi_seq_max;
    rc = seq->set_range(seq, _rangemin, _rangemax);
    rc = cvtdberr(dbi, "seq->set_range", rc, _debug);
    if (rc) goto exit;

    if (dbi->dbi_seq_flags)
	_flags = dbi->dbi_seq_flags;
    rc = seq->set_flags(seq, _flags);
    rc = cvtdberr(dbi, "seq->set_flags", rc, _debug);
    if (rc) goto exit;

    k.data = (void *)keyp;
    k.size = (u_int32_t) (keylen > 0 ? keylen : strlen(keyp));
    rc = seq->open(seq, _txnid, &k, _oflags);
    rc = cvtdberr(dbi, "seq->open", rc, _debug);
    if (rc) goto exit;

exit:
    if (rc == 0 && seqp != NULL)
	*seqp = seq;
    else {
	int xx = seq->close(seq, 0);  
	xx = cvtdberr(dbi, "seq->close", xx, _debug);
    }

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%p[%u],%p) seq %p rc %d %s\n", __FUNCTION__, dbi, keyp, (unsigned)keylen, seqp, (seqp ? *seqp : NULL), rc, _KEYDATA(&k, NULL, NULL, NULL)));

    return rc;
}

/**
 * Return handle for an index database.
 * @param rpmdb         rpm database
 * @param rpmtag        rpm tag
 * @retval *dbip	index database handle
 * @return              0 on success
 */
static int db3open(rpmdb rpmdb, rpmTag rpmtag, dbiIndex * dbip)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *dbip, fileSystem, internalState @*/
{
    /*@-nestedextern -shadow@*/
    extern struct _dbiVec db3vec;
    /*@=nestedextern =shadow@*/
    const char * urlfn = NULL;
    const char * root;
    const char * home;
    const char * dbhome;
    const char * dbfile;
    const char * dbsubfile;
    const char * dbiBN;
    dbiIndex dbi = NULL;
    int rc = 0;
    int xx;

    DB * db = NULL;
    DB_ENV * dbenv = NULL;
    DB_TXN * _txnid = NULL;
    DBTYPE dbi_type = DB_UNKNOWN;
    rpmuint32_t oflags;
    int _printit;

    if (dbip)
	*dbip = NULL;

    /*
     * Parse db configuration parameters.
     */
    /*@-mods@*/
    if ((dbi = db3New(rpmdb, rpmtag)) == NULL)
	/*@-nullstate@*/
	return 1;
	/*@=nullstate@*/
    /*@=mods@*/
    dbi->dbi_api = DB_VERSION_MAJOR;
    dbiBN = mapTagName(rpmdb, dbi);
    dbi->dbi_txnid = NULL;
    _txnid = NULL;

    /*
     * Get the prefix/root component and directory path.
     */
    root = (dbi->dbi_root ? dbi->dbi_root : rpmdb->db_root);
    if ((root[0] == '/' && root[1] == '\0') || rpmdb->db_chrootDone)
	root = NULL;
    home = (dbi->dbi_home ? dbi->dbi_home : rpmdb->db_home);

    /*
     * Either the root or directory components may be a URL. Concatenate,
     * convert the URL to a path, and add the name of the file.
     */
    /*@-mods@*/
    urlfn = rpmGenPath(root, home, NULL);
    /*@=mods@*/
    (void) urlPath(urlfn, &dbhome);
    if (dbi->dbi_temporary) {
	dbfile = NULL;
	dbsubfile = NULL;
    } else {
#ifdef	HACK	/* XXX necessary to support dbsubfile */
	dbfile = (dbi->dbi_file ? dbi->dbi_file : db3basename);
	dbsubfile = (dbi->dbi_subfile ? dbi->dbi_subfile : dbiBN);
#else
	dbfile = (dbi->dbi_file ? dbi->dbi_file : dbiBN);
	dbsubfile = NULL;
#endif
    }

    oflags = (dbi->dbi_oeflags | dbi->dbi_oflags);
    /* XXX permit DB_TRUNCATE iff a secondary index. */
    if (dbi->dbi_primary) oflags &= ~DB_TRUNCATE;

#if 0	/* XXX rpmdb: illegal flag combination specified to DB->open */
    if ( dbi->dbi_mode & O_EXCL) oflags |= DB_EXCL;
#endif

    /*
     * Map open mode flags onto configured database/environment flags.
     */
    if (dbi->dbi_temporary) {
	oflags |= DB_CREATE;
	dbi->dbi_oeflags |= DB_CREATE;
	oflags &= ~DB_RDONLY;
	dbi->dbi_oflags &= ~DB_RDONLY;
    } else {
	if (!(dbi->dbi_mode & (O_RDWR|O_WRONLY))) oflags |= DB_RDONLY;
	if (dbi->dbi_mode & O_CREAT) {
	    oflags |= DB_CREATE;
	    dbi->dbi_oeflags |= DB_CREATE;
	}
	/* XXX permit DB_TRUNCATE iff a secondary index. */
	if (dbi->dbi_primary && (dbi->dbi_mode & O_TRUNC))
	    oflags |= DB_TRUNCATE;
    }

    /*
     * Create the /var/lib/rpm directory if it doesn't exist (root only).
     */
    (void) rpmioMkpath(dbhome, 0755, getuid(), getgid());

    /*
     * Avoid incompatible DB_CREATE/DB_RDONLY flags on DBENV->open.
     */
    if (dbi->dbi_use_dbenv) {

	if (access(dbhome, W_OK) == -1) {

	    /* dbhome is unwritable, don't attempt DB_CREATE on DB->open ... */
	    oflags &= ~DB_CREATE;
	    oflags &= ~DB_AUTO_COMMIT;

	    /* ... but DBENV->open might still need DB_CREATE ... */
	    if (dbi->dbi_eflags & DB_PRIVATE) {
		dbi->dbi_eflags &= ~DB_JOINENV;
	    } else {
		dbi->dbi_eflags |= DB_JOINENV;
		dbi->dbi_oeflags &= ~DB_CREATE;
#ifdef	DYING
		dbi->dbi_oeflags &= ~DB_THREAD;
#endif
		/* ... but, unless DB_PRIVATE is used, skip DBENV. */
		dbi->dbi_use_dbenv = 0;
	    }

	    /* ... DB_RDONLY maps dbhome perms across files ...  */
	    if (dbi->dbi_temporary) {
		oflags |= DB_CREATE;
		dbi->dbi_oeflags |= DB_CREATE;
		oflags &= ~DB_RDONLY;
		dbi->dbi_oflags &= ~DB_RDONLY;
	    } else {
		oflags |= DB_RDONLY;
		/* ... and DB_WRITECURSOR won't be needed ...  */
		dbi->dbi_oflags |= DB_RDONLY;
	    }

	} else {	/* dbhome is writable, check for persistent dbenv. */
	    /*@-mods@*/
	    const char * dbf = rpmGetPath(dbhome, "/__db.001", NULL);
	    /*@=mods@*/

#if defined(RPM_VENDOR_OPENPKG) /* bdb-allow-zero-sized-files */
	    /* Make sure RPM passes DB_CREATE to Berkeley-DB also
	       if file exists, but is (still) zero-sized. */
	    struct stat sb;
	    long size = -1;
	    if (stat(dbf, &sb) == 0)
		size = (long)sb.st_size;
	    if (access(dbf, F_OK) == -1 || size == 0)
#else
	    if (access(dbf, F_OK) == -1)
#endif
	    {
		/* ... non-existent (or unwritable) DBENV, will create ... */
		dbi->dbi_oeflags |= DB_CREATE;
		dbi->dbi_eflags &= ~DB_JOINENV;
	    } else {
		/* ... pre-existent (or bogus) DBENV, will join ... */
		if (dbi->dbi_eflags & DB_PRIVATE) {
		    dbi->dbi_eflags &= ~DB_JOINENV;
		} else {
		    dbi->dbi_eflags |= DB_JOINENV;
		    dbi->dbi_oeflags &= ~DB_CREATE;
#ifdef	DYING
		    dbi->dbi_oeflags &= ~DB_THREAD;
#endif
		}
	    }
	    /* ... transactionally protected open's need DB_AUTO_COMMIT ... */
	    if (rpmdb->_dbi[0]
	     && rpmdb->_dbi[0]->dbi_eflags & DB_INIT_TXN)
		oflags |= DB_AUTO_COMMIT;
	    dbf = _free(dbf);
	}
    }

    /*
     * Avoid incompatible DB_CREATE/DB_RDONLY flags on DB->open.
     */
    if ((oflags & DB_CREATE) && (oflags & DB_RDONLY)) {
	/* dbhome is writable, and DB->open flags may conflict. */
	const char * dbfn = (dbfile ? dbfile : dbiBN);
	/*@-mods@*/
	const char * dbf = rpmGetPath(dbhome, "/", dbfn, NULL);
	/*@=mods@*/

	if (access(dbf, F_OK) == -1) {
	    /* File does not exist, DB->open might create ... */
	    oflags &= ~DB_RDONLY;
	} else {
	    /* File exists, DB->open need not create ... */
	    oflags &= ~DB_CREATE;
	}

	/* Only writers need DB_WRITECURSOR ... */
	if (!(oflags & DB_RDONLY) && access(dbf, W_OK) == 0) {
	    dbi->dbi_oflags &= ~DB_RDONLY;
	} else {
	    dbi->dbi_oflags |= DB_RDONLY;
	}
	dbf = _free(dbf);
    }

    /*
     * Set db type if creating or truncating.
     */
    if (oflags & (DB_CREATE|DB_TRUNCATE))
	dbi_type = (DBTYPE) dbi->dbi_type;

    if (dbi->dbi_use_dbenv) {
	/*@-mods@*/
	if (rpmdb->db_dbenv == NULL) {
	    rc = db_init(dbi, dbhome, dbfile, dbsubfile, &dbenv);
	    switch (rc) {
	    default:
		break;
	    case DB_RUNRECOVERY:
		if (getuid() != 0)
		    break;
		rpmlog(RPMLOG_NOTICE, _("Re-opening dbenv with DB_RECOVER ...\n"));
		/* XXX Attempt db_recover -ev (i.e. dbenv w DB_INIT_LOCK) */
		dbi->dbi_eflags |= DB_RECOVER;
		rc = db_init(dbi, dbhome, dbfile, dbsubfile, &dbenv);
		dbi->dbi_eflags &= ~DB_RECOVER;
		if (rc) {
		    rpmlog(RPMLOG_NOTICE, _("\nrecovery failed. Exiting ...\n"));
		    exit(EXIT_FAILURE);
		}
		rpmlog(RPMLOG_NOTICE, _(".\nrecovery succeeded.\n"));
assert(dbenv);
		rpmdb->db_dbenv = dbenv;
		rpmdb->db_opens = 1;
		break;

#if defined(DB_VERSION_MISMATCH) /* Nuke __db* files and retry open once. */
	    case DB_VERSION_MISMATCH:
#endif
	    case EINVAL:
		if (getuid() != 0)
		    break;
		{   char * filename = (char *) alloca(BUFSIZ);
		    struct stat st;
		    int i;

		    for (i = 0; i < 16; i++) {
			sprintf(filename, "%s/__db.%03d", dbhome, i);
			(void)rpmCleanPath(filename);
			if (Stat(filename, &st)
			  && (errno == ENOENT || errno == EINVAL))
			    continue;
			xx = Unlink(filename);
		    }
		}
		dbi->dbi_oeflags |= DB_CREATE;
		dbi->dbi_eflags &= ~DB_JOINENV;
		rc = db_init(dbi, dbhome, dbfile, dbsubfile, &dbenv);
		/* XXX db_init EINVAL was masked. */
		rc = cvtdberr(dbi, "dbenv->open", rc, _debug);
		if (rc)
		    break;
		/*@fallthrough@*/
	    case 0:
assert(dbenv);
		rpmdb->db_dbenv = dbenv;
		rpmdb->db_opens = 1;
		break;
	    }
	} else {
assert(rpmdb && rpmdb->db_dbenv);
	    dbenv = (DB_ENV *) rpmdb->db_dbenv;
	    rpmdb->db_opens++;
	}
	/*@=mods@*/
    }

    rpmlog(RPMLOG_DEBUG, D_("opening  db index       %s/%s %s mode=0x%x\n"),
		dbhome, (dbfile ? dbfile : dbiBN),
		prDbiOpenFlags(oflags, 0), dbi->dbi_mode);

    if (rc == 0) {
	static int _lockdbfd = 0;

	/*@-moduncon@*/ /* FIX: annotate db3 methods */
	rc = db_create(&db, dbenv, dbi->dbi_cflags);
	/*@=moduncon@*/
	rc = cvtdberr(dbi, "db_create", rc, _debug);
	if (rc == 0 && db != NULL) {

/* XXX 3.3.4 change. */
	    if (rc == 0 &&
			rpmdb->db_malloc && rpmdb->db_realloc && rpmdb->db_free)
	    {
		rc = db->set_alloc(db,
			rpmdb->db_malloc, rpmdb->db_realloc, rpmdb->db_free);
		rc = cvtdberr(dbi, "db->set_alloc", rc, _debug);
	    }

/* 4.1: db->set_cache_priority(???) */
/* 4.1: db->set_encrypt(???) */

/* 5.3: db->set_lk_exclusive(???) */

	    if (rc == 0 && dbi->dbi_lorder) {
		rc = db->set_lorder(db, dbi->dbi_lorder);
		rc = cvtdberr(dbi, "db->set_lorder", rc, _debug);
	    }
	    if (rc == 0 && dbi->dbi_pagesize) {
		rc = db->set_pagesize(db, dbi->dbi_pagesize);
		rc = cvtdberr(dbi, "db->set_pagesize", rc, _debug);
	    }
 /* 4.1: db->set_paniccall(???) */
	    if (rc == 0 && oflags & DB_CREATE) {
		switch(dbi->dbi_type) {
		default:
		case DB_HASH:
		    if (dbi->dbi_h_ffactor) {
			rc = db->set_h_ffactor(db, dbi->dbi_h_ffactor);
			rc = cvtdberr(dbi, "db->set_h_ffactor", rc, _debug);
			if (rc) break;
		    }
		    if (dbi->dbi_h_nelem) {
			rc = db->set_h_nelem(db, dbi->dbi_h_nelem);
			rc = cvtdberr(dbi, "db->set_h_nelem", rc, _debug);
			if (rc) break;
		    }
		    if (dbi->dbi_h_flags) {
			rc = db->set_flags(db, dbi->dbi_h_flags);
			rc = cvtdberr(dbi, "db->set_h_flags", rc, _debug);
			if (rc) break;
		    }
		    if (dbi->dbi_h_hash_fcn) {
			rc = db->set_h_hash(db, dbi->dbi_h_hash_fcn);
			rc = cvtdberr(dbi, "db->set_h_hash", rc, _debug);
			if (rc) break;
		    }
#if defined(DB_INIT_MUTEX)	/* XXX db-6.1.19 breaks "make distcheck" */
		    if (dbi->dbi_h_dup_compare_fcn) {
			rc = db->set_h_compare(db, dbi->dbi_h_dup_compare_fcn);
			rc = cvtdberr(dbi, "db->set_h_compare", rc, _debug);
			if (rc) break;
		    }
#endif
		    break;
		case DB_BTREE:
/* 4.1: db->set_append_recno(???) */
		    if (dbi->dbi_bt_flags) {
			rc = db->set_flags(db, dbi->dbi_bt_flags);
			rc = cvtdberr(dbi, "db->set_bt_flags", rc, _debug);
			if (rc) break;
		    }
		    if (dbi->dbi_bt_minkey) {
			rc = db->set_bt_minkey(db, dbi->dbi_bt_minkey);
			rc = cvtdberr(dbi, "db->set_bt_minkey", rc, _debug);
			if (rc) break;
		    }
#if defined(DB_INIT_MUTEX)	/* XXX db-6.1.19 breaks "make distcheck" */
		    if (dbi->dbi_bt_compare_fcn) {
			rc = db->set_bt_compare(db, dbi->dbi_bt_compare_fcn);
			rc = cvtdberr(dbi, "db->set_bt_compare", rc, _debug);
			if (rc) break;
		    }
		    if (dbi->dbi_bt_dup_compare_fcn) {
			rc = db->set_dup_compare(db, dbi->dbi_bt_dup_compare_fcn);
			rc = cvtdberr(dbi, "db->set_dup_compare", rc, _debug);
			if (rc) break;
		    }
#endif
		    if (dbi->dbi_bt_prefix_fcn) {
			rc = db->set_bt_prefix(db, dbi->dbi_bt_prefix_fcn);
			rc = cvtdberr(dbi, "db->set_bt_prefix", rc, _debug);
			if (rc) break;
		    }
		    break;
		case DB_RECNO:
/* 4.1: db->set_append_recno(???) */
		    if (dbi->dbi_re_delim) {
			rc = db->set_re_delim(db, dbi->dbi_re_delim);
			rc = cvtdberr(dbi, "db->set_re_selim", rc, _debug);
			if (rc) break;
		    }
		    if (dbi->dbi_re_len) {
			rc = db->set_re_len(db, dbi->dbi_re_len);
			rc = cvtdberr(dbi, "db->set_re_len", rc, _debug);
			if (rc) break;
		    }
		    if (dbi->dbi_re_pad) {
			rc = db->set_re_pad(db, dbi->dbi_re_pad);
			rc = cvtdberr(dbi, "db->set_re_pad", rc, _debug);
			if (rc) break;
		    }
		    if (dbi->dbi_re_source) {
			rc = db->set_re_source(db, dbi->dbi_re_source);
			rc = cvtdberr(dbi, "db->set_re_source", rc, _debug);
			if (rc) break;
		    }
		    break;
		case DB_QUEUE:
		    if (dbi->dbi_q_extentsize) {
			rc = db->set_q_extentsize(db, dbi->dbi_q_extentsize);
			rc = cvtdberr(dbi, "db->set_q_extentsize", rc, _debug);
			if (rc) break;
		    }
		    break;
#if (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 2) || (DB_VERSION_MAJOR >= 6)
		case DB_HEAP:
		    if (dbi->dbi_heapsize) {
			static uint32_t _gbytes = 0;
			uint32_t _bytes = dbi->dbi_heapsize;
			static uint32_t _flags = 0;
assert(dbi->dbi_heapsize >= (3 * dbi->dbi_pagesize));
			rc = db->set_heapsize(db, _gbytes, _bytes, _flags);
			rc = cvtdberr(dbi, "db->set_heapsize", rc, _debug);
			if (rc) break;
		    }
#if (DB_VERSION_MAJOR == 5 && DB_VERSION_MINOR >= 3) || (DB_VERSION_MAJOR >= 6)
		    if (dbi->dbi_heap_regionsize) {
			uint32_t _npages = dbi->dbi_heap_regionsize;
/* XXX assert (_npages <= "maximum region size for the database's page size");*/
			rc = db->set_heap_regionsize(db, _npages);
			rc = cvtdberr(dbi, "db->set_heap_regionsize", rc, _debug);
			if (rc) break;
		    }
#endif
		    break;
#endif
		}
	    }

	    if (rc == 0) {
		const char * dbfullpath;
		const char * dbpath;
		char * t;
		int nb;

		nb = strlen(dbhome);
		if (dbfile)	nb += 1 + strlen(dbfile);
		dbfullpath = t = (char *) alloca(nb + 1);

		t = stpcpy(t, dbhome);
		if (dbfile)
		    t = stpcpy( stpcpy( t, "/"), dbfile);
#ifdef	HACK	/* XXX necessary to support dbsubfile */
		dbpath = (!dbi->dbi_use_dbenv && !dbi->dbi_temporary)
			? dbfullpath : dbfile;
#else
#ifdef	PLD_CHROOT
		/* XXX Make dbpath relative. */
		dbpath = (!dbi->dbi_use_dbenv)
			? dbfullpath : dbfile;
#else
		dbpath = (!dbi->dbi_temporary)
			? dbfullpath : dbfile;
#endif	/* PLD_CHROOT */
#endif	/* HACK */

		rc = (db->open)(db, _txnid, dbpath, dbsubfile,
		    dbi_type, oflags, dbi->dbi_perms);

		if (rc == 0 && dbi_type == DB_UNKNOWN) {
		    xx = db->get_type(db, &dbi_type);
		    if (xx == 0)
			dbi->dbi_type = dbi_type;
		}
	    }

	    /* XXX return rc == errno without printing */
	    _printit = (rc > 0 ? 0 : _debug);
	    xx = cvtdberr(dbi, "db->open", rc, _printit);

	    /*
	     * Lock a file using fcntl(2). Traditionally this is Packages,
	     * the file used to store metadata of installed header(s),
	     * as Packages is always opened, and should be opened first,
	     * for any rpmdb access.
	     *
	     * If no DBENV is used, then access is protected with a
	     * shared/exclusive locking scheme, as always.
	     *
	     * With a DBENV, the fcntl(2) lock is necessary only to keep
	     * the riff-raff from playing where they don't belong, as
	     * the DBENV should provide it's own locking scheme. So try to
	     * acquire a lock, but permit failures, as some other
	     * DBENV player may already have acquired the lock.
	     *
	     * With NPTL posix mutexes, revert to fcntl lock on non-functioning
	     * glibc/kernel combinations.
	     */
	    if (rc == 0 && dbi->dbi_lockdbfd &&
#if defined(DB_RPCCLIENT)
		!((dbi->dbi_ecflags & DB_RPCCLIENT) && dbi->dbi_host) &&
#endif
		(!dbi->dbi_use_dbenv || _lockdbfd++ == 0))
	    {
		int fdno = -1;

		if (!(db->fd(db, &fdno) == 0 && fdno >= 0)) {
		    rc = 1;
		} else {
		    struct flock l;
		    memset(&l, 0, sizeof(l));
		    l.l_whence = 0;
		    l.l_start = 0;
		    l.l_len = 0;
		    l.l_type = (dbi->dbi_mode & (O_RDWR|O_WRONLY))
				? F_WRLCK : F_RDLCK;
		    l.l_pid = 0;

		    rc = fcntl(fdno, F_SETLK, (void *) &l);
		    if (rc) {
			/* Warning iff using non-private CDB locking. */
			rc = ((dbi->dbi_use_dbenv &&
				(dbi->dbi_eflags & DB_INIT_CDB) &&
				!(dbi->dbi_eflags & DB_PRIVATE))
			    ? 0 : 1);
			rpmlog( (rc ? RPMLOG_ERR : RPMLOG_WARNING),
				_("cannot get %s lock on %s/%s\n"),
				((dbi->dbi_mode & (O_RDWR|O_WRONLY))
					? _("exclusive") : _("shared")),
				dbhome, (dbfile ? dbfile : ""));
		    } else if (dbfile) {
			rpmlog(RPMLOG_DEBUG,
				D_("locked   db index       %s/%s\n"),
				dbhome, dbfile);
		    }
		}
	    }
	}
    }

    dbi->dbi_db = (void *) db;
    if (db)
	db->app_private = dbi;

DBIDEBUG(dbi, (stderr, "<-- %s(%p,%s,%p) dbi %p rc %d %s\n", __FUNCTION__, rpmdb, tagName(rpmtag), dbip, dbi, rc, _OFLAGS(dbi->dbi_oflags)));

    if (rc == 0 && dbi->dbi_db != NULL && dbip != NULL) {
	dbi->dbi_vec = &db3vec;
	*dbip = dbi;
	if (dbi->dbi_primary) {
	    rpmTag Ptag = tagValue(dbi->dbi_primary);
	    dbiIndex Pdbi = NULL;
	    int (*_callback)(DB *, const DBT *, const DBT *, DBT *)
			= db3Acallback;
#ifdef	NOTYET	/* XXX KISS for now */
	    int _flags = DB_IMMUTABLE_KEY;
#else
	    int _flags = 0;
#endif
assert(Ptag == RPMDBI_PACKAGES && Ptag != rpmtag);
	    Pdbi = dbiOpen(rpmdb, Ptag, 0);
assert(Pdbi != NULL);
	    if (oflags & (DB_CREATE|DB_TRUNCATE)) _flags |= DB_CREATE;
	    xx = db3associate(Pdbi, dbi, _callback, _flags);
	}
	if (dbi->dbi_seq_id) {
	    char * end = NULL;
	    uint32_t u = (uint32_t) strtoll(dbi->dbi_seq_id, &end, 0);

	    /* Reset the Seqno counter to the next primary key */
	    if (oflags & (DB_CREATE|DB_TRUNCATE))
		dbi->dbi_seq_initial = rpmdb->db_maxkey + 1;

	    if (*end == '\0')
		xx = seqid_init(dbi,(const char *)&u, sizeof(u), (DB_SEQUENCE **) &dbi->dbi_seq);
	    else
		xx = seqid_init(dbi, dbi->dbi_seq_id, 0, (DB_SEQUENCE **) &dbi->dbi_seq);
	    if (xx) {
		(void) db3close(dbi, 0);
		dbi = NULL;
		if (dbip) *dbip = dbi;
	    }
	}
    } else {
	(void) db3close(dbi, 0);
	dbi = NULL;
	if (dbip) *dbip = dbi;
    }

    urlfn = _free(urlfn);

    /*@-nullstate -compmempass@*/
    return rc;
    /*@=nullstate =compmempass@*/
}

/** \ingroup db3
 */
/*@-exportheadervar@*/
/*@observer@*/ /*@unchecked@*/
struct _dbiVec db3vec = {
    DB_VERSION_STRING, DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH,
    db3open, db3close, db3sync, db3associate, db3associate_foreign, db3join,
    db3exists, db3seqno,
    db3copen, db3cclose, db3cdup, db3cdel, db3cget, db3cpget, db3cput, db3ccount,
    db3byteswapped, db3stat
};
/*@=exportheadervar@*/
/*@=type@*/
