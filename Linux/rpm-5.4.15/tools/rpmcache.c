/**
 * \file tools/rpmcache.c
 */

#include "system.h"
const char *__progname;

#include <fnmatch.h>
#include <fts.h>

#include <rpmio.h>
#include <rpmiotypes.h>
#include <poptIO.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include <rpmdb.h>

#include "rpmps.h"

#include "misc.h"	/* XXX rpmMkdirPath */

#define	_RPMGI_INTERNAL
#include <rpmgi.h>

#include <rpmcli.h>

#include "debug.h"


#ifdef __cplusplus

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

#else	/* __cplusplus */

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

static int _debug = 0;

/* XXX should be flag in ts */
static int noCache = -1;

static ARGV_t ftsSet;

const char * bhpath;
int bhpathlen = 0;
int bhlvl = -1;

struct ftsglob_s {
    const char ** patterns;
    int fnflags;
};

static struct ftsglob_s * bhglobs;
static int nbhglobs = 5;

static int indent = 2;

typedef struct Item_s {
    const char * path;
    uint32_t size;
    uint32_t mtime;
    rpmds this;
    Header h;
} * Item;

static Item * items = NULL;
static int nitems = 0;

static inline Item freeItem(Item item) {
    if (item != NULL) {
	item->path = _free(item->path);
	(void)rpmdsFree(item->this);
	item->this = NULL;
	(void)headerFree(item->h);
	item->h = NULL;
	item = _free(item);
    }
    return NULL;
}

static inline Item newItem(void) {
    Item item = xcalloc(1, sizeof(*item));
    return item;
}

static int cmpItem(const void * a, const void * b) {
    Item aitem = *(Item *)a;
    Item bitem = *(Item *)b;
    int rc = strcmp(rpmdsN(aitem->this), rpmdsN(bitem->this));
    return rc;
}

static void freeItems(void) {
    int i;
    for (i = 0; i < nitems; i++)
	items[i] = freeItem(items[i]);
    items = _free(items);
    nitems = 0;
}

static int ftsCachePrint(/*@unused@*/ rpmts ts, FILE * fp)
{
    int rc = 0;
    int i;

    if (fp == NULL) fp = stdout;
    for (i = 0; i < nitems; i++) {
	Item ip;

	ip = items[i];
	if (ip == NULL) {
	    rc = 1;
	    break;
	}

	fprintf(fp, "%s\n", ip->path);
    }
    return rc;
}

static int ftsCacheUpdate(rpmts ts)
{
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    uint32_t tid = rpmtsGetTid(ts);
    rpmmi mi;
    unsigned char * md5;
    int rc = 0;
    int xx;
    int i;

    rc = rpmtsCloseDB(ts);
    rc = rpmDefineMacro(NULL, "_dbpath %{_cache_dbpath}", RMIL_CMDLINE);
    rc = rpmtsOpenDB(ts, O_RDWR);
    if (rc != 0)
	return rc;

    for (i = 0; i < nitems; i++) {
	Item ip;

	ip = items[i];
	if (ip == NULL) {
	    rc = 1;
	    break;
	}

	/* --- Check that identical package is not already cached. */
	he->tag = RPMTAG_SIGMD5;
	xx = headerGet(ip->h, he, 0);
	md5 = he->p.ui8p;
 	if (!xx || md5 == NULL) {
	    md5 = _free(md5);
	    rc = 1;
	    break;
	}
        mi = rpmtsInitIterator(ts, RPMTAG_SIGMD5, md5, 16);
	md5 = _free(md5);
	rc = rpmmiCount(mi);
        mi = rpmmiFree(mi);
	if (rc) {
	    rc = 0;
	    continue;
	}

	/* --- Add cache tags to new cache header. */
	he->tag = RPMTAG_CACHECTIME;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &tid;
	he->c = 1;
	he->append = 1;
	rc = headerPut(ip->h, he, 0);
	he->append = 0;
	if (rc != 1) break;

	he->tag = RPMTAG_CACHEPKGPATH;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &ip->path;
	he->c = 1;
	he->append = 1;
	rc = headerPut(ip->h, he, 0);
	he->append = 0;
	if (rc != 1) break;

	he->tag = RPMTAG_CACHEPKGSIZE;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ip->size;
	he->c = 1;
	he->append = 1;
	rc = headerPut(ip->h, he, 0);
	he->append = 0;
	if (rc != 1) break;

	he->tag = RPMTAG_CACHEPKGMTIME;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ip->mtime;
	he->c = 1;
	he->append = 1;
	rc = headerPut(ip->h, he, 0);
	he->append = 0;
	if (rc != 1) break;

	/* --- Add new cache header to database. */
	if (!(rpmtsVSFlags(ts) & RPMVSF_NOHDRCHK))
	    rc = rpmdbAdd(rpmtsGetRdb(ts), tid, ip->h, ts);
	else
	    rc = rpmdbAdd(rpmtsGetRdb(ts), tid, ip->h, NULL);
	if (rc) break;

    }
    xx = rpmtsCloseDB(ts);
    return rc;
}

