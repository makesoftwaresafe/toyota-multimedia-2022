/*@-modfilesys@*/
/** \ingroup rpmgi
 * \file lib/rpmgi.c
 */
#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>		/* XXX fnpyKey */
#include <rpmcb.h>
#include <rpmmacro.h>		/* XXX rpmExpand */
#include <rpmtypes.h>
#include <rpmtag.h>
#include <rpmdb.h>

#include <rpmte.h>		/* XXX rpmElementType */
#include <pkgio.h>		/* XXX rpmElementType */

#define	_RPMGI_INTERNAL
#define	_RPMTS_INTERNAL		/* XXX ts->probs et al */
#include <rpmgi.h>

#include "manifest.h"

#include <rpmcli.h>	/* XXX rpmcliInstallFoo() */

#include "debug.h"

/*@access FD_t @*/		/* XXX void * arg */
/*@access fnpyKey @*/
/*@access rpmmi @*/
/*@access rpmts @*/
/*@access rpmps @*/

/**
 */
/*@unchecked@*/
int _rpmgi_debug = 0;

/**
 */
/*@unchecked@*/
rpmgiFlags giFlags = RPMGI_NONE;

/**
 */
/*@unchecked@*/
static int indent = 2;

/**
 */
/*@unchecked@*/ /*@observer@*/
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

/**
 */
/*@observer@*/
static const char * ftsInfoStr(int fts_info)
	/*@*/
{

    if (!(fts_info >= 1 && fts_info <= 14))
	fts_info = 0;
/*@-compmempass@*/
    return ftsInfoStrings[ fts_info ];
/*@=compmempass@*/
}

/**
 * Open a file after macro expanding path.
 * @todo There are two error messages printed on header, then manifest failures.
 * @param path		file path
 * @param fmode		open mode
 * @return		file handle
 */
/*@null@*/
static FD_t rpmgiOpen(const char * path, const char * fmode)
	/*@globals rpmGlobalMacroContext, h_errno, errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, h_errno, errno, internalState @*/
{
    const char * fn = rpmExpand(path, NULL);
    FD_t fd;

    /* FIXME (see http://rpm5.org/community/rpm-devel/0523.html) */
    errno = 0;
    fd = Fopen(fn, fmode);

    if (fd == NULL || Ferror(fd)) {
	rpmlog(RPMLOG_ERR, _("open of %s failed: %s\n"), fn, Fstrerror(fd));
	if (fd != NULL) (void) Fclose(fd);
	fd = NULL;
    }
    fn = _free(fn);

#if defined(POSIX_FADV_WILLNEED)
    if(fd != NULL)
    	(void) Fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);
#endif

    return fd;
}

/**
 * Load manifest into iterator arg list.
 * @param gi		generalized iterator
 * @param path		file path
 * @return		RPMRC_OK on success
 */
static rpmRC rpmgiLoadManifest(rpmgi gi, const char * path)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies gi, rpmGlobalMacroContext, h_errno, internalState @*/
{
    FD_t fd = rpmgiOpen(path, "r%{?_rpmgio}");
    rpmRC rpmrc = RPMRC_FAIL;

    if (fd != NULL) {
	rpmrc = rpmReadPackageManifest(fd, &gi->argc, &gi->argv);
	(void) Fclose(fd);
	switch (rpmrc) {
	case RPMRC_NOTFOUND:
	case RPMRC_FAIL:
	default:
	    gi->rc = rpmrc;
	    break;
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
	case RPMRC_NOSIG:
	case RPMRC_OK:
	    /* XXX manifest tried after *.rpm forces a reset. here? */
	    if (gi->rc == RPMRC_NOTFOUND)
		gi->rc = RPMRC_OK;
	    break;
	}
    } else {
	gi->rc = RPMRC_NOTFOUND;	/* XXX other failures? */
    }

    return rpmrc;
}

