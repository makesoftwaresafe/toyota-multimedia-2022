/** \ingroup rpmio
 * \file rpmio/rpmodbc.c
 */

#include "system.h"

#if defined(HAVE_SQLEXT_H)
#include <sqlext.h>
#endif

#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <rpmmacro.h>
#include <rpmurl.h>
#include <argv.h>

#define	_RPMODBC_INTERNAL
#include <rpmodbc.h>

#include "debug.h"

/*@unchecked@*/
int _odbc_debug = 0;

#define DBG(_t, _l) \
  if ((_t) || _odbc_debug) fprintf _l

#define	SPEW(_t, _rc, _odbc)	\
  { if ((_t) || _odbc_debug) \
	fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, (_odbc), \
		(_rc)); \
  }

#if !defined(WITH_UNIXODBC)	/* XXX retrofit */
#define SQL_SUCCEEDED(rc) (((rc)&(~1))==0)
#define	SQL_NO_DATA	100
#define SQL_FETCH_NEXT		1
#define SQL_FETCH_FIRST		2
#define SQL_FETCH_LAST		3
#define SQL_FETCH_PRIOR		4
#define SQL_FETCH_ABSOLUTE	5
#define SQL_FETCH_RELATIVE	6

#define	SQL_C_CHAR		1
#define	SQL_C_SHORT		5
#define	SQL_C_LONG		4
#define	SQL_C_FLOAT		6
#define	SQL_C_DOUBLE		8
#define	SQL_C_DATE		9
#define	SQL_C_TIME		10
#define	SQL_C_TIMESTAMP		11

#define SQL_COLUMN_NAME		1
#define SQL_COLUMN_TABLE_NAME	15
#define SQL_COLUMN_LABEL	18

#define	SQL_COMMIT		0
#define	SQL_ROLLBACK		1

#endif

/*==============================================================*/
#if defined(WITH_UNIXODBC)
static int Xchkodbc(/*@unused@*/ ODBC_t odbc, int ht, const char * msg,
                int error, int printit,
                const char * func, const char * fn, unsigned ln)
{
    int rc = error;

    /* XXX filter SQL_NO_DATA(100) return. */
    if (printit && !SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA) {
	HNDL_t H;
	short ret;
	int i;

        fprintf(stderr, "error: %s:%s:%u: %s(%d):\n",
                func, fn, ln, msg, rc);

	switch (ht) {
	case SQL_HANDLE_ENV:	H = odbc->env;	break;
	case SQL_HANDLE_DBC:	H = odbc->dbc;	break;
	case SQL_HANDLE_STMT:	H = odbc->stmt;	break;
	case SQL_HANDLE_DESC:	H = odbc->desc;	break;
	default:		H = NULL;		break;
	}

assert(H);
	i = 0;
	do {
	    int  native;
	    unsigned char state[7];
	    unsigned char text[256];
	    short len;
	    ret = SQLGetDiagRec(H->ht, H->hp, ++i, state, &native, text,
                            sizeof(text), &len);
	    if (SQL_SUCCEEDED(ret))
		fprintf(stderr, "\t%s:%ld:%ld:%s\n",
				state, (long)i, (long)native, text);
	} while (ret == SQL_SUCCESS);
    }

    return rc;
}

/* XXF FIXME: add logic to set printit */
#define CHECK(_o, _t, _m, _rc)  \
    Xchkodbc(_o, _t, _m, _rc, 1, __FUNCTION__, __FILE__, __LINE__)
#else
#define CHECK(_o, _t, _m, _rc)  (-1)
#endif	/* WITH_UNIXODBC */

static void * hFree(ODBC_t odbc, HNDL_t H)
{
    int xx;

    if (H) {
	if (H->hp)
	    xx = CHECK(odbc, H->ht, "SQLFreeHandle",
			SQLFreeHandle(H->ht, H->hp));
	H->ht = 0;
	H->hp = NULL;
	H = _free(H);
    }
    return NULL;
};

static void * hAlloc(ODBC_t odbc, int ht)
{
    HNDL_t PH;
    HNDL_t H = xmalloc(sizeof(*H));
    SQLHANDLE * parent;
    int xx;

    H->ht = ht;
    H->hp = NULL;

    switch (ht) {
    case SQL_HANDLE_ENV:	PH = NULL;		break;
    case SQL_HANDLE_DBC:	PH = odbc->env;		break;
    case SQL_HANDLE_STMT:	PH = odbc->dbc;		break;
    case SQL_HANDLE_DESC:	PH = odbc->dbc;		break;
    default:			PH = NULL;		break;
    }
    parent = (PH ? PH->hp : NULL);

    xx = CHECK(odbc, H->ht, "SQLAllocHandle",
		SQLAllocHandle(H->ht, parent, &H->hp));
assert(H->hp);

    return H;
};

/*==============================================================*/
typedef struct key_s {
    int32_t	t;
    uint32_t	v;
    const char *n;
} KEY;

