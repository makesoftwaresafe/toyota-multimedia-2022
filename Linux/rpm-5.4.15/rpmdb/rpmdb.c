/** \ingroup rpmdb dbi
 * \file rpmdb/rpmdb.c
 */

#include "system.h"

#include <sys/file.h>

#include <rpmiotypes.h>
#include <rpmlog.h>
#include <rpmpgp.h>
#include <rpmurl.h>
#include <rpmhash.h>		/* hashFunctionString */
#define	_MIRE_INTERNAL
#include <rpmmacro.h>
#include <rpmsq.h>
#include <rpmsx.h>
#include <argv.h>

#define	_RPMBF_INTERNAL
#include <rpmbf.h>

#include <rpmtypes.h>
#define	_RPMTAG_INTERNAL
#include "header_internal.h"	/* XXX for HEADERFLAG_MAPPED */

#define	_RPMDB_INTERNAL
#include "rpmdb.h"

#include "pkgio.h"
#include "fprint.h"
#include "legacy.h"

#include "debug.h"

#if defined(__LCLINT__)
#define	UINT32_T	u_int32_t
#else
#define	UINT32_T	rpmuint32_t
#endif

/* XXX retrofit the *BSD typedef for the deprived. */
#if defined(__QNXNTO__)
typedef	rpmuint32_t	u_int32_t;
#endif

/*@access dbiIndexSet@*/
/*@access dbiIndexItem@*/
/*@access miRE@*/
/*@access Header@*/		/* XXX compared with NULL */
/*@access rpmmi@*/
/*@access rpmts@*/		/* XXX compared with NULL */

#ifdef __cplusplus
GENfree(dbiIndex *)
GENfree(dbiIndexSet)
GENfree(dbiIndexItem)
#endif	/* __cplusplus */

/*@unchecked@*/
int _rpmdb_debug = 0;

/*@unchecked@*/
int _rpmmi_debug = 0;

#define	_DBI_FLAGS	0
#define	_DBI_PERMS	0644
#define	_DBI_MAJOR	-1

/**
 * Return dbi index used for rpm tag.
 * @param db		rpm database
 * @param tag		rpm tag
 * @return		dbi index, 0xffffffff on error
 */
static size_t dbiTagToDbix(rpmdb db, rpmTag tag)
	/*@*/
{
    size_t dbix;

    if (db->db_tags != NULL)
    for (dbix = 0; dbix < db->db_ndbi; dbix++) {
	if (tag != db->db_tags[dbix].tag)
	    continue;
	return dbix;
    }
    return 0xffffffff;
}

/**
 * Initialize database (index, tag) tuple from configuration.
 */
/*@-exportheader@*/
static void dbiTagsInit(/*@null@*/ tagStore_t * dbiTagsP,
		/*@null@*/ size_t * dbiNTagsP)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies *dbiTagsP, *dbiNTagsP, rpmGlobalMacroContext, internalState @*/
{
/*@observer@*/
    static const char * const _dbiTagStr_default =
	"Packages:Name:Basenames:Group:Requirename:Providename:Conflictname:Triggername:Dirnames:Requireversion:Provideversion:Installtid:Sigmd5:Sha1header:Filedigests:Depends:Pubkeys";
    tagStore_t dbiTags = NULL;
    size_t dbiNTags = 0;
    char * dbiTagStr = NULL;
    char * o, * oe;
    rpmTag tag;
    size_t dbix;
    int bingo;

    dbiTagStr = rpmExpand("%{?_dbi_tags}", NULL);
    if (!(dbiTagStr && *dbiTagStr)) {
	dbiTagStr = _free(dbiTagStr);
	dbiTagStr = xstrdup(_dbiTagStr_default);
    }

#ifdef	NOISY
if (_rpmdb_debug)
fprintf(stderr, "--> %s(%p, %p) dbiTagStr %s\n", __FUNCTION__, dbiTagsP, dbiNTagsP, dbiTagStr);
#endif
    /* Always allocate package index */
    dbiTags = (tagStore_t) xcalloc(1, sizeof(*dbiTags));
    dbiTags[dbiNTags].str = xstrdup("Packages");
    dbiTags[dbiNTags].tag = RPMDBI_PACKAGES;
    dbiTags[dbiNTags].iob = NULL;
    dbiNTags++;

    for (o = dbiTagStr; o && *o; o = oe) {
	while (*o && xisspace((int)*o))
	    o++;
	if (*o == '\0')
	    break;
	for (oe = o; oe && *oe; oe++) {
	    if (xisspace((int)*oe))
		/*@innerbreak@*/ break;
	    if (oe[0] == ':' && !(oe[1] == '/' && oe[2] == '/'))
		/*@innerbreak@*/ break;
	}
	if (oe && *oe)
	    *oe++ = '\0';
	tag = tagValue(o);

	bingo = 0;
	if (dbiTags != NULL)
	for (dbix = 0; dbix < dbiNTags; dbix++) {
	    if (tag == dbiTags[dbix].tag) {
		bingo = 1;
		/*@innerbreak@*/ break;
	    }
	}
	if (bingo)
	    continue;

	dbiTags = (tagStore_t) xrealloc(dbiTags, (dbiNTags + 1) * sizeof(*dbiTags));
	dbiTags[dbiNTags].str = xstrdup(o);
	dbiTags[dbiNTags].tag = tag;
	dbiTags[dbiNTags].iob = NULL;
#ifdef	NOISY
if (_rpmdb_debug) {
fprintf(stderr, "\t%u %s(", (unsigned)dbiNTags, o);
if (tag & 0x40000000)
    fprintf(stderr, "0x%x)\n", tag);
else
    fprintf(stderr, "%d)\n", tag);
}
#endif
	dbiNTags++;
    }

    if (dbiNTagsP != NULL)
	*dbiNTagsP = dbiNTags;
    if (dbiTagsP != NULL)
	*dbiTagsP = dbiTags;
    else
	dbiTags = tagStoreFree(dbiTags, dbiNTags);
    dbiTagStr = _free(dbiTagStr);
}
/*@=exportheader@*/

/*@-redecl@*/
#define	DB1vec		NULL
#define	DB2vec		NULL

#if defined(WITH_DB)
/*@-exportheadervar -declundef @*/
/*@observer@*/ /*@unchecked@*/
extern struct _dbiVec db3vec;
/*@=exportheadervar =declundef @*/
#define	DB3vec		&db3vec
/*@=redecl@*/
#else
#define DB3vec		NULL
#endif

#if defined(HAVE_SQLITE3_H)	/* XXX test --with-sqlite=external */
/*@-exportheadervar -declundef @*/
/*@observer@*/ /*@unchecked@*/
extern struct _dbiVec sqlitevec;
/*@=exportheadervar =declundef @*/
#define	SQLITEvec	&sqlitevec
/*@=redecl@*/
#else
#define SQLITEvec	NULL
#endif

/*@-nullassign@*/
/*@observer@*/ /*@unchecked@*/
static struct _dbiVec *mydbvecs[] = {
    DB1vec, DB1vec, DB2vec, DB3vec, SQLITEvec, NULL
};
/*@=nullassign@*/

static inline int checkfd(const char * devnull, int fdno, int flags)
	/*@*/
{
    struct stat sb;
    int ret = 0;

    if (fstat(fdno, &sb) == -1 && errno == EBADF)
	ret = (open(devnull, flags) == fdno) ? 1 : 2;
    return ret;
}

dbiIndex dbiOpen(rpmdb db, rpmTag tag, /*@unused@*/ unsigned int flags)
{
    static int _oneshot = 0;
    size_t dbix;
    dbiIndex dbi = NULL;
    int _dbapi;
    int rc = 0;

    /* Insure that stdin/stdout/stderr are open, lest stderr end up in rpmdb. */
   if (!_oneshot) {
	static const char _devnull[] = "/dev/null";
/*@-noeffect@*/
#if defined(STDIN_FILENO)
	(void) checkfd(_devnull, STDIN_FILENO, O_RDONLY);
#endif
#if defined(STDOUT_FILENO)
	(void) checkfd(_devnull, STDOUT_FILENO, O_WRONLY);
#endif
#if defined(STDERR_FILENO)
	(void) checkfd(_devnull, STDERR_FILENO, O_WRONLY);
#endif
/*@=noeffect@*/
	_oneshot++;
   }

assert(db != NULL);					/* XXX sanity */
assert(db->_dbi != NULL);				/* XXX sanity */

    /* Is this index configured? */
    dbix = dbiTagToDbix(db, tag);
    if (dbix >= db->db_ndbi)
	goto exit;

    /* Is this index already open ? */
    if ((dbi = db->_dbi[dbix]) != NULL)
	goto exit;

    _dbapi = db->db_api;
assert(_dbapi == 3 || _dbapi == 4);
assert(mydbvecs[_dbapi] != NULL);

    rc = (*mydbvecs[_dbapi]->open) (db, tag, &dbi);
    if (rc) {
	static uint8_t _printed[128];
	if (!_printed[dbix & 0x1f]++)
	    rpmlog(RPMLOG_ERR,
		_("cannot open %s(%u) index: %s(%d)\n\tDB: %s\n"),
		tagName(tag), tag,
		(rc > 0 ? strerror(rc) : ""), rc,
		((mydbvecs[_dbapi]->dbv_version != NULL)
		? mydbvecs[_dbapi]->dbv_version : "unknown"));
	dbi = db3Free(dbi);
	goto exit;
    }
    db->_dbi[dbix] = dbi;

exit:

/*@-modfilesys@*/
if (_rpmdb_debug)
fprintf(stderr, "<== dbiOpen(%p, %s(%u), 0x%x) dbi %p = %p[%u:%u]\n", db, tagName(tag), tag, flags, dbi, db->_dbi, (unsigned)dbix, (unsigned)db->db_ndbi);
/*@=modfilesys@*/

/*@-compdef -nullstate@*/ /* FIX: db->_dbi may be NULL */
    return dbi;
/*@=compdef =nullstate@*/
}

/*@-redef@*/
union _dbswap {
    uint64_t ul;
    uint32_t ui;
    uint16_t us;
    uint8_t uc[8];
};
/*@=redef@*/

/*@unchecked@*/
static union _dbswap _endian = { 0x11223344 };

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

typedef struct _setSwap_s {
    union _dbswap hdr;
    union _dbswap tag;
    uint32_t fp;
} * setSwap;

/* XXX assumes hdrNum is first int in dbiIndexItem */
static int hdrNumCmp(const void * one, const void * two)
	/*@*/
{
    const int * a = (const int *) one;
    const int * b = (const int *) two;
    return (*a - *b);
}

/**
 * Append element(s) to set of index database items.
 * @param set		set of index database items
 * @param recs		array of items to append to set
 * @param nrecs		number of items
 * @param recsize	size of an array item
 * @param sortset	should resulting set be sorted?
 * @return		0 success, 1 failure (bad args)
 */
static int dbiAppendSet(dbiIndexSet set, const void * recs,
	int nrecs, size_t recsize, int sortset)
	/*@modifies *set @*/
{
    const char * rptr = (const char *) recs;
    size_t rlen = (recsize < sizeof(*(set->recs)))
		? recsize : sizeof(*(set->recs));

    if (set == NULL || recs == NULL || nrecs <= 0 || recsize == 0)
	return 1;

    set->recs = (dbiIndexItem) xrealloc(set->recs,
			(set->count + nrecs) * sizeof(*(set->recs)));

    memset(set->recs + set->count, 0, nrecs * sizeof(*(set->recs)));

    while (nrecs-- > 0) {
	/*@-mayaliasunique@*/
	memcpy(set->recs + set->count, rptr, rlen);
	/*@=mayaliasunique@*/
	rptr += recsize;
	set->count++;
    }

    if (sortset && set->count > 1)
	qsort(set->recs, set->count, sizeof(*(set->recs)), hdrNumCmp);

    return 0;
}

/* XXX transaction.c */
unsigned int dbiIndexSetCount(dbiIndexSet set) {
    return set->count;
}

/* XXX transaction.c */
uint32_t dbiIndexRecordOffset(dbiIndexSet set, unsigned int recno) {
    return set->recs[recno].hdrNum;
}

/* XXX transaction.c */
uint32_t dbiIndexRecordFileNumber(dbiIndexSet set, unsigned int recno) {
    return set->recs[recno].tagNum;
}

/* XXX transaction.c */
dbiIndexSet dbiFreeIndexSet(dbiIndexSet set) {
    if (set) {
	set->recs = _free(set->recs);
	set = _free(set);
    }
    return set;
}

struct rpmmi_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@dependent@*/ /*@null@*/
    rpmmi		mi_next;
/*@refcounted@*/
    rpmdb		mi_db;
    rpmTag		mi_rpmtag;
    dbiIndexSet		mi_set;
    DBC *		mi_dbc;
    unsigned int	mi_count;
    uint32_t		mi_setx;
    void *		mi_keyp;
    const char *	mi_primary;
    size_t		mi_keylen;
