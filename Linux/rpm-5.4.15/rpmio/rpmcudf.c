#include "system.h"

#define	_RPMIOB_INTERNAL	/* XXX necessary? */
#include <rpmiotypes.h>
#include <rpmmacro.h>
#include <argv.h>

#define _RPMCUDF_INTERNAL
#include "rpmcudf.h"

#include "debug.h"

/*@unchecked@*/
int _rpmcudf_debug = 0;

#ifdef	NOTYET
/*@unchecked@*/ /*@relnull@*/
rpmcudf _rpmcudfI = NULL;
#endif

/*==============================================================*/

#if defined(WITH_CUDF)

/* forward ref */
static void rpmcudvPrint(rpmcudf cudf, rpmcudv v, int free);

static rpmcudv rpmcudvFree(rpmcudv v)
{
    if (v == NULL)
	return NULL;
    switch (v->typ) {
    default :
	fprintf(stderr, "%s: unexpected type: %d\n", __FUNCTION__, v->typ);
assert(0);
	break;
    case RPMCUDV_NOTYPE:
	break;
    case RPMCUDV_INT:
    case RPMCUDV_POSINT:
    case RPMCUDV_NAT:
    case RPMCUDV_BOOL:
	break;
    case RPMCUDV_STRING:
    case RPMCUDV_PKGNAME:
    case RPMCUDV_IDENT:
    case RPMCUDV_ENUM:
	free(v->val.s);
	break;
    case RPMCUDV_VPKGFORMULA:
	cudf_free_vpkgformula(v->val.f);
	break;
    case RPMCUDV_VPKG:
    case RPMCUDV_VEQPKG:
	cudf_free_vpkg(v->val.vpkg);
	break;
    case RPMCUDV_VPKGLIST:
    case RPMCUDV_VEQPKGLIST:
	cudf_free_vpkglist(v->val.vpkgs);
	break;
    case RPMCUDV_TYPEDECL:
assert(0);	/* XXX unimplemented */
	break;

    case RPMCUDV_PACKAGE:
assert(0);	/* XXX unimplemented */
	break;
    case RPMCUDV_CUDFDOC:
	cudf_free_doc(v->val.doc);
	break;
    case RPMCUDV_CUDF:
	cudf_free_cudf(v->val.cudf);
	break;
    case RPMCUDV_EXTRA:
	g_hash_table_destroy(v->val.extra);
	break;
    }
    memset(v, 0, sizeof(*v));		/* trash & burn */
    return NULL;
}

static const char * relops[] = {
    "?0?",		/* 0 RELOP_NOP */
    "=",		/* 1 RELOP_EQ */
    "!=",		/* 2 RELOP_NEQ */
    ">=",		/* 3 RELOP_GEQ */
    ">",		/* 4 RELOP_GT */
    "<=",		/* 5 RELOP_LEQ */
    "<",		/* 6 RELOP_LT */
    "?7?",		/* 7 */
};

/* Print a package version predicate */
static
void print_vpkg(rpmcudf cudf, rpmcudv v)
{

assert(v->typ == RPMCUDV_VPKG || v->typ == RPMCUDV_VEQPKG);

    if (v->val.vpkg) {
	size_t nb = strlen(v->val.vpkg->name) + 5 + 64;
	char * b = alloca(nb);
	char * be = b;

	be = stpcpy(be, v->val.vpkg->name);
	if (v->val.vpkg->relop) {
	    snprintf(be, 5+64, " %s %d",
		relops[v->val.vpkg->relop & 0x7],
		v->val.vpkg->version);
	    be += strlen(be);
	}
	rpmiobAppend(cudf->iob, b, 0);
    }
}

/* Print a list of package predicates, separated by a given separator */
static
void print_vpkglist(rpmcudf cudf, rpmcudv v, const char * sep)
{
cudf_vpkglist_t l;
GList * last;

assert(v->typ == RPMCUDV_VPKGLIST || v->typ == RPMCUDV_VEQPKGLIST);

    for (l = v->val.vpkgs, last=g_list_last(l); l != NULL; l = g_list_next(l)) {
struct rpmcudv_s x;
x.typ = RPMCUDV_VPKG;
x.val.vpkg = g_list_nth_data(l, 0);
rpmcudvPrint(cudf, &x, 0);
	if (l != last)
	    rpmiobAppend(cudf->iob, sep, 0);
    }
}

