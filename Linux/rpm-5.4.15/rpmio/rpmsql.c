#include "system.h"

#include <popt.h>

#define	_RPMIOB_INTERNAL	/* rpmiobSlurp */
#include "rpmio_internal.h"	/* XXX fdGetFILE */
#include <rpmmacro.h>
#include <rpmdir.h>
#include <rpmurl.h>
#include <mire.h>

#if defined(WITH_DBSQL)
#include <dbsql.h>
#elif defined(WITH_SQLITE)
#define SQLITE_OS_UNIX 1
#define SQLITE_THREADSAFE 1
#define SQLITE_THREAD_OVERRIDE_LOCK -1
#define SQLITE_TEMP_STORE 1
#include <sqlite3.h>
#endif	/* WITH_SQLITE */

#define	_RPMSQL_INTERNAL
#define	_RPMVT_INTERNAL
#define	_RPMVC_INTERNAL
#include <rpmsql.h>

#ifdef	NOTYET		/* XXX FIXME */
#include <editline/readline.h>
#elif defined(HAVE_READLINE) && HAVE_READLINE==1
# include <readline/readline.h>
# include <readline/history.h>
#endif

# define readline(sql, p) local_getline(sql, p)
# define add_history(X)
# define read_history(X)
# define write_history(X)
# define stifle_history(X)

#include "debug.h"

/*@unchecked@*/
int _rpmsql_debug = 0;

/*@unchecked@*/
int _rpmvt_debug = 0;

/*@unchecked@*/
int _rpmvc_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmsql _rpmsqlI = NULL;

/*@unchecked@*/
volatile int _rpmsqlSeenInterrupt;

#if defined(WITH_SQLITE)
/*@unchecked@*/
static struct rpmsql_s _sql;
#endif /* defined(WITH_SQLITE) */

/*==============================================================*/

#define VTDBG(_vt, _l) if ((_vt) && (_vt)->debug) fprintf _l
#define VTDBGNOISY(_vt, _l) if ((_vt) && (_vt)->debug < 0) fprintf _l

/**
 * rpmvt pool destructor.
 */
static void rpmvtFini(void * _VT)
	/*@globals fileSystem @*/
	/*@modifies *_VT, fileSystem @*/
{
    struct rpmVT_s * VT = _VT;
    rpmvt vt = &VT->vt;
    

VTDBGNOISY(vt, (stderr, "==> %s(%p)\n", __FUNCTION__, vt));
    vt->argv = argvFree(vt->argv);
    vt->argc = 0;
    vt->fields = argvFree(vt->fields);
    vt->nfields = 0;
    vt->cols = argvFree(vt->cols);
    vt->ncols = 0;
    vt->av = argvFree(vt->av);
    vt->ac = 0;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmvtPool;

static rpmvt rpmvtGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmvtPool, fileSystem @*/
	/*@modifies pool, _rpmvtPool, fileSystem @*/
{
    struct rpmVT_s * VT;

    if (_rpmvtPool == NULL) {
	_rpmvtPool = rpmioNewPool("vt", sizeof(*VT), -1, _rpmvt_debug,
			NULL, NULL, rpmvtFini);
	pool = _rpmvtPool;
    }
    VT = (struct rpmVT_s *) rpmioGetPool(pool, sizeof(*VT));
    memset(((char *)VT)+sizeof(VT->_item), 0, sizeof(*VT)-sizeof(VT->_item));
    return &VT->vt;
}

rpmvt rpmvtNew(void * db, void * pModule, const char *const * argv, rpmvd vd)
{
    rpmvt vt = rpmvtLink(rpmvtGetPool(_rpmvtPool));

    vt->db = db;
    (void) argvAppend(&vt->argv, (ARGV_t) argv);
    vt->argc = argvCount(vt->argv);
    if (vd->split && vd->parse && *vd->parse) {
	char * parse = rpmExpand(vd->parse, NULL);
	int xx;
	xx = argvSplit(&vt->fields, parse, vd->split);
assert(xx == 0);
	vt->nfields = argvCount(vt->fields);
	parse = _free(parse);
    }

    vt->av = NULL;
    vt->ac = 0;

    vt->vd = vd;
    vt->debug = _rpmvt_debug;

VTDBG(vt, (stderr, "\tdbpath: %s\n", vd->dbpath));
VTDBG(vt, (stderr, "\tprefix: %s\n", vd->prefix));
VTDBG(vt, (stderr, "\t split: %s\n", vd->split));
VTDBG(vt, (stderr, "\t parse: %s\n", vd->parse));
VTDBG(vt, (stderr, "\t regex: %s\n", vd->regex));

    return vt;
}

/*==============================================================*/

#if defined(WITH_SQLITE)

typedef struct key_s {
    const char * k;
    uint32_t v;
} KEY;
static KEY sqlTypes[] = {
    { "blob",	SQLITE_BLOB },
    { "float",	SQLITE_FLOAT },
    { "int",	SQLITE_INTEGER },
    { "integer",SQLITE_INTEGER },
    { "null",	SQLITE_NULL },
    { "text",	SQLITE_TEXT },
};
static size_t nsqlTypes = sizeof(sqlTypes) / sizeof(sqlTypes[0]);

static const char * hasSqlType(const char * s)
{
    int i;
    for (i = 0; i < (int)nsqlTypes; i++) {
	const char * k = sqlTypes[i].k;
	const char * se = strcasestr(s, k);
	if (se == NULL || se <= s || se[-1] != ' ')
	    continue;
	se += strlen(k);
	if (*se && *se != ' ')
	    continue;
	return se;
    }
    return NULL;
}

static char * _rpmvtJoin(const char * a, const char ** argv, const char * z)
{
    static const char _type[] = " TEXT";
    const char ** av;
    size_t na = (sizeof("\t")-1) + (a ? strlen(a) : 0);
    size_t nb = 0;
    size_t nz = (z ? strlen(z) : 0) + strlen(_type) + (sizeof(",\n")-1);
    char *t, *te;

    for (av = argv; *av != NULL; av++)
	nb += na + strlen(*av) + nz;

    te = t = xmalloc(nb + 1);
    for (av = argv; *av != NULL; av++) {
	*te++ = '\t';
	if (a)
	    te = stpcpy(te, a);
	te = stpcpy(te, *av);
	if (hasSqlType(*av) == NULL)
	    te = stpcpy(te, _type);
	if (z)
	    te = stpcpy(te, z);
	*te++ = ',';
	*te++ = '\n';
    }
    *te = '\0';

    return t;
}

static char * _rpmvtAppendCols(rpmvt vt, const char ** av)
{
    char * h = _rpmvtJoin("", av, "");
    int xx = argvAppend(&vt->cols, av);
    char * u;
    char * hu;
    /* XXX permit user column overrides w/o a argv[3] selector. */
    rpmvd vd = vt->vd;
    int fx = (vd->fx == 3 ? 3 : 4);

    av = (const char **) (vt->argc > fx ? &vt->argv[fx] : vt->fields);
assert(av);
    u = _rpmvtJoin("", av, "");
    u[strlen(u)-2] = ' ';	/* XXX nuke the final comma */
    xx = argvAppend(&vt->cols, av);

#define	dbN	vt->argv[1]
#define	tblN	vt->argv[2]
    hu = rpmExpand("CREATE TABLE ", dbN, ".", tblN, " (\n", h, u, ");", NULL);
#undef	dbN
#undef	tblN

    u = _free(u);
    h = _free(h);

VTDBG(vt, (stderr, "%s\n", hu));
    return hu;
}

int rpmvtLoadArgv(rpmvt vt, rpmvt * vtp)
{
    sqlite3 * db = (sqlite3 *) vt->db;
    rpmvd vd = vt->vd;

    static const char * hidden[] = { "path HIDDEN", "id HIDDEN", NULL };
    const char * hu;

    char * uri = NULL;
    struct stat sb;

    const char * fn = NULL;

    int rc = SQLITE_OK;
    int xx;

VTDBG(vt, (stderr, "--> %s(%p,%p)\n", __FUNCTION__, vt, vtp));
if (vt->debug)
argvPrint("vt->argv", (ARGV_t)vt->argv, NULL);

    /* Set the columns in the schema. */
    hu = _rpmvtAppendCols(vt, hidden);
    rc = rpmsqlCmd(NULL, "declare_vtab", db,
			sqlite3_declare_vtab(db, hu));
    hu = _free(hu);

    if (vt->argv[3]) {
	/* XXX slice out the quotes that sqlite passes through ...  */
	static char _quotes[] = "'\"";
	int quoted = (strchr(_quotes, *vt->argv[3]) != NULL);
	const char * prefix;
	const char * path = NULL;
	/* XXX Prefer user override to global prefix (if absolute path). */
	(void) urlPath(vt->argv[3]+quoted, &path);
	prefix = (*path != '/' && vd->prefix ? vd->prefix : "");
	uri = rpmGetPath(prefix, path, NULL);
	uri[strlen(uri)-quoted] = '\0';
    } else
	uri = rpmGetPath(vd->prefix, fn, NULL);

    (void) urlPath(uri, (const char **) &fn);

    if (!strcasecmp(vt->argv[0], "nixdb")) {
	const char * out = rpmExpand("%{sql ", vd->dbpath, ":",
	    "select path from ValidPaths where glob('", fn, "', path);",
		"}", NULL);
	(void) argvSplit(&vt->av, out, "\n");
	out = _free(out);
    } else

    if (!strcasecmp(vt->argv[0], "Env")) {
	int fx = 4;	/* XXX user column overrides? */
if (vt->debug)
fprintf(stderr, " ENV: getenv(%p[%d])\n", &vt->argv[fx], argvCount(&vt->argv[fx]));
	/* XXX permit glob selector filtering from argv[3]? */
	xx = argvAppend(&vt->av, (ARGV_t)environ);
    } else

    if (fn[0] == '/') {
if (vt->debug)
fprintf(stderr, "*** uri %s fn %s\n", uri, fn);
	if (Glob_pattern_p(uri, 0)) {	/* XXX uri */
	    const char ** av = NULL;
	    int ac = 0;
		
	    xx = rpmGlob(uri, &ac, &av);	/* XXX uri */
if (vt->debug)
fprintf(stderr, "GLOB: %d = Glob(%s) av %p[%d]\n", xx, uri, av, ac);
	    if (xx)
		rc = SQLITE_NOTFOUND;		/* XXX */
	    else
		xx = argvAppend(&vt->av, (ARGV_t)av);
	    av = argvFree(av);
	} else
	if (uri[strlen(uri)-1] == '/') {
	    DIR * dir = Opendir(uri);
	    struct dirent * dp;
if (vt->debug)
fprintf(stderr, " DIR: %p = Opendir(%s)\n", dir, uri);
	    if (dir == NULL)
		rc = SQLITE_NOTFOUND;		/* XXX */
	    else
	    while ((dp = Readdir(dir)) != NULL) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
		    continue;
		fn = rpmGetPath(uri, "/", dp->d_name, NULL);
		xx = argvAdd(&vt->av, fn);
		fn = _free(fn);
	    }
	    if (dir) xx = Closedir(dir);
	} else
	if (!Lstat(uri, &sb)) {
	    rpmiob iob = NULL;
	    xx = rpmiobSlurp(uri, &iob);
if (vt->debug)
fprintf(stderr, "FILE: %d = Slurp(%s)\n", xx, uri);
	    if (!xx)
		xx = argvSplit(&vt->av, rpmiobStr(iob), "\n");
	    else
		rc = SQLITE_NOTFOUND;		/* XXX */
	    iob = rpmiobFree(iob);
	} else
	    rc = SQLITE_NOTFOUND;		/* XXX */
    } else {
	xx = argvAppend(&vt->av, (ARGV_t)&vt->argv[3]);
if (vt->debug)
fprintf(stderr, "LIST: %d = Append(%p[%d])\n", xx, &vt->argv[3], argvCount(&vt->argv[3]));
    }

    vt->ac = argvCount((ARGV_t)vt->av);

    uri = _free(uri);

if (vt->debug)
argvPrint("vt->av", (ARGV_t)vt->av, NULL);

    if (vtp) {
	if (!rc)
	    *vtp = (rpmvt) vt;
	else {
	    *vtp = NULL;
	    (void) rpmvtFree(vt);
	    vt = NULL;
	}
    } else {
	vt = rpmvtFree(vt);
	vt = NULL;
    }

VTDBG(vt, (stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, vt, vtp, rc));

    return rc;
}

/*==============================================================*/

static struct rpmvd_s _argVD = {
};

int rpmvtCreate(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_argVD), vtp);
}

int rpmvtConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_argVD), vtp);
}

#ifdef	NOTYET
static void dumpInfo(const char * msg, const struct sqlite3_index_info * s)
{
fprintf(stderr, "--------------------- %s\n", (msg ? msg : ""));
#define _PRT(f,v) fprintf(stderr, "%20s: " #f "\n", #v, s->v)
        _PRT(%p, aConstraintUsage);
        _PRT(%d, idxNum);
        _PRT(%s, idxStr);
        _PRT(%d, needToFreeIdxStr);
        _PRT(%d, orderByConsumed);
        _PRT(%g, estimatedCost);
#undef  _PRT
}
#endif

int rpmvtBestIndex(rpmvt vt, void * _pInfo)
{
    sqlite3_index_info * pInfo = (sqlite3_index_info *) _pInfo;
    int rc = SQLITE_OK;
#ifdef	NOTYET
    int i;
#endif

VTDBG(vt, (stderr, "--> %s(%p,%p)\n", __FUNCTION__, vt, pInfo));

#ifdef	NOTYET
    if (pInfo->aConstraint)
    for (i = 0; i < pInfo->nConstraint; i++) {
	const struct sqlite3_index_constraint * p = pInfo->aConstraint + i;
	fprintf(stderr, "\tcol %s(%d) 0x%02x 0x%02x\n", vt->cols[p->iColumn], p->iColumn,
		p->op, p->usable);
    }
    if (pInfo->aOrderBy)
    for (i = 0; i < pInfo->nOrderBy; i++) {
	const struct sqlite3_index_orderby * p = pInfo->aOrderBy + i;
	fprintf(stderr, "\tcol %s(%d) %s\n", vt->cols[p->iColumn], p->iColumn,
		(p->desc ? "DESC" : "ASC"));
    }
    dumpInfo(__FUNCTION__, pInfo);
#endif

VTDBG(vt, (stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, vt, pInfo, rc));

    return rc;
}

int rpmvtDisconnect(rpmvt vt)
{
    (void) rpmvtFree(vt);
    return 0;	/* SQLITE_OK */
}

int rpmvtDestroy(rpmvt vt)
{
    (void) rpmvtFree(vt);
    return 0;	/* SQLITE_OK */
}

static const char * dumpArg(rpmvArg _v)
{
    static char buf[BUFSIZ];
    char * b = buf;
    size_t nb = sizeof(buf);
    sqlite3_value * v = (sqlite3_value *) _v;
    int vtype = sqlite3_value_type(v);
    unsigned long long ll;
    double d;
    const void * p;
    const char * s;

    snprintf(b, nb, "%p(%d)", v, vtype);
    nb -= strlen(b);
    b += strlen(b);

    switch (vtype) {
    case SQLITE_INTEGER:
	ll = (unsigned long long) sqlite3_value_int64(v);
	snprintf(b, nb, " INT  %lld", ll);
	break;
    case SQLITE_FLOAT:
	d = sqlite3_value_double(v);
	snprintf(b, nb, " REAL %g", d);
	break;
    case SQLITE_BLOB:
	p = sqlite3_value_blob(v);
	snprintf(b, nb, " BLOB %p", p);
	break;
    case SQLITE_NULL:
	snprintf(b, nb, " NULL");
	break;
    case SQLITE_TEXT:
	s = (const char *)sqlite3_value_text(v);
	snprintf(b, nb, " TEXT \"%s\"", s);
	break;
    default:
	snprintf(b, nb, " UNKNOWN");
	break;
    }

    return buf;
}

static void dumpArgv(const char * msg, int argc, rpmvArg * _argv)
{
    if (argc > 0 && _argv) {
	int i;
	fprintf(stderr, "--------------------- %s\n", (msg ? msg : ""));
	for (i = 0; i < argc; i++)
	    fprintf(stderr, "\targv[%d] %s\n", i, dumpArg(_argv[i]));
    }
}

int rpmvtUpdate(rpmvt vt, int argc, rpmvArg * _argv, int64_t * pRowid)
{
    sqlite3_value ** argv = (sqlite3_value **) _argv;
    int rc = SQLITE_OK;

VTDBG(vt, (stderr, "--> %s(%p,%p[%u],%p)\n", __FUNCTION__, vt, argv, (unsigned)argc, pRowid));

    if (argc == 0 || argv == NULL) {
if (vt->debug)
dumpArgv("ERROR", argc, _argv);
	rc = SQLITE_NOTFOUND;	/* XXX */
    } else
    if (argc == 1) {
VTDBG(vt, (stderr, "\tDELETE ROW 0x%llx\n", *(unsigned long long *)argv[0]));
    } else
    if (argv[0] == NULL) {
VTDBG(vt, (stderr, "\tADD ROW 0x%llx\n", *(unsigned long long *)argv[1]));
if (vt->debug)
dumpArgv("ADD ROW", argc, _argv);
    } else
    if (argv[0] == argv[1]) {
VTDBG(vt, (stderr, "\tUPDATE ROW 0x%llx\n", *(unsigned long long *)argv[1]));
if (vt->debug)
dumpArgv("UPDATE argv", argc-2, _argv+2);
    } else {
VTDBG(vt, (stderr, "\tREPLACE ROW 0x%llx from 0x%llx\n",
		*(unsigned long long *)argv[0], *(unsigned long long *)argv[1]));
if (vt->debug)
dumpArgv("REPLACE argv", argc-2, _argv+2);
    }

VTDBG(vt, (stderr, "<-- %s(%p,%p[%u],%p) rc %d\n", __FUNCTION__, vt, argv, (unsigned)argc, pRowid, rc));
    return rc;
}

int rpmvtBegin(rpmvt vt)
{
    int rc = SQLITE_OK;
VTDBG(vt, (stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, vt, rc));
    return rc;
}

int rpmvtSync(rpmvt vt)
{
    int rc = SQLITE_OK;
VTDBG(vt, (stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, vt, rc));
    return rc;
}

int rpmvtCommit(rpmvt vt)
{
    int rc = SQLITE_OK;
VTDBG(vt, (stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, vt, rc));
    return rc;
}

int rpmvtRollback(rpmvt vt)
{
    int rc = SQLITE_OK;
VTDBG(vt, (stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, vt, rc));
    return rc;
}

int rpmvtFindFunction(rpmvt vt, int nArg, const char * zName,
		void (**pxFunc)(void *, int, rpmvArg *),
		void ** ppArg)
{
    int rc = SQLITE_OK;
VTDBG(vt, (stderr, "<-- %s(%p,%d,%s,%p,%p) rc %d\n", __FUNCTION__, vt, nArg, zName, pxFunc, ppArg, rc));
    return rc;
}

int rpmvtRename(rpmvt vt, const char * zNew)
{
    int rc = SQLITE_OK;
VTDBG(vt, (stderr, "<-- %s(%p,%s) rc %d\n", __FUNCTION__, vt, zNew, rc));
    return rc;
}
#endif /* defined(WITH_SQLITE) */

/*==============================================================*/

#define VCDBG(_vc, _l) if ((_vc)->debug) fprintf _l
#define VCDBGNOISY(_vc, _l) if ((_vc)->debug < 0) fprintf _l

/**
 * rpmvc pool destructor.
 */