/*@refcounted@*/ /*@null@*/
    Header		mi_h;
    int			mi_sorted;
    int			mi_cflags;
    int			mi_modified;
    uint32_t		mi_prevoffset;	/* header instance (big endian) */
    uint32_t		mi_offset;	/* header instance (big endian) */
    uint32_t		mi_bntag;	/* base name tag (native endian) */
/*@refcounted@*/ /*@null@*/
    rpmbf		mi_bf;		/* Iterator instance Bloom filter. */
    int			mi_nre;
/*@only@*/ /*@null@*/
    miRE		mi_re;

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;				/*!< (unused) keep splint happy */
#endif
};

/*@unchecked@*/
static rpmdb rpmdbRock;

/*@unchecked@*/ /*@exposed@*/ /*@null@*/
static rpmmi rpmmiRock;

int rpmdbCheckTerminate(int terminate)
	/*@globals rpmdbRock, rpmmiRock @*/
	/*@modifies rpmdbRock, rpmmiRock @*/
{
    sigset_t newMask, oldMask;
    static int terminating = 0;

    if (terminating) return 1;

    (void) sigfillset(&newMask);		/* block all signals */
    (void) sigprocmask(SIG_BLOCK, &newMask, &oldMask);

    if (sigismember(&rpmsqCaught, SIGINT)
     || sigismember(&rpmsqCaught, SIGQUIT)
     || sigismember(&rpmsqCaught, SIGHUP)
     || sigismember(&rpmsqCaught, SIGTERM)
     || sigismember(&rpmsqCaught, SIGPIPE)
#ifdef	NOTYET		/* XXX todo++ */
     || sigismember(&rpmsqCaught, SIGXCPU)
     || sigismember(&rpmsqCaught, SIGXFSZ)
#endif
     || terminate)
	terminating = 1;

    if (terminating) {
	rpmdb db;
	rpmmi mi;

	while ((mi = rpmmiRock) != NULL) {
/*@i@*/	    rpmmiRock = mi->mi_next;
	    mi->mi_next = NULL;
/*@i@*/	    mi = rpmmiFree(mi);
	}

/*@-newreftrans@*/
	while ((db = rpmdbRock) != NULL) {
/*@i@*/	    rpmdbRock = db->db_next;
	    db->db_next = NULL;
	    (void) rpmdbClose(db);
	}
/*@=newreftrans@*/
    }

    (void) sigprocmask(SIG_SETMASK, &oldMask, NULL);
    return terminating;
}

int rpmdbCheckSignals(void)
{

    if (rpmdbCheckTerminate(0)) {
/*@-abstract@*/ /* sigset_t is abstract type */
	rpmlog(RPMLOG_DEBUG, D_("Exiting on signal(0x%lx) ...\n"), *((unsigned long *)&rpmsqCaught));
/*@=abstract@*/
	exit(EXIT_FAILURE);
    }
    return 0;
}

/**
 * Block all signals, returning previous signal mask.
 * @param db		rpm database
 * @retval *oldMask	previous sigset
 * @return		0 on success
 */
static int blockSignals(/*@unused@*/ rpmdb db, /*@out@*/ sigset_t * oldMask)
	/*@globals fileSystem @*/
	/*@modifies *oldMask, fileSystem @*/
{
    sigset_t newMask;

    (void) sigfillset(&newMask);		/* block all signals */
    (void) sigprocmask(SIG_BLOCK, &newMask, oldMask);
    (void) sigdelset(&newMask, SIGINT);
    (void) sigdelset(&newMask, SIGQUIT);
    (void) sigdelset(&newMask, SIGHUP);
    (void) sigdelset(&newMask, SIGTERM);
    (void) sigdelset(&newMask, SIGPIPE);
    return sigprocmask(SIG_BLOCK, &newMask, NULL);
}

/**
 * Restore signal mask.
 * @param db		rpm database
 * @param oldMask	previous sigset
 * @return		0 on success
 */
/*@mayexit@*/
static int unblockSignals(/*@unused@*/ rpmdb db, sigset_t * oldMask)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    (void) rpmdbCheckSignals();
    return sigprocmask(SIG_SETMASK, oldMask, NULL);
}

/**
 * Return header query string.
 * @warning Only compound header extensions are available here.
 * @param h		header
 * @param qfmt		header sprintf format
 * @return		header query string
 */
static inline /*@null@*/ const char * queryHeader(Header h, const char * qfmt)
	/*@globals headerCompoundFormats, fileSystem, internalState @*/
	/*@modifies h, fileSystem, internalState @*/
{
    const char * errstr = "(unkown error)";
    const char * str;

/*@-modobserver@*/
    str = headerSprintf(h, qfmt, NULL, headerCompoundFormats, &errstr);
/*@=modobserver@*/
    if (str == NULL)
	rpmlog(RPMLOG_ERR, _("incorrect format: \"%s\": %s\n"), qfmt, errstr);
    return str;
}

/**
 * Write added/removed header info into %{_hrmib_path}.
 * @param db		rpm database
 * @param h		header
 * @param adding	adding an rpmdb header?
 * @return		0 on success
 */
static int rpmdbExportHR_MIB(/*@unused@*/ rpmdb db, Header h, int adding)
	/*@globals headerCompoundFormats, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies h, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    static int oneshot;
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char * fn = NULL;
    int rc = -1;
    int xx;

    {	const char * fnfmt = rpmGetPath("%{?_hrmib_path}", NULL);
	if (fnfmt && *fnfmt)
	    fn = queryHeader(h, fnfmt);
	fnfmt = _free(fnfmt);
    }

    if (fn == NULL)
	goto exit;

    /* Lazily create the directory in chroot's if configured. */
    if (!oneshot) {
	char * _fn = xstrdup(fn);
	char * dn = dirname(_fn);
	mode_t _mode = 0755;
	uid_t _uid = 0;
	gid_t _gid = 0;
	/* If not a directory, then disable, else don't retry. */
	errno = 0;
	oneshot = (rpmioMkpath(dn, _mode, _uid, _gid) ? -1 : 1);
	_fn = _free(_fn);
    }
    /* If directory is AWOL, don't bother exporting info. */
    if (oneshot < 0)
	goto exit;

    if (adding) {
	FD_t fd = Fopen(fn, "w.fdio");

	if (fd != NULL) {
	    xx = Fclose(fd);
	    fd = NULL;
	    he->tag = RPMTAG_INSTALLTID;
	    if (headerGet(h, he, 0)) {
		struct utimbuf stamp;
		stamp.actime = he->p.ui32p[0];
		stamp.modtime = he->p.ui32p[0];
		if (!Utime(fn, &stamp))
		    rpmlog(RPMLOG_DEBUG, "  +++ %s\n", fn);
	    }
	    he->p.ptr = _free(he->p.ptr);
	}
    } else {
	if (!Unlink(fn))
	    rpmlog(RPMLOG_DEBUG, "  --- %s\n", fn);
    }
    rc = 0;

exit:
    fn = _free(fn);
    return rc;
}

#ifdef	NOTYET		/* XXX mixed {bdb,sqlite3} dbenv issues w db-6.1.19 */
static const char l10n_sql_init[] = "\
CREATE TABLE IF NOT EXISTS l10n (\n\
  k     TEXT UNIQUE PRIMARY KEY NOT NULL,\n\
  v     TEXT NOT NULL\n\
);\n\
";
static const char l10n_sql_qfmt[] =
#include "wdj_l10n_sqlite"
;

/**
 * Update added header info into %{__l10ndir} sqlite3 database.
 * @param db		rpm database
 * @param h		header
 * @param adding	adding an rpmdb header?
 * @return		0 on success
 */
static int rpmdbExportL10N_SQL(/*@unused@*/ rpmdb db, Header h, int adding)
	/*@globals headerCompoundFormats, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies h, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    static int oneshot;
    char * fn = NULL;
    char * t = NULL;
    int rc = 0;		/* XXX errors? */

    /* If directory isn't configured, don't bother. */
    fn = rpmGetPath("%{?__l10ndir:%{__l10ndir}/sqldb}", NULL);
    if (!(fn && *fn)) {
	oneshot = -1;
	goto exit;
    }

    /* Lazily create the directory in chroot's if configured. */
    if (!oneshot) {
	char * _fn = xstrdup(fn);
	char * dn = dirname(_fn);
	static mode_t _mode = 0755;
	static uid_t _uid = 0;
	static gid_t _gid = 0;
	/* If not a directory, then disable, else don't retry. */
	errno = 0;
	oneshot = (rpmioMkpath(dn, _mode, _uid, _gid) ? -1 : 1);
	_fn = _free(_fn);
	if (oneshot > 0) {
	    t = rpmExpand("%{sql -echo ", fn, ":\n", l10n_sql_init, "}", NULL);
	    t = _free(t);
	}
    }

    /* If directory is AWOL, don't bother. */
    if (oneshot < 0)
	goto exit;

    if (adding) {
	/* XXX macro expand before headerSprintf? */
	/* XXX skip gpg(...)? */
	const char * SQL = queryHeader(h, (char *)l10n_sql_qfmt);
	t = rpmExpand("%{sql -echo ", fn, ":\n",
			"BEGIN TRANSACTION;\n",
			SQL,
			"COMMIT TRANSACTION;\n",
			"}", NULL);
	t = _free(t);
	SQL = _free(SQL);
    } else
assert(0);	/* XXX remove on erase? */

exit:
    t = _free(t);
    fn = _free(fn);
    return rc;
}
#endif	/* NOTYET */

/**
 * Write added/removed header info.
 * @param db		rpm database
 * @param h		header
 * @param adding	adding an rpmdb header?
 * @return		0 on success
 */
static int rpmdbExportInfo(/*@unused@*/ rpmdb db, Header h, int adding)
	/*@globals headerCompoundFormats, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies h, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    int rc = 0;
    int xx;

    /* Add/remove stamp file in %{?_hrmib_path} (if configured). */
    xx = rpmdbExportHR_MIB(db, h, adding);

#ifdef	NOTYET		/* XXX mixed {bdb,sqlite3} dbenv issues w db-6.1.19 */
    /* Add localization %{?__l10ndir} sqlite3 database (if configured). */
    if (adding)
	xx = rpmdbExportL10N_SQL(db, h, adding);
#endif
(void)xx;

    return rc;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmdbPool;

static rpmdb rpmdbGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmdbPool, fileSystem @*/
	/*@modifies pool, _rpmdbPool, fileSystem @*/
{
    rpmdb db;

    if (_rpmdbPool == NULL) {
	_rpmdbPool = rpmioNewPool("db", sizeof(*db), -1, _rpmdb_debug,
			NULL, NULL, NULL);
	pool = _rpmdbPool;
    }
    db = (rpmdb) rpmioGetPool(pool, sizeof(*db));
    memset(((char *)db)+sizeof(db->_item), 0, sizeof(*db)-sizeof(db->_item));
    return db;
}

int rpmdbOpenAll(rpmdb db)
{
    int rc = 0;

    if (db == NULL) return -2;

  if (db->db_tags != NULL && db->_dbi != NULL) {
    size_t dbix;
    for (dbix = 0; dbix < db->db_ndbi; dbix++) {
	if ((int)db->db_tags[dbix].tag < 0)
	    continue;
	if (db->_dbi[dbix] != NULL)
	    continue;
	switch (db->db_tags[dbix].tag) {
	case RPMDBI_AVAILABLE:
	case RPMDBI_ADDED:
	case RPMDBI_REMOVED:
	case RPMDBI_DEPCACHE:
	case RPMDBI_BTREE:
	case RPMDBI_HASH:
	case RPMDBI_QUEUE:
	case RPMDBI_RECNO:
	case RPMDBI_HEAP:
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	default:
	    /*@switchbreak@*/ break;
	}
	(void) dbiOpen(db, db->db_tags[dbix].tag, db->db_flags);
    }
  }
    return rc;
}

int rpmdbBlockDBI(rpmdb db, int tag)
{
    rpmTag tagn = (rpmTag)(tag >= 0 ? tag : -tag);
    size_t dbix;

    if (db == NULL || db->_dbi == NULL)
	return 0;

    if (db->db_tags != NULL)
    for (dbix = 0; dbix < db->db_ndbi; dbix++) {
	if (db->db_tags[dbix].tag != tagn)
	    continue;
	db->db_tags[dbix].tag = (rpmTag) tag;
	return 0;
    }
    return 0;
}

int rpmdbCloseDBI(rpmdb db, int tag)
{
    size_t dbix;
    int rc = 0;

    if (db == NULL || db->_dbi == NULL)
	return 0;

    if (db->db_tags != NULL)
    for (dbix = 0; dbix < db->db_ndbi; dbix++) {
	if (db->db_tags[dbix].tag != (rpmTag)tag)
	    continue;
	if (db->_dbi[dbix] != NULL) {
	    int xx;
	    /*@-unqualifiedtrans@*/		/* FIX: double indirection. */
	    xx = dbiClose(db->_dbi[dbix], 0);
	    if (xx && rc == 0) rc = xx;
	    db->_dbi[dbix] = NULL;
	    /*@=unqualifiedtrans@*/
	}
	break;
    }
    return rc;
}

