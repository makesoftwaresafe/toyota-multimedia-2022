/** \ingroup rpmfi
 * \file lib/rpmfi.c
 * Routines to handle file info tag sets.
 */

#include "system.h"

#include <rpmiotypes.h>	/* XXX fnpyKey */
#include <rpmio.h>
#include <rpmlog.h>
#include <rpmbf.h>
#include <rpmurl.h>	/* XXX urlGetPath */
#define	_RPMDIR_INTERNAL
#include <rpmdir.h>
#include <rpmmacro.h>	/* XXX rpmCleanPath */
#include <ugid.h>

#define	_RPMAV_INTERNAL	/* XXX avOpendir */
#include <rpmdav.h>

#include <rpmtypes.h>
#include <rpmtag.h>

#define	_FPRINT_INTERNAL
#include "fprint.h"

#define	_IOSM_INTERNAL
#define	_RPMFI_INTERNAL
#include "fsm.h"	/* XXX newFSM() */
#include "legacy.h"	/* XXX dodigest */

#include "rpmds.h"

#define	_RPMTE_INTERNAL	/* relocations */
#include "rpmte.h"
#include "rpmts.h"

#include <rpmcli.h>	/* XXX rpmHeaderFormats */

#include "debug.h"

/*@access IOSM_t @*/	/* XXX cast */

/*@access rpmte @*/
/*@access rpmts @*/	/* XXX cast */

/*@access FSM_t @*/	/* XXX fsm->repackaged */
/*@access DIR @*/

#ifdef __cplusplus
GENfree(int *)
GENpair(rpmuint16_t *)
GENpair(rpmuint32_t *)
GENfree(rpmRelocation)
#endif	/* __cplusplus */

/**
 */
struct rpmRelocation_s {
/*@only@*/ /*@null@*/
    const char * oldPath;	/*!< NULL here evals to RPMTAG_DEFAULTPREFIX, */
/*@only@*/ /*@null@*/
    const char * newPath;	/*!< NULL means to omit the file completely! */
};

/*@unchecked@*/
int _rpmfi_debug = 0;

/**
 * Remove occurrences of trailing character from string.
 * @param s		string
 * @param c		character to strip
 * @return 		string
 */
static /*@only@*/
char * stripTrailingChar(/*@only@*/ char * s, char c)
	/*@modifies *s */
{
    char * t;
/*@-boundswrite@*/
    for (t = s + strlen(s) - 1; *t == c && t >= s; t--)
	*t = '\0';
/*@=boundswrite@*/
    return s;
}

int rpmfiFC(rpmfi fi)
{
    return (fi != NULL ? fi->fc : 0);
}

int rpmfiDC(rpmfi fi)
{
    return (fi != NULL ? fi->dc : 0);
}

#ifdef	NOTYET
int rpmfiDI(rpmfi fi)
{
}
#endif

int rpmfiFX(rpmfi fi)
{
    return (fi != NULL ? fi->i : -1);
}

int rpmfiSetFX(rpmfi fi, int fx)
{
    int i = -1;

    if (fi != NULL && fx >= 0 && fx < (int)fi->fc) {
	i = fi->i;
	fi->i = fx;
	fi->j = fi->dil[fi->i];
    }
    return i;
}

int rpmfiDX(rpmfi fi)
{
    return (fi != NULL ? fi->j : -1);
}

int rpmfiSetDX(rpmfi fi, int dx)
{
    int j = -1;

    if (fi != NULL && dx >= 0 && dx < (int)fi->dc) {
	j = fi->j;
	fi->j = dx;
    }
    return j;
}

int rpmfiIsSource(rpmfi fi)
{
    return (fi != NULL ? fi->isSource : 0);
}

const char * rpmfiBN(rpmfi fi)
{
    const char * BN = NULL;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->bnl != NULL)
	    BN = fi->bnl[fi->i];
    }
    return BN;
}

const char * rpmfiDN(rpmfi fi)
{
    const char * DN = NULL;

    if (fi != NULL && fi->j >= 0 && fi->j < (int)fi->dc) {
	if (fi->dnl != NULL)
	    DN = fi->dnl[fi->j];
    }
    return DN;
}

const char * rpmfiFN(rpmfi fi)
{
    const char * FN = "";

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	const char *dn;
	char * t;
	if (fi->fn == NULL)
	    fi->fn = (char *) xmalloc(fi->fnlen + 1);
	FN = t = fi->fn;
	(void) urlPath(fi->dnl[fi->dil[fi->i]], &dn);
	*t = '\0';
	t = stpcpy(t, dn);
	t = stpcpy(t, fi->bnl[fi->i]);
    }
    return FN;
}

void * rpmfiFNBF(rpmfi fi)
{
    void * _fnbf = NULL;
    if (fi != NULL) {
	if (fi->_fnbf == NULL) {
	    char * fn = (char *) alloca(fi->fnlen + 1);
	    static double e = 1.0e-4;
	    size_t n = (fi->fc > 10 ? fi->fc : 10);
	    size_t m = 0;
	    size_t k = 0;
	    rpmbf bf;
	    int i;

	    rpmbfParams(n, e, &m, &k);
	    bf = rpmbfNew(m, k, 0);
	    for (i = 0; i < (int)fi->fc; i++) {
		const char * dn;
		int xx;
		dn = NULL;
		(void) urlPath(fi->dnl[fi->dil[i]], &dn);
		dn = stpcpy(stpcpy(fn, dn), fi->bnl[i]);
		xx = rpmbfAdd(bf, fn, (size_t)(dn - fn));
assert(xx == 0);
	    }
	    fi->_fnbf = bf;
	}
	_fnbf = fi->_fnbf;
    }
    return _fnbf;
}

size_t rpmfiFNMaxLen(rpmfi fi)
{
    return (fi ? fi->fnlen : 0);
}

rpmuint32_t rpmfiFFlags(rpmfi fi)
{
    rpmuint32_t FFlags = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fflags != NULL)
	    FFlags = fi->fflags[fi->i];
    }
    return FFlags;
}

rpmuint32_t rpmfiSetFFlags(rpmfi fi, rpmuint32_t FFlags)
{
    rpmuint32_t oFFlags = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fflags != NULL && fi->h == NULL) {
	    oFFlags = fi->fflags[fi->i];
	    *((rpmuint32_t *)(fi->fflags + fi->i)) = FFlags;
	}
    }
    return oFFlags;
}

rpmuint32_t rpmfiVFlags(rpmfi fi)
{
    rpmuint32_t VFlags = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->vflags != NULL)
	    VFlags = fi->vflags[fi->i];
    }
    return VFlags;
}

rpmuint32_t rpmfiSetVFlags(rpmfi fi, rpmuint32_t VFlags)
{
    rpmuint32_t oVFlags = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->vflags != NULL && fi->h == NULL) {
	    oVFlags = fi->vflags[fi->i];
	    *((rpmuint32_t *)(fi->vflags + fi->i)) = VFlags;
	}
    }
    return oVFlags;
}

rpmuint16_t rpmfiFMode(rpmfi fi)
{
    rpmuint16_t fmode = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fmodes != NULL)
	    fmode = fi->fmodes[fi->i];
    }
    return fmode;
}

rpmfileState rpmfiFState(rpmfi fi)
{
    rpmfileState fstate = (rpmfileState) RPMFILE_STATE_MISSING;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fstates != NULL)
	    fstate = (rpmfileState) fi->fstates[fi->i];
    }
    return fstate;
}

rpmfileState rpmfiSetFState(rpmfi fi, rpmfileState fstate)
{
    rpmuint32_t ofstate = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fstates != NULL) {
	    ofstate = fi->fstates[fi->i];
	    fi->fstates[fi->i] = fstate;
	}
    }
    return (rpmfileState) ofstate;
}

const unsigned char * rpmfiDigest(rpmfi fi, int * algop, size_t * lenp)
{
    unsigned char * digest = NULL;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->digests != NULL) {
	    digest = fi->digests + (fi->digestlen * fi->i);
	    if (algop != NULL)
		*algop = (fi->fdigestalgos
			? fi->fdigestalgos[fi->i] : fi->digestalgo);
	    if (lenp != NULL)
		*lenp = fi->digestlen;
	}
    }
    return digest;
}

const char * rpmfiFLink(rpmfi fi)
{
    const char * flink = NULL;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->flinks != NULL)
	    flink = fi->flinks[fi->i];
    }
    return flink;
}

rpmuint32_t rpmfiFSize(rpmfi fi)
{
    rpmuint32_t fsize = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fsizes != NULL)
	    fsize = fi->fsizes[fi->i];
    }
    return fsize;
}

rpmuint16_t rpmfiFRdev(rpmfi fi)
{
    rpmuint16_t frdev = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->frdevs != NULL)
	    frdev = fi->frdevs[fi->i];
    }
    return frdev;
}

rpmuint32_t rpmfiFInode(rpmfi fi)
{
    rpmuint32_t finode = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->finodes != NULL)
	    finode = fi->finodes[fi->i];
    }
    return finode;
}

rpmuint32_t rpmfiColor(rpmfi fi)
{
    rpmuint32_t color = 0;

    if (fi != NULL)
	/* XXX ignore all but lsnibble for now. */
	color = fi->color & 0xf;
    return color;
}

rpmuint32_t rpmfiFColor(rpmfi fi)
{
    rpmuint32_t fcolor = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fcolors != NULL)
	    /* XXX ignore all but lsnibble for now. */
	    fcolor = (fi->fcolors[fi->i] & 0x0f);
    }
    return fcolor;
}