#define _ENTRY(_t, _v)      { _t, SQL_ATTR_##_v, #_v, }
static KEY SQL_ATTRS[] = {
#if defined(WITH_UNIXODBC)
  /* sql.h */
    _ENTRY(8+1, APP_PARAM_DESC),	/* ptr */
    _ENTRY(8+1, APP_ROW_DESC),		/* ptr */
    _ENTRY(8+1, IMP_PARAM_DESC),	/* RO ptr */
    _ENTRY(8+1, IMP_ROW_DESC),		/* RO ptr */
  /* sqlext.h */
    _ENTRY(  8, ASYNC_ENABLE),		/* ul */
	/* SQL_ATTR_ASYNC_STMT_EVENT ptr */
	/* SQL_ATTR_ASYNC_STMT_PCALLBACK ptr */
	/* SQL_ATTR_ASYNC_STMT_PCONTEXT ptr */
    _ENTRY(  8, CONCURRENCY),		/* ul */
	/* SQL_ATTR_CURSOR_SCROLLABLE ul */
	/* SQL_ATTR_CURSOR_SENSITIVITY ul */
    _ENTRY(  8, CURSOR_TYPE),		/* ul */
    _ENTRY(  8, ENABLE_AUTO_IPD),	/* ul */
    _ENTRY(8+8, FETCH_BOOKMARK_PTR),	/* *l */
    _ENTRY(  8, KEYSET_SIZE),		/* ul */
    _ENTRY(  8, MAX_LENGTH),		/* ul */
    _ENTRY(  8, MAX_ROWS),		/* ul */
	/* SQL_ATTR_METADATA_ID ul */
    _ENTRY(  8, NOSCAN),		/* ul */
    _ENTRY(8+8, PARAM_BIND_OFFSET_PTR),	/* *ul */
    _ENTRY(  8, PARAM_BIND_TYPE),	/* ul */
    _ENTRY(8+2, PARAM_OPERATION_PTR),	/* *uh */
    _ENTRY(8+2, PARAM_STATUS_PTR),	/* *uh */
    _ENTRY(8+8, PARAMS_PROCESSED_PTR),	/* *ul */
    _ENTRY(  8, PARAMSET_SIZE),		/* ul */
    _ENTRY(  8, QUERY_TIMEOUT),		/* ul */
    _ENTRY(  8, RETRIEVE_DATA),		/* ul */
    _ENTRY(  8, ROW_ARRAY_SIZE),	/* ul */
    _ENTRY(8+8, ROW_BIND_OFFSET_PTR),	/* *ul */
    _ENTRY(  8, ROW_BIND_TYPE),		/* ul */
    _ENTRY(  8, ROW_NUMBER),		/* RO ul */
    _ENTRY(8+2, ROW_OPERATION_PTR),	/* *uh */
    _ENTRY(8+2, ROW_STATUS_PTR),	/* *uh */
    _ENTRY(8+8, ROWS_FETCHED_PTR),	/* *ul */
    _ENTRY(  8, SIMULATE_CURSOR),	/* ul */
    _ENTRY(  8, USE_BOOKMARKS),		/* ul */
#else
    { 0, 0, "UNKNOWN" },
#endif	/* WITH_UNIXODBC */
};
#undef	_ENTRY
static size_t nSQL_ATTRS = sizeof(SQL_ATTRS) / sizeof(SQL_ATTRS[0]);

static int odbcDumpStmt(ODBC_t odbc, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    int rc = 0;
    size_t i;

    for (i = 0; i < nSQL_ATTRS; i++) {
	int _attr = SQL_ATTRS[i].v;
	unsigned long _ul;
	void * _p;
	unsigned short * _uhp;
	unsigned long * _ulp;
	int ns;

	_ul = 0;
	_p = NULL;
	_uhp = NULL;
	_ulp = NULL;
	ns = 0;
	switch (SQL_ATTRS[i].t) {
	default:
	    continue;
	    break;
	case   8:
	    rc = odbcGetStmtAttr(odbc, _attr, &_ul, sizeof(_ul), &ns);
	    if (_ul != 0)
fprintf(fp, "\t%s:\t0x%lx\n", SQL_ATTRS[i].n, _ul);
	    break;
	case 8+1:
	    rc = odbcGetStmtAttr(odbc, _attr, &_p, sizeof(_p), &ns);
	    if (_p != NULL)
fprintf(fp, "\t%s:\t%p\n", SQL_ATTRS[i].n, _p);
	    break;
	case 8+2:
	    rc = odbcGetStmtAttr(odbc, _attr, &_uhp, sizeof(_uhp), &ns);
	    if (_uhp != NULL)
fprintf(fp, "\t%s:\t*(%p) = 0x%hx\n", SQL_ATTRS[i].n, _uhp, *_uhp);
	    break;
	case 8+8:
	    rc = odbcGetStmtAttr(odbc, _attr, &_ulp, sizeof(_ulp), &ns);
	    if (_uhp != NULL)
fprintf(fp, "\t%s:\t*(%p) = 0x%lx\n", SQL_ATTRS[i].n, _ulp, *_ulp);
	    break;
	}
    }

    return rc;
}

int odbcGetStmtAttr(ODBC_t odbc, int _attr, void * _bp, int _nb, int * nsp)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = -1;
    if (stmt)
	rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLGetStmtAttr",
		SQLGetStmtAttr(stmt, _attr, _bp, _nb, nsp));

    return rc;
}

int odbcSetStmtAttr(ODBC_t odbc, int _attr, void * _bp, int ns)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = -1;
    if (stmt)
	rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLSetStmtAttr",
		SQLSetStmtAttr(stmt, _attr, _bp, ns));
    return rc;
}