/* XXX query.c, rpminstall.c, verify.c */
/*@-incondefs@*/
int rpmdbClose(rpmdb db)
	/*@globals rpmdbRock @*/
	/*@modifies rpmdbRock @*/
{
    static const char msg[] = "rpmdbClose";
    rpmdb * prev, next;
    size_t dbix;
    int rc = 0;

    if (db == NULL)
	return rc;

    yarnPossess(db->_item.use);
/*@-modfilesys@*/
if (_rpmdb_debug)
fprintf(stderr, "--> db %p -- %ld %s at %s:%u\n", db, yarnPeekLock(db->_item.use), msg, __FILE__, __LINE__);

    /*@-usereleased@*/
    if (yarnPeekLock(db->_item.use) <= 1L) {

	if (db->_dbi)
	for (dbix = db->db_ndbi; dbix;) {
	    int xx;
	    dbix--;
	    if (db->_dbi[dbix] == NULL)
		continue;
	    /*@-unqualifiedtrans@*/		/* FIX: double indirection. */
	    xx = dbiClose(db->_dbi[dbix], 0);
	    if (xx && rc == 0) rc = xx;
	    db->_dbi[dbix] = NULL;
	    /*@=unqualifiedtrans@*/
	}
	db->db_errpfx = _free(db->db_errpfx);
	db->db_root = _free(db->db_root);
	db->db_home = _free(db->db_home);
	db->db_tags = tagStoreFree(db->db_tags, db->db_ndbi);
	db->_dbi = _free(db->_dbi);
	db->db_ndbi = 0;

/*@-newreftrans@*/
	prev = &rpmdbRock;
	while ((next = *prev) != NULL && next != db)
	    prev = &next->db_next;
	if (next) {
/*@i@*/	    *prev = next->db_next;
	    next->db_next = NULL;
	}
/*@=newreftrans@*/

	if (rpmdbRock == NULL && rpmmiRock == NULL) {
	    /* Last close uninstalls special signal handling. */
	    (void) rpmsqEnable(-SIGHUP,	NULL);
	    (void) rpmsqEnable(-SIGINT,	NULL);
	    (void) rpmsqEnable(-SIGTERM,	NULL);
	    (void) rpmsqEnable(-SIGQUIT,	NULL);
	    (void) rpmsqEnable(-SIGPIPE,	NULL);
	    /* Pending signals strike here. */
	    (void) rpmdbCheckSignals();
	}

    /*@=usereleased@*/
	db = (rpmdb)rpmioPutPool((rpmioItem)db);
    } else
	yarnTwist(db->_item.use, BY, -1);

    return rc;
}
/*@=incondefs@*/

/**
 * Return macro expanded absolute path to rpmdb.
 * @param uri		desired path
 * @return		macro expanded absolute path
 */
static const char * rpmdbURIPath(const char *uri)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char * s = rpmGetPath(uri, NULL);
    ARGV_t av = NULL;
    int xx = argvSplit(&av, s, ":");
    const char * fn = NULL;
    /* XXX av contains a colon separated path split, use the 1st path. */
    urltype ut = urlPath(av[0], &fn);
    
xx = xx;

    switch (ut) {
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	fn = xstrdup(av[0]);
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:
    case URL_IS_HTTP:
    case URL_IS_HTTPS:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	/* HACK: strip the URI prefix for these schemes. */
	fn = rpmGetPath(fn, NULL);
	break;
    }

    /* Convert relative to absolute paths. */
    if (ut != URL_IS_PATH)	/* XXX permit file:///... URI's */
    if (fn && *fn && *fn != '/') {
	char dn[PATH_MAX];
	char *t;
	dn[0] = '\0';
	if ((t = Realpath(".", dn)) != NULL) {
	    t += strlen(dn);
	    if (t > dn && t[-1] != '/')
		*t++ = '/';
	    t = stpncpy(t, fn, (sizeof(dn) - (t - dn)));
	    *t = '\0';
	    fn = _free(fn);
	    fn = rpmGetPath(dn, NULL);
	}
    }

    av = argvFree(av);
    s = _free(s);
assert(fn != NULL);
    return fn;
}

#define	_DB_ROOT	"/"
#define	_DB_HOME	"%{?_dbpath}"
#define	_DB_FLAGS	0
#define _DB_MODE	0
#define _DB_PERMS	0644

#define _DB_MAJOR	3
#define	_DB_ERRPFX	"rpmdb"

/*@-exportheader -globs -mods @*/
/*@only@*/ /*@null@*/
rpmdb rpmdbNew(/*@kept@*/ /*@null@*/ const char * root,
		/*@kept@*/ /*@null@*/ const char * home,
		int mode, mode_t perms, int flags)
	/*@*/
{
    rpmdb db = rpmdbGetPool(_rpmdbPool);
    const char * epfx = _DB_ERRPFX;

/*@-modfilesys@*/ /*@-nullpass@*/
if (_rpmdb_debug)
fprintf(stderr, "==> rpmdbNew(%s, %s, 0x%x, 0%o, 0x%x) db %p\n", root, home, mode, perms, flags, db);
/*@=modfilesys@*/ /*@=nullpass@*/

    if (!(perms & 0600)) perms = 0644;	/* XXX sanity */

    db->db_root = rpmdbURIPath( (root && *root ? root : _DB_ROOT) );
    db->db_home = rpmdbURIPath( (home && *home ? home : _DB_HOME) );

    if (!(db->db_home && db->db_home[0] && db->db_home[0] != '%')) {
	rpmlog(RPMLOG_ERR, _("no dbpath has been set\n"));
	db->db_root = _free(db->db_root);
	db->db_home = _free(db->db_home);
	db = (rpmdb) rpmioPutPool((rpmioItem)db);
	/*@-globstate@*/ return NULL; /*@=globstate@*/
    }

    db->db_flags = (flags >= 0) ? flags : _DB_FLAGS;
    db->db_mode = (mode >= 0) ? mode : _DB_MODE;
    db->db_perms = (perms > 0)	? perms : _DB_PERMS;
#if defined(WITH_DB)
    db->db_api = _DB_MAJOR;
#else
    /* XXX FIXME: hotwired sqlite3 if %_dbapi isn't defined */
    db->db_api = rpmExpandNumeric("%{?_dbapi}%{!?_dbapi:4}");
#endif
    db->db_errpfx = rpmExpand( (epfx && *epfx ? epfx : _DB_ERRPFX), NULL);

    db->db_remove_env = 0;
    db->db_chrootDone = 0;
    db->db_maxkey = 0;
    db->db_errcall = NULL;
    db->db_errfile = NULL;
    db->db_malloc = NULL;
    db->db_realloc = NULL;
    db->db_free = NULL;
    db->db_export = rpmdbExportInfo;
    db->db_h = NULL;

    db->db_next = NULL;
    db->db_opens = 0;

    db->db_dbenv = NULL;
    db->db_txn = NULL;
    db->db_logc = NULL;
    db->db_mpf = NULL;

    dbiTagsInit(&db->db_tags, &db->db_ndbi);
    db->_dbi = (dbiIndex *) xcalloc(db->db_ndbi, sizeof(*db->_dbi));

    memset(&db->db_getops, 0, sizeof(db->db_getops));
    memset(&db->db_putops, 0, sizeof(db->db_putops));
    memset(&db->db_delops, 0, sizeof(db->db_delops));

    /*@-globstate@*/
    return (rpmdb) rpmdbLink(db, __FUNCTION__);
    /*@=globstate@*/
}
/*@=exportheader =globs =mods @*/

static int rpmdbOpenDatabase(/*@null@*/ const char * prefix,
		/*@null@*/ const char * dbpath,
		int _dbapi, /*@null@*/ /*@out@*/ rpmdb *dbp,
		int mode, mode_t perms, int flags)
	/*@globals rpmdbRock, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies rpmdbRock, *dbp, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    rpmdb db;
    int rc;
    int xx;

    /* Insure that _dbapi has one of -1, 1, 2, or 3 */
    if (_dbapi < -1 || _dbapi > 4)
	_dbapi = -1;
    if (_dbapi == 0)
	_dbapi = 1;

    if (dbp)
	*dbp = NULL;
    if (mode & O_WRONLY) 
	return 1;

    db = rpmdbNew(prefix, dbpath, mode, perms, flags);
    if (db == NULL)
	return 1;

    if (rpmdbRock == NULL && rpmmiRock == NULL) {
	/* First open installs special signal handling. */
	(void) rpmsqEnable(SIGHUP,	NULL);
	(void) rpmsqEnable(SIGINT,	NULL);
	(void) rpmsqEnable(SIGTERM,	NULL);
	(void) rpmsqEnable(SIGQUIT,	NULL);
	(void) rpmsqEnable(SIGPIPE,	NULL);
    }

/*@-assignexpose -newreftrans@*/
/*@i@*/	db->db_next = rpmdbRock;
	rpmdbRock = db;
/*@=assignexpose =newreftrans@*/

    db->db_api = _dbapi;

    {	size_t dbix;

	rc = 0;
	if (db->db_tags != NULL)
	for (dbix = 0; rc == 0 && dbix < db->db_ndbi; dbix++) {
	    tagStore_t dbiTag = db->db_tags + dbix;
	    rpmTag tag = dbiTag->tag;
	    dbiIndex dbi;

	    /* Filter out temporary databases */
	    switch (tag) {
	    case RPMDBI_AVAILABLE:
	    case RPMDBI_ADDED:
	    case RPMDBI_REMOVED:
	    case RPMDBI_DEPCACHE:
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    default:
		/*@switchbreak@*/ break;
	    }

	    dbi = dbiOpen(db, tag, 0);
	    if (dbi == NULL) {
		rc = -2;
		break;
	    }

	    switch (tag) {
	    case RPMDBI_PACKAGES:
		if (dbi == NULL) rc |= 1;
#if 0
		/* XXX open only Packages, indices created on the fly. */
		if (db->db_api == 3)
#endif
		    goto exit;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    case RPMTAG_NAME:
		if (dbi == NULL) rc |= 1;
		/*@switchbreak@*/ break;
	    default:
		/*@switchbreak@*/ break;
	    }
	}
    }

exit:
    if (rc || dbp == NULL)
	xx = rpmdbClose(db);
    else {
/*@-assignexpose -newreftrans@*/
/*@i@*/	*dbp = db;
/*@=assignexpose =newreftrans@*/
    }

    return rc;
}

/* XXX python/rpmmodule.c */
int rpmdbOpen (const char * prefix, rpmdb *dbp, int mode, mode_t perms)
{
    int _dbapi;
#if defined(WITH_DB)
    _dbapi = _DB_MAJOR;
#elif defined(WITH_SQLITE)
    _dbapi = 4;		/* XXX FIXME: hotwired sqlite3 */
#else
    _dbapi = rpmExpandNumeric("%{?_dbapi}");
#endif
    return rpmdbOpenDatabase(prefix, NULL, _dbapi, dbp, mode, perms, 0);
}

int rpmdbCount(rpmdb db, rpmTag tag, const void * keyp, size_t keylen)
{
    unsigned int count = 0;
    DBC * dbcursor = NULL;
    DBT k = DBT_INIT;
    DBT v = DBT_INIT;
    dbiIndex dbi;
    int rc;
    int xx;

    if (db == NULL || keyp == NULL)
	return 0;

    dbi = dbiOpen(db, tag, 0);
    if (dbi == NULL)
	return 0;

    if (keylen == 0)
	keylen = strlen((char *)keyp);

/*@-temptrans@*/
    k.data = (void *) keyp;
/*@=temptrans@*/
    k.size = (UINT32_T) keylen;

    xx = dbiCopen(dbi, dbiTxnid(dbi), &dbcursor, 0);
    rc = dbiGet(dbi, dbcursor, &k, &v, DB_SET);
    switch (rc) {
    case 0:
	rc = dbiCount(dbi, dbcursor, &count, 0);
	if (rc != 0)
	    rc = -1;
	else
	    rc = count;
	break;
    case DB_NOTFOUND:
	rc = 0;
	break;
    default:
	rpmlog(RPMLOG_ERR, _("error(%d) getting records from %s index\n"),
		rc, tagName(dbi->dbi_rpmtag));
	rc = -1;
	break;
    }
    xx = dbiCclose(dbi, dbcursor, 0);
    dbcursor = NULL;
    return rc;
}

/* XXX python/upgrade.c, install.c, uninstall.c */
int rpmdbCountPackages(rpmdb db, const char * N)
{
    return rpmdbCount(db, RPMTAG_NAME, N, strlen(N));
}