/* Print a package formula */
static
void print_vpkgformula(rpmcudf cudf, rpmcudv v)
{
cudf_vpkgformula_t l;
GList * last;

assert(v->typ == RPMCUDV_VPKGFORMULA);

    for (l = v->val.f, last=g_list_last(l); l != NULL; l = g_list_next(l)) {
	struct rpmcudv_s x;
	x.typ = RPMCUDV_VPKGLIST;
	x.val.vpkg = g_list_nth_data(l, 0);
	/* XXX use rpmcudvPrint? */
	print_vpkglist(cudf, &x, " | ");
	if (l != last)
	    rpmiobAppend(cudf->iob, ", ", 0);
    }
}

/* Print a CUDF preamble */
static
void print_preamble(rpmcudf cudf)
{
    static char *props[] = {
	"preamble", "property", "univ-checksum", "status-checksum",
	"req-checksum", NULL
    };
    char ** prop;

rpmcudv v = &cudf->V;
assert(v->typ == RPMCUDV_CUDFDOC);
    if (v->val.doc->has_preamble)
    for (prop = props; *prop != NULL; prop++) {
	const char * s = cudf_pre_property(v->val.doc->preamble, *prop);
	char * t = rpmExpand("  ", *prop, ": ", s, NULL);
	rpmiobAppend(cudf->iob, t, 1);
	t = _free(t);
	s = _free(s);
    }
}

/* Print a CUDF request */
static
void print_request(rpmcudf cudf)
{
    static char *props[] = {
	"request", "install", "remove", "upgrade", NULL
    };
    char ** prop;

rpmcudv v = &cudf->V;
assert(v->typ == RPMCUDV_CUDFDOC);
    if (v->val.doc->has_request)
    for (prop = props; *prop != NULL; prop++) {
	const char * s = cudf_req_property(v->val.doc->request, *prop);
	char * t = rpmExpand("  ", *prop, ": ", s, NULL);
	rpmiobAppend(cudf->iob, t, 1);
	t = _free(t);
	s = _free(s);
    }
}

/* Print a possible value of the "keep" package property */
static
void print_keep(rpmcudf cudf, int keep)
{
    const char * s;
    switch (keep) {
    default :
	fprintf(stderr, "%s: unexpected keep value: %d\n", __FUNCTION__, keep);
assert(0);
	break;
    case KEEP_NONE:	s = "  keep: version";	break;
    case KEEP_VERSION:	s = "  keep: version";	break;
    case KEEP_PACKAGE:	s = "  keep: package";	break;
    case KEEP_FEATURE:	s = "  keep: feature";	break;
    }
    rpmiobAppend(cudf->iob, s, 1);
}

/* Print a generic property, i.e. a pair <name, typed value> */
static
void print_property(void * k, void * v, void * _cudf)
{
    rpmcudf cudf = _cudf;
    rpmiobAppend(cudf->iob, "  ", 0);
    rpmiobAppend(cudf->iob, (char *)k, 0);
    rpmiobAppend(cudf->iob, ": ", 0);
    rpmcudvPrint(cudf, v, 0);
    rpmiobAppend(cudf->iob, "", 1);
}

/* Print to stdout a set of extra properties */
#define print_extra(_cudf, e)	g_hash_table_foreach(e, print_property, _cudf)

static void rpmcudvPrint(rpmcudf cudf, rpmcudv v, int free)
{
    const char * s = NULL;
    char t[64];

    if (v == NULL)
	return;

    switch (v->typ) {
    default :
	fprintf(stderr, "%s: unexpected type: %d\n", __FUNCTION__, v->typ);
assert(0);
	break;
    case RPMCUDV_NOTYPE:
	break;
    case RPMCUDV_INT:
    case RPMCUDV_POSINT:
    case RPMCUDV_NAT:
	snprintf(t, sizeof(t), "%d", v->val.i);
	s = t;
	break;
    case RPMCUDV_BOOL:
	s = (v->val.i ? "true" : "false");
	break;
    case RPMCUDV_STRING:
    case RPMCUDV_PKGNAME:
    case RPMCUDV_IDENT:
    case RPMCUDV_ENUM:
	s = v->val.s;
	break;
    case RPMCUDV_VPKGFORMULA:
	print_vpkgformula(cudf, v);
if (free) rpmcudvFree(v);
	break;
    case RPMCUDV_VPKG:
    case RPMCUDV_VEQPKG:
	print_vpkg(cudf, v);
	break;
    case RPMCUDV_VPKGLIST:
    case RPMCUDV_VEQPKGLIST:
	print_vpkglist(cudf, v, ", ");
if (free) rpmcudvFree(v);
	break;
    case RPMCUDV_TYPEDECL:
assert(0);	/* XXX unimplemented */
	break;

    case RPMCUDV_PACKAGE:
assert(0);	/* XXX unimplemented */
	break;
    case RPMCUDV_CUDFDOC:
assert(0);	/* XXX unimplemented */
	break;
    case RPMCUDV_CUDF:
assert(0);	/* XXX unimplemented */
	break;
    case RPMCUDV_EXTRA:
	print_extra(cudf, v->val.extra);
if (free) rpmcudvFree(v);
	break;
    }
    if (s)
	rpmiobAppend(cudf->iob, s, 0);
}