/*==============================================================*/
#define _ENTRY(_t, _v)      { _t, SQL_##_v, #_v, }
static KEY SQL_INFOS[] = {
#if defined(WITH_UNIXODBC)
  /* sql.h */
    _ENTRY(  0, FETCH_DIRECTION),		/* ul? (unixODBC) */

    _ENTRY(  1, ACCESSIBLE_PROCEDURES),		/* c: Y/N */
    _ENTRY(  1, ACCESSIBLE_TABLES),		/* c: Y/N */
	/* SQL_ACTIVE_ENVIRONMENTS uh */
	/* SQL_AGGREGATE_FUNCTIONS ui */
	/* SQL_ALTER_DOMAIN ui */
    _ENTRY(  4, ALTER_TABLE),			/* ui */
	/* SQL_ASYNC_DBC_FUNCTIONS ui */
	/* SQL_ASYNC_MODE ui */
	/* SQL_ASYNC_NOTIFICATION ui */
	/* SQL_BATCH_ROW_COUNT ui */
	/* SQL_BATCH_SUPPORT ui */
	/* SQL_BOOKMARK_PERSISTENCE ui */
	/* SQL_CATALOG_LOCATION uh */
    _ENTRY(  1, CATALOG_NAME),			/* c: Y/N */
	/* SQL_CATALOG_NAME_SEPARATOR s */
	/* SQL_CATALOG_TERM s */
	/* SQL_CATALOG_USAGE ui */
    _ENTRY(256, COLLATION_SEQ),			/* s */
	/* SQL_COLUMN_ALIAS c: Y/N */
	/* SQL_CONCAT_NULL_BEHAVIOR uh */
	/* SQL_CONVERT_type ui */
	/* SQL_CONVERT_FINCTIONS ui */
	/* SQL_CORRELATION_NAME uh */
	/* SQL_CREATE_ASSERTION ui */
	/* SQL_CREATE_CHARACTER_SET ui */
	/* SQL_CREATE_COLLATION ui */
	/* SQL_CREATE_DOMAIN ui */
	/* SQL_CREATE_SCHEMA ui */
	/* SQL_CREATE_TABLE ui */
	/* SQL_CREATE_TRANSLATION ui */
	/* SQL_CREATE_VIEW ui */
    _ENTRY(  2, CURSOR_COMMIT_BEHAVIOR),	/* uh */
    _ENTRY(  2, CURSOR_ROLLBACK_BEHAVIOR),	/* uh */
    _ENTRY(  4, CURSOR_SENSITIVITY),		/* ui */
    _ENTRY(256, DATA_SOURCE_NAME),		/* s */
    _ENTRY(  1, DATA_SOURCE_READ_ONLY),		/* c: Y/N */
	/* SQL_DATABASE_NAME s */
	/* SQL_DATETIME_LITERALS ui */
    _ENTRY(256, DBMS_NAME),			/* s */
    _ENTRY(256, DBMS_VER),			/* s */
	/* SQL_DDL_INDEX ui */
    _ENTRY(  4, DEFAULT_TXN_ISOLATION),		/* ui */
    _ENTRY(  1, DESCRIBE_PARAMETER),		/* c: Y/N */
	/* SQL_DM_VER s */
	/* SQL_DRIVER_AWARE_POOLING_SUPPORT ui */
	/* SQL_DRIVER_HDBCSQL_DRIVER_HENV ul */
	/* SQL_DRIVER_HDESC ul */
	/* SQL_DRIVER_HLIB ul */
	/* SQL_DRIVER_HSTMT ul */
	/* SQL_DRIVER_NAME s */
	/* SQL_DRIVER_ODBC_VER s */
	/* SQL_DRIVER_VER s */
	/* SQL_DROP_ASSERTION ui */
	/* SQL_DROP_CHARACTER_SET ui */
	/* SQL_DROP_COLLATION ui */
	/* SQL_DROP_DOMAIN ui */
	/* SQL_DROP_SCHEMA ui */
	/* SQL_DROP_TABLE ui */
	/* SQL_DROP_TRANSLATION ui */
	/* SQL_DROP_VIEW ui */
	/* SQL_DYNAMIC_CURSOR_ATTRIBUTES ui */
	/* SQL_DYNAMIC_CURSOR_ATTRIBUTES2 ui */
	/* SQL_EXPRESSIONS_IN_ORDERBY c: Y/N */
	/* SQL_FILE_USAGE uh */
	/* SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2 ui */
    _ENTRY(  4, GETDATA_EXTENSIONS),		/* ui */
	/* SQL_GROUP_BY uh */
    _ENTRY(  2, IDENTIFIER_CASE),		/* uh */
    _ENTRY(  2, IDENTIFIER_QUOTE_CHAR),		/* uh */
	/* SQL_IDENTIFIER_CASE uh */
	/* SQL_INDEX_KEYWORDS ui */
	/* SQL_INFO_SCHEMA_VIEWS ui */
	/* SQL_INSERT_STATEMENT ui */
    _ENTRY(  1, INTEGRITY),			/* c: Y/N */
	/* SQL_KEYSET_CURSOR_ATTRIBUTES ui */
	/* SQL_KEYSET_CURSOR_ATTRIBUTES2 ui */
	/* SQL_KEYWORDS s */
	/* SQL_LIKE_ESCAPE_CLAUSE c: Y/N */
	/* SQL_MAX_ASYNC_CONCURRENT_STATEMENTS ui */
	/* SQL_MAX_BINARY_LITERAL_LEN ui */
    _ENTRY(  2, MAX_CATALOG_NAME_LEN),		/* uh */
    _ENTRY(  2, MAXIMUM_CATALOG_NAME_LENGTH),	/* uh */
	/* SQL_MAX_CHAR_LITERAL_LEN ui */
    _ENTRY(  2, MAX_COLUMN_NAME_LEN),		/* uh */
    _ENTRY( -2, MAXIMUM_COLUMN_NAME_LENGTH),	/* uh */
    _ENTRY(  2, MAX_COLUMNS_IN_GROUP_BY),	/* uh */
    _ENTRY( -2, MAXIMUM_COLUMNS_IN_GROUP_BY),	/* uh */
    _ENTRY(  2, MAX_COLUMNS_IN_INDEX),		/* uh */
    _ENTRY( -2, MAXIMUM_COLUMNS_IN_INDEX),	/* uh */
    _ENTRY(  2, MAX_COLUMNS_IN_ORDER_BY),	/* uh */
    _ENTRY( -2, MAXIMUM_COLUMNS_IN_ORDER_BY),	/* uh */
    _ENTRY(  2, MAX_COLUMNS_IN_SELECT),		/* uh */
    _ENTRY( -2, MAXIMUM_COLUMNS_IN_SELECT),	/* uh */
    _ENTRY(  2, MAX_COLUMNS_IN_TABLE),		/* uh */
    _ENTRY(  2, MAX_CONCURRENT_ACTIVITIES),	/* uh */
    _ENTRY( -2, MAXIMUM_CONCURRENT_ACTIVITIES),	/* uh */
    _ENTRY(  2, MAX_CURSOR_NAME_LEN),		/* uh */
    _ENTRY( -2, MAXIMUM_CURSOR_NAME_LENGTH),	/* uh */
    _ENTRY(  2, MAX_DRIVER_CONNECTIONS),	/* uh */
    _ENTRY( -2, MAXIMUM_DRIVER_CONNECTIONS),	/* uh */
    _ENTRY(  2, MAX_IDENTIFIER_LEN),		/* uh */
    _ENTRY( -2, MAXIMUM_IDENTIFIER_LENGTH),	/* uh */
    _ENTRY(  2, MAX_INDEX_SIZE),		/* uh */
    _ENTRY( -2, MAXIMUM_INDEX_SIZE),		/* uh */
	/* SQL_MAX_PROCEDURE_NAME_LEN uh */
    _ENTRY(  4, MAX_ROW_SIZE),			/* ui */
    _ENTRY( -4, MAXIMUM_ROW_SIZE),		/* ui */
	/* SQL_MAX_ROW_SIZE_INCLUDES_LONG c: Y/N */
    _ENTRY(  2, MAX_SCHEMA_NAME_LEN),		/* uh */
    _ENTRY( -2, MAXIMUM_SCHEMA_NAME_LENGTH),	/* uh */
    _ENTRY(  4, MAX_STATEMENT_LEN),		/* ui */
    _ENTRY( -4, MAXIMUM_STATEMENT_LENGTH),	/* ui */
    _ENTRY(  2, MAX_TABLE_NAME_LEN),		/* uh */
    _ENTRY(  2, MAX_TABLES_IN_SELECT),		/* uh */
    _ENTRY( -2, MAXIMUM_TABLES_IN_SELECT),	/* uh */
    _ENTRY(  2, MAX_USER_NAME_LEN),		/* uh */
    _ENTRY( -2, MAXIMUM_USER_NAME_LENGTH),	/* uh */
	/* SQL_MULT_RESULT_SETS c: Y/N */
	/* SQL_MULT_ACTIVE_TXN c: Y/N */
	/* SQL_NEED_LONG_DATA_LEN c: Y/N */
	/* SQL_NON_NULLABLE_COLUMNS uh */
    _ENTRY(  2, NULL_COLLATION),		/* uh */
	/* SQL_NUMERIC_FUNCTIONS ui */
	/* SQL_ODBC_INTERFACE_CONFORMANCE ui */
	/* SQL_ODBC_VER s */
    _ENTRY(  4, OJ_CAPABILITIES),		/* ui */
    _ENTRY( -4, OUTER_JOIN_CAPABILITIES),	/* ui */
    _ENTRY(  1, ORDER_BY_COLUMNS_IN_SELECT),	/* c: Y/N */
	/* SQL_PARAM_ARRAY_ROW_COUNTS ui */
	/* SQL_PARAM_ARRAY_SELECTS ui */
	/* SQL_PROCEDURE_TERM s */
	/* SQL_PROCEDURES c: Y/N */
	/* SQL_POS_OPERATIONS ui */
	/* SQL_QUOTED_IDENTIFIER_CASE uh */
	/* SQL_ROW_UPDATES c: Y/N */
	/* SQL_SCHEMA_TERM s */
	/* SQL_SCHEMA_USAGE ui */
	/* SQL_SCROLL_OPTIONS ui */
    _ENTRY(  4, SCROLL_CONCURRENCY),	/* FIXME: ??? */
    _ENTRY(256, SEARCH_PATTERN_ESCAPE),		/* s */
    _ENTRY(256, SERVER_NAME),			/* s */
    _ENTRY(256, SPECIAL_CHARACTERS),		/* s */
	/* SQL_SQL_CONFORMANCE ui */
	/* SQL_SQL92_DATETIME_FUNCTIONS ui */
	/* SQL_SQL92_FOREIGN_KEY_DELETE_RULE ui */
	/* SQL_SQL92_FOREIGN_KEY_UPDATE_RULE ui */
	/* SQL_SQL92_GRANT ui */
	/* SQL_SQL92_NUMERIC_VALUE_FUNCTIONS ui */
	/* SQL_SQL92_PREDICATES ui */
	/* SQL_SQL92_RELATIONAL_JOIN_OPERATORS ui */
	/* SQL_SQL92_REVOKE ui */
	/* SQL_SQL92_ROW_VALUE_CONSTRUCTOR ui */
	/* SQL_SQL92_STRING_FUNCTIONS ui */
	/* SQL_SQL92_VALUE_EXPRESSIONS ui */
	/* SQL_STANDARD_CLI_CONFORMANCE ui */
	/* SQL_STATIC_CURSOR_ATTRIBUTES1 ui */
	/* SQL_STATIC_CURSOR_ATTRIBUTES2 ui */
	/* SQL_STRING_FUNCTIONS ui */
	/* SQL_SUBQUERIES ui */
	/* SQL_SYSTEM_FUNCTIONS ui */
	/* SQL_TABLE_TERM s */
	/* SQL_TIMEDATE_ADD_INTERVALS ui */
	/* SQL_TIMEDATE_DIFF_INTERVALS ui */
	/* SQL_TIMEDATE_FUNCTIONS ui */
    _ENTRY(  2, TXN_CAPABLE),			/* uh */
    _ENTRY( -2, TRANSACTION_CAPABLE),		/* uh */
    _ENTRY(  4, TXN_ISOLATION_OPTION),		/* ui */
    _ENTRY( -4, TRANSACTION_ISOLATION_OPTION),	/* ui */
	/* SQL_UNION ui */
    _ENTRY(256, USER_NAME),			/* s */
    _ENTRY(256, XOPEN_CLI_YEAR),		/* s */
#else
    { 0, 0, "UNKNOWN" },
#endif	/* WITH_UNIXODBC */
};
#undef	_ENTRY
static size_t nSQL_INFOS = sizeof(SQL_INFOS) / sizeof(SQL_INFOS[0]);

