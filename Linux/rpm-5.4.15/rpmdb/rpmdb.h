#ifndef H_RPMDB
#define H_RPMDB
/*@-bounds@*/

/** \ingroup rpmdb dbi db1 db3
 * \file rpmdb/rpmdb.h
 * Access RPM indices using Berkeley DB interface(s).
 */

#include <assert.h>
#include <mire.h>

#include <rpmtypes.h>
#include <rpmtag.h>	/* XXX Header typedef */

#if defined(_RPMDB_INTERNAL)
#if defined(WITH_DB)
#include <db.h>
#else
#include "db_emu.h"
#endif
#endif

#if defined(_RPMDB_INTERNAL)
#define DBT_INIT /*@-fullinitblock@*/ {0} /*@-fullinitblock@*/ 	/* -Wno-missing-field-initializers */
#endif

/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmdb_debug;
/*@unchecked@*/
extern int _rpmmi_debug;
/*@=exportlocal@*/

#ifdef	NOTYET
/** \ingroup rpmdb
 * Database of headers and tag value indices.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmdb_s * rpmdb;

/** \ingroup rpmdb
 * Database iterator.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmmi_s * rpmmi;
#endif

/**
 */
typedef /*@abstract@*/ struct _dbiIndexItem * dbiIndexItem;

/** \ingroup rpmdb
 * A single element (i.e. inverted list from tag values) of a database.
 */
typedef /*@abstract@*/ struct _dbiIndexSet * dbiIndexSet;

/**
 */
typedef /*@abstract@*/ struct _dbiIndex * dbiIndex;

#if defined(_RPMDB_INTERNAL)
#include <rpmio.h>
#include <rpmbf.h>
#include <rpmsw.h>

#if !defined(SWIG)	/* XXX inline dbiFoo() need */
/** \ingroup dbi
 * A single item from an index database (i.e. the "data returned").
 */
struct _dbiIndexItem {
    uint32_t hdrNum;			/*!< header instance in db */
    uint32_t tagNum;			/*!< tag index in header */
};

/** \ingroup dbi
 * Items retrieved from the index database.
 */
struct _dbiIndexSet {
/*@owned@*/
    struct _dbiIndexItem * recs;	/*!< array of records */
    unsigned int count;			/*!< number of records */
};

/** \ingroup dbi
 * Private methods for accessing an index database.
 */