/* Return pointer to first RE character (or NUL terminator) */
static const char * stemEnd(const char * s)
	/*@*/
{
    int c;

    while ((c = (int)*s)) {
	switch (c) {
	case '.':
	case '^':
	case '$':
	case '?':
	case '*':
	case '+':
	case '|':
	case '[':
	case '(':
	case '{':
	case '\0':
	    goto exit;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case '\\':
	    s++;
	    if (*s == '\0') goto exit;
	    /*@fallthrough@*/
	default:
	    /*@switchbreak@*/ break;
	}
	s++;
    }
exit:
    return s;
}

/*@only@*/
static const char * _str2PCREpat(/*@null@*/ const char *_pre, const char *s,
		/*@null@*/ const char *_post)
	/*@*/
{
    static const char _REchars[] = "^.*(|)[]+?{}$";
    size_t nt = 0;
    const char * se;
    char * t;
    char * te;

    /* Find the PCRE pattern length, including escapes. */
    for (se = s; *se != '\0'; se++, nt++)
	if (strchr(_REchars, *se)) nt++;
    nt += strlen(_pre) + strlen(_post);

    /* Build the PCRE pattern, escaping characters as needed. */
    te = t = (char *) xmalloc(nt + 1);
    te = stpcpy(te, _pre);
    for (se = s; *se != '\0'; *te++ = *se++)
	if (strchr(_REchars, *se)) *te++ = '\\';
    te = stpcpy(te, _post);
    *te = '\0';

/*@-dependenttrans@*/
    return t;
/*@=dependenttrans@*/
}

/**
 * Retrieve prinary/secondary keys for  a pattern match.
 * @todo Move to Berkeley DB db3.c when dbiIndexSet is eliminated.
 * @param db		rpm database
 * @param tag		rpm tag
 * @param mode		type of pattern match
 * @param pat		pattern to match (NULL iterates all keys).
 * @retval *matches	array or primary keys that match (or NULL)
 * @retval *argvp       array of secondary keys that match (or NULL)
 */
static int dbiMireKeys(rpmdb db, rpmTag tag, rpmMireMode mode,
		/*@null@*/ const char * pat,
		/*@null@*/ dbiIndexSet * matches,
		/*@null@*/ const char *** argvp)
	/*@globals internalState @*/
	/*@modifies *matches, *argvp, internalState @*/
{
    DBC * dbcursor = NULL;
    DBT k = DBT_INIT;
    DBT p = DBT_INIT;
    DBT v = DBT_INIT;
    dbiIndex dbi;
    miRE mire = NULL;
    uint32_t _flags = DB_NEXT;
    ARGV_t av = NULL;
    dbiIndexSet set = NULL;
    const char * b = NULL;
    size_t nb = 0;
    int ret = 1;		/* assume error */
    int rc;
    int xx;

    dbi = dbiOpen(db, tag, 0);
    if (dbi == NULL)
	goto exit;

if (_rpmmi_debug || dbi->dbi_debug)
fprintf(stderr, "--> %s(%p, %s(%u), %d, \"%s\", %p, %p)\n", __FUNCTION__, db, tagName(tag), (unsigned)tag, mode, pat, matches, argvp);

    if (pat) {

        mire = mireNew(mode, 0);
        xx = mireRegcomp(mire, pat);

	/* Initialize the secondary retrieval key. */
	switch (mode) {
	default:
assert(0);			/* XXX sanity */
	    /*@notreached@*/ break;
	case RPMMIRE_GLOB:
	    break;
	case RPMMIRE_REGEX:
	case RPMMIRE_PCRE:
	    if (*pat == '^') pat++;

	    /* If partial match on stem won't help, just iterate. */
	    nb = stemEnd(pat) - pat;
	    if (nb == 0) {
		k.doff = 0;
		goto doit;
	    }

	    /* Remove the escapes in the stem. */
	    {	char *be;
		b = be = (char *) xmalloc(nb + 1);
		while (nb--) {
		    if ((*be = *pat++) != '\\')
			be++;
		}
		*be = '\0';
	    }
	    nb = strlen(b);

	    /* Set stem length for partial match retrieve. */
	    k.flags = DB_DBT_PARTIAL;
	    k.dlen = nb;
	    k.size = nb;
	    k.data = (void *) b;
	    _flags = DB_SET_RANGE;
	    break;
	case RPMMIRE_STRCMP:
	    k.size = (UINT32_T) strlen(pat);
	    k.data = (void *) pat;
	    _flags = DB_SET;
	    break;
	}
    }

doit:
    v.flags |= DB_DBT_PARTIAL;

    xx = dbiCopen(dbi, dbiTxnid(dbi), &dbcursor, 0);

    /* Iterate over matches, collecting primary/secondary keys. */
    while ((rc = dbiPget(dbi, dbcursor, &k, &p, &v, _flags)) == 0) {
	uint32_t hdrNum;
	const char * s;
	size_t ns;

	if (_flags == DB_SET) _flags = DB_NEXT_DUP;
	if (b != NULL && nb > 0) {

	    /* Exit if the stem doesn't match. */
	    if (k.size < nb || memcmp(b, k.data, nb))
		break;

	    /* Retrieve the full record after DB_SET_RANGE. */
	    if (_flags == DB_SET_RANGE) {
		memset (&k, 0, sizeof(k));
		xx = dbiPget(dbi, dbcursor, &k, &p, &v, DB_CURRENT);
		_flags = DB_NEXT;
	    }
	}

	/* Get the secondary key. */
	s = (const char * ) k.data;
	ns = k.size;

	/* Skip if not matched. */
	if (mire && mireRegexec(mire, s, ns) < 0)
	    continue;
	    
	/* Get a native endian copy of the primary package key. */
	memcpy(&hdrNum, p.data, sizeof(hdrNum));
	hdrNum = _ntoh_ui(hdrNum);

	/* Collect primary keys. */
	if (matches) {
	    if (set == NULL)
		set = (dbiIndexSet) xcalloc(1, sizeof(*set));
	    /* XXX TODO: sort/uniqify set? */
	    (void) dbiAppendSet(set, &hdrNum, 1, sizeof(hdrNum), 0);
	}

	/* Collect secondary keys. */
	if (argvp) {
	    char * a = (char *) memcpy(xmalloc(ns+1), s, ns);
	    a[ns] = '\0';
	    xx = argvAdd(&av, a);
	    a = _free(a);
	}
    }

    xx = dbiCclose(dbi, dbcursor, 0);
    dbcursor = NULL;

    switch (rc) {
    case 0:
    case DB_NOTFOUND:
	ret = 0;
	break;
    default:
	rpmlog(RPMLOG_ERR, _("error(%d) getting keys from %s index\n"),
		rc, tagName(dbi->dbi_rpmtag));
	break;
    }

exit:
    if (ret == 0) {
	if (matches) {
	    /* XXX TODO: sort/uniqify set? */
	    *matches = set;
	    set = NULL;
	}
	if (argvp)
	    xx = argvAppend(argvp, av);
    }
    set = dbiFreeIndexSet(set);
    av = argvFree(av);
    b = _free(b);
    mire = mireFree(mire);
if (_rpmmi_debug || (dbi && dbi->dbi_debug))
fprintf(stderr, "<-- %s(%p, %s(%u), %d, %p, %p, %p) rc %d %p[%u]\n", __FUNCTION__, db, tagName(tag), (unsigned)tag, mode, pat, matches, argvp, ret, (matches && *matches ? (*matches)->recs : NULL), (matches && *matches ? (*matches)->count : 0));
    return ret;
}

int rpmdbMireApply(rpmdb db, rpmTag tag, rpmMireMode mode, const char * pat,
		const char *** argvp)
{
    int rc = dbiMireKeys(db, tag, mode, pat, NULL, argvp);
if (_rpmmi_debug)
fprintf(stderr, "<-- %s(%p, %s(%u), %d, \"%s\", %p) rc %d\n", __FUNCTION__, db, tagName(tag), (unsigned)tag, mode, pat, argvp, rc);
    return rc;
}

int rpmmiGrowBasename(rpmmi mi, const char * bn)
{
    rpmTag _tag = RPMTAG_BASENAMES;
    rpmMireMode _mode = RPMMIRE_STRCMP;
    dbiIndexSet set = NULL;
    unsigned int i;
    int rc = 1;		/* assume error */

    if (mi == NULL || mi->mi_db == NULL || bn == NULL || *bn == '\0')
	goto exit;

#ifdef	NOTYET
assert(mi->mi_rpmtag == _tag);
#endif
    /* Retrieve set of headers that contain the base name. */
    rc = dbiMireKeys(mi->mi_db, _tag, _mode, bn, &set, NULL);
    if (rc == 0 && set != NULL) {
	rpmuint32_t tagNum = hashFunctionString(0, bn, 0);
	/* Set tagNum to the hash of the basename. */
	for (i = 0; i < set->count; i++)
	    set->recs[i].tagNum = tagNum;
	if (mi->mi_set == NULL)
	    mi->mi_set = (dbiIndexSet) xcalloc(1, sizeof(*mi->mi_set));
	(void) dbiAppendSet(mi->mi_set, set->recs, set->count, sizeof(*set->recs), 0);
    }
    rc = 0;

exit:
if (_rpmmi_debug)
fprintf(stderr, "<-- %s(%p, \"%s\")\trc %d set %p %p[%u]\n", __FUNCTION__, mi, bn, rc, set, (set ? set->recs : NULL), (unsigned)(set ? set->count : 0));
    set = dbiFreeIndexSet(set);
    return rc;
}

/**
 * Attempt partial matches on name[-version[-release]] strings.
 * @param dbi		index database handle (always RPMTAG_NVRA)
 * @param pat		pattern to match against secondary keys
 * @retval matches	set of header instances that match
 * @return 		RPMRC_OK on match, RPMRC_NOMATCH or RPMRC_FAIL
 */
static rpmRC dbiFindMatches(dbiIndex dbi,
		const char * pat, /*@out@*/ dbiIndexSet * matches)
	/*@*/
{
    const char * s = pat;
    size_t ns = (s ? strlen(s) : 0);
    DBC * dbcursor = NULL;
    rpmRC rc = RPMRC_NOTFOUND;
    int ret;
    int xx;

    if (ns == 0) goto exit;

    xx = dbiCopen(dbi, dbiTxnid(dbi), &dbcursor, 0);

    /* Add ^...$ *RE anchors. Escape pattern characters. */
    {	rpmTag tag = dbi->dbi_rpmtag;
	rpmMireMode mode = RPMMIRE_PCRE;
	static const char _post_NVRA[] = "(-[^-]+-[^-]+\\.[^.]+|-[^-]+\\.[^.]+|\\.[^.]+|)$";
	const char * _pat;

	switch (tag) {
	default:
	    mode = RPMMIRE_PCRE;
	    _pat = _str2PCREpat("^", s, ".*$");
	    break;
	case RPMTAG_NVRA:
	    mode = RPMMIRE_PCRE;
	    _pat = (s[0] == '^' || s[ns-1] == '$')
		? xstrdup(s)
		: _str2PCREpat("^", s, _post_NVRA);
	    break;
	case RPMTAG_FILEPATHS:
	    if (s[0] == '^' || s[ns-1] == '$')
		mode = RPMMIRE_PCRE;
	    else
#ifdef NOTYET
	    if (s[0] == '/' && Glob_pattern_p(s, 1))
		mode = RPMMIRE_GLOB;
	    else
#endif
		mode = RPMMIRE_STRCMP;
	    _pat = xstrdup(s);
	    break;
	}

	ret = dbiMireKeys(dbi->dbi_rpmdb, tag, mode, _pat, matches, NULL);

	_pat = _free(_pat);
    }

    switch (ret) {
    case 0:		rc = RPMRC_OK;		break;
    case DB_NOTFOUND:	rc = RPMRC_NOTFOUND;	break;
    default:		rc = RPMRC_FAIL;
	rpmlog(RPMLOG_ERR, _("error(%d) getting records from %s index\n"),
		ret, tagName(dbi->dbi_rpmtag));
	break;
    }

    xx = dbiCclose(dbi, dbcursor, 0);
    dbcursor = NULL;

exit:
/*@-unqualifiedtrans@*/ /* FIX: double indirection */
    if (rc != RPMRC_OK && matches && *matches)
	*matches = dbiFreeIndexSet(*matches);
/*@=unqualifiedtrans@*/
    return rc;
}

void * dbiStatsAccumulator(dbiIndex dbi, int opx)
{
    void * sw = NULL;
    switch (opx) {
    case 14:	/* RPMTS_OP_DBGET */
	sw = &dbi->dbi_rpmdb->db_getops;
	break;
    case 15:	/* RPMTS_OP_DBPUT */
	sw = &dbi->dbi_rpmdb->db_putops;
	break;
    default:	/* XXX wrong, but let's not return NULL. */
    case 16:	/* RPMTS_OP_DBDEL */
	sw = &dbi->dbi_rpmdb->db_delops;
	break;
    }
    return sw;
}