static int odbcDumpInfo(ODBC_t odbc, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    int rc = 0;
    size_t i;

    for (i = 0; i < nSQL_INFOS; i++) {
	int _type = SQL_INFOS[i].v;
	char _c;
	unsigned short _uh;
	unsigned int _ui;
	char _s[256];
	short ns;

	_c = '\0';
	_uh = 0;
	_ui = 0;
	_s[0] = '\0';
	ns = 0;
	switch (SQL_INFOS[i].t) {
	default:
	    continue;
	    break;
	case   1:
	    rc = odbcGetInfo(odbc, _type, &_c, sizeof(_c), &ns);
	    if (_c != '\0')
fprintf(fp, "\t%s:\t%c\n", SQL_INFOS[i].n, _c);
	    break;
	case   2:
	    rc = odbcGetInfo(odbc, _type, &_uh, sizeof(_uh), &ns);
	    if (_uh != 0)
fprintf(fp, "\t%s:\t0x%hx\n", SQL_INFOS[i].n, _uh);
	    break;
	case   4:
	    rc = odbcGetInfo(odbc, _type, &_ui, sizeof(_ui), &ns);
	    if (_ui != 0)
fprintf(fp, "\t%s:\t0x%x\n", SQL_INFOS[i].n, _ui);
	    break;
	case 256:
	    rc = odbcGetInfo(odbc, _type, &_s, sizeof(_s), &ns);
	    if (_s[0] != '\0')
fprintf(fp, "\t%s:\t%s\n", SQL_INFOS[i].n, _s);
	    break;
	}
    }

    return rc;
}