struct _dbiVec {
    const char * dbv_version;		/*!< DB version string */
    int dbv_major;			/*!< DB version major */
    int dbv_minor;			/*!< DB version minor */
    int dbv_patch;			/*!< DB version patch */

/** \ingroup dbi
 * Return handle for an index database.
 * @param rpmdb		rpm database
 * @param tag		rpm tag
 * @return		0 on success
 */
    int (*open) (rpmdb rpmdb, rpmTag tag, /*@out@*/ dbiIndex * dbip)
	/*@globals fileSystem @*/
	/*@modifies *dbip, fileSystem @*/;

/** \ingroup dbi
 * Close index database, and destroy database handle.
 * @param dbi		index database handle
 * @param flags		(unused)
 * @return		0 on success
 */
    int (*close) (/*@only@*/ dbiIndex dbi, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/;

/** \ingroup dbi
 * Flush pending operations to disk.
 * @param dbi		index database handle
 * @param flags		(unused)
 * @return		0 on success
 */
    int (*sync) (dbiIndex dbi, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/** \ingroup dbi
 * Associate secondary database with primary.
 * @param dbi		index database handle
 * @param dbisecondary	secondary index database handle
 * @param callback	create secondary key from primary (NULL if DB_RDONLY)
 * @param flags		DB_CREATE or 0
 * @return		0 on success
 */
    int (*associate) (dbiIndex dbi, dbiIndex dbisecondary,
                int (*callback) (DB *, const DBT *, const DBT *, DBT *),
                unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/;

/** \ingroup dbi
 * Associate foreign secondary database with primary.
 * @param dbi		index database handle
 * @param dbisecondary	secondary index database handle
 * @param callback	create secondary key from primary (NULL if DB_RDONLY)
 * @param flags		DB_CREATE or 0
 * @return		0 on success
 */
    int (*associate_foreign) (dbiIndex dbi, dbiIndex dbisecondary,
                int (*callback) (DB *, const DBT *, DBT *, const DBT *, int *),
                unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/;

/** \ingroup dbi
 * Return join cursor for list of cursors.
 * @param dbi		index database handle
 * @param curslist	NULL terminated list of database cursors
 * @retval dbcp		address of join database cursor
 * @param flags		DB_JOIN_NOSORT or 0
 * @return		0 on success
 */
    int (*join) (dbiIndex dbi, DBC ** curslist, /*@out@*/ DBC ** dbcp,
                unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcp, fileSystem @*/;

/** \ingroup dbi
 * Return whether key exists in a database.
 * @param dbi		index database handle
 * @param key		retrieve key value/length/flags
 * @param flags		usually 0
 * @return		0 if key exists, DB_NOTFOUND if not, else error
 */
    int (*exists) (dbiIndex dbi, DBT * key, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/;

/** \ingroup dbi
 * Return next sequence number.
 * @param dbi		index database handle (with attached sequence)
 * @retval *seqnop	IN: delta (0 does seqno++) OUT: returned 64bit seqno
 * @param flags		usually 0
 * @return		0 on success
 */
    int (*seqno) (dbiIndex dbi, /*@null@*/ int64_t * seqnop, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *seqnop, fileSystem @*/;

/** \ingroup dbi
 * Open database cursor.
 * @param dbi		index database handle
 * @param txnid		database transaction handle
 * @retval dbcp		address of new database cursor
 * @param dbiflags	DB_WRITECURSOR or 0
 * @return		0 on success
 */
    int (*copen) (dbiIndex dbi, /*@null@*/ DB_TXN * txnid,
			/*@out@*/ DBC ** dbcp, unsigned int dbiflags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *txnid, *dbcp, fileSystem @*/;

/** \ingroup dbi
 * Close database cursor.
 * @param dbi		index database handle
 * @param dbcursor	database cursor
 * @param flags		(unused)
 * @return		0 on success
 */
    int (*cclose) (dbiIndex dbi, /*@only@*/ DBC * dbcursor, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcursor, fileSystem @*/;

/** \ingroup dbi
 * Duplicate a database cursor.
 * @param dbi		index database handle
 * @param dbcursor	database cursor
 * @retval dbcp		address of new database cursor
 * @param flags		DB_POSITION for same position, 0 for uninitialized
 * @return		0 on success
 */
    int (*cdup) (dbiIndex dbi, DBC * dbcursor, /*@out@*/ DBC ** dbcp,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcp, fileSystem @*/;

/** \ingroup dbi
 * Delete (key,data) pair(s) using db->del or dbcursor->c_del.
 * @param dbi		index database handle
 * @param dbcursor	database cursor (NULL will use db->del)
 * @param key		delete key value/length/flags
 * @param data		delete data value/length/flags
 * @param flags		(unused)
 * @return		0 on success
 */
    int (*cdel) (dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key, DBT * data,
			unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, fileSystem @*/;

/** \ingroup dbi
 * Retrieve (key,data) pair using db->get or dbcursor->c_get.
 * @param dbi		index database handle
 * @param dbcursor	database cursor (NULL will use db->get)
 * @param key		retrieve key value/length/flags
 * @param data		retrieve data value/length/flags
 * @param flags		(unused)
 * @return		0 on success
 */
    int (*cget) (dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key, DBT * data,
			unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, *key, *data, fileSystem @*/;

/** \ingroup dbi
 * Retrieve (key,data) pair using dbcursor->c_pget.
 * @param dbi		index database handle
 * @param dbcursor	database cursor
 * @param key		secondary retrieve key value/length/flags
 * @param pkey		primary retrieve key value/length/flags
 * @param data		primary retrieve data value/length/flags
 * @param flags		DB_NEXT, DB_SET, or 0
 * @return		0 on success
 */
    int (*cpget) (dbiIndex dbi, /*@null@*/ DBC * dbcursor,
		DBT * key, DBT * pkey, DBT * data, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, *key, *pkey, *data, fileSystem @*/;

/** \ingroup dbi
 * Store (key,data) pair using db->put or dbcursor->c_put.
 * @param dbi		index database handle
 * @param dbcursor	database cursor (NULL will use db->put)
 * @param key		store key value/length/flags
 * @param data		store data value/length/flags
 * @param flags		(unused)
 * @return		0 on success
 */
    int (*cput) (dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key, DBT * data,
			unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, fileSystem @*/;

/** \ingroup dbi
 * Retrieve count of (possible) duplicate items using dbcursor->c_count.
 * @param dbi		index database handle
 * @param dbcursor	database cursor
 * @param countp	address of count
 * @param flags		(unused)
 * @return		0 on success
 */
    int (*ccount) (dbiIndex dbi, DBC * dbcursor,
			/*@out@*/ unsigned int * countp,
			unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, fileSystem @*/;

/** \ingroup dbi
 * Is database byte swapped?
 * @param dbi		index database handle
 * @return		0 no
 */
    int (*byteswapped) (dbiIndex dbi)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/** \ingroup dbi
 * Save statistics in database handle.
 * @param dbi		index database handle
 * @param flags		retrieve statistics that don't require traversal?
 * @return		0 on success
 */
    int (*stat) (dbiIndex dbi, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/;
};

/** \ingroup dbi
 * Describes an index database (implemented on Berkeley db3 functionality).
 */
struct _dbiIndex {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@relnull@*/
    const char * dbi_root;	/*!< chroot(2) component of path */
/*@null@*/
    const char * dbi_home;	/*!< directory component of path */
/*@relnull@*/
    const char * dbi_file;	/*!< file component of path */
/*@relnull@*/
    const char * dbi_subfile;
/*@null@*/
    const char * dbi_tmpdir;	/*!< temporary directory */

    int	dbi_ecflags;		/*!< db_env_create flags */
    int	dbi_cflags;		/*!< db_create flags */
    int	dbi_oeflags;		/*!< common (db,dbenv)->open flags */
    int	dbi_eflags;		/*!< dbenv->open flags */
    int	dbi_oflags;		/*!< db->open flags */
    int	dbi_tflags;		/*!< dbenv->txn_begin flags */

    int	dbi_type;		/*!< db index type */
    unsigned dbi_mode;		/*!< mode to use on open */
    int	dbi_perms;		/*!< file permission used when creating */
    long dbi_shmkey;		/*!< shared memory base key */
    int	dbi_api;		/*!< Berkeley API type */

    int	dbi_use_dbenv;		/*!< use db environment? */
    int	dbi_no_fsync;		/*!< no-op fsync for db */
    int	dbi_no_dbsync;		/*!< don't call dbiSync */
    int	dbi_lockdbfd;		/*!< do fcntl lock on db fd */
    int	dbi_temporary;		/*!< non-persistent index/table */
    int	dbi_debug;

    rpmbf dbi_bf;

/*@null@*/
    char * dbi_host;
    unsigned long dbi_cl_timeout;
    unsigned long dbi_sv_timeout;

	/* dbenv parameters */
    int	dbi_lorder;
/*@unused@*/
    /* XXX db-4.3.14 adds dbenv as 1st arg. */
    void (*db_errcall) (void * dbenv, const char *db_errpfx, char *buffer)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
/*@unused@*/ /*@shared@*/
    FILE *	dbi_errfile;
    const char * dbi_errpfx;
    int	dbi_region_init;
    unsigned int dbi_thread_count;

	/* locking sub-system parameters */
	/* logging sub-system parameters */
	/* mpool sub-system parameters */
	/* mutex sub-system parameters */
	/* replication sub-system parameters */

	/* sequences sub-system parameters */
    const char * dbi_seq_id;
    unsigned int dbi_seq_cachesize;
    unsigned int dbi_seq_flags;
    int64_t dbi_seq_initial;
    int64_t dbi_seq_min;
    int64_t dbi_seq_max;
    int64_t dbi_seqno;

	/* transaction sub-system parameters */
#if 0
    int	(*dbi_tx_recover) (DB_ENV *dbenv, DBT *log_rec,
				DB_LSN *lsnp, int redo, void *info)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
#endif
	/* dbinfo parameters */
    int	dbi_pagesize;		/*!< (fs blksize) */
    int	dbi_heapsize;		/*!< DB_HEAP */
    int	dbi_heap_regionsize;	/*!< DB_HEAP */
/*@unused@*/ /*@null@*/
    void * (*dbi_malloc) (size_t nbytes)
	/*@*/;
	/* hash access parameters */
    unsigned int dbi_h_ffactor;	/*!< */
    unsigned int (*dbi_h_hash_fcn) (DB *, const void *bytes,
				unsigned int length)
	/*@*/;
    unsigned int dbi_h_nelem;	/*!< */
    unsigned int dbi_h_flags;	/*!< DB_DUP, DB_DUPSORT */
    int (*dbi_h_dup_compare_fcn) (DB *, const DBT *, const DBT *, size_t *)
	/*@*/;
	/* btree access parameters */
    int	dbi_bt_flags;
    int	dbi_bt_minkey;
    int	(*dbi_bt_compare_fcn) (DB *, const DBT *, const DBT *, size_t *)
	/*@*/;
    int	(*dbi_bt_dup_compare_fcn) (DB *, const DBT *, const DBT *, size_t *)
	/*@*/;
    size_t (*dbi_bt_prefix_fcn) (DB *, const DBT *, const DBT *)
	/*@*/;
	/* recno access parameters */
    int	dbi_re_flags;
    int	dbi_re_delim;
    unsigned int dbi_re_len;
    int	dbi_re_pad;
    const char * dbi_re_source;
	/* queue access parameters */
    unsigned int dbi_q_extentsize;

/*@null@*/
    const char * dbi_primary;	/*!< Primary table for secondary index. */
/*@null@*/
    const char * dbi_foreign;

/*@refcounted@*/
    rpmdb dbi_rpmdb;		/*!< the parent rpm database */
    rpmTag dbi_rpmtag;		/*!< rpm tag used for index */
    size_t dbi_jlen;		/*!< size of join key */

    int dbi_table_nkeys;	/*!< Sqlite3: COUNT(key) */
    int dbi_table_exists;	/*!< Sqlite3: table exists? */

/*@only@*/ /*@relnull@*/
    void * dbi_seq;		/*!< Berkeley DB_SEQUENCE handle */
/*@only@*/ /*@relnull@*/
    void * dbi_db;		/*!< Berkeley DB handle */
/*@only@*/ /*@null@*/
    void * dbi_txnid;		/*!< Berkeley DB_TXN handle */
/*@only@*/ /*@null@*/
    void * dbi_stats;		/*!< Berkeley DB statistics */

/*@observer@*/
    const struct _dbiVec * dbi_vec;	/*!< private methods */

};
#endif	/* !defined(SWIG) */

/** \ingroup rpmdb
 * Describes the collection of index databases used by rpm.
 */
struct rpmdb_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@owned@*/ /*@relnull@*/
    const char * db_root;	/*!< rpmdb path prefix */
/*@owned@*/
    const char * db_home;	/*!< rpmdb directory path */
    int		db_flags;
    int		db_mode;	/*!< rpmdb open mode */
    int		db_perms;	/*!< rpmdb open permissions */
    int		db_api;		/*!< Berkeley API type */
/*@owned@*/
    const char * db_errpfx;	/*!< Berkeley DB error msg prefix. */

    int		db_remove_env;	/*!< Discard dbenv on close? */
    uint32_t	db_maxkey;	/*!< Max. primary key. */

    int		db_chrootDone;	/*!< If chroot(2) done, ignore db_root. */
    void (*db_errcall) (const char * db_errpfx, char * buffer)
	/*@*/;
/*@shared@*/
    FILE *	db_errfile;	/*!< Berkeley DB stderr clone. */
/*@only@*/
    void * (*db_malloc) (size_t nbytes)
	/*@*/;
/*@only@*/
    void * (*db_realloc) (/*@only@*//*@null@*/ void * ptr, size_t nbytes)
	/*@*/;
    void (*db_free) (/*@only@*/ void * ptr)
	/*@modifies *ptr @*/;

    int	(*db_export) (rpmdb db, Header h, int adding);

/*@refcounted@*/
    Header db_h;		/*!< Currently active header */

    rpmdb	db_next;	/*!< Chain of rpmdbOpen'ed rpmdb's. */
    int		db_opens;	/*!< No. of opens for this rpmdb. */

    void *	db_dbenv;	/*!< Berkeley DB_ENV handle. */
    void *	db_txn;		/*!< Berkeley DB_TXN handle */
    void *	db_logc;	/*!< Berkeley DB_LOGC handle */
    void *	db_mpf;		/*!< Berkeley DB_MPOOLFILE handle */

    tagStore_t	db_tags;	/*!< Tag name/value mappings. */
    size_t	db_ndbi;	/*!< No. of tag indices. */
/*@only@*/ /*@null@*/
    dbiIndex * _dbi;		/*!< Tag indices. */

    struct rpmop_s db_getops;	/*!< dbiGet statistics. */
    struct rpmop_s db_putops;	/*!< dbiPut statistics. */
    struct rpmop_s db_delops;	/*!< dbiDel statistics. */

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* defined(_RPMDB_INTERNAL) */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_RPMDB_INTERNAL)
/*@-exportlocal@*/
#if defined(WITH_DB) || defined(WITH_SQLITE)
/** \ingroup db3
 * Return new configured index database handle instance.
 * @param rpmdb		rpm database
 * @param tag		rpm tag
 * @return		index database handle
 */
/*@unused@*/ /*@only@*/ /*@null@*/
dbiIndex db3New(rpmdb rpmdb, rpmTag tag)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/** \ingroup db3
 * Destroy index database handle instance.
 * @param dbi		index database handle
 * @return		NULL always
 */
/*@null@*/
dbiIndex db3Free(/*@only@*/ /*@null@*/ dbiIndex dbi)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, fileSystem, internalState @*/;
#define	db3Free(_dbi)	\
    ((dbiIndex)rpmioFreePoolItem((rpmioItem)(_dbi), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup db3
 * Format db3 open flags for debugging print.
 * @param dbflags		db open flags
 * @param print_dbenv_flags	format db env flags instead?
 * @return			formatted flags (static buffer)
 */
/*@-redecl@*/
/*@exposed@*/
extern const char * prDbiOpenFlags(int dbflags, int print_dbenv_flags)
	/*@*/;
/*@=redecl@*/
#endif

/** \ingroup dbi
 * Return handle for an index database.
 * @param db		rpm database
 * @param tag		rpm tag
 * @param flags		(unused)
 * @return		index database handle
 */
/*@only@*/ /*@null@*/ dbiIndex dbiOpen(/*@null@*/ rpmdb db, rpmTag tag,
		unsigned int flags)
	/*@globals rpmGlobalMacroContext, errno, h_errno, internalState @*/
	/*@modifies db, rpmGlobalMacroContext, errno, internalState @*/;

/**
 * Return dbiStats accumulator structure.
 * @param dbi		index database handle
 * @param opx		per-rpmdb accumulator index (aka rpmtsOpX)
 * @return		per-rpmdb accumulator pointer
 */
void * dbiStatsAccumulator(dbiIndex dbi, int opx)
        /*@*/;

#if !defined(SWIG)
/*@-globuse -mustmod @*/ /* FIX: vector annotations */
/** \ingroup dbi
 * Open a database cursor.
 * @param dbi		index database handle
 * @param txnid		database transaction handle
 * @retval dbcp		returned database cursor
 * @param flags		DB_WRITECURSOR if writing, or 0
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiCopen(dbiIndex dbi, /*@null@*/ DB_TXN * txnid,
		/*@out@*/ DBC ** dbcp, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcp, fileSystem @*/
{
    return (*dbi->dbi_vec->copen) (dbi, txnid, dbcp, flags);
}

/** \ingroup dbi
 * Close a database cursor.
 * @param dbi		index database handle
 * @param dbcursor	database cursor
 * @param flags		(unused)
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiCclose(dbiIndex dbi, /*@only@*/ DBC * dbcursor, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcursor, fileSystem @*/
{
    return (*dbi->dbi_vec->cclose) (dbi, dbcursor, flags);
}

/** \ingroup dbi
 * Duplicate a database cursor.
 * @param dbi		index database handle
 * @param dbcursor	database cursor
 * @retval dbcp		address of new database cursor
 * @param flags		DB_POSITION for same position, 0 for uninitialized
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiCdup(dbiIndex dbi, DBC * dbcursor, /*@out@*/ DBC ** dbcp,
		unsigned int flags)
	/*@modifies dbi, *dbcp @*/
{
    return (*dbi->dbi_vec->cdup) (dbi, dbcursor, dbcp, flags);
}

/** \ingroup dbi
 * Delete (key,data) pair(s) from index database.
 * @param dbi		index database handle
 * @param dbcursor	database cursor (NULL will use db->del)
 * @param key		delete key value/length/flags
 * @param data		delete data value/length/flags
 * @param flags		(unused)
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiDel(dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key, DBT * data,
		unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, *dbcursor, fileSystem, internalState @*/
{
    rpmop sw = (rpmop)dbiStatsAccumulator(dbi, 16);	/* RPMTS_OP_DBDEL */
    int rc;
    assert(key->data != NULL && key->size > 0);
    (void) rpmswEnter(sw, 0);
    rc = (dbi->dbi_vec->cdel) (dbi, dbcursor, key, data, flags);
    (void) rpmswExit(sw, data->size);
    return rc;
}

/** \ingroup dbi
 * Retrieve (key,data) pair from index database.
 * @param dbi		index database handle
 * @param dbcursor	database cursor (NULL will use db->get)
 * @param key		retrieve key value/length/flags
 * @param data		retrieve data value/length/flags
 * @param flags		(unused)
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiGet(dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key, DBT * data,
		unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, *dbcursor, *key, *data, fileSystem, internalState @*/
{
    rpmop sw = (rpmop)dbiStatsAccumulator(dbi, 14);	/* RPMTS_OP_DBGET */
    int rc;
    (void) rpmswEnter(sw, 0);
    rc = (dbi->dbi_vec->cget) (dbi, dbcursor, key, data, flags);
    (void) rpmswExit(sw, data->size);
    return rc;
}

/** \ingroup dbi
 * Retrieve (key,data) pair using dbcursor->c_pget.
 * @param dbi		index database handle
 * @param dbcursor	database cursor (NULL will use db->get)
 * @param key		secondary retrieve key value/length/flags
 * @param pkey		primary retrieve key value/length/flags
 * @param data		primary retrieve data value/length/flags
 * @param flags		DB_NEXT, DB_SET, or 0
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiPget(dbiIndex dbi, /*@null@*/ DBC * dbcursor,
		DBT * key, DBT * pkey, DBT * data, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, *dbcursor, *key, *pkey, *data, fileSystem, internalState @*/
{
    rpmop sw = (rpmop)dbiStatsAccumulator(dbi, 14);	/* RPMTS_OP_DBGET */
    int rc;
    (void) rpmswEnter(sw, 0);
    rc = (dbi->dbi_vec->cpget) (dbi, dbcursor, key, pkey, data, flags);
    (void) rpmswExit(sw, data->size);
    return rc;
}

/** \ingroup dbi
 * Store (key,data) pair in index database.
 * @param dbi		index database handle
 * @param dbcursor	database cursor (NULL will use db->put)
 * @param key		store key value/length/flags
 * @param data		store data value/length/flags
 * @param flags		(unused)
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiPut(dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key, DBT * data,
		unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, *dbcursor, *key, fileSystem, internalState @*/
{
    rpmop sw = (rpmop)dbiStatsAccumulator(dbi, 15);	/* RPMTS_OP_DBPUT */
    int rc;
    assert(key->data != NULL && key->size > 0 && data->data != NULL && data->size > 0);
    (void) rpmswEnter(sw, 0);
    rc = (dbi->dbi_vec->cput) (dbi, dbcursor, key, data, flags);
    (void) rpmswExit(sw, data->size);
    return rc;
}

/** \ingroup dbi
 * Retrieve count of (possible) duplicate items.
 * @param dbi		index database handle
 * @param dbcursor	database cursor
 * @param countp	address of count
 * @param flags		(unused)
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiCount(dbiIndex dbi, DBC * dbcursor, /*@out@*/ unsigned int * countp,
		unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, fileSystem @*/
{
    return (*dbi->dbi_vec->ccount) (dbi, dbcursor, countp, flags);
}

/** \ingroup dbi
 * Close index database.
 * @param dbi		index database handle
 * @param flags		(unused)
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiClose(/*@only@*/ dbiIndex dbi, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    return (*dbi->dbi_vec->close) (dbi, flags);
}

/** \ingroup dbi
 * Flush pending operations to disk.
 * @param dbi		index database handle
 * @param flags		(unused)
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiSync (dbiIndex dbi, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    return (*dbi->dbi_vec->sync) (dbi, flags);
}

/** \ingroup dbi
 * Return whether key exists in a database.
 * @param dbi		index database handle
 * @param key		retrieve key value/length/flags
 * @param flags		usually 0
 * @return		0 if key exists, DB_NOTFOUND if not, else error
 */
/*@unused@*/ static inline
int dbiExists(dbiIndex dbi, /*@out@*/ DBT * key, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    return (*dbi->dbi_vec->exists) (dbi, key, flags);
}

/** \ingroup dbi
 * Return next sequence number.
 * @param dbi		index database handle (with attached sequence)
 * @retval *seqnop	IN: delta (0 does seqno++) OUT: returned 64bit seqno
 * @param flags		usually 0
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiSeqno(dbiIndex dbi, /*@null@*/ int64_t * seqnop, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *seqnop, fileSystem @*/
{
    return (*dbi->dbi_vec->seqno) (dbi, seqnop, flags);
}

/** \ingroup dbi
 * Associate secondary database with primary.
 * @param dbi		index database handle
 * @param dbisecondary	secondary index database handle
 * @param callback	create secondary key from primary (NULL if DB_RDONLY)
 * @param flags		DB_CREATE or 0
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiAssociate(dbiIndex dbi, dbiIndex dbisecondary,
                int (*callback) (DB *, const DBT *, const DBT *, DBT *),
                unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, fileSystem @*/
{
    return (*dbi->dbi_vec->associate) (dbi, dbisecondary, callback, flags);
}

/** \ingroup dbi
 * Return join cursor for list of cursors.
 * @param dbi		index database handle
 * @param curslist	NULL terminated list of database cursors
 * @retval dbcp		address of join database cursor
 * @param flags		DB_JOIN_NOSORT or 0
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiJoin(dbiIndex dbi, DBC ** curslist, /*@out@*/ DBC ** dbcp,
                unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcp, fileSystem @*/
{
    return (*dbi->dbi_vec->join) (dbi, curslist, dbcp, flags);
}

/** \ingroup dbi
 * Is database byte swapped?
 * @param dbi		index database handle
 * @return		0 same order, 1 swapped order
 */
/*@unused@*/ static inline
int dbiByteSwapped(dbiIndex dbi)
	/*@modifies dbi @*/
{
    return (*dbi->dbi_vec->byteswapped) (dbi);
}

/** \ingroup dbi
 * Return dbi statistics.
 * @param dbi		index database handle
 * @param flags		DB_FAST_STAT or 0
 * @return		0 on success
 */
/*@unused@*/ static inline
int dbiStat(dbiIndex dbi, unsigned int flags)
	/*@modifies dbi @*/
{
    return (*dbi->dbi_vec->stat) (dbi, flags);
}

/** \ingroup dbi
 * Return dbi transaction id.
 * @param dbi		index database handle
 * @return		transaction id
 */
/*@unused@*/ static inline /*@null@*/
DB_TXN * dbiTxnid(dbiIndex dbi)
	/*@*/
{
    rpmdb rpmdb = (dbi ? dbi->dbi_rpmdb : NULL);
    DB_TXN * _txn = (DB_TXN*)(rpmdb ? rpmdb->db_txn : NULL);
    return _txn;
}

#if defined(_RPMDB_INTERNAL)
#if defined(WITH_DB)
#if !defined(__LCLINT__)
/*@unused@*/ static inline
int rpmlkId(rpmdb rpmdb, uint32_t *_idp)
{
    DB_ENV * dbenv = (DB_ENV *) rpmdb->db_dbenv;
    int rc = (rpmdb->_dbi[0]->dbi_eflags & 0x080)
	? dbenv->lock_id(dbenv, _idp) : ENOTSUP;
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p) id 0x%x rc %d\n", "dbenv->lock_id", dbenv, _idp, *_idp, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlkIdFree(rpmdb rpmdb, uint32_t _id)
{
    DB_ENV * dbenv = (DB_ENV *)rpmdb->db_dbenv;
    int rc = (rpmdb->_dbi[0]->dbi_eflags & 0x080)
	? dbenv->lock_id_free(dbenv, _id) : ENOTSUP;
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%u) rc %d\n", "dbenv->lock_id_free", dbenv, (unsigned)_id, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlkGet(rpmdb rpmdb, DBT * _object, uint32_t _lockmode, void * _lock)
{
    DB_ENV * dbenv = (DB_ENV *)rpmdb->db_dbenv;
    uint32_t _locker = 0x12344321;
    uint32_t _flags = 0;
    int rc = (rpmdb->_dbi[0]->dbi_eflags & 0x080)
	? dbenv->lock_get(dbenv, _locker, _flags, _object, (db_lockmode_t)_lockmode, (DB_LOCK*)_lock)
	: ENOTSUP;
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,0x%x,0x%x,%p,0x%x,%p) rc %d\n", "dbenv->lock_get", dbenv, _locker, _flags, _object, _lockmode, _lock, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlkPut(rpmdb rpmdb, void * _lock)
{
    DB_ENV * dbenv = (DB_ENV *)rpmdb->db_dbenv;
    int rc = (rpmdb->_dbi[0]->dbi_eflags & 0x080)
	? dbenv->lock_put(dbenv, (DB_LOCK*)_lock)
	: ENOTSUP;
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p) rc %d\n", "dbenv->lock_put", dbenv, _lock, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlgcOpen(rpmdb rpmdb)
{
    DB_ENV * dbenv = (DB_ENV *)rpmdb->db_dbenv;
    DB_LOGC * _logc = NULL;
    uint32_t _flags = 0;
    int rc = (rpmdb->_dbi[0]->dbi_eflags & 0x100)
	? dbenv->log_cursor(dbenv, &_logc, _flags) : ENOTSUP;
    rpmdb->db_logc = (!rc ? _logc : NULL);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p,0x%x) logc %p rc %d\n", "dbenv->log_cursor", dbenv, &_logc, _flags, _logc, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlgcGet(rpmdb rpmdb, DB_LSN * _lsn, DBT * data, uint32_t flags)
{
    DB_LOGC * _logc = rpmdb->db_logc;
    int rc = (_logc ? _logc->get(_logc, _lsn, data, flags) : ENOTSUP);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p,%p,0x%x) rc %d\n", "logc->get", _logc, _lsn, data, flags, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlgcClose(rpmdb rpmdb)
{
    DB_LOGC * _logc = rpmdb->db_logc;
    uint32_t _flags = 0;
    int rc = (_logc ? _logc->close(_logc, _flags) : ENOTSUP);
    rpmdb->db_logc = NULL;
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p) rc %d\n", "logc->close", _logc, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlgcFile(rpmdb rpmdb, const DB_LSN * _lsn, char * name, size_t len)
{
    DB_ENV * dbenv = (DB_ENV*)rpmdb->db_dbenv;
    int rc = dbenv->log_file(dbenv, _lsn, name, len);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p,%p[%u]) \"%s\" rc %d\n", "dbenv->log_file", dbenv, _lsn, name, (unsigned)len, name, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlgcFlush(rpmdb rpmdb, const DB_LSN * _lsn)
{
    DB_ENV * dbenv = (DB_ENV*)rpmdb->db_dbenv;
    int rc = dbenv->log_flush(dbenv, _lsn);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p) rc %d\n", "dbenv->log_flush", dbenv, _lsn, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlgcPrintf(rpmdb rpmdb, const char * fmt, void *_A1, void *_A2, void *_A3, void *_A4, void *_A5)
{
    DB_ENV * dbenv = (DB_ENV*)rpmdb->db_dbenv;
    DB_TXN * _txnid = (DB_TXN*)rpmdb->db_txn;
    int rc = dbenv->log_printf(dbenv, _txnid, fmt, _A1, _A2, _A3, _A4, _A5);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p,\"%s\", ...) rc %d\n", "dbenv->log_printf", dbenv, _txnid, fmt, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmlgcPut(rpmdb rpmdb, DB_LSN * _lsn, const DBT * data, uint32_t flags)
{
    DB_ENV * dbenv = (DB_ENV*)rpmdb->db_dbenv;
    int rc = dbenv->log_put(dbenv, _lsn, data, flags);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p,%p,0x%x) rc %d\n", "dbenv->log_put", dbenv, _lsn, data, flags, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmmpfSyncAll(rpmdb rpmdb)
{
    DB_ENV * dbenv = (DB_ENV*)rpmdb->db_dbenv;
    DB_LSN * _lsn = NULL;
    int rc = (rpmdb->_dbi[0]->dbi_eflags & 0x100)
	? dbenv->memp_sync(dbenv, _lsn) : ENOTSUP;
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p) rc %d\n", "dbenv->memp_sync", dbenv, _lsn, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmmpfTrickle(rpmdb rpmdb)
{
    DB_ENV * dbenv = (DB_ENV*)rpmdb->db_dbenv;
    int _percent = 20;
    int _nwrote = 0;
    int rc = (rpmdb->_dbi[0]->dbi_eflags & 0x100)
	? dbenv->memp_trickle(dbenv, _percent, &_nwrote) : ENOTSUP;
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%d) nwrote %d rc %d\n", "dbenv->memp_trickle", dbenv, _percent, _nwrote, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmmpfClose(rpmdb rpmdb)
{
    DB_MPOOLFILE * mpf = rpmdb->db_mpf;
    uint32_t _flags = 0;
    int rc = (mpf ? mpf->close(mpf, _flags) : ENOTSUP);
    rpmdb->db_mpf = NULL;
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p) rc %d\n", "mpf->close", mpf, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmmpfGet(rpmdb rpmdb, uint32_t * _pgnop, uint32_t _flags, void ** _pagep)
{
    DB_MPOOLFILE * mpf = rpmdb->db_mpf;
    DB_TXN * _txnid = (DB_TXN*)rpmdb->db_txn;
    int rc = mpf->get(mpf, _pgnop, _txnid, _flags, _pagep);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p,%p,0x%x,%p) rc %d\n", "mpf->get", mpf, _pgnop, _txnid, _flags, _pagep, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmmpfOpen(rpmdb rpmdb, /*@null@*/ const char * fn, uint32_t flags)
{
    DB_ENV * dbenv = (DB_ENV*)rpmdb->db_dbenv;
    DB_MPOOLFILE * mpf = NULL;
    int _perms = rpmdb->_dbi[0]->dbi_perms;
    size_t _pagesize = BUFSIZ;
    uint32_t _fcreate_flags = 0;
    int rc = (rpmdb->_dbi[0]->dbi_eflags & 0x100)
	? dbenv->memp_fcreate(dbenv, &mpf, _fcreate_flags) : ENOTSUP;

if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,&mpf,0x%x) mpf %p rc %d\n", "dbenv->memp_fcreate", dbenv, _fcreate_flags, mpf, rc);
    if (rc) goto exit;
    rc = mpf->open(mpf, fn, flags, _perms, _pagesize);
    rpmdb->db_mpf = (!rc ? mpf : NULL);		/* FIXME: mpf memleak */
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%s,0x%x) rc %d\n", "mpf->open", mpf, fn, flags, rc);

exit:
    return rc;
}

/*@unused@*/ static inline
int rpmmpfPut(rpmdb rpmdb, void * _page, uint32_t flags)
{
    DB_MPOOLFILE * mpf = rpmdb->db_mpf;
    uint32_t _priority = DB_PRIORITY_DEFAULT;
    int rc = mpf->put(mpf, _page, (DB_CACHE_PRIORITY)_priority, flags);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p,%p,0x%x,0x%x) rc %d\n", "mpf->put", mpf, _page, _priority, flags, rc);
    return rc;
}

/*@unused@*/ static inline
int rpmmpfSync(rpmdb rpmdb)
{
    DB_MPOOLFILE * mpf = rpmdb->db_mpf;
    int rc = (mpf ? mpf->sync(mpf) : ENOTSUP);
if (_rpmdb_debug)
fprintf(stderr, "<-- %s(%p) rc %d\n", "mpf->close", mpf, rc);
    return rc;
}
#endif	/* __LCLINT__ */
#endif	/* defined(WITH_DB) */
#endif	/* _RPMDB_INTERNAL */
/*@=globuse =mustmod @*/
#endif	/* !defined(SWIG) */

/*@=exportlocal@*/

/** \ingroup dbi
 * Destroy set of index database items.
 * @param set	set of index database items
 * @return	NULL always
 */
/*@null@*/
dbiIndexSet dbiFreeIndexSet(/*@only@*/ /*@null@*/ dbiIndexSet set)
	/*@modifies set @*/;

/** \ingroup dbi
 * Count items in index database set.
 * @param set	set of index database items
 * @return	number of items
 */
unsigned int dbiIndexSetCount(dbiIndexSet set)
	/*@*/;

/** \ingroup dbi
 * Return record offset of header from element in index database set.
 * @param set	set of index database items
 * @param recno	index of item in set
 * @return	record offset of header
 */
uint32_t dbiIndexRecordOffset(dbiIndexSet set, unsigned int recno)
	/*@*/;

/** \ingroup dbi
 * Return file index from element in index database set.
 * @param set	set of index database items
 * @param recno	index of item in set
 * @return	file index
 */
uint32_t dbiIndexRecordFileNumber(dbiIndexSet set, unsigned int recno)
	/*@*/;
#endif	/* defined(_RPMDB_INTERNAL) */

/** \ingroup rpmdb
 * Unreference a database instance.
 * @param db		rpm database
 * @param msg
 * @return		NULL always
 */
/*@unused@*/ /*@null@*/
rpmdb rpmdbUnlink (/*@killref@*/ /*@only@*/ rpmdb db, const char * msg)
	/*@modifies db @*/;
#define	rpmdbUnlink(_db, _msg)	\
	((rpmdb)rpmioUnlinkPoolItem((rpmioItem)(_db), _msg, __FILE__, __LINE__))

/** \ingroup rpmdb
 * Reference a database instance.
 * @param db		rpm database
 * @param msg
 * @return		new rpm database reference
 */
/*@unused@*/ /*@newref@*/
rpmdb rpmdbLink (rpmdb db, const char * msg)
	/*@modifies db @*/;
#define	rpmdbLink(_db, _msg)	\
	((void *)rpmioLinkPoolItem((rpmioItem)(_db), _msg, __FILE__, __LINE__))

/** @todo document rpmdbNew
 */
/*@only@*/ /*@null@*/
rpmdb rpmdbNew(/*@kept@*/ /*@null@*/ const char * root,
		/*@kept@*/ /*@null@*/ const char * home,
		int mode, mode_t perms, int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Open rpm database.
 * @param prefix	path to top of install tree
 * @retval dbp		address of rpm database
 * @param mode		open(2) flags:  O_RDWR or O_RDONLY (O_CREAT also)
 * @param perms		database permissions
 * @return		0 on success
 */
int rpmdbOpen(/*@null@*/ const char * prefix, /*@null@*/ /*@out@*/ rpmdb * dbp,
		int mode, mode_t perms)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *dbp, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Block access to a single database index.
 * @param db		rpm database
 * @param tag		rpm tag (negative to block)
 * @return              0 on success
 */
int rpmdbBlockDBI(/*@null@*/ rpmdb db, int tag)
	/*@modifies db @*/;

/**
 * Close a single database index.
 * @param db		rpm database
 * @param tag		rpm tag
 * @return              0 on success
 */
int rpmdbCloseDBI(/*@null@*/ rpmdb db, int tag)
	/*@globals fileSystem @*/
	/*@modifies db, fileSystem @*/;

/** \ingroup rpmdb
 * Close all database indices and free rpmdb.
 * @param db		rpm database
 * @return		0 on success
 */
int rpmdbClose (/*@killref@*/ /*@only@*/ /*@null@*/ rpmdb db)
	/*@globals fileSystem @*/
	/*@modifies db, fileSystem @*/;

/** \ingroup rpmdb
 * Open all database indices.
 * @param db		rpm database
 * @return		0 on success
 */
/*@-exportlocal@*/
int rpmdbOpenAll (/*@null@*/ rpmdb db)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies db, rpmGlobalMacroContext, internalState @*/;
/*@=exportlocal@*/

/** \ingroup rpmdb
 * Return number of instances of key in a tag index.
 * @param db		rpm database
 * @param tag		rpm tag
 * @param keyp		key data
 * @param keylen	key data length (0 will use strlen(keyp))
 * @return		number of instances
 */
int rpmdbCount(/*@null@*/ rpmdb db, rpmTag tag,
		const void * keyp, size_t keylen)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies db, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Return number of instances of package in Name index.
 * @param db		rpm database
 * @param N		rpm package name
 * @return		number of instances
 */
int rpmdbCountPackages(/*@null@*/ rpmdb db, const char * N)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies db, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Return header instance for current position of rpmdb iterator.
 * @param mi		rpm database iterator
 * @return		current header instance
 */
uint32_t rpmmiInstance(/*@null@*/ rpmmi mi)
	/*@*/;

/** \ingroup rpmdb
 * Return basename tag for current position of rpmdb iterator.
 * @param mi		rpm database iterator
 * @return		current basename tag
 */
uint32_t rpmmiBNTag(/*@null@*/ rpmmi mi)
	/*@*/;

/** \ingroup rpmdb
 * Return number of elements in rpm database iterator.
 * @param mi		rpm database iterator
 * @return		number of elements
 */
unsigned int rpmmiCount(/*@null@*/ rpmmi mi)
	/*@*/;

/** \ingroup rpmdb
 * Append items to set of package instances to iterate.
 * @param mi		rpm database iterator
 * @param hdrNums	array of package instances
 * @param nHdrNums	number of elements in array
 * @return		0 on success, 1 on failure (bad args)
 */
int rpmmiGrow(/*@null@*/ rpmmi mi,
		/*@null@*/ const uint32_t * hdrNums, int nHdrNums)
	/*@modifies mi @*/;

/** \ingroup rpmdb
 * Append packages containing common basename to iterator.
 * @param mi		rpm database iterator
 * @param bn		basename key
 * @return		0 on success, 1 on failure (bad args)
 */
int rpmmiGrowBasename(rpmmi mi, const char * bn)
	/*@modifies mi @*/;

/** \ingroup rpmdb
 * Sort iterator instances.
 * @param mi		rpm database iterator
 * @return		0 on success
 */
int rpmmiSort(/*@null@*/ rpmmi mi)
	/*@modifies mi @*/;

/** \ingroup rpmdb
 * Remove items from set of package instances to iterate.
 * @note Sorted hdrNums are always passed in rpmlib.
 * @param mi		rpm database iterator
 * @param hdrNums	array of package instances
 * @param nHdrNums	number of elements in array
 * @param sorted	is the array sorted? (array will be sorted on return)
 * @return		0 on success, 1 on failure (bad args)
 */
int rpmmiPrune(/*@null@*/ rpmmi mi,
		/*@null@*/ uint32_t * hdrNums, int nHdrNums, int sorted)
	/*@modifies mi, hdrNums @*/;

/** \ingroup rpmdb
 * Add pattern to iterator selector.
 * @param mi		rpm database iterator
 * @param tag		rpm tag
 * @param mode		type of pattern match
 * @param pattern	pattern to match
 * @return		0 on success
 */
int rpmmiAddPattern(/*@null@*/ rpmmi mi, rpmTag tag,
		rpmMireMode mode, /*@null@*/ const char * pattern)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies mi, mode, rpmGlobalMacroContext, internalState @*/;

/** \ingroup rpmdb
 * Prepare iterator for lazy writes.
 * @note Must be called before rpmmiNext() with CDB model database.
 * @param mi		rpm database iterator
 * @param rewrite	new value of rewrite
 * @return		previous value
 */
int rpmmiSetRewrite(/*@null@*/ rpmmi mi, int rewrite)
	/*@modifies mi @*/;

/** \ingroup rpmdb
 * Modify iterator to mark header for lazy write on release.
 * @param mi		rpm database iterator
 * @param modified	new value of modified
 * @return		previous value
 */
int rpmmiSetModified(/*@null@*/ rpmmi mi, int modified)
	/*@modifies mi @*/;

/** \ingroup rpmdb
 * Return database iterator.
 * @param db		rpm database
 * @param tag		rpm tag
 * @param keyp		key data (NULL for sequential access)
 * @param keylen	key data length (0 will use strlen(keyp))
 * @return		NULL on failure
 */
/*@only@*/ /*@null@*/
rpmmi rpmmiInit(/*@null@*/ rpmdb db, rpmTag tag,
			/*@null@*/ const void * keyp, size_t keylen)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies db, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Return next package header from iteration.
 * @param mi		rpm database iterator
 * @return		NULL on end of iteration.
 */
/*@null@*/
Header rpmmiNext(/*@null@*/ rpmmi mi)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mi, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Check rpmdb signal handler for trapped signal and/or requested exit.
 * Clean up any open iterators and databases on termination condition.
 * On non-zero exit any open references to rpmdb are invalid and cannot
 * be accessed anymore, calling process should terminate immediately.
 *
 * @param terminate	0 to only check for signals, 1 to terminate anyway
 * @return		0 to continue, 1 if termination cleanup was done.
 */
/*@mayexit@*/
int rpmdbCheckTerminate(int terminate)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Check for and exit on termination signals.
 */
/*@mayexit@*/
int rpmdbCheckSignals(void)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Unreference a rpm database iterator.
 * @param mi		rpm database iterator
 * @return		NULL on last dereference
 */
/*@null@*/
rpmmi rpmmiUnlink(/*@only@*/ /*@null@*/rpmmi mi)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mi, rpmGlobalMacroContext, fileSystem, internalState @*/;
#define rpmmiUnlink(_mi)  \
    ((rpmmi)rpmioUnlinkPoolItem((rpmioItem)(_mi), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmdb
 * Reference a rpm database iterator.
 * @param mi		rpm database iterator
 * @return		NULL on last dereference
 */
/*@null@*/
rpmmi rpmmiLink(/*@only@*/ /*@null@*/rpmmi mi)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mi, rpmGlobalMacroContext, fileSystem, internalState @*/;
#define rpmmiLink(_mi)  \
    ((rpmmi)rpmioLinkPoolItem((rpmioItem)(_mi), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmdb
 * Destroy rpm database iterator.
 * @param mi		rpm database iterator
 * @return		NULL always
 */
/*@null@*/
rpmmi rpmmiFree(/*@only@*/ /*@null@*/rpmmi mi)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mi, rpmGlobalMacroContext, fileSystem, internalState @*/;
#define rpmmiFree(_mi)  \
    ((rpmmi)rpmioFreePoolItem((rpmioItem)(_mi), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmdb
 * Return array of keys matching a pattern.
 * @param db		rpm database
 * @param tag		rpm tag
 * @param mode		type of pattern match
 * @param pat		pattern to match
 * @retval *argvp	array of keys that match
 * @return		0 on success
 */
int rpmdbMireApply(rpmdb db, rpmTag tag, rpmMireMode mode, const char * pat,
		const char *** argvp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies db, *argvp,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Add package header to rpm database and indices.
 * @param db		rpm database
 * @param iid		install transaction id (iid = 0 or -1 to skip)
 * @param h		header
 * @param ts		(unused) transaction set (or NULL)
 * @return		0 on success
 */
int rpmdbAdd(/*@null@*/ rpmdb db, int iid, Header h, /*@null@*/ rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies db, h, ts,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmdb
 * Remove package header from rpm database and indices.
 * @param db		rpm database
 * @param rid		(unused) remove transaction id (rid = 0 or -1 to skip)
 * @param hdrNum	package instance number in database
 * @param ts		(unused) transaction set (or NULL)
 * @return		0 on success
 */
int rpmdbRemove(/*@null@*/ rpmdb db, /*@unused@*/ int rid, uint32_t hdrNum,
		/*@null@*/ rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies db, ts,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Mergesort, same arguments as qsort(2).
 */
/*@unused@*/
int rpm_mergesort(void *base, size_t nmemb, size_t size,
                int (*cmp) (const void *, const void *))
	/*@globals errno @*/
	/*@modifies base, errno @*/;

#ifdef __cplusplus
}
#endif

/*@=bounds@*/
#endif	/* H_RPMDB */