const char * rpmfiFClass(rpmfi fi)
{
    const char * fclass = NULL;

    if (fi != NULL && fi->fcdictx != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	int cdictx = fi->fcdictx[fi->i];
	if (fi->cdict != NULL && cdictx >= 0 && cdictx < (int)fi->ncdict)
	    fclass = fi->cdict[cdictx];
    }
    return fclass;
}

const char * rpmfiFContext(rpmfi fi)
{
    const char * fcontext = NULL;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fcontexts != NULL)
	    fcontext = fi->fcontexts[fi->i];
    }
    return fcontext;
}

rpmuint32_t rpmfiFDepends(rpmfi fi, const rpmuint32_t ** fddictp)
{
    int fddictx = -1;
    int fddictn = 0;
    const rpmuint32_t * fddict = NULL;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fddictn != NULL)
	    fddictn = fi->fddictn[fi->i];
	if (fddictn > 0 && fi->fddictx != NULL)
	    fddictx = fi->fddictx[fi->i];
	if (fi->ddict != NULL && fddictx >= 0 && (fddictx+fddictn) <= (int)fi->nddict)
	    fddict = fi->ddict + fddictx;
    }
/*@-dependenttrans -onlytrans @*/
    if (fddictp)
	*fddictp = fddict;
/*@=dependenttrans =onlytrans @*/
    return fddictn;
}

rpmuint32_t rpmfiFNlink(rpmfi fi)
{
    rpmuint32_t nlink = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	/* XXX rpm-2.3.12 has not RPMTAG_FILEINODES */
	if (fi->finodes && fi->frdevs) {
	    rpmuint32_t finode = fi->finodes[fi->i];
	    rpmuint16_t frdev = fi->frdevs[fi->i];
	    int j;

	    for (j = 0; j < (int)fi->fc; j++) {
		if (fi->frdevs[j] == frdev && fi->finodes[j] == finode)
		    nlink++;
	    }
	}
    }
    return nlink;
}

rpmuint32_t rpmfiFMtime(rpmfi fi)
{
    rpmuint32_t fmtime = 0;

    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fmtimes != NULL)
	    fmtime = fi->fmtimes[fi->i];
    }
    return fmtime;
}

const char * rpmfiFUser(rpmfi fi)
{
    const char * fuser = NULL;

    /* XXX add support for ancient RPMTAG_FILEUIDS? */
    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fuser != NULL)
	    fuser = fi->fuser[fi->i];
    }
    return fuser;
}

const char * rpmfiFGroup(rpmfi fi)
{
    const char * fgroup = NULL;

    /* XXX add support for ancient RPMTAG_FILEGIDS? */
    if (fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	if (fi->fgroup != NULL)
	    fgroup = fi->fgroup[fi->i];
    }
    return fgroup;
}

void * rpmfiBloomFN(const rpmfi fi)
{
/*@-assignexpose -retexpose @*/
    return (fi != NULL ? fi->_fnbf : NULL);
/*@=assignexpose =retexpose @*/
}

void * rpmfiExclude(const rpmfi fi)
{
    return (fi != NULL ? fi->exclude : NULL);
}

int rpmfiNExclude(const rpmfi fi)
{
    return (fi != NULL ? fi->nexclude : 0);
}

void * rpmfiInclude(const rpmfi fi)
{
    return (fi != NULL ? fi->include : NULL);
}

int rpmfiNInclude(const rpmfi fi)
{
    return (fi != NULL ? fi->ninclude : 0);
}

struct fingerPrint_s * rpmfiFpsIndex(rpmfi fi, int ix)
{
    struct fingerPrint_s * fps = NULL;
    if (fi != NULL && fi->fps != NULL && ix >= 0 && ix < (int)fi->fc) {
	fps = fi->fps + ix;
    }
    return fps;
}

void rpmfiFpLookup(rpmfi fi, fingerPrintCache fpc)
{
    if (fi->fc > 0 && fi->fps == NULL) {
	fi->fps = (struct fingerPrint_s *) xcalloc(fi->fc, sizeof(*fi->fps));
    }
    fpLookupList(fpc, fi->dnl, fi->bnl, fi->dil, fi->fc, fi->fps);
}

int rpmfiNext(rpmfi fi)
{
    int i = -1;

    if (fi != NULL && ++fi->i >= 0) {
	if (fi->i < (int)fi->fc) {
	    i = fi->i;
	    if (fi->dil != NULL)
		fi->j = fi->dil[fi->i];
	} else
	    fi->i = -1;

/*@-modfilesys @*/
if (_rpmfi_debug  < 0 && i != -1)
fprintf(stderr, "*** fi %p\t%s[%d] %s%s\n", fi, (fi->Type ? fi->Type : "?Type?"), i, (i >= 0 ? fi->dnl[fi->j] : ""), (i >= 0 ? fi->bnl[fi->i] : ""));
/*@=modfilesys @*/

    }

    return i;
}

rpmfi rpmfiInit(rpmfi fi, int fx)
{
    if (fi != NULL) {
	if (fx >= 0 && fx < (int)fi->fc) {
	    fi->i = fx - 1;
	    fi->j = -1;
	}
    }

    /*@-refcounttrans@*/
    return fi;
    /*@=refcounttrans@*/
}

int rpmfiNextD(rpmfi fi)
{
    int j = -1;

    if (fi != NULL && ++fi->j >= 0) {
	if (fi->j < (int)fi->dc)
	    j = fi->j;
	else
	    fi->j = -1;

/*@-modfilesys @*/
if (_rpmfi_debug  < 0 && j != -1)
fprintf(stderr, "*** fi %p\t%s[%d]\n", fi, (fi->Type ? fi->Type : "?Type?"), j);
/*@=modfilesys @*/

    }

    return j;
}

rpmfi rpmfiInitD(rpmfi fi, int dx)
{
    if (fi != NULL) {
	if (dx >= 0 && dx < (int)fi->fc)
	    fi->j = dx - 1;
	else
	    fi = NULL;
    }

    /*@-refcounttrans@*/
    return fi;
    /*@=refcounttrans@*/
}

/**
 * Identify a file type.
 * @param ft		file type
 * @return		string to identify a file type
 */
static /*@observer@*/
const char * rpmfiFtstring (rpmFileTypes ft)
	/*@*/
{
    switch (ft) {
    case XDIR:	return "directory";
    case CDEV:	return "char dev";
    case BDEV:	return "block dev";
    case LINK:	return "link";
    case SOCK:	return "sock";
    case PIPE:	return "fifo/pipe";
    case REG:	return "file";
    default:	return "unknown file type";
    }
    /*@notreached@*/
}

/**
 * Return file type from mode_t.
 * @param mode		file mode bits (from header)
 * @return		file type
 */
static rpmFileTypes rpmfiWhatis(rpmuint16_t mode)
	/*@*/
{
    if (S_ISDIR(mode))	return XDIR;
    if (S_ISCHR(mode))	return CDEV;
    if (S_ISBLK(mode))	return BDEV;
    if (S_ISLNK(mode))	return LINK;
/*@-unrecog@*/
    if (S_ISSOCK(mode))	return SOCK;
/*@=unrecog@*/
    if (S_ISFIFO(mode))	return PIPE;
    return REG;
}

int rpmfiCompare(const rpmfi afi, const rpmfi bfi)
	/*@*/
{
    rpmFileTypes awhat = rpmfiWhatis(rpmfiFMode(afi));
    rpmFileTypes bwhat = rpmfiWhatis(rpmfiFMode(bfi));

    if (awhat != bwhat) return 1;

    if (awhat == LINK) {
	const char * alink = rpmfiFLink(afi);
	const char * blink = rpmfiFLink(bfi);
	if (alink == blink) return 0;
	if (alink == NULL) return 1;
	if (blink == NULL) return -1;
	return strcmp(alink, blink);
    } else if (awhat == REG) {
	int aalgo = 0;
	size_t alen = 0;
	const unsigned char * adigest = rpmfiDigest(afi, &aalgo, &alen);
	int balgo = 0;
	size_t blen = 0;
	const unsigned char * bdigest = rpmfiDigest(bfi, &balgo, &blen);
	/* XXX W2DO? changing file digest algo may break rpmfiCompare. */
	if (!(aalgo == balgo && alen == blen))
	    return -1;
	if (adigest == bdigest) return 0;
	if (adigest == NULL) return 1;
	if (bdigest == NULL) return -1;
	return memcmp(adigest, bdigest, alen);
    }

    return 0;
}