Header rpmgiReadHeader(rpmgi gi, const char * path)
{
    FD_t fd = rpmgiOpen(path, "r%{?_rpmgio}");
    Header h = NULL;

    if (fd != NULL) {
	/* XXX what if path needs expansion? */
	rpmRC rpmrc = rpmReadPackageFile(gi->ts, fd, path, &h);

	(void) Fclose(fd);

	switch (rpmrc) {
	case RPMRC_NOTFOUND:
	    /* XXX Read a package manifest. Restart ftswalk on success. */
	case RPMRC_FAIL:
	default:
	    (void)headerFree(h);
	    h = NULL;
	    gi->rc = rpmrc;
	    break;
	case RPMRC_NOSIG:		/* XXX FIXME */
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
	case RPMRC_OK:
	    break;
	}
    } else {
	gi->rc = RPMRC_NOTFOUND;	/* XXX other failures? */
    }

    return h;
}

/**
 * Load next key from argv list.
 * @param gi		generalized iterator
 * @return		RPMRC_OK on success
 */
static rpmRC rpmgiLoadNextKey(rpmgi gi)
	/*@modifies gi @*/
{
    rpmRC rpmrc = RPMRC_NOTFOUND;
    if (gi->argv != NULL && gi->argv[gi->i] != NULL) {
	gi->keyp = gi->argv[gi->i];
	gi->keylen = 0;
	rpmrc = RPMRC_OK;
    } else {
	gi->i = -1;
	gi->keyp = NULL;
	gi->keylen = 0;
    }
    return rpmrc;
}

/**
 * Read next header from package, lazily expanding manifests as found.
 * @todo An empty file read as manifest truncates argv returning RPMRC_NOTFOUND.
 * @todo Errors, e.g. non-existent path in manifest, will terminate iteration.
 * @todo Chained manifests lose an arg someplace.
 * @param gi		generalized iterator
 * @return		RPMRC_OK on success
 */
static rpmRC rpmgiLoadReadHeader(rpmgi gi)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies gi, rpmGlobalMacroContext, h_errno, internalState @*/
{
    rpmRC rpmrc = RPMRC_NOTFOUND;
    Header h = NULL;

    if (gi->argv != NULL && gi->argv[gi->i] != NULL)
    do {
	const char * fn;	/* XXX gi->hdrPath? */

	fn = gi->argv[gi->i];
	/* XXX Skip +bing -bang =boom special arguments. */
	if (strchr("-+=", *fn) == NULL && !(gi->flags & RPMGI_NOHEADER)) {
	    h = rpmgiReadHeader(gi, fn);
	    if (h != NULL)
		rpmrc = RPMRC_OK;
	    else
		rpmrc = (rpmRC) gi->rc;
	} else
	    rpmrc = RPMRC_OK;

	if (rpmrc == RPMRC_OK || gi->flags & RPMGI_NOMANIFEST)
	    break;
	if (rpmrc == RPMRC_NOSIG) {
	    /* XXX move error message to caller. */
	    rpmlog(RPMLOG_NOTICE, _("not signed: %s\n"), fn);
	    break;
	}
	if (errno == ENOENT)
	    break;

	/* Not a header, so try for a manifest. */
	gi->argv[gi->i] = NULL;		/* Mark the insertion point */
	rpmrc = rpmgiLoadManifest(gi, fn);
	/* XXX its unclear if RPMRC_NOTFOUND should fail or continue here. */
	if (rpmrc != RPMRC_OK) {
	    gi->argv[gi->i] = fn;	/* Manifest failed, restore fn */
	    break;
	}
	fn = _free(fn);
	rpmrc = RPMRC_NOTFOUND;
    } while (1);

    if (rpmrc == RPMRC_OK && h != NULL)
	gi->h = headerLink(h);
    (void)headerFree(h);
    h = NULL;

    return rpmrc;
}

/**
 * Filter file tree walk path.
 * @param gi		generalized iterator
 * @return		RPMRC_OK on success
 */
