#include "system.h"

#include <sql.h>
#include <sqlext.h>

#include <poptIO.h>

#define	_RPMODBC_INTERNAL
#include <rpmodbc.h>

#include "debug.h"

#if 0
static char * _odbc_uri = "mysql://luser:jasnl@harwich.jbj.org/test";
static char * _odbc_uri = "sqlserver://Export:export99@127.0.0.1:1433/test";
#else
static char * _odbc_uri = "postgres://luser:jasnl@harwich.jbj.org/test";
#endif
static int _odbc_flags = 0;

#define	SPEW(_t, _rc, _odbc)	\
  { if ((_t) || _odbc_debug ) \
	fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, (_odbc), \
		(_rc)); \
  }

/*==============================================================*/

static int odbcOpen(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

static int odbcClose(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

static int odbcCOpen(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

static int odbcCClose(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

static int odbcCDup(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

static int odbcCDel(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

static int odbcCGet(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

static int odbcCPut(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

static int odbcCCount(ODBC_t odbc)
{
    int rc = -1;
SPEW(0, rc, odbc);
    return rc;
}

/*==============================================================*/
/* XXX c99 only */
#define	_mkSTMT(_sql, _params)	\
  &(struct _STMT_s) { .sql = _sql, .params = _params }

#ifdef	 NOTYET		/* XXX todo++ */
#define	_mkP1(_chararray) \
  &(_PARAM_t) { \
    &(struct _PARAM_s) { \
	.ParameterNumber = 1, \
	.ValueType = SQL_C_CHAR, \
	.ParameterType = SQL_CHAR, \
	.LengthPrecision = sizeof(_chararray), \
	.ParameterScale = 0, \
	.ParameterValue = _chararray, \
	.Strlen_or_Ind = NULL, \
    }, \
  }
  _mkSTMT("DROP TABLE IF EXISTS Packages", NULL),
  _mkSTMT(
"CREATE TABLE Packages (\n\
    i INTEGER PRIMARY KEY NOT NULL,\n\
    Nvra VARCHAR(64)\n\
)", NULL),
  _mkSTMT("INSERT INTO Packages VALUES( 1, 'Bing' )", NULL),
  _mkSTMT("INSERT INTO Packages VALUES( 2, 'Bang' )", NULL),
  _mkSTMT("INSERT INTO Packages VALUES( 3, 'Boom' )", NULL),
  _mkSTMT("SELECT * FROM Packages", NULL),
  _mkSTMT("DROP TABLE Packages", NULL),
#endif

static _STMT_t _odbc_stmts[] = {
  _mkSTMT("DROP TABLE IF EXISTS TournamentExport", NULL),
  _mkSTMT(
"CREATE TABLE TournamentExport (\n\
	Entry_Sakey	integer NOT NULL,\n\
	TRNumber	varchar(50) NOT NULL,\n\
	FirstName	varchar(50) NOT NULL,\n\
	LastName	varchar(50) NOT NULL,\n\
	City		varchar(50) NULL,\n\
	State		varchar(20) NULL,\n\
	Country		varchar(50) NULL,\n\
	DateOfBirth	timestamp NULL,\n\
	Tournament_Sakey	integer NOT NULL,\n\
	TournamentGame_Sakey	integer NOT NULL,\n\
	TournamentGame_Date	timestamp NOT NULL,\n\
	TournamentName	varchar(50) NOT NULL,\n\
	MultiDayFlag	smallint NOT NULL,\n\
	PitDesc		varchar(50) NULL,\n\
	TableDesc	varchar(50) NULL,\n\
	SeatID		integer NULL,\n\
	EntryNumber	integer NOT NULL,\n\
	VoidFlag	smallint NOT NULL,\n\
	LastUpdated	timestamp NOT NULL\n\
)", NULL),
  _mkSTMT("\
INSERT INTO TournamentExport VALUES(1, '0016701000000', 'Joe', 'Blow', 'Miami', 'FL', 'USA', '1979-03-15T00:00:00.000', 1, 1, '2012-02-01T00:00:00.000', 'Event #1', 0, 'Amazon Blue', '22', 3, 1, 0, '2012-02-29T10:05:00.000')\
", NULL),
  _mkSTMT("\
INSERT INTO TournamentExport VALUES(2, '0016701000001', 'John', 'Smith', 'Las Vegas', 'NV', 'USA', '1980-01-01T00:00:00.000', 2, 3, '2012-02-01T00:00:00.000', 'Event #2', 0, 'Amazon Orange', '133', 6, 1, 0, '2012-02-29T10:05:00.000')\
", NULL),
  _mkSTMT("\
INSERT INTO TournamentExport VALUES(3, '0016701000000', 'Joe', 'Blow', 'Miami', 'FL', 'USA', '1979-03-15T00:00:00.000', 2, 3, '2012-02-01T00:00:00.000', 'Event #2', 0, 'Amazon Orange', '133', 5, 2, 0, '2012-02-29T10:05:00.000')\
", NULL),
  _mkSTMT("\
INSERT INTO TournamentExport VALUES(4, '0016701000002', 'Jason', 'Brown', 'Las Vegas', 'NV', 'USA', '1970-01-01T00:00:00.000', 1, 1, '2012-02-01T00:00:00.000', 'Event #1', 0, 'Amazon Orange', '124', 1, 2, 1, '2012-02-29T10:05:00.000')\
", NULL),
  _mkSTMT("\
INSERT INTO TournamentExport VALUES(5, '0016701000002', 'Jason', 'Brown', 'Las Vegas', 'NV', 'USA', '1970-01-01T00:00:00.000', 1, 1, '2012-02-01T00:00:00.000', 'Event #1', 0, 'Amazon Blue', '22', 4, 3, 1, '2012-02-29T10:05:00.000')\
", NULL),
  _mkSTMT("\
INSERT INTO TournamentExport VALUES(6, '0016701000002', 'Jason', 'Brown', 'Las Vegas', 'NV', 'USA', '1970-01-01T00:00:00.000', 1, 1, '2012-02-01T00:00:00.000', 'Event #1', 0, 'Amazon Blue', '26', 6, 4, 0, '2012-02-29T10:05:00.000')\
", NULL),
  _mkSTMT("\
INSERT INTO TournamentExport VALUES(7, '0016701000002', 'Jason', 'Brown', 'Las Vegas', 'NV', 'USA', '1970-01-01T00:00:00.000', 3, 4, '2012-02-10T00:00:00.000', 'Event #3', 1, 'Amazon Orange', '134', 2, 1, 0, '2012-02-29T10:05:00.000')\
", NULL),
  _mkSTMT("\
INSERT INTO TournamentExport VALUES(8, '0016701000000', 'Joe', 'Blow', 'Miami', 'FL', 'USA', '1979-03-15T00:00:00.000', 3, 5, '2012-02-11T00:00:00.000', 'Event #3', 1, 'Amazon Orange', '134', 2, 1, 0, '2012-02-29T10:05:00.000')\
", NULL),
  _mkSTMT("SELECT * FROM TournamentExport", NULL),
  NULL
};

#undef	_mkSTMT

static int odbcStmt(ODBC_t odbc, _STMT_t stmt, void * _fp)
{
    FILE * fp = (FILE *) (_fp ? _fp : stderr);
    int rc = -1;
    const char * s = stmt->sql;
    _PARAM_t * params = stmt->params;
int xx;
(void)xx;

fprintf(fp, "==> %s\n", s);
    rc = odbcPrepare(odbc, s, 0);
#ifdef	NOTYET
fprintf(fp, "before: %s\n", odbcGetCursorName(odbc));
    xx = odbcSetCursorName(odbc, "poker", 0);
fprintf(fp, " after: %s\n", odbcGetCursorName(odbc));
#endif

    while (params && *params)
	rc = odbcBindParameter(odbc, *params++);

    rc = odbcExecute(odbc);
#ifdef	NOTYET
    xx = odbcCloseCursor(odbc);
#endif
    rc = odbcPrint(odbc, fp);

SPEW(0, rc, odbc);
    return rc;
}

static int odbcRun(ODBC_t odbc, _STMT_t * stmts, void * _fp)
{
    int rc = -1;
int xx;

    while (stmts && *stmts)
	rc = odbcStmt(odbc, *stmts++, _fp);

    xx = odbcCommit(odbc);
    xx = odbcEndTran(odbc, 0);

SPEW(0, rc, odbc);
    return rc;
}

/*==============================================================*/

static struct poptOption odbcOptionsTable[] = {
 { NULL, 'f', POPT_ARG_INT,	&_odbc_flags, -1,
 	NULL, NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext con = rpmioInit(argc, argv, odbcOptionsTable);
    char ** av = (char **) poptGetArgs(con);
    int ac = argvCount((ARGV_t)av);
    const char * _uri = (ac > 1 ? av[1] : _odbc_uri);
    ODBC_t odbc = odbcNew(_uri, _odbc_flags);
    FILE * _odbc_fp = stderr;
    int rc = 0;
    int xx;

fprintf(_odbc_fp, "==> %s\n", "ListDrivers");
    rc = odbcListDrivers(odbc, _odbc_fp);
fprintf(_odbc_fp, "==> %s\n", "ListDataSources");
    rc = odbcListDataSources(odbc, _odbc_fp);

fprintf(_odbc_fp, "==> %s\n", "Connect");
    rc = odbcConnect(odbc, _uri);

    xx = odbcRun(odbc, _odbc_stmts, _odbc_fp);

fprintf(_odbc_fp, "==> %s\n", "Tables");
    xx = odbcTables(odbc, "TournamentExport");
    xx = odbcPrint(odbc, _odbc_fp);
    xx = odbcTables(odbc, NULL);
    xx = odbcCancel(odbc);

fprintf(_odbc_fp, "==> %s\n", "Columns");
    xx = odbcColumns(odbc, "TournamentExport", NULL);
    xx = odbcPrint(odbc, _odbc_fp);
    xx = odbcColumns(odbc, NULL, NULL);
    xx = odbcCancel(odbc);

fprintf(_odbc_fp, "==> %s\n", "Statistics");
    xx = odbcStatistics(odbc, "TournamentExport");
    xx = odbcPrint(odbc, _odbc_fp);
    xx = odbcStatistics(odbc, "TournamentExport");	/* XXX FIXME: !NULL */
    xx = odbcCancel(odbc);

fprintf(_odbc_fp, "<== %s\n", "Disconnect");
    rc = odbcDisconnect(odbc);

    xx = odbcOpen(odbc);
    xx = odbcCOpen(odbc);

    xx = odbcCDup(odbc);
    xx = odbcCDel(odbc);
    xx = odbcCGet(odbc);
    xx = odbcCPut(odbc);
    xx = odbcCCount(odbc);

    xx = odbcCClose(odbc);
    xx = odbcClose(odbc);

    odbc = odbcFree(odbc);

    con = rpmioFini(con);
/*@i@*/ urlFreeCache();

    return rc;
}