static rpmRC cacheStashLatest(rpmgi gi, Header h)
{
    FTSENT * fts = gi->fts;
    rpmds add = NULL;
    struct stat sb, * st;
    int ec = -1;	/* assume not found */
    int i = 0;
    int xx;

    rpmlog(RPMLOG_DEBUG, "============== %s\n", fts->fts_accpath);

    /* XXX DIEDIEDIE: check platform compatibility. */

    add = rpmdsThis(h, RPMTAG_REQUIRENAME, (RPMSENSE_EQUAL|RPMSENSE_LESS));

    if (items != NULL && nitems > 0) {
	Item needle = memset(alloca(sizeof(*needle)), 0, sizeof(*needle));
	Item * found, * fneedle = &needle;
	
	needle->this = add;

	found = bsearch(fneedle, items, nitems, sizeof(*found), cmpItem);

	/* Rewind to the first item with same name. */
	while (found > items && cmpItem(found-1, fneedle) == 0)
	    found--;

	/* Check that all saved items are newer than this item. */
	if (found != NULL)
	while (found < (items + nitems) && cmpItem(found, fneedle) == 0) {
	    ec = rpmdsCompare(needle->this, (*found)->this);
	    if (ec == 0) {
		found++;
		continue;
	    }
	    i = found - items;
	    break;
	}
    }

    /*
     * At this point, ec is
     *	-1	no item with the same name has been seen.
     *	0	item exists, but already saved item EVR is newer.
     *	1	item exists, but already saved item EVR is same/older.
     */
    if (ec == 0) {
	goto exit;
    } else if (ec == 1) {
	items[i] = freeItem(items[i]);
    } else {
	i = nitems++;
	items = xrealloc(items, nitems * sizeof(*items));
    }

    items[i] = newItem();
    items[i]->path = xstrdup(fts->fts_path);
    st = fts->fts_statp;
    if (st == NULL || ((long)st & 0xffff0000) == 0L) {
	st = &sb;
	memset(st, 0, sizeof(*st));
	xx = Stat(fts->fts_accpath, &sb);
    }

    if (st != NULL) {
	items[i]->size = st->st_size;
	items[i]->mtime = st->st_mtime;
    }
    st = NULL;
    items[i]->this = rpmdsThis(h, RPMTAG_PROVIDENAME, RPMSENSE_EQUAL);
    items[i]->h = headerLink(h);

    if (nitems > 1)
	qsort(items, nitems, sizeof(*items), cmpItem);

#if 0
    fprintf(stderr, "\t%*s [%d] %s\n",
		indent * (fts->fts_level < 0 ? 0 : fts->fts_level), "",
		i, fts->fts_name);
#endif

exit:
    (void)rpmdsFree(add);
    add = NULL;
    return (ec ? RPMRC_NOTFOUND : RPMRC_OK);
}