/*@null@*/
static rpmRC rpmgiWalkPathFilter(rpmgi gi)
	/*@*/
{
    FTSENT * fts = gi->fts;
    rpmRC rpmrc = RPMRC_NOTFOUND;
    const char * s;

if (_rpmgi_debug < 0)
rpmlog(RPMLOG_DEBUG, "FTS_%s\t%*s %s%s\n", ftsInfoStr(fts->fts_info),
		indent * (fts->fts_level < 0 ? 0 : fts->fts_level), "",
		fts->fts_name,
	((fts->fts_info == FTS_D || fts->fts_info == FTS_DP) ? "/" : ""));

    switch (fts->fts_info) {
    case FTS_D:		/* preorder directory */
	break;
    case FTS_DP:	/* postorder directory */
	break;
    case FTS_F:		/* regular file */
	if ((size_t)fts->fts_namelen <= sizeof(".rpm"))
	    break;
	/* Ignore all but *.rpm files. */
	s = fts->fts_name + fts->fts_namelen + 1 - sizeof(".rpm");
	if (strcmp(s, ".rpm"))
	    break;
	rpmrc = RPMRC_OK;
	break;
    case FTS_NS:	/* stat(2) failed */
    case FTS_DNR:	/* unreadable directory */
    case FTS_ERR:	/* error; errno is set */
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
	break;
    }
    return rpmrc;
}

/**
 * Read header from next package, lazily walking file tree.
 * @param gi		generalized iterator
 * @return		RPMRC_OK on success
 */
/*@null@*/
static rpmRC rpmgiWalkReadHeader(rpmgi gi)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies gi, rpmGlobalMacroContext, h_errno, internalState @*/
{
    rpmRC rpmrc = RPMRC_NOTFOUND;

    if (gi->ftsp != NULL)
    while ((gi->fts = Fts_read(gi->ftsp)) != NULL) {
	if (gi->walkPathFilter)
	    rpmrc = (*gi->walkPathFilter) (gi);
	else
	    rpmrc = rpmgiWalkPathFilter(gi);
	if (rpmrc == RPMRC_OK)
	    break;
    }

    if (rpmrc == RPMRC_OK) {
	Header h = NULL;
	if (!(gi->flags & RPMGI_NOHEADER)) {
	    /* XXX rpmrc = rpmgiLoadReadHeader(gi); */
	    if (gi->fts != NULL)	/* XXX can't happen */
		h = rpmgiReadHeader(gi, gi->fts->fts_path);
	}
	if (h != NULL) {
	    gi->h = headerLink(h);
	    (void)headerFree(h);
	    h = NULL;
/*@-noeffectuncon@*/
	    if (gi->stash != NULL)
		(void) (*gi->stash) (gi, gi->h);
/*@=noeffectuncon@*/
	}
    }

    return rpmrc;
}

const char * rpmgiEscapeSpaces(const char * s)
{
    const char * se;
    const char * t;
    char * te;
    size_t nb = 0;

    for (se = s; *se; se++) {
	if (isspace(*se))
	    nb++;
	nb++;
    }
    nb++;

    t = te = (char *) xmalloc(nb);
    for (se = s; *se; se++) {
	if (isspace(*se))
	    *te++ = '\\';
	*te++ = *se;
    }
    *te = '\0';
    return t;
}

/**
 * Append globbed arg list to iterator.
 * @param gi		generalized iterator
 * @param argv		arg list to be globbed (or NULL)
 * @returns		RPMRC_OK on success
 */
static rpmRC rpmgiGlobArgv(rpmgi gi, /*@null@*/ ARGV_t argv)
	/*@globals internalState @*/
	/*@modifies gi, internalState @*/
{
    const char * arg;
    rpmRC rpmrc = RPMRC_OK;
    int ac = 0;
    int xx;

    /* XXX Expand globs only if requested or for gi specific tags */
    if ((gi->flags & RPMGI_NOGLOB)
     || !(gi->tag == RPMDBI_HDLIST || gi->tag == RPMDBI_ARGLIST || gi->tag == RPMDBI_FTSWALK))
    {
	if (argv != NULL) {
	    while (argv[ac] != NULL)
		ac++;
/*@-nullstate@*/ /* XXX argv is not NULL */
	    xx = argvAppend(&gi->argv, argv);
/*@=nullstate@*/
	}
	gi->argc = ac;
	return rpmrc;
    }

    if (argv != NULL)
    while ((arg = *argv++) != NULL) {
	const char * t = rpmgiEscapeSpaces(arg);
	ARGV_t av = NULL;

	xx = rpmGlob(t, &ac, &av);
	xx = argvAppend(&gi->argv, av);
	gi->argc += ac;
	av = argvFree(av);
	t = _free(t);
	ac = 0;
    }
    return rpmrc;
}

/**
 * Return rpmdb match iterator with filters (if any) set.
 * @param gi		generalized iterator
 * @returns		RPMRC_OK on success
 */
static rpmRC rpmgiInitFilter(rpmgi gi)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies gi, rpmGlobalMacroContext, h_errno, internalState @*/
{
    rpmRC rpmrc = RPMRC_OK;
    ARGV_t av;
    int got = 0;

    gi->mi = rpmtsInitIterator(gi->ts, gi->tag, gi->keyp, gi->keylen);

if (_rpmgi_debug < 0)
fprintf(stderr, "*** gi %p key %p[%d]\tmi %p\n", gi, gi->keyp, (int)gi->keylen, gi->mi);

    if (gi->argv != NULL)
    for (av = (const char **) gi->argv; *av != NULL; av++) {
	if (gi->tag == RPMDBI_PACKAGES) {
	    int tag = RPMTAG_NAME;
	    const char * pat;
	    char * a, * ae;

	    pat = a = xstrdup(*av);
	    tag = RPMTAG_NAME;

	    /* Parse for "tag=pattern" args. */
	    if ((ae = strchr(a, '=')) != NULL) {
		*ae++ = '\0';
		if (*a != '\0') {	/* XXX HACK: permit '=foo' */
		    tag = tagValue(a);
#ifdef	DYING	/* XXX arbitrary tags always have a return value */
		    if (tag < 0) {
			rpmlog(RPMLOG_NOTICE, _("unknown tag: \"%s\"\n"), a);
			got = -1;
		    }
#endif
		}
		pat = ae;
		got++;
	    }
	    if (got) {
if (_rpmgi_debug  < 0)
fprintf(stderr, "\tav %p[%d]: \"%s\" -> %s ~= \"%s\"\n", gi->argv, (int)(av - gi->argv), *av, tagName((rpmTag)tag), pat);
		got = rpmmiAddPattern(gi->mi, (rpmTag)tag, RPMMIRE_DEFAULT, pat);
	    }
	    a = _free(a);
	}

	if (got >= 0)
	    continue;

	gi->mi = rpmmiFree(gi->mi);	/* XXX odd side effect? */
	rpmrc = RPMRC_FAIL;
	break;
    }

    return rpmrc;
}

/*@-mustmod@*/
static void rpmgiFini(void * _gi)
	/*@modifies _gi @*/
{
    rpmgi gi = (rpmgi) _gi;
    int xx;

    gi->hdrPath = _free(gi->hdrPath);
    (void)headerFree(gi->h);
    gi->h = NULL;

    gi->argv = argvFree(gi->argv);

    if (gi->ftsp != NULL) {
	xx = Fts_close(gi->ftsp);
	gi->ftsp = NULL;
	gi->fts = NULL;
    }
    if (gi->fd != NULL) {
	xx = Fclose(gi->fd);
	gi->fd = NULL;
    }
    gi->tsi = rpmtsiFree(gi->tsi);
    gi->mi = rpmmiFree(gi->mi);
    (void)rpmtsFree(gi->ts); 
    gi->ts = NULL;
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmgiPool;

static rpmgi rpmgiGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmgiPool, fileSystem, internalState @*/
	/*@modifies pool, _rpmgiPool, fileSystem, internalState @*/
{
    rpmgi gi;

    if (_rpmgiPool == NULL) {
	_rpmgiPool = rpmioNewPool("gi", sizeof(*gi), -1, _rpmgi_debug,
			NULL, NULL, rpmgiFini);
	pool = _rpmgiPool;
    }
    gi = (rpmgi) rpmioGetPool(pool, sizeof(*gi));
    memset(((char *)gi)+sizeof(gi->_item), 0, sizeof(*gi)-sizeof(gi->_item));
    return gi;
}

rpmgi rpmgiNew(rpmts ts, int tag, const void * keyp, size_t keylen)
{
    rpmgi gi = rpmgiGetPool(_rpmgiPool);

    if (gi == NULL)	/* XXX can't happen */
	return NULL;

/*@-assignexpose -castexpose @*/
    gi->ts = rpmtsLink(ts, "rpmgiNew");
/*@=assignexpose =castexpose @*/
    gi->tsOrder = rpmcliInstallOrder;
    gi->tag = (rpmTag) tag;
/*@-assignexpose@*/
    gi->keyp = keyp;
/*@=assignexpose@*/
    gi->keylen = keylen;

    gi->flags = (rpmgiFlags)0;
    gi->active = 0;
    gi->i = -1;
    gi->hdrPath = NULL;
    gi->h = NULL;
    gi->rc = 0;

    gi->tsi = NULL;
    gi->mi = NULL;
    gi->fd = NULL;
    gi->argv = (ARGV_t) xcalloc(1, sizeof(*gi->argv));
    gi->argc = 0;
    gi->ftsOpts = 0;
    gi->ftsp = NULL;
    gi->fts = NULL;
    gi->walkPathFilter = NULL;
    gi->stash = NULL;

    return rpmgiLink(gi, "rpmgiNew");
}

/*@observer@*/ /*@unchecked@*/
static const char * _query_hdlist_path  = "/usr/share/comps/%{_arch}/hdlist";

rpmRC rpmgiNext(/*@null@*/ rpmgi gi)
{
    char hnum[32];
    rpmRC rpmrc = RPMRC_NOTFOUND;
    int xx;

    if (gi == NULL)
	return rpmrc;

if (_rpmgi_debug)
fprintf(stderr, "--> %s(%p) tag %s\n", __FUNCTION__, gi, tagName(gi->tag));

    /* Free header from previous iteration. */
    (void)headerFree(gi->h);
    gi->h = NULL;
    gi->hdrPath = _free(gi->hdrPath);
    hnum[0] = '\0';

    if (++gi->i >= 0)
    switch (gi->tag) {
    default:
	if (!gi->active) {
nextkey:
	    rpmrc = rpmgiLoadNextKey(gi);
	    if (rpmrc != RPMRC_OK)
		goto enditer;
	    rpmrc = rpmgiInitFilter(gi);
	    if (rpmrc != RPMRC_OK || gi->mi == NULL) {
		gi->mi = rpmmiFree(gi->mi);	/* XXX unnecessary */
		gi->i++;
		goto nextkey;
	    }
	    rpmrc = RPMRC_NOTFOUND;	/* XXX hack */
	    gi->active = 1;
	}
	if (gi->mi != NULL) {	/* XXX unnecessary */
	    Header h = rpmmiNext(gi->mi);
	    if (h != NULL) {
		if (!(gi->flags & RPMGI_NOHEADER))
		    gi->h = headerLink(h);
		/* XXX use h->origin instead. */
		sprintf(hnum, "%u", (unsigned)rpmmiInstance(gi->mi));
		gi->hdrPath = rpmExpand("rpmdb h# ", hnum, NULL);
		rpmrc = RPMRC_OK;
		/* XXX header reference held by iterator, so no headerFree */
	    }
	}
	if (rpmrc != RPMRC_OK) {
	    gi->mi = rpmmiFree(gi->mi);
	    goto nextkey;
	}
	break;
    case RPMDBI_PACKAGES:
	if (!gi->active) {
	    rpmrc = rpmgiInitFilter(gi);
	    if (rpmrc != RPMRC_OK) {
		gi->mi = rpmmiFree(gi->mi);	/* XXX unnecessary */
		goto enditer;
	    }
	    rpmrc = RPMRC_NOTFOUND;	/* XXX hack */
	    gi->active = 1;
	}
	if (gi->mi != NULL) {	/* XXX unnecessary */
	    Header h = rpmmiNext(gi->mi);
	    if (h != NULL) {
		if (!(gi->flags & RPMGI_NOHEADER))
		    gi->h = headerLink(h);
		/* XXX use h->origin instead. */
		sprintf(hnum, "%u", (unsigned)rpmmiInstance(gi->mi));
		gi->hdrPath = rpmExpand("rpmdb h# ", hnum, NULL);
		rpmrc = RPMRC_OK;
		/* XXX header reference held by iterator, so no headerFree */
	    }
	}
	if (rpmrc != RPMRC_OK) {
	    gi->mi = rpmmiFree(gi->mi);
	    goto enditer;
	}
	break;
    case RPMDBI_REMOVED:
    case RPMDBI_ADDED:
    {	rpmte p;
	rpmElementType teType = (rpmElementType)0;
	const char * teTypeString = NULL;

	if (!gi->active) {
	    gi->tsi = rpmtsiInit(gi->ts);
	    gi->active = 1;
	}
	if ((p = rpmtsiNext(gi->tsi, teType)) != NULL) {
	    Header h = rpmteHeader(p);
	    if (h != NULL)
		if (!(gi->flags & RPMGI_NOHEADER)) {
		    gi->h = headerLink(h);
		switch(rpmteType(p)) {
		case TR_ADDED:	teTypeString = "+++";	/*@switchbreak@*/break;
		case TR_REMOVED: teTypeString = "---";	/*@switchbreak@*/break;
		}
		sprintf(hnum, "%u", (unsigned)gi->i);
		gi->hdrPath = rpmExpand("%s h# ", teTypeString, hnum, NULL);
		rpmrc = RPMRC_OK;
		(void)headerFree(h);
		h = NULL;
	    }
	}
	if (rpmrc != RPMRC_OK) {
	    gi->tsi = rpmtsiFree(gi->tsi);
	    goto enditer;
	}
    }	break;
    case RPMDBI_HDLIST:
	if (!gi->active) {
	    const char * path = rpmExpand("%{?_query_hdlist_path}", NULL);
	    if (path == NULL || *path == '\0') {
		path = _free(path);
		path = rpmExpand(_query_hdlist_path, NULL);
	    }
	    gi->fd = rpmgiOpen(path, "rm%{?_rpmgio}");
	    gi->active = 1;
	    path = _free(path);
	}
	if (gi->fd != NULL) {
	    Header h = NULL;
	    const char item[] = "Header";
	    const char * msg = NULL;
/*@+voidabstract@*/
	    rpmrc = rpmpkgRead(item, gi->fd, &h, &msg);
/*@=voidabstract@*/
	    switch(rpmrc) {
		default:
		    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", "rpmpkgRead", item, msg);
		    /*@fallthrough@*/
		case RPMRC_NOTFOUND:
		    h = NULL;
		    /*@fallthrough@*/
		case RPMRC_OK:
		    break;
	    }
	    msg = _free(msg);
	    if (h != NULL) {
		if (!(gi->flags & RPMGI_NOHEADER))
		    gi->h = headerLink(h);
		sprintf(hnum, "%u", (unsigned)gi->i);
		gi->hdrPath = rpmExpand("hdlist h# ", hnum, NULL);
		rpmrc = RPMRC_OK;
		(void)headerFree(h);
		h = NULL;
	    }
	}
	if (rpmrc != RPMRC_OK) {
	    if (gi->fd != NULL) (void) Fclose(gi->fd);
	    gi->fd = NULL;
	    goto enditer;
	}
	break;
    case RPMDBI_ARGLIST:
	/* XXX gi->active initialize? */
if (_rpmgi_debug  < 0)
fprintf(stderr, "*** gi %p\t%p[%d]: %s\n", gi, gi->argv, gi->i, gi->argv[gi->i]);
	/* Read next header, lazily expanding manifests as found. */
	rpmrc = rpmgiLoadReadHeader(gi);

	if (rpmrc != RPMRC_OK)	/* XXX check this */
	    goto enditer;

	gi->hdrPath = xstrdup(gi->argv[gi->i]);
	break;
    case RPMDBI_FTSWALK:
	if (gi->argv == NULL || gi->argv[0] == NULL)		/* HACK */
	    goto enditer;

	if (!gi->active) {
	    gi->ftsp = Fts_open((char *const *)gi->argv, gi->ftsOpts, NULL);
	    /* XXX NULL with open(2)/malloc(3) errno set */
	    gi->active = 1;
	}

	/* Read next header, lazily walking file tree. */
	rpmrc = rpmgiWalkReadHeader(gi);

	if (rpmrc != RPMRC_OK) {
	    xx = Fts_close(gi->ftsp);
	    gi->ftsp = NULL;
	    goto enditer;
	}

	if (gi->fts != NULL)
	    gi->hdrPath = xstrdup(gi->fts->fts_path);
	break;
    }

    if ((gi->flags & RPMGI_TSADD) && gi->h != NULL) {
	/* XXX rpmgi hack: Save header in transaction element. */
	if (gi->flags & RPMGI_ERASING) {
	    uint32_t hdrNum = headerGetInstance(gi->h);
	    xx = rpmtsAddEraseElement(gi->ts, gi->h, hdrNum);
	} else
	    xx = rpmtsAddInstallElement(gi->ts, gi->h, (fnpyKey)gi->hdrPath, 2, NULL);
    }
    goto exit;

enditer:
    if (gi->flags & RPMGI_TSORDER) {
	rpmts ts = gi->ts;

	/* Block access to indices used for depsolving. */
	if (!(gi->flags & RPMGI_ERASING)) {
	    (void) rpmtsSetGoal(ts, TSM_INSTALL);
	    xx = rpmdbBlockDBI(rpmtsGetRdb(ts), -RPMDBI_DEPCACHE);
	    xx = rpmdbBlockDBI(rpmtsGetRdb(ts), -RPMTAG_BASENAMES);
	    xx = rpmdbBlockDBI(rpmtsGetRdb(ts), -RPMTAG_PROVIDENAME);
	} else {
	    (void) rpmtsSetGoal(ts, TSM_ERASE);
	}

	/* XXX query/verify will need the glop added to a buffer instead. */
	xx = rpmcliInstallCheck(ts);
	xx = rpmcliInstallSuggests(ts);

	/* Permit access to indices used for depsolving. */
	if (!(gi->flags & RPMGI_ERASING)) {
	    xx = rpmdbBlockDBI(rpmtsGetRdb(ts), RPMTAG_PROVIDENAME);
	    xx = rpmdbBlockDBI(rpmtsGetRdb(ts), RPMTAG_BASENAMES);
	    xx = rpmdbBlockDBI(rpmtsGetRdb(ts), RPMDBI_DEPCACHE);
	}

	/* XXX Display dependency loops with rpm -qvT. */
	if (rpmIsVerbose()) {
	    rpmdepFlags _depflags = (rpmdepFlags)
	    	(rpmtsDFlags(ts) | RPMDEPS_FLAG_DEPLOOPS);
	    (void) rpmtsSetDFlags(ts, _depflags);
	}

	xx = (*gi->tsOrder) (ts);

	/* XXX hackery alert! */
	gi->tag = (!(gi->flags & RPMGI_ERASING) ? RPMDBI_ADDED : RPMDBI_REMOVED);
	gi->flags = (rpmgiFlags) (gi->flags & ~(RPMGI_TSADD|RPMGI_TSORDER));

    }

    (void)headerFree(gi->h);
    gi->h = NULL;
    gi->hdrPath = _free(gi->hdrPath);
    gi->i = -1;
    gi->active = 0;

exit:
if (_rpmgi_debug)
fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, gi, rpmrc);
    return rpmrc;
}

rpmgiFlags rpmgiGetFlags(rpmgi gi)
{
    return (gi != NULL ? gi->flags : RPMGI_NONE);
}

const char * rpmgiHdrPath(rpmgi gi)
{
    return (gi != NULL ? gi->hdrPath : NULL);
}

Header rpmgiHeader(rpmgi gi)
{
/*@-compdef -refcounttrans -retexpose -usereleased@*/
    return (gi != NULL ? gi->h : NULL);
/*@=compdef =refcounttrans =retexpose =usereleased@*/
}

rpmts rpmgiTs(rpmgi gi)
{
/*@-compdef -refcounttrans -retexpose -usereleased@*/
    return (gi != NULL ? gi->ts : NULL);
/*@=compdef =refcounttrans =retexpose =usereleased@*/
}

int rpmgiRc(rpmgi gi)
{
    return (gi != NULL ? gi->rc : RPMRC_OK);
}

rpmRC rpmgiSetArgs(rpmgi gi, ARGV_t argv, int ftsOpts, rpmgiFlags flags)
{
    if (gi == NULL) return RPMRC_FAIL;
    gi->ftsOpts = ftsOpts;
    gi->flags = flags;
    return rpmgiGlobArgv(gi, argv);
}

/*@=modfilesys@*/