static void rpmvcFini(void * _VC)
	/*@globals fileSystem @*/
	/*@modifies *_VC, fileSystem @*/
{
    struct rpmVC_s * VC = _VC;
    rpmvc vc = &VC->vc;

VCDBGNOISY(vc, (stderr, "==> %s(%p)\n", __FUNCTION__, vc));
    if (vc->vt)
	(void) rpmvtFree(vc->vt);
    vc->vt = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmvcPool;

static rpmvc rpmvcGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmvcPool, fileSystem @*/
	/*@modifies pool, _rpmvcPool, fileSystem @*/
{
    struct rpmVC_s * VC;

    if (_rpmvcPool == NULL) {
	_rpmvcPool = rpmioNewPool("vc", sizeof(*VC), -1, _rpmvc_debug,
			NULL, NULL, rpmvcFini);
	pool = _rpmvcPool;
    }
    VC = (struct rpmVC_s *) rpmioGetPool(pool, sizeof(*VC));
    memset(((char *)VC)+sizeof(VC->_item), 0, sizeof(*VC)-sizeof(VC->_item));
    return &VC->vc;
}

rpmvc rpmvcNew(rpmvt vt, int nrows)
{
    rpmvc vc = rpmvcLink(rpmvcGetPool(_rpmvcPool));

    vc->vt = rpmvtLink(vt);
    vc->ix = -1;

    vc->debug = _rpmvc_debug;
    vc->nrows = nrows;
    vc->vd = NULL;

    return vc;
}

/*==============================================================*/

#if defined(WITH_SQLITE)

int rpmvcOpen(rpmvt vt, rpmvc * vcp)
{
    rpmvc vc = rpmvcNew(vt, vt->ac);
    int rc = SQLITE_OK;

    if (vcp)
	*vcp = vc;
    else
	(void) rpmvcFree(vc);

    return rc;
}

int rpmvcClose(rpmvc vc)
{
    /* XXX unnecessary but the debug spewage is confusing. */
    if (vc->vt)
	(void) rpmvtFree(vc->vt);
    vc->vt = NULL;
    (void) rpmvcFree(vc);
    return 0;	/* SQLITE_OK */
}

int rpmvcFilter(rpmvc vc, int idxNum, const char * idxStr,
		int argc, rpmvArg * _argv)
{
    sqlite3_value ** argv = (sqlite3_value **) _argv;
    int rc = SQLITE_OK;

VCDBGNOISY(vc, (stderr, "--> %s(%p,%d,%s,%p[%u]) [%d:%d]\n", __FUNCTION__, vc, idxNum, idxStr, argv, (unsigned)argc, vc->ix, vc->nrows));
dumpArgv(__FUNCTION__, argc, _argv);

    if (vc->nrows > 0)
	vc->ix = 0;

VCDBGNOISY(vc, (stderr, "<-- %s(%p,%d,%s,%p[%u]) [%d:%d] rc %d\n", __FUNCTION__, vc, idxNum, idxStr, argv, (unsigned)argc, vc->ix, vc->nrows, rc));

    return rc;
}

int rpmvcNext(rpmvc vc)
{
    int rc = SQLITE_OK;

    if (vc->ix >= 0 && vc->ix < vc->nrows)		/* XXX needed? */
	vc->ix++;

if (!(vc->ix >= 0 && vc->ix < vc->nrows))
VCDBGNOISY(vc, (stderr, "<-- %s(%p) rc %d (%d:%d)\n", __FUNCTION__, vc, rc, vc->ix, vc->nrows));
    return rc;
}

int rpmvcEof(rpmvc vc)
{
    int rc = (vc->ix >= 0 && vc->ix < vc->nrows ? 0 : 1);
    
if (rc)
VCDBGNOISY(vc, (stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, vc, rc));
    return rc;
}

/*==============================================================*/

int rpmvcColumn(rpmvc vc, void * _pContext, int colx)
{
    sqlite3_context * pContext = (sqlite3_context *) _pContext;
    rpmvt vt = vc->vt;
    rpmvd vd = vt->vd;
    const char * path = vt->av[vc->ix];
    const char * col = vt->cols[colx];
    int rc = SQLITE_OK;

    size_t nb;
    miRE mire = NULL;
    int noffsets = 0;
    int * offsets = NULL;
    int xx;
    int i;

    /* Use a PCRE pattern for parsing column value. */
    if (vd->regex) {
	mire = mireNew(RPMMIRE_REGEX, 0);
	xx = mireSetCOptions(mire, RPMMIRE_REGEX, 0, 0, NULL);
	xx = mireRegcomp(mire, vd->regex);	/* XXX move to rpmvtNew */

	noffsets = 10 * 3;
	nb = noffsets * sizeof(*offsets);
	offsets = memset(alloca(nb), -1, nb);
	xx = mireSetEOptions(mire, offsets, noffsets);

nb = strlen(path);
	xx = mireRegexec(mire, path, nb);
assert(xx == 0);

	for (i = 0; i < noffsets; i += 2) {
	    if (offsets[i] < 0)
		continue;
assert(offsets[i  ] >= 0 && offsets[i  ] <= (int)nb);
assert(offsets[i+1] >= 0 && offsets[i+1] <= (int)nb);
	    offsets[i+1] -= offsets[i];	/* XXX convert offset to length */
VCDBGNOISY(vc, (stderr, "\t%d [%d,%d] %.*s\n", i/2, offsets[i], offsets[i+1], offsets[i+1], path+offsets[i]));
	}

    }

    if (!strcmp(col, "path"))
	sqlite3_result_text(pContext, path, -1, SQLITE_STATIC);
    else
    if (vd->regex) {
	/* Use a PCRE pattern for parsing column value. */
assert(vt->fields);
	for (i = 0; i < vt->nfields; i++) {
	    /* Slurp file contents for unknown field values. */
	    /* XXX procdb/yumdb */
	    /* XXX uri's? */
	    if (path[0] == '/' && !strcmp("*", vt->fields[i])) {
		const char * fn = rpmGetPath(path, "/", col, NULL);
		if (!Access(fn, R_OK)) {
		    rpmiob iob = NULL;
		    xx = rpmiobSlurp(fn, &iob);
		    sqlite3_result_text(pContext, rpmiobStr(iob), rpmiobLen(iob), SQLITE_TRANSIENT);
		    iob = rpmiobFree(iob);
		} else
		    sqlite3_result_null(pContext);
		break;
	    } else
	    if (!strcmp(col, vt->fields[i])) {
		int ix = 2 * (i + 1);
		const char * s = path + offsets[ix];
		size_t ns = offsets[ix+1]; /* XXX convert offset to length */
		sqlite3_result_text(pContext, s, ns, SQLITE_STATIC);
		break;
	    }
	}
	if (i == vt->nfields)
	    sqlite3_result_null(pContext);
    } else
    if (vd->split && strlen(vd->split) == 1 && vt->nfields > 0) {
	/* Simple argv split on a separator char. */
	/* XXX using argvSplit has extra malloc's, needs SQLITE_TRANSIENT */
	ARGV_t av = NULL;	/* XXX move to rpmvcNext for performance */
	xx = argvSplit(&av, path, vd->split);
assert(vt->fields);
	for (i = 0; i < vt->nfields; i++) {
	    if (strcmp(col, vt->fields[i]))
		continue;
	    sqlite3_result_text(pContext, av[i], -1, SQLITE_TRANSIENT);
	    break;
	}
	if (i == vt->nfields)
	    sqlite3_result_null(pContext);
	av = argvFree(av);
    } else
	sqlite3_result_null(pContext);	/* XXX unnecessary */

    if (mire) {
	xx = mireSetEOptions(mire, NULL, 0);
	mire = mireFree(mire);
    }

if (rc)
VCDBG(vc, (stderr, "<-- %s(%p,%p,%d) rc %d\n", __FUNCTION__, vc, pContext, colx, rc));

    return rc;
}

int rpmvcRowid(rpmvc vc, int64_t * pRowid)
{
    int rc = SQLITE_OK;

    if (pRowid)
	*pRowid = vc->ix;

if (rc)
VCDBG(vc, (stderr, "<-- %s(%p,%p) rc %d rowid 0x%llx\n", __FUNCTION__, vc, pRowid, rc, (unsigned long long)(pRowid ? *pRowid : 0xf00)));
    return rc;
}
#endif /* defined(WITH_SQLITE) */

/*==============================================================*/

static void _rpmsqlDebugDump(rpmsql sql,
		const char * _func, const char * _fn, unsigned _ln)
{
SQLDBG((stderr, "==> %s:%u %s(%p) _rpmsqlI %p\n", _fn, _ln, _func, sql, _rpmsqlI));
    if (sql) {
	fprintf(stderr, "\t    flags: 0x%x\n", sql->flags);
	fprintf(stderr, "\t       av: %p[%u]\n", sql->av, (unsigned)argvCount(sql->av));
	fprintf(stderr, "\t        I: %p\n", sql->I);
	fprintf(stderr, "\t        S: %p\n", sql->S);
	fprintf(stderr, "\t     init: %s\n", sql->zInitFile);
	fprintf(stderr, "\t database: %s\n", sql->zDbFilename);
	fprintf(stderr, "\t    table: %s\n", sql->zDestTable);

	fprintf(stderr, "\t     mode: 0x%x\n", sql->mode);
	fprintf(stderr, "\t      cnt: 0x%x\n", sql->cnt);
	fprintf(stderr, "\t      iob: %p\n", sql->iob);
	fprintf(stderr, "\t   IN ifd: %p\n", sql->ifd);
	fprintf(stderr, "\t  OUT ofd: %p\n", sql->ofd);
	fprintf(stderr, "\t  LOG lfd: %p\n", sql->lfd);
	fprintf(stderr, "\tTRACE tfd: %p\n", sql->tfd);

	if (sql->explainPrev.valid) {
	    fprintf(stderr, "\t  explain:\n");
	    fprintf(stderr, "\t\t mode: 0x%x\n", sql->explainPrev.mode);
	    fprintf(stderr, "\t\tflags: 0x%x\n", sql->explainPrev.flags);
	}

	fprintf(stderr, "\tseparator: %.*s\n", (int)sizeof(sql->separator), sql->separator);
	fprintf(stderr, "\tnullvalue: %.*s\n", (int)sizeof(sql->nullvalue), sql->nullvalue);
	fprintf(stderr, "\t  outfile: %s\n", sql->outfile);
	fprintf(stderr, "\t     home: %s\n", sql->zHome);
	fprintf(stderr, "\t   initrc: %s\n", sql->zInitrc);
	fprintf(stderr, "\t  history: %s\n", sql->zHistory);
	fprintf(stderr, "\t   prompt: %s\n", sql->zPrompt);
	fprintf(stderr, "\t continue: %s\n", sql->zContinue);

	fprintf(stderr, "\t      buf: %p[%u]\n", sql->buf, (unsigned)sql->nbuf);
	fprintf(stderr, "\t        b: %p[%u]\n", sql->b, (unsigned)sql->nb);
    }
}
#define	rpmsqlDebugDump(_sql) \
	_rpmsqlDebugDump(_sql, __FUNCTION__, __FILE__, __LINE__)

#if defined(WITH_SQLITE)
/**
 * Print an error message and exit (if requested).
 * @param lvl		error level (1 adds "Error: ", >=2 exits)
 * @param fmt		msg format
 */
/*@mayexit@*/ /*@printflike@*/
static void rpmsql_error(int lvl, const char *fmt, ...)
#if defined(__GNUC__) && __GNUC__ >= 2
	__attribute__((format (printf, 2, 3)))
#endif
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
static void
rpmsql_error(int lvl, const char *fmt, ...)
{
    va_list ap;

    (void) fflush(NULL);
    if (lvl >= 1)
	(void) fprintf(stderr, "Error: ");
    va_start(ap, fmt);
    (void) vfprintf(stderr, fmt, ap);
    va_end (ap);
    (void) fprintf(stderr, "\n");
    if (lvl > 1)
	exit(EXIT_FAILURE);
}

/**
 * Check sqlite3 return, displaying error messages.
 * @param sql		sql interpreter
 * @return		SQLITE_OK on success
 */
int rpmsqlCmd(/*@null@*/ rpmsql sql, const char * msg, void * _db, int rc)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    sqlite3 * db;

    switch (rc) {
    case SQLITE_OK:
    case SQLITE_ROW:
    case SQLITE_DONE:
	/* XXX ignore noisy debug spewage */
	if (1 || !_rpmsql_debug)
	    break;
	/*@fallthrough@*/
    default:
	/* XXX system sqlite3 w loadable modules */
	if (sql)
	    db = (sqlite3 *) (_db ? _db : sql->I);
	else
	    db = (sqlite3 *) _db;
	rpmsql_error(0, "sqlite3_%s(%p): rc(%d) %s", msg, db, rc,
		sqlite3_errmsg(db));
	break;
    }
    return rc;
}
#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

#if defined(WITH_SQLITE)
/**
 * Begin timing an operation
 * @param sql		sql interpreter
 */
static void _rpmsqlBeginTimer(rpmsql sql)
{
    if (sql->enableTimer)
	getrusage(RUSAGE_SELF, &sql->sBegin);
}

/* Return the difference of two time_structs in seconds */
static double timeDiff(struct timeval *pStart, struct timeval *pEnd)
{
    return (pEnd->tv_usec - pStart->tv_usec) * 0.000001 +
	(double) (pEnd->tv_sec - pStart->tv_sec);
}

/**
 * Print the timing results.
 * @param sql		sql interpreter
 */
static void _rpmsqlEndTimer(rpmsql sql)
{
    if (sql->enableTimer) {
	struct rusage sEnd;
	char b[BUFSIZ];
	size_t nb;
	size_t nw;
assert(sql->ofd);
	getrusage(RUSAGE_SELF, &sEnd);
	snprintf(b, sizeof(b), "CPU Time: user %f sys %f\n",
	       timeDiff(&sql->sBegin.ru_utime, &sEnd.ru_utime),
	       timeDiff(&sql->sBegin.ru_stime, &sEnd.ru_stime));
	nb = strlen(b);
	nw = Fwrite(b, 1, nb, sql->ofd);
assert(nb == nw);
    }
}

#define BEGIN_TIMER(_sql)	 _rpmsqlBeginTimer(_sql)
#define END_TIMER(_sql)		 _rpmsqlEndTimer(_sql)
#define HAS_TIMER 1

#define ArraySize(X)  (int)(sizeof(X)/sizeof(X[0]))
#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

/**
 * Return the global interpreter, creating laziliy if needed.
 * @return 		global sql interpreter
 */
static rpmsql rpmsqlI(void)
	/*@globals _rpmsqlI @*/
	/*@modifies _rpmsqlI @*/
{
    if (_rpmsqlI == NULL)
	_rpmsqlI = rpmsqlNew(NULL, 0);
SQLDBG((stderr, "<== %s() _rpmsqlI %p\n", __FUNCTION__, _rpmsqlI));
    return _rpmsqlI;
}

#if defined(WITH_SQLITE)
/**
 * Format output and send to file or iob (as requested).
 * @param sql		sql interpreter
 * @param fmt		format string
 */
/*@printflike@*/
static int rpmsqlFprintf(rpmsql sql, const char *fmt, ...)
#if defined(__GNUC__) && __GNUC__ >= 2
	__attribute__((format (printf, 2, 3)))
#endif
	/*@*/;
static int rpmsqlFprintf(rpmsql sql, const char *fmt, ...)
{
    char b[BUFSIZ];
    size_t nb = sizeof(b);
    int rc;
    va_list ap;

    if (sql == NULL) sql = rpmsqlI();
assert(sql);

    /* Format the output */
    va_start(ap, fmt);
    rc = vsnprintf(b, nb, fmt, ap);
    va_end(ap);
    /* XXX just in case */
    if (!(rc >= 0 && rc < (int)nb))
	rc = nb - 1;
    b[rc] = '\0';

    /* Dispose of the output. */
    if (sql->ofd) {
	size_t nw = Fwrite(b, 1, rc, sql->ofd);
assert((int)nw == rc);
    }
    if (sql->iob)
	(void) rpmiobAppend(sql->iob, b, 0);

    return rc;
}

/**
 * This routine works like printf in that its first argument is a
 * format string and subsequent arguments are values to be substituted
 * in place of % fields.  The result of formatting this string
 * is written to sql->tfd.
 */
#ifdef SQLITE_ENABLE_IOTRACE
static void iotracePrintf(const char *zFormat, ...)
{
    char * z;
    size_t nz;
    size_t nw;
    va_list ap;

    if (_rpmsqlI == NULL || _rpmsqlI->tfd == NULL)
	return;
    va_start(ap, zFormat);
    z = sqlite3_vmprintf(zFormat, ap);
    va_end(ap);
    nz = strlen(z);
    nw = Fwrite(z, 1, nz, sql->tfd);
assert(nz == nw);
    sqlite3_free(z);
}
#endif

#if defined(SQLITE_CONFIG_LOG)
/**
 * A callback for the sqlite3_log() interface.
 * @param _sql		sql interpreter
 */
static void shellLog(void *_sql, int iErrCode, const char *zMsg)
{
    rpmsql sql = (rpmsql) _sql;
    if (sql && sql->lfd) {
	char num[32];
	int xx = snprintf(num, sizeof(num), "(%d) ", iErrCode);
	const char * t = rpmExpand(num, zMsg, "\n", NULL);
	size_t nt = strlen(t);
	size_t nw = Fwrite(t, 1, nt, sql->lfd);
assert(nt == nw);
	xx = Fflush(sql->lfd);
    }
}
#endif

/*==============================================================*/
/**
 * X is a pointer to the first byte of a UTF-8 character.  Increment
 * X so that it points to the next character.  This only works right
 * if X points to a well-formed UTF-8 string.
 */
#ifdef	NOTYET	/* XXX figger multibyte char's. */
#define sqliteNextChar(X)	while( (0xc0&*++(X))==0x80 ){}
#define sqliteCharVal(X)	sqlite3ReadUtf8(X)
#else
#define sqliteNextChar(X)	while( (     *++(X))       ) break
#define sqliteCharVal(X)	        (int)(*(X))
#endif

#include	<math.h>

/**
 * This is a macro that facilitates writting wrappers for math.h functions
 * it creates code for a function to use in SQlite that gets one numeric input
 * and returns a floating point value.
 *
 * Could have been implemented using pointers to functions but this way it's inline
 * and thus more efficient. Lower * ranking though...
 * 
 * Parameters:
 * name:      function name to de defined (eg: sinFunc)
 * function:  function defined in math.h to wrap (eg: sin)
 * domain:    boolean condition that CAN'T happen in terms of the input parameter rVal
 *            (eg: rval<0 for sqrt)
 */
#define GEN_MATH_WRAP_DOUBLE_1(name, function, domain) \
static void name(sqlite3_context *context, int argc, sqlite3_value **argv) {\
  double rVal = 0.0;\
assert(argc==1);\
  switch (sqlite3_value_type(argv[0])) {\
    case SQLITE_NULL:\
      sqlite3_result_null(context);\
      break;\
    default:\
      rVal = sqlite3_value_double(argv[0]);\
      if (domain)\
        sqlite3_result_error(context, "domain error", -1);\
      else\
        sqlite3_result_double(context, function(rVal));\
      break;\
  }\
}

/**
 * Example of GEN_MATH_WRAP_DOUBLE_1 usage
 * this creates function sqrtFunc to wrap the math.h standard function sqrt(x)=x^0.5
 * notice the domain rVal<0 is the condition that signals a domain error HAS occurred
 */
GEN_MATH_WRAP_DOUBLE_1(sqrtFunc, sqrt, rVal < 0)

/* trignometric functions */
GEN_MATH_WRAP_DOUBLE_1(acosFunc, acos, rVal < -1.0 || rVal > 1.0)
GEN_MATH_WRAP_DOUBLE_1(asinFunc, asin, rVal < -1.0 || rVal > 1.0)
GEN_MATH_WRAP_DOUBLE_1(atanFunc, atan, 0)

/**
 * Many of systems don't have inverse hyperbolic trig functions so this will emulate
 * them on those systems in terms of log and sqrt (formulas are too trivial to demand 
 * written proof here)
 */
#ifdef REFERENCE
static double acosh(double x)
{
    return log(x + sqrt(x * x - 1.0));
}
#endif

GEN_MATH_WRAP_DOUBLE_1(acoshFunc, acosh, rVal < 1)
#ifdef REFERENCE
static double asinh(double x)
{
    return log(x + sqrt(x * x + 1.0));
}
#endif

GEN_MATH_WRAP_DOUBLE_1(asinhFunc, asinh, 0)
#ifdef REFERENCE
static double atanh(double x)
{
    return (1.0 / 2.0) * log((1 + x) / (1 - x));
}
#endif

GEN_MATH_WRAP_DOUBLE_1(atanhFunc, atanh, rVal > 1.0 || rVal < -1.0)

/**
 * math.h doesn't require cot (cotangent) so it's defined here
 */
static double cot(double x)
{
    return 1.0 / tan(x);
}

GEN_MATH_WRAP_DOUBLE_1(sinFunc, sin, 0)
GEN_MATH_WRAP_DOUBLE_1(cosFunc, cos, 0)
GEN_MATH_WRAP_DOUBLE_1(tanFunc, tan, 0)		/* XXX DOMAIN */
GEN_MATH_WRAP_DOUBLE_1(cotFunc, cot, 0)		/* XXX DOMAIN */

static double coth(double x)
{
    return 1.0 / tanh(x);
}

/**
 * Many systems don't have hyperbolic trigonometric functions so this will emulate
 * them on those systems directly from the definition in terms of exp
 */
#ifdef REFERENCE
static double sinh(double x)
{
    return (exp(x) - exp(-x)) / 2.0;
}
#endif
GEN_MATH_WRAP_DOUBLE_1(sinhFunc, sinh, 0)

#ifdef REFERENCE
static double cosh(double x)
{
    return (exp(x) + exp(-x)) / 2.0;
}
#endif
GEN_MATH_WRAP_DOUBLE_1(coshFunc, cosh, 0)

#ifdef REFERENCE
static double tanh(double x)
{
    return sinh(x) / cosh(x);
}
#endif
GEN_MATH_WRAP_DOUBLE_1(tanhFunc, tanh, 0)
GEN_MATH_WRAP_DOUBLE_1(cothFunc, coth, 0)	/* XXX DOMAIN */

/**
 * Some systems lack log in base 10. This will emulate.
 */
#ifdef REFERENCE
static double log10(double x)
{
    static double l10 = -1.0;
    if (l10 < 0.0) {
	l10 = log(10.0);
    }
    return log(x) / l10;
}
#endif
GEN_MATH_WRAP_DOUBLE_1(logFunc, log, rVal <= 0.0)
GEN_MATH_WRAP_DOUBLE_1(log10Func, log10, rVal <= 0.0)
GEN_MATH_WRAP_DOUBLE_1(expFunc, exp, 0)

/**
 * Fallback for systems where math.h doesn't define M_PI
 */
#ifndef M_PI
/*
 * static double PI = acos(-1.0);
 * #define M_PI (PI)
 */
#define M_PI 3.14159265358979323846
#endif

/**
 * Convert Degrees into Radians.
 * @param x	degrees
 * @return	radians
 */
static double deg2rad(double x)
{
    return x * M_PI / 180.0;
}

/**
 * Convert Radians into Degrees.
 * @param x	radians
 * @return	degrees
 */
static double rad2deg(double x)
{
    return 180.0 * x / M_PI;
}
GEN_MATH_WRAP_DOUBLE_1(rad2degFunc, rad2deg, 0)
GEN_MATH_WRAP_DOUBLE_1(deg2radFunc, deg2rad, 0)

/**
 * constant function that returns the value of PI=3.1415...
 * @param context
 * @param argc
 * @param argv
 */
static void piFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    sqlite3_result_double(context, M_PI);
}

/**
 * Implements the sqrt function, it has the peculiarity of returning an integer when the
 * the argument is an integer.
 * Since SQLite isn't strongly typed (almost untyped actually) this is a bit pedantic
 * @param context
 * @param argc
 * @param argv
 */
static void squareFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    double rVal = 0.0;
    int64_t iVal;

assert(argc == 2);
    switch (sqlite3_value_type(argv[0])) {
    case SQLITE_INTEGER:
	iVal = sqlite3_value_int64(argv[0]);
	sqlite3_result_int64(context, iVal * iVal);
	break;
    case SQLITE_NULL:
	sqlite3_result_null(context);
	break;
    default:
	rVal = sqlite3_value_double(argv[0]);
	sqlite3_result_double(context, rVal * rVal);
	break;
    }
}

/**
 * Wraps the pow math.h function.
 * When both the base and the exponent are integers the result should be integer
 * (see sqrt just before this). Here the result is always double
 * @param context
 * @param argc
 * @param argv
 */
static void powerFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    double r1 = 0.0;
    double r2 = 0.0;

assert(argc == 2);

    if (sqlite3_value_type(argv[0]) == SQLITE_NULL
	|| sqlite3_value_type(argv[1]) == SQLITE_NULL) {
	sqlite3_result_null(context);
    } else {
	r1 = sqlite3_value_double(argv[0]);
	r2 = sqlite3_value_double(argv[1]);
	if (r1 <= 0.0) {
	    /* base must be positive */
	    sqlite3_result_error(context, "domain error", -1);
	} else {
	    sqlite3_result_double(context, pow(r1, r2));
	}
    }
}

/**
 * atan2 wrapper.
 * @param context
 * @param argc
 * @param argv
 */
static void atn2Func(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    double r1 = 0.0;
    double r2 = 0.0;

assert(argc == 2);

    if (sqlite3_value_type(argv[0]) == SQLITE_NULL
	|| sqlite3_value_type(argv[1]) == SQLITE_NULL) {
	sqlite3_result_null(context);
    } else {
	r1 = sqlite3_value_double(argv[0]);
	r2 = sqlite3_value_double(argv[1]);
	sqlite3_result_double(context, atan2(r1, r2));
    }
}

/**
 * Implementation of the sign() function.
 * return one of 3 possibilities +1,0 or -1 when the argument is respectively
 * positive, 0 or negative.
 * When the argument is NULL the result is also NULL (completly conventional)
 * @param context
 * @param argc
 * @param argv
 */
static void signFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    double rVal = 0.0;
    int64_t iVal;

assert(argc == 1);
    switch (sqlite3_value_type(argv[0])) {
    case SQLITE_INTEGER:
	iVal = sqlite3_value_int64(argv[0]);
	iVal = (iVal > 0) ? 1 : (iVal < 0) ? -1 : 0;
	sqlite3_result_int64(context, iVal);
	break;
    case SQLITE_NULL:
	sqlite3_result_null(context);
	break;
    default:
	/* 2nd change below. Line for abs was: if( rVal<0 ) rVal = rVal * -1.0;  */

	rVal = sqlite3_value_double(argv[0]);
	rVal = (rVal > 0) ? 1 : (rVal < 0) ? -1 : 0;
	sqlite3_result_double(context, rVal);
	break;
    }
}

/**
 * smallest integer value not less than argument.
 * @param context
 * @param argc
 * @param argv
 */
static void ceilFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    double rVal = 0.0;
    int64_t iVal;

assert(argc == 1);
    switch (sqlite3_value_type(argv[0])) {
    case SQLITE_INTEGER:
	iVal = sqlite3_value_int64(argv[0]);
	sqlite3_result_int64(context, iVal);
	break;
    case SQLITE_NULL:
	sqlite3_result_null(context);
	break;
    default:
	rVal = sqlite3_value_double(argv[0]);
	sqlite3_result_int64(context, ceil(rVal));
	break;
    }
}

/**
 * largest integer value not greater than argument.
 * @param context
 * @param argc
 * @param argv
 */
static void floorFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    double rVal = 0.0;
    int64_t iVal;

assert(argc == 1);
    switch (sqlite3_value_type(argv[0])) {
    case SQLITE_INTEGER:
	iVal = sqlite3_value_int64(argv[0]);
	sqlite3_result_int64(context, iVal);
	break;
    case SQLITE_NULL:
	sqlite3_result_null(context);
	break;
    default:
	rVal = sqlite3_value_double(argv[0]);
	sqlite3_result_int64(context, floor(rVal));
	break;
    }
}

/**
 * Given a string (s) in the first argument and an integer (n) in the second returns the 
 * string that constains s contatenated n times
 * @param context
 * @param argc
 * @param argv
 */
static void replicateFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    unsigned char *z;		/* input string */
    unsigned char *zo;		/* result string */
    int iCount;			/* times to repeat */
    size_t nLen;		/* length of the input string (no multibyte considerations) */
    size_t nTLen;		/* length of the result string (no multibyte considerations) */
    int i = 0;

    if (argc != 2 || SQLITE_NULL == sqlite3_value_type(argv[0]))
	return;

    iCount = sqlite3_value_int64(argv[1]);

    if (iCount < 0) {
	sqlite3_result_error(context, "domain error", -1);
    } else {
	nLen = sqlite3_value_bytes(argv[0]);
	nTLen = nLen * iCount;
	z = xmalloc(nTLen + 1);
	zo = xmalloc(nLen + 1);
	strcpy((char *) zo, (char *) sqlite3_value_text(argv[0]));

	for (i = 0; i < iCount; ++i)
	    strcpy((char *) (z + i * nLen), (char *) zo);

	sqlite3_result_text(context, (char *) z, -1, free);
	zo = _free(zo);
    }
}

static void properFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    const unsigned char *z;	/* input string */
    unsigned char *zo;		/* output string */
    unsigned char *zt;		/* iterator */
    char r;
    int c = 1;