static const char * ftsInfoStrings[] = {
    "UNKNOWN",
    "D",
    "DC",
    "DEFAULT",
    "DNR",
    "DOT",
    "DP",
    "ERR",
    "F",
    "INIT",
    "NS",
    "NSOK",
    "SL",
    "SLNONE",
    "W",
};

static const char * ftsInfoStr(int fts_info) {
    if (!(fts_info >= 1 && fts_info <= 14))
	fts_info = 0;
    return ftsInfoStrings[ fts_info ];
}

static rpmRC cacheWalkPathFilter(rpmgi gi)
{
    FTS * ftsp = gi->ftsp;
    FTSENT * fts = gi->fts;
    struct ftsglob_s * bhg;
    const char ** patterns;
    const char * pattern;
    const char * s;
    int lvl;
    int xx;

    switch (fts->fts_info) {
    case FTS_D:		/* preorder directory */
	if (fts->fts_pathlen < bhpathlen)
	    break;

	/* Grab the level of the beehive top directory. */
	if (bhlvl < 0) {
	    if (fts->fts_pathlen == bhpathlen && !strcmp(fts->fts_path, bhpath))
		bhlvl = fts->fts_level;
	    else
		break;
	}
	lvl = fts->fts_level - bhlvl;

	if (lvl < 0)
	    break;

#if 0
	if (_debug)
	    fprintf(stderr, "FTS_%s\t%*s %s\n", ftsInfoStr(fts->fts_info),
		indent * (fts->fts_level < 0 ? 0 : fts->fts_level), "",
		fts->fts_name);
#endif

	/* Full path glob expression check. */
	bhg = bhglobs;

	if ((patterns = bhg->patterns) != NULL)
	while ((pattern = *patterns++) != NULL) {
	    if (*pattern == '/')
		xx = fnmatch(pattern, fts->fts_path, bhg->fnflags);
	    else
		xx = fnmatch(pattern, fts->fts_name, bhg->fnflags);
	    if (xx == 0)
		break;
	}

	/* Level specific glob expression check(s). */
	if (lvl == 0 || lvl >= nbhglobs)
	    break;
	bhg += lvl;

	if ((patterns = bhg->patterns) != NULL)
	while ((pattern = *patterns++) != NULL) {
	    if (*pattern == '/')
		xx = fnmatch(pattern, fts->fts_path, bhg->fnflags);
	    else
		xx = fnmatch(pattern, fts->fts_name, bhg->fnflags);
	    if (xx == 0)
		break;
	    else
		xx = Fts_set(ftsp, fts, FTS_SKIP);
	}

	break;
    case FTS_DP:	/* postorder directory */
#if 0
	if (_debug)
	    fprintf(stderr, "FTS_%s\t%*s %s\n", ftsInfoStr(fts->fts_info),
		indent * (fts->fts_level < 0 ? 0 : fts->fts_level), "",
		fts->fts_name);
#endif
	break;
    case FTS_F:		/* regular file */
#if 0
	if (_debug)
	    fprintf(stderr, "FTS_%s\t%*s %s\n", ftsInfoStr(fts->fts_info),
		indent * (fts->fts_level < 0 ? 0 : fts->fts_level), "",
		fts->fts_name);
#endif
	if (fts->fts_level >= 0) {
	    /* Ignore source packages. */
	    if (!strcmp(fts->fts_parent->fts_name, "SRPMS")) {
		xx = Fts_set(ftsp, fts->fts_parent, FTS_SKIP);
		break;
	    }
	}

	/* Ignore all but *.rpm files. */
	s = fts->fts_name + fts->fts_namelen + 1 - sizeof(".rpm");
	if (strcmp(s, ".rpm"))
	    break;

	break;
    case FTS_NS:	/* stat(2) failed */
    case FTS_DNR:	/* unreadable directory */
    case FTS_ERR:	/* error; errno is set */
	if (_debug)
	    fprintf(stderr, "FTS_%s\t%*s %s\n", ftsInfoStr(fts->fts_info),
		indent * (fts->fts_level < 0 ? 0 : fts->fts_level), "",
		fts->fts_name);
	break;
    case FTS_DC:	/* directory that causes cycles */
    case FTS_DEFAULT:	/* none of the above */
    case FTS_DOT:	/* dot or dot-dot */
    case FTS_INIT:	/* initialized only */
    case FTS_NSOK:	/* no stat(2) requested */
    case FTS_SL:	/* symbolic link */
    case FTS_SLNONE:	/* symbolic link without target */
    case FTS_W:		/* whiteout object */
    default:
	if (_debug)
	    fprintf(stderr, "FTS_%s\t%*s %s\n", ftsInfoStr(fts->fts_info),
		indent * (fts->fts_level < 0 ? 0 : fts->fts_level), "",
		fts->fts_name);
	break;
    }

    return RPMRC_OK;
}