int rpmfiDecideFate(const rpmfi ofi, rpmfi nfi, int skipMissing)
{
    const char * fn = rpmfiFN(nfi);
    int newFlags = rpmfiFFlags(nfi);
    char buffer[1024+1];
    rpmFileTypes dbWhat, newWhat, diskWhat;
    struct stat sb;
    int save = (newFlags & RPMFILE_NOREPLACE) ? FA_ALTNAME : FA_SAVE;

    if (Lstat(fn, &sb)) {
	/*
	 * The file doesn't exist on the disk. Create it unless the new
	 * package has marked it as missingok, or allfiles is requested.
	 */
	if (skipMissing && (newFlags & RPMFILE_MISSINGOK)) {
	    rpmlog(RPMLOG_DEBUG, D_("%s skipped due to missingok flag\n"),
			fn);
	    return FA_SKIP;
	} else {
	    return FA_CREATE;
	}
    }

    diskWhat = rpmfiWhatis((rpmuint16_t)sb.st_mode);
    dbWhat = rpmfiWhatis(rpmfiFMode(ofi));
    newWhat = rpmfiWhatis(rpmfiFMode(nfi));

    /*
     * RPM >= 2.3.10 shouldn't create config directories -- we'll ignore
     * them in older packages as well.
     */
    if (newWhat == XDIR)
	return FA_CREATE;

    if (diskWhat != newWhat && dbWhat != REG && dbWhat != LINK)
	return save;
    else if (newWhat != dbWhat && diskWhat != dbWhat)
	return save;
    else if (dbWhat != newWhat)
	return FA_CREATE;
    else if (dbWhat != LINK && dbWhat != REG)
	return FA_CREATE;

    /*
     * This order matters - we'd prefer to CREATE the file if at all
     * possible in case something else (like the timestamp) has changed.
     */
    memset(buffer, 0, sizeof(buffer));
    if (dbWhat == REG) {
	int oalgo = 0;
	size_t olen = 0;
	const unsigned char * odigest;
	int nalgo = 0;
	size_t nlen = 0;
	const unsigned char * ndigest;
	odigest = rpmfiDigest(ofi, &oalgo, &olen);
	if (diskWhat == REG) {
	    if (!(newFlags & RPMFILE_SPARSE))
	    if (dodigest(oalgo, fn, (unsigned char *)buffer, 0, NULL))
		return FA_CREATE;	/* assume file has been removed */
	    if (odigest && !memcmp(odigest, buffer, olen))
		return FA_CREATE;	/* unmodified config file, replace. */
	}
	ndigest = rpmfiDigest(nfi, &nalgo, &nlen);
/*@-nullpass@*/
	if (odigest && ndigest && oalgo == nalgo && olen == nlen
	 && !memcmp(odigest, ndigest, nlen))
	    return FA_SKIP;	/* identical file, don't bother. */
/*@=nullpass@*/
    } else /* dbWhat == LINK */ {
	const char * oFLink, * nFLink;
	oFLink = rpmfiFLink(ofi);
	if (diskWhat == LINK) {
	    if (Readlink(fn, buffer, sizeof(buffer) - 1) == -1)
		return FA_CREATE;	/* assume file has been removed */
	    buffer[sizeof(buffer)-1] = '\0';
	    if (oFLink && !strcmp(oFLink, buffer))
		return FA_CREATE;	/* unmodified config file, replace. */
	}
	nFLink = rpmfiFLink(nfi);
/*@-nullpass@*/
	if (oFLink && nFLink && !strcmp(oFLink, nFLink))
	    return FA_SKIP;	/* identical file, don't bother. */
/*@=nullpass@*/
    }

    /*
     * The config file on the disk has been modified, but
     * the ones in the two packages are different. It would
     * be nice if RPM was smart enough to at least try and
     * merge the difference ala CVS, but...
     */
    return save;
}

/*@observer@*/
const char * rpmfiTypeString(rpmfi fi)
{
    switch(rpmteType((rpmte)fi->te)) {
    case TR_ADDED:	return " install";
    case TR_REMOVED:	return "   erase";
    default:		return "???";
    }
    /*@noteached@*/
}

#define alloca_strdup(_s)	strcpy((char *)alloca(strlen(_s)+1), (_s))

/**
 * Relocate files in header.
 * @todo multilib file dispositions need to be checked.
 * @param ts		transaction set
 * @param fi		transaction element file info
 * @param origH		package header
 * @param actions	file dispositions
 * @return		header with relocated files
 */
static
Header relocateFileList(const rpmts ts, rpmfi fi,
		Header origH, iosmFileAction * actions)
	/*@globals rpmGlobalMacroContext, h_errno,
		internalState @*/
	/*@modifies ts, fi, origH, actions, rpmGlobalMacroContext,
		internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmte p = rpmtsRelocateElement(ts);
    static int _printed = 0;
    int allowBadRelocate = (rpmtsFilterFlags(ts) & RPMPROB_FILTER_FORCERELOCATE);
    rpmRelocation relocations = NULL;
    int numRelocations;
    const char ** validRelocations;
    rpmTagType validType;
    int numValid;
    const char ** baseNames;
    const char ** dirNames;
    rpmuint32_t * dirIndexes;
    rpmuint32_t fileCount;
    rpmuint32_t dirCount;
    rpmuint32_t mydColor = rpmExpandNumeric("%{?_autorelocate_dcolor}");
    rpmuint32_t * fFlags = NULL;
    rpmuint32_t * fColors = NULL;
    rpmuint32_t * dColors = NULL;
    rpmuint16_t * fModes = NULL;
    Header h;
    int nrelocated = 0;
    size_t fileAlloced = 0;
    char * fn = NULL;
    int haveRelocatedFile = 0;
    int reldel = 0;
    size_t len;
    int i, j;
    int xx;

    he->tag = RPMTAG_PREFIXES;
    xx = headerGet(origH, he, 0);
    validType = he->t;
    validRelocations = he->p.argv;
    numValid = he->c;
    if (!xx)
	numValid = 0;

assert(p != NULL);
    numRelocations = 0;
    if (p->relocs)
	while (p->relocs[numRelocations].newPath ||
	       p->relocs[numRelocations].oldPath)
	    numRelocations++;

    /*
     * If no relocations are specified (usually the case), then return the
     * original header. If there are prefixes, however, then INSTPREFIXES
     * should be added, but, since relocateFileList() can be called more
     * than once for the same header, don't bother if already present.
     */
    if (p->relocs == NULL || numRelocations == 0) {
	if (numValid) {
	    if (!headerIsEntry(origH, RPMTAG_INSTPREFIXES)) {
		he->tag = RPMTAG_INSTPREFIXES;
		he->t = validType;
		he->p.argv = validRelocations;
		he->c = numValid;
		xx = headerPut(origH, he, 0);
	    }
	    validRelocations = _free(validRelocations);
	}
	/* XXX FIXME multilib file actions need to be checked. */
/*@-castexpose@*/
	return headerLink(origH);
/*@=castexpose@*/
    }

/*@-castexpose@*/
    h = headerLink(origH);