assert(argc == 1);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
	sqlite3_result_null(context);
	return;
    }

    z = sqlite3_value_text(argv[0]);
    zo = (unsigned char *) xstrdup((const char *)z);
    zt = zo;

    while ((r = *(z++)) != 0) {
	if (xisblank(r)) {
	    c = 1;
	} else {
	    r = (c == 1) ? xtoupper(r) : xtolower(r);
	    c = 0;
	}
	*(zt++) = r;
    }
    *zt = '\0';

    sqlite3_result_text(context, (char *) zo, -1, free);
}

#ifdef	NOTYET	/* XXX figger multibyte char's. */
/**
 * given an input string (s) and an integer (n) adds spaces at the beginning of  s
 * until it has a length of n characters.
 * When s has a length >= n it's a NOP
 * padl(NULL) = NULL
 * @param context
 * @param argc
 * @param argv
 */
static void padlFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    size_t ilen;		/* length to pad to */
    size_t zl;			/* length of the input string (UTF-8 chars) */
    size_t i;
    const char *zi;		/* input string */
    char *zo;			/* output string */
    char *zt;

assert(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
	sqlite3_result_null(context);
    } else {
	zi = (const char *) sqlite3_value_text(argv[0]);
	ilen = sqlite3_value_int64(argv[1]);
	/* check domain */
	if (ilen < 0) {
	    sqlite3_result_error(context, "domain error", -1);
	    return;
	}
	zl = sqlite3utf8CharLen(zi, -1);
	if (zl >= ilen) {
	    /* string is longer than the requested pad length, return the same string (dup it) */
	    sqlite3_result_text(context, xstrdup(zi), -1, free);
	} else {
	    zo = xmalloc(strlen(zi) + ilen - zl + 1);
	    zt = zo;
	    for (i = 1; i + zl <= ilen; ++i)
		*(zt++) = ' ';
	    /* no need to take UTF-8 into consideration here */
	    strcpy(zt, zi);
	    sqlite3_result_text(context, zo, -1, free);
	}
    }
}

/**
 * given an input string (s) and an integer (n) appends spaces at the end of  s
 * until it has a length of n characters.
 * When s has a length >= n it's a NOP
 * padl(NULL) = NULL
 * @param context
 * @param argc
 * @param argv
 */
static void padrFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    size_t ilen;		/* length to pad to */
    size_t zl;			/* length of the input string (UTF-8 chars) */
    size_t zll;			/* length of the input string (bytes) */
    size_t i;
    const char *zi;		/* input string */
    char *zo;			/* output string */
    char *zt;

assert(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
	sqlite3_result_null(context);
    } else {
	int64_t _ilen;
	zi = (const char *) sqlite3_value_text(argv[0]);
	_ilen = sqlite3_value_int64(argv[1]);
	/* check domain */
	if (_ilen < 0) {
	    sqlite3_result_error(context, "domain error", -1);
	    return;
	}
	ilen = _ilen;
	zl = sqlite3utf8CharLen(zi, -1);
	if (zl >= ilen) {
	    /* string is longer than the requested pad length, return the same string (dup it) */
	    sqlite3_result_text(context, xstrdup(zi), -1, free);
	} else {
	    zll = strlen(zi);
	    zo = xmalloc(zll + ilen - zl + 1);
	    zt = strcpy(zo, zi) + zll;
	    for (i = 1; i + zl <= ilen; ++i)
		*(zt++) = ' ';
	    *zt = '\0';
	    sqlite3_result_text(context, zo, -1, free);
	}
    }
}

/**
 * given an input string (s) and an integer (n) appends spaces at the end of  s
 * and adds spaces at the beginning of s until it has a length of n characters.
 * Tries to add has many characters at the left as at the right.
 * When s has a length >= n it's a NOP
 * padl(NULL) = NULL
 * @param context
 * @param argc
 * @param argv
 */
static void padcFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    size_t ilen;		/* length to pad to */
    size_t zl;			/* length of the input string (UTF-8 chars) */
    size_t zll;			/* length of the input string (bytes) */
    size_t i;
    const char *zi;		/* input string */
    char *zo;			/* output string */
    char *zt;

assert(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
	sqlite3_result_null(context);
    } else {
	int64_t _ilen;
	zi = (const char *) sqlite3_value_text(argv[0]);
	_ilen = sqlite3_value_int64(argv[1]);
	/* check domain */
	if (_ilen < 0) {
	    sqlite3_result_error(context, "domain error", -1);
	    return;
	}
	ilen = _ilen;
	zl = sqlite3utf8CharLen(zi, -1);
	if (zl >= ilen) {
	    /* string is longer than the requested pad length, return the same string (dup it) */
	    sqlite3_result_text(context, xstrdup(zi), -1, free);
	} else {
	    zll = strlen(zi);
	    zo = xmalloc(zll + ilen - zl + 1);
	    zt = zo;
	    for (i = 1; 2 * i + zl <= ilen; ++i)
		*(zt++) = ' ';
	    strcpy(zt, zi);
	    zt += zll;
	    for (; i + zl <= ilen; ++i)
		*(zt++) = ' ';
	    *zt = '\0';
	    sqlite3_result_text(context, zo, -1, free);
	}
    }
}
#endif

/**
 * given 2 string (s1,s2) returns the string s1 with the characters NOT in s2 removed
 * assumes strings are UTF-8 encoded
 * @param context
 * @param argc
 * @param argv
 */
static void strfilterFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    const char *zi1;		/* first parameter string (searched string) */
    const char *zi2;		/* second parameter string (vcontains valid characters) */
    const char *z1;
    const char *z21;
    const char *z22;
    char *zo;			/* output string */
    char *zot;
    int c1;
    int c2;

assert(argc == 2);

    if (sqlite3_value_type(argv[0]) == SQLITE_NULL
	|| sqlite3_value_type(argv[1]) == SQLITE_NULL) {
	sqlite3_result_null(context);
    } else {
	zi1 = (const char *) sqlite3_value_text(argv[0]);
	zi2 = (const char *) sqlite3_value_text(argv[1]);
	zo = xmalloc(strlen(zi1) + 1);
	zot = zo;
	z1 = zi1;
	while ((c1 = sqliteCharVal(z1)) != 0) {
	    z21 = zi2;
	    while ((c2 = sqliteCharVal(z21)) != 0 && c2 != c1)
		sqliteNextChar(z21);
	    if (c2 != 0) {
		z22 = z21;
		sqliteNextChar(z22);
		strncpy(zot, z21, z22 - z21);
		zot += z22 - z21;
	    }
	    sqliteNextChar(z1);
	}
	*zot = '\0';

	sqlite3_result_text(context, zo, -1, free);
    }
}

/**
 * Given a string z1, retutns the (0 based) index of it's first occurrence
 * in z2 after the first s characters.
 * Returns -1 when there isn't a match.
 * updates p to point to the character where the match occurred.
 * This is an auxiliary function.
 * @param z1
 * @param z2
 * @param s
 * @retval *p
 * @return	index of 1st match (or -1)
 */
static int _substr(const char *z1, const char *z2, int s, const char **p)
{
    int c = 0;
    int rVal = -1;
    const char *zt1;
    const char *zt2;
    int c1;
    int c2;

    if (*z1 == '\0')
	return -1;

    while ((sqliteCharVal(z2) != 0) && (c++) < s)
	sqliteNextChar(z2);

    c = 0;
    while ((sqliteCharVal(z2)) != 0) {
	zt1 = z1;
	zt2 = z2;

	do {
	    c1 = sqliteCharVal(zt1);
	    c2 = sqliteCharVal(zt2);
	    sqliteNextChar(zt1);
	    sqliteNextChar(zt2);
	} while (c1 == c2 && c1 != 0 && c2 != 0);

	if (c1 == 0) {
	    rVal = c;
	    break;
	}

	sqliteNextChar(z2);
	++c;
    }
    if (p)
	*p = z2;
    return rVal >= 0 ? rVal + s : rVal;
}

/**
 * given 2 input strings (s1,s2) and an integer (n) searches from the nth character
 * for the string s1. Returns the position where the match occurred.
 * Characters are counted from 1.
 * 0 is returned when no match occurs.
 * @param context
 * @param argc
 * @param argv
 */

static void charindexFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    const char *z1;	/* s1 string */
    const char *z2;	/* s2 string */
    int s = 0;
    int rVal = 0;

assert(argc == 2 || argc == 3);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])
     || SQLITE_NULL == sqlite3_value_type(argv[1])) {
	sqlite3_result_null(context);
	return;
    }

    z1 = (const char *) sqlite3_value_text(argv[0]);
    z2 = (const char *) sqlite3_value_text(argv[1]);
    if (argc == 3) {
	s = sqlite3_value_int(argv[2]) - 1;
	if (s < 0)
	    s = 0;
    } else {
	s = 0;
    }

    rVal = _substr(z1, z2, s, NULL);
    sqlite3_result_int(context, rVal + 1);
}

/**
 * given a string (s) and an integer (n) returns the n leftmost (UTF-8) characters
 * if the string has a length <= n or is NULL this function is NOP
 * @param context
 * @param argc
 * @param argv
 */
static void leftFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    int c = 0;
    int cc = 0;
    int l = 0;
    const unsigned char *z;	/* input string */
    const unsigned char *zt;
    unsigned char *rz;		/* output string */

assert(argc == 2);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])
     || SQLITE_NULL == sqlite3_value_type(argv[1])) {
	sqlite3_result_null(context);
	return;
    }

    z = sqlite3_value_text(argv[0]);
    l = sqlite3_value_int(argv[1]);
    zt = z;

    while (sqliteCharVal(zt) && c++ < l)
	sqliteNextChar(zt);

    cc = zt - z;

    rz = xmalloc(zt - z + 1);
    strncpy((char *) rz, (char *) z, zt - z);
    *(rz + cc) = '\0';
    sqlite3_result_text(context, (char *) rz, -1, free);
}

/**
 * given a string (s) and an integer (n) returns the n rightmost (UTF-8) characters
 * if the string has a length <= n or is NULL this function is NOP
 * @param context
 * @param argc
 * @param argv
 */
static void rightFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    int l = 0;
    int c = 0;
    int cc = 0;
    const char *z;
    const char *zt;
    const char *ze;
    char *rz;

assert(argc == 2);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])
     || SQLITE_NULL == sqlite3_value_type(argv[1])) {
	sqlite3_result_null(context);
	return;
    }

    z = (const char *) sqlite3_value_text(argv[0]);
    l = sqlite3_value_int(argv[1]);
    zt = z;

    while (sqliteCharVal(zt) != 0) {
	sqliteNextChar(zt);
	++c;
    }

    ze = zt;
    zt = z;

    cc = c - l;
    if (cc < 0)
	cc = 0;

    while (cc-- > 0) {
	sqliteNextChar(zt);
    }

    rz = xmalloc(ze - zt + 1);
    strcpy((char *) rz, (char *) (zt));
    sqlite3_result_text(context, (char *) rz, -1, free);
}

/**
 * removes the whitespaces at the beginning of a string.
 * @param s	input string
 * @return	trimmed string
 */
static const char * ltrim(const char *s)
{
    while (*s == ' ')
	++s;
    return s;
}

/**
 * removes the whitespaces at the end of a string.
 * !mutates the input string!
 * @param s	input string
 * @return	trimmed string
 */
static const char * rtrim(char *s)
{
    char *ss = s + strlen(s) - 1;
    while (ss >= s && *ss == ' ')
	--ss;
    *(ss + 1) = '\0';
    return s;
}

/**
 *  Removes the whitespace at the beginning of a string
 * @param context
 * @param argc
 * @param argv
 */
static void ltrimFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    const char *z;

assert(argc == 1);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
	sqlite3_result_null(context);
	return;
    }
    z = (const char *) sqlite3_value_text(argv[0]);
    sqlite3_result_text(context, xstrdup(ltrim(z)), -1, free);
}

/**
 *  Removes the whitespace at the end of a string
 * @param context
 * @param argc
 * @param argv
 */
static void rtrimFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    const char *z;

assert(argc == 1);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
	sqlite3_result_null(context);
	return;
    }
    z = (const char *) sqlite3_value_text(argv[0]);
    sqlite3_result_text(context, rtrim(xstrdup(z)), -1, free);
}

/**
 *  Removes the whitespace at the beginning and end of a string
 * @param context
 * @param argc
 * @param argv
 */
static void trimFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    const char *z;

assert(argc == 1);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
	sqlite3_result_null(context);
	return;
    }
    z = (const char *) sqlite3_value_text(argv[0]);
    sqlite3_result_text(context, rtrim(xstrdup(ltrim(z))), -1, free);
}

/**
 * given a pointer to a string s1, the length of that string (l1), a new string (s2)
 * and it's length (l2) appends s2 to s1.
 * All lengths in bytes.
 * This is just an auxiliary function
 * @param s1
 * @param l1
 * @param s2
 * @param l2
 */
static void _append(char **s1, int l1, const char *s2, int l2)
{
    *s1 = xrealloc(*s1, (l1 + l2 + 1) * sizeof(char));
    strncpy((*s1) + l1, s2, l2);
    *(*(s1) + l1 + l2) = '\0';
}

/**
 * given strings s, s1 and s2 replaces occurrences of s1 in s by s2
 * @param context
 * @param argc
 * @param argv
 */
static void replaceFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    const char *z1;		/* string s (first parameter) */
    const char *z2;		/* string s1 (second parameter) string to look for */
    const char *z3;		/* string s2 (third parameter) string to replace occurrences of s1 with */
    size_t lz1;
    size_t lz2;
    size_t lz3;
    int lzo = 0;
    char *zo = 0;
    int ret = 0;
    const char *zt1;
    const char *zt2;

assert(argc == 3);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
	sqlite3_result_null(context);
	return;
    }

    z1 = (const char *)sqlite3_value_text(argv[0]);
    z2 = (const char *)sqlite3_value_text(argv[1]);
    z3 = (const char *)sqlite3_value_text(argv[2]);
    /* handle possible null values */
    if (z2 == NULL)
	z2 = "";
    if (z3 == NULL)
	z3 = "";

    lz1 = strlen(z1);
    lz2 = strlen(z2);
    lz3 = strlen(z3);

#if 0
    /* special case when z2 is empty (or null) nothing will be changed */
    if (0 == lz2) {
	sqlite3_result_text(context, xstrdup(z1), -1, free);
	return;
    }
#endif

    zt1 = z1;
    zt2 = z1;

    while (1) {
	ret = _substr(z2, zt1, 0, &zt2);

	if (ret < 0)
	    break;

	_append(&zo, lzo, zt1, zt2 - zt1);
	lzo += zt2 - zt1;
	_append(&zo, lzo, z3, lz3);
	lzo += lz3;

	zt1 = zt2 + lz2;
    }
    _append(&zo, lzo, zt1, lz1 - (zt1 - z1));
    sqlite3_result_text(context, zo, -1, free);
}

/**
 * given a string returns the same string but with the characters in reverse order
 * @param context
 * @param argc
 * @param argv
 */
static void reverseFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    const char *z;
    const char *zt;
    char *rz;
    char *rzt;
    size_t l;
    int i;

assert(argc == 1);
    if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
	sqlite3_result_null(context);
	return;
    }
    z = (const char *)sqlite3_value_text(argv[0]);
    l = strlen(z);
    rz = xmalloc(l + 1);
    rzt = rz + l;
    *(rzt--) = '\0';

    zt = z;
    while (sqliteCharVal(zt) != 0) {
	z = zt;
	sqliteNextChar(zt);
	for (i = 1; zt - i >= z; ++i)
	    *(rzt--) = *(zt - i);
    }

    sqlite3_result_text(context, rz, -1, free);
}

#ifdef	NOTYET		/* XXX needs the sqlite3 map function */
/**
 * An instance of the following structure holds the context of a
 * stdev() or variance() aggregate computation.
 * implementaion of http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Algorithm_II
 * less prone to rounding errors
 */
typedef struct StdevCtx StdevCtx;
struct StdevCtx {
    double rM;
    double rS;
    int64_t cnt;			/* number of elements */
};

/**
 * An instance of the following structure holds the context of a
 * mode() or median() aggregate computation.
 * Depends on structures defined in map.c (see map & map)
 * These aggregate functions only work for integers and floats although
 * they could be made to work for strings. This is usually considered meaningless.
 * Only usual order (for median), no use of collation functions (would this even make sense?)
 */
typedef struct ModeCtx ModeCtx;
struct ModeCtx {
    int64_t riM;		/* integer value found so far */
    double rdM;			/* double value found so far */
    int64_t cnt;		/* number of elements so far */
    double pcnt;		/* number of elements smaller than a percentile */
    int64_t mcnt;		/* maximum number of occurrences (for mode) */
    int64_t mn;			/* number of occurrences (for mode and percentiles) */
    int64_t is_double;		/* whether the computation is being done for doubles (>0) or integers (=0) */
    map *m;			/* map structure used for the computation */
    int done;			/* whether the answer has been found */
};

/**
 * called for each value received during a calculation of stdev or variance.
 * @param context
 * @param argc
 * @param argv
 */
static void varianceStep(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    StdevCtx *p;
    double delta;
    double x;

assert(argc == 1);
    p = sqlite3_aggregate_context(context, sizeof(*p));
    /* only consider non-null values */
    if (SQLITE_NULL != sqlite3_value_numeric_type(argv[0])) {
	p->cnt++;
	x = sqlite3_value_double(argv[0]);
	delta = (x - p->rM);
	p->rM += delta / p->cnt;
	p->rS += delta * (x - p->rM);
    }
}

/**
 * called for each value received during a calculation of mode of median.
 * @param context
 * @param argc
 * @param argv
 */
static void modeStep(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    ModeCtx *p;
    int64_t xi = 0;
    double xd = 0.0;
    int64_t *iptr;
    double *dptr;
    int type;

assert(argc == 1);
    type = sqlite3_value_numeric_type(argv[0]);

    if (type == SQLITE_NULL)
	return;

    p = sqlite3_aggregate_context(context, sizeof(*p));

    if (0 == (p->m)) {
	p->m = calloc(1, sizeof(map));
	if (type == SQLITE_INTEGER) {
	    /* map will be used for integers */
	    *(p->m) = map_make(int_cmp);
	    p->is_double = 0;
	} else {
	    p->is_double = 1;
	    /* map will be used for doubles */
	    *(p->m) = map_make(double_cmp);
	}
    }

    ++(p->cnt);

    if (0 == p->is_double) {
	xi = sqlite3_value_int64(argv[0]);
	iptr = (int64_t *) calloc(1, sizeof(int64_t));
	*iptr = xi;
	map_insert(p->m, iptr);
    } else {
	xd = sqlite3_value_double(argv[0]);
	dptr = (double *) calloc(1, sizeof(double));
	*dptr = xd;
	map_insert(p->m, dptr);
    }
}

/**
 *  Auxiliary function that iterates all elements in a map and finds the mode
 *  (most frequent value).
 *  @param e
 *  @param c
 *  @param pp
 */
static void modeIterate(void *e, int64_t c, void *pp)
{
    int64_t ei;
    double ed;
    ModeCtx *p = (ModeCtx *) pp;

    if (0 == p->is_double) {
	ei = *(int *) (e);

	if (p->mcnt == c) {
	    ++p->mn;
	} else if (p->mcnt < c) {
	    p->riM = ei;
	    p->mcnt = c;
	    p->mn = 1;
	}
    } else {
	ed = *(double *) (e);

	if (p->mcnt == c) {
	    ++p->mn;
	} else if (p->mcnt < c) {
	    p->rdM = ed;
	    p->mcnt = c;
	    p->mn = 1;
	}
    }
}

/**
 *  Auxiliary function that iterates all elements in a map and finds the median
 *  (the value such that the number of elements smaller is equal the the number
 *  of elements larger).
 *  @param e
 *  @param c
 *  @param pp
 */
static void medianIterate(void *e, int64_t c, void *pp)
{
    int64_t ei;
    double ed;
    double iL;
    double iR;
    int il;
    int ir;
    ModeCtx *p = (ModeCtx *) pp;

    if (p->done > 0)
	return;

    iL = p->pcnt;
    iR = p->cnt - p->pcnt;
    il = p->mcnt + c;
    ir = p->cnt - p->mcnt;

    if (il >= iL) {
	if (ir >= iR) {
	    ++p->mn;
	    if (0 == p->is_double) {
		ei = *(int *) (e);
		p->riM += ei;
	    } else {
		ed = *(double *) (e);
		p->rdM += ed;
	    }
	} else {
	    p->done = 1;
	}
    }
    p->mcnt += c;
}

/**
 * Returns the mode value.
 * @param context
 */
static void modeFinalize(sqlite3_context * context)
{
    ModeCtx *p = sqlite3_aggregate_context(context, 0);
    if (p && p->m) {
	map_iterate(p->m, modeIterate, p);
	map_destroy(p->m);
	free(p->m);

	if (1 == p->mn) {
	    if (0 == p->is_double)
		sqlite3_result_int64(context, p->riM);
	    else
		sqlite3_result_double(context, p->rdM);
	}
    }
}

/**
 * auxiliary function for percentiles.
 * @param context
 */
static void _medianFinalize(sqlite3_context * context)
{
    ModeCtx *p = (ModeCtx *) sqlite3_aggregate_context(context, 0);
    if (p && p->m) {
	p->done = 0;
	map_iterate(p->m, medianIterate, p);
	map_destroy(p->m);
	free(p->m);

	if (0 == p->is_double)
	    if (1 == p->mn)
		sqlite3_result_int64(context, p->riM);
	    else
		sqlite3_result_double(context, p->riM * 1.0 / p->mn);
	else
	    sqlite3_result_double(context, p->rdM / p->mn);
    }
}

/**
 * Returns the median value.
 * @param context
 */
static void medianFinalize(sqlite3_context * context)
{
    ModeCtx *p = (ModeCtx *) sqlite3_aggregate_context(context, 0);
    if (p != NULL) {
	p->pcnt = (p->cnt) / 2.0;
	_medianFinalize(context);
    }
}

/**
 * Returns the lower_quartile value.
 * @param context
 */
static void lower_quartileFinalize(sqlite3_context * context)
{
    ModeCtx *p = (ModeCtx *) sqlite3_aggregate_context(context, 0);
    if (p != NULL) {
	p->pcnt = (p->cnt) / 4.0;
	_medianFinalize(context);
    }
}

/**
 * Returns the upper_quartile value.
 * @param context
 */
static void upper_quartileFinalize(sqlite3_context * context)
{
    ModeCtx *p = (ModeCtx *) sqlite3_aggregate_context(context, 0);
    if (p != NULL) {
	p->pcnt = (p->cnt) * 3 / 4.0;
	_medianFinalize(context);
    }
}

/**
 * Returns the stdev value.
 * @param context
 */
static void stdevFinalize(sqlite3_context * context)
{
    StdevCtx *p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1)
	sqlite3_result_double(context, sqrt(p->rS / (p->cnt - 1)));
    else
	sqlite3_result_double(context, 0.0);
}

/**
 * Returns the variance value.
 * @param context
 */
static void varianceFinalize(sqlite3_context * context)
{
    StdevCtx *p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1)
	sqlite3_result_double(context, p->rS / (p->cnt - 1));
    else
	sqlite3_result_double(context, 0.0);
}
#endif

/**
 * Return a rpm macro expansion result.
 * @param context
 * @param argc
 * @param argv
 */
static void expandFunc(sqlite3_context * context,
		int argc, sqlite3_value ** argv)
{
    sqlite3_result_text(context,
    	rpmExpand((const char *)sqlite3_value_text(argv[0]), NULL), -1, free);
}

/**
 * REGEXP for sqlite3 using miRE patterns.
 * @param context
 * @param argc
 * @param argv
 */
static void regexpFunc(sqlite3_context* context,
		int argc, sqlite3_value** argv)
{
    const char * value = (const char *) sqlite3_value_text(argv[0]);
    const char * pattern = (const char *) sqlite3_value_text(argv[1]);
    miRE mire = mireNew(RPMMIRE_REGEX, 0);
    int rc = mireRegcomp(mire, pattern);

    rc = mireRegexec(mire, value, strlen(value));
    switch (rc) {
    case 0:
    case 1:
	sqlite3_result_int(context, rc);
	break;
    default:
	sqlite3_result_error(context, "invalid pattern", -1);
	break;
    }
    mire = mireFree(mire);
}