/**
 * Initialize fts and glob structures.
 * @param ts		transaction set
 * @param argv		package names to match
 */
static void initGlobs(/*@unused@*/ rpmts ts, const char ** argv)
{
    char buf[BUFSIZ];
    int i;

    buf[0] = '\0';
    if (argv != NULL && * argv != NULL) {
	const char * arg;
	int single = (Glob_pattern_p(argv[0], 0) && argv[1] == NULL);
	char * t;

	t = buf;
	if (!single)
	    t = stpcpy(t, "@(");
	while ((arg = *argv++) != NULL) {
	    t = stpcpy(t, arg);
	    *t++ = '|';
	}
	t[-1] = (char)(single ? '\0' : ')');
	*t = '\0';
    }

    bhpath = rpmExpand("%{_bhpath}", NULL);
    bhpathlen = strlen(bhpath);

    ftsSet = xcalloc(2, sizeof(*ftsSet));
    ftsSet[0] = rpmExpand("%{_bhpath}", NULL);

    nbhglobs = 5;
    bhglobs = xcalloc(nbhglobs, sizeof(*bhglobs));
    for (i = 0; i < nbhglobs; i++) {
	const char * pattern;
	const char * macro;

	switch (i) {
	case 0:
	    macro = "%{_bhpath}";
	    break;
	case 1:
	    macro = "%{_bhcoll}";
	    break;
	case 2:
	    macro = (buf[0] == '\0' ? "%{_bhN}" : buf);
	    break;
	case 3:
	    macro = "%{_bhVR}";
	    break;
	case 4:
	    macro = "%{_bhA}";
	    break;
	default:
	    macro = NULL;
	    break;
	}
	bhglobs[i].patterns = xcalloc(2, sizeof(*bhglobs[i].patterns));
	if (macro == NULL)
	    continue;
	pattern = rpmExpand(macro, NULL);
	if (pattern == NULL || *pattern == '\0') {
	    pattern = _free(pattern);
	    continue;
	}
	bhglobs[i].patterns[0] = pattern;
	bhglobs[i].fnflags = (FNM_PATHNAME | FNM_PERIOD | FNM_EXTMATCH);
	if (bhglobs[i].patterns[0] != NULL)
	    rpmlog(RPMLOG_DEBUG, "\t%d \"%s\"\n",
		i, bhglobs[i].patterns[0]);
    }
}

static void freeGlobs(void)
{
    int i;
    for (i = 0; i < nbhglobs; i++) {
	bhglobs[i].patterns[0] = _free(bhglobs[i].patterns[0]);
	bhglobs[i].patterns = _free(bhglobs[i].patterns);
    }
    bhglobs = _free(bhglobs);
    ftsSet[0] = _free(ftsSet[0]);
    ftsSet = _free(ftsSet);
}