/*@=castexpose@*/

    relocations = (rpmRelocation) alloca(sizeof(*relocations) * numRelocations);

    /* Build sorted relocation list from raw relocations. */
    for (i = 0; i < numRelocations; i++) {
	char * t;

	/*
	 * Default relocations (oldPath == NULL) are handled in the UI,
	 * not rpmlib.
	 */
	if (p->relocs[i].oldPath == NULL) continue; /* XXX can't happen */

	/* FIXME: Trailing /'s will confuse us greatly. Internal ones will 
	   too, but those are more trouble to fix up. :-( */
	t = alloca_strdup(p->relocs[i].oldPath);
	relocations[i].oldPath = (t[0] == '/' && t[1] == '\0')
	    ? t
	    : stripTrailingChar(t, '/');

	/* An old path w/o a new path is valid, and indicates exclusion */
	if (p->relocs[i].newPath) {
	    int del;

	    t = alloca_strdup(p->relocs[i].newPath);
	    relocations[i].newPath = (t[0] == '/' && t[1] == '\0')
		? t
		: stripTrailingChar(t, '/');

	    /*@-nullpass@*/	/* FIX:  relocations[i].oldPath == NULL */
	    /* Verify that the relocation's old path is in the header. */
	    for (j = 0; j < numValid; j++) {
		if (!strcmp(validRelocations[j], relocations[i].oldPath))
		    /*@innerbreak@*/ break;
	    }

	    /* XXX actions check prevents problem from being appended twice. */
	    if (j == numValid && !allowBadRelocate && actions) {
		rpmps ps = rpmtsProblems(ts);
		rpmpsAppend(ps, RPMPROB_BADRELOCATE,
			rpmteNEVR(p), rpmteKey(p),
			relocations[i].oldPath, NULL, NULL, 0);
		ps = rpmpsFree(ps);
	    }
	    del =
		(int)strlen(relocations[i].newPath) - (int)strlen(relocations[i].oldPath);
	    /*@=nullpass@*/

	    if (del > reldel)
		reldel = del;
	} else {
	    relocations[i].newPath = NULL;
	}
    }

    /* stupid bubble sort, but it's probably faster here */
    for (i = 0; i < numRelocations; i++) {
	int madeSwap;
	madeSwap = 0;
	for (j = 1; j < numRelocations; j++) {
	    struct rpmRelocation_s tmpReloc;
	    if (relocations[j - 1].oldPath == NULL || /* XXX can't happen */
		relocations[j    ].oldPath == NULL || /* XXX can't happen */
	strcmp(relocations[j - 1].oldPath, relocations[j].oldPath) <= 0)
		/*@innercontinue@*/ continue;
	    /*@-usereleased@*/ /* LCL: ??? */
	    tmpReloc = relocations[j - 1];
	    relocations[j - 1] = relocations[j];
	    relocations[j] = tmpReloc;
	    /*@=usereleased@*/
	    madeSwap = 1;
	}
	if (!madeSwap) break;
    }

    if (!_printed) {
	_printed = 1;
	rpmlog(RPMLOG_DEBUG, D_("========== relocations\n"));
	for (i = 0; i < numRelocations; i++) {
	    if (relocations[i].oldPath == NULL) continue; /* XXX can't happen */
	    if (relocations[i].newPath == NULL)
		rpmlog(RPMLOG_DEBUG, D_("%5d exclude  %s\n"),
			i, relocations[i].oldPath);
	    else
		rpmlog(RPMLOG_DEBUG, D_("%5d relocate %s -> %s\n"),
			i, relocations[i].oldPath, relocations[i].newPath);
	}
    }

    /* Add relocation values to the header */
    if (numValid) {
	const char ** actualRelocations;
	int numActual;

	actualRelocations = (const char **) xmalloc(numValid * sizeof(*actualRelocations));
	numActual = 0;
	for (i = 0; i < numValid; i++) {
	    for (j = 0; j < numRelocations; j++) {
		if (relocations[j].oldPath == NULL || /* XXX can't happen */
		    strcmp(validRelocations[i], relocations[j].oldPath))
		    /*@innercontinue@*/ continue;
		/* On install, a relocate to NULL means skip the path. */
		if (relocations[j].newPath) {
		    actualRelocations[numActual] = relocations[j].newPath;
		    numActual++;
		}
		/*@innerbreak@*/ break;
	    }
	    if (j == numRelocations) {
		actualRelocations[numActual] = validRelocations[i];
		numActual++;
	    }
	}

	if (numActual) {
	    he->tag = RPMTAG_INSTPREFIXES;
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->p.argv = actualRelocations;
	    he->c = numActual;
	    xx = headerPut(h, he, 0);
	}

	actualRelocations = _free(actualRelocations);
	validRelocations = _free(validRelocations);
    }

    he->tag = RPMTAG_BASENAMES;
    xx = headerGet(h, he, 0);
    baseNames = he->p.argv;
    fileCount = he->c;
    he->tag = RPMTAG_DIRINDEXES;
    xx = headerGet(h, he, 0);
    dirIndexes = he->p.ui32p;
    he->tag = RPMTAG_DIRNAMES;
    xx = headerGet(h, he, 0);
    dirNames = he->p.argv;
    dirCount = he->c;
    he->tag = RPMTAG_FILEFLAGS;
    xx = headerGet(h, he, 0);
    fFlags = he->p.ui32p;
    he->tag = RPMTAG_FILECOLORS;
    xx = headerGet(h, he, 0);
    fColors = he->p.ui32p;
    he->tag = RPMTAG_FILEMODES;
    xx = headerGet(h, he, 0);
    fModes = he->p.ui16p;

    dColors = (rpmuint32_t *) alloca(dirCount * sizeof(*dColors));
    memset(dColors, 0, dirCount * sizeof(*dColors));

    /*
     * For all relocations, we go through sorted file/relocation lists 
     * backwards so that /usr/local relocations take precedence over /usr 
     * ones.
     */

    /* Relocate individual paths. */

    for (i = fileCount - 1; i >= 0; i--) {
	rpmFileTypes ft;
	size_t fnlen;

	len = reldel +
		strlen(dirNames[dirIndexes[i]]) + strlen(baseNames[i]) + 1;
	if (len >= fileAlloced) {
	    fileAlloced = len * 2;
	    fn = (char *) xrealloc(fn, fileAlloced);
	}

assert(fn != NULL);		/* XXX can't happen */
	*fn = '\0';
	fnlen = stpcpy( stpcpy(fn, dirNames[dirIndexes[i]]), baseNames[i]) - fn;

if (fColors != NULL) {
/* XXX pkgs may not have unique dirNames, so color all dirNames that match. */
for (j = 0; j < (int)dirCount; j++) {
if (strcmp(dirNames[dirIndexes[i]], dirNames[j])) /*@innercontinue@*/ continue;
dColors[j] |= fColors[i];
}
}

	/*
	 * See if this file path needs relocating.
	 */
	/*
	 * XXX FIXME: Would a bsearch of the (already sorted) 
	 * relocation list be a good idea?
	 */
	for (j = numRelocations - 1; j >= 0; j--) {
	    if (relocations[j].oldPath == NULL) /* XXX can't happen */
		/*@innercontinue@*/ continue;
	    len = strcmp(relocations[j].oldPath, "/")
		? strlen(relocations[j].oldPath)
		: 0;

	    if (fnlen < len)
		/*@innercontinue@*/ continue;
	    /*
	     * Only subdirectories or complete file paths may be relocated. We
	     * don't check for '\0' as our directory names all end in '/'.
	     */
	    if (!(fn[len] == '/' || fnlen == len))
		/*@innercontinue@*/ continue;

	    if (strncmp(relocations[j].oldPath, fn, len))
		/*@innercontinue@*/ continue;
	    /*@innerbreak@*/ break;
	}
	if (j < 0) continue;

/*@-nullderef@*/ /* FIX: fModes may be NULL */
	ft = rpmfiWhatis(fModes[i]);
/*@=nullderef@*/

	/* On install, a relocate to NULL means skip the path. */
	if (relocations[j].newPath == NULL) {
	    if (ft == XDIR) {
		/* Start with the parent, looking for directory to exclude. */
		for (j = dirIndexes[i]; j < (int)dirCount; j++) {
		    len = strlen(dirNames[j]) - 1;
		    while (len > 0 && dirNames[j][len-1] == '/') len--;
		    if (fnlen != len)
			/*@innercontinue@*/ continue;
		    if (strncmp(fn, dirNames[j], fnlen))
			/*@innercontinue@*/ continue;
		    /*@innerbreak@*/ break;
		}
	    }
	    if (actions) {
		actions[i] = FA_SKIPNSTATE;
		rpmlog(RPMLOG_DEBUG, D_("excluding %s %s\n"),
			rpmfiFtstring(ft), fn);
	    }
	    continue;
	}

	/* Relocation on full paths only, please. */
	if (fnlen != len) continue;

	if (actions)
	    rpmlog(RPMLOG_DEBUG, D_("relocating %s to %s\n"),
		    fn, relocations[j].newPath);
	nrelocated++;

	strcpy(fn, relocations[j].newPath);
	{   char * te = strrchr(fn, '/');
	    if (te) {
		if (te > fn) te++;	/* root is special */
		fnlen = te - fn;
	    } else
		te = fn + strlen(fn);
	    /*@-nullpass -nullderef@*/	/* LCL: te != NULL here. */
	    if (strcmp(baseNames[i], te)) /* basename changed too? */
		baseNames[i] = alloca_strdup(te);
	    *te = '\0';			/* terminate new directory name */
	    /*@=nullpass =nullderef@*/
	}

	/* Does this directory already exist in the directory list? */
	for (j = 0; j < (int)dirCount; j++) {
	    if (fnlen != strlen(dirNames[j]))
		/*@innercontinue@*/ continue;
	    if (strncmp(fn, dirNames[j], fnlen))
		/*@innercontinue@*/ continue;
	    /*@innerbreak@*/ break;
	}
	
	if (j < (int)dirCount) {
	    dirIndexes[i] = j;
	    continue;
	}

	/* Creating new paths is a pita */
	if (!haveRelocatedFile) {
	    const char ** newDirList;

	    haveRelocatedFile = 1;
	    newDirList = (const char **) xmalloc((dirCount + 1) * sizeof(*newDirList));
	    for (j = 0; j < (int)dirCount; j++)
		newDirList[j] = alloca_strdup(dirNames[j]);
	    dirNames = _free(dirNames);
	    dirNames = newDirList;
	} else {
	    dirNames = (const char **) xrealloc(dirNames, 
			       sizeof(*dirNames) * (dirCount + 1));
	}

	dirNames[dirCount] = alloca_strdup(fn);
	dirIndexes[i] = dirCount;
	dirCount++;
    }

    /* Finish off by relocating directories. */
    for (i = dirCount - 1; i >= 0; i--) {
	for (j = numRelocations - 1; j >= 0; j--) {

           /* XXX Don't autorelocate uncolored directories. */
           if (j == p->autorelocatex
            && (dColors[i] == 0 || !(dColors[i] & mydColor)))
               /*@innercontinue@*/ continue;

	    if (relocations[j].oldPath == NULL) /* XXX can't happen */
		/*@innercontinue@*/ continue;
	    len = strcmp(relocations[j].oldPath, "/")
		? strlen(relocations[j].oldPath)
		: 0;

	    if (len && strncmp(relocations[j].oldPath, dirNames[i], len))
		/*@innercontinue@*/ continue;

	    /*
	     * Only subdirectories or complete file paths may be relocated. We
	     * don't check for '\0' as our directory names all end in '/'.
	     */
	    if (dirNames[i][len] != '/')
		/*@innercontinue@*/ continue;

	    if (relocations[j].newPath) { /* Relocate the path */
		const char * s = relocations[j].newPath;
		size_t slen = strlen(s) + strlen(dirNames[i]) - len;
		char * t = (char *) alloca(slen + 1);

		(void) stpcpy( stpcpy(t, s) , dirNames[i] + len);

		/* Unfortunately rpmCleanPath strips the trailing slash.. */
		(void) rpmCleanPath(t);
		slen = strlen(t);
		t[slen] = '/';
		t[slen+1] = '\0';

		if (actions)
		    rpmlog(RPMLOG_DEBUG,
			D_("relocating directory %s to %s\n"), dirNames[i], t);
		dirNames[i] = t;
		nrelocated++;
	    }
	}
    }

    /* Save original filenames in header and replace (relocated) filenames. */
    if (nrelocated) {
	he->tag = RPMTAG_BASENAMES;
	xx = headerGet(h, he, 0);
	he->tag = RPMTAG_ORIGBASENAMES;
	xx = headerPut(h, he, 0);
	he->p.ptr = _free(he->p.ptr);

	he->tag = RPMTAG_DIRNAMES;
	xx = headerGet(h, he, 0);
	he->tag = RPMTAG_ORIGDIRNAMES;
	xx = headerPut(h, he, 0);
	he->p.ptr = _free(he->p.ptr);

	he->tag = RPMTAG_DIRINDEXES;
	xx = headerGet(h, he, 0);
	he->tag = RPMTAG_ORIGDIRINDEXES;
	xx = headerPut(h, he, 0);
	he->p.ptr = _free(he->p.ptr);

	he->tag = RPMTAG_BASENAMES;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = baseNames;
	he->c = fileCount;
	xx = headerMod(h, he, 0);
	fi->bnl = _free(fi->bnl);
	xx = headerGet(h, he, 0);
/*@-dependenttrans@*/
	fi->bnl = he->p.argv;
/*@=dependenttrans@*/
	fi->fc = he->c;

	he->tag = RPMTAG_DIRNAMES;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = dirNames;
	he->c = dirCount;
	xx = headerMod(h, he, 0);
	fi->dnl = _free(fi->dnl);
	xx = headerGet(h, he, 0);
	fi->dnl = he->p.argv;
	fi->dc = he->c;

	he->tag = RPMTAG_DIRINDEXES;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = dirIndexes;
	he->c = fileCount;
	xx = headerMod(h, he, 0);
	fi->dil = _free(fi->dil);
	xx = headerGet(h, he, 0);
/*@-dependenttrans@*/
	fi->dil = he->p.ui32p;
/*@=dependenttrans@*/
    }

    baseNames = _free(baseNames);
    dirIndexes = _free(dirIndexes);
    dirNames = _free(dirNames);
    fFlags = _free(fFlags);
    fColors = _free(fColors);
    fModes = _free(fModes);