static struct rpmsqlCF_s __CF[] = {
    /* math.h extensions */
  { "acos",		1, 0, SQLITE_UTF8,	0, acosFunc, NULL, NULL },
  { "asin",		1, 0, SQLITE_UTF8,	0, asinFunc, NULL, NULL },
  { "atan",		1, 0, SQLITE_UTF8,	0, atanFunc, NULL, NULL },
  { "atn2",		2, 0, SQLITE_UTF8,	0, atn2Func, NULL, NULL },
    /* XXX alias */
  { "atan2",		2, 0, SQLITE_UTF8,	0, atn2Func, NULL, NULL },
  { "acosh",		1, 0, SQLITE_UTF8,	0, acoshFunc, NULL, NULL },
  { "asinh",		1, 0, SQLITE_UTF8,	0, asinhFunc, NULL, NULL },
  { "atanh",		1, 0, SQLITE_UTF8,	0, atanhFunc, NULL, NULL },

#ifdef	NOTYET
  { "difference",	2, 0, SQLITE_UTF8,	0, differenceFunc, NULL, NULL },
#endif
  { "degrees",		1, 0, SQLITE_UTF8,	0, rad2degFunc, NULL, NULL },
  { "radians",		1, 0, SQLITE_UTF8,	0, deg2radFunc, NULL, NULL },

  { "cos",		1, 0, SQLITE_UTF8,	0, cosFunc, NULL, NULL },
  { "sin",		1, 0, SQLITE_UTF8,	0, sinFunc, NULL, NULL },
  { "tan",		1, 0, SQLITE_UTF8,	0, tanFunc, NULL, NULL },
  { "cot",		1, 0, SQLITE_UTF8,	0, cotFunc, NULL, NULL },
  { "cosh",		1, 0, SQLITE_UTF8,	0, coshFunc, NULL, NULL },
  { "sinh",		1, 0, SQLITE_UTF8,	0, sinhFunc, NULL, NULL },
  { "tanh",		1, 0, SQLITE_UTF8,	0, tanhFunc, NULL, NULL },
  { "coth",		1, 0, SQLITE_UTF8,	0, cothFunc, NULL, NULL },

  { "exp",		1, 0, SQLITE_UTF8,	0, expFunc, NULL, NULL },
  { "log",		1, 0, SQLITE_UTF8,	0, logFunc, NULL, NULL },
  { "log10",		1, 0, SQLITE_UTF8,	0, log10Func, NULL, NULL },
  { "power",		2, 0, SQLITE_UTF8,	0, powerFunc, NULL, NULL },
  { "sign",		1, 0, SQLITE_UTF8,	0, signFunc, NULL, NULL },
  { "sqrt",		1, 0, SQLITE_UTF8,	0, sqrtFunc, NULL, NULL },
  { "square",		1, 0, SQLITE_UTF8,	0, squareFunc, NULL, NULL },

  { "ceil",		1, 0, SQLITE_UTF8,	0, ceilFunc, NULL, NULL },
  { "floor",		1, 0, SQLITE_UTF8,	0, floorFunc, NULL, NULL },

  { "pi",		0, 0, SQLITE_UTF8,	1, piFunc, NULL, NULL },

    /* string extensions */
  { "replicate",	2, 0, SQLITE_UTF8,	0, replicateFunc, NULL, NULL },
  { "charindex",	2, 0, SQLITE_UTF8,	0, charindexFunc, NULL, NULL },
  { "charindex",	3, 0, SQLITE_UTF8,	0, charindexFunc, NULL, NULL },
  { "leftstr",		2, 0, SQLITE_UTF8,	0, leftFunc, NULL, NULL },
  { "rightstr",		2, 0, SQLITE_UTF8,	0, rightFunc, NULL, NULL },
  { "ltrim",		1, 0, SQLITE_UTF8,	0, ltrimFunc, NULL, NULL },
  { "rtrim",		1, 0, SQLITE_UTF8,	0, rtrimFunc, NULL, NULL },
  { "trim",		1, 0, SQLITE_UTF8,	0, trimFunc, NULL, NULL },
  { "replace",		3, 0, SQLITE_UTF8,	0, replaceFunc, NULL, NULL },
  { "reverse",		1, 0, SQLITE_UTF8,	0, reverseFunc, NULL, NULL },
  { "proper",		1, 0, SQLITE_UTF8,	0, properFunc, NULL, NULL },
#ifdef	NOTYET	/* XXX figger multibyte char's. */
  { "padl",		2, 0, SQLITE_UTF8,	0, padlFunc, NULL, NULL },
  { "padr",		2, 0, SQLITE_UTF8,	0, padrFunc, NULL, NULL },
  { "padc",		2, 0, SQLITE_UTF8,	0, padcFunc, NULL, NULL },
#endif
  { "strfilter",	2, 0, SQLITE_UTF8,	0, strfilterFunc, NULL, NULL },

    /* statistical aggregate extensions */
#ifdef	NOTYET		/* XXX needs the sqlite3 map function */
  { "stdev",		1, 0, SQLITE_UTF8,	0, NULL, varianceStep, stdevFinalize  },
  { "variance",		1, 0, SQLITE_UTF8,	0, NULL, varianceStep, varianceFinalize  },
  { "mode",		1, 0, SQLITE_UTF8,	0, NULL, modeStep, modeFinalize },
  { "median",		1, 0, SQLITE_UTF8,	0, NULL, modeStep, medianFinalize },
  { "lower_quartile",	1, 0, SQLITE_UTF8,	0, NULL, modeStep, lower_quartileFinalize },
  { "upper_quartile",	1, 0, SQLITE_UTF8,	0, NULL, modeStep, upper_quartileFinalize },
#endif

    /* RPM extensions. */
  { "expand",		1, 0, SQLITE_UTF8,	0, expandFunc, NULL, NULL },
  { "regexp",		2, 0, SQLITE_UTF8,	0, regexpFunc, NULL, NULL },
  { NULL,		0, 0, 0,		0, NULL, NULL, NULL }
};

rpmsqlCF _rpmsqlCFT = __CF;

int _rpmsqlLoadCFT(rpmsql sql, void * _CF)
{
    sqlite3 * db = (sqlite3 *)sql->I;
    rpmsqlCF CF;
    int rc = 0;

SQLDBG((stderr, "--> %s(%p,%p)\n", __FUNCTION__, sql, _CF));
    if (_CF == NULL)
	_CF = _rpmsqlCFT;
    if (_CF)
    for (CF = (rpmsqlCF) _CF; CF->zName != NULL; CF++) {
	void * _pApp = NULL;
	int xx;

	switch (CF->argType) {
	default:
	case 0:	 _pApp = NULL;		break;
	case 1:	 _pApp = (void *)db;	break;
	case 2:	 _pApp = (void *)-1;	break;
	}

	xx = rpmsqlCmd(sql, "create_function", db,
		sqlite3_create_function(db, CF->zName, CF->nArg, CF->eTextRep,
			_pApp,	CF->xFunc, CF->xStep, CF->xFinal));
SQLDBG((stderr, "\t%s(%s) xx %d\n", "sqlite3_create_function", CF->zName, xx));
	if (xx && rc == 0)
	    rc = xx;

#ifdef	NOTYET
	if (CF->needColSeq) {
	    FuncDef *pFunc = sqlite3FindFunction(db, CF->zName,
				strlen(CF_>zName), CF->nArg, CF->eTextRep, 0);
	    if (pFunc) pFunc->needCollSeq = 1;
	}
#endif

    }
SQLDBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, sql, rc));
    return rc;
}

/*==============================================================*/

static struct rpmvd_s _envVD = {
	.split	= "=",
	.parse	= "key=val",
	.regex	= "^([^=]+)=(.*)$",
	.idx	= 1,
};

static int envCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_envVD), vtp);
}

struct sqlite3_module envModule = {
    .xCreate	= (void *) envCreateConnect,
    .xConnect	= (void *) envCreateConnect,
};

/*==============================================================*/

static struct rpmvd_s _grdbVD = {
	.prefix	= "%{?_etc_group}%{!?_etc_group:/etc/group}",
	.split	= ":",
	/* XXX "group" is a reserved keyword. */
	.parse	= "_group:passwd:gid:groups",
	.regex	= "^([^:]*):([^:]*):([^:]*):([^:]*)$",
	.idx	= 3,
};

static int grdbCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_grdbVD), vtp);
}

struct sqlite3_module grdbModule = {
    .xCreate	= (void *) grdbCreateConnect,
    .xConnect	= (void *) grdbCreateConnect,
};

/*==============================================================*/

static struct rpmvd_s _procdbVD = {
	.prefix = "%{?_procdb}%{!?_procdb:/proc/[0-9]}",
	.split	= "/-",
	.parse	= "dir/pid/*",
	.regex	= "^(.+/)([0-9]+)$",
	.idx	= 2,
};

static int procdbCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_procdbVD), vtp);
}

struct sqlite3_module procdbModule = {
    .xCreate	= (void *) procdbCreateConnect,
    .xConnect	= (void *) procdbCreateConnect,
};

/*==============================================================*/

static struct rpmvd_s _pwdbVD = {
	.prefix	= "%{?_etc_passwd}%{!?_etc_passwd:/etc/passwd}",
	.split	= ":",
	.parse	= "user:passwd:uid:gid:gecos:dir:shell",
	.regex	= "^([^:]*):([^:]*):([^:]*):([^:]*):([^:]*):([^:]*):([^:]*)$",
	.idx	= 3,
};

static int pwdbCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_pwdbVD), vtp);
}

struct sqlite3_module pwdbModule = {
    .xCreate	= (void *) pwdbCreateConnect,
    .xConnect	= (void *) pwdbCreateConnect,
};

/*==============================================================*/

static struct rpmvd_s _repodbVD = {
	/* XXX where to map the default? */
	.prefix = "%{?_repodb}%{!?_repodb:/X/popt/}",
	.split	= "/-.",
	.parse	= "dir/file-NVRA-N-V-R.A",
	.regex	= "^(.+/)(((.*)-([^-]+)-([^-]+)\\.([^.]+))\\.rpm)$",
	.idx	= 2,
};

static int repodbCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_repodbVD), vtp);
}

struct sqlite3_module repodbModule = {
    .xCreate	= (void *) repodbCreateConnect,
    .xConnect	= (void *) repodbCreateConnect,
};

/*==============================================================*/

static int _stat_debug = 0;

static struct rpmvd_s _statVD = {
	.split	= " ,",
	.parse	= "st_dev,st_ino,st_mode,st_nlink,st_uid,st_gid,st_rdev,st_size,st_blksize,st_blocks,st_atime,st_mtime,st_ctime",
};

static int statCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_statVD), vtp);
}

static int statColumn(rpmvc vc, void * _pContext, int colx)
{
    sqlite3_context * pContext = (sqlite3_context *) _pContext;
    rpmvt vt = vc->vt;
    const char * path = vt->av[vc->ix];
    const char * col = vt->cols[colx];
    struct stat sb, *st = &sb;	/* XXX move to rpmvcNext for performance */
    int ret = Lstat(path, &sb);
    int rc = SQLITE_OK;

if (_stat_debug < 0)
fprintf(stderr, "--> %s(%p,%p,%d)\n", __FUNCTION__, vc, pContext, colx);


    if (!strcmp(col, "path"))
	sqlite3_result_text(pContext, path, -1, SQLITE_STATIC);
    else if (!strcmp(col, "st_dev") && !ret)
	sqlite3_result_int64(pContext, st->st_dev);
    else if (!strcmp(col, "st_ino") && !ret)
	sqlite3_result_int64(pContext, st->st_ino);
    else if (!strcmp(col, "st_mode") && !ret)
	sqlite3_result_int64(pContext, st->st_mode);
    else if (!strcmp(col, "st_nlink") && !ret)
	sqlite3_result_int64(pContext, st->st_nlink);
    else if (!strcmp(col, "st_uid") && !ret)
	sqlite3_result_int64(pContext, st->st_uid);
    else if (!strcmp(col, "st_gid") && !ret)
	sqlite3_result_int64(pContext, st->st_gid);
    else if (!strcmp(col, "st_rdev") && !ret)
	sqlite3_result_int64(pContext, st->st_rdev);
    else if (!strcmp(col, "st_size") && !ret)
	sqlite3_result_int64(pContext, st->st_size);
    else if (!strcmp(col, "st_blksize") && !ret)
	sqlite3_result_int64(pContext, st->st_blksize);
    else if (!strcmp(col, "st_blocks") && !ret)
	sqlite3_result_int64(pContext, st->st_blocks);
    else if (!strcmp(col, "st_atime") && !ret)
	sqlite3_result_int64(pContext, st->st_atime);
    else if (!strcmp(col, "st_mtime") && !ret)
	sqlite3_result_int64(pContext, st->st_mtime);
    else if (!strcmp(col, "st_ctime") && !ret)
	sqlite3_result_int64(pContext, st->st_ctime);
	/* XXX pick up *BSD derangements */
    else
	sqlite3_result_null(pContext);

if (_stat_debug < 0)
fprintf(stderr, "<-- %s(%p,%p,%d) rc %d\n", __FUNCTION__, vc, pContext, colx, rc);

    return rc;
}

struct sqlite3_module statModule = {
    .xCreate	= (void *) statCreateConnect,
    .xConnect	= (void *) statCreateConnect,
    .xColumn	= (void *) statColumn,
};

/*==============================================================*/

static struct rpmvd_s _yumdbVD = {
	.prefix = "%{?_yumdb}%{!?_yumdb:/var/lib/yum/yumdb}/",
	.split	= "/-",
	.parse	= "dir/hash-NVRA-N-V-R-A/*",
	.regex	= "^(.+/)([^-]+)-((.*)-([^-]+)-([^-]+)-([^-]+))$",
	.idx	= 2,
};

static int yumdbCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_yumdbVD), vtp);
}

struct sqlite3_module yumdbModule = {
    .xCreate	= (void *) yumdbCreateConnect,
    .xConnect	= (void *) yumdbCreateConnect,
};

/*==============================================================*/

struct sqlite3_module _rpmvmTemplate = {
    .xCreate	= (void *) rpmvtCreate,
    .xConnect	= (void *) rpmvtConnect,
    .xBestIndex	= (void *) rpmvtBestIndex,
    .xDisconnect = (void *) rpmvtDisconnect,
    .xDestroy	= (void *) rpmvtDestroy,
    .xOpen	= (void *) rpmvcOpen,
    .xClose	= (void *) rpmvcClose,
    .xFilter	= (void *) rpmvcFilter,
    .xNext	= (void *) rpmvcNext,
    .xEof	= (void *) rpmvcEof,
    .xColumn	= (void *) rpmvcColumn,
    .xRowid	= (void *) rpmvcRowid,
    .xUpdate	= (void *) rpmvtUpdate,
    .xBegin	= (void *) rpmvtBegin,
    .xSync	= (void *) rpmvtSync,
    .xCommit	= (void *) rpmvtCommit,
    .xRollback	= (void *) rpmvtRollback,
    .xFindFunction = (void *) rpmvtFindFunction,
    .xRename	= (void *) rpmvtRename
};

static struct rpmsqlVMT_s __VMT[] = {
  { "Argv",		NULL, NULL },
  { "Env",		&envModule, NULL },
  { "Grdb",		&grdbModule, NULL },
  { "Procdb",		&procdbModule, NULL },
  { "Pwdb",		&pwdbModule, NULL },
  { "Repodb",		&repodbModule, NULL },
  { "Stat",		&statModule, NULL },
  { "Yumdb",		&yumdbModule, NULL },
  { NULL,		NULL, NULL }
};

static void rpmsqlVMFree(void * _VM)
	/*@*/
{
SQLDBG((stderr, "--> %s(%p)\n", __FUNCTION__, _VM));
    if (_VM)
	free(_VM);
}

#ifdef	UNUSED
static void dumpVM(const char * msg, const rpmsqlVM s)
{
fprintf(stderr, "--------------------- %s\n", (msg ? msg : ""));
#define	VMPRT(f) if (s->f) fprintf(stderr, "%20s: %p\n", #f, s->f)
	VMPRT(xCreate);
	VMPRT(xConnect);
	VMPRT(xBestIndex);
	VMPRT(xDisconnect);
	VMPRT(xDestroy);
	VMPRT(xOpen);
	VMPRT(xClose);
	VMPRT(xFilter);
	VMPRT(xNext);
	VMPRT(xEof);
	VMPRT(xColumn);
	VMPRT(xRowid);
	VMPRT(xUpdate);
	VMPRT(xBegin);
	VMPRT(xSync);
	VMPRT(xCommit);
	VMPRT(xRollback);
	VMPRT(xFindFunction);
	VMPRT(xRename);
#undef	VMPRT
}
#endif

static /*@only@*/ rpmsqlVM rpmsqlVMNew(/*@null@*/ const rpmsqlVM s)
{
    rpmsqlVM t = xcalloc(1, sizeof(*t));

SQLDBG((stderr, "--> %s(%p)\n", __FUNCTION__, s));
    *t = _rpmvmTemplate;	/* structure assignment */

    if (s) {
	if (s->iVersion) t->iVersion = s->iVersion;
#define	VMCPY(f) if (s->f) t->f = ((s->f != (void *)-1) ? s->f : NULL)
	VMCPY(xCreate);
	VMCPY(xConnect);
	VMCPY(xBestIndex);
	VMCPY(xDisconnect);
	VMCPY(xDestroy);
	VMCPY(xOpen);
	VMCPY(xClose);
	VMCPY(xFilter);
	VMCPY(xNext);
	VMCPY(xEof);
	VMCPY(xColumn);
	VMCPY(xRowid);
	VMCPY(xUpdate);
	VMCPY(xBegin);
	VMCPY(xSync);
	VMCPY(xCommit);
	VMCPY(xRollback);
	VMCPY(xFindFunction);
	VMCPY(xRename);
#undef	VMCPY
    }
SQLDBG((stderr, "<-- %s(%p) %p\n", __FUNCTION__, s, t));
    return t;
}

int _rpmsqlLoadVMT(void * _db, const rpmsqlVMT _VMT)
{
    sqlite3 * db = (sqlite3 *) _db;
    rpmsqlVMT VMT;
    int rc = 0;

SQLDBG((stderr, "--> %s(%p,%p)\n", __FUNCTION__, _db, _VMT));
    for (VMT = (rpmsqlVMT)_VMT; VMT->zName != NULL; VMT++) {
	int xx;

	xx = rpmsqlCmd(_rpmsqlI, "create_module_v2", db,
                sqlite3_create_module_v2(db, VMT->zName,
			rpmsqlVMNew(VMT->module), VMT->data, rpmsqlVMFree));
SQLDBG((stderr, "\t%s(%s) xx %d\n", "sqlite3_create_module_v2", VMT->zName, xx));
	if (xx && rc == 0)
	    rc = xx;

    }
SQLDBG((stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, _db, _VMT, rc));
    return rc;
}

/*==============================================================*/
/* XXX HACK: AWOL in -lsqlite3 on CM14 */
#if SQLITE_VERSION_NUMBER <= 3006015
#define	sqlite3_enable_load_extension(db, onoff)		SQLITE_OK
#define sqlite3_load_extension(db, zFile, zProc, pzErrMsg)	SQLITE_OK
#endif

/**
 * Make sure the database is open.  If it is not, then open it.  If
 * the database fails to open, print an error message and exit.
 * @param sql		sql interpreter
 */
static int _rpmsqlOpenDB(rpmsql sql)
{
    int rc = -1;	/* assume failure */
    sqlite3 * db;

assert(sql);

    db = (sqlite3 *)sql->I;
    if (db == NULL) {
	int rc;
	rc = rpmsqlCmd(sql, "open", db,		/* XXX watchout: arg order */
		sqlite3_open(sql->zDbFilename, &db));
	sql->I = db;

	if (db && rc == SQLITE_OK) {
	    (void) _rpmsqlLoadCFT(sql, _rpmsqlCFT);
	    (void) _rpmsqlLoadVMT(db, __VMT);
	}

	if (db == NULL || sqlite3_errcode(db) != SQLITE_OK) {
	    /* XXX rpmlog */
	    rpmsql_error(1, _("unable to open database \"%s\": %s"),
		    sql->zDbFilename, sqlite3_errmsg(db));
	    goto exit;
	}
	/* Enable extension loading (if not disabled). */
	if (!F_ISSET(sql, NOLOAD))
	    (void) rpmsqlCmd(sql, "enable_load_extension", db,
			sqlite3_enable_load_extension(db, 1));
    }
    rc = 0;

exit:
SQLDBG((stderr, "<-- %s(%p) rc %d %s\n", __FUNCTION__, sql, rc, sql->zDbFilename));
    return rc;
}

#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

#if defined(WITH_SQLITE)
/**
 * Determines if a string is a number of not.
 * @param z
 * @retval *realnum
 */
static int isNumber(const char *z, int *realnum)
{
    if (*z == '-' || *z == '+')
	z++;
    if (!isdigit(*z))
	return 0;
    z++;
    if (realnum)
	*realnum = 0;
    while (isdigit(*z))
	z++;
    if (*z == '.') {
	z++;
	if (!isdigit(*z))
	    return 0;
	while (isdigit(*z))
	    z++;
	if (realnum)
	    *realnum = 1;
    }
    if (*z == 'e' || *z == 'E') {
	z++;
	if (*z == '+' || *z == '-')
	    z++;
	if (!isdigit(*z))
	    return 0;
	while (isdigit(*z))
	    z++;
	if (realnum)
	    *realnum = 1;
    }
    return *z == 0;
}

/**
 * Compute a string length that is limited to what can be stored in
 * lower 30 bits of a 32-bit signed integer.
 * @param z
 */
static int strlen30(const char *z)
{
    const char *z2 = z;
    while (*z2)
	z2++;
    return 0x3fffffff & (int) (z2 - z);
}
#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/
#if defined(WITH_SQLITE)
/**
 * Output the given string as a hex-encoded blob (eg. X'1234' )
 * @param sql		sql interpreter
 * @param pBlob		octets
 * @param nBlob		no. of octets
 */
static void output_hex_blob(rpmsql sql, const void *pBlob, int nBlob)
{
    char *zBlob = (char *) pBlob;
    int i;

SQLDBG((stderr, "--> %s(%p,%p[%u])\n", __FUNCTION__, sql, pBlob, (unsigned)nBlob));
    rpmsqlFprintf(sql, "X'");
    for (i = 0; i < nBlob; i++)
	rpmsqlFprintf(sql, "%02x", zBlob[i]);
    rpmsqlFprintf(sql, "'");
}

/**
 * Output the given string as a quoted string using SQL quoting conventions.
 * @param sql		sql interpreter
 * @param z		string
 */
static void output_quoted_string(rpmsql sql, const char *z)
{
    int i;
    int nSingle = 0;
SQLDBG((stderr, "--> %s(%p,%s)\n", __FUNCTION__, sql, z));
    for (i = 0; z[i]; i++) {
	if (z[i] == '\'')
	    nSingle++;
    }
    if (nSingle == 0) {
	rpmsqlFprintf(sql, "'%s'", z);
    } else {
	rpmsqlFprintf(sql, "'");
	while (*z) {
	    for (i = 0; z[i] && z[i] != '\''; i++)
		;
	    if (i == 0) {
		rpmsqlFprintf(sql, "''");
		z++;
	    } else if (z[i] == '\'') {
		rpmsqlFprintf(sql, "%.*s''", i, z);
		z += i + 1;
	    } else {
		rpmsqlFprintf(sql, "%s", z);
		break;
	    }
	}
	rpmsqlFprintf(sql, "'");
    }
}

/**
 * Output the given string as a quoted according to C or TCL quoting rules.
 * @param sql		sql interpreter
 * @param z		string
 */
static void output_c_string(rpmsql sql, const char *z)
{
    unsigned int c;
SQLDBG((stderr, "--> %s(%p,%s)\n", __FUNCTION__, sql, z));
    rpmsqlFprintf(sql, "\"");
    while ((c = *(z++)) != 0) {
	if (c == '\\')
	    rpmsqlFprintf(sql, "\\\\");
	else if (c == '\t')
	    rpmsqlFprintf(sql, "\\t");
	else if (c == '\n')
	    rpmsqlFprintf(sql, "\\n");
	else if (c == '\r')
	    rpmsqlFprintf(sql, "\\r");
	else if (!isprint(c))
	    rpmsqlFprintf(sql, "\\%03o", c & 0xff);
	else
	    rpmsqlFprintf(sql, "%c", c);
    }
    rpmsqlFprintf(sql, "\"");
}

/**
 * Output the given string with characters that are special to
 * HTML escaped.
 * @param sql		sql interpreter
 * @param z		string
 */