/**
 * Rewrite a header into packages (if necessary) and free the header.
 *   Note: this is called from a markReplacedFiles iteration, and *must*
 *   preserve the "join key" (i.e. offset) for the header.
 * @param mi		database iterator
 * @param dbi		index database handle
 * @return 		0 on success
 */
static int miFreeHeader(rpmmi mi, dbiIndex dbi)
	/*@globals fileSystem, internalState @*/
	/*@modifies mi, dbi, fileSystem, internalState @*/
{
    int rc = 0;

    if (mi == NULL || mi->mi_h == NULL)
	return 0;

    if (dbi && mi->mi_dbc && mi->mi_modified && mi->mi_prevoffset) {
	DBT k = DBT_INIT;
	DBT v = DBT_INIT;
	int xx;

/*@i@*/	k.data = (void *) &mi->mi_prevoffset;
	k.size = (UINT32_T) sizeof(mi->mi_prevoffset);
	{   size_t len = 0;
	    v.data = headerUnload(mi->mi_h, &len);
	    v.size = (UINT32_T) len;
	}

	if (v.data != NULL) {
	    sigset_t signalMask;
	    (void) blockSignals(dbi->dbi_rpmdb, &signalMask);
	    rc = dbiPut(dbi, mi->mi_dbc, &k, &v, DB_KEYLAST);
	    if (rc) {
		rpmlog(RPMLOG_ERR,
			_("error(%d) storing record h#%u into %s\n"),
			rc, (unsigned)_ntoh_ui(mi->mi_prevoffset),
			tagName(dbi->dbi_rpmtag));
	    }
	    xx = dbiSync(dbi, 0);
	    (void) unblockSignals(dbi->dbi_rpmdb, &signalMask);
	}
	v.data = _free(v.data); /* headerUnload */
	v.size = 0;
    }

    (void)headerFree(mi->mi_h);
    mi->mi_h = NULL;

/*@-nullstate@*/
    return rc;
/*@=nullstate@*/
}

static void rpmmiFini(void * _mi)
	/*@globals rpmmiRock @*/
	/*@modifies _mi, rpmmiRock @*/
{
    rpmmi mi = (rpmmi) _mi;
    rpmmi * prev, next;
    dbiIndex dbi;
    int xx;

    prev = &rpmmiRock;
    while ((next = *prev) != NULL && next != mi)
	prev = &next->mi_next;
    if (next) {
/*@i@*/	*prev = next->mi_next;
	next->mi_next = NULL;
    }

    /* XXX NOTFOUND exits traverse here w mi->mi_db == NULL. b0rked imho. */
    if (mi->mi_db) {
	dbi = dbiOpen(mi->mi_db, RPMDBI_PACKAGES, 0);
assert(dbi != NULL);				/* XXX sanity */

	xx = miFreeHeader(mi, dbi);

	if (mi->mi_dbc)
	    xx = dbiCclose(dbi, mi->mi_dbc, 0);
	mi->mi_dbc = NULL;
	/* XXX rpmdbUnlink will not do.
	 * NB: must be called after rpmmiRock cleanup.
	 */
	(void) rpmdbClose(mi->mi_db);
	mi->mi_db = NULL;
    }

    (void) mireFreeAll(mi->mi_re, mi->mi_nre);
    mi->mi_re = NULL;

    (void) rpmbfFree(mi->mi_bf);
    mi->mi_bf = NULL;
    mi->mi_set = dbiFreeIndexSet(mi->mi_set);

    mi->mi_keyp = _free(mi->mi_keyp);
    mi->mi_keylen = 0;
    mi->mi_primary = _free(mi->mi_primary);

    /* XXX this needs to be done elsewhere, not within destructor. */
    (void) rpmdbCheckSignals();
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmmiPool;

static rpmmi rpmmiGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmdbPool, fileSystem @*/
	/*@modifies pool, _rpmdbPool, fileSystem @*/
{
    rpmmi mi;

    if (_rpmmiPool == NULL) {
	_rpmmiPool = rpmioNewPool("mi", sizeof(*mi), -1, _rpmmi_debug,
			NULL, NULL, rpmmiFini);
	pool = _rpmmiPool;
    }
    mi = (rpmmi) rpmioGetPool(pool, sizeof(*mi));
    memset(((char *)mi)+sizeof(mi->_item), 0, sizeof(*mi)-sizeof(mi->_item));
    return mi;
}

uint32_t rpmmiInstance(rpmmi mi)
{
    /* Get a native endian copy of the primary package key. */
    uint32_t rc = _ntoh_ui(mi ? mi->mi_offset : 0);
if (_rpmmi_debug)
fprintf(stderr, "<-- %s(%p) rc %u\n", __FUNCTION__, mi, (unsigned)rc);
    return rc;
}

uint32_t rpmmiBNTag(rpmmi mi) {
    uint32_t rc = (mi ? mi->mi_bntag : 0);
if (_rpmmi_debug)
fprintf(stderr, "<-- %s(%p) rc %u\n", __FUNCTION__, mi, (unsigned)rc);
    return rc;
}

unsigned int rpmmiCount(rpmmi mi)
{
    unsigned int rc;
    int initDbc;

    /* XXX Secondary db associated with Packages needs cursor record count */
    if (mi && mi->mi_primary && ((initDbc = mi->mi_dbc == NULL) || mi->mi_count == 0)) {
	dbiIndex dbi = dbiOpen(mi->mi_db, mi->mi_rpmtag, 0);
	DBT k = DBT_INIT;
	DBT v = DBT_INIT;
	int xx;
	if(initDbc) {
assert(dbi != NULL);	/* XXX dbiCopen doesn't handle dbi == NULL */
	    xx = dbiCopen(dbi, dbiTxnid(dbi), &mi->mi_dbc, mi->mi_cflags);
	}
	k.data = mi->mi_keyp;
	k.size = (u_int32_t)mi->mi_keylen;
if (k.data && k.size == 0) k.size = (UINT32_T) strlen((char *)k.data);
if (k.data && k.size == 0) k.size++;	/* XXX "/" fixup. */
	if (!dbiGet(dbi, mi->mi_dbc, &k, &v, DB_SET))
	    xx = dbiCount(dbi, mi->mi_dbc, &mi->mi_count, 0);
	if(initDbc)
	    mi->mi_dbc = NULL;
    }

    rc = (mi ? mi->mi_count : 0);

if (_rpmmi_debug)
fprintf(stderr, "<-- %s(%p) rc %u\n", __FUNCTION__, mi, (unsigned)rc);
    return rc;
}

/**
 * Compare iterator selectors by rpm tag (qsort/bsearch).
 * @param a		1st iterator selector
 * @param b		2nd iterator selector
 * @return		result of comparison
 */
static int mireCmp(const void * a, const void * b)
{
/*@-castexpose @*/
    const miRE mireA = (const miRE) a;
    const miRE mireB = (const miRE) b;
/*@=castexpose @*/
    return (mireA->tag - mireB->tag);
}

/**
 * Copy pattern, escaping for appropriate mode.
 * @param tag		rpm tag
 * @retval modep	type of pattern match
 * @param pattern	pattern to duplicate
 * @return		duplicated pattern
 */
static /*@only@*/ char * mireDup(rpmTag tag, rpmMireMode *modep,
			const char * pattern)
	/*@modifies *modep @*/
	/*@requires maxSet(modep) >= 0 @*/
{
    const char * s;
    char * pat;
    char * t;
    int brackets;
    size_t nb;
    int c;

    switch (*modep) {
    default:
    case RPMMIRE_DEFAULT:
	if (tag == RPMTAG_DIRNAMES || tag == RPMTAG_BASENAMES
	 || tag == RPMTAG_FILEPATHS)
	{
	    *modep = RPMMIRE_GLOB;
	    pat = xstrdup(pattern);
	    break;
	}

	nb = strlen(pattern) + sizeof("^$");

	/* Find no. of bytes needed for pattern. */
	/* periods and plusses are escaped, splats become '.*' */
	c = (int) '\0';
	brackets = 0;
	for (s = pattern; *s != '\0'; s++) {
	    switch (*s) {
	    case '.':
	    case '+':
	    case '*':
		if (!brackets) nb++;
		/*@switchbreak@*/ break;
	    case '\\':
		s++;
		/*@switchbreak@*/ break;
	    case '[':
		brackets = 1;
		/*@switchbreak@*/ break;
	    case ']':
		if (c != (int) '[') brackets = 0;
		/*@switchbreak@*/ break;
	    }
	    c = (int) *s;
	}

	pat = t = (char *) xmalloc(nb);

	if (pattern[0] != '^') *t++ = '^';

	/* Copy pattern, escaping periods, prefixing splats with period. */
	c = (int) '\0';
	brackets = 0;
	for (s = pattern; *s != '\0'; s++, t++) {
	    switch (*s) {
	    case '.':
	    case '+':
		if (!brackets) *t++ = '\\';
		/*@switchbreak@*/ break;
	    case '*':
		if (!brackets) *t++ = '.';
		/*@switchbreak@*/ break;
	    case '\\':
		*t++ = *s++;
		/*@switchbreak@*/ break;
	    case '[':
		brackets = 1;
		/*@switchbreak@*/ break;
	    case ']':
		if (c != (int) '[') brackets = 0;
		/*@switchbreak@*/ break;
	    }
	    *t = *s;
	    c = (int) *t;
	}

	if (s > pattern && s[-1] != '$') *t++ = '$';
	*t = '\0';
	*modep = RPMMIRE_REGEX;
	break;
    case RPMMIRE_STRCMP:
    case RPMMIRE_REGEX:
    case RPMMIRE_GLOB:
	pat = xstrdup(pattern);
	break;
    }

    return pat;
}

int rpmmiAddPattern(rpmmi mi, rpmTag tag,
		rpmMireMode mode, const char * pattern)
{
    static rpmMireMode defmode = (rpmMireMode)-1;
    miRE nmire = NULL;
    miRE mire = NULL;
    const char * allpat = NULL;
    int notmatch = 0;
    int rc = 0;

    if (defmode == (rpmMireMode)-1) {
	const char *t = rpmExpand("%{?_query_selector_match}", NULL);

	if (*t == '\0' || !strcmp(t, "default"))
	    defmode = RPMMIRE_DEFAULT;
	else if (!strcmp(t, "strcmp"))
	    defmode = RPMMIRE_STRCMP;
	else if (!strcmp(t, "regex"))
	    defmode = RPMMIRE_REGEX;
	else if (!strcmp(t, "glob"))
	    defmode = RPMMIRE_GLOB;
	else
	    defmode = RPMMIRE_DEFAULT;
	t = _free(t);
     }

    if (mi == NULL || pattern == NULL)
	return rc;

    /* Leading '!' inverts pattern match sense, like "grep -v". */
    if (*pattern == '!') {
	notmatch = 1;
	pattern++;
    }

    nmire = mireNew(mode, tag);
assert(nmire != NULL);
    allpat = mireDup((rpmTag)nmire->tag, &nmire->mode, pattern);

    if (nmire->mode == RPMMIRE_DEFAULT)
	nmire->mode = defmode;

    rc = mireRegcomp(nmire, allpat);
    if (rc)
	goto exit;

    if (mi->mi_re == NULL) {
	mi->mi_re = mireGetPool(_mirePool);
	mire = mireLink(mi->mi_re);
    } else {
	yarnLock use =  mi->mi_re->_item.use;
	void *pool = mi->mi_re->_item.pool;
	mi->mi_re = (miRE) xrealloc(mi->mi_re, (mi->mi_nre + 1) * sizeof(*mi->mi_re));
if (_mire_debug)
fprintf(stderr, "    mire %p[%u] realloc\n", mi->mi_re, mi->mi_nre+1);
	mire = mi->mi_re + mi->mi_nre;
	memset(mire, 0, sizeof(*mire));
	/* XXX ensure no segfault, copy the use/pool from 1st item. */
/*@-assignexpose@*/
	mire->_item.use = use;
	mire->_item.pool = pool;
/*@=assignexpose@*/
    }
    mi->mi_nre++;
    
    mire->mode = nmire->mode;
    mire->pattern = nmire->pattern;	nmire->pattern = NULL;
    mire->preg = nmire->preg;		nmire->preg = NULL;
    mire->cflags = nmire->cflags;
    mire->eflags = nmire->eflags;
    mire->fnflags = nmire->fnflags;
    mire->tag = nmire->tag;
    mire->notmatch = notmatch;
    /* XXX todo: permit PCRE patterns to be used. */
    mire->offsets = NULL;
    mire->noffsets = 0;

    if (mi->mi_nre > 1)
	qsort(mi->mi_re, mi->mi_nre, sizeof(*mi->mi_re), mireCmp);

exit:
if (_rpmmi_debug)
fprintf(stderr, "<-- %s(%p, %u(%s), %u, \"%s\") rc %d mi_re %p[%u]\n", __FUNCTION__, mi, (unsigned)tag, tagName(tag), (unsigned)mode, pattern, rc, (mi ? mi->mi_re: NULL), (unsigned)(mi ? mi->mi_nre : 0));
    allpat = _free(allpat);
    nmire = mireFree(nmire);
    return rc;
}

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

/**
 * Convert binary blob to printable hex string.
 * @param data		binary data
 * @param size		size of data in bytes
 * @return		malloc'd hex string
 */
/*@only@*/
static char * bin2hex(const void *data, size_t size)
	/*@*/
{
    static char hex[] = "0123456789abcdef";
    const char * s = (const char *) data;
    char * t, * val;
    val = t = (char *) xmalloc(size * 2 + 1);
    while (size-- > 0) {
	unsigned i;
	i = (unsigned) *s++;
	*t++ = hex[ (i >> 4) & 0xf ];
	*t++ = hex[ (i     ) & 0xf ];
    }
    *t = '\0';

    return val;
}

/**
 * Return iterator selector match.
 * @param mi		rpm database iterator
 * @return		1 if header should be skipped
 */
/*@-onlytrans@*/	/* XXX miRE array, not refcounted. */
static int mireSkip (const rpmmi mi)
	/*@globals internalState @*/
	/*@modifies mi->mi_re, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    char numbuf[32];
    miRE mire;
    int ntags = 0;
    int nmatches = 0;
    int i;
    int rc;

    if (mi->mi_h == NULL)	/* XXX can't happen */
	return 1;

    /*
     * Apply tag tests, implicitly "||" for multiple patterns/values of a
     * single tag, implicitly "&&" between multiple tag patterns.
     */
    if ((mire = mi->mi_re) == NULL)
	return 0;

    for (i = 0; i < mi->mi_nre; i++, mire++) {
	int anymatch;

	he->tag = (rpmTag) mire->tag;

	if (!headerGet(mi->mi_h, he, 0)) {
	    if (he->tag != RPMTAG_EPOCH) {
		ntags++;
		continue;
	    }
	    he->t = RPM_UINT32_TYPE;
	    he->p.ui32p = (rpmuint32_t *) xcalloc(1, sizeof(*he->p.ui32p));
	    he->c = 1;
	}

	anymatch = 0;		/* no matches yet */
	while (1) {
	    unsigned j;
	    switch (he->t) {
	    case RPM_UINT8_TYPE:
		for (j = 0; j < (unsigned) he->c; j++) {
		    sprintf(numbuf, "%u", (unsigned) he->p.ui8p[j]);
		    rc = mireRegexec(mire, numbuf, 0);
		    if ((rc >= 0 && !mire->notmatch) || (rc < 0 && mire->notmatch))
			anymatch++;
		}
		/*@switchbreak@*/ break;
	    case RPM_UINT16_TYPE:
		for (j = 0; j < (unsigned) he->c; j++) {
		    sprintf(numbuf, "%u", (unsigned) he->p.ui16p[j]);
		    rc = mireRegexec(mire, numbuf, 0);
		    if ((rc >= 0 && !mire->notmatch) || (rc < 0 && mire->notmatch))
			anymatch++;
		}
		/*@switchbreak@*/ break;
	    case RPM_UINT32_TYPE:
		for (j = 0; j < (unsigned) he->c; j++) {
		    sprintf(numbuf, "%u", (unsigned) he->p.ui32p[j]);
		    rc = mireRegexec(mire, numbuf, 0);
		    if ((rc >= 0 && !mire->notmatch) || (rc < 0 && mire->notmatch))
			anymatch++;
		}
		/*@switchbreak@*/ break;
	    case RPM_UINT64_TYPE:
/*@-duplicatequals@*/
		for (j = 0; j < (unsigned) he->c; j++) {
		    sprintf(numbuf, "%llu", (unsigned long long)he->p.ui64p[j]);
		    rc = mireRegexec(mire, numbuf, 0);
		    if ((rc >= 0 && !mire->notmatch) || (rc < 0 && mire->notmatch))
			anymatch++;
		}
/*@=duplicatequals@*/
		/*@switchbreak@*/ break;
	    case RPM_STRING_TYPE:
		rc = mireRegexec(mire, he->p.str, 0);
		if ((rc >= 0 && !mire->notmatch) || (rc < 0 && mire->notmatch))
		    anymatch++;
		/*@switchbreak@*/ break;
	    case RPM_STRING_ARRAY_TYPE:
		for (j = 0; j < (unsigned) he->c; j++) {
		    rc = mireRegexec(mire, he->p.argv[j], 0);
		    if ((rc >= 0 && !mire->notmatch) || (rc < 0 && mire->notmatch)) {
			anymatch++;
			/*@innerbreak@*/ break;
		    }
		}
		/*@switchbreak@*/ break;
	    case RPM_BIN_TYPE:
	    {	const char * s;
assert(he->p.ptr != NULL);
		s = bin2hex(he->p.ptr, he->c);
		rc = mireRegexec(mire, s, 0);
		if ((rc >= 0 && !mire->notmatch) || (rc < 0 && mire->notmatch))
		    anymatch++;
		s = _free(s);
	    }   /*@switchbreak@*/ break;
	    case RPM_I18NSTRING_TYPE:
#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(0);
#endif
	    default:
		/*@switchbreak@*/ break;
	    }
	    if ((i+1) < mi->mi_nre && mire[0].tag == mire[1].tag) {
		i++;
		mire++;
		/*@innercontinue@*/ continue;
	    }
	    /*@innerbreak@*/ break;
	}

	he->p.ptr = _free(he->p.ptr);

	if (anymatch)
	    nmatches++;
	ntags++;
    }

    return (ntags > 0 && ntags == nmatches ? 0 : 1);
}
/*@=onlytrans@*/