static rpmcudp rpmcudpFree(rpmcudp cudp)
{
    if (cudp) {
	memset(cudp, 0, sizeof(*cudp));		/* trash & burn */
	free(cudp);
    }
    return NULL;
}

static rpmcudp rpmcudpNew(rpmcudf cudf)
{
    rpmcudp cudp = xcalloc(1, sizeof(*cudp));
    cudp->l = cudf->V.val.doc->packages;
    return cudp;
}

static rpmcudv rpmcudpNext(rpmcudp cudp)
{
    if (cudp == NULL || cudp->l == NULL)
	return NULL;
    cudp->V.typ = RPMCUDV_PACKAGE;
    cudp->V.val.pkg = (cudf_package_t) g_list_nth_data(cudp->l, 0);
    cudp->l = g_list_next(cudp->l);
    return &cudp->V;
}

static const char * rpmcudpName(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    return cudf_pkg_name(v->val.pkg);
}

/* XXX const char * return? */
static int rpmcudpVersion(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    return cudf_pkg_version(v->val.pkg);
}

static int rpmcudpInstalled(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    return cudf_pkg_installed(v->val.pkg);
}

static int rpmcudpWasInstalled(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    return cudf_pkg_was_installed(v->val.pkg);
}

static rpmcudv rpmcudpW(rpmcudp cudp, int typ, void * ptr)
{
rpmcudv w = &cudp->W;
    w->typ = typ;
    w->val.ptr = ptr;
    return w;
}

static rpmcudv rpmcudpDepends(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    return rpmcudpW(cudp, RPMCUDV_VPKGFORMULA, cudf_pkg_depends(v->val.pkg));
}

static rpmcudv rpmcudpConflicts(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    return rpmcudpW(cudp, RPMCUDV_VPKGLIST, cudf_pkg_conflicts(v->val.pkg));
}

static rpmcudv rpmcudpProvides(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    return rpmcudpW(cudp, RPMCUDV_VPKGLIST, cudf_pkg_provides(v->val.pkg));
}

static int rpmcudpKeep(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    /* XXX return rpmcudv instead? */
    return cudf_pkg_keep(v->val.pkg);
}

static rpmcudv rpmcudpExtra(rpmcudp cudp)
{
rpmcudv v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);
    return rpmcudpW(cudp, RPMCUDV_EXTRA, cudf_pkg_extra(v->val.pkg));
}

/* Print a CUDF package */
static
void print_package(rpmcudf cudf, rpmcudp cudp)
{
    char t[256];
rpmcudv v;

v = &cudp->V;
assert(v->typ == RPMCUDV_PACKAGE);

    snprintf(t, sizeof(t), "  package: %s", rpmcudpName(cudp));
    rpmiobAppend(cudf->iob, t, 1);

    snprintf(t, sizeof(t), "  version: %d", rpmcudpVersion(cudp));
    rpmiobAppend(cudf->iob, t, 1);

    snprintf(t, sizeof(t), "  installed: %s",
		rpmcudpInstalled(cudp) ? "true" : "false");
    rpmiobAppend(cudf->iob, t, 1);

    snprintf(t, sizeof(t), "  was-installed: %s",
		rpmcudpWasInstalled(cudp) ? "true" : "false");
    rpmiobAppend(cudf->iob, t, 1);

    rpmiobAppend(cudf->iob, "  depends: ", 0);
    rpmcudvPrint(cudf, rpmcudpDepends(cudp), 1);
    rpmiobAppend(cudf->iob, "", 1);

    rpmiobAppend(cudf->iob, "  conflicts: ", 0);
    rpmcudvPrint(cudf, rpmcudpConflicts(cudp), 1);
    rpmiobAppend(cudf->iob, "", 1);

    rpmiobAppend(cudf->iob, "  provides: ", 0);
    rpmcudvPrint(cudf, rpmcudpProvides(cudp), 1);
    rpmiobAppend(cudf->iob, "", 1);

    /* XXX use rpmcudvPrint? */
    print_keep(cudf, rpmcudpKeep(cudp));

    rpmcudvPrint(cudf, rpmcudpExtra(cudp), 0);

    rpmiobAppend(cudf->iob, "", 1);

}
#endif	/* WITH_CUDF */