int odbcGetInfo(ODBC_t odbc, int _type, void * _bp, int _nb, short * nsp)
{
    SQLHANDLE * dbc = odbc->dbc->hp;
    int rc = -1;
    if (dbc)
	rc = CHECK(odbc, SQL_HANDLE_DBC, "SQLGetInfo",
		SQLGetInfo(dbc, _type, _bp, _nb, nsp));
    return rc;
}

/*==============================================================*/
#define _ENTRY(_t, _v)      { _t, SQL_ATTR_##_v, #_v, }
static KEY SQL_EATTRS[] = {
#if defined(WITH_UNIXODBC)
  /* sqlext.h */
    _ENTRY(  4, CONNECTION_POOLING),		/* ui */
    _ENTRY(  4, CP_MATCH),			/* ui */
    _ENTRY(  4, ODBC_VERSION),			/* ui */
	/* SQL_ATTR_OUTPUT_NTS ui */

    _ENTRY(  0, UNIXODBC_SYSPATH),		/* */
    _ENTRY(  0, UNIXODBC_VERSION),		/* */
    _ENTRY(  0, UNIXODBC_ENVATTR),		/* */
#endif	/* WITH_UNIXODBC */
};
#undef	_ENTRY
static size_t nSQL_EATTRS = sizeof(SQL_EATTRS) / sizeof(SQL_EATTRS[0]);

static int odbcDumpEnvAttr(ODBC_t odbc, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    int rc = 0;
    size_t i;

    for (i = 0; i < nSQL_EATTRS; i++) {
	int _type = SQL_EATTRS[i].v;
	unsigned int _ui;
	int ns;

	_ui = 0;
	ns = 0;
	switch (SQL_EATTRS[i].t) {
	default:
	    continue;
	    break;
	case   4:
	    rc = odbcGetEnvAttr(odbc, _type, &_ui, sizeof(_ui), &ns);
	    if (_ui != 0)
fprintf(fp, "\t%s:\t0x%x\n", SQL_EATTRS[i].n, _ui);
	    break;
	}
    }

    return rc;
}

int odbcGetEnvAttr(ODBC_t odbc, int _type, void * _bp, int _nb, int * nsp)
{
    SQLHANDLE * env = odbc->env->hp;
    int rc = -1;
    if (env)
	rc = CHECK(odbc, SQL_HANDLE_ENV, "SQLGetEnvAttr",
		SQLGetEnvAttr(env, _type, _bp, _nb, nsp));
    return rc;
}

int odbcSetEnvAttr(ODBC_t odbc, int _type, void * _bp, int ns)
{
    SQLHANDLE * env = odbc->env->hp;
    int rc = -1;
    if (env)
	rc = CHECK(odbc, SQL_HANDLE_ENV, "SQLSetEnvAttr",
		SQLSetEnvAttr(env, _type, _bp, ns));
    return rc;
}

/*==============================================================*/
#define _ENTRY(_t, _v)      { _t, SQL_COLUMN_##_v, #_v, }
static KEY SQL_CATTRS[] = {
#if defined(WITH_UNIXODBC)
  /* sqlext.h */
    _ENTRY(  0, COUNT),				/* */
    _ENTRY(  0, NAME),				/* */
    _ENTRY(  4, TYPE),				/* */
    _ENTRY(  4, LENGTH),			/* */
    _ENTRY(  4, PRECISION),			/* */
    _ENTRY(  4, SCALE),				/* */
    _ENTRY(  4, DISPLAY_SIZE),			/* */
    _ENTRY(  0, NULLABLE),			/* */
    _ENTRY(  4, UNSIGNED),			/* */
    _ENTRY(  4, MONEY),				/* */
    _ENTRY(  4, UPDATABLE),			/* */
    _ENTRY(  4, AUTO_INCREMENT),		/* */
    _ENTRY(  4, CASE_SENSITIVE),		/* */
    _ENTRY(  4, SEARCHABLE),			/* */
    _ENTRY(256, TYPE_NAME),			/* */
    _ENTRY(256, TABLE_NAME),			/* */
    _ENTRY(256, OWNER_NAME),			/* */
    _ENTRY(256, QUALIFIER_NAME),		/* */
    _ENTRY(256, LABEL),				/* */
#else
    { 0, 0, "UNKNOWN" },
#endif	/* WITH_UNIXODBC */
};
#undef	_ENTRY
static size_t nSQL_CATTRS = sizeof(SQL_CATTRS) / sizeof(SQL_CATTRS[0]);

static int odbcDumpColAttrs(ODBC_t odbc, int colx, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    int rc = 0;
    size_t i;

    for (i = 0; i < nSQL_CATTRS; i++) {
	int _type = SQL_CATTRS[i].v;
	char b[BUFSIZ];
	size_t nb = sizeof(b);;
	short ns;
	long got;

	b[0] = '\0';
	ns = 0xb00b;
	got = 0xdeadbeef;
	switch (SQL_CATTRS[i].t) {
	default:
	    continue;
	    break;
#ifdef	NOTYET	/* XXX unimplemented in unixODBC=>postgres */
	case   0:
	    rc = odbcColAttribute(odbc, colx, _type, b, nb, &ns, &got);
fprintf(fp, "\t%s:\tgot %lx %p[%hu] = \"%s\"\n", SQL_CATTRS[i].n, got, b, ns, b);
	    break;
#endif
	case   4:
	    rc = odbcColAttribute(odbc, colx, _type, b, nb, &ns, &got);
fprintf(fp, "\t%s:\t0x%lx\n", SQL_CATTRS[i].n, got);
	    break;
	case 256:
	    rc = odbcColAttribute(odbc, colx, _type, b, nb, &ns, &got);
fprintf(fp, "\t%s:\t%s\n", SQL_CATTRS[i].n, b);
	    break;
	}
    }

    return rc;
}