int rpmmiSetRewrite(rpmmi mi, int rewrite)
{
    int rc;
    if (mi == NULL)
	return 0;
    rc = (mi->mi_cflags & DB_WRITECURSOR) ? 1 : 0;
    if (rewrite)
	mi->mi_cflags |= DB_WRITECURSOR;
    else
	mi->mi_cflags &= ~DB_WRITECURSOR;
    return rc;
}

int rpmmiSetModified(rpmmi mi, int modified)
{
    int rc;
    if (mi == NULL)
	return 0;
    rc = mi->mi_modified;
    mi->mi_modified = modified;
    return rc;
}

/*@unchecked@*/
static int _rpmmi_usermem = 1;

static int rpmmiGet(dbiIndex dbi, DBC * dbcursor, DBT * kp, DBT * pk, DBT * vp,
                unsigned int flags)
	/*@globals internalState @*/
	/*@modifies dbi, dbcursor, *kp, *pk, *vp, internalState @*/
{
    int map;
    int rc;

    switch (dbi->dbi_rpmdb->db_api) {
    default:	map = 0;		break;
    case 3:	map = _rpmmi_usermem;	break;	/* Berkeley DB */
    }

    if (map) {
	static const int _prot = PROT_READ | PROT_WRITE;
	static const int _flags = MAP_PRIVATE| MAP_ANONYMOUS;
	static const int _fdno = -1;
	static const off_t _off = 0;

	memset(vp, 0, sizeof(*vp));
	vp->flags |= DB_DBT_USERMEM;
	rc = dbiGet(dbi, dbcursor, kp, vp, flags);
	if (rc == DB_BUFFER_SMALL) {
	    size_t uhlen = vp->size;
	    void * uh = mmap(NULL, uhlen, _prot, _flags, _fdno, _off);
	    if (uh == NULL || uh == (void *)-1)
		fprintf(stderr,
		    "==> mmap(%p[%u], 0x%x, 0x%x, %d, 0x%x) error(%d): %s\n",
		    NULL, (unsigned)uhlen, _prot, _flags, _fdno, (unsigned)_off,
		    errno, strerror(errno));

	    vp->ulen = (u_int32_t)uhlen;
	    vp->data = uh;
	    if (dbi->dbi_primary && pk)
		rc = dbiPget(dbi, dbcursor, kp, pk, vp, flags);
	    else
		rc = dbiGet(dbi, dbcursor, kp, vp, flags);
	    if (rc == 0) {
		if (mprotect(uh, uhlen, PROT_READ) != 0)
		    fprintf(stderr, "==> mprotect(%p[%u],0x%x) error(%d): %s\n",
			uh, (unsigned)uhlen, PROT_READ,
			errno, strerror(errno));
	    } else {
		if (munmap(uh, uhlen) != 0)
		    fprintf(stderr, "==> munmap(%p[%u]) error(%d): %s\n",
                	uh, (unsigned)uhlen, errno, strerror(errno));
	    }
	}
    } else
	rc = dbiGet(dbi, dbcursor, kp, vp, flags);
if (_rpmmi_debug || dbi->dbi_debug)
fprintf(stderr, "<-- %s(%p(%s),%p,%p,%p,0x%x) rc %d\n", __FUNCTION__, dbi, tagName(dbi->dbi_rpmtag), dbcursor, kp, vp, flags, rc);

    return rc;
}

Header rpmmiNext(rpmmi mi)
{
    dbiIndex dbi;
    DBT k = DBT_INIT;
    DBT p = DBT_INIT;
    DBT v = DBT_INIT;
    void * uh;
    size_t uhlen;
rpmTag tag;
unsigned int _flags;
    int map;
    int rc;
    int xx;

    if (mi == NULL)
	return NULL;

    /* Find the tag to open. */
    tag = (mi->mi_set == NULL && mi->mi_primary != NULL
		? mi->mi_rpmtag : RPMDBI_PACKAGES);
    dbi = dbiOpen(mi->mi_db, tag, 0);
    if (dbi == NULL)
	return NULL;

    switch (dbi->dbi_rpmdb->db_api) {
    default:	map = 0;		break;
    case 3:	map = _rpmmi_usermem;	break;	/* Berkeley DB */
    }

if (_rpmmi_debug || dbi->dbi_debug)
fprintf(stderr, "--> %s(%p) dbi %p(%s)\n", __FUNCTION__, mi, dbi, tagName(tag));

    /*
     * Cursors are per-iterator, not per-dbi, so get a cursor for the
     * iterator on 1st call. If the iteration is to rewrite headers, and the
     * CDB model is used for the database, then the cursor needs to
     * marked with DB_WRITECURSOR as well.
     */
    if (mi->mi_dbc == NULL) {
	xx = dbiCopen(dbi, dbiTxnid(dbi), &mi->mi_dbc, mi->mi_cflags);
	k.data = mi->mi_keyp;
	k.size = (u_int32_t)mi->mi_keylen;
if (k.data && k.size == 0) k.size = (UINT32_T) strlen((char *)k.data);
if (k.data && k.size == 0) k.size++;	/* XXX "/" fixup. */
	_flags = DB_SET;
    } else
	_flags = (mi->mi_setx ? DB_NEXT_DUP : DB_SET);

next:
    if (mi->mi_set) {
	/* The set of header instances is known in advance. */
	if (!(mi->mi_setx < mi->mi_set->count))
	    return NULL;
	mi->mi_offset = _hton_ui(dbiIndexRecordOffset(mi->mi_set, mi->mi_setx));
	mi->mi_bntag = dbiIndexRecordFileNumber(mi->mi_set, mi->mi_setx);
	mi->mi_setx++;

	/* If next header is identical, return it now. */
	if (mi->mi_offset == mi->mi_prevoffset && mi->mi_h != NULL)
	    return mi->mi_h;

	/* Should this header be skipped? */
	if (mi->mi_bf != NULL
	 && rpmbfChk(mi->mi_bf, &mi->mi_offset, sizeof(mi->mi_offset)) > 0)
	    goto next;

	/* Fetch header by offset. */
	k.data = &mi->mi_offset;
	k.size = (UINT32_T)sizeof(mi->mi_offset);
	rc = rpmmiGet(dbi, mi->mi_dbc, &k, NULL, &v, DB_SET);
    }
    else if (dbi->dbi_primary) {
	rc = rpmmiGet(dbi, mi->mi_dbc, &k, &p, &v, _flags);
	switch (rc) {
	default:
assert(0);
	    /*@notreached@*/ break;
	case DB_NOTFOUND:
	    return NULL;
	    /*@notreached@*/ break;
	case 0:
	    mi->mi_setx++;
assert((size_t)p.size == sizeof(mi->mi_offset));
	    memcpy(&mi->mi_offset, p.data, sizeof(mi->mi_offset));
	    /* If next header is identical, return it now. */
	    if (mi->mi_offset == mi->mi_prevoffset && mi->mi_h != NULL)
		return mi->mi_h;
	    break;
	}
	_flags = DB_NEXT_DUP;
    }
    else {
	/* Iterating Packages database. */
assert(mi->mi_rpmtag == RPMDBI_PACKAGES);

	/* Fetch header with DB_NEXT. */
	/* Instance 0 is the largest header instance in legacy databases,
	 * and must be skipped. */
	do {
	    rc = rpmmiGet(dbi, mi->mi_dbc, &k, NULL, &v, DB_NEXT);
	    if (rc == 0) {
assert((size_t)k.size == sizeof(mi->mi_offset));
		memcpy(&mi->mi_offset, k.data, sizeof(mi->mi_offset));
	    }
	} while (rc == 0 && mi->mi_offset == 0);
    }

    /* Did the header blob load correctly? */
    if (rc)
	return NULL;

    /* Should this header be skipped? */
    if (mi->mi_set == NULL && mi->mi_bf != NULL
     && rpmbfChk(mi->mi_bf, &mi->mi_offset, sizeof(mi->mi_offset)) > 0)
	goto next;

    uh = v.data;
    uhlen = v.size;
    if (uh == NULL)
	return NULL;

    /* Rewrite current header (if necessary) and unlink. */
    xx = miFreeHeader(mi, dbi);

    if (map) {
/*@-onlytrans@*/
	mi->mi_h = headerLoad(uh);
/*@=onlytrans@*/
	if (mi->mi_h) {
	    mi->mi_h->flags |= HEADERFLAG_MAPPED;
	    mi->mi_h->flags |= HEADERFLAG_RDONLY;
	}
    } else
	mi->mi_h = headerCopyLoad(uh);

    if (mi->mi_h == NULL) {
	rpmlog(RPMLOG_ERR,
		_("rpmdb: header #%u cannot be loaded -- skipping.\n"),
		(unsigned)_ntoh_ui(mi->mi_offset));
	/* damaged header should not be reused */
	if (mi->mi_h) {
	    (void)headerFree(mi->mi_h);
	    mi->mi_h = NULL;
	}
	/* TODO: skip more mi_set records */
	goto next;
    }

    /* Skip this header if iterator selector (if any) doesn't match. */
    if (mireSkip(mi))
	goto next;

    /* Mark header with its instance number. */
    {	char origin[32];
	uint32_t hdrNum = _ntoh_ui(mi->mi_offset);
	sprintf(origin, "rpmdb (h#%u)", (unsigned)hdrNum);
	(void) headerSetOrigin(mi->mi_h, origin);
	(void) headerSetInstance(mi->mi_h, hdrNum);
    }

    mi->mi_prevoffset = mi->mi_offset;
    mi->mi_modified = 0;

/*@-compdef -retalias -retexpose -usereleased @*/
    return mi->mi_h;
/*@=compdef =retalias =retexpose =usereleased @*/
}