/*==============================================================*/

static void rpmcudfFini(void * _cudf)
        /*@globals fileSystem @*/
        /*@modifies *_cudf, fileSystem @*/
{
    rpmcudf cudf = (rpmcudf) _cudf;

#if defined(WITH_CUDF)
    (void) rpmcudvFree(&cudf->V);
    memset(&cudf->V, 0, sizeof(cudf->V));
#endif
    (void)rpmiobFree(cudf->iob);
    cudf->iob = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmcudfPool;

static rpmcudf rpmcudfGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmcudfPool, fileSystem @*/
        /*@modifies pool, _rpmcudfPool, fileSystem @*/
{
    rpmcudf cudf;

    if (_rpmcudfPool == NULL) {
        _rpmcudfPool = rpmioNewPool("cudf", sizeof(*cudf), -1, _rpmcudf_debug,
                        NULL, NULL, rpmcudfFini);
        pool = _rpmcudfPool;
    }
    cudf = (rpmcudf) rpmioGetPool(pool, sizeof(*cudf));
    memset(((char *)cudf)+sizeof(cudf->_item), 0, sizeof(*cudf)-sizeof(cudf->_item));
    return cudf;
}

#ifdef	NOTYET
static rpmcudf rpmcudfI(void)
	/*@globals _rpmcudfI @*/
	/*@modifies _rpmcudfI @*/
{
    if (_rpmcudfI == NULL)
	_rpmcudfI = rpmcudfNew(NULL, 0);
    return _rpmcudfI;
}
#endif

rpmcudf rpmcudfNew(char ** av, uint32_t flags)
{
    rpmcudf cudf =
#ifdef	NOTYET
	(flags & 0x80000000) ? rpmcudfI() :
#endif
	rpmcudfGetPool(_rpmcudfPool);
    const char * fn = (av ? av[0] : NULL);
    int typ = flags;
    static int oneshot = 0;

    if (!oneshot) {
#if defined(WITH_CUDF)
	cudf_init();
#else
	fn = fn;
	typ = typ;
#endif
	oneshot++;
    }

if (_rpmcudf_debug)
fprintf(stderr, "==> %s(%p, %d) cudf %p\n", __FUNCTION__, av, flags, cudf);

#if defined(WITH_CUDF)
    if (fn != NULL)
    switch (typ) {
    default:
assert(0);
	break;
    case RPMCUDV_CUDFDOC:
	cudf->V.typ = typ;
	cudf->V.val.ptr = cudf_parse_from_file((char *)fn);
	break;
    case RPMCUDV_CUDF:
	cudf->V.typ = typ;
	cudf->V.val.ptr = cudf_load_from_file((char *)fn);
	break;
    }
#endif
    cudf->iob = rpmiobNew(0);
    return rpmcudfLink(cudf);
}

#ifdef	NOTYET
static const char * rpmcudfSlurp(const char * arg)
	/*@*/
{
    rpmiob iob = NULL;
    const char * val = NULL;
    struct stat sb;
    int xx;

    if (!strcmp(arg, "-")) {	/* Macros from stdin arg. */
	xx = rpmiobSlurp(arg, &iob);
    } else
    if ((arg[0] == '/' || strchr(arg, ' ') == NULL)
     && !Stat(arg, &sb)
     && S_ISREG(sb.st_mode)) {	/* Macros from a file arg. */
	xx = rpmiobSlurp(arg, &iob);
    } else {			/* Macros from string arg. */
	iob = rpmiobAppend(rpmiobNew(strlen(arg)+1), arg, 0);
    }

    val = xstrdup(rpmiobStr(iob));
    iob = rpmiobFree(iob);
    return val;
}
#endif

/*==============================================================*/

int rpmcudfHasPreamble(rpmcudf cudf)
{
    int rc = 0;
#if defined(WITH_CUDF)
    switch(cudf->V.typ) {
    default:			assert(0);	break;
    case RPMCUDV_NOTYPE:	break;
    case RPMCUDV_CUDFDOC:	rc = cudf->V.val.doc->has_preamble;	break;
    case RPMCUDV_CUDF:		rc = cudf->V.val.cudf->has_preamble;	break;
    }
#endif
    return rc;
}

int rpmcudfHasRequest(rpmcudf cudf)
{
    int rc = 0;
#if defined(WITH_CUDF)
    switch(cudf->V.typ) {
    default:			assert(0);	break;
    case RPMCUDV_NOTYPE:	break;
    case RPMCUDV_CUDFDOC:	rc = cudf->V.val.doc->has_request;	break;
    case RPMCUDV_CUDF:		rc = cudf->V.val.cudf->has_request;	break;
    }
#endif
    return rc;
}

int rpmcudfIsConsistent(rpmcudf cudf)
{
    int rc = 0;
#if defined(WITH_CUDF)
    switch(cudf->V.typ) {
    default:			assert(0);	break;
    case RPMCUDV_NOTYPE:	break;
    case RPMCUDV_CUDF:
	rc = cudf_is_consistent(cudf->V.val.cudf->universe);
	break;
    }
#endif
    return rc;
}

int rpmcudfInstalledSize(rpmcudf cudf)
{
    int rc = 0;
#if defined(WITH_CUDF)
    switch(cudf->V.typ) {
    default:			assert(0);	break;
    case RPMCUDV_NOTYPE:	break;
    case RPMCUDV_CUDF:
	rc = cudf_installed_size(cudf->V.val.cudf->universe);
	break;
    }
#endif
    return rc;
}

int rpmcudfUniverseSize(rpmcudf cudf)
{
    int rc = 0;
#if defined(WITH_CUDF)
    switch(cudf->V.typ) {
    default:			assert(0);	break;
    case RPMCUDV_NOTYPE:	break;
    case RPMCUDV_CUDF:
	rc = cudf_universe_size(cudf->V.val.cudf->universe);
	break;
    }
#endif
    return rc;
}

void rpmcudfPrintPreamble(rpmcudf cudf)
{
#if defined(WITH_CUDF)
    rpmiobAppend(cudf->iob, "Has preamble: ", 0);
    rpmiobAppend(cudf->iob, (rpmcudfHasPreamble(cudf) ? "yes" : "no"), 1);
    if (rpmcudfHasPreamble(cudf)) {
	rpmiobAppend(cudf->iob, "Preamble: ", 1);
	print_preamble(cudf);
	rpmiobAppend(cudf->iob, "", 1);
    }
#endif
}

void rpmcudfPrintRequest(rpmcudf cudf)
{
#if defined(WITH_CUDF)
    rpmiobAppend(cudf->iob, "Has request: ", 0);
    rpmiobAppend(cudf->iob, (rpmcudfHasRequest(cudf) ? "yes" : "no"), 1);
    if (rpmcudfHasRequest(cudf)) {
	rpmiobAppend(cudf->iob, "Request: ", 1);
	print_request(cudf);
	rpmiobAppend(cudf->iob, "", 1);
    }
#endif
}

void rpmcudfPrintUniverse(rpmcudf cudf)
{
#if defined(WITH_CUDF)
    if (cudf->V.typ == RPMCUDV_CUDFDOC) {
	rpmcudp cudp = rpmcudpNew(cudf);
	rpmiobAppend(cudf->iob, "Universe:", 1);
	while (rpmcudpNext(cudp) != NULL)
	    print_package(cudf, cudp);
	cudp = rpmcudpFree(cudp);
    }

    if (cudf->V.typ == RPMCUDV_CUDFDOC) {
	cudf_universe_t univ = cudf_load_universe(cudf->V.val.doc->packages);
	char t[256];

	snprintf(t, sizeof(t), "Universe size: %d/%d (installed/total)",
			cudf_installed_size(univ), cudf_universe_size(univ));
	rpmiobAppend(cudf->iob, t, 1);
	snprintf(t, sizeof(t), "Universe consistent: %s",
			cudf_is_consistent(univ) ?  "yes" : "no");
	rpmiobAppend(cudf->iob, t, 1);
	cudf_free_universe(univ);
    }
#endif
}

int rpmcudfIsSolution(rpmcudf X, rpmcudf Y)
{
    int rc = 0;
#if defined(WITH_CUDF)
assert(X->V.typ == RPMCUDV_CUDF);
assert(Y->V.typ == RPMCUDV_CUDF);
    rc = cudf_is_solution(X->V.val.cudf, Y->V.val.cudf->universe);
#endif
    return rc;
}
