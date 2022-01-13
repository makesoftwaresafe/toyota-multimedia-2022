#ifndef rpmsql_h
#define RPMSQL_H

/** \ingroup rpmio
 * \file rpmio/rpmsql.h
 */

#include <stdio.h>

#include <rpmiotypes.h>
#include <rpmio.h>
#include <argv.h>

typedef void * rpmvArg;

typedef /*@abstract@*/ struct rpmvd_s * rpmvd;

typedef /*@abstract@*/ struct rpmvc_s * rpmvc;
typedef /*@abstract@*/ struct rpmvt_s * rpmvt;
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmsql_s * rpmsql;

/*@unchecked@*/
extern int _rpmsql_debug;

/*@unchecked@*/
extern int _rpmvt_debug;

/*@unchecked@*/
extern int _rpmvc_debug;

/*@unchecked@*/
extern rpmsql _rpmsqlI;

/*@unchecked@*/
extern volatile int _rpmsqlSeenInterrupt;

#if defined(_RPMSQL_INTERNAL)

#define F_ISSET(_sql, _FLAG) ((_sql)->flags & (RPMSQL_FLAGS_##_FLAG))
#define SQLDBG(_l) if (_rpmsql_debug) fprintf _l

/**
 * Interpreter flags.
 */
enum rpmsqlFlags_e {
    RPMSQL_FLAGS_NONE		= 0,
    RPMSQL_FLAGS_INTERACTIVE	= (1 <<  0),	/*    -interactive */
    RPMSQL_FLAGS_BAIL		= (1 <<  1),	/*    -bail */
    RPMSQL_FLAGS_NOLOAD		= (1 <<  2),	/*    -[no]load */
    RPMSQL_FLAGS_CREATE		= (1 <<  3),	/*    -[no]create */

    RPMSQL_FLAGS_ECHO		= (1 << 16),	/*    -echo */
    RPMSQL_FLAGS_SHOWHDR	= (1 << 17),	/*    -[no]header */
    RPMSQL_FLAGS_WRITABLE	= (1 << 18),	/* PRAGMA writable_schema */

    RPMSQL_FLAGS_PROMPT		= (1 << 24),	/* STDIN from tty */
};

/**
 * Operational modes.
 */
enum rpmsqlModes_e {
    RPMSQL_MODE_LINE	= 0,	/* One column per line.  Blank line between records */
    RPMSQL_MODE_COLUMN	= 1,	/* One record per line in neat columns */
    RPMSQL_MODE_LIST	= 2,	/* One record per line with a separator */
    RPMSQL_MODE_SEMI	= 3,	/* Same as MODE_LIST but append ";" to each line */
    RPMSQL_MODE_HTML	= 4,	/* Generate an XHTML table */
    RPMSQL_MODE_INSERT	= 5,	/* Generate SQL "insert" statements */
    RPMSQL_MODE_TCL	= 6,	/* Generate ANSI-C or TCL quoted elements */
    RPMSQL_MODE_CSV	= 7,	/* Quote strings, numbers are plain */
    RPMSQL_MODE_EXPLAIN	= 8,	/* Like MODE_COLUMN, but do not truncate data */
};

struct previous_mode {
    int valid;			/* Is there legit data in here? */
    uint32_t mode;
    uint32_t flags;
    int colWidth[100];
};

struct rpmsql_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    uint32_t flags;		/*!< control bits */
    const char ** av;		/*!< arguments */

    void * I;			/* The database (sqlite *) */
/*@null@*/
    void * S;			/* Current statement if any (sqlite3_stmt *) */

    const char * zInitFile;	/*    -init FILE */

    const char * zDbFilename;	/* Name of the database file */
    const char * zDestTable;	/* Name of destination table iff MODE_INSERT */

    uint32_t mode;		/* Operational mode. */

    int cnt;			/* Number of records displayed so far */

    FD_t ifd;			/* Read input here. */
    FD_t ofd;			/* Write output here */
    FD_t lfd;			/* Write log output here */
    FD_t tfd;			/* Write I/O traces here */
    rpmiob iob;			/* Output I/O buffer collector */

    int enableTimer;		/* Timer enabled? */
    struct rusage sBegin;	/* Saved resource info for start. */

	/* Holds the mode information just before .explain ON */
    struct previous_mode explainPrev;
    char separator[20];		/* Separator character for MODE_LIST */
    int colWidth[100];		/* Requested width of each column when in column mode */
    int actualWidth[100];	/* Actual width of each column */
    char nullvalue[20];		/* Text to print for NULL from the database */
    const char * outfile;	/* Filename for *out (NULL is stdout) */

    /* XXX INTERACTIVE cruft. */
    const char * zHome;		/* HOME */
    const char * zInitrc;	/* ~/.sqliterc */
    const char * zHistory;	/* ~/.sqlite_history */
    const char * zPrompt;	/* "sql> " */
    const char * zContinue;	/* "...> " */

    /* Sliding window input line buffer. */
    char * buf;
    size_t nbuf;