static void output_html_string(rpmsql sql, const char *z)
{
    int i;
SQLDBG((stderr, "--> %s(%p,%s)\n", __FUNCTION__, sql, z));
    while (*z) {
	for (i = 0; z[i]
	     && z[i] != '<'
	     && z[i] != '&'
	     && z[i] != '>' && z[i] != '\"' && z[i] != '\''; i++) {
	}
	if (i > 0)
	    rpmsqlFprintf(sql, "%.*s", i, z);
	if (z[i] == '<')
	    rpmsqlFprintf(sql, "&lt;");
	else if (z[i] == '&')
	    rpmsqlFprintf(sql, "&amp;");
	else if (z[i] == '>')
	    rpmsqlFprintf(sql, "&gt;");
	else if (z[i] == '\"')
	    rpmsqlFprintf(sql, "&quot;");
	else if (z[i] == '\'')
	    rpmsqlFprintf(sql, "&#39;");
	else
	    break;
	z += i + 1;
    }
}

/**
 * If a field contains any character identified by a 1 in the following
 * array, then the string must be quoted for CSV.
 */
/*@unchecked@*/
static const char needCsvQuote[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

/**
 * Output a single term of CSV.  Actually, sql->separator is used for
 * the separator, which may or may not be a comma.  sql->nullvalue is
 * the null value.  Strings are quoted using ANSI-C rules.  Numbers
 * appear outside of quotes.
 * @param sql		sql interpreter
 * @param z		string
 * @param bSep		item separator
 */
static void output_csv(rpmsql sql, const char *z, int bSep)
{
SQLDBG((stderr, "--> %s(%p,%s,0x%x)\n", __FUNCTION__, sql, z, bSep));
    if (z == 0) {
	rpmsqlFprintf(sql, "%s", sql->nullvalue);
    } else {
	int i;
	int nSep = strlen30(sql->separator);
	for (i = 0; z[i]; i++) {
	    if (needCsvQuote[((unsigned char *) z)[i]]
		|| (z[i] == sql->separator[0] &&
		    (nSep == 1 || memcmp(z, sql->separator, nSep) == 0))) {
		i = 0;
		break;
	    }
	}
	if (i == 0) {
	    rpmsqlFprintf(sql, "\"");
	    for (i = 0; z[i]; i++) {
		if (z[i] == '"')
		    rpmsqlFprintf(sql, "\"");
		rpmsqlFprintf(sql, "%c", z[i]);
	    }
	    rpmsqlFprintf(sql, "\"");
	} else {
	    rpmsqlFprintf(sql, "%s", z);
	}
    }
    if (bSep)
	rpmsqlFprintf(sql, "%s", sql->separator);
}

/**
 * This is the callback routine that the shell
 * invokes for each row of a query result.
 * @param _sql		sql interpreter
 * @param nArg
 * @param azArg
 * @param azCol
 * @param aiType
 */
static int _rpmsqlShellCallback(void * _sql, int nArg, char **azArg, char **azCol,
			  int *aiType)
{
    rpmsql sql = (rpmsql)  _sql;
    int w;
    int i;

SQLDBG((stderr, "--> %s(%p,%d,%p,%p,%p)\n", __FUNCTION__,  _sql, nArg, azArg, azCol, aiType));
    switch (sql->mode) {
    case RPMSQL_MODE_LINE:
	w = 5;
	if (azArg == 0)
	    break;
	for (i = 0; i < nArg; i++) {
	    int len = strlen30(azCol[i] ? azCol[i] : "");
	    if (len > w)
		w = len;
	}
	if (sql->cnt++ > 0)
	    rpmsqlFprintf(sql, "\n");
	for (i = 0; i < nArg; i++)
	    rpmsqlFprintf(sql, "%*s = %s\n", w, azCol[i],
			azArg[i] ? azArg[i] : sql->nullvalue);
	break;
    case RPMSQL_MODE_EXPLAIN:
    case RPMSQL_MODE_COLUMN:
	if (sql->cnt++ == 0) {
	    for (i = 0; i < nArg; i++) {
		int n;
		w = (i < ArraySize(sql->colWidth) ? sql->colWidth[i] : 0);

		if (w <= 0) {
		    w = strlen30(azCol[i] ? azCol[i] : "");
		    if (w < 10)
			w = 10;
		    n = strlen30(azArg && azArg[i]
				? azArg[i] : sql-> nullvalue);
		    if (w < n)
			w = n;
		}
		if (i < ArraySize(sql->actualWidth))
		    sql->actualWidth[i] = w;
		if (F_ISSET(sql, SHOWHDR)) {
		    rpmsqlFprintf(sql, "%-*.*s%s", w, w, azCol[i],
				i == nArg - 1 ? "\n" : "  ");
		}
	    }
	    if (F_ISSET(sql, SHOWHDR)) {
		for (i = 0; i < nArg; i++) {
		    w = (i < ArraySize(sql->actualWidth)
				? sql->actualWidth[i] : 10);

		    rpmsqlFprintf(sql, "%-*.*s%s", w, w,
		"-----------------------------------"
		"----------------------------------------------------------",
				i == nArg - 1 ? "\n" : "  ");
		}
	    }
	}
	if (azArg == 0)
	    break;
	for (i = 0; i < nArg; i++) {
	    w = (i < ArraySize(sql->actualWidth) ? sql->actualWidth[i] : 10);
	    if (sql->mode == RPMSQL_MODE_EXPLAIN && azArg[i] &&
		strlen30(azArg[i]) > w) {
		w = strlen30(azArg[i]);
	    }
	    rpmsqlFprintf(sql, "%-*.*s%s", w, w,
			azArg[i] ? azArg[i] : sql->nullvalue,
			i == nArg - 1 ? "\n" : "  ");
	}
	break;
    case RPMSQL_MODE_SEMI:
    case RPMSQL_MODE_LIST:
	if (sql->cnt++ == 0 && F_ISSET(sql, SHOWHDR)) {
	    for (i = 0; i < nArg; i++)
		rpmsqlFprintf(sql, "%s%s", azCol[i],
			    i == nArg - 1 ? "\n" : sql->separator);
	}

	if (azArg == 0)
	    break;
	for (i = 0; i < nArg; i++) {
	    char *z = azArg[i];
	    if (z == 0)
		z = sql->nullvalue;
	    rpmsqlFprintf(sql, "%s", z);
	    if (i < nArg - 1)
		rpmsqlFprintf(sql, "%s", sql->separator);
	    else if (sql->mode == RPMSQL_MODE_SEMI)
		rpmsqlFprintf(sql, ";\n");
	    else
		rpmsqlFprintf(sql, "\n");
	}
	break;
    case RPMSQL_MODE_HTML:
	if (sql->cnt++ == 0 && F_ISSET(sql, SHOWHDR)) {
	    rpmsqlFprintf(sql, "<TR>");
	    for (i = 0; i < nArg; i++) {
		rpmsqlFprintf(sql, "<TH>");
		output_html_string(sql, azCol[i]);
		rpmsqlFprintf(sql, "</TH>\n");
	    }
	    rpmsqlFprintf(sql, "</TR>\n");
	}
	if (azArg == 0)
	    break;
	rpmsqlFprintf(sql, "<TR>");
	for (i = 0; i < nArg; i++) {
	    rpmsqlFprintf(sql, "<TD>");
	    output_html_string(sql, azArg[i] ? azArg[i] : sql->nullvalue);
	    rpmsqlFprintf(sql, "</TD>\n");
	}
	rpmsqlFprintf(sql, "</TR>\n");
	break;
    case RPMSQL_MODE_TCL:
	if (sql->cnt++ == 0 && F_ISSET(sql, SHOWHDR)) {
	    for (i = 0; i < nArg; i++) {
		output_c_string(sql, azCol[i] ? azCol[i] : "");
		rpmsqlFprintf(sql, "%s", sql->separator);
	    }
	    rpmsqlFprintf(sql, "\n");
	}
	if (azArg == 0)
	    break;
	for (i = 0; i < nArg; i++) {
	    output_c_string(sql, azArg[i] ? azArg[i] : sql->nullvalue);
	    rpmsqlFprintf(sql, "%s", sql->separator);
	}
	rpmsqlFprintf(sql, "\n");
	break;
    case RPMSQL_MODE_CSV:
	if (sql->cnt++ == 0 && F_ISSET(sql, SHOWHDR)) {
	    for (i = 0; i < nArg; i++)
		output_csv(sql, azCol[i] ? azCol[i] : "", i < nArg - 1);
	    rpmsqlFprintf(sql, "\n");
	}
	if (azArg == 0)
	    break;
	for (i = 0; i < nArg; i++) 
	    output_csv(sql, azArg[i], i < nArg - 1);
	rpmsqlFprintf(sql, "\n");
	break;
    case RPMSQL_MODE_INSERT:
	sql->cnt++;
	if (azArg == 0)
	    break;
	rpmsqlFprintf(sql, "INSERT INTO %s VALUES(", sql->zDestTable);
	for (i = 0; i < nArg; i++) {
	    char *zSep = i > 0 ? "," : "";
	    if ((azArg[i] == 0) || (aiType && aiType[i] == SQLITE_NULL)) {
		rpmsqlFprintf(sql, "%sNULL", zSep);
	    } else if (aiType && aiType[i] == SQLITE_TEXT) {
		if (zSep[0])
		    rpmsqlFprintf(sql, "%s", zSep);
		output_quoted_string(sql, azArg[i]);
	    } else if (aiType
			   && (aiType[i] == SQLITE_INTEGER
			       || aiType[i] == SQLITE_FLOAT)) {
		rpmsqlFprintf(sql, "%s%s", zSep, azArg[i]);
	    } else if (aiType && aiType[i] == SQLITE_BLOB && sql->S) {
		sqlite3_stmt * pStmt = (sqlite3_stmt *)sql->S;
		const void *pBlob = sqlite3_column_blob(pStmt, i);
		int nBlob = sqlite3_column_bytes(pStmt, i);
		if (zSep[0])
		    rpmsqlFprintf(sql, "%s", zSep);
		output_hex_blob(sql, pBlob, nBlob);
	    } else if (isNumber(azArg[i], 0)) {
		rpmsqlFprintf(sql, "%s%s", zSep, azArg[i]);
	    } else {
		if (zSep[0])
		    rpmsqlFprintf(sql, "%s", zSep);
		output_quoted_string(sql, azArg[i]);
	    }
	}
	rpmsqlFprintf(sql, ");\n");
	break;
    }
    return 0;
}

/**
 * This is the callback routine that the SQLite library
 * invokes for each row of a query result.
 * @param _sql		sql interpreter
 * @param nArg
 * @param azArg
 * @param azCol
 */
static int callback(void *_sql, int nArg, char **azArg, char **azCol)
{
    /* since we don't have type info, call the _rpmsqlShellCallback with a NULL value */
    return _rpmsqlShellCallback(_sql, nArg, azArg, azCol, NULL);
}

/**
 * Set the destination table field of the rpmsql object to
 * the name of the table given.  Escape any quote characters in the
 * table name.
 * @param sql		sql interpreter
 * @param zName
 */
static void set_table_name(rpmsql sql, const char *zName)
{
    int i, n;
    int needQuote;
    char *z;

SQLDBG((stderr, "--> %s(%p,%s)\n", __FUNCTION__, sql, zName));
    sql->zDestTable = _free(sql->zDestTable);
    if (zName == NULL)
	return;
    needQuote = !xisalpha((unsigned char) *zName) && *zName != '_';
    for (i = n = 0; zName[i]; i++, n++) {
	if (!xisalnum((unsigned char) zName[i]) && zName[i] != '_') {
	    needQuote = 1;
	    if (zName[i] == '\'')
		n++;
	}
    }
    if (needQuote)
	n += 2;
    sql->zDestTable = z = xmalloc(n + 1);
    n = 0;
    if (needQuote)
	z[n++] = '\'';
    for (i = 0; zName[i]; i++) {
	z[n++] = zName[i];
	if (zName[i] == '\'')
	    z[n++] = '\'';
    }
    if (needQuote)
	z[n++] = '\'';
    z[n] = 0;
}

/**
 * zIn is either a pointer to a NULL-terminated string in memory obtained
 * from malloc(), or a NULL pointer. The string pointed to by zAppend is
 * added to zIn, and the result returned in memory obtained from malloc().
 * zIn, if it was not NULL, is freed.
 *
 * If the third argument, quote, is not NUL, then it is used as a 
 * quote character for zAppend.
 * @param zIn		output string (or NULL)
 * @param zAppend	input string
 * @param quote		quote char (or NUL)
 * @return
 */
static char *appendText(char *zIn, char const *zAppend, char quote)
{
    int len;
    int i;
    int nAppend = strlen30(zAppend);
    int nIn = (zIn ? strlen30(zIn) : 0);

SQLDBG((stderr, "--> %s(%s,%s,0x%02x)\n", __FUNCTION__, zIn, zAppend, quote));
    len = nAppend + nIn + 1;
    if (quote) {
	len += 2;
	for (i = 0; i < nAppend; i++) {
	    if (zAppend[i] == quote)
		len++;
	}
    }

    zIn = (char *) xrealloc(zIn, len);

    if (quote) {
	char *zCsr = &zIn[nIn];
	*zCsr++ = quote;
	for (i = 0; i < nAppend; i++) {
	    *zCsr++ = zAppend[i];
	    if (zAppend[i] == quote)
		*zCsr++ = quote;
	}
	*zCsr++ = quote;
	*zCsr++ = '\0';
assert((zCsr - zIn) == len);
    } else {
	memcpy(&zIn[nIn], zAppend, nAppend);
	zIn[len - 1] = '\0';
    }

    return zIn;
}


/**
 * Execute a query statement that has a single result column.  Print
 * that result column on a line by itself with a semicolon terminator.
 *
 * This is used, for example, to show the schema of the database by
 * querying the SQLITE_MASTER table.
 * @param sql		sql interpreter
 * @param db
 * @param zSelect
 * @param zFirstRow
 */
static int run_table_dump_query(rpmsql sql, sqlite3 * db,
				const char *zSelect, const char *zFirstRow)
{
    sqlite3_stmt * pSelect;
    int rc;
SQLDBG((stderr, "--> %s(%p,%p,%s,%s)\n", __FUNCTION__, sql, db, zSelect, zFirstRow));
    rc = rpmsqlCmd(sql, "prepare", db,
	sqlite3_prepare(db, zSelect, -1, &pSelect, 0));
    if (rc || pSelect == NULL)
	return rc;

    while ((rc = rpmsqlCmd(sql, "step", db,
		sqlite3_step(pSelect))) == SQLITE_ROW)
    {
	if (zFirstRow) {
	    rpmsqlFprintf(sql, "%s", zFirstRow);
	    zFirstRow = NULL;
	}
	rpmsqlFprintf(sql, "%s;\n", sqlite3_column_text(pSelect, 0));
    }

    return rpmsqlCmd(sql, "finalize", db,
	sqlite3_finalize(pSelect));
}
#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

#if defined(WITH_SQLITE)
#define	iseol(_c)	((char)(_c) == '\n' || (char)(_c) == '\r')

/**
 * fgets(3) analogue that reads \\ continuations. Last newline always trimmed.
 * @param buf		input buffer
 * @param nbuf		inbut buffer size (bytes)
 * @param sql		sql interpreter
 * @return		buffer, or NULL on end-of-file
 */
/*@null@*/
static char *
rpmsqlFgets(/*@returned@*/ char * buf, size_t nbuf, rpmsql sql)
	/*@globals fileSystem @*/
	/*@modifies buf, fileSystem @*/
{
    FD_t ifd = sql->ifd;
/* XXX sadly, fgets(3) cannot be used against a LIBIO wrapped .fpio FD_t */
FILE * ifp = (!F_ISSET(sql, PROMPT) ? fdGetFILE(ifd) : stdin);
    char *q = buf - 1;		/* initialize just before buffer. */
    size_t nb = 0;
    size_t nr = 0;
    int pc = 0, bc = 0;
    char *p = buf;

#ifdef	NOISY	/* XXX obliterates CLI input */
SQLDBG((stderr, "--> %s(%p[%u],%p) ifd %p fp %p fileno %d fdno %d\n", __FUNCTION__, buf, (unsigned)nbuf, sql, ifd, ifp, (ifp ? fileno(ifp) : -3), Fileno(ifd)));
#endif	/* NOISY */
assert(ifp != NULL);

    if (ifp != NULL)
    do {
	*(++q) = '\0';			/* terminate and move forward. */
	if (fgets(q, (int)nbuf, ifp) == NULL)	/* read next line. */
	    break;
	nb = strlen(q);
	nr += nb;			/* trim trailing \r and \n */
	for (q += nb - 1; nb > 0 && iseol(*q); q--)
	    nb--;
	for (; p <= q; p++) {
	    switch (*p) {
		case '\\':
		    switch (*(p+1)) {
			case '\r': /*@switchbreak@*/ break;
			case '\n': /*@switchbreak@*/ break;
			case '\0': /*@switchbreak@*/ break;
			default: p++; /*@switchbreak@*/ break;
		    }
		    /*@switchbreak@*/ break;
		case '%':
		    switch (*(p+1)) {
			case '{': p++, bc++; /*@switchbreak@*/ break;
			case '(': p++, pc++; /*@switchbreak@*/ break;
			case '%': p++; /*@switchbreak@*/ break;
		    }
		    /*@switchbreak@*/ break;
		case '{': if (bc > 0) bc++; /*@switchbreak@*/ break;
		case '}': if (bc > 0) bc--; /*@switchbreak@*/ break;
		case '(': if (pc > 0) pc++; /*@switchbreak@*/ break;
		case ')': if (pc > 0) pc--; /*@switchbreak@*/ break;
	    }
	}
	if (nb == 0 || (*q != '\\' && !bc && !pc) || *(q+1) == '\0') {
	    *(++q) = '\0';		/* trim trailing \r, \n */
	    break;
	}
	q++; p++; nb++;			/* copy newline too */
	nbuf -= nb;
	if (*q == '\r')			/* XXX avoid \r madness */
	    *q = '\n';
    } while (nbuf > 0);

SQLDBG((stderr, "<-- %s(%p[%u],%p) nr %u\n", __FUNCTION__, buf, (unsigned)nbuf, sql, (unsigned)nr));

    return (nr > 0 ? buf : NULL);
}

/**
 * This routine reads a line of text from FILE in, stores
 * the text in memory obtained from malloc() and returns a pointer
 * to the text.  NULL is returned at end of file, or if malloc()
 * fails.
 *
 * The interface is like "readline" but no command-line editing
 * is done.
 * @param sql		sql interpreter
 * @param zPrompt	prompt string
 */
static char *local_getline(rpmsql sql, /*@null@*/const char *zPrompt)
{
    char * t;

SQLDBG((stderr, "--> %s(%s) ofd %p\n", __FUNCTION__, zPrompt, sql->ofd));

    if (sql->ofd && zPrompt && *zPrompt) {
	size_t nb = strlen(zPrompt);
	size_t nw = Fwrite(zPrompt, 1, nb, sql->ofd);
assert(nb == nw);
	(void) Fflush(sql->ofd);
    }

assert(sql->ifd != NULL);
    t = rpmsqlFgets(sql->buf, sql->nbuf, sql);

SQLDBG((stderr, "<-- %s(%s) ofd %p\n", __FUNCTION__, zPrompt, sql->ofd));

    return t;
}

/**
 * Retrieve a single line of input text.
 *
 * zPrior is a string of prior text retrieved.  If not the empty
 * string, then issue a continuation prompt.
 * @param sql		sql interpreter
 * @param zPrior	string
 */
static char *rpmsqlInputOneLine(rpmsql sql, const char *zPrior)
{
    const char *zPrompt;
    char *zResult;

SQLDBG((stderr, "--> %s(%s)\n", __FUNCTION__, zPrior));

assert(sql->buf != NULL);
assert(sql->ifd != NULL);

    if (!F_ISSET(sql, PROMPT)) {
	zResult = local_getline(sql, NULL);
    } else {
	zPrompt = (zPrior && zPrior[0]) ? sql->zContinue : sql->zPrompt;
	zResult = readline(sql, zPrompt);
	if (zResult) {
#if defined(HAVE_READLINE) && HAVE_READLINE==1
	    if (*zResult)
		add_history(zResult);
	    /* XXX readline returns malloc'd memory. copy & free. */
	    if (zResult != sql->buf) {
		strncpy(sql->buf, zResult, sql->nbuf);
		zResult = _free(zResult);
		zResult = sql->buf;
	    }
#endif
	}
    }

SQLDBG((stderr, "<-- %s(%s)\n", __FUNCTION__, zPrior));

    return zResult;
}

#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

#if defined(WITH_SQLITE)
/**
 * Allocate space and save off current error string.
 * @param db
 */
static char *save_err_msg(sqlite3 * db)
{
    const char * s = sqlite3_errmsg(db);
    int nb = strlen30(s) + 1;
    return memcpy(xmalloc(nb), s, nb);
}

/**
 * Execute a statement or set of statements.  Print 
 * any result rows/columns depending on the current mode 
 * set via the supplied callback.
 *
 * This is very similar to SQLite's built-in sqlite3_exec() 
 * function except it takes a slightly different callback 
 * and callback data argument.
 * @param sql		sql interpreter
 * @param zSql
 * @param xCallback
 * @retval *pzErrMsg
 */
static int _rpmsqlShellExec(rpmsql sql, const char *zSql,
		      int (*xCallback) (void *, int, char **, char **, int *),
		      char **pzErrMsg
    )
{
    sqlite3 * db = (sqlite3 *) sql->I;
    sqlite3_stmt * pStmt = NULL;	/* Statement to execute. */
    int rc = SQLITE_OK;		/* Return Code */
    const char *zLeftover;	/* Tail of unprocessed SQL */

SQLDBG((stderr, "--> %s(%p,%s,%p,%p)\n", __FUNCTION__, sql, zSql, xCallback, pzErrMsg));
    if (pzErrMsg)
	*pzErrMsg = NULL;

    while (zSql[0] && rc == SQLITE_OK) {
	rc = rpmsqlCmd(sql, "prepare_v2", db,
		sqlite3_prepare_v2(db, zSql, -1, &pStmt, &zLeftover));
	if (rc)
	    goto bottom;

	/* this happens for a comment or white-space */
	if (pStmt == NULL)
	    goto bottom;

	/* echo the sql statement if echo on */
	if (sql->ofd && F_ISSET(sql, ECHO)) {
	    const char *zStmtSql = sqlite3_sql(pStmt);
	    rpmsqlFprintf(sql, "%s\n", zStmtSql ? zStmtSql : zSql);
	    (void) Fflush(sql->ofd);
	}

	/* perform the first step.  this will tell us if we
	 ** have a result set or not and how wide it is.
	 */
	rc = rpmsqlCmd(sql, "step", db,
		sqlite3_step(pStmt));
	/* if we have a result set... */
	if (rc == SQLITE_ROW) {
	    /* if we have a callback... */
	    if (xCallback) {
		/* allocate space for col name ptr, value ptr, and type */
		int nCol = sqlite3_column_count(pStmt);
		size_t nb = 3 * nCol * sizeof(const char *) + 1;
		char ** azCols = xmalloc(nb);		/* Result names */
		char ** azVals = &azCols[nCol];		/* Result values */
		int * aiTypes = (int *) &azVals[nCol];	/* Result types */
		int i;

		/* save off ptrs to column names */
		for (i = 0; i < nCol; i++)
		    azCols[i] = (char *) sqlite3_column_name(pStmt, i);

		/* save off the prepared statement handle and reset row count */
		sql->S = (void *) pStmt;
		sql->cnt = 0;
		do {
		    /* extract the data and data types */
		    for (i = 0; i < nCol; i++) {
			azVals[i] = (char *) sqlite3_column_text(pStmt, i);
			aiTypes[i] = sqlite3_column_type(pStmt, i);
			if (!azVals[i] && (aiTypes[i] != SQLITE_NULL)) {
			    rc = SQLITE_NOMEM;
			    break;	/* from for */
			}
		    }	/* end for */

		    /* if data and types extraction failed... */
		    if (rc != SQLITE_ROW)
			break;

		    /* call the supplied callback with the result row data */
		    if (xCallback (sql, nCol, azVals, azCols, aiTypes)) {
			rc = SQLITE_ABORT;
			break;
		    }
		    rc = rpmsqlCmd(sql, "step", db,
				sqlite3_step(pStmt));
		} while (rc == SQLITE_ROW);
		azCols = _free(azCols);
		sql->S = NULL;
	    } else {
		do {
		    rc = rpmsqlCmd(sql, "step", db,
				sqlite3_step(pStmt));
		} while (rc == SQLITE_ROW);
	    }
	}

	/* Finalize the statement just executed. If this fails, save a 
	 ** copy of the error message. Otherwise, set zSql to point to the
	 ** next statement to execute. */
	rc = rpmsqlCmd(sql, "finalize", db,
		sqlite3_finalize(pStmt));

bottom:
	/* On error, retrieve message and exit. */
	if (rc) {
	    if (pzErrMsg)
		*pzErrMsg = save_err_msg(db);
	    break;
	}

	/* Move to next sql statement */
	zSql = zLeftover;
	while (xisspace(zSql[0]))
	    zSql++;
    }				/* end while */

    return rc;
}
#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

#if defined(WITH_SQLITE)

/**
 * This is a different callback routine used for dumping the database.
 * Each row received by this callback consists of a table name,
 * the table type ("index" or "table") and SQL to create the table.
 * This routine should print text sufficient to recreate the table.
 * @param _sql		sql interpreter
 * @param nArg
 * @param azArg
 * @param azCol
 */
static int dump_callback(void *_sql, int nArg, char **azArg, char **azCol)
{
    rpmsql sql = (rpmsql) _sql;
    sqlite3 * db = (sqlite3 *) sql->I;
    int rc;
    const char *zTable;
    const char *zType;
    const char *zSql;
    const char *zPrepStmt = 0;
    int ec = 1;		/* assume failure */

SQLDBG((stderr, "--> %s(%p,%d,%p,%p)\n", __FUNCTION__, _sql, nArg, azArg, azCol));
    azCol = azCol;
    if (nArg != 3)
	goto exit;
    zTable = azArg[0];
    zType = azArg[1];
    zSql = azArg[2];

    if (!strcmp(zTable, "sqlite_sequence")) {
	zPrepStmt = "DELETE FROM sqlite_sequence;\n";
    } else if (!strcmp(zTable, "sqlite_stat1")) {
	rpmsqlFprintf(sql, "ANALYZE sqlite_master;\n");
    } else if (!strncmp(zTable, "sqlite_", 7)) {
	ec = 0;		/* XXX success */
	goto exit;
    } else if (!strncmp(zSql, "CREATE VIRTUAL TABLE", 20)) {
	char *zIns;
	if (!F_ISSET(sql, WRITABLE)) {
	    rpmsqlFprintf(sql, "PRAGMA writable_schema=ON;\n");
	    sql->flags |= RPMSQL_FLAGS_WRITABLE;
	}
	zIns =
	    sqlite3_mprintf
	    ("INSERT INTO sqlite_master(type,name,tbl_name,rootpage,sql)"
	     "VALUES('table','%q','%q',0,'%q');", zTable, zTable, zSql);
	rpmsqlFprintf(sql, "%s\n", zIns);
	sqlite3_free(zIns);
	ec = 0;		/* XXX success */
	goto exit;
    } else
	rpmsqlFprintf(sql, "%s;\n", zSql);

    if (!strcmp(zType, "table")) {
	sqlite3_stmt * pTableInfo = NULL;
	char *zSelect = 0;
	char *zTableInfo = 0;
	char *zTmp = 0;
	int nRow = 0;

	zTableInfo = appendText(zTableInfo, "PRAGMA table_info(", 0);
	zTableInfo = appendText(zTableInfo, zTable, '"');
	zTableInfo = appendText(zTableInfo, ");", 0);

	rc = rpmsqlCmd(sql, "prepare", db,
		sqlite3_prepare(db, zTableInfo, -1, &pTableInfo, 0));
	zTableInfo = _free(zTableInfo);
	if (rc != SQLITE_OK || !pTableInfo)
	    goto exit;

	zSelect = appendText(zSelect, "SELECT 'INSERT INTO ' || ", 0);

	zTmp = appendText(zTmp, zTable, '"');
	if (zTmp)
	    zSelect = appendText(zSelect, zTmp, '\'');
	zTmp = _free(zTmp);	/* XXX coverity #1035922 */
	
	zSelect = appendText(zSelect, " || ' VALUES(' || ", 0);
	rc = rpmsqlCmd(sql, "step", db,
		sqlite3_step(pTableInfo));
	while (rc == SQLITE_ROW) {
	    const char *zText =
		(const char *) sqlite3_column_text(pTableInfo, 1);
	    zSelect = appendText(zSelect, "quote(", 0);
	    zSelect = appendText(zSelect, zText, '"');
	    rc = rpmsqlCmd(sql, "step", db,
			sqlite3_step(pTableInfo));
	    if (rc == SQLITE_ROW)
		zSelect = appendText(zSelect, ") || ',' || ", 0);
	    else
		zSelect = appendText(zSelect, ") ", 0);
	    nRow++;
	}
	rc = rpmsqlCmd(sql, "finalize", db,
		sqlite3_finalize(pTableInfo));
	if (rc != SQLITE_OK || nRow == 0) {
	    zSelect = _free(zSelect);
	    goto exit;
	}

	zSelect = appendText(zSelect, "|| ')' FROM  ", 0);
	zSelect = appendText(zSelect, zTable, '"');

	rc = run_table_dump_query(sql, db, zSelect, zPrepStmt);
	if (rc == SQLITE_CORRUPT) {
	    zSelect = appendText(zSelect, " ORDER BY rowid DESC", 0);
	    rc = run_table_dump_query(sql, db, zSelect, NULL);
	}
	zSelect = _free(zSelect);
    }
    ec = 0;	/* XXX success */
exit:
    return ec;
}

/**
 * Run zQuery.  Use dump_callback() as the callback routine so that
 * the contents of the query are output as SQL statements.
 *
 * If we get a SQLITE_CORRUPT error, rerun the query after appending
 * "ORDER BY rowid DESC" to the end.
 * @param sql		sql interpreter
 * @param zQuery
 * @retval *pzErrMsg
 */
static int run_schema_dump_query(rpmsql sql,
				 const char *zQuery, char **pzErrMsg)
{
    sqlite3 * db = (sqlite3 *) sql->I;
    int rc;

SQLDBG((stderr, "--> %s(%p,%s,%p)\n", __FUNCTION__, sql, zQuery, pzErrMsg));
    rc = rpmsqlCmd(sql, "exec", db,
		sqlite3_exec(db, zQuery, dump_callback, sql, pzErrMsg));
    if (rc == SQLITE_CORRUPT) {
	char *zQ2;
	if (pzErrMsg)
	    sqlite3_free(*pzErrMsg);
	zQ2 = rpmExpand(zQuery, " ORDER BY rowid DESC", NULL);
	rc = rpmsqlCmd(sql, "exec", db,
		sqlite3_exec(db, zQ2, dump_callback, sql, pzErrMsg));
	zQ2 = _free(zQ2);
    }
    return rc;
}

/*
 * Text of a help message
 */
/*@unchecked@*/
static char zHelp[] =
    ".backup ?DB? FILE      Backup DB (default \"main\") to FILE\n"
    ".bail ON|OFF           Stop after hitting an error.  Default OFF\n"
    ".databases             List names and files of attached databases\n"
    ".dump ?TABLE? ...      Dump the database in an SQL text format\n"
    "                         If TABLE specified, only dump tables matching\n"
    "                         LIKE pattern TABLE.\n"
    ".echo ON|OFF           Turn command echo on or off\n"
    ".exit                  Exit this program\n"
    ".explain ?ON|OFF?      Turn output mode suitable for EXPLAIN on or off.\n"
    "                         With no args, it turns EXPLAIN on.\n"
    ".header(s) ON|OFF      Turn display of headers on or off\n"
    ".help                  Show this message\n"
    ".import FILE TABLE     Import data from FILE into TABLE\n"
    ".indices ?TABLE?       Show names of all indices\n"
    "                         If TABLE specified, only show indices for tables\n"
    "                         matching LIKE pattern TABLE.\n"
#ifdef SQLITE_ENABLE_IOTRACE
    ".iotrace FILE          Enable I/O diagnostic logging to FILE\n"
#endif
    ".load FILE ?ENTRY?     Load an extension library\n"
    ".log FILE|off          Turn logging on or off.  FILE can be stderr/stdout\n"
    ".mode MODE ?TABLE?     Set output mode where MODE is one of:\n"
    "                         csv      Comma-separated values\n"
    "                         column   Left-aligned columns.  (See .width)\n"
    "                         html     HTML <table> code\n"
    "                         insert   SQL insert statements for TABLE\n"
    "                         line     One value per line\n"
    "                         list     Values delimited by .separator string\n"
    "                         tabs     Tab-separated values\n"
    "                         tcl      TCL list elements\n"
    ".nullvalue STRING      Print STRING in place of NULL values\n"
    ".output FILENAME       Send output to FILENAME\n"
    ".output stdout         Send output to the screen\n"
    ".prompt MAIN CONTINUE  Replace the standard prompts\n"
    ".quit                  Exit this program\n"
    ".read FILENAME         Execute SQL in FILENAME\n"
    ".restore ?DB? FILE     Restore content of DB (default \"main\") from FILE\n"
    ".schema ?TABLE?        Show the CREATE statements\n"
    "                         If TABLE specified, only show tables matching\n"
    "                         LIKE pattern TABLE.\n"
    ".separator STRING      Change separator used by output mode and .import\n"
    ".show                  Show the current values for various settings\n"
    ".tables ?TABLE?        List names of tables\n"
    "                         If TABLE specified, only list tables matching\n"
    "                         LIKE pattern TABLE.\n"
    ".timeout MS            Try opening locked tables for MS milliseconds\n"
    ".width NUM1 NUM2 ...   Set column widths for \"column\" mode\n";

static char zTimerHelp[] =
    ".timer ON|OFF          Turn the CPU timer measurement on or off\n";

/*
 * Do C-language style dequoting.
 *
 *    \t    -> tab
 *    \n    -> newline
 *    \r    -> carriage return
 *    \NNN  -> ascii character NNN in octal
 *    \\    -> backslash
 */
static void resolve_backslashes(char *z)
{
    int i, j;
    char c;
    for (i = j = 0; (c = z[i]) != 0; i++, j++) {
	if (c == '\\') {
	    c = z[++i];
	    if (c == 'n') {
		c = '\n';
	    } else if (c == 't') {
		c = '\t';
	    } else if (c == 'r') {
		c = '\r';
	    } else if (c >= '0' && c <= '7') {
		c -= '0';
		if (z[i + 1] >= '0' && z[i + 1] <= '7') {
		    i++;
		    c = (c << 3) + z[i] - '0';
		    if (z[i + 1] >= '0' && z[i + 1] <= '7') {
			i++;
			c = (c << 3) + z[i] - '0';
		    }
		}
	    }
	}
	z[j] = c;
    }
    z[j] = 0;
}

/**
 * Interpret zArg as a boolean value.  Return either 0 or 1.
 * @param zArg
 */
static int booleanValue(const char * zArg)
{
    int val = atoi(zArg);
    if (!strcasecmp(zArg, "on") || !strcasecmp(zArg, "yes"))
	val = 1;
SQLDBG((stderr, "<-- %s(%s) val %d\n", __FUNCTION__, zArg, val));
    return val;
}

/*@unchecked@*/ /*@observer@*/
static const char *modeDescr[] = {
    "line",
    "column",
    "list",
    "semi",
    "html",
    "insert",
    "tcl",
    "csv",
    "explain",
};

/* forward ref @*/
static int rpmsqlInput(rpmsql sql);

static int rpmsqlFOpen(const char * fn, FD_t *fdp)
	/*@modifies *fdp @*/
{
    FD_t fd = *fdp;
    int rc = 0;

SQLDBG((stderr, "--> %s(%s,%p) fd %p\n", __FUNCTION__, fn, fdp, fd));

    if (fd)
	(void) Fclose(fd);	/* XXX stdout/stderr were dup'd */
    fd = NULL;
    /* XXX permit numeric fdno's? */
    if (fn == NULL)
	fd = NULL;
    else if (!strcmp(fn, "stdout") || !strcmp(fn, "-"))
	fd = fdDup(STDOUT_FILENO);
    else if (!strcmp(fn, "stderr"))
	fd = fdDup(STDERR_FILENO);
    else if (!strcmp(fn, "off"))
	fd = NULL;
    else {
	fd = Fopen(fn, "wb");
	if (fd == NULL || Ferror(fd)) {
	    rpmsql_error(1, _("cannot open \"%s\""), fn);
	    if (fd) (void) Fclose(fd);
	    fd = NULL;
	    rc = 1;
	}
    }
    *fdp = fd;

SQLDBG((stderr, "<-- %s(%s,%p) fd %p rc %d\n", __FUNCTION__, fn, fdp, fd, rc));

    return rc;
}

/**
 * Process .foo SQLITE3 meta command.
 *
 * @param sql		sql interpreter
 * @param zLine
 * @return		0 on success, 1 on error, 2 to exit
 */
static int rpmsqlMetaCommand(rpmsql sql, char *zLine)
{
    sqlite3 * db = (sqlite3 *)sql->I;
    int i = 1;
    int nArg = 0;
    int n, c;
    int rc = 0;
    char *azArg[50];

SQLDBG((stderr, "--> %s(%p,%s)\n", __FUNCTION__, sql, zLine));

    /* Parse the input line into tokens.  */
    while (zLine[i] && nArg < ArraySize(azArg)) {
	while (xisspace((unsigned char) zLine[i]))
	    i++;
	if (zLine[i] == '\0')
	    break;
	if (zLine[i] == '\'' || zLine[i] == '"') {
	    int delim = zLine[i++];
	    azArg[nArg++] = &zLine[i];
	    while (zLine[i] && zLine[i] != delim)
		i++;
	    if (zLine[i] == delim)
		zLine[i++] = '\0';
	    if (delim == '"')
		resolve_backslashes(azArg[nArg - 1]);
	} else {
	    azArg[nArg++] = &zLine[i];
	    while (zLine[i] && !xisspace((unsigned char) zLine[i]))
		i++;
	    if (zLine[i])
		zLine[i++] = 0;
	    resolve_backslashes(azArg[nArg - 1]);
	}
    }

    /* Process the input line. */
    if (nArg == 0)
	return 0;		/* no tokens, no error */
    n = strlen30(azArg[0]);
    c = azArg[0][0];
    if (c == 'b' && n >= 3 && !strncmp(azArg[0], "backup", n)
	&& nArg > 1 && nArg < 4) {
	const char *zDestFile;
	const char *zDb;
	sqlite3 * pDest;
	sqlite3_backup *pBackup;
	if (nArg == 2) {
	    zDestFile = azArg[1];
	    zDb = "main";
	} else {
	    zDestFile = azArg[2];
	    zDb = azArg[1];
	}
	rc = rpmsqlCmd(sql, "open", pDest,
		sqlite3_open(zDestFile, &pDest));
	if (rc) {
#ifdef	DYING
	    rpmsql_error(1, _("cannot open \"%s\""), zDestFile);
#endif
	    (void) rpmsqlCmd(sql, "close", pDest,
		sqlite3_close(pDest));
	    return 1;
	}
	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	pBackup = sqlite3_backup_init(pDest, "main", db, zDb);
	if (pBackup == NULL) {
	    rpmsql_error(1, "%s", sqlite3_errmsg(pDest));
	    (void) rpmsqlCmd(sql, "close", pDest,
			sqlite3_close(pDest));
	    return 1;
	}
	while ((rc = rpmsqlCmd(sql, "backup_step", db,
			sqlite3_backup_step(pBackup, 100))) == SQLITE_OK)
	    ;
	(void) rpmsqlCmd(sql, "backup_finish", pBackup,
			sqlite3_backup_finish(pBackup));
	if (rc == SQLITE_DONE) {
	    rc = 0;
	} else {
	    rpmsql_error(1, "%s", sqlite3_errmsg(pDest));
	    rc = 1;
	}
	(void) rpmsqlCmd(sql, "close", pDest,
			sqlite3_close(pDest));
    } else
     if (c == 'b' && n >= 3 && !strncmp(azArg[0], "bail", n)
	     && nArg > 1 && nArg < 3) {
	if (booleanValue(azArg[1]))
	    sql->flags |= RPMSQL_FLAGS_BAIL;
	else
	    sql->flags &= ~RPMSQL_FLAGS_BAIL;
    } else
     if (c == 'd' && n > 1 && !strncmp(azArg[0], "databases", n) && nArg == 1) {
	/* XXX recursion b0rkage lies here. */
	uint32_t _flags = sql->flags;
	uint32_t _mode = sql->mode;
	int _cnt = sql->cnt;;
	int _colWidth[3];
	char *zErrMsg = NULL;
	memcpy(_colWidth, sql->colWidth, sizeof(_colWidth));
	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	sql->flags |= RPMSQL_FLAGS_SHOWHDR;
	sql->mode = RPMSQL_MODE_COLUMN;
	sql->colWidth[0] = 3;
	sql->colWidth[1] = 15;
	sql->colWidth[2] = 58;
	sql->cnt = 0;
	(void) rpmsqlCmd(sql, "exec", db,
		sqlite3_exec(db, "PRAGMA database_list;", callback, sql, &zErrMsg));
	if (zErrMsg) {
	    rpmsql_error(1, "%s", zErrMsg);
	    sqlite3_free(zErrMsg);
	    rc = 1;
	}
	memcpy(sql->colWidth, _colWidth, sizeof(_colWidth));
	sql->cnt = _cnt;
	sql->mode = _mode;
	sql->flags = _flags;
    } else
     if (c == 'd' && !strncmp(azArg[0], "dump", n) && nArg < 3) {
	char * t;
	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	/* When playing back a "dump", the content might appear in an order
	 ** which causes immediate foreign key constraints to be violated.
	 ** So disable foreign-key constraint enforcement to prevent problems. */
	rpmsqlFprintf(sql, "PRAGMA foreign_keys=OFF;\n");
	rpmsqlFprintf(sql, "BEGIN TRANSACTION;\n");
	sql->flags &= ~RPMSQL_FLAGS_WRITABLE;
	(void) rpmsqlCmd(sql, "exec", db,
		sqlite3_exec(db, "PRAGMA writable_schema=ON", 0, 0, 0));
	if (nArg == 1) {
	    t = rpmExpand("SELECT name, type, sql FROM sqlite_master"
			  " WHERE sql NOT NULL AND type=='table'"
			  " AND name!='sqlite_sequence'", NULL);
	    run_schema_dump_query(sql, t, NULL);
	    t = _free(t);
	    t = rpmExpand("SELECT name, type, sql FROM sqlite_master"
			  " WHERE name=='sqlite_sequence'", NULL);
	    run_schema_dump_query(sql, t, NULL);
	    t = _free(t);
	    t = rpmExpand("SELECT sql FROM sqlite_master"
			  " WHERE sql NOT NULL AND type IN ('index','trigger','view')", NULL);
	    run_table_dump_query(sql, db, t, NULL);
	    t = _free(t);
	} else {
	    int i;
	    for (i = 1; i < nArg; i++) {
		t = rpmExpand(  "SELECT name, type, sql FROM sqlite_master"
				" WHERE tbl_name LIKE '", azArg[i], "'"
				" AND type=='table' AND sql NOT NULL", NULL);
		run_schema_dump_query(sql, t, NULL);
		t = _free(t);
		t = rpmExpand(  "SELECT sql FROM sqlite_master"
				" WHERE sql NOT NULL"
				" AND type IN ('index','trigger','view')"
				" AND tbl_name LIKE '", azArg[i], "'", NULL);
		run_table_dump_query(sql, db, t, NULL);
		t = _free(t);
	    }
	}
	if (F_ISSET(sql, WRITABLE)) {
	    rpmsqlFprintf(sql, "PRAGMA writable_schema=OFF;\n");
	    sql->flags &= ~RPMSQL_FLAGS_WRITABLE;
	}
	(void) rpmsqlCmd(sql, "exec", db,
		sqlite3_exec(db, "PRAGMA writable_schema=OFF", 0, 0, 0));
	rpmsqlFprintf(sql, "COMMIT;\n");
    } else
     if (c == 'e' && !strncmp(azArg[0], "echo", n) && nArg > 1 && nArg < 3) {
	if (booleanValue(azArg[1]))
	    sql->flags |= RPMSQL_FLAGS_ECHO;
	else
	    sql->flags &= ~RPMSQL_FLAGS_ECHO;
    } else
     if (c == 'e' && !strncmp(azArg[0], "exit", n) && nArg == 1) {
	rc = 2;
    } else
     if (c == 'e' && !strncmp(azArg[0], "explain", n) && nArg < 3) {
	int val = nArg >= 2 ? booleanValue(azArg[1]) : 1;
	if (val == 1) {
	    if (!sql->explainPrev.valid) {
		sql->explainPrev.valid = 1;
		sql->explainPrev.mode = sql->mode;
		sql->explainPrev.flags = sql->flags;
		memcpy(sql->explainPrev.colWidth, sql->colWidth,
		       sizeof(sql->colWidth));
	    }
	    /* We could put this code under the !p->explainValid
	     ** condition so that it does not execute if we are already in
	     ** explain mode. However, always executing it allows us an easy
	     ** way to reset to explain mode in case the user previously
	     ** did an .explain followed by a .width, .mode or .header
	     ** command.
	     */
	    sql->mode = RPMSQL_MODE_EXPLAIN;
	    sql->flags |= RPMSQL_FLAGS_SHOWHDR;
	    memset(sql->colWidth, 0, ArraySize(sql->colWidth));
	    sql->colWidth[0] = 4;	/* addr */
	    sql->colWidth[1] = 13;	/* opcode */
	    sql->colWidth[2] = 4;	/* P1 */
	    sql->colWidth[3] = 4;	/* P2 */
	    sql->colWidth[4] = 4;	/* P3 */
	    sql->colWidth[5] = 13;	/* P4 */
	    sql->colWidth[6] = 2;	/* P5 */
	    sql->colWidth[7] = 13;	/* Comment */
	} else if (sql->explainPrev.valid) {
	    sql->explainPrev.valid = 0;
	    sql->mode = sql->explainPrev.mode;
	    sql->flags = sql->explainPrev.flags;
	    memcpy(sql->colWidth, sql->explainPrev.colWidth,
		   sizeof(sql->colWidth));
	}
    } else
     if (c == 'h'
      && (!strncmp(azArg[0], "header", n) || !strncmp(azArg[0], "headers", n))
	     && nArg > 1 && nArg < 3)
     {
	if (booleanValue(azArg[1]))
	    sql->flags |= RPMSQL_FLAGS_SHOWHDR;
	else
	    sql->flags &= ~RPMSQL_FLAGS_SHOWHDR;
    } else
     if (c == 'h' && !strncmp(azArg[0], "help", n)) {
	rpmsql_error(0, "%s", zHelp);
	if (HAS_TIMER)
	    rpmsql_error(0, "%s", zTimerHelp);
    } else
     if (c == 'i' && !strncmp(azArg[0], "import", n) && nArg == 3) {
	char *zTable = azArg[2];	/* Insert data into this table */
	char *zFile = azArg[1];	/* The file from which to extract data */
	sqlite3_stmt * pStmt = NULL;/* A statement */
	int nCol;		/* Number of columns in the table */
	int nByte;		/* Number of bytes in an SQL string */
	int i, j;		/* Loop counters */
	int nSep;		/* Number of bytes in sql->separator[] */
	char *zSql;		/* An SQL statement */
	char *zLine;		/* A single line of input from the file */
	char **azCol;		/* zLine[] broken up into columns */
	char *zCommit;		/* How to commit changes */
	int lineno = 0;		/* Line number of input file */

	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	nSep = strlen30(sql->separator);
	if (nSep == 0) {
	    rpmsql_error(1, _("non-null separator required for import"));
	    return 1;
	}
	zSql = sqlite3_mprintf("SELECT * FROM '%q'", zTable);
assert(zSql != NULL);
	nByte = strlen30(zSql);
	rc = rpmsqlCmd(sql, "prepare", db,
		sqlite3_prepare(db, zSql, -1, &pStmt, 0));
	sqlite3_free(zSql);
	if (rc) {
#ifdef	DYING
	    sqlite3 * db = (sqlite3 *)sql->I;
	    rpmsql_error(1, "%s", sqlite3_errmsg(db));
#endif
	    if (pStmt)
		(void) rpmsqlCmd(sql, "finalize", db,
			sqlite3_finalize(pStmt));
	    return 1;
	}
	nCol = sqlite3_column_count(pStmt);
	(void) rpmsqlCmd(sql, "finalize", db,
			sqlite3_finalize(pStmt));
	pStmt = 0;
	if (nCol == 0)
	    return 0;		/* no columns, no error */
	zSql = xmalloc(nByte + 20 + nCol * 2);
	sqlite3_snprintf(nByte + 20, zSql, "INSERT INTO '%q' VALUES(?",
			 zTable);
	j = strlen30(zSql);
	for (i = 1; i < nCol; i++) {
	    zSql[j++] = ',';
	    zSql[j++] = '?';
	}
	zSql[j++] = ')';
	zSql[j] = 0;
	rc = rpmsqlCmd(sql, "prepare", db,
		sqlite3_prepare(db, zSql, -1, &pStmt, 0));
	zSql = _free(zSql);
	if (rc) {
#ifdef	DYING
	    sqlite3 * db = (sqlite3 *)sql->I;
	    rpmsql_error(1, "%s", sqlite3_errmsg(db));
#endif
	    if (pStmt)
		(void) rpmsqlCmd(sql, "finalize", db,
			sqlite3_finalize(pStmt));
	    return 1;
	}
assert(sql->ifd == NULL);
	sql->ifd = Fopen(zFile, "rb.fpio");
	if (sql->ifd == NULL || Ferror(sql->ifd)) {
	    rpmsql_error(1, _("cannot open \"%s\""), zFile);
	    (void) rpmsqlCmd(sql, "finalize", db,
			sqlite3_finalize(pStmt));
	    if (sql->ifd) (void) Fclose(sql->ifd);
	    sql->ifd = NULL;
	    return 1;
	}
assert(sql->buf == NULL);
sql->nbuf = BUFSIZ;
sql->buf = xmalloc(sql->nbuf);
	azCol = malloc(sizeof(azCol[0]) * (nCol + 1));
	if (azCol == NULL) {
	    if (sql->ifd) (void) Fclose(sql->ifd);
	    sql->ifd = NULL;
	    (void) rpmsqlCmd(sql, "finalize", db,
			sqlite3_finalize(pStmt));
assert(azCol);
	}
	(void) rpmsqlCmd(sql, "exec", db,
		sqlite3_exec(db, "BEGIN", 0, 0, 0));
	zCommit = "COMMIT";
	while ((zLine = local_getline(sql, NULL)) != NULL) {
	    char *z;
	    i = 0;
	    lineno++;
	    azCol[0] = zLine;
	    for (i = 0, z = zLine; *z && *z != '\n' && *z != '\r'; z++) {
		if (*z == sql->separator[0] && !strncmp(z, sql->separator, nSep)) {
		    *z = '\0';
		    i++;
		    if (i < nCol) {
			azCol[i] = &z[nSep];
			z += nSep - 1;
		    }
		}
	    }			/* end for */
	    *z = '\0';
	    if (i + 1 != nCol) {
		rpmsql_error(1,
			_("%s line %d: expected %d columns of data but found %d"),
			zFile, lineno, nCol, i + 1);
		zCommit = "ROLLBACK";
		rc = 1;
		break;		/* from while */
	    }
	    for (i = 0; i < nCol; i++)
		rc = rpmsqlCmd(sql, "bind_text", db,
			sqlite3_bind_text(pStmt, i + 1, azCol[i], -1, SQLITE_STATIC));
	    rc = rpmsqlCmd(sql, "step", db,
			sqlite3_step(pStmt));
	    rc = rpmsqlCmd(sql, "reset", db,
		sqlite3_reset(pStmt));
	    if (rc) {
#ifdef	DYING
		sqlite3 * db = (sqlite3 *)sql->I;
		rpmsql_error(1, "%s", sqlite3_errmsg(db));
#endif
		zCommit = "ROLLBACK";
		rc = 1;
		break;		/* from while */
	    }
	}			/* end while */
	azCol = _free(azCol);
	if (sql->ifd) (void) Fclose(sql->ifd);
	sql->ifd = NULL;
sql->buf = _free(sql->buf);
sql->nbuf = 0;
	(void) rpmsqlCmd(sql, "finalize", db,
		sqlite3_finalize(pStmt));
	(void) rpmsqlCmd(sql, "exec", db,
		sqlite3_exec(db, zCommit, 0, 0, 0));
    } else
     if (c == 'i' && !strncmp(azArg[0], "indices", n) && nArg < 3) {
	/* XXX recursion b0rkage lies here. */
	uint32_t _flags = sql->flags;
	uint32_t _mode = sql->mode;
	char * t;
	char *zErrMsg = NULL;
	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	sql->flags &= ~RPMSQL_FLAGS_SHOWHDR;
	sql->mode = RPMSQL_MODE_LIST;
	if (nArg == 1) {
	    t = rpmExpand("SELECT name FROM sqlite_master"
			  " WHERE type='index' AND name NOT LIKE 'sqlite_%'"
			  " UNION ALL "
			  "SELECT name FROM sqlite_temp_master"
			  " WHERE type='index'"
			  " ORDER BY 1", NULL);
	    rc = rpmsqlCmd(sql, "exec", db,
			sqlite3_exec(db, t, callback, sql, &zErrMsg));
	    t = _free(t);
	} else {
	    t = rpmExpand("SELECT name FROM sqlite_master"
			  " WHERE type='index' AND tbl_name LIKE '", azArg[1], "'",
			  " UNION ALL "
			  "SELECT name FROM sqlite_temp_master"
			  " WHERE type='index' AND tbl_name LIKE '", azArg[1], "'",
			  " ORDER BY 1", NULL);
	    rc = rpmsqlCmd(sql, "exec", db,
			sqlite3_exec(db, t, callback, sql, &zErrMsg));
	    t = _free(t);
	}
	if (zErrMsg) {
	    rpmsql_error(1, "%s", zErrMsg);
	    sqlite3_free(zErrMsg);
	    rc = 1;
	} else if (rc) {
#ifdef	DYING
	    rpmsql_error(1, _("querying sqlite_master and sqlite_temp_master"));
#endif
	    rc = 1;
	}
	sql->mode = _mode;
	sql->flags = _flags;
    } else

#ifdef SQLITE_ENABLE_IOTRACE
    if (c == 'i' && !strncmp(azArg[0], "iotrace", n)) {
	extern void (*sqlite3IoTrace) (const char *, ...);
	rc = rpmsqlFOpen((nArg >= 2 ? azArg[1] : NULL), &sql->tfd);
	sqlite3IoTrace = (sql->tfd ? iotracePrintf : NULL);
    } else
#endif

    if (c == 'l' && !strncmp(azArg[0], "load", n) && nArg >= 2) {
	const char *zFile, *zProc;
	char *zErrMsg = 0;
	zFile = azArg[1];
	zProc = nArg >= 3 ? azArg[2] : 0;
	if (!F_ISSET(sql, NOLOAD)) {
	    _rpmsqlOpenDB(sql);
	    db = (sqlite3 *)sql->I;
	    rc = rpmsqlCmd(sql, "load_extension", db,
			sqlite3_load_extension(db, zFile, zProc, &zErrMsg));
	    if (rc) {
		rpmsql_error(1, "%s", zErrMsg);
		sqlite3_free(zErrMsg);
		rc = 1;
	    }
	}
    } else

    if (c == 'l' && !strncmp(azArg[0], "log", n) && nArg >= 1) {
	/* XXX set rc? */
	(void) rpmsqlFOpen((nArg >= 2 ? azArg[1] : NULL), &sql->lfd);
    } else
     if (c == 'm' && !strncmp(azArg[0], "mode", n) && nArg == 2) {
	int n2 = strlen30(azArg[1]);
	if ((n2 == 4 && !strncmp(azArg[1], "line", n2))
	    || (n2 == 5 && !strncmp(azArg[1], "lines", n2))) {
	    sql->mode = RPMSQL_MODE_LINE;
	} else if ((n2 == 6 && !strncmp(azArg[1], "column", n2))
		   || (n2 == 7 && !strncmp(azArg[1], "columns", n2))) {
	    sql->mode = RPMSQL_MODE_COLUMN;
	} else if (n2 == 4 && !strncmp(azArg[1], "list", n2)) {
	    sql->mode = RPMSQL_MODE_LIST;
	} else if (n2 == 4 && !strncmp(azArg[1], "html", n2)) {
	    sql->mode = RPMSQL_MODE_HTML;
	} else if (n2 == 3 && !strncmp(azArg[1], "tcl", n2)) {
	    sql->mode = RPMSQL_MODE_TCL;
	} else if (n2 == 3 && !strncmp(azArg[1], "csv", n2)) {
	    sql->mode = RPMSQL_MODE_CSV;
	    (void) stpcpy(sql->separator, ",");
	} else if (n2 == 4 && !strncmp(azArg[1], "tabs", n2)) {
	    sql->mode = RPMSQL_MODE_LIST;
	    (void) stpcpy(sql->separator, "\t");
	} else if (n2 == 6 && !strncmp(azArg[1], "insert", n2)) {
	    sql->mode = RPMSQL_MODE_INSERT;
	    set_table_name(sql, "table");
	} else {
	    rpmsql_error(1, _("mode should be one of: %s"),
		    "column csv html insert line list tabs tcl");
	    rc = 1;
	}
    } else
     if (c == 'm' && !strncmp(azArg[0], "mode", n) && nArg == 3) {
	int n2 = strlen30(azArg[1]);
	if (n2 == 6 && !strncmp(azArg[1], "insert", n2)) {
	    sql->mode = RPMSQL_MODE_INSERT;
	    set_table_name(sql, azArg[2]);
	} else {
	    rpmsql_error(1, _("invalid arguments: "
		    " \"%s\". Enter \".help\" for help"), azArg[2]);
	    rc = 1;
	}
    } else
     if (c == 'n' && !strncmp(azArg[0], "nullvalue", n) && nArg == 2) {
	(void) stpncpy(sql->nullvalue, azArg[1], sizeof(sql->nullvalue)-1);
    } else
     if (c == 'o' && !strncmp(azArg[0], "output", n) && nArg == 2) {
	rc = rpmsqlFOpen((nArg >= 2 ? azArg[1] : NULL), &sql->ofd);

	/* Make sure sql->ofd squirts _SOMEWHERE_. Save the name too. */
	sql->outfile = _free(sql->outfile);
	if (sql->ofd)
	    sql->outfile = xstrdup(azArg[1]);
	else {
	    sql->ofd = fdDup(STDOUT_FILENO);
	    sql->outfile = xstrdup("stdout");
	}
    } else
     if (c == 'p' && !strncmp(azArg[0], "prompt", n)
	     && (nArg == 2 || nArg == 3)) {
	if (nArg >= 2) {
	    sql->zPrompt = _free(sql->zPrompt);
	    sql->zPrompt = xstrdup(azArg[1]);
	}
	if (nArg >= 3) {
	    sql->zContinue = _free(sql->zContinue);
	    sql->zContinue = xstrdup(azArg[2]);
	}
    } else
     if (c == 'q' && !strncmp(azArg[0], "quit", n) && nArg == 1) {
	rc = 2;
    } else
     if (c == 'r' && n >= 3 && !strncmp(azArg[0], "read", n)
	     && nArg == 2) {
	FD_t _ifd = sql->ifd;
	sql->ifd = Fopen(azArg[1], "rb.fpio");
	if (sql->ifd == NULL || Ferror(sql->ifd)) {
	    rpmsql_error(1, _("cannot open \"%s\""), azArg[1]);
	    rc = 1;
	} else {
	    /* XXX .read assumes .echo off? */
	    rc = rpmsqlInput(sql);
	}
	if (sql->ifd) (void) Fclose(sql->ifd);
	sql->ifd = _ifd;
    } else
     if (c == 'r' && n >= 3 && !strncmp(azArg[0], "restore", n)
	     && nArg > 1 && nArg < 4) {
	const char *zSrcFile;
	const char *zDb;
	sqlite3 * pSrc;
	sqlite3_backup *pBackup;
	int nTimeout = 0;

	if (nArg == 2) {
	    zSrcFile = azArg[1];
	    zDb = "main";
	} else {
	    zSrcFile = azArg[2];
	    zDb = azArg[1];
	}
	rc = rpmsqlCmd(sql, "open", pSrc,	/* XXX watchout: arg order */
		sqlite3_open(zSrcFile, &pSrc));
	if (rc) {
#ifdef	DYING
	    rpmsql_error(1, _("cannot open \"%s\""), zSrcFile);
#endif
	    (void) rpmsqlCmd(sql, "close", pSrc,
		sqlite3_close(pSrc));
	    return 1;
	}
	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	pBackup = sqlite3_backup_init(db, zDb, pSrc, "main");
	if (pBackup == 0) {
	    rpmsql_error(1, "%s", sqlite3_errmsg(db));
	    (void) rpmsqlCmd(sql, "close", db,
			sqlite3_close(pSrc));
	    return 1;
	}
	while ((rc = sqlite3_backup_step(pBackup, 100)) == SQLITE_OK
	       || rc == SQLITE_BUSY)
	{
	    if (rc == SQLITE_BUSY) {
		if (nTimeout++ >= 3)
		    break;
		sqlite3_sleep(100);
	    }
	}
	sqlite3_backup_finish(pBackup);
	switch (rc) {
	case SQLITE_DONE:
	    rc = 0;
	    break;
	case SQLITE_BUSY:
	case SQLITE_LOCKED:
	    rpmsql_error(1, _("source database is busy"));
	    rc = 1;
	    break;
	default:
	    rpmsql_error(1, "%s", sqlite3_errmsg(db));
	    rc = 1;
	    break;
	}
	(void) rpmsqlCmd(sql, "close", pSrc,
		sqlite3_close(pSrc));
    } else
     if (c == 's' && !strncmp(azArg[0], "schema", n) && nArg < 3) {
	/* XXX recursion b0rkage lies here. */
	uint32_t _flags = sql->flags;
	uint32_t _mode = sql->mode;
	char *zErrMsg = 0;
	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	sql->flags &= ~RPMSQL_FLAGS_SHOWHDR;
	sql->mode = RPMSQL_MODE_SEMI;
	if (nArg > 1) {
	    int i;
	    for (i = 0; azArg[1][i]; i++)
		azArg[1][i] = (char) tolower(azArg[1][i]);
	    if (!strcmp(azArg[1], "sqlite_master")) {
		char *new_argv[2], *new_colv[2];
		new_argv[0] = "CREATE TABLE sqlite_master (\n"
		    "  type text,\n"
		    "  name text,\n"
		    "  tbl_name text,\n"
		    "  rootpage integer,\n" "  sql text\n" ")";
		new_argv[1] = 0;
		new_colv[0] = "sql";
		new_colv[1] = 0;
		callback(sql, 1, new_argv, new_colv);
		rc = SQLITE_OK;
	    } else if (!strcmp(azArg[1], "sqlite_temp_master")) {
		char *new_argv[2], *new_colv[2];
		new_argv[0] = "CREATE TEMP TABLE sqlite_temp_master (\n"
		    "  type text,\n"
		    "  name text,\n"
		    "  tbl_name text,\n"
		    "  rootpage integer,\n" "  sql text\n" ")";
		new_argv[1] = 0;
		new_colv[0] = "sql";
		new_colv[1] = 0;
		callback(sql, 1, new_argv, new_colv);
		rc = SQLITE_OK;
	    } else {
		char * t;
		t = rpmExpand(	"SELECT sql FROM "
				"  (SELECT sql sql, type type, tbl_name tbl_name, name name"
				"     FROM sqlite_master UNION ALL"
				"   SELECT sql, type, tbl_name, name FROM sqlite_temp_master)"
				" WHERE tbl_name LIKE '", azArg[1], "'"
				" AND type!='meta' AND sql NOTNULL "
				"ORDER BY substr(type,2,1), name", NULL);
		rc = rpmsqlCmd(sql, "exec", db,
			sqlite3_exec(db, t, callback, sql, &zErrMsg));
		t = _free(t);
	    }
	    sql->mode = _mode;
	    sql->flags = _flags;
	} else {
	    rc = rpmsqlCmd(sql, "exec", db,
			sqlite3_exec(db,
			      "SELECT sql FROM "
			      "  (SELECT sql sql, type type, tbl_name tbl_name, name name"
			      "     FROM sqlite_master UNION ALL"
			      "   SELECT sql, type, tbl_name, name FROM sqlite_temp_master) "
			      "WHERE type!='meta' AND sql NOTNULL AND name NOT LIKE 'sqlite_%'"
			      "ORDER BY substr(type,2,1), name",
			      callback, sql, &zErrMsg));
	}
	if (zErrMsg) {
	    rpmsql_error(1, "%s", zErrMsg);
	    sqlite3_free(zErrMsg);
	    rc = 1;
	} else if (rc != SQLITE_OK) {
	    rpmsql_error(1, _("querying schema information"));
	    rc = 1;
	} else {
	    rc = 0;
	}
    } else
     if (c == 's' && !strncmp(azArg[0], "separator", n) && nArg == 2) {
	(void) stpncpy(sql->separator, azArg[1], sizeof(sql->separator)-1);
    } else
     if (c == 's' && !strncmp(azArg[0], "show", n) && nArg == 1) {
	int i;
	rpmsqlFprintf(sql, "%9.9s: %s\n", "echo", F_ISSET(sql, ECHO) ? "on" : "off");
	rpmsqlFprintf(sql, "%9.9s: %s\n", "explain",
		sql->explainPrev.valid ? "on" : "off");
	rpmsqlFprintf(sql, "%9.9s: %s\n", "headers",
		F_ISSET(sql, SHOWHDR) ? "on" : "off");
	rpmsqlFprintf(sql, "%9.9s: %s\n", "mode", modeDescr[sql->mode]);
	rpmsqlFprintf(sql, "%9.9s: ", "nullvalue");
	output_c_string(sql, sql->nullvalue);
	rpmsqlFprintf(sql, "\n");
	rpmsqlFprintf(sql, "%9.9s: %s\n", "output",
		(sql->outfile ? sql->outfile : "stdout"));
	rpmsqlFprintf(sql, "%9.9s: ", "separator");
	output_c_string(sql, sql->separator);
	rpmsqlFprintf(sql, "\n");
	rpmsqlFprintf(sql, "%9.9s: ", "width");
	for (i = 0;
	     i < (int) ArraySize(sql->colWidth) && sql->colWidth[i] != 0;
	     i++)
	{
	    rpmsqlFprintf(sql, "%d ", sql->colWidth[i]);
	}
	rpmsqlFprintf(sql, "\n");
    } else
     if (c == 't' && n > 1 && !strncmp(azArg[0], "tables", n) && nArg < 3) {
	char **azResult;
	int nRow;
	char *zErrMsg;
	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	if (nArg == 1) {
	    rc = rpmsqlCmd(sql, "get_table", db,
			sqlite3_get_table(db,
				   "SELECT name FROM sqlite_master "
				   "WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%' "
				   "UNION ALL "
				   "SELECT name FROM sqlite_temp_master "
				   "WHERE type IN ('table','view') "
				   "ORDER BY 1",
				   &azResult, &nRow, 0, &zErrMsg));
	} else {
	    char * t;
	    t = rpmExpand("SELECT name FROM sqlite_master "
			  " WHERE type IN ('table','view') AND name LIKE '", azArg[1], "'"
			  " UNION ALL "
			  "SELECT name FROM sqlite_temp_master"
			  " WHERE type IN ('table','view') AND name LIKE '", azArg[1], "'"
				   "ORDER BY 1", NULL);
	    rc = rpmsqlCmd(sql, "get_table", db,
			sqlite3_get_table(db, t, &azResult, &nRow, 0,&zErrMsg));
	    t = _free(t);
	}
	if (zErrMsg) {
	    rpmsql_error(1, "%s", zErrMsg);
	    sqlite3_free(zErrMsg);
	    rc = 1;
	} else if (rc) {
	    rpmsql_error(1, _("querying sqlite_master and sqlite_temp_master"));
	    rc = 1;
	} else {
	    int len, maxlen = 0;
	    int i, j;
	    int nPrintCol, nPrintRow;
	    for (i = 1; i <= nRow; i++) {
		if (azResult[i] == 0)
		    continue;
		len = strlen30(azResult[i]);
		if (len > maxlen)
		    maxlen = len;
	    }
	    nPrintCol = 80 / (maxlen + 2);
	    if (nPrintCol < 1)
		nPrintCol = 1;
	    nPrintRow = (nRow + nPrintCol - 1) / nPrintCol;
	    for (i = 0; i < nPrintRow; i++) {
		for (j = i + 1; j <= nRow; j += nPrintRow) {
		    char *zSp = j <= nPrintRow ? "" : "  ";
		    rpmsqlFprintf(sql, "%s%-*s", zSp, maxlen,
			   azResult[j] ? azResult[j] : "");
		}
		rpmsqlFprintf(sql, "\n");
	    }
	}
	sqlite3_free_table(azResult);
    } else
     if (c == 't' && n > 4 && !strncmp(azArg[0], "timeout", n) && nArg == 2) {
	_rpmsqlOpenDB(sql);
	db = (sqlite3 *)sql->I;
	(void) rpmsqlCmd(sql, "busy_timeout", db,
		sqlite3_busy_timeout(db, atoi(azArg[1])));
    } else
     if (HAS_TIMER && c == 't' && n >= 5
	     && !strncmp(azArg[0], "timer", n) && nArg == 2) {
	sql->enableTimer = booleanValue(azArg[1]);
    } else
     if (c == 'w' && !strncmp(azArg[0], "width", n) && nArg > 1) {
	int j;
assert(nArg <= ArraySize(azArg));
	for (j = 1; j < nArg && j < ArraySize(sql->colWidth); j++)
	    sql->colWidth[j - 1] = atoi(azArg[j]);
    } else
    {
	rpmsql_error(1, _("unknown command or invalid arguments: "
		" \"%s\". Enter \".help\" for help"), azArg[0]);
	rc = 1;
    }

    return rc;
}

#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

#if defined(WITH_SQLITE)

/**
 * Return TRUE if a semicolon occurs anywhere in the first N characters
 * of string z[].
 * @param z	input string
 * @param N	no. of chars to search
 * @return
 */
static int _contains_semicolon(const char *z, int N)
{
    int rc = 0;
    int i;
    for (i = 0; i < N; i++) {
	if (z[i] != ';')
	    continue;
	rc = 1;
	break;
    }
SQLDBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, z, rc));
    return rc;
}