/*@-dependenttrans@*/
    fn = _free(fn);
/*@=dependenttrans@*/

/*@-retalias@*/
    return h;
/*@=retalias@*/
}

int rpmfiSetHeader(rpmfi fi, Header h)
{
    if (fi->h != NULL)
	(void)headerFree(fi->h);
    fi->h = NULL;
/*@-assignexpose -castexpose @*/
    if (h != NULL)
	fi->h = headerLink(h);
/*@=assignexpose =castexpose @*/
    return 0;
}

static void rpmfiFini(void * _fi)
	/*@modifies *_fi @*/
{
    rpmfi fi = (rpmfi) _fi;

    /* Free pre- and post-transaction script and interpreter strings. */
    fi->pretrans = _free(fi->pretrans);
    fi->pretransprog = _free(fi->pretransprog);
    fi->posttrans = _free(fi->posttrans);
    fi->posttransprog = _free(fi->posttransprog);
    fi->verifyscript = _free(fi->verifyscript);
    fi->verifyscriptprog = _free(fi->verifyscriptprog);

    if (fi->fc > 0) {
	fi->bnl = _free(fi->bnl);
	fi->dnl = _free(fi->dnl);

	fi->flinks = _free(fi->flinks);
	fi->flangs = _free(fi->flangs);
	fi->fdigests = _free(fi->fdigests);
	fi->digests = _free(fi->digests);

	fi->cdict = _free(fi->cdict);

	fi->fuser = _free(fi->fuser);
	fi->fgroup = _free(fi->fgroup);

	fi->fstates = _free(fi->fstates);

	fi->fmtimes = _free(fi->fmtimes);
	fi->fmodes = _free(fi->fmodes);
	fi->fflags = _free(fi->fflags);
	fi->vflags = _free(fi->vflags);
	fi->fsizes = _free(fi->fsizes);
	fi->frdevs = _free(fi->frdevs);
	fi->finodes = _free(fi->finodes);
	fi->dil = _free(fi->dil);

	fi->fcolors = _free(fi->fcolors);
	fi->fcdictx = _free(fi->fcdictx);
	fi->ddict = _free(fi->ddict);
	fi->fddictx = _free(fi->fddictx);
	fi->fddictn = _free(fi->fddictn);
    }

/*@-globs@*/	/* Avoid rpmGlobalMacroContext */
    fi->fsm = freeFSM((IOSM_t)fi->fsm);
/*@=globs@*/

    fi->_fnbf = rpmbfFree((rpmbf)fi->_fnbf);
    fi->exclude = (miRE) mireFreeAll(fi->exclude, fi->nexclude);
    fi->include = (miRE) mireFreeAll(fi->include, fi->ninclude);

    fi->fn = _free(fi->fn);
    fi->apath = _free(fi->apath);
    fi->fmapflags = _free(fi->fmapflags);

    fi->obnl = _free(fi->obnl);
    fi->odnl = _free(fi->odnl);

    fi->fcontexts = _free(fi->fcontexts);

    fi->actions = _free(fi->actions);
    fi->replacedSizes = _free(fi->replacedSizes);

    (void)headerFree(fi->h);
    fi->h = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmfiPool;

static rpmfi rpmfiGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmfiPool, fileSystem, internalState @*/
	/*@modifies pool, _rpmfiPool, fileSystem, internalState @*/
{
    rpmfi fi;

    if (_rpmfiPool == NULL) {
	_rpmfiPool = rpmioNewPool("fi", sizeof(*fi), -1, _rpmfi_debug,
			NULL, NULL, rpmfiFini);
	pool = _rpmfiPool;
    }
    fi = (rpmfi) rpmioGetPool(pool, sizeof(*fi));
    memset(((char *)fi)+sizeof(fi->_item), 0, sizeof(*fi)-sizeof(fi->_item));
    return fi;
}

/**
 * Convert hex to binary nibble.
 * @param c		hex character
 * @return		binary nibble
 */
static inline unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (c - '0');
    if (c >= 'A' && c <= 'F')
	return (c - 'A') + 10;
    if (c >= 'a' && c <= 'f')
	return (c - 'a') + 10;
    return 0;
}

#define _fdupestring(_h, _tag, _data) \
    he->tag = _tag; \
    xx = headerGet((_h), he, 0); \
    _data = he->p.str;

#define _fdupedata(_h, _tag, _cast, _data) \
    he->tag = _tag; \
    xx = headerGet((_h), he, 0); \
    _data = (_cast) he->p.ptr;