/*@null@*/
    char * b;
    size_t nb;

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMSQL_INTERNAL */

#ifdef	_RPMVT_INTERNAL
struct rpmvt_vtab_s {
    const void * pModule;
    int nRef;
    char * zErrMsg;
};
struct rpmvt_s {
    struct rpmvt_vtab_s _base;	/* for sqlite */
    void * db;			/* SQL database handle. */

    int argc;
    const char ** argv;

    int nfields;
    const char ** fields;

    int ncols;			/* No. of column items. */
    const char ** cols;		/* Column headers/types. */

    int ac;
    const char ** av;

    int debug;

    void * _ts;
    void * _gi;
    void * _h;

    rpmvd vd;		/* Data object. */
};
struct rpmVT_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    struct rpmvt_s vt;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
struct rpmvd_s {
    const char * dbpath;
    const char * prefix;
    const char * split;
    const char * parse;
    const char * regex;
    int fx;			/*!< argv index for column overrides */
    int idx;
};
#endif	/* _RPMVT_INTERNAL */

#ifdef	_RPMVC_INTERNAL
struct rpmvc_cursor_s {
    const void * pVTab;
};
struct rpmvc_s {
    struct rpmvc_cursor_s _base;/* for sqlite */
    rpmvt vt;			/*!< Linkage to virtual table. */
    int ix;			/*!< Current row index. */
    int nrows;			/*!< No. of row items. */
    int debug;
    rpmvd vd;			/*!< Data object. */
};
struct rpmVC_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    struct rpmvc_s vc;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMVC_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check sqlite3 return code, displaying error messages.
 * @param sql		sql interpreter
 * @param msg		sql method name
 * @param _db		sql database handle (i.e. "sqlite3 *")
 * @param rc		sql method return code
 * @return		rc is returned
 */
int rpmsqlCmd(rpmsql sql, const char * msg, /*@null@*/ void * _db,
		/*@returned@*/ int rc)
        /*@globals fileSystem @*/
        /*@modifies fileSystem @*/;

/**
 * Unreference a sql interpreter instance.
 * @param sql		sql interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmsql rpmsqlUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmsql sql)
	/*@modifies sql @*/;