/**
 * Test to see if a line consists entirely of whitespace.
 * @param z	input string
 */
static int _all_whitespace(const char *z)
{
    int rc = 1;

    for (; *z; z++) {
	if (xisspace(*(unsigned char *) z))
	    continue;
	if (*z == '/' && z[1] == '*') {
	    z += 2;
	    while (*z && (*z != '*' || z[1] != '/'))
		z++;
	    if (*z == '\0') {
		rc = 0;
		break;
	    }
	    z++;
	    continue;
	}
	if (*z == '-' && z[1] == '-') {
	    z += 2;
	    while (*z && *z != '\n')
		z++;
	    if (*z == '\0')
		break;
	    continue;
	}
	rc = 0;
	break;
    }

SQLDBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, z, rc));
    return rc;
}

/**
 * Return TRUE if the line typed in is an SQL command terminator other
 * than a semi-colon.  The SQL Server style "go" command is understood
 * as is the Oracle "/".
 * @param zLine	input string
 */
static int _is_command_terminator(const char *zLine)
{
    int rc = 1;

    while (xisspace(*(unsigned char *) zLine))
	zLine++;
    if (zLine[0] == '/' && _all_whitespace(&zLine[1]))
	goto exit;		/* Oracle */
    if (xtolower(zLine[0]) == 'g' && xtolower(zLine[1]) == 'o'
     && _all_whitespace(&zLine[2]))
	goto exit;		/* SQL Server */
    rc = 0;
exit:
SQLDBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, zLine, rc));
    return rc;
}