int rpmmiSort(rpmmi mi)
{
    int rc = 0;

    if (mi && mi->mi_set && mi->mi_set->recs && mi->mi_set->count > 0) {
    /*
     * mergesort is much (~10x with lots of identical basenames) faster
     * than pure quicksort, but glibc uses msort_with_tmp() on stack.
     */
    if (mi->mi_set->count > 1) {
#if defined(__GLIBC__)
	    qsort(mi->mi_set->recs, mi->mi_set->count,
		sizeof(*mi->mi_set->recs), hdrNumCmp);
#else
	    rpm_mergesort(mi->mi_set->recs, mi->mi_set->count,
		sizeof(*mi->mi_set->recs), hdrNumCmp);
#endif
	}
	mi->mi_sorted = 1;
#ifdef	NOTNOW
    {	struct _dbiIndexItem * rec;
	int i;
	for (i = 0, rec = mi->mi_set->recs; i < mi->mi_set->count; i++, rec++) {
	    fprintf(stderr, "\t%p[%u] =  %p: %u %u %u\n", mi->mi_set->recs,
			i, rec, rec->hdrNum, rec->tagNum, rec->fpNum);
	}
    }
#endif
    }
    return rc;
}

/* XXX TODO: a Bloom Filter on removed packages created once, not each time. */
int rpmmiPrune(rpmmi mi, uint32_t * hdrNums, int nHdrNums, int sorted)
{
    int rc = (mi == NULL || hdrNums == NULL || nHdrNums <= 0);

    if (!rc) {
	int i;
	if (mi->mi_bf == NULL) {
	    static size_t nRemoves = 2 * 8192;	/* XXX population estimate */
	    static double e = 1.0e-4;
	    size_t m = 0;
	    size_t k = 0;
	    rpmbfParams(nRemoves, e, &m, &k);
	    mi->mi_bf = rpmbfNew(m, k, 0);
	}
	for (i = 0; i < nHdrNums; i++) {
	    uint32_t mi_offset = _hton_ui(hdrNums[i]);
	    int xx = rpmbfAdd(mi->mi_bf, &mi_offset, sizeof(mi_offset));
assert(xx == 0);
	}
    }

if (_rpmmi_debug)
fprintf(stderr, "<-- %s(%p, %p[%u], %d) rc %d h# %u\n", __FUNCTION__, mi, hdrNums, (unsigned)nHdrNums, sorted, rc, (unsigned) (hdrNums ? hdrNums[0] : 0));
    return rc;
}

int rpmmiGrow(rpmmi mi, const uint32_t * hdrNums, int nHdrNums)
{
    int rc = (mi == NULL || hdrNums == NULL || nHdrNums <= 0);

    if (!rc) {
	if (mi->mi_set == NULL)
	    mi->mi_set = (dbiIndexSet) xcalloc(1, sizeof(*mi->mi_set));
	(void) dbiAppendSet(mi->mi_set, hdrNums, nHdrNums, sizeof(*hdrNums), 0);
    }

if (_rpmmi_debug)
fprintf(stderr, "<-- %s(%p, %p[%u]) rc %d h# %u\n", __FUNCTION__, mi, hdrNums, (unsigned)nHdrNums, rc, (unsigned) (hdrNums ? hdrNums[0] : 0));
    return rc;
}

/*@-dependenttrans -exposetrans -globstate @*/
rpmmi rpmmiInit(rpmdb db, rpmTag tag, const void * keyp, size_t keylen)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmmi mi = NULL;
    dbiIndexSet set = NULL;
    dbiIndex dbi = NULL;
    int usePatterns = 0;

    if (db == NULL)
	goto exit;

    (void) rpmdbCheckSignals();

    /* XXX Control for whether patterns are permitted. */
    switch (tag) {
    default:	break;
    case 2:	/* XXX HACK to remove RPMDBI_LABEL from RPM. */
	/* XXX rpmlog message warning RPMDBI is deprecated? */
	tag = RPMTAG_NVRA;
	/*@fallthrough@*/
    case RPMTAG_NVRA:
#ifdef	NOTYET		/* XXX JS unit tests break. */
    case RPMTAG_NAME:
#endif
#ifdef	RPM_VENDOR_MANDRIVA_XXX	/* XXX rpm -qf /non/existent breaks */
    case RPMTAG_PROVIDENAME:
#endif
    case RPMTAG_VERSION:
    case RPMTAG_RELEASE:
    case RPMTAG_ARCH:
    case RPMTAG_OS:
    case RPMTAG_GROUP:
	usePatterns = 1;
	break;
#ifndef	NOTYET		/* XXX can't quite do this yet */
    /* XXX HACK to remove the existing complexity of RPMTAG_BASENAMES */
    case RPMTAG_BASENAMES:
	if (keyp == NULL)	/* XXX rpmdbFindFpList & grow are speshul */
	    break;
	tag = RPMTAG_FILEPATHS;
	/*@fallthrough@*/
#endif
    case RPMTAG_FILEPATHS:
    case RPMTAG_DIRNAMES:
	usePatterns = 1;
	break;
    }

    dbi = dbiOpen(db, tag, 0);
#ifdef	NOTYET	/* XXX non-configured tag indices force NULL return */
assert(dbi != NULL);					/* XXX sanity */
#else
    if (dbi == NULL)
	goto exit;
#endif

    mi = rpmmiGetPool(_rpmmiPool);
    (void)rpmioLinkPoolItem((rpmioItem)mi, __FUNCTION__, __FILE__, __LINE__);

/* XXX FIXME: prints unprintable characters (while debugging). */
if (_rpmmi_debug || (dbi && dbi->dbi_debug))
fprintf(stderr, "--> %s(%p, %s, %p[%u]=\"%s\") dbi %p mi %p\n", __FUNCTION__, db, tagName(tag), keyp, (unsigned)keylen, (keyp != NULL && (keylen == 0 || ((const char *)keyp)[keylen] == '\0') ? (const char *)keyp : "???"), dbi, mi);

    /* Chain cursors for teardown on abnormal exit. */
    mi->mi_next = rpmmiRock;
    rpmmiRock = mi;

    if (tag == RPMDBI_PACKAGES && keyp == NULL) {
	/* Special case #1: sequentially iterate Packages database. */
	assert(keylen == 0);
	/* This should be the only case when (set == NULL). */
    }
    else if (tag == RPMDBI_PACKAGES) {
	/* Special case #2: will fetch header instance. */
	uint32_t hdrNum;
assert(keylen == sizeof(hdrNum));
	memcpy(&hdrNum, keyp, sizeof(hdrNum));
	/* The set has only one element, which is hdrNum. */
	set = (dbiIndexSet) xcalloc(1, sizeof(*set));
	set->count = 1;
	set->recs = (dbiIndexItem) xcalloc(1, sizeof(set->recs[0]));
	set->recs[0].hdrNum = hdrNum;
    }
    else if (keyp == NULL) {
	/* XXX Special case #3: empty iterator with rpmmiGrow() */
	assert(keylen == 0);
    }
    else if (usePatterns) {
	/* XXX Special case #4: gather primary keys with patterns. */
	rpmRC rc;

	rc = dbiFindMatches(dbi, (const char *)keyp, &set);
#if defined(RPM_VENDOR_MANDRIVA)
	/*
	 * Hack to workaround disttag/distepoch pattern matching issue to buy some
	 * time to come up with better pattern fix..
	 * One size should fit all now.. ;)
	 *
	 * This patch will try match NVR first, then for all matches returned,
	 * it will match disttag, distepoch & arch individually.
	 */

	/* We'll only try this if query fails */
	if(!rc && ((const char*)keyp)[0] != '^' && tag == RPMTAG_NVRA &&
	       	(set == NULL || set->count < 1)) {
	    size_t i;
	    char *tmp = (char*)keyp;

	    /* If pattern has less than three '-', it can't contain disttag, so
	     * no point in trying */
	    for (i = 0; (tmp = strchr(tmp, '-')); i++)
		tmp++;

	    if (i >= 3) {
		dbiIndex pdbi;
		DBC *pdbc;
		const char *origkeyp = keyp;
		size_t klen = strlen(keyp)+1;
		size_t size = 0;
		int xx;

		keyp = alloca(klen);
		stpcpy((char*)keyp, origkeyp);
		tmp = strrchr(keyp, '-');
		*tmp = '\0';
		rc = dbiFindMatches(dbi, keyp, &set);

		pdbi = dbiOpen(db, RPMDBI_PACKAGES, 0);
		xx = dbiCopen(pdbi, dbiTxnid(pdbi), &pdbc, 0);

		for(i = 0; set && i < set->count; i++) {
		    DBT k = DBT_INIT;
		    DBT v = DBT_INIT;
		    Header h;
		    uint32_t offset = _hton_ui(set->recs[i].hdrNum);
		    rpmTag checkTags[] =
		    { RPMTAG_DISTTAG, RPMTAG_DISTEPOCH, RPMTAG_ARCH };
		    int j;

		    memset(&k, 0, sizeof(k));
		    memset(&v, 0, sizeof(v));
		    k.data = &offset;
		    k.size = sizeof(offset);

		    xx = dbiGet(dbi, pdbc, &k, &v, DB_SET);
		    h = headerLoad(v.data);
		    tmp = (char*)((size_t)keyp + strlen(keyp) + 1);

		    for (j = 0; j < (int)(sizeof(checkTags)/sizeof(checkTags[0])) &&
			    *tmp != '\0'; j++) {
			he->tag = checkTags[j];
			if(headerGet(h, he, 0)) {
			    size_t len = strlen(he->p.str);

			    if (he->tag == RPMTAG_ARCH && *tmp == '.')
				tmp++;

			    if(!strncmp(he->p.str, tmp, len))
				tmp += len;
			    _free(he->p.ptr);
			}
		    }
		    if(j && *tmp  == '\0') {
			set->recs[size].hdrNum = set->recs[i].hdrNum;
			set->recs[size].tagNum = set->recs[i].tagNum;
			size++;
		    }

		    h = headerFree(h);
		}
		if(set && set->count != size) {
		    set->count = size;
		    set->recs = xrealloc(set->recs, size * sizeof(*set->recs));
		}

		xx = dbiCclose(pdbi, pdbc, 0);
	    }
	}
#endif

	if ((rc  && rc != RPMRC_NOTFOUND) || set == NULL || set->count < 1) { /* error or empty set */
	    set = dbiFreeIndexSet(set);
	    rpmmiRock = mi->mi_next;
	    mi->mi_next = NULL;
	    mi = (rpmmi)rpmioFreePoolItem((rpmioItem)mi, __FUNCTION__, __FILE__, __LINE__);
	    return NULL;
	}
    }
    else if (dbi && dbi->dbi_primary != NULL) {
	/* XXX Special case #5: secondary index associated w primary table. */
    }
    else {
	/* Common case: retrieve join keys. */
assert(0);
    }