#define	rpmsqlUnlink(_sql)	\
    ((rpmsql)rpmioUnlinkPoolItem((rpmioItem)(_sql), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a sql interpreter instance.
 * @param sql		sql interpreter
 * @return		new sql interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmsql rpmsqlLink (/*@null@*/ rpmsql sql)
	/*@modifies sql @*/;
#define	rpmsqlLink(_sql)	\
    ((rpmsql)rpmioLinkPoolItem((rpmioItem)(_sql), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a sql interpreter.
 * @param sql		sql interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmsql rpmsqlFree(/*@killref@*/ /*@null@*/rpmsql sql)
	/*@globals fileSystem @*/
	/*@modifies sql, fileSystem @*/;
#define	rpmsqlFree(_sql)	\
    ((rpmsql)rpmioFreePoolItem((rpmioItem)(_sql), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a sql interpreter.
 * @param av		sql interpreter args (or NULL)
 * @param flags		sql interpreter flags
 * @return		new sql interpreter
 */
/*@newref@*/ /*@null@*/
rpmsql rpmsqlNew(/*@null@*/ char ** av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Return arguments from a sql interpreter.
 * @param sql		sql interpreter
 * @retval *argcp	no. of arguments
 * @return		sql interpreter args
 */
/*@null@*/
const char ** rpmsqlArgv(/*@null@*/ rpmsql sql, /*@null@*/ int * argcp)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Execute sql from STRING | FILE | STDIN | INTERACTIVE.
 *
 * The str argument is used to determine how it should be run:
 * A leading '/' indicates a FILE, containing SQL commands.
 * A "-" or "stdin" argument used STD for SQL commands.
 * An empty "" string assumes INTERACTIVE, like STDIN but with prompts.
 * Otherwise, the STRING argument is treated as a sql command.
 *
 * @param sql		sql interpreter (NULL uses global interpreter)
 * @param str		sql string to execute (NULL returns RPMRC_FAIL)
 * @param *resultp	sql interpreter result
 * @return		RPMRC_OK on success
 */
rpmRC rpmsqlRun(rpmsql sql, /*@null@*/ const char * str,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies sql, *resultp, fileSystem, internalState @*/;

#ifdef	_RPMSQL_INTERNAL
typedef struct rpmsqlCF_s * rpmsqlCF;
struct rpmsqlCF_s {
    const char * zName;
    int8_t nArg;
    uint8_t argType;		/* 0: none.  1: db  2: (-1) */
    uint8_t eTextRep;		/* SQLITE_UTF8 or SQLITE_UTF16 */
    uint8_t  needCollSeq;
    void (*xFunc)();	/* (sqlite3_context * context, int argc, sqlite3_value ** argv); */
    void (*xStep)();	/* (sqlite3_context * context, int argc, sqlite3_value ** argv); */
    void (*xFinal)();	/* (sqlite3_context * context); */
};

/**
 * Load sqlite3 function extensions.
 * @param sql		sql interpreter
 * @param _CF		create function table (NULL uses _rpmsqlCFT)
 */
int _rpmsqlLoadCFT(rpmsql sql, /*@null@*/ void * _CF)
	/*@*/;

typedef struct sqlite3_module * rpmsqlVM;
typedef struct rpmsqlVMT_s * rpmsqlVMT;

struct rpmsqlVMT_s {
    const char * zName;
    const rpmsqlVM module;
    void * data;
};

/**
 * Load sqlite3 virtual tables.
 * @param _db		sqlite3 handle
 * @param _VMT		virtual module/table array
 * @return		0 on success
 */
int _rpmsqlLoadVMT(void * _db, rpmsqlVMT _VMT)
	/*@*/;
#endif	/* _RPMSQL_INTERNAL */

#ifdef	_RPMVT_INTERNAL
/**
 * Unreference a virtual table instance.
 * @param vt		virtual table
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmvt rpmvtUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmvt vt)
	/*@modifies vt @*/;
#define	rpmvtUnlink(_vt)	\
    ((rpmvt)(rpmioUnlinkPoolItem(((rpmioItem)(_vt))-1, __FUNCTION__, __FILE__, __LINE__)+1))

/**
 * Reference a virtual table instance.
 * @param vt		virtual table
 * @return		new virtual table reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmvt rpmvtLink (/*@null@*/ rpmvt vt)
	/*@modifies vt @*/;
#define	rpmvtLink(_vt)	\
    ((rpmvt)(rpmioLinkPoolItem(((rpmioItem)(_vt))-1, __FUNCTION__, __FILE__, __LINE__)+1))

/**
 * Derference a virtual table instance.
 * @param vt		virtual table
 * @return		NULL on last dereference
 */
/*@null@*/
rpmvt rpmvtFree(/*@killref@*/ /*@null@*/rpmvt vt)
	/*@globals fileSystem @*/
	/*@modifies vt, fileSystem @*/;
#define	rpmvtFree(_vt)	\
    ((rpmvt)rpmioFreePoolItem(((rpmioItem)(_vt))-1, __FUNCTION__, __FILE__, __LINE__))

int rpmvtLoadArgv(rpmvt vt, rpmvt * vtp)
	/*@*/;
rpmvt rpmvtNew(void * db, void * pModule, const char *const *argv, rpmvd vd)
	/*@*/;

/**
 * Create a virtual table.
 * @param _db		sql database handle
 * @param pAux		rpmsql object instance
 * @param argc		no. of arguments
 * @param argv		argument array
 * @retval *vtp	virtual table
 * @retval *pzErr	error message
 * @return		0 on success
 */
int rpmvtCreate(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
	/*@*/;

/**
 * Connect to a virtual table.
 * @param _db		sql database handle
 * @param pAux		rpmsql object instance
 * @param argc		no. of arguments
 * @param argv		argument array
 * @retval *vtp		virtual table
 * @retval *pzErr	error message
 * @return		0 on success
 */
int rpmvtConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
	/*@*/;

/**
 * Optimize a virtual table query.
 * @param vt		virtual table
 * @retval _pInfo	query to optimize 
 * @return		0 on success
 */
int rpmvtBestIndex(rpmvt vt, void * _pInfo)
	/*@*/;

/**
 * Disconnect (and destroy) a virtual table.
 * @param vt		virtual table
 * @return		0 on success
 */
int rpmvtDisconnect(rpmvt vt)
	/*@*/;

/**
 * Destroy a virtual table.
 * @param vt		virtual table
 * @return		0 on success
 */
int rpmvtDestroy(rpmvt vt)
	/*@*/;

/**
 * Update a virtual table.
 * @param vt		virtual table
 * @param argc
 * @param _argv
 * @retval *pRowid	(insert) new rowid
 * @return		0 on success
 */
int rpmvtUpdate(rpmvt vt, int argc, rpmvArg * _argv, int64_t * pRowid)
	/*@*/;

/**
 * Begin a virtual table transaction.
 * @param vt		virtual table
 * @return		0 on success
 */
int rpmvtBegin(rpmvt vt)
	/*@*/;

/**
 * Sync a virtual table transaction.
 * @param vt		virtual table
 * @return		0 on success
 */
int rpmvtSync(rpmvt vt)
	/*@*/;

/**
 * Commit a virtual table transaction.
 * @param vt		virtual table
 * @return		0 on success
 */
int rpmvtCommit(rpmvt vt)
	/*@*/;

/**
 * Rollback a virtual table transaction.
 * @param vt		virtual table
 * @return		0 on success
 */
int rpmvtRollback(rpmvt vt)
	/*@*/;

/**
 * Permit per-table function overloading.
 * @param vt		virtual table
 * @param nArg		no. of function args
 * @param zName		function name
 * @retval *pxFunc	overloaded function
 * @retval *ppArg	user data
 * @return		function is overloaded?
 */
int rpmvtFindFunction(rpmvt vt, int nArg, const char * zName,
		void (**pxFunc)(void *, int, rpmvArg *),
		void ** ppArg)
	/*@*/;

/**
 * Rename a virtual table.
 * @param vt		virtual table
 * @param zNew		new name
 * @return		0 on success
 */
int rpmvtRename(rpmvt vt, const char * zNew)
	/*@*/;
#endif	/* _RPMVT_INTERNAL */

#ifdef	_RPMVC_INTERNAL
/**
 * Unreference a virtual cursor instance.
 * @param vc		virtual cursor
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmvc rpmvcUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmvc vc)
	/*@modifies vc @*/;
#define	rpmvcUnlink(_vc)	\
    ((rpmvc)(rpmioUnlinkPoolItem(((rpmioItem)(_vc))-1, __FUNCTION__, __FILE__, __LINE__)+1))

/**
 * Reference a virtual cursor instance.
 * @param vc		virtual cursor
 * @return		new virtual cursor reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmvc rpmvcLink (/*@null@*/ rpmvc vc)
	/*@modifies vc @*/;
#define	rpmvcLink(_vc)	\
    ((rpmvc)(rpmioLinkPoolItem(((rpmioItem)(_vc))-1, __FUNCTION__, __FILE__, __LINE__)+1))

/**
 * Derference a virtual cursor instance.
 * @param vc		virtual cursor
 * @return		NULL on last dereference
 */
/*@null@*/
rpmvc rpmvcFree(/*@killref@*/ /*@null@*/rpmvc vc)
	/*@globals fileSystem @*/
	/*@modifies vc, fileSystem @*/;
#define	rpmvcFree(_vc)	\
    ((rpmvc)rpmioFreePoolItem(((rpmioItem)(_vc))-1, __FUNCTION__, __FILE__, __LINE__))

rpmvc rpmvcNew(rpmvt vt, int nrows)
	/*@*/;

/**
 * Create a virtual cursor.
 * @param vt		virtual table
 * @retval *vcp		virtual cursor
 * @return		0 on success
 */
int rpmvcOpen(rpmvt vt, rpmvc * vcp)
	/*@*/;

/**
 * Destroy a virtual cursor.
 * @param vc		virtual cursor
 * @return		0 on success
 */
int rpmvcClose(rpmvc vc)
	/*@*/;

/**
 * Start a virtual table search.
 * @param vc		virtual cursor
 * @param idxNum
 * @param idxStr
 * @param argc
 * @param _argv
 * @return		0 on success
 */
int rpmvcFilter(rpmvc vc, int idxNum, const char * idxStr,
		int argc, rpmvArg * _argv)
	/*@*/;

/**
 * Advance cursor to next item.
 * @param vc		virtual cursor
 * @return		0 on success
 */
int rpmvcNext(rpmvc vc)
	/*@*/;

/**
 * Is current cursor row invalid?
 * @param vc		virtual cursor
 * @return		1 if invalid, otherwise 0
 */
int rpmvcEof(rpmvc vc)
	/*@*/;

/**
 * Return a cursor column value.
 * @param vc		virtual cursor
 * @param _pContext
 * @param colx		column number
 * @return		0 on success
 */
int rpmvcColumn(rpmvc vc, void * _pContext, int colx)
	/*@*/;

/**
 * Return a cursor row identifier.
 * @param vc		virtual cursor
 * @retval *pRowid	row identifier
 * @return		0 on success
 */
int rpmvcRowid(rpmvc vc, int64_t * pRowid)
	/*@*/;

#endif	/* _RPMVC_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* RPMSQL_H */