static rpmVSFlags vsflags = (rpmVSFlags) 0;

static struct poptOption optionsTable[] = {
 /* XXX FIXME: --nolegacy looks bogus */
 { "nolegacy", '\0', POPT_BIT_SET,      &vsflags, RPMVSF_NEEDPAYLOAD,
	N_("don't verify header+payload signature"), NULL },

 { "cache", '\0', POPT_ARG_VAL,   &noCache, 0,
	N_("update cache database"), NULL },
 { "nocache", '\0', POPT_ARG_VAL,   &noCache, -1,
	N_("don't update cache database, only print package paths"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioFtsPoptTable, 0,
        N_("File tree walk options:"),
        NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
	N_("Common options for all rpm modes and executables:"),
	NULL },

    POPT_AUTOALIAS
    POPT_AUTOHELP
    POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmcliInit(argc, argv, optionsTable);
    rpmts ts = NULL;
    rpmgi gi = NULL;
    const char * s;
    int ec = 1;
    rpmRC rpmrc;
    int xx;

    if (optCon == NULL)
        exit(EXIT_FAILURE);

    /* Configure the path to cache database, creating if necessary. */
    s = rpmExpand("%{?_cache_dbpath}", NULL);
    if (!(s && *s))
	rpmrc = RPMRC_FAIL;
    else
	rpmrc = rpmMkdirPath(s, "cache_dbpath");
    if (rpmrc == RPMRC_OK && Access(s, W_OK))
	rpmrc = RPMRC_FAIL;
    s = _free(s);
    if (rpmrc != RPMRC_OK) {
	fprintf(stderr, _("%s: %%{_cache_dbpath} macro is mis-configured.\n"),
		__progname);
        exit(EXIT_FAILURE);
    }

    ts = rpmtsCreate();

#if defined(SUPPORT_NOSIGNATURES)
    if (!QVA_ISSET(rpmcliQueryFlags, DIGEST)) {
	VSF_SET(vsflags, NOSHA1HEADER);
	VSF_SET(vsflags, NOMD5HEADER);
	VSF_SET(vsflags, NOSHA1);
	VSF_SET(vsflags, NOMD5);
    }
    if (!QVA_ISSET(rpmcliQueryFlags, SIGNATURE)) {
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);
    }
    if (!QVA_ISSET(rpmcliQueryFlags, HDRCHK)) {
	VSF_SET(vsflags, NOHDRCHK);
    }
    VSF_CLR(vsflags, NEEDPAYLOAD);	/* XXX needed? */
#endif

    (void) rpmtsSetVSFlags(ts, vsflags);

    {   uint32_t tid = (uint32_t) time(NULL);
	(void) rpmtsSetTid(ts, tid);
    }

    initGlobs(ts, poptGetArgs(optCon));

    gi = rpmgiNew(ts, RPMDBI_FTSWALK, NULL, 0);

    if (rpmioFtsOpts == 0)
	rpmioFtsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);

    if (noCache)
	rpmioFtsOpts |= FTS_NOSTAT;
    else
	rpmioFtsOpts &= ~FTS_NOSTAT;

    xx = rpmgiSetArgs(gi, ftsSet, rpmioFtsOpts, giFlags);

    gi->walkPathFilter = cacheWalkPathFilter;
    gi->stash = cacheStashLatest;
    while ((rpmrc = rpmgiNext(gi)) == RPMRC_OK)
	{};

    if (noCache)
	ec = ftsCachePrint(ts, stdout);
    else
	ec = ftsCacheUpdate(ts);
    if (ec) {
	fprintf(stderr, _("%s: cache operation failed: ec %d.\n"),
		__progname, ec);
    }

    freeItems();
    freeGlobs();

    gi = rpmgiFree(gi);
    (void)rpmtsFree(ts); 
    ts = NULL;
    optCon = rpmcliFini(optCon);

    return ec;
}