/*@-strictusereleased@*/
rpmfi rpmfiNew(const void * _ts, Header h, rpmTag tagN, int flags)
{
/*@-castexpose@*/
    const rpmts ts = (const rpmts) _ts;
/*@=castexpose@*/
    int scareMem = (flags & 0x1);
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmte p;
    rpmfi fi = NULL;
    const char * Type;
    unsigned char * t;
    pgpHashAlgo dalgo;
    int xx;
    int i;

assert(scareMem == 0);		/* XXX always allocate memory */
    if (tagN == RPMTAG_BASENAMES) {
	Type = "Files";
    } else {
	Type = "?Type?";
	goto exit;
    }

    fi = rpmfiGetPool(_rpmfiPool);
    if (fi == NULL)	/* XXX can't happen */
	goto exit;

    fi->magic = RPMFIMAGIC;
    fi->Type = Type;
    fi->i = -1;
    fi->tagN = tagN;

    fi->h = NULL;
    fi->isSource =
	(headerIsEntry(h, RPMTAG_SOURCERPM) == 0 &&
	 headerIsEntry(h, RPMTAG_RPMVERSION) != 0 &&
	 headerIsEntry(h, RPMTAG_ARCH) != 0);

    if (fi->fsm == NULL)
	fi->fsm = newFSM();

    ((FSM_t)fi->fsm)->repackaged = (headerIsEntry(h, RPMTAG_REMOVETID) ? 1 : 0);

    /* 0 means unknown */
    he->tag = RPMTAG_ARCHIVESIZE;
    xx = headerGet(h, he, 0);
    fi->archivePos = 0;
    fi->archiveSize = (xx && he->p.ui32p ? he->p.ui32p[0] : 0);
    he->p.ptr = _free(he->p.ptr);

    /* Extract pre- and post-transaction script and interpreter strings. */
    _fdupestring(h, RPMTAG_PRETRANS, fi->pretrans);
    _fdupestring(h, RPMTAG_PRETRANSPROG, fi->pretransprog);
    _fdupestring(h, RPMTAG_POSTTRANS, fi->posttrans);
    _fdupestring(h, RPMTAG_POSTTRANSPROG, fi->posttransprog);
    _fdupestring(h, RPMTAG_VERIFYSCRIPT, fi->verifyscript);
    _fdupestring(h, RPMTAG_VERIFYSCRIPTPROG, fi->verifyscriptprog);

    he->tag = RPMTAG_BASENAMES;
    xx = headerGet(h, he, 0);
    /* XXX 3.0.x SRPM's can be used, relative fn's at RPMTAG_OLDFILENAMES. */
    if (xx == 0 && fi->isSource) {
	he->tag = RPMTAG_OLDFILENAMES;
	xx = headerGet(h, he, 0);
    }
    fi->bnl = he->p.argv;
    fi->fc = he->c;
    if (!xx) {
	fi->fc = 0;
	fi->dc = 0;
	goto exit;
    }
    _fdupedata(h, RPMTAG_DIRNAMES, const char **, fi->dnl);
    fi->dc = he->c;
    /* XXX 3.0.x SRPM's can be used, relative fn's at RPMTAG_OLDFILENAMES. */
    if (fi->dc == 0 && fi->isSource) {
	fi->dc = 1;
	fi->dnl = (const char **) xcalloc(3, sizeof(*fi->dnl));
	fi->dnl[0] = (const char *)&fi->dnl[2];
	fi->dil = (rpmuint32_t *) xcalloc(fi->fc, sizeof(*fi->dil));
    } else {
	_fdupedata(h, RPMTAG_DIRINDEXES, rpmuint32_t *, fi->dil);
    }
    _fdupedata(h, RPMTAG_FILEMODES, rpmuint16_t *, fi->fmodes);
    _fdupedata(h, RPMTAG_FILEFLAGS, rpmuint32_t *, fi->fflags);
    _fdupedata(h, RPMTAG_FILEVERIFYFLAGS, rpmuint32_t *, fi->vflags);
    _fdupedata(h, RPMTAG_FILESIZES, rpmuint32_t *, fi->fsizes);

    _fdupedata(h, RPMTAG_FILECOLORS, rpmuint32_t *, fi->fcolors);
    fi->color = 0;
    if (fi->fcolors != NULL)
    for (i = 0; i < (int)fi->fc; i++)
	fi->color |= fi->fcolors[i];
    _fdupedata(h, RPMTAG_CLASSDICT, const char **, fi->cdict);
    fi->ncdict = he->c;
    _fdupedata(h, RPMTAG_FILECLASS, rpmuint32_t *, fi->fcdictx);

    _fdupedata(h, RPMTAG_DEPENDSDICT, rpmuint32_t *, fi->ddict);
    fi->nddict = he->c;
    _fdupedata(h, RPMTAG_FILEDEPENDSX, rpmuint32_t *, fi->fddictx);
    _fdupedata(h, RPMTAG_FILEDEPENDSN, rpmuint32_t *, fi->fddictn);

    _fdupedata(h, RPMTAG_FILESTATES, rpmuint8_t *, fi->fstates);
    if (xx == 0 || fi->fstates == NULL)
	fi->fstates = (rpmuint8_t *) xcalloc(fi->fc, sizeof(*fi->fstates));

    fi->action = FA_UNKNOWN;
    fi->flags = 0;

if (fi->actions == NULL)
	fi->actions = (int *) xcalloc(fi->fc, sizeof(*fi->actions));

    /* XXX TR_REMOVED needs IOSM_MAP_{ABSOLUTE,ADDDOT} IOSM_ALL_HARDLINKS */
    fi->mapflags =
		IOSM_MAP_PATH | IOSM_MAP_MODE | IOSM_MAP_UID | IOSM_MAP_GID;

    _fdupedata(h, RPMTAG_FILELINKTOS, const char **, fi->flinks);
    _fdupedata(h, RPMTAG_FILELANGS, const char **, fi->flangs);

    dalgo = PGPHASHALGO_ERROR;
    fi->fdigestalgos = NULL;
    _fdupedata(h, RPMTAG_FILEDIGESTALGOS, rpmuint32_t *, fi->fdigestalgos);
    if (fi->fdigestalgos) {
	/* XXX Insure that all algorithms are either 0 or constant. */
	for (i = 0; i < (int)fi->fc; i++) {
	    if (fi->fdigestalgos[i] == 0)
		continue;
	    if (dalgo == PGPHASHALGO_ERROR)
		dalgo = (pgpHashAlgo) (fi->fdigestalgos[i] & 0xff);
	    else
assert(dalgo == (pgpHashAlgo)fi->fdigestalgos[i]);
	}
	fi->fdigestalgos = _free(fi->fdigestalgos);
    } else {
	he->tag = RPMTAG_FILEDIGESTALGO;
	xx = headerGet(h, he, 0);
	if (xx)
	    dalgo = (pgpHashAlgo) he->p.ui32p[0];
	he->p.ptr = _free(he->p.ptr);
    }

    switch (dalgo) {
    default: dalgo = PGPHASHALGO_MD5;	fi->digestlen = 128/8; break;
    case PGPHASHALGO_MD2:	fi->digestlen = 128/8;	break;
    case PGPHASHALGO_MD5:	fi->digestlen = 128/8;	break;
    case PGPHASHALGO_SHA1:	fi->digestlen = 160/8;	break;
    case PGPHASHALGO_RIPEMD128:	fi->digestlen = 128/8;	break;
    case PGPHASHALGO_RIPEMD160:	fi->digestlen = 160/8;	break;
    case PGPHASHALGO_RIPEMD256:	fi->digestlen = 256/8;	break;
    case PGPHASHALGO_RIPEMD320:	fi->digestlen = 320/8;	break;
    case PGPHASHALGO_SHA224:	fi->digestlen = 224/8;	break;
    case PGPHASHALGO_SHA256:	fi->digestlen = 256/8;	break;
    case PGPHASHALGO_SHA384:	fi->digestlen = 384/8;	break;
    case PGPHASHALGO_SHA512:	fi->digestlen = 512/8;	break;
    case PGPHASHALGO_CRC32:	fi->digestlen = 32/8;	break;
    }
    fi->digestalgo = dalgo;

    fi->digests = NULL;
    _fdupedata(h, RPMTAG_FILEDIGESTS, const char **, fi->fdigests);
    if (fi->fdigests) {
	t = (unsigned char *) xmalloc(fi->fc * fi->digestlen);
	fi->digests = t;
	for (i = 0; i < (int)fi->fc; i++) {
	    const char * fdigests;
	    int j;

	    fdigests = fi->fdigests[i];
	    if (!(fdigests && *fdigests != '\0')) {
		memset(t, 0, fi->digestlen);
		t += fi->digestlen;
		continue;
	    }
	    for (j = 0; j < (int)fi->digestlen; j++, t++, fdigests += 2)
		*t = (nibble(fdigests[0]) << 4) | nibble(fdigests[1]);
	}
	fi->fdigests = _free(fi->fdigests);
    }

    /* XXX TR_REMOVED doesn't need fmtimes, frdevs, finodes, or fcontexts */
    _fdupedata(h, RPMTAG_FILEMTIMES, rpmuint32_t *, fi->fmtimes);
    _fdupedata(h, RPMTAG_FILERDEVS, rpmuint16_t *, fi->frdevs);
    _fdupedata(h, RPMTAG_FILEINODES, rpmuint32_t *, fi->finodes);
    _fdupedata(h, RPMTAG_FILECONTEXTS, const char **, fi->fcontexts);

    fi->replacedSizes = (rpmuint32_t *) xcalloc(fi->fc, sizeof(*fi->replacedSizes));

    _fdupedata(h, RPMTAG_FILEUSERNAME, const char **, fi->fuser);
    _fdupedata(h, RPMTAG_FILEGROUPNAME, const char **, fi->fgroup);

    if (ts != NULL)
    if (fi != NULL)
    if ((p = rpmtsRelocateElement(ts)) != NULL && rpmteType(p) == TR_ADDED
     && headerIsEntry(h, RPMTAG_SOURCERPM)
     && !headerIsEntry(h, RPMTAG_ORIGBASENAMES))
    {
	const char * fmt = rpmGetPath("%{?_autorelocate_path}", NULL);
	const char * errstr;
	char * newPath;
	Header foo;

	/* XXX error handling. */
	newPath = headerSprintf(h, fmt, NULL, rpmHeaderFormats, &errstr);
	fmt = _free(fmt);

	/* XXX Make sure autoreloc is not already specified. */
	i = p->nrelocs;
	if (newPath != NULL && *newPath != '\0' && p->relocs != NULL)
	for (i = 0; i < p->nrelocs; i++) {
/*@-nullpass@*/ /* XXX {old,new}Path might be NULL */
	   if (strcmp(p->relocs[i].oldPath, "/"))
		continue;
	   if (strcmp(p->relocs[i].newPath, newPath))
		continue;
/*@=nullpass@*/
	   break;
	}

	/* XXX test for incompatible arch triggering autorelocation is dumb. */
	/* XXX DIEDIEDIE: used to test '... && p->archScore == 0' */
	if (newPath != NULL && *newPath != '\0' && i == p->nrelocs) {

	    p->relocs =
		(rpmRelocation) xrealloc(p->relocs, (p->nrelocs + 2) * sizeof(*p->relocs));
	    p->relocs[p->nrelocs].oldPath = xstrdup("/");
	    p->relocs[p->nrelocs].newPath = xstrdup(newPath);
	    p->autorelocatex = p->nrelocs;
	    p->nrelocs++;
	    p->relocs[p->nrelocs].oldPath = NULL;
	    p->relocs[p->nrelocs].newPath = NULL;
	}
	newPath = _free(newPath);

/* XXX DYING */
if (fi->actions == NULL)
	fi->actions = (int *) xcalloc(fi->fc, sizeof(*fi->actions));
	/*@-compdef@*/ /* FIX: fi->digests undefined */
	foo = relocateFileList(ts, fi, h, (iosmFileAction *) fi->actions);
	/*@=compdef@*/
	(void)headerFree(fi->h);
	fi->h = NULL;
	fi->h = headerLink(foo);
	(void)headerFree(foo);
	foo = NULL;
    }

    if (fi->isSource && fi->dc == 1 && *fi->dnl[0] == '\0') {
	const char ** av = (const char **) xcalloc(4+1, sizeof(*av));
	char * te;
	size_t nb;

	xx = headerMacrosLoad(h);
	av[0] = rpmGenPath(rpmtsRootDir(ts), "%{_sourcedir}", "");
	av[1] = rpmGenPath(rpmtsRootDir(ts), "%{_specdir}", "");
	av[2] = rpmGenPath(rpmtsRootDir(ts), "%{_patchdir}", "");
	av[3] = rpmGenPath(rpmtsRootDir(ts), "%{_icondir}", "");
	av[4] = NULL;
	xx = headerMacrosUnload(h);

	/* Hack up a header RPM_STRING_ARRAY_TYPE array. */
	fi->dnl = _free(fi->dnl);
	fi->dc = 4;
	nb = fi->dc * sizeof(*av);
	for (i = 0; i < (int)fi->dc; i++)
	    nb += strlen(av[i]) + sizeof("/");

	fi->dnl = (const char **) xmalloc(nb);
	te = (char *) (&fi->dnl[fi->dc]);
	*te = '\0';
	for (i = 0; i < (int)fi->dc; i++) {
	    fi->dnl[i] = te;
	    te = stpcpy( stpcpy(te, av[i]), "/");
	    *te++ = '\0';
	}
	av = argvFree(av);

	/* Map basenames to appropriate directories. */
	for (i = 0; i < (int)fi->fc; i++) {
	    if (fi->fflags[i] & RPMFILE_SOURCE)
		fi->dil[i] = 0;
	    else if (fi->fflags[i] & RPMFILE_SPECFILE)
		fi->dil[i] = 1;
	    else if (fi->fflags[i] & RPMFILE_PATCH)
		fi->dil[i] = 2;
	    else if (fi->fflags[i] & RPMFILE_ICON)
		fi->dil[i] = 3;
	    else {
		const char * b = fi->bnl[i];
		const char * be = b + strlen(b) - sizeof(".spec") - 1;

		fi->dil[i] = (be > b && !strcmp(be, ".spec") ? 1 : 0);
	    }
	}
    }

    if (!scareMem)
	(void)headerFree(fi->h);
    fi->h = NULL;

    fi->fn = NULL;
    fi->fnlen = 0;
    for (i = 0; i < (int)fi->fc; i++) {
	size_t fnlen = strlen(fi->dnl[fi->dil[i]]) + strlen(fi->bnl[i]);
	fnlen++;	/* XXX guarantee space for pesky trailing '/' */
	if (fnlen > fi->fnlen)
	    fi->fnlen = fnlen;
    }
    
    fi->dperms = 0755;
    fi->fperms = 0644;

exit:
/*@-modfilesys@*/
if (_rpmfi_debug < 0)
fprintf(stderr, "*** fi %p\t%s[%d]\n", fi, Type, (fi ? fi->fc : 0));
/*@=modfilesys@*/

    /*@-compdef -nullstate@*/ /* FIX: rpmfi null annotations */
    return rpmfiLink(fi, (fi ? fi->Type : NULL));
    /*@=compdef =nullstate@*/
}
/*@=strictusereleased@*/

int rpmfiAddRelocation(rpmRelocation * relp, int * nrelp,
		const char * oldPath, const char * newPath)
{
/*@-unqualifiedtrans@*/
    *relp = (rpmRelocation) xrealloc(*relp, sizeof(**relp) * ((*nrelp) + 1));
/*@=unqualifiedtrans@*/
    (*relp)[*nrelp].oldPath = (oldPath ? xstrdup(oldPath) : NULL);
    (*relp)[*nrelp].newPath = (newPath ? xstrdup(newPath) : NULL);
    (*nrelp)++;
    return 0;
}

rpmRelocation rpmfiFreeRelocations(rpmRelocation relocs)
{
    if (relocs) {
	rpmRelocation r;
	for (r = relocs; (r->oldPath || r->newPath); r++) {
	    r->oldPath = _free(r->oldPath);
	    r->newPath = _free(r->newPath);
	}
	relocs = _free(relocs);
    }
    return NULL;
}

rpmRelocation rpmfiDupeRelocations(rpmRelocation relocs, int * nrelocsp)
{
    rpmRelocation newr = NULL;
    int nrelocs = 0;

    if (relocs) {
	rpmRelocation r;
	int i;

	for (r = relocs; r->oldPath || r->newPath; r++)
	    nrelocs++;
	newr = (rpmRelocation) xmalloc((nrelocs + 1) * sizeof(*relocs));

	for (i = 0, r = relocs; r->oldPath || r->newPath; i++, r++) {
	    newr[i].oldPath = r->oldPath ? xstrdup(r->oldPath) : NULL;
	    newr[i].newPath = r->newPath ? xstrdup(r->newPath) : NULL;
	}
	newr[i].oldPath = NULL;
	newr[i].newPath = NULL;
    }
    if (nrelocsp)
	*nrelocsp = nrelocs;
    return newr;
}

int rpmfiFStat(rpmfi fi, struct stat * st)
{
    int rc = -1;

    if (st != NULL && fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc) {
	memset(st, 0, sizeof(*st));
	st->st_dev =
	st->st_rdev = fi->frdevs[fi->i];
	st->st_ino = fi->finodes[fi->i];
	st->st_mode = fi->fmodes[fi->i];
	st->st_nlink = rpmfiFNlink(fi) + (int)S_ISDIR(st->st_mode);
	if (unameToUid(fi->fuser[fi->i], &st->st_uid) == -1)
	    st->st_uid = 0;		/* XXX */
	if (gnameToGid(fi->fgroup[fi->i], &st->st_gid) == -1)
	    st->st_gid = 0;		/* XXX */
	st->st_size = fi->fsizes[fi->i];
	st->st_blksize = 4 * 1024;	/* XXX */
	st->st_blocks = (st->st_size + (st->st_blksize - 1)) / st->st_blksize;
	st->st_atime =
	st->st_ctime =
	st->st_mtime = fi->fmtimes[fi->i];
	rc = 0;
    }

    return rc;
}

int rpmfiStat(rpmfi fi, const char * path, struct stat * st)
{
    size_t pathlen = strlen(path);
    int rc = -1;
    int i;

    while (pathlen > 0 && path[pathlen-1] == '/')
	pathlen--;

    /* If not actively iterating, initialize. */
    if (!(fi != NULL && fi->i >= 0 && fi->i < (int)fi->fc))
	fi = rpmfiInit(fi, 0);

    while ((i = rpmfiNext(fi)) >= 0) {
	const char * fn = rpmfiFN(fi);
	size_t fnlen = strlen(fn);

	if (pathlen != fnlen || strncmp(path, fn, fnlen))
	    continue;
	rc = rpmfiFStat(fi, st);
	break;
    }

/*@-modfilesys@*/
if (_rpmfi_debug)
fprintf(stderr, "*** rpmfiStat(%p, %s, %p) rc %d\n", fi, path, st, rc);
/*@=modfilesys@*/

    return rc;
}

void * rpmfiOpendir(rpmfi fi, const char * name)
{
    const char * dn = name;
    size_t dnlen = strlen(dn);
    const char ** fnames = NULL;
    rpmuint16_t * fmodes = NULL;
    DIR * dir;
    int xx;
    int i, j;

    j = 0;
    fmodes = (rpmuint16_t *) xcalloc(fi->fc, sizeof(*fmodes));

    /* XXX todo full iteration is pig slow, fi->dil can be used for speedup. */
    fi = rpmfiInit(fi, 0);
    while ((i = rpmfiNext(fi)) >= 0) {
	const char * fn = rpmfiFN(fi);
	size_t fnlen = strlen(fn);

	if (fnlen <= dnlen)
	    continue;
	if (strncmp(dn, fn, dnlen) || fn[dnlen] != '/')
	    continue;

	/* XXX todo basename, or orphandir/.../basname, needs to be used. */
	/* Trim the directory part of the name. */
	xx = argvAdd(&fnames, fn + dnlen + 1);
	fmodes[j++] = fi->fmodes[i];
    }

    /* Add "." & ".." to the argv array. */
    dir = (DIR *) avOpendir(name, fnames, fmodes);

    fnames = argvFree(fnames);
    fmodes = _free(fmodes);

/*@-modfilesys +voidabstract @*/
if (_rpmfi_debug)
fprintf(stderr, "*** rpmfiOpendir(%p, %s) dir %p\n", fi, name, dir);
/*@=modfilesys =voidabstract @*/

    return (void *)dir;
}

void rpmfiBuildFClasses(Header h,
	/*@out@*/ const char *** fclassp, /*@out@*/ rpmuint32_t * fcp)
{
    int scareMem = 0;
    rpmfi fi = rpmfiNew(NULL, h, RPMTAG_BASENAMES, scareMem);
    const char * FClass;
    const char ** av;
    int ac;
    size_t nb;
    char * t;

    if ((ac = rpmfiFC(fi)) <= 0) {
	av = NULL;
	ac = 0;
	goto exit;
    }

    /* Compute size of file class argv array blob. */
    nb = (ac + 1) * sizeof(*av);
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	FClass = rpmfiFClass(fi);
	if (FClass && *FClass != '\0')
	    nb += strlen(FClass);
	nb += 1;
    }

    /* Create and load file class argv array. */
    av = (const char **) xmalloc(nb);
    t = ((char *) av) + ((ac + 1) * sizeof(*av));
    ac = 0;
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	FClass = rpmfiFClass(fi);
	av[ac++] = t;
	if (FClass && *FClass != '\0')
	    t = stpcpy(t, FClass);
	*t++ = '\0';
    }
    av[ac] = NULL;	/* XXX tag arrays are not NULL terminated. */

exit:
    fi = rpmfiFree(fi);
    if (fclassp)
	*fclassp = av;
    else
	av = _free(av);
    if (fcp) *fcp = ac;
}

#ifdef	DYING
void rpmfiBuildFContexts(Header h,
	/*@out@*/ const char *** fcontextp, /*@out@*/ rpmuint32_t * fcp)
{
    int scareMem = 0;
    rpmfi fi = rpmfiNew(NULL, h, RPMTAG_BASENAMES, scareMem);
    const char * fcontext;
    const char ** av;
    int ac;
    size_t nb;
    char * t;

    if ((ac = rpmfiFC(fi)) <= 0) {
	av = NULL;
	ac = 0;
	goto exit;
    }

    /* Compute size of argv array blob. */
    nb = (ac + 1) * sizeof(*av);
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	fcontext = rpmfiFContext(fi);
	if (fcontext && *fcontext != '\0')
	    nb += strlen(fcontext);
	nb += 1;
    }

    /* Create and load argv array. */
    av = (const char **) xmalloc(nb);
    t = ((char *) av) + ((ac + 1) * sizeof(*av));
    ac = 0;
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	fcontext = rpmfiFContext(fi);
	av[ac++] = t;
	if (fcontext && *fcontext != '\0')
	    t = stpcpy(t, fcontext);
	*t++ = '\0';
    }
    av[ac] = NULL;	/* XXX tag arrays are not NULL terminated. */

exit:
    fi = rpmfiFree(fi);
    if (fcontextp)
	*fcontextp = av;
    else
	av = _free(av);
    if (fcp) *fcp = ac;
}

void rpmfiBuildFSContexts(Header h,
	/*@out@*/ const char *** fcontextp, /*@out@*/ rpmuint32_t * fcp)
{
    int scareMem = 0;
    rpmfi fi = rpmfiNew(NULL, h, RPMTAG_BASENAMES, scareMem);
    const char ** av;
    int ac;
    size_t nb;
    char * t;
    char * fctxt = NULL;
    size_t fctxtlen = 0;
    int * fcnb;

    if ((ac = rpmfiFC(fi)) <= 0) {
	av = NULL;
	ac = 0;
	goto exit;
    }

    /* Compute size of argv array blob, concatenating file contexts. */
    nb = ac * sizeof(*fcnb);
    fcnb = memset(alloca(nb), 0, nb);
    ac = 0;
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	const char *fn;
	security_context_t scon = NULL;

	fn = rpmfiFN(fi);
	fcnb[ac] = lgetfilecon(fn, &scon);
	if (fcnb[ac] > 0) {
	    fctxt = (char *) xrealloc(fctxt, fctxtlen + fcnb[ac]);
	    memcpy(fctxt+fctxtlen, scon, fcnb[ac]);
	    fctxtlen += fcnb[ac];
	    freecon(scon);
	}
	ac++;
    }

    /* Create and load argv array from concatenated file contexts. */
    nb = (ac + 1) * sizeof(*av) + fctxtlen;
    av = (const char **) xmalloc(nb);
    t = ((char *) av) + ((ac + 1) * sizeof(*av));
    if (fctxt != NULL && fctxtlen > 0)
	(void) memcpy(t, fctxt, fctxtlen);
    ac = 0;
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	av[ac] = "";
	if (fcnb[ac] > 0) {
	    av[ac] = t;
	    t += fcnb[ac];
	}
	ac++;
    }
    av[ac] = NULL;	/* XXX tag arrays are not NULL terminated. */

exit:
    fi = rpmfiFree(fi);
    if (fcontextp)
	*fcontextp = av;
    else
	av = _free(av);
    if (fcp) *fcp = ac;
}

void rpmfiBuildREContexts(Header h,
	/*@out@*/ const char *** fcontextp, /*@out@*/ rpmuint32_t * fcp)
{
    int scareMem = 0;
    rpmfi fi = rpmfiNew(NULL, h, RPMTAG_BASENAMES, scareMem);
    const char ** av = NULL;
    int ac;
    size_t nb;
    char * t;
    char * fctxt = NULL;
    size_t fctxtlen = 0;
    int * fcnb;

    if ((ac = rpmfiFC(fi)) <= 0) {
	ac = 0;
	goto exit;
    }

    /* Read security context patterns. */
    {	const char *fn = rpmGetPath("%{?__file_context_path}", NULL);
/*@-moduncon -noeffectuncon @*/
	if (fn != NULL && *fn != '\0')
	    (void)matchpathcon_init(fn);
/*@=moduncon =noeffectuncon @*/
	fn = _free(fn);
    }

    /* Compute size of argv array blob, concatenating file contexts. */
    nb = ac * sizeof(*fcnb);
    fcnb = memset(alloca(nb), 0, nb);
    ac = 0;
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	const char *fn;
	mode_t fmode;
	security_context_t scon;

	fn = rpmfiFN(fi);
	fmode = rpmfiFMode(fi);
	scon = NULL;
/*@-moduncon@*/
	if (matchpathcon(fn, fmode, &scon) == 0 && scon != NULL) {
	    fcnb[ac] = strlen(scon) + 1;
	    if (fcnb[ac] > 0) {
		fctxt = (char *) xrealloc(fctxt, fctxtlen + fcnb[ac]);
		memcpy(fctxt+fctxtlen, scon, fcnb[ac]);
		fctxtlen += fcnb[ac];
	    }
	    freecon(scon);
	}
/*@=moduncon@*/
	ac++;
    }

    /* Create and load argv array from concatenated file contexts. */
    nb = (ac + 1) * sizeof(*av) + fctxtlen;
    av = (const char **) xmalloc(nb);
    t = ((char *) av) + ((ac + 1) * sizeof(*av));
    (void) memcpy(t, fctxt, fctxtlen);
    ac = 0;
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	av[ac] = "";
	if (fcnb[ac] > 0) {
	    av[ac] = t;
	    t += fcnb[ac];
	}
	ac++;
    }
    av[ac] = NULL;	/* XXX tag arrays are not NULL terminated. */

exit:
/*@-moduncon -noeffectuncon @*/
    matchpathcon_fini();
/*@=moduncon =noeffectuncon @*/
    fi = rpmfiFree(fi);
    if (fcontextp)
	*fcontextp = av;
    else
	av = _free(av);
    if (fcp) *fcp = ac;
}
#endif

void rpmfiBuildFDeps(Header h, rpmTag tagN,
	/*@out@*/ const char *** fdepsp, /*@out@*/ rpmuint32_t * fcp)
{
    int scareMem = 0;
    rpmfi fi = rpmfiNew(NULL, h, RPMTAG_BASENAMES, scareMem);
    rpmds ds = NULL;
    const char ** av;
    int ac;
    size_t nb;
    char * t;
    char deptype = 'R';
    char mydt;
    const char * DNEVR;
    const rpmuint32_t * ddict;
    unsigned ix;
    int ndx;

    if ((ac = rpmfiFC(fi)) <= 0) {
	av = NULL;
	ac = 0;
	goto exit;
    }

    if (tagN == RPMTAG_PROVIDENAME)
	deptype = 'P';
    else if (tagN == RPMTAG_REQUIRENAME)
	deptype = 'R';

    ds = rpmdsNew(h, tagN, scareMem);

    /* Compute size of file depends argv array blob. */
    nb = (ac + 1) * sizeof(*av);
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	ddict = NULL;
	ndx = rpmfiFDepends(fi, &ddict);
	if (ddict != NULL)
	while (ndx-- > 0) {
	    ix = *ddict++;
	    mydt = ((ix >> 24) & 0xff);
	    if (mydt != deptype)
		/*@innercontinue@*/ continue;
	    ix &= 0x00ffffff;
	    (void) rpmdsSetIx(ds, ix-1);
	    if (rpmdsNext(ds) < 0)
		/*@innercontinue@*/ continue;
	    DNEVR = rpmdsDNEVR(ds);
	    if (DNEVR != NULL)
		nb += strlen(DNEVR+2) + 1;
	}
	nb += 1;
    }

    /* Create and load file depends argv array. */
    av = (const char **) xmalloc(nb);
    t = ((char *) av) + ((ac + 1) * sizeof(*av));
    ac = 0;
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	av[ac++] = t;
	ddict = NULL;
	ndx = rpmfiFDepends(fi, &ddict);
	if (ddict != NULL)
	while (ndx-- > 0) {
	    ix = *ddict++;
	    mydt = ((ix >> 24) & 0xff);
	    if (mydt != deptype)
		/*@innercontinue@*/ continue;
	    ix &= 0x00ffffff;
	    (void) rpmdsSetIx(ds, ix-1);
	    if (rpmdsNext(ds) < 0)
		/*@innercontinue@*/ continue;
	    DNEVR = rpmdsDNEVR(ds);
	    if (DNEVR != NULL) {
		t = stpcpy(t, DNEVR+2);
		*t++ = ' ';
		*t = '\0';
	    }
	}
	*t++ = '\0';
    }
    av[ac] = NULL;	/* XXX tag arrays are not NULL terminated. */

exit:
    fi = rpmfiFree(fi);
    (void)rpmdsFree(ds);
    ds = NULL;
    if (fdepsp)
	*fdepsp = av;
    else
	av = _free(av);
    if (fcp) *fcp = ac;
}