/**
 * Return true if zSql is a complete SQL statement.  Return false if it
 * ends in the middle of a string literal or C-style comment.
 * @param zSql
 * @param nSql
 * @return		true if complete sql statement
 */
static int _is_complete(char *zSql, int nSql)
{
    int rc = 1;
    if (zSql == NULL)
	goto exit;
    zSql[nSql] = ';';
    zSql[nSql + 1] = '\0';
    rc = sqlite3_complete(zSql);
    zSql[nSql] = '\0';
exit:
SQLDBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, zSql, rc));
    return rc;
}

/**
 * Read input from *in and process it.  If *in==0 then input
 * is interactive - the user is typing it it.  Otherwise, input
 * is coming from a file or device.  A prompt is issued and history
 * is saved only if input is interactive.  An interrupt signal will
 * cause this routine to exit immediately, unless input is interactive.
 *
 * @param sql		sql interpreter
 * @return		the number of errors
 */
static int rpmsqlInput(rpmsql sql)
{
    sqlite3 * db = (sqlite3 *) sql->I;
    char *zLine = 0;
    char *zSql = 0;
    int nSql = 0;
    int nSqlPrior = 0;
    char *zErrMsg;
    int rc;
    int errCnt = 0;
    int lineno = 0;
    int startline = 0;

char * _buf = sql->buf;
size_t _nbuf = sql->nbuf;

SQLDBG((stderr, "--> %s(%p)\n", __FUNCTION__, sql));
if (_rpmsql_debug < 0)
rpmsqlDebugDump(sql);

sql->nbuf = BUFSIZ;
sql->buf = xmalloc(sql->nbuf);

    while (errCnt == 0 || !F_ISSET(sql, BAIL) || F_ISSET(sql, PROMPT))
    {
	if (sql->ofd) Fflush(sql->ofd);
	zLine = rpmsqlInputOneLine(sql, zSql);
	if (zLine == NULL)
	    break;		/* We have reached EOF */
	if (_rpmsqlSeenInterrupt) {
	    if (!F_ISSET(sql, PROMPT))
		break;
	    _rpmsqlSeenInterrupt = 0;
	}
	lineno++;
	if ((zSql == NULL || zSql[0] == '\0') && _all_whitespace(zLine))
	    continue;
	if (zLine && zLine[0] == '.' && nSql == 0) {
	    if (F_ISSET(sql, ECHO))
		rpmsqlFprintf(sql, "%s\n", zLine);
	    rc = rpmsqlMetaCommand(sql, zLine);
	    if (rc == 2)	/* exit requested */
		break;
	    else if (rc)
		errCnt++;
	    continue;
	}
	if (_is_command_terminator(zLine) && _is_complete(zSql, nSql))
	    memcpy(zLine, ";", 2);
	nSqlPrior = nSql;
	if (zSql == NULL) {
	    int i;
	    for (i = 0; zLine[i] && xisspace((unsigned char) zLine[i]); i++)
		;
	    if (zLine[i] != '\0') {
		nSql = strlen30(zLine);
		zSql = xmalloc(nSql + 3);
		memcpy(zSql, zLine, nSql + 1);
		startline = lineno;
	    }
	} else {
	    int len = strlen30(zLine);
	    zSql = xrealloc(zSql, nSql + len + 4);
	    zSql[nSql++] = '\n';
	    memcpy(&zSql[nSql], zLine, len + 1);
	    nSql += len;
	}
	if (zSql && _contains_semicolon(&zSql[nSqlPrior], nSql - nSqlPrior)
	    && sqlite3_complete(zSql)) {
	    sql->cnt = 0;
	    _rpmsqlOpenDB(sql);
	    db = (sqlite3 *)sql->I;
	    BEGIN_TIMER(sql);
	    rc = _rpmsqlShellExec(sql, zSql, _rpmsqlShellCallback, &zErrMsg);
	    END_TIMER(sql);
	    if (rc || zErrMsg) {
		char zPrefix[100];
		if (!F_ISSET(sql, PROMPT) || !F_ISSET(sql, INTERACTIVE))
		    snprintf(zPrefix, sizeof(zPrefix),
				     "near line %d: ", startline);
		else
		    zPrefix[0] = '\0';
		rpmsql_error(1, "%s%s", zPrefix,
			zErrMsg ? zErrMsg : sqlite3_errmsg(db));
		zErrMsg = _free(zErrMsg);
		errCnt++;
	    }
	    zSql = _free(zSql);
	    nSql = 0;
	}
    }
    if (zSql) {
	if (!_all_whitespace(zSql))
	    rpmsql_error(1, _("incomplete SQL: %s"), zSql);
	zSql = _free(zSql);
    }

sql->buf = _free(sql->buf);
sql->buf = _buf;
sql->nbuf = _nbuf;

SQLDBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, sql, errCnt));

    return errCnt;
}

