#ifndef	H_RPMODBC
#define	H_RPMODBC

/** \ingroup rpmio
 * \file rpmio/rpmodbc.h
 */

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _odbc_debug;

/** \ingroup rpmio
 */
typedef /*@refcounted@*/ struct ODBC_s * ODBC_t;
typedef struct HNDL_s * HNDL_t;

typedef struct _STMT_s * _STMT_t;
typedef struct _PARAM_s * _PARAM_t;

#if defined(_RPMODBC_INTERNAL)
#if !defined(SQL_HANDLE_STMT)	/* XXX retrofit <sql.h> constants. */
#define	SQL_HANDLE_ENV	1
#define	SQL_HANDLE_DBC	2
#define	SQL_HANDLE_STMT	3
#define	SQL_HANDLE_DESC	4
#define	SQLHANDLE	void
#endif

struct HNDL_s {
    int ht;
    void * hp;
};

/** \ingroup rpmio
 */
struct ODBC_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;
    int flags;

    void * u;
    const char * db;

    HNDL_t env;
    HNDL_t dbc;
    HNDL_t stmt;
    HNDL_t desc;

    int ncols;
    int nrows;
    int cx;
    int rx;

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

struct _PARAM_s {	/* XXX remapped from <sqltypes.h> */
    unsigned short ParameterNumber;
    short InputOutputType;
    short ValueType;
    short ParameterType;
    unsigned long ColumnSize;
    short DecimalDigits;
    void * ParameterValuePtr;
    long BufferLength;
    long * StrLen_or_IndPtr;
};
 
struct _STMT_s {
    const char * sql;
    _PARAM_t * params;
};
#endif	/* _RPMODBC_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a odbc wrapper instance.
 * @param odbc		odbc wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
ODBC_t odbcUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ ODBC_t odbc)
	/*@modifies odbc @*/;
#define	odbcUnlink(_odbc)	\
    ((ODBC_t)rpmioUnlinkPoolItem((rpmioItem)(_odbc), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a odbc wrapper instance.
 * @param odbc		odbc wrapper
 * @return		new odbc wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
ODBC_t odbcLink (/*@null@*/ ODBC_t odbc)
	/*@modifies odbc @*/;
#define	odbcLink(_odbc)	\
    ((ODBC_t)rpmioLinkPoolItem((rpmioItem)(_odbc), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a odbc wrapper.
 * @param odbc		odbc wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
ODBC_t odbcFree(/*@killref@*/ /*@null@*/ODBC_t odbc)
	/*@globals fileSystem @*/
	/*@modifies odbc, fileSystem @*/;
#define	odbcFree(_odbc)	\
    ((ODBC_t)rpmioFreePoolItem((rpmioItem)(_odbc), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a odbc wrapper.
 * @param fn		odbc file
 * @param flags		odbc flags
 * @return		new odbc wrapper
 */
/*@newref@*/ /*@null@*/
ODBC_t odbcNew(const char * fn, int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

int odbcGetEnvAttr(ODBC_t odbc, int _type, void * _bp, int _nb, int * nsp)
	/*@*/;
int odbcSetEnvAttr(ODBC_t odbc, int _type, void * _bp, int ns)
	/*@*/;
int odbcGetInfo(ODBC_t odbc, int _type, void * _bp, int _nb, short * nsp)
	/*@*/;
int odbcGetStmtAttr(ODBC_t odbc, int _attr, void * _bp, int _nb, int * nsp)
	/*@*/;
int odbcSetStmtAttr(ODBC_t odbc, int _attr, void * _bp, int ns)
	/*@*/;

int odbcConnect(ODBC_t odbc, /*@null@*/ const char * uri)
	/*@*/;
int odbcDisconnect(ODBC_t odbc)
	/*@*/;

int odbcListDataSources(ODBC_t odbc, void *_fp)
	/*@*/;
int odbcListDrivers(ODBC_t odbc, void *_fp)
	/*@*/;

int odbcTables(ODBC_t odbc, const char * tblname)
	/*@*/;
int odbcColumns(ODBC_t odbc, const char * tblname, const char * colname)
	/*@*/;
int odbcStatistics(ODBC_t odbc, const char * tblname)
	/*@*/;

int odbcCloseCursor(ODBC_t odbc)
	/*@*/;
const char * odbcGetCursorName(ODBC_t odbc)
	/*@*/;
int odbcSetCursorName(ODBC_t odbc, const char * s, size_t ns)
	/*@*/;

int odbcEndTran(ODBC_t odbc, int _rollback)
	/*@*/;
int odbcCommit(ODBC_t odbc)
	/*@*/;
int odbcRollback(ODBC_t odbc)
	/*@*/;

int odbcNRows(ODBC_t odbc)
	/*@*/;
int odbcNCols(ODBC_t odbc)
	/*@*/;
int odbcCancel(ODBC_t odbc)
	/*@*/;
int odbcFetch(ODBC_t odbc)
	/*@*/;
int odbcFetchScroll(ODBC_t odbc, short FetchOrientation, long FetchOffset)
	/*@*/;
int odbcGetData(ODBC_t odbc,
		unsigned short Col_or_Param_Num,
		short TargetType,
		void * TargetValuePtr,
		long BufferLength,
		long * StrLen_or_IndPtr)
	/*@*/;
int odbcColAttribute(ODBC_t odbc,
		unsigned short ColumnNumber,
		unsigned short FieldIdentifier,
		void * CharacterAttributePtr,
		short BufferLength,
		short * StringLengthPtr,
		long * NumericAttributePtr)
	/*@*/;

int odbcPrint(ODBC_t odbc, void * _fp)
	/*@*/;

int odbcExecDirect(ODBC_t odbc, const char * s, size_t ns)
	/*@*/;

int odbcPrepare(ODBC_t odbc, const char * s, size_t ns)
	/*@*/;

int odbcBindCol(ODBC_t odbc, unsigned short ColumnNumber, short TargetType,
		void * TargetValuePtr, long BufferLength, long * StrLen_or_Ind)
	/*@*/;
int odbcBindParameter(ODBC_t odbc, _PARAM_t param)
	/*@*/;

int odbcExecute(ODBC_t odbc)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMODBC */
