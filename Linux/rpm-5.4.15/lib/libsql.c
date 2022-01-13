#include "system.h"

#define	_RPMIOB_INTERNAL	/* rpmiobSlurp */
#include <fts.h>
#include <mire.h>
#include <rpmdir.h>
#include <rpmmacro.h>
#include <rpmurl.h>

#define _RPMSQL_INTERNAL
#define _RPMVT_INTERNAL
#define _RPMVC_INTERNAL
#include <rpmsql.h>

#include <sqlite3.h>

#include <rpmtypes.h>
#define	_RPMTAG_INTERNAL
#include <rpmtag.h>

#include <rpmts.h>

#include <rpmgi.h>

#include "debug.h"

/*==============================================================*/

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

#ifdef	REFERENCE
enum rpmTagType_e {
    	/* RPM_NULL_TYPE =  0	- never been used. */
	/* RPM_CHAR_TYPE =  1	- never been used, same as RPM_UINT8_TYPE. */
    RPM_UINT8_TYPE		=  2,
    RPM_UINT16_TYPE		=  3,
    RPM_UINT32_TYPE		=  4,
    RPM_UINT64_TYPE		=  5,
    RPM_STRING_TYPE		=  6,
    RPM_BIN_TYPE		=  7,
    RPM_STRING_ARRAY_TYPE	=  8,
    RPM_I18NSTRING_TYPE		=  9
	/* RPM_ASN1_TYPE = 10	- never been used. */
	/* RPM_OPENPGP_TYPE= 11	- never been used. */
};
#define RPM_MIN_TYPE            2
#define RPM_MAX_TYPE            9
#define RPM_MASK_TYPE           0x0000ffff

static KEY tagTypes[] = {
    { "",		0 },
    { "char",		RPM_UINT8_TYPE },
    { "uint8",		RPM_UINT8_TYPE },
    { "uint16",		RPM_UINT16_TYPE },
    { "uint32",		RPM_UINT32_TYPE },
    { "uint64",		RPM_UINT64_TYPE },
    { "string",		RPM_STRING_TYPE },
    { "octets",		RPM_BIN_TYPE },
    { "argv",		RPM_STRING_ARRAY_TYPE },
    { "i18nstring",	RPM_I18NSTRING_TYPE }
};
#endif	/* REFERENCE */

static KEY tagTypes[] = {
    { "",		0 },
    { "INTEGER",	RPM_UINT8_TYPE },
    { "INTEGER",	RPM_UINT8_TYPE },
    { "INTEGER",	RPM_UINT16_TYPE },
    { "INTEGER",	RPM_UINT32_TYPE },
    { "INTEGER",	RPM_UINT64_TYPE },
    { "TEXT",		RPM_STRING_TYPE },
    { "BLOB",		RPM_BIN_TYPE },
    { "TEXT",		RPM_STRING_ARRAY_TYPE },
    { "TEXT",		RPM_I18NSTRING_TYPE },
};
#ifdef	UNUSED
static size_t ntagTypes = sizeof(tagTypes) / sizeof(tagTypes[0]);
#endif

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

/*==============================================================*/
static int _tag_debug;

static char * tagJoin(const char * a, const char ** argv, const char * z)
{
    static const char _type[] = " TEXT";
    const char ** av;
    size_t na = (sizeof("\t")-1) + (a ? strlen(a) : 0);
    size_t nb = 0;
    size_t nz = (z ? strlen(z) : 0) + strlen(_type) + (sizeof(",\n")-1);
    char *t, *te;

    for (av = argv; *av != NULL; av++)
	nb += na + strlen(*av) + nz;

    te = t = (char *) xmalloc(nb + 1);
    for (av = argv; *av != NULL; av++) {
	*te++ = '\t';
	te = stpcpy(te, a);
	te = stpcpy(te, *av);
	if (hasSqlType(*av) == NULL)
	    te = stpcpy(te, _type);
	te = stpcpy(te, z);
	*te++ = ',';
	*te++ = '\n';
    }
    *te = '\0';

    return t;
}

static char * tagAppendCols(rpmvt vt, const char ** av)
{
    char * h = tagJoin("", av, "");
    int xx = argvAppend(&vt->cols, av);
    char * u;
    char * hu;

    /* XXX no selector so cols mapped from vt->argv[3] */
    av = (const char **) (vt->argc > 3 ? &vt->argv[3] : vt->fields);
assert(av);
    u = tagJoin("", av, "");
    u[strlen(u)-2] = ' ';	/* XXX nuke the final comma */
    xx = argvAppend(&vt->cols, av);

#define	dbN	vt->argv[1]
#define	tblN	vt->argv[2]
    hu = rpmExpand("CREATE TABLE ", dbN, ".", tblN, " (\n", h, u, ");", NULL);
#undef	dbN
#undef	tblN

    u = _free(u);
    h = _free(h);

if (_tag_debug)
fprintf(stderr, "%s\n", hu);
    return hu;
}

static int tagLoadArgv(rpmvt vt, rpmvt * vtp)
{
    sqlite3 * db = (sqlite3 *) vt->db;
    rpmvd vd = vt->vd;

    const char * hu;

    int rc = SQLITE_OK;
    int xx;

SQLDBG((stderr, "--> %s(%p,%p)\n", __FUNCTION__, vt, vtp));
if (_tag_debug)
argvPrint("vt->argv", (ARGV_t)vt->argv, NULL);

    /* Set the columns in the schema. */
  { static const char * _empty[] = { NULL };
    hu = tagAppendCols(vt, _empty);
    rc = rpmsqlCmd(NULL, "declare_vtab", db,
			sqlite3_declare_vtab(db, hu));
    hu = _free(hu);
  }

  {
    headerTagTableEntry _rpmTagTable = rpmTagTable;
    const struct headerTagTableEntry_s * tbl;

#ifdef	NOTYET
    /* XXX this should use rpmHeaderFormats, but there are linkage problems. */
    headerSprintfExtension _rpmHeaderFormats = headerCompoundFormats;
    headerSprintfExtension exts = _rpmHeaderFormats;
    headerSprintfExtension ext;
    int extNum;
#endif

    for (tbl = _rpmTagTable; tbl && tbl->name; tbl++) {
	const char * name = tbl->name + (sizeof("RPMTAG_")-1);
	char tag[32];
	uint32_t tx = (tbl->type & RPM_MASK_TYPE);
	char attrs[32];
	char * t;

	snprintf(tag, sizeof(tag), "%d", tbl->val);
	snprintf(attrs, sizeof(attrs), "%d", (tbl->type >> 16));
assert(tx >= RPM_MIN_TYPE && tx <= RPM_MAX_TYPE);
	t = rpmExpand(tag, vd->split, attrs, vd->split, tagTypes[tx].k,
		vd->split, name, NULL);
	xx = argvAdd(&vt->av, t);
	t = _free(t);

    }

#ifdef	NOTYET
    for (ext = exts, extNum = 0; ext != NULL && ext->type != HEADER_EXT_LAST;
	ext = (ext->type == HEADER_EXT_MORE ? *ext->u.more : ext+1), extNum++)
    {
	if (ext->name == NULL || ext->type != HEADER_EXT_TAG)
	    continue;

	/* XXX don't print header tags twice. */
	if (tagValue(ext->name) > 0)
	    continue;
	fprintf(fp, "%s\n", ext->name + 7);
    }
#endif

  }

    vt->ac = argvCount((ARGV_t)vt->av);

if (_tag_debug)
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

SQLDBG((stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, vt, vtp, rc));

    return rc;
}

/*==============================================================*/

static struct rpmvd_s _tagVD = {
	.split	= "|",
	.parse	= "tag integer primary key|attrs integer|type|name",
};

static int tagCreate(void * _db, void * pAux,
                int argc, const char *const * argv,
                rpmvt * vtp, char ** pzErr)
{
    return tagLoadArgv(rpmvtNew(_db, pAux, argv, &_tagVD), vtp);
}

struct sqlite3_module tagModule = {
    .iVersion	= 0,
    .xCreate	= (void *) tagCreate,
    .xConnect	= (void *) tagCreate,
};

/*==============================================================*/
static int _xxx_debug = 0;

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

    te = t = (char *) xmalloc(nb + 1);
    for (av = argv; *av != NULL; av++) {
	*te++ = '\t';
	te = stpcpy(te, a);
	te = stpcpy(te, *av);
	if (hasSqlType(*av) == NULL)
	    te = stpcpy(te, _type);
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

    av = (const char **) (vt->argc > 4 ? &vt->argv[4] : vt->fields);
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

if (_xxx_debug)
fprintf(stderr, "%s\n", hu);
    return hu;
}

static int _rpmvtLoadArgv(rpmvt vt, rpmvt * vtp)
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
    int i;

if (_xxx_debug)
fprintf(stderr, "--> %s(%p,%p)\n", __FUNCTION__, vt, vtp);
if (_xxx_debug)
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

    if (fn[0] == '/') {
if (_xxx_debug)
fprintf(stderr, "*** uri %s fn %s\n", uri, fn);
	if (Glob_pattern_p(uri, 0)) {	/* XXX uri */
	    const char ** av = NULL;
	    int ac = 0;
		
	    xx = rpmGlob(uri, &ac, &av);	/* XXX uri */
if (_xxx_debug)
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
if (_xxx_debug)
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
if (_xxx_debug)
fprintf(stderr, "FILE: %d = Slurp(%s)\n", xx, uri);
	    if (!xx)
		xx = argvSplit(&vt->av, rpmiobStr(iob), "\n");
	    else
		rc = SQLITE_NOTFOUND;		/* XXX */
	    iob = rpmiobFree(iob);
	} else
	    rc = SQLITE_NOTFOUND;		/* XXX */
    } else
    if (!strcasecmp(vt->argv[0], "Env")) {
if (_xxx_debug)
fprintf(stderr, " ENV: getenv(%p[%d])\n", &vt->argv[3], argvCount(&vt->argv[3]));
	for (i = 3; i < vt->argc; i++) {
	    char * t = rpmExpand(vt->argv[i], "=", getenv(vt->argv[i]), NULL);
	    xx = argvAdd(&vt->av, t);
	    t = _free(t);
	}
    } else {
	xx = argvAppend(&vt->av, (ARGV_t)&vt->argv[3]);
if (_xxx_debug)
fprintf(stderr, "LIST: %d = Append(%p[%d])\n", xx, &vt->argv[3], argvCount(&vt->argv[3]));
    }

    vt->ac = argvCount((ARGV_t)vt->av);

    uri = _free(uri);

if (_xxx_debug)
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

if (_xxx_debug)
fprintf(stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, vt, vtp, rc);

    return rc;
}

/*==============================================================*/
static int hdrColumn(rpmvc vc, void * _pContext, int colx)
{
    sqlite3_context * pContext = (sqlite3_context *) _pContext;
    rpmvt vt = vc->vt;
    const char * path = vt->av[vc->ix];
    const char * col = vt->cols[colx];

    int xx;

    int rc = SQLITE_OK;

if (_xxx_debug < 0)
fprintf(stderr, "--> %s(%p,%p,%d)\n", __FUNCTION__, vc, pContext, colx);

    if (!strcmp(col, "path"))
	sqlite3_result_text(pContext, path, -1, SQLITE_STATIC);

    else {
	HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
	Header h = (Header) vt->_h;

assert(h);
	he->tag = tagValue(col);
	if ((xx = headerGet(h, he, 0)) == 0)
	    sqlite3_result_null(pContext);
	else {
if (_xxx_debug)
fprintf(stderr, "*** tag %u t %u p.ptr %p c %u\n", he->tag, he->t, he->p.ptr, he->c);
	    switch (he->t) {
	    default:
	    case RPM_UINT8_TYPE:
		sqlite3_result_int(pContext, he->p.ui8p[0]);
		break;
	    case RPM_UINT16_TYPE:
		sqlite3_result_int(pContext, he->p.ui16p[0]);
		break;
	    case RPM_UINT32_TYPE:
		sqlite3_result_int(pContext, he->p.ui32p[0]);
		break;
	    case RPM_UINT64_TYPE:
		sqlite3_result_int64(pContext, he->p.ui64p[0]);
		break;
	    case RPM_BIN_TYPE:
		sqlite3_result_blob(pContext, he->p.ptr, he->c, SQLITE_TRANSIENT);
		break;
	    case RPM_I18NSTRING_TYPE:
#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(0);
#endif
	    case RPM_STRING_TYPE:
		sqlite3_result_text(pContext, he->p.str, -1, SQLITE_TRANSIENT);
		break;
	    case RPM_STRING_ARRAY_TYPE:
		sqlite3_result_text(pContext, he->p.argv[0], -1, SQLITE_TRANSIENT);
		break;
	    }
	}
	he->p.ptr = _free(he->p.ptr);
    }

if (_xxx_debug < 0)
fprintf(stderr, "<-- %s(%p,%p,%d) rc %d\n", __FUNCTION__, vc, pContext, colx, rc);

    return rc;
}

/*==============================================================*/
static int hdrLoadNext(rpmvc vc)
{
    rpmvt vt = vc->vt;
    rpmgi gi = (rpmgi) vt->_gi;
    Header h = (Header) vt->_h;
    rpmRC rc;

    if (h) {
	(void) headerFree(h);
	h = NULL;
	vt->_h = h;
    }

    if (vc->ix >= 0 && vc->ix < vc->nrows) {
	rc = rpmgiNext(gi);
	h = rpmgiHeader(gi);
	vt->_h = headerLink(h);
    } else
	rc = RPMRC_NOTFOUND;

    return rc;
}

/*==============================================================*/

static int hdrFilter(rpmvc vc, int idxNum, const char * idxStr,
		int argc, rpmvArg * _argv)
{
    sqlite3_value ** argv = (sqlite3_value **) _argv;
    int rc = SQLITE_OK;

if (_xxx_debug)
fprintf(stderr, "--> %s(%p,%d,%s,%p[%u]) [%d:%d]\n", __FUNCTION__, vc, idxNum, idxStr, argv, (unsigned)argc, vc->ix, vc->nrows);

    if (vc->nrows > 0) {
	vc->ix = 0;
	if (hdrLoadNext(vc)) rc = SQLITE_NOTFOUND;	/* XXX */
    }

if (_xxx_debug)
fprintf(stderr, "<-- %s(%p,%d,%s,%p[%u]) [%d:%d] rc %d\n", __FUNCTION__, vc, idxNum, idxStr, argv, (unsigned)argc, vc->ix, vc->nrows, rc);

    return rc;
}

static int hdrNext(rpmvc vc)
{
    int rc = SQLITE_OK;

    if (vc->ix >= 0 && vc->ix < vc->nrows) {		/* XXX needed? */
	vc->ix++;
	if (hdrLoadNext(vc)) rc = SQLITE_NOTFOUND;	/* XXX */
rc = 0;
    }

if (!(vc->ix >= 0 && vc->ix < vc->nrows))
if (_xxx_debug)
fprintf(stderr, "<-- %s(%p) [%d:%d] rc %d\n", __FUNCTION__, vc, vc->ix, vc->nrows, rc);
    return rc;
}

static int hdrEof(rpmvc vc)
{
rpmvt vt = vc->vt;
    int rc = (vc->ix >= 0 && vc->ix < vc->nrows ? 0 : 1);
    
if (_xxx_debug)
fprintf(stderr, "<-- %s(%p) h %p rc %d\n", __FUNCTION__, vc, vt->_h, rc);
    return rc;
}

/*==============================================================*/

static struct rpmvd_s _hdrVD = {
	/* XXX where to map the default? */
	.prefix = "%{?_repodb}%{!?_repodb:/X/popt/}",
	.split  = "/|",
	.parse  = "Name|Epoch integer|Version|Release|Arch|Os|Providename|Provideversion|Provideflags|Sigmd5 blob|Nvra|Summary",
};

static int hdrCreate(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
    int rc = _rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_hdrVD), vtp);

    if (!rc && vtp) {
	rpmvt vt = *vtp;
	rpmts _ts = rpmtsCreate();
	int _tag = RPMDBI_ARGLIST;
	rpmgi _gi = rpmgiNew(_ts, _tag, NULL, 0);
	int _FtsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);
	int _giFlags = RPMGI_NONE;
	rpmRC rc = rpmgiSetArgs(_gi, vt->av, _FtsOpts, _giFlags);
rc = rc;
	vt->_ts = _ts;
	vt->_gi = _gi;
    }
    return rc;
}

static int hdrDestroy(rpmvt vt)
{
    vt->_h = headerFree(vt->_h);
    vt->_gi = rpmgiFree(vt->_gi);
    vt->_ts = rpmtsFree(vt->_ts);
    (void) rpmvtFree(vt);
    return 0;   /* SQLITE_OK */
}


struct sqlite3_module hdrModule = {
    .iVersion	= 0,
    .xCreate	= (void *) hdrCreate,
    .xConnect	= (void *) hdrCreate,
    .xColumn	= (void *) hdrColumn,
    .xFilter	= (void *) hdrFilter,
    .xNext	= (void *) hdrNext,
    .xEof	= (void *) hdrEof,
    .xDisconnect= (void *) hdrDestroy,
    .xDestroy	= (void *) hdrDestroy,
};

/*==============================================================*/

static struct rpmsqlVMT_s __VMT[] = {
  { "Rpmtags",		&tagModule, NULL },
  { "Header",		&hdrModule, NULL },
  { NULL,		NULL, NULL }
};

extern int sqlite3_extension_init(void * _db);
int sqlite3_extension_init(void * _db)
{
    int rc = 0;		/* SQLITE_OK */
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, _db);
    rc = _rpmsqlLoadVMT(_db, __VMT);
fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, _db, rc);
    return rc;
}