int odbcColAttribute(ODBC_t odbc,
		unsigned short ColumnNumber,
		unsigned short FieldIdentifier,
		void * CharacterAttributePtr,
		short BufferLength,
		short * StringLengthPtr,
		long * NumericAttributePtr)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = -1;
    (void)stmt;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLColAttribute",
	    SQLColAttribute(stmt,
		ColumnNumber,
		FieldIdentifier,
		CharacterAttributePtr,
		BufferLength,
		StringLengthPtr,
		NumericAttributePtr));

SPEW(0, rc, odbc);
    return rc;
}

/*==============================================================*/

int odbcCloseCursor(ODBC_t odbc)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = SQL_NO_DATA;
    (void)stmt;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLCloseCursor",
	    SQLCloseCursor(stmt));

SPEW(0, rc, odbc);
    return rc;
}

const char * odbcGetCursorName(ODBC_t odbc)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = SQL_NO_DATA;
    char b[BUFSIZ];
    short nb = sizeof(b);
    char * t = NULL;
    short ns;
    (void)stmt;
    (void)nb;
    (void)ns;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLGetCursorName",
	    SQLGetCursorName(stmt, (SQLCHAR *)b, nb, &ns));

SPEW(0, rc, odbc);
    if (rc == 0)
	t = xstrdup(b);
    return t;
}

int odbcSetCursorName(ODBC_t odbc, const char * s, size_t ns)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = SQL_NO_DATA;
    (void)stmt;

    if (ns == 0)
	ns = strlen(s);
    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLSetCursorName",
	    SQLSetCursorName(stmt, (SQLCHAR *)s, (SQLSMALLINT)ns));

SPEW(0, rc, odbc);
    return rc;
}

/*==============================================================*/

int odbcEndTran(ODBC_t odbc, int _rollback)
{
    SQLHANDLE * dbc = odbc->dbc->hp;	/* XXX env? */
    short _completion = (_rollback ? SQL_ROLLBACK : SQL_COMMIT);
    int rc = 0;
    (void)dbc;

    rc = CHECK(odbc, SQL_HANDLE_DBC, "SQLEndTran",
	    SQLEndTran(SQL_HANDLE_DBC, dbc, _completion));

SPEW(0, rc, odbc);
    return rc;
}

int odbcCommit(ODBC_t odbc)
{
    return odbcEndTran(odbc, SQL_COMMIT);
}

int odbcRollback(ODBC_t odbc)
{
    return odbcEndTran(odbc, SQL_ROLLBACK);
}

/*==============================================================*/

int odbcConnect(ODBC_t odbc, const char * uri)
{
    const char * db = NULL;
    SQLHANDLE * dbc;
    urlinfo u = NULL;
    int rc = -1;
int xx;

DBG(0, (stderr, "--> %s(%p,%s)\n", __FUNCTION__, odbc, uri));

    /*XXX FIXME: SQLConnect by DSN should be done here. */

    if (uri) {
	const char * dbpath = NULL;
	int ut = urlPath(uri, &dbpath);
assert(ut == URL_IS_MYSQL || ut == URL_IS_POSTGRES || ut == URL_IS_SQLSERVER);
	rc = urlSplit(uri, &u);
	db = rpmExpand(u->scheme, "_", basename((char *)dbpath), NULL);
    } else {
	u = odbc->u;
	db = xstrdup(odbc->db);
    }
assert(u);
assert(db);

DBG(0, (stderr, "\tdb: %s\n", db));
DBG(0, (stderr, "\t u: %s\n", u->user));
DBG(0, (stderr, "\tpw: %s\n", u->password));

    if (odbc->dbc == NULL)	/* XXX lazy? */
	odbc->dbc = hAlloc(odbc, SQL_HANDLE_DBC);
    dbc = odbc->dbc->hp;

    rc = CHECK(odbc, SQL_HANDLE_DBC, "SQLConnect",
		SQLConnect(dbc,
			(SQLCHAR *) db, SQL_NTS,
			(SQLCHAR *) u->user, SQL_NTS,
			(SQLCHAR *) u->password, SQL_NTS));

	/* XXX FIXME: SQLDriverConnect should print once?. */

    if (rc == 0) {
	xx = odbcDumpInfo(odbc, NULL);
    }

SPEW(0, rc, odbc);
    db = _free(db);
    return rc;
}

int odbcDisconnect(ODBC_t odbc)
{
    SQLHANDLE * dbc = odbc->dbc->hp;
    int rc = 0;
    (void)dbc;

    rc = CHECK(odbc, SQL_HANDLE_DBC, "SQLDisconnect",
		SQLDisconnect(dbc));

    odbc->desc = hFree(odbc, odbc->desc);	/* XXX needed? */
    odbc->stmt = hFree(odbc, odbc->stmt);	/* XXX needed? */
    odbc->dbc = hFree(odbc, odbc->dbc);		/* XXX lazy? */

SPEW(0, rc, odbc);
    return rc;
}

int odbcListDataSources(ODBC_t odbc, void *_fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    SQLHANDLE * env = odbc->env->hp;
    int rc = 0;
    (void)fp;

    unsigned char dsn[256];
    unsigned char desc[256];
    short dsn_ret;
    short desc_ret;
    unsigned short direction = SQL_FETCH_FIRST;
int xx;
    (void)env;
    (void)dsn_ret;
    (void)desc_ret;

    while (SQL_SUCCEEDED((xx = CHECK(odbc, SQL_HANDLE_ENV, "SQLDataSources",
		SQLDataSources(env, direction,
			dsn, sizeof(dsn), &dsn_ret,
			desc, sizeof(desc), &desc_ret)))))
    {
	direction = SQL_FETCH_NEXT;
	fprintf(fp, "\t%s - %s\n", dsn, desc);
#ifdef	NOTYET
	if (ret == SQL_SUCCESS_WITH_INFO) fprintf(fp, "\tdata truncation\n");
#endif
    }

SPEW(0, rc, odbc);
    return rc;
}

int odbcListDrivers(ODBC_t odbc, void *_fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    int rc = 0;
    (void)fp;

    SQLHANDLE * env = odbc->env->hp;
    unsigned char driver[256];
    unsigned char attr[256];
    short driver_ret;
    short attr_ret;
    unsigned short direction = SQL_FETCH_FIRST;
int xx;
    (void)env;
    (void)driver_ret;
    (void)attr_ret;

    while (SQL_SUCCEEDED((xx = CHECK(odbc, SQL_HANDLE_ENV, "SQLDrivers",
		SQLDrivers(env, direction,
			driver, sizeof(driver), &driver_ret,
			attr, sizeof(attr), &attr_ret)))))
    {
	direction = SQL_FETCH_NEXT;
	fprintf(fp, "\t%s - %s\n", driver, attr);
#ifdef	NOTYET
	if (xx == SQL_SUCCESS_WITH_INFO) fprintf(fp, "\tdata truncation\n");
#endif
    }

SPEW(0, rc, odbc);
    return rc;
}

int odbcNRows(ODBC_t odbc)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    long rows = 0;
    int rc = 0;
    (void)stmt;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLRowCount",
	    SQLRowCount(stmt, &rows));
    rc = rows;

SPEW(0, rc, odbc);
    return rc;
}

int odbcNCols(ODBC_t odbc)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    short columns = 0;
    int rc = 0;
    (void)stmt;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLNumResultCols",
	    SQLNumResultCols(stmt, &columns));
    rc = columns;

SPEW(0, rc, odbc);
    return rc;
}

int odbcCancel(ODBC_t odbc)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = SQL_NO_DATA;
    (void)stmt;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLCancel",
	    SQLCancel(stmt));
    odbc->stmt = hFree(odbc, odbc->stmt);	/* XXX lazy? */

SPEW(0, rc, odbc);
    return rc;
}

int odbcFetch(ODBC_t odbc)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = SQL_NO_DATA;
    (void)stmt;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLFetch",
	    SQLFetch(stmt));

SPEW(0, rc, odbc);
    return rc;
}

int odbcFetchScroll(ODBC_t odbc, short FetchOrientation, long FetchOffset)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = SQL_NO_DATA;
    (void)stmt;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLFetch",
	    SQLFetchScroll(stmt, FetchOrientation, FetchOffset));

SPEW(0, rc, odbc);
    return rc;
}

int odbcGetData(ODBC_t odbc,
		unsigned short Col_or_Param_Num,
		short TargetType,
		void * TargetValuePtr,
		long BufferLength,
		long * StrLen_or_IndPtr)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = -1;
    (void)stmt;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLGetData",
	    SQLGetData(stmt,
		Col_or_Param_Num,
		TargetType,
		TargetValuePtr,
		BufferLength,
		StrLen_or_IndPtr));

SPEW(0, rc, odbc);
    return rc;
}

int odbcPrint(ODBC_t odbc, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = 0;
    char b[BUFSIZ];
    size_t nb = sizeof(b);
    long got;
    ARGV_t av = NULL;
    int i;
int xx;
    (void)stmt;

DBG(0, (stderr, "--> %s(%p,%p)\n", __FUNCTION__, odbc, fp));

if (_odbc_debug < 0)
xx = odbcDumpStmt(odbc, fp);

    odbc->ncols = odbcNCols(odbc);
    odbc->nrows = 0;

    if (odbc->ncols)
    for (i = 0; i < odbc->ncols; i++) {
	size_t nw;
	short ns;
if (_odbc_debug < 0)
xx = odbcDumpColAttrs(odbc, i+1, NULL);
	ns = 0;
	xx = odbcColAttribute(odbc, i+1, SQL_COLUMN_LABEL,
			b, nb,  &ns, &got);
	if (xx)
	    nw = snprintf(b, nb, "  Column %d", i+1);
	xx = argvAdd(&av, b);
    }

    /* XXX filter SQL_NO_DATA(100) return. */
    if (odbc->ncols)
    while (SQL_SUCCEEDED((xx = odbcFetch(odbc))))
    {

	fprintf(fp, "Row %d\n", ++odbc->nrows);
	for (i = 0; i < odbc->ncols; i++) {

	    /* XXX filter -1 return (columns start with 1 not 0). */
	    xx = odbcGetData(odbc, i+1, SQL_C_CHAR, b, nb, &got);
	    if (SQL_SUCCEEDED(xx)) {
		if (got == 0) strcpy(b, "NULL");
		fprintf(fp, "  %20s : %s\n", av[i], b);
	    }
	}
    }

    odbc->nrows = 0;
    odbc->ncols = 0;

    odbc->stmt = hFree(odbc, odbc->stmt);	/* XXX lazy? */

    av = argvFree(av);

SPEW(0, rc, odbc);

    return rc;
}

int odbcTables(ODBC_t odbc, const char * tblname)
{
    SQLHANDLE * stmt;
    int rc = 0;

    if (odbc->stmt == NULL)
	odbc->stmt = hAlloc(odbc, SQL_HANDLE_STMT);
    stmt = odbc->stmt->hp;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLTables",
		SQLTables(stmt,
			NULL, 0,
			NULL, 0,
			(SQLCHAR *) tblname, SQL_NTS,
			NULL, 0));

SPEW(0, rc, odbc);
    return rc;
}

int odbcColumns(ODBC_t odbc, const char * tblname, const char * colname)
{
    SQLHANDLE * stmt;
    int rc = 0;

    if (odbc->stmt == NULL)
	odbc->stmt = hAlloc(odbc, SQL_HANDLE_STMT);
    stmt = odbc->stmt->hp;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLColumns",
    		SQLColumns(stmt,
			NULL, 0,
			NULL, 0,
			(SQLCHAR *) tblname, SQL_NTS,
			(SQLCHAR *) colname, SQL_NTS));

SPEW(0, rc, odbc);
    return rc;
}

int odbcStatistics(ODBC_t odbc, const char * tblname)
{
    SQLHANDLE * stmt;
    int rc = 0;

    if (odbc->stmt == NULL)
	odbc->stmt = hAlloc(odbc, SQL_HANDLE_STMT);
    stmt = odbc->stmt->hp;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLStatistics",
    		SQLStatistics(stmt,
			NULL, 0,
			NULL, 0,
			(SQLCHAR *) tblname, SQL_NTS,
			0, 0));

SPEW(0, rc, odbc);
    return rc;
}

int odbcExecDirect(ODBC_t odbc, const char * s, size_t ns)
{
    SQLHANDLE * stmt;
    int rc = 0;

DBG(0, (stderr, "--> %s(%p,%s,%u)\n", __FUNCTION__, odbc, s, (unsigned)ns));

    if (ns == 0)
	ns = strlen(s);

    if (odbc->stmt == NULL)
	odbc->stmt = hAlloc(odbc, SQL_HANDLE_STMT);
    stmt = odbc->stmt->hp;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLExecDirect",
		SQLExecDirect(stmt, (SQLCHAR *) s, (SQLINTEGER) ns));

SPEW(0, rc, odbc);
    return rc;
}

int odbcPrepare(ODBC_t odbc, const char * s, size_t ns)
{
    SQLHANDLE * stmt;
    int rc = 0;

DBG(0, (stderr, "--> %s(%p,%s,%u)\n", __FUNCTION__, odbc, s, (unsigned)ns));

    if (ns == 0)
	ns = strlen(s);

    /* XXX FIXME: programmer error */
    odbc->stmt = hFree(odbc, odbc->stmt);

    if (odbc->stmt == NULL)
	odbc->stmt = hAlloc(odbc, SQL_HANDLE_STMT);
    stmt = odbc->stmt->hp;

    rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLPrepare",
		SQLPrepare(stmt, (SQLCHAR *) s, (SQLINTEGER) ns));

SPEW(0, rc, odbc);
    return rc;
}

int odbcBindCol(ODBC_t odbc, unsigned short ColumnNumber, short TargetType,
		void * TargetValuePtr, long BufferLength, long * StrLen_or_Ind)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLBindCol",
		SQLBindCol(stmt,
			ColumnNumber,
			TargetType,
			TargetValuePtr,
			BufferLength,
			StrLen_or_Ind));
    (void)stmt;

SPEW(0, rc, odbc);
    return rc;
}

int odbcBindParameter(ODBC_t odbc, _PARAM_t param)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLBindParameter",
		SQLBindParameter(stmt,
			param->ParameterNumber,
			param->InputOutputType,
			param->ValueType,
			param->ParameterType,
			param->ColumnSize,
			param->DecimalDigits,
			param->ParameterValuePtr,
			param->BufferLength,
			param->StrLen_or_IndPtr));
    (void)stmt;

SPEW(0, rc, odbc);
    return rc;
}

int odbcExecute(ODBC_t odbc)
{
    SQLHANDLE * stmt = odbc->stmt->hp;
    int rc = CHECK(odbc, SQL_HANDLE_STMT, "SQLExecute",
		SQLExecute(stmt));
    (void)stmt;

SPEW(0, rc, odbc);
    return rc;
}

/*==============================================================*/

static void odbcFini(void * _odbc)
	/*@globals fileSystem @*/
	/*@modifies *_odbc, fileSystem @*/
{
    ODBC_t odbc = _odbc;

    odbc->desc = hFree(odbc, odbc->desc);
    odbc->stmt = hFree(odbc, odbc->stmt);
    odbc->dbc = hFree(odbc, odbc->dbc);
    odbc->env = hFree(odbc, odbc->env);

    odbc->db = _free(odbc->db);
    odbc->u = urlFree(odbc->u, __FUNCTION__);
    odbc->fn = _free(odbc->fn);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _odbcPool = NULL;

static ODBC_t odbcGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _odbcPool, fileSystem @*/
	/*@modifies pool, _odbcPool, fileSystem @*/
{
    ODBC_t odbc;

    if (_odbcPool == NULL) {
	_odbcPool = rpmioNewPool("odbc", sizeof(*odbc), -1, _odbc_debug,
			NULL, NULL, odbcFini);
	pool = _odbcPool;
    }
    odbc = (ODBC_t) rpmioGetPool(pool, sizeof(*odbc));
    memset(((char *)odbc)+sizeof(odbc->_item), 0, sizeof(*odbc)-sizeof(odbc->_item));
    return odbc;
}

static char * _odbc_uri = "mysql://luser:jasnl@localhost/test";

ODBC_t odbcNew(const char * fn, int flags)
{
    ODBC_t odbc = odbcGetPool(_odbcPool);
int xx;

    if (fn == NULL)
	fn = _odbc_uri;
    odbc->fn = xstrdup(fn);
    odbc->flags = flags;

    {	const char * dbpath = NULL;
	int ut = urlPath(fn, &dbpath);
	urlinfo u = NULL;

	xx = urlSplit(fn, &u);
assert(ut == URL_IS_MYSQL || ut == URL_IS_POSTGRES || URL_IS_SQLSERVER);
	odbc->db = rpmExpand(u->scheme, "_", basename((char *)dbpath), NULL);
	odbc->u = urlLink(u, __FUNCTION__);
    }

    odbc->env = hAlloc(odbc, SQL_HANDLE_ENV);
#if defined(WITH_UNIXODBC)
    xx = odbcSetEnvAttr(odbc, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
#endif

if (_odbc_debug < 0)
xx = odbcDumpEnvAttr(odbc, NULL);

	/* XXX FIXME: SQLDriverConnect should be done here. */

    return odbcLink(odbc);
}