/*@-assignexpose@*/
    mi->mi_db = (rpmdb) rpmdbLink(db, __FUNCTION__);
/*@=assignexpose@*/
    mi->mi_rpmtag = tag;

    mi->mi_dbc = NULL;
    mi->mi_set = set;
    mi->mi_setx = 0;
    mi->mi_count = (set ? set->count : 0);

    mi->mi_primary = (dbi && dbi->dbi_primary
		? xstrdup(dbi->dbi_primary) : NULL);

    /* Coerce/swab integer keys. Save key ind keylen in the iterator. */
    switch (tagType(tag) & 0xffff) {
    case RPM_UINT8_TYPE:
assert(keylen == sizeof(he->p.ui8p[0]));
	mi->mi_keylen = sizeof(he->p.ui32p[0]);	/* XXX coerce to uint32_t */
	mi->mi_keyp = he->p.ui32p = (rpmuint32_t *) xmalloc(mi->mi_keylen);
	he->p.ui32p[0] = 0;
	memcpy(&he->p.ui8p[3], keyp, keylen);
	break;
    case RPM_UINT16_TYPE:
assert(keylen == sizeof(he->p.ui16p[0]));
	mi->mi_keylen = sizeof(he->p.ui32p[0]);	/* XXX coerce to uint32_t */
	mi->mi_keyp = he->p.ui32p = (rpmuint32_t *) xmalloc(mi->mi_keylen);
	he->p.ui32p[0] = 0;
	memcpy(&he->p.ui16p[1], keyp, keylen);
	he->p.ui16p[1] = _hton_us(he->p.ui16p[1]);
	break;
#if !defined(__LCLINT__)	/* LCL: buggy */
    case RPM_UINT32_TYPE:
assert(keylen == sizeof(he->p.ui32p[0]));
	mi->mi_keylen = keylen;
/*@-mayaliasunique@*/
	mi->mi_keyp = memcpy((he->p.ptr = xmalloc(keylen)), keyp, keylen);
/*@=mayaliasunique@*/
	he->p.ui32p[0] = _hton_ui(he->p.ui32p[0]);
	break;
    case RPM_UINT64_TYPE:
assert(keylen == sizeof(he->p.ui64p[0]));
	mi->mi_keylen = keylen;
/*@-mayaliasunique@*/
	mi->mi_keyp = memcpy((he->p.ptr = xmalloc(keylen)), keyp, keylen);
/*@=mayaliasunique@*/
	{   uint32_t _tmp = he->p.ui32p[0];
	    he->p.ui32p[0] = _hton_ui(he->p.ui32p[1]);
	    he->p.ui32p[1] = _hton_ui(_tmp);
	}
	break;
#endif	/* !defined(__LCLINT__) */
    case RPM_I18NSTRING_TYPE:       /* XXX never occurs */
#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(0);
#endif
    case RPM_BIN_TYPE:
    case RPM_STRING_TYPE:
    case RPM_STRING_ARRAY_TYPE:
    default:
	mi->mi_keylen = keylen;
	if (keyp)
	    mi->mi_keyp = (keylen > 0
		? memcpy(xmalloc(keylen), (char *)keyp, keylen)
		: xstrdup((char *)keyp));
	else
	    mi->mi_keyp = NULL;
	break;
    }
    he->p.ptr = NULL;

    mi->mi_h = NULL;
    mi->mi_sorted = 0;
    mi->mi_cflags = 0;
    mi->mi_modified = 0;
    mi->mi_prevoffset = 0;
    mi->mi_offset = 0;
    mi->mi_nre = 0;
    mi->mi_re = NULL;

exit:
    return mi;
}
/*@=dependenttrans =exposetrans =globstate @*/

/* XXX psm.c */
int rpmdbRemove(rpmdb db, /*@unused@*/ int rid, uint32_t hdrNum,
		/*@unused@*/ rpmts ts)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    Header h = NULL;
    sigset_t signalMask;
    dbiIndex dbi;
    size_t dbix;
    int rc = RPMRC_FAIL;		/* XXX RPMRC */
    int xx;

    if (db == NULL)
	return RPMRC_OK;		/* XXX RPMRC */

    /* Retrieve header for use by associated secondary index callbacks. */
    {	rpmmi mi;
	mi = rpmmiInit(db, RPMDBI_PACKAGES, &hdrNum, sizeof(hdrNum));
	h = rpmmiNext(mi);
	if (h)
	    h = headerLink(h);
	mi = rpmmiFree(mi);
    }

    if (h == NULL) {
	rpmlog(RPMLOG_ERR, _("%s: cannot read header at 0x%x\n"),
	      "rpmdbRemove", (unsigned)hdrNum);
	return RPMRC_FAIL;		/* XXX RPMRC */
    }

    he->tag = RPMTAG_NVRA;
    xx = headerGet(h, he, 0);
    rpmlog(RPMLOG_DEBUG, "  --- h#%8u %s\n", (unsigned)hdrNum, he->p.str);
    he->p.ptr = _free(he->p.ptr);

    (void) blockSignals(db, &signalMask);

    dbix = db->db_ndbi - 1;
    if (db->db_tags != NULL)
    do {
	tagStore_t dbiTag = db->db_tags + dbix;
	DBC * dbcursor;
	DBT k;
	DBT v;
	uint32_t ui;

	dbi = NULL;
	dbcursor = NULL;
	(void) memset(&k, 0, sizeof(k));
	(void) memset(&v, 0, sizeof(v));
	(void) memset(he, 0, sizeof(*he));
	he->tag = dbiTag->tag;

	switch (he->tag) {
	default:
	    /* Don't bother if tag is not present. */
	    if (!headerGet(h, he, 0))
		/*@switchbreak@*/ break;

	    dbi = dbiOpen(db, he->tag, 0);
	    if (dbi == NULL)	goto exit;

	    he->p.ptr = _free(he->p.ptr);
	    /*@switchbreak@*/ break;
	case RPMDBI_AVAILABLE:	/* Filter out temporary databases */
	case RPMDBI_ADDED:
	case RPMDBI_REMOVED:
	case RPMDBI_DEPCACHE:
	case RPMDBI_SEQNO:
	    /*@switchbreak@*/ break;
	case RPMDBI_PACKAGES:
	    if (db->db_export != NULL)
		xx = db->db_export(db, h, 0);

	    ui = _hton_ui(hdrNum);
	    k.data = &ui;
	    k.size = (UINT32_T) sizeof(ui);

	    /* New h ref for use by associated secondary index callbacks. */
	    db->db_h = headerLink(h);

	    dbi = dbiOpen(db, he->tag, 0);
	    if (dbi == NULL)	goto exit;

	    rc = dbiCopen(dbi, dbiTxnid(dbi), &dbcursor, DB_WRITECURSOR);
	    rc = dbiGet(dbi, dbcursor, &k, &v, DB_SET);
	    if (!rc)
		rc = dbiDel(dbi, dbcursor, &k, &v, 0);
	    xx = dbiCclose(dbi, dbcursor, DB_WRITECURSOR);

	    /* Unreference db_h used by associated secondary index callbacks. */
	    (void) headerFree(db->db_h);
	    db->db_h = NULL;

	    if (!dbi->dbi_no_dbsync)
		xx = dbiSync(dbi, 0);

	    /*@switchbreak@*/ break;
	}
    } while (dbix-- > 0);

    /* Unreference header used by associated secondary index callbacks. */
    (void) headerFree(h);
    h = NULL;
    rc = RPMRC_OK;		/* XXX RPMRC */

exit:
    (void) unblockSignals(db, &signalMask);
    return rc;
}

/* XXX install.c */
int rpmdbAdd(rpmdb db, int iid, Header h, /*@unused@*/ rpmts ts)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    sigset_t signalMask;
    dbiIndex dbi;
    size_t dbix;
    uint32_t hdrNum = headerGetInstance(h);
    int rc = RPMRC_FAIL;		/* XXX RPMRC */
    int xx;

    if (db == NULL)
	return RPMRC_OK;		/* XXX RPMRC */

if (_rpmdb_debug)
fprintf(stderr, "--> %s(%p, %u, %p, %p) h# %u\n", __FUNCTION__, db, (unsigned)iid, h, ts, (unsigned)hdrNum);

assert(headerIsEntry(h, RPMTAG_REMOVETID) == 0);	/* XXX sanity */

    /* Add the install transaction id. */
    if (iid != 0 && iid != -1) {
	rpmuint32_t tid[2];
	tid[0] = iid;
	tid[1] = 0;
	he->tag = RPMTAG_INSTALLTID;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = tid;
	he->c = 2;
	if (!headerIsEntry(h, he->tag))
/*@-compmempass@*/
	   xx = headerPut(h, he, 0);
/*@=compmempass@*/
    }

/* XXX pubkeys used to set RPMTAG_PACKAGECOLOR here. */
assert(headerIsEntry(h, RPMTAG_PACKAGECOLOR) != 0);	/* XXX sanity */

    (void) blockSignals(db, &signalMask);

    /* Assign a primary Packages key for new Header's. */
    if (hdrNum == 0) {
	int64_t seqno = 0;

	dbi = dbiOpen(db, RPMDBI_SEQNO, 0);
	if (dbi == NULL) goto exit;

	if ((xx = dbiSeqno(dbi, &seqno, 0)) == 0) {
	    hdrNum = seqno;
	    (void) headerSetInstance(h, hdrNum);
	} else
	    goto exit;
    }

/* XXX ensure that the header instance is set persistently. */
if (hdrNum == 0) {
assert(hdrNum > 0);
assert(hdrNum == headerGetInstance(h));
}

    dbi = dbiOpen(db, RPMDBI_PACKAGES, 0);
    if (dbi == NULL) goto exit;

    dbix = db->db_ndbi - 1;
    if (db->db_tags != NULL)
    do {
	tagStore_t dbiTag = db->db_tags + dbix;
	DBC * dbcursor;
	DBT k;
	DBT v;
	uint32_t ui;

	dbi = NULL;
	dbcursor = NULL;
	(void) memset(&k, 0, sizeof(k));
	(void) memset(&v, 0, sizeof(v));
	(void) memset(he, 0, sizeof(*he));
	he->tag = dbiTag->tag;

	switch (he->tag) {
	default:
	    /* Don't bother if tag is not present. */
	    if (!headerGet(h, he, 0))
		/*@switchbreak@*/ break;

	    dbi = dbiOpen(db, he->tag, 0);
	    if (dbi == NULL) goto exit;

	    he->p.ptr = _free(he->p.ptr);
	    /*@switchbreak@*/ break;
	case RPMDBI_AVAILABLE:	/* Filter out temporary databases */
	case RPMDBI_ADDED:
	case RPMDBI_REMOVED:
	case RPMDBI_DEPCACHE:
	case RPMDBI_SEQNO:
	    /*@switchbreak@*/ break;
	case RPMDBI_PACKAGES:
	    if (db->db_export != NULL)
		xx = db->db_export(db, h, 1);

	    ui = _hton_ui(hdrNum);
	    k.data = (void *) &ui;
	    k.size = (UINT32_T) sizeof(ui);

	    {   size_t len = 0;
		v.data = headerUnload(h, &len);
assert(v.data != NULL);
		v.size = (UINT32_T) len;
	    }

	    /* New h ref for use by associated secondary index callbacks. */
	    db->db_h = headerLink(h);

	    dbi = dbiOpen(db, he->tag, 0);
	    if (dbi == NULL) goto exit;

	    xx = dbiCopen(dbi, dbiTxnid(dbi), &dbcursor, DB_WRITECURSOR);
	    xx = dbiPut(dbi, dbcursor, &k, &v, DB_KEYLAST);
	    xx = dbiCclose(dbi, dbcursor, DB_WRITECURSOR);

	    /* Unreference db_h used by associated secondary index callbacks. */
	    (void) headerFree(db->db_h);
	    db->db_h = NULL;

	    if (!dbi->dbi_no_dbsync)
		xx = dbiSync(dbi, 0);

	    v.data = _free(v.data); /* headerUnload */
	    v.size = 0;
	    /*@switchbreak@*/ break;
	}

    } while (dbix-- > 0);
    rc = RPMRC_OK;			/* XXX RPMRC */

exit:
    (void) unblockSignals(db, &signalMask);
    return rc;
}