/**
 * Read input from the sqliterc file parameter. 
 * If sqliterc is NULL, take input from ~/.sqliterc
 *
 * @param sql		sql interpreter
 * @param sqliterc	file name
 * @return		the number of errors
 */
static int rpmsqlInitRC(rpmsql sql, const char *sqliterc)
{
    int rc = 0;

SQLDBG((stderr, "--> %s(%p,%s)\n", __FUNCTION__, sql, sqliterc));
if (_rpmsql_debug < 0)
rpmsqlDebugDump(sql);

    if (sqliterc == NULL) 
	sqliterc = sql->zInitrc;
    if (sqliterc) {
	FD_t _ifd = sql->ifd;
	sql->ifd = Fopen(sqliterc, "rb.fpio");
	if (!(sql->ifd == NULL || Ferror(sql->ifd))) {
	    if (F_ISSET(sql, INTERACTIVE))
		rpmsql_error(0, "-- Loading resources from %s", sqliterc);
	    rc = rpmsqlInput(sql);
	}
	if (sql->ifd) (void) Fclose(sql->ifd);
	sql->ifd = _ifd;
    }

SQLDBG((stderr, "<-- %s(%p,%s) rc %d\n", __FUNCTION__, sql, sqliterc, rc));

    return rc;
}

#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

#if defined(WITH_SQLITE)
/**
 * POPT argument processing callback.
 */
static void rpmsqlArgCallback(poptContext con,
			      /*@unused@ */ enum poptCallbackReason reason,
			      const struct poptOption *opt,
			      const char *arg,
			      /*@unused@ */ void *_data)
	/*@ */
{
    rpmsql sql = &_sql;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'S':				/*    -separator x */
assert(arg != NULL);
	(void) stpncpy(sql->separator, arg, sizeof(sql->separator)-1);
	break;
    case 'N':				/*    -nullvalue text */
assert(arg != NULL);
	(void) stpncpy(sql->nullvalue, arg, sizeof(sql->nullvalue)-1);
	break;
    case 'V':				/*    -version */
	printf("%s\n", sqlite3_libversion());
	/*@-exitarg@ */ exit(0); /*@=exitarg@ */
	/*@notreached@ */ break;
    default:
	/* XXX fprintf(stderr, ...)? */
	rpmsql_error(0, _("%s: Unknown callback(0x%x)\n"),
		    __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@ */ exit(2); /*@=exitarg@ */
	/*@notreached@ */ break;
    }
}

/*@unchecked@*/ /*@observer@*/
static struct poptOption _rpmsqlOptions[] = {
    /*@-type@*//* FIX: cast? */
    {NULL, '\0',
     POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
     rpmsqlArgCallback, 0, NULL, NULL},
/*@=type@*/

 { "debug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_DOC_HIDDEN, &_rpmsql_debug, -1,
	N_("Debug embedded SQL interpreter"), NULL},
 { "create", '\0', POPT_BIT_SET|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_DOC_HIDDEN, &_sql.flags, RPMSQL_FLAGS_CREATE,
	N_("create database if not exists"), NULL},

 { "init", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	&_sql.zInitFile, 0,
	N_("read/process named FILE"), N_("FILE") },
 { "echo", '\0', POPT_BIT_SET|POPT_ARGFLAG_ONEDASH,	&_sql.flags, RPMSQL_FLAGS_ECHO,
	N_("print commands before execution"), NULL },

 { "load", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_ONEDASH, &_sql.flags, RPMSQL_FLAGS_NOLOAD,
	N_("disable extension loading (normally enabled)"), NULL },
 { "header", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_ONEDASH, &_sql.flags, RPMSQL_FLAGS_SHOWHDR,
	N_("turn headers on or off"), NULL },

 { "bail", '\0', POPT_BIT_SET|POPT_ARGFLAG_ONEDASH,	&_sql.flags, RPMSQL_FLAGS_BAIL,
	N_("stop after hitting an error"), NULL },

 { "interactive", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_ONEDASH,	&_sql.flags, RPMSQL_FLAGS_INTERACTIVE,
	N_("force interactive I/O"), NULL },
 { "batch", '\0', POPT_BIT_CLR|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_ONEDASH, &_sql.flags, RPMSQL_FLAGS_INTERACTIVE,
	N_("force batch I/O"), NULL },

 { "column", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&_sql.mode, RPMSQL_MODE_COLUMN,
	N_("set output mode to 'column'"), NULL },
 { "csv", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&_sql.mode, RPMSQL_MODE_CSV,
	N_("set output mode to 'csv'"), NULL },
 { "html", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&_sql.mode, RPMSQL_MODE_HTML,
	N_("set output mode to HTML"), NULL },
 { "line", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&_sql.mode, RPMSQL_MODE_LINE,
	N_("set output mode to 'line'"), NULL },
 { "list", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH,	&_sql.mode, RPMSQL_MODE_LIST,
	N_("set output mode to 'list'"), NULL },
 { "separator", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	0, 'S',
	N_("set output field separator (|)"), N_("CHAR") },
 { "nullvalue", '\0', POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,	0, 'N',
	N_("set text string for NULL values"), N_("TEXT") },

 { "version", '\0', POPT_ARG_NONE|POPT_ARGFLAG_ONEDASH,	0, 'V',
	N_("show SQLite version"), NULL},

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL},
#endif

    POPT_AUTOHELP
  { NULL, (char) -1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: dbsql [OPTIONS] FILENAME [SQL]\n\
FILENAME is the name of an SQLite database. A new database is created\n\
if the file does not previously exist.\n\
\n\
OPTIONS include:\n\
   -help                show this message\n\
   -init filename       read/process named file\n\
   -echo                print commands before execution\n\
   -[no]header          turn headers on or off\n\
   -bail                stop after hitting an error\n\
   -interactive         force interactive I/O\n\
   -batch               force batch I/O\n\
   -column              set output mode to 'column'\n\
   -csv                 set output mode to 'csv'\n\
   -html                set output mode to HTML\n\
   -line                set output mode to 'line'\n\
   -list                set output mode to 'list'\n\
   -separator 'x'       set output field separator (|)\n\
   -nullvalue 'text'    set text string for NULL values\n\
   -version             show SQLite version\n\
"), NULL},

    POPT_TABLEEND
};

#endif	/* defined(WITH_SQLITE) */

/*==============================================================*/

/**
 * rpmsql pool destructor.
 */
static void rpmsqlFini(void * _sql)
	/*@globals fileSystem @*/
	/*@modifies *_sql, fileSystem @*/
{
    rpmsql sql = _sql;

SQLDBG((stderr, "==> %s(%p)\n", __FUNCTION__, sql));

    sql->zDestTable = _free(sql->zDestTable);

    if (sql->ifd)
	(void) Fclose(sql->ifd);	/* XXX stdin dup'd */
    sql->ifd = NULL;
    if (sql->ofd)
	(void) Fclose(sql->ofd);	/* XXX stdout/stderr were dup'd */
    sql->ofd = NULL;
    if (sql->lfd)
	(void) Fclose(sql->lfd);
    sql->lfd = NULL;
    if (sql->tfd)
	(void) Fclose(sql->tfd);
    sql->tfd = NULL;

    sql->buf = _free(sql->buf);
    sql->buf = sql->b = NULL;
    sql->nbuf = sql->nb = 0;

    /* XXX INTERACTIVE cruft. */
    sql->zHome = _free(sql->zHome);
    sql->zInitrc = _free(sql->zInitrc);
    sql->zHistory = _free(sql->zHistory);
    sql->zPrompt = _free(sql->zPrompt);
    sql->zContinue = _free(sql->zContinue);

    sql->outfile = _free(sql->outfile);

    sql->zDbFilename = _free(sql->zDbFilename);
    sql->zInitFile = _free(sql->zInitFile);
    sql->av = argvFree(sql->av);
#if defined(WITH_SQLITE)
    if (sql->I) {
	sqlite3 * db = (sqlite3 *)sql->I;
	(void) rpmsqlCmd(sql, "close", db,
		sqlite3_close(db));
    }
#endif
    sql->I = NULL;
    (void) rpmiobFree(sql->iob);
    sql->iob = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmsqlPool;

static rpmsql rpmsqlGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmsqlPool, fileSystem @*/
	/*@modifies pool, _rpmsqlPool, fileSystem @*/
{
    rpmsql sql;

    if (_rpmsqlPool == NULL) {
	_rpmsqlPool = rpmioNewPool("sql", sizeof(*sql), -1, _rpmsql_debug,
			NULL, NULL, rpmsqlFini);
	pool = _rpmsqlPool;
    }
    sql = (rpmsql) rpmioGetPool(pool, sizeof(*sql));
    memset(((char *)sql)+sizeof(sql->_item), 0, sizeof(*sql)-sizeof(sql->_item));
    return sql;
}

const char ** rpmsqlArgv(rpmsql sql, int * argcp)
{
    const char ** av = sql->av;

    if (argcp)
	*argcp = argvCount(av);
    return av;
}

#if defined(WITH_SQLITE)
/**
 * Process object OPTIONS and ARGS.
 * @param sql		sql interpreter
 */
static void rpmsqlInitPopt(rpmsql sql, int ac, char ** av, poptOption tbl)
	/*@modifies sql @*/
{
    poptContext con;
    int rc;

    if (av == NULL || av[0] == NULL || av[1] == NULL)
	goto exit;

    con = poptGetContext(av[0], ac, (const char **)av, tbl, 0);

    /* Process all options into _sql, whine if unknown options. */
    while ((rc = poptGetNextOpt(con)) > 0) {
	const char * arg = poptGetOptArg(con);
	arg = _free(arg);
	switch (rc) {
	default:
	    /* XXX fprintf(stderr, ...)? */
	    rpmsql_error(0, _("%s: option table misconfigured (%d)\n"),
			__FUNCTION__, rc);
	    break;
	}
    }
    /* XXX FIXME: arrange error return iff rc < -1. */
if (rc < -1)
SQLDBG((stderr, "%s: poptGetNextOpt rc(%d): %s\n", __FUNCTION__, rc, poptStrerror(rc)));

    /* Move the POPT parsed values into the current rpmsql object. */
    sql->flags = _sql.flags;
    sql->mode = _sql.mode;
    if (_sql.zInitFile) {
	sql->zInitFile = _free(sql->zInitFile);
	sql->zInitFile = _sql.zInitFile;
	_sql.zInitFile = NULL;
    }
    memcpy(sql->separator, _sql.separator, sizeof(sql->separator));
    memcpy(sql->nullvalue, _sql.nullvalue, sizeof(sql->nullvalue));

    sql->av = argvFree(sql->av);
    rc = argvAppend(&sql->av, poptGetArgs(con));

    con = poptFreeContext(con);

exit:
    /* If not overridden, set the separator according to mode. */
    if (sql->separator[0] == '\0')
    switch (sql->mode) {
    default:
    case RPMSQL_MODE_LIST:	(void)stpcpy(sql->separator, "|");	break;
    case RPMSQL_MODE_CSV:	(void)stpcpy(sql->separator, ",");	break;
    }

SQLDBG((stderr, "<== %s(%p, %p[%u], %p)\n", __FUNCTION__, sql, av, (unsigned)ac, tbl));
}
#endif /* defined(WITH_SQLITE) */

rpmsql rpmsqlNew(char ** av, uint32_t flags)
{
    rpmsql sql =
	(flags & 0x80000000) ? rpmsqlI() :
	rpmsqlGetPool(_rpmsqlPool);
    int ac = argvCount((ARGV_t)av);

SQLDBG((stderr, "==> %s(%p[%u], 0x%x)\n", __FUNCTION__, av, (unsigned)ac, flags));
if (av && _rpmsql_debug < 0)
argvPrint("av", (ARGV_t)av, NULL);

    sql->flags = flags;		/* XXX useful? */

#if defined(WITH_SQLITE)
    /* XXX Avoid initialization on global interpreter creation path. */
    if (av) {
	static int _oneshot;
	sqlite3 * db = NULL;
	int xx;

	if (!_oneshot) {
#if defined(SQLITE_CONFIG_LOG)
	    sqlite3_config(SQLITE_CONFIG_LOG, shellLog, sql);
#endif
	    sqlite3_config(SQLITE_CONFIG_SINGLETHREAD);
	    _oneshot++;
	}

	/* Initialize defaults for popt parsing. */
	memset(&_sql, 0, sizeof(_sql));
	sql->flags = _sql.flags = flags; /* XXX INTERACTIVE defaulted here. */
	sql->mode = _sql.mode = RPMSQL_MODE_LIST;

	rpmsqlInitPopt(sql, ac, av, (poptOption) _rpmsqlOptions);

	/* The 1st argument is the database to open (or :memory: default). */
	if (sql->av && sql->av[0]) {
	    sql->zDbFilename = xstrdup(sql->av[0]);	/* XXX strdup? */
	    /* If requested or database already exists, open immediately. */
	    if (F_ISSET(sql, CREATE) || !Access(sql->zDbFilename, R_OK)) {
		xx = rpmsqlCmd(sql, "open", db,	/* XXX watchout: arg order */
			sqlite3_open(sql->zDbFilename, &db));
		sql->I = (void *) db;
	    }
	} else
	    sql->zDbFilename = xstrdup(":memory:");

	/* Read ~/.sqliterc (if specified), then reparse options. */
	if (sql->zInitFile || F_ISSET(sql, INTERACTIVE)) {
	    sql->ofd = fdDup(STDOUT_FILENO);
	    xx = rpmsqlInitRC(sql, sql->zInitFile);
	    if (sql->ofd) (void) Fclose(sql->ofd);
	    sql->ofd = NULL;
	    rpmsqlInitPopt(sql, ac, av, (poptOption) _rpmsqlOptions);
	}

    }

    { /* XXX INTERACTIVE cruft. */
	static const char _zInitrc[]	= "/.sqliterc";
	static const char _zHistory[]	= "/.sqlite_history";
	/* XXX getpwuid? */
sql->zHome = _free(sql->zHome);
	{   char * t = getenv("HOME");
	    sql->zHome = xstrdup((t ? t : "/"));
	}
sql->zInitrc = _free(sql->zInitrc);
	sql->zInitrc = rpmGetPath(sql->zHome, _zInitrc, NULL);
sql->zHistory = _free(sql->zHistory);
	sql->zHistory = rpmGetPath(sql->zHome, _zHistory, NULL);
	/*
	 ** Prompt strings. Initialized in main. Settable with
	 **   .prompt main continue
	 */
	/* Initialize the prompt from basename(argv[0]). */
	if (sql->zPrompt == NULL) {	/* XXX this test is useless */
	    char * t = xstrdup((av && av[0] ? av[0] : "sql"));
	    char * bn = basename(t);
sql->zPrompt = _free(sql->zPrompt);
	    sql->zPrompt = rpmExpand(bn, "> ", NULL);
	    t = _free(t);
sql->zContinue = _free(sql->zContinue);
	    sql->zContinue = t = xstrdup(sql->zPrompt);
	    while (*t && *t != '>')
		*t++ = '-';
	}
    }
#else	/* WITH_SQLITE */
    if (av)
	(void) argvAppend(&sql->av, (ARGV_t) av);	/* XXX useful? */
#endif	/* WITH_SQLITE */

    /* Set sane defaults for output sink(s) dependent on INTERACTIVE. */
    if (F_ISSET(sql, INTERACTIVE)) {
	if (sql->ofd == NULL)
	    sql->ofd = fdDup(STDOUT_FILENO);
    } else {
	if (sql->iob == NULL)
	    sql->iob = rpmiobNew(0);
    }

    return rpmsqlLink(sql);
}

rpmRC rpmsqlRun(rpmsql sql, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

SQLDBG((stderr, "==> %s(%p,%p[%u]) \"%s\"\n", __FUNCTION__, sql, str, (unsigned)(str ? strlen(str) : 0), str));
SQLDBG((stderr, "==========>\n%s\n<==========\n", str));

    if (sql == NULL) sql = rpmsqlI();

#if defined(WITH_SQLITE)
    if (str != NULL) {
	const char * s = str;

	/* Ignore leading whitespace. */
	while (*s && xisspace((int)*s))
	    s++;

	/* Perform the SQL operation(s). */
	if (*s == '\0') {				/* INTERACTIVE */
	    static int oneshot;
	    uint32_t _flags = sql->flags;
	    FD_t _ofd = sql->ofd;
	    FD_t _ifd = sql->ifd;

SQLDBG((stderr, "*** %s: INTERACTIVE\n", __FUNCTION__));
	    sql->flags |= RPMSQL_FLAGS_INTERACTIVE;
	    if (sql->ofd == NULL)
		sql->ofd = fdDup(STDOUT_FILENO);
	    if (!oneshot) {
#ifdef	REFERENCE
		extern char *db_full_version(int *, int *, int *, int *, int *);
		fprintf(sql->out, "%s\n"
			"Enter \".help\" for instructions\n"
			"Enter SQL statements terminated with a \";\"\n",
			db_full_version(NULL, NULL, NULL, NULL, NULL));
#endif
		size_t nb;
		size_t nw;
		char * t = rpmExpand(
			"SQLite version ", sqlite3_libversion(), "\n",
#if SQLITE_VERSION_NUMBER > 3006015
			"\t(", sqlite3_sourceid(), ")\n",
#endif
			"Enter \".help\" for instructions\n",
			"Enter SQL statements terminated with a \";\"\n", NULL);
		nb = strlen(t);
		nw = Fwrite(t, 1, nb, sql->ofd);
		(void) Fflush(sql->ofd);
assert(nb == nw);
		t = _free(t);
#if defined(HAVE_READLINE) && HAVE_READLINE==1
		if (sql->zHistory)
		    read_history(sql->zHistory);
#endif
		oneshot++;
	    }

	    sql->ifd = Fdopen(fdDup(fileno(stdin)), "rb.fpio");
assert(sql->ifd);

sql->flags |= RPMSQL_FLAGS_PROMPT;
	    rc = rpmsqlInput(sql);
sql->flags &= ~RPMSQL_FLAGS_PROMPT;

	    if (sql->ifd) (void) Fclose(sql->ifd);
	    sql->ifd = _ifd;

            if (sql->zHistory) {
                stifle_history(100);
                write_history(sql->zHistory);
            }
	    if (_ofd == NULL)
		(void) Fclose(sql->ofd);
	    sql->ofd = _ofd;
	    sql->flags = _flags;
	    if (rc != 0) rc = RPMRC_FAIL;
	} else
	if (!strcmp(s, "-") || !strcmp(s, "stdin")) {		/* STDIN */
FD_t _ofd = sql->ofd;
SQLDBG((stderr, "*** %s: STDIN\n", __FUNCTION__));

if (sql->ofd == NULL) sql->ofd = fdDup(STDOUT_FILENO);
assert(sql->ofd);

assert(sql->ifd == NULL);
	    sql->ifd = Fdopen(fdDup(fileno(stdin)), "rb.fpio");
assert(sql->ifd);

	    rc = rpmsqlInput(sql);

	    if (sql->ifd) (void) Fclose(sql->ifd);
	    sql->ifd = NULL;

if (_ofd == NULL) (void) Fclose(sql->ofd);
	    sql->ofd = _ofd;

	    if (rc != 0) rc = RPMRC_FAIL;
	} else
	if (*s == '/') {				/* FILE */
	    FD_t _ifd = sql->ifd;
SQLDBG((stderr, "*** %s: FILE\n", __FUNCTION__));
	    sql->ifd = Fopen(s, "rb.fpio");
	    if (!(sql->ifd == NULL || Ferror(sql->ifd))) {
		rc = rpmsqlInput(sql);
	    }
	    if (sql->ifd) (void) Fclose(sql->ifd);
	    sql->ifd = _ifd;
	    if (rc != 0) rc = RPMRC_FAIL;
	} else {					/* STRING */
SQLDBG((stderr, "*** %s: STRING\n", __FUNCTION__));
	    if (*s == '.') {
		char * t = xstrdup(s);
		rc = rpmsqlMetaCommand(sql, t);
		t = _free(t);
	    } else {
		sqlite3 * db;
		char * zErrMsg = NULL;
		_rpmsqlOpenDB(sql);
		db = (sqlite3 *)sql->I;
		rc = _rpmsqlShellExec(sql, s, _rpmsqlShellCallback, &zErrMsg);
		if (zErrMsg) {
		    rpmsql_error(1, "%s", zErrMsg);
		    zErrMsg = _free(zErrMsg);
		    if (rc == 0) rc = RPMRC_FAIL;
		} else if (rc != 0) {
		    rpmsql_error(1, _("unable to process SQL \"%s\""), s);
		    rc = RPMRC_FAIL;
		}
	    }
	}
	    
	/* Return the SQL output. */
	if (sql->iob) {
	    (void) rpmiobRTrim(sql->iob);
SQLDBG((stderr, "==========>\n%s\n<==========\n", rpmiobStr(sql->iob)));
	    if (resultp)
		*resultp = rpmiobStr(sql->iob);		/* XXX strdup? */
	}

    }
#endif	/* WITH_SQLITE */

SQLDBG((stderr, "<== %s(%p,%p[%u]) rc %d\n", __FUNCTION__, sql, str, (unsigned)(str ? strlen(str) : 0), rc));

    return rc;
}
