/** \ingroup payload
 * \file lib/fsm.c
 * File state machine to handle a payload from a package.
 */

#include "system.h"

#include <rpmio_internal.h>	/* XXX urlPath, fdGetCpioPos */
#include <rpmcb.h>		/* XXX fnpyKey */
#include "rpmsq.h"
#include <rpmsx.h>
#if defined(SUPPORT_AR_PAYLOADS)
#include "ar.h"
#endif
#include "cpio.h"
#include "tar.h"
#include "ugid.h"		/* XXX unameToUid() and gnameToGid() */

#include <rpmtag.h>
#include <rpmtypes.h>
#define	_RPMDB_INTERNAL
#include <rpmdb.h>

#define	_RPMFI_INTERNAL
#include "rpmfi.h"

#define	_IOSM_INTERNAL
#include <fsm.h>
#define	fsmUNSAFE	fsmStage

#define	_USE_RPMTE
#if defined(_USE_RPMTE)
#include "rpmte.h"
#endif
#include "rpmts.h"

#include "debug.h"

/*@access FD_t @*/	/* XXX void ptr args */
/*@access FSMI_t @*/
/*@access IOSM_t @*/
/*@access IOSMI_t @*/

/*@access rpmfi @*/

/*@access rpmsx @*/	/* XXX cast */
/*@access rpmte @*/	/* XXX cast */
/*@access rpmts @*/	/* XXX cast */

#ifdef __cplusplus
GENfree(const void *)
GENfree(unsigned short *)
GENfree(DNLI_t)
GENfree(FSMI_t)
#endif	/* __cplusplus */

#define	alloca_strdup(_s)	strcpy((char *)alloca(strlen(_s)+1), (_s))

#define	_FSM_DEBUG	0
/*@unchecked@*/
int _fsm_debug = _FSM_DEBUG;

/*@-exportheadervar@*/
/*@unchecked@*/
int _fsm_threads = 0;
/*@=exportheadervar@*/

/**
 * Retrieve transaction set from file state machine iterator.
 * @param fsm		file state machine
 * @return		transaction set
 */
static rpmts fsmGetTs(const IOSM_t fsm)
	/*@*/
{
    const FSMI_t iter = fsm->iter;
    /*@-compdef -refcounttrans -retexpose -usereleased @*/
    return (rpmts) (iter ? iter->ts : NULL);
    /*@=compdef =refcounttrans =retexpose =usereleased @*/
}

/**
 * Retrieve transaction element file info from file state machine iterator.
 * @param fsm		file state machine
 * @return		transaction element file info
 */
static rpmfi fsmGetFi(const IOSM_t fsm)
	/*@*/
{
    const FSMI_t iter = fsm->iter;
    /*@-compdef -refcounttrans -retexpose -usereleased @*/
    return (rpmfi) (iter ? iter->fi : NULL);
    /*@=compdef =refcounttrans =retexpose =usereleased @*/
}

#define	SUFFIX_RPMORIG	".rpmorig"
#define	SUFFIX_RPMSAVE	".rpmsave"
#define	SUFFIX_RPMNEW	".rpmnew"

/** \ingroup payload
 * Build path to file from file info, ornamented with subdir and suffix.
 * @param fsm		file state machine data
 * @param st		file stat info
 * @param subdir	subdir to use (NULL disables)
 * @param suffix	suffix to use (NULL disables)
 * @retval		path to file
 */
static /*@only@*//*@null@*/
const char * fsmFsPath(/*@special@*/ /*@null@*/ const IOSM_t fsm,
		/*@null@*/ const struct stat * st,
		/*@null@*/ const char * subdir,
		/*@null@*/ const char * suffix)
	/*@uses fsm->dirName, fsm->baseName */
	/*@*/
{
    const char * s = NULL;

    if (fsm) {
	char * t;
	int nb;
	nb = strlen(fsm->dirName) +
	    (st && !S_ISDIR(st->st_mode) ? (subdir ? strlen(subdir) : 0) : 0) +
	    (st && !S_ISDIR(st->st_mode) ? (suffix ? strlen(suffix) : 0) : 0) +
	    strlen(fsm->baseName) + 1;
	s = t = (char *) xmalloc(nb);
	t = stpcpy(t, fsm->dirName);
	if (st && !S_ISDIR(st->st_mode))
	    if (subdir) t = stpcpy(t, subdir);
	t = stpcpy(t, fsm->baseName);
	if (st && !S_ISDIR(st->st_mode))
	    if (suffix) t = stpcpy(t, suffix);
    }
    return s;
}

/** \ingroup payload
 * Destroy file info iterator.
 * @param _iter		file info iterator
 * @retval		NULL always
 */
static /*@null@*/ void * mapFreeIterator(/*@only@*//*@null@*/ void * _iter)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    FSMI_t iter = (FSMI_t) _iter;
    if (iter) {
	iter->fi = rpmfiUnlink(iter->fi, "mapIterator");
/*@-internalglobs@*/ /* XXX rpmswExit() */
	(void)rpmtsFree(iter->ts); 
	iter->ts = NULL;
/*@=internalglobs@*/
    }
    return _free(iter);
}

/** \ingroup payload
 * Create file info iterator.
 * @param fi		transaction element file info
 * @param reverse	iterate in reverse order?
 * @return		file info iterator
 */
static void *
mapInitIterator(rpmfi fi, int reverse)
	/*@modifies fi @*/
{
    FSMI_t iter = NULL;

    iter = (FSMI_t) xcalloc(1, sizeof(*iter));
/*@-assignexpose -castexpose @*/
    iter->fi = rpmfiLink(fi, "mapIterator");
/*@=assignexpose =castexpose @*/
    iter->reverse = reverse;
    iter->i = (iter->reverse ? (fi->fc - 1) : 0);
    iter->isave = iter->i;
    return iter;
}

/** \ingroup payload
 * Return next index into file info.
 * @param _iter		file info iterator
 * @return		next index, -1 on termination
 */
static int mapNextIterator(/*@null@*/ void * _iter)
	/*@*/
{
    FSMI_t iter = (FSMI_t) _iter;
    int i = -1;

    if (iter) {
/*@-onlytrans@*/
	const rpmfi fi = (rpmfi) iter->fi;
/*@=onlytrans@*/
	if (iter->reverse) {
	    if (iter->i >= 0)	i = iter->i--;
	} else {
    	    if (iter->i < (int)fi->fc)	i = iter->i++;
	}
	iter->isave = i;
    }
    return i;
}

/** \ingroup payload
 */
static int cpioStrCmp(const void * a, const void * b)
	/*@*/
{
    const char * aurl = *(const char **)a;
    const char * burl = *(const char **)b;
    const char * afn = NULL;
    const char * bfn = NULL;

    (void) urlPath(aurl, &afn);
    (void) urlPath(burl, &bfn);

#ifdef	VERY_OLD_BUGGY_RPM_PACKAGES
    /* XXX Some rpm-2.4 packages from 1997 have basename only in payloads. */
    if (strchr(afn, '/') == NULL)
	bfn = strrchr(bfn, '/') + 1;
#endif

    /* Match rpm-4.0 payloads with ./ prefixes. */
    if (afn[0] == '.' && afn[1] == '/')	afn += 2;
    if (bfn[0] == '.' && bfn[1] == '/')	bfn += 2;

    /* If either path is absolute, make it relative to '/'. */
    if (afn[0] == '/')	afn += 1;
    if (bfn[0] == '/')	bfn += 1;

    return strcmp(afn, bfn);
}

/** \ingroup payload
 * Locate archive path in file info.
 * @param iter		file info iterator
 * @param fsmPath	archive path
 * @return		index into file info, -1 if archive path was not found
 */
static int mapFind(/*@null@*/ FSMI_t iter, const char * fsmPath)
	/*@modifies iter @*/
{
    int ix = -1;

    if (iter) {
/*@-onlytrans@*/
	const rpmfi fi = (rpmfi) iter->fi;
/*@=onlytrans@*/
	size_t fc = rpmfiFC(fi);
	if (fi && fc > 0 && fi->apath && fsmPath && *fsmPath) {
	    const char ** p = NULL;

	    if (fi->apath != NULL)
		p = (const char **)
			bsearch(&fsmPath, fi->apath, fc, sizeof(fsmPath),
				cpioStrCmp);
	    if (p) {
		iter->i = p - fi->apath;
		ix = mapNextIterator(iter);
	    }
	}
    }
    return ix;
}

/** \ingroup payload
 * Directory name iterator.
 */
typedef struct dnli_s {
    rpmfi fi;
/*@only@*/ /*@null@*/
    char * active;
    int reverse;
    int isave;
    int i;
} * DNLI_t;

/** \ingroup payload
 * Destroy directory name iterator.
 * @param _dnli		directory name iterator
 * @retval		NULL always
 */
static /*@null@*/ void * dnlFreeIterator(/*@only@*//*@null@*/const void * _dnli)
	/*@modifies a @*/
{
    if (_dnli) {
	DNLI_t dnli = (DNLI_t) _dnli;
	if (dnli->active) free(dnli->active);
    }
    return _free(_dnli);
}

/** \ingroup payload
 */
static inline int dnlCount(/*@null@*/ const DNLI_t dnli)
	/*@*/
{
    return (int) (dnli ? dnli->fi->dc : 0);
}

/** \ingroup payload
 */
static inline int dnlIndex(/*@null@*/ const DNLI_t dnli)
	/*@*/
{
    return (dnli ? dnli->isave : -1);
}

/** \ingroup payload
 * Create directory name iterator.
 * @param fsm		file state machine data
 * @param reverse	traverse directory names in reverse order?
 * @return		directory name iterator
 */
/*@-usereleased@*/
static /*@only@*/ /*@null@*/
void * dnlInitIterator(/*@special@*/ const IOSM_t fsm,
		int reverse)
	/*@uses fsm->iter @*/ 
	/*@*/
{
    rpmfi fi = fsmGetFi(fsm);
    const char * dnl;
    DNLI_t dnli;
    int i, j;

    if (fi == NULL)
	return NULL;
    dnli = (DNLI_t) xcalloc(1, sizeof(*dnli));
    dnli->fi = fi;
    dnli->reverse = reverse;
    dnli->i = (int) (reverse ? fi->dc : 0);

    if (fi->dc) {
	dnli->active = (char *) xcalloc(fi->dc, sizeof(*dnli->active));

	/* Identify parent directories not skipped. */
	if ((fi = rpmfiInit(fi, 0)) != NULL)
	while ((i = rpmfiNext(fi)) >= 0) {
            if (!iosmFileActionSkipped(fi->actions[i])) dnli->active[fi->dil[i]] = 1;
	}

	/* Exclude parent directories that are explicitly included. */
	if ((fi = rpmfiInit(fi, 0)) != NULL)
	while ((i = rpmfiNext(fi)) >= 0) {
	    rpmuint32_t dil;
	    size_t dnlen, bnlen;

	    if (!S_ISDIR(fi->fmodes[i]))
		continue;

	    dil = fi->dil[i];
	    dnlen = strlen(fi->dnl[dil]);
	    bnlen = strlen(fi->bnl[i]);

	    for (j = 0; j < (int)fi->dc; j++) {
		size_t jlen;

		if (!dnli->active[j] || j == (int)dil)
		    /*@innercontinue@*/ continue;
		(void) urlPath(fi->dnl[j], &dnl);
		jlen = strlen(dnl);
		if (jlen != (dnlen+bnlen+1))
		    /*@innercontinue@*/ continue;
		if (strncmp(dnl, fi->dnl[dil], dnlen))
		    /*@innercontinue@*/ continue;
		if (strncmp(dnl+dnlen, fi->bnl[i], bnlen))
		    /*@innercontinue@*/ continue;
		if (dnl[dnlen+bnlen] != '/' || dnl[dnlen+bnlen+1] != '\0')
		    /*@innercontinue@*/ continue;
		/* This directory is included in the package. */
		dnli->active[j] = 0;
		/*@innerbreak@*/ break;
	    }
	}

	/* Print only once per package. */
	if (!reverse) {
	    j = 0;
	    for (i = 0; i < (int)fi->dc; i++) {
		if (!dnli->active[i]) continue;
		if (j == 0) {
		    j = 1;
		    rpmlog(RPMLOG_DEBUG,
	D_("========== Directories not explicitly included in package:\n"));
		}
		(void) urlPath(fi->dnl[i], &dnl);
		rpmlog(RPMLOG_DEBUG, "%10d %s\n", i, dnl);
	    }
	    if (j)
		rpmlog(RPMLOG_DEBUG, "==========\n");
	}
    }
    return dnli;
}
/*@=usereleased@*/

/** \ingroup payload
 * Return next directory name (from file info).
 * @param dnli		directory name iterator
 * @return		next directory name
 */
static /*@observer@*/ /*@null@*/
const char * dnlNextIterator(/*@null@*/ DNLI_t dnli)
	/*@modifies dnli @*/
{
    const char * dn = NULL;

    if (dnli) {
	rpmfi fi = dnli->fi;
	int i = -1;

	if (dnli->active)
	do {
	    i = (!dnli->reverse ? dnli->i++ : --dnli->i);
	} while (i >= 0 && i < (int)fi->dc && !dnli->active[i]);

	if (i >= 0 && i < (int)fi->dc)
	    dn = fi->dnl[i];
	else
	    i = -1;
	dnli->isave = i;
    }
    return dn;
}

#if defined(WITH_PTHREADS)
static void * fsmThread(void * _fsm)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies arg, fileSystem, internalState @*/
{
    IOSM_t fsm = (IOSM_t) _fsm;
/*@-unqualifiedtrans@*/
    return ((void *) ((long)fsmStage(fsm, fsm->nstage)));
/*@=unqualifiedtrans@*/
}
#endif

int fsmNext(IOSM_t fsm, iosmFileStage nstage)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    fsm->nstage = nstage;
#if defined(WITH_PTHREADS)
    if (fsm->multithreaded)
	return rpmsqJoin( rpmsqThread(fsmThread, fsm) );
#endif
    return fsmStage(fsm, fsm->nstage);
}

/** \ingroup payload
 * Save hard link in chain.
 * @param fsm		file state machine data
 * @return		Is chain only partially filled?
 */
static int saveHardLink(/*@special@*/ /*@partial@*/ IOSM_t fsm)
	/*@uses fsm->links, fsm->ix, fsm->sb, fsm->goal, fsm->nsuffix @*/
	/*@defines fsm->li @*/
	/*@releases fsm->path @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    struct stat * st = &fsm->sb;
    int rc = 0;
    int ix = -1;
    int j;

    /* Find hard link set. */
    for (fsm->li = fsm->links; fsm->li; fsm->li = fsm->li->next) {
	if (fsm->li->sb.st_ino == st->st_ino && fsm->li->sb.st_dev == st->st_dev)
	    break;
    }

    /* New hard link encountered, add new link to set. */
    if (fsm->li == NULL) {
	fsm->li = (struct hardLink_s *) xcalloc(1, sizeof(*fsm->li));
	fsm->li->next = NULL;
	fsm->li->sb = *st;	/* structure assignment */
	fsm->li->nlink = (int) st->st_nlink;
	fsm->li->linkIndex = fsm->ix;
	fsm->li->createdPath = -1;

	fsm->li->filex = (int *) xcalloc(st->st_nlink, sizeof(fsm->li->filex[0]));
	memset(fsm->li->filex, -1, (st->st_nlink * sizeof(fsm->li->filex[0])));
	fsm->li->nsuffix = (const char **) xcalloc(st->st_nlink, sizeof(*fsm->li->nsuffix));

	if (fsm->goal == IOSM_PKGBUILD)
	    fsm->li->linksLeft = (int) st->st_nlink;
	if (fsm->goal == IOSM_PKGINSTALL)
	    fsm->li->linksLeft = 0;

	/*@-kepttrans@*/
	fsm->li->next = fsm->links;
	/*@=kepttrans@*/
	fsm->links = fsm->li;
    }

    if (fsm->goal == IOSM_PKGBUILD) --fsm->li->linksLeft;
    fsm->li->filex[fsm->li->linksLeft] = fsm->ix;
    /*@-observertrans -dependenttrans@*/
    fsm->li->nsuffix[fsm->li->linksLeft] = fsm->nsuffix;
    /*@=observertrans =dependenttrans@*/
    if (fsm->goal == IOSM_PKGINSTALL) fsm->li->linksLeft++;

    if (fsm->goal == IOSM_PKGBUILD)
	return (fsm->li->linksLeft > 0);

    if (fsm->goal != IOSM_PKGINSTALL)
	return 0;

    if (!(st->st_size || fsm->li->linksLeft == (int) st->st_nlink))
	return 1;

    /* Here come the bits, time to choose a non-skipped file name. */
    {	rpmfi fi = fsmGetFi(fsm);

	for (j = fsm->li->linksLeft - 1; j >= 0; j--) {
	    ix = fsm->li->filex[j];
	    if (ix < 0 || iosmFileActionSkipped(fi->actions[ix]))
		continue;
	    break;
	}
    }

    /* Are all links skipped or not encountered yet? */
    if (ix < 0 || j < 0)
	return 1;	/* XXX W2DO? */

    /* Save the non-skipped file name and map index. */
    fsm->li->linkIndex = j;
    fsm->path = _free(fsm->path);
    fsm->ix = ix;
    rc = fsmNext(fsm, IOSM_MAP);
    return rc;
}

/** \ingroup payload
 * Destroy set of hard links.
 * @param li		set of hard links
 * @return		NULL always
 */
static /*@null@*/ void * freeHardLink(/*@only@*/ /*@null@*/ struct hardLink_s * li)
	/*@modifies li @*/
{
    if (li) {
	li->nsuffix = _free(li->nsuffix);	/* XXX elements are shared */
	li->filex = _free(li->filex);
    }
    return _free(li);
}

IOSM_t newFSM(void)
{
    IOSM_t fsm = (IOSM_t) xcalloc(1, sizeof(*fsm));
    return fsm;
}

IOSM_t freeFSM(IOSM_t fsm)
{
    if (fsm) {
	fsm->path = _free(fsm->path);
	while ((fsm->li = fsm->links) != NULL) {
	    fsm->links = fsm->li->next;
	    fsm->li->next = NULL;
	    fsm->li = freeHardLink((struct hardLink_s *)fsm->li);
	}
	fsm->dnlx = _free(fsm->dnlx);
	fsm->ldn = _free(fsm->ldn);
	fsm->iter = mapFreeIterator(fsm->iter);
    }
    return _free(fsm);
}

#if defined(SUPPORT_AR_PAYLOADS)
static int arSetup(IOSM_t fsm, rpmfi fi)
	/*@modifies fsm @*/
{
    const char * path;
    char * t;
    size_t lmtablen = 0;
    size_t nb;

    /* Calculate size of ar(1) long member table. */
    if ((fi = rpmfiInit(fi, 0)) != NULL)
    while (rpmfiNext(fi) >= 0) {
#ifdef	NOTYET
	if (fi->apath) {
	    const char * apath = NULL;
	    (void) urlPath(fi->apath[ix], &apath);
	    path = apath + fi->striplen;
	} else
#endif
	    path = rpmfiBN(fi);
	if ((nb = strlen(path)) < 15)
	    continue;
	lmtablen += nb + 1;	/* trailing \n */
    }

    /* Anything to do? */
    if (lmtablen == 0)
	return 0;

    /* Create and load ar(1) long member table. */
    fsm->lmtab = t = (char *) xmalloc(lmtablen + 1);	/* trailing \0 */
    fsm->lmtablen = lmtablen;
    fsm->lmtaboff = 0;
    if ((fi = rpmfiInit(fi, 0)) != NULL)
    while (rpmfiNext(fi) >= 0) {
#ifdef	NOTYET
	if (fi->apath) {
	    const char * apath = NULL;
	    (void) urlPath(fi->apath[ix], &apath);
	    path = apath + fi->striplen;
	} else
#endif
	    path = rpmfiBN(fi);
	if ((nb = strlen(path)) < 15)
	    continue;
	t = stpcpy(t, path);
	*t++ = '\n';
    }
    *t = '\0';
    
    return 0;
}
#endif

int fsmSetup(void * _fsm, iosmFileStage goal, const char * afmt,
		const void * _ts, const void * _fi, FD_t cfd,
		unsigned int * archiveSize, const char ** failedFile)
{
    IOSM_t fsm = (IOSM_t) _fsm;
/*@-castexpose@*/
    const rpmts ts = (const rpmts) _ts;
    const rpmfi fi = (const rpmfi) _fi;
/*@=castexpose@*/
#if defined(_USE_RPMTE)
    int reverse = (rpmteType(fi->te) == TR_REMOVED && fi->action != FA_COPYOUT);
    int adding = (rpmteType(fi->te) == TR_ADDED);
#else
    int reverse = 0;	/* XXX HACK: devise alternative means */
    int adding = 1;	/* XXX HACK: devise alternative means */
#endif
    size_t pos = 0;
    int rc, ec = 0;

    fsm->debug = _fsm_debug;
    fsm->multithreaded = _fsm_threads;
    fsm->adding = adding;

/*@+voidabstract -nullpass@*/
if (fsm->debug < 0)
fprintf(stderr, "--> fsmSetup(%p, 0x%x, \"%s\", %p, %p, %p, %p, %p)\n", fsm, goal, afmt, (void *)ts, fi, cfd, archiveSize, failedFile);
/*@=voidabstract =nullpass@*/

    _iosmNext = &fsmNext;
    if (fsm->headerRead == NULL) {
	if (afmt != NULL && (!strcmp(afmt, "tar") || !strcmp(afmt, "ustar"))) {
if (fsm->debug < 0)
fprintf(stderr, "\ttar vectors set\n");
	    fsm->headerRead = &tarHeaderRead;
	    fsm->headerWrite = &tarHeaderWrite;
	    fsm->trailerWrite = &tarTrailerWrite;
	    fsm->blksize = TAR_BLOCK_SIZE;
	} else
#if defined(SUPPORT_AR_PAYLOADS)
	if (afmt != NULL && !strcmp(afmt, "ar")) {
if (fsm->debug < 0)
fprintf(stderr, "\tar vectors set\n");
	    fsm->headerRead = &arHeaderRead;
	    fsm->headerWrite = &arHeaderWrite;
	    fsm->trailerWrite = &arTrailerWrite;
	    fsm->blksize = 2;
	    if (goal == IOSM_PKGBUILD || goal == IOSM_PKGERASE)
		(void) arSetup(fsm, fi);
	} else
#endif
	{
if (fsm->debug < 0)
fprintf(stderr, "\tcpio vectors set\n");
	    fsm->headerRead = &cpioHeaderRead;
	    fsm->headerWrite = &cpioHeaderWrite;
	    fsm->trailerWrite = &cpioTrailerWrite;
	    fsm->blksize = 4;
	}
    }

    fsm->goal = goal;
    if (cfd != NULL) {
/*@-assignexpose -castexpose @*/
	fsm->cfd = fdLink(cfd, "persist (fsm)");
/*@=assignexpose =castexpose @*/
	pos = fdGetCpioPos(fsm->cfd);
	fdSetCpioPos(fsm->cfd, 0);
    }
/*@-mods@*/	/* LCL: avoid void * _ts/_fi annotations for now. */
    fsm->iter = mapInitIterator(fi, reverse);
/*@-assignexpose -castexpose @*/
    fsm->iter->ts = rpmtsLink(ts, "mapIterator");
/*@=assignexpose =castexpose @*/
    fsm->nofcontexts = (rpmtsFlags(ts) & RPMTRANS_FLAG_NOCONTEXTS);
/*@=mods@*/
    fsm->nofdigests =
	(ts != NULL && !(rpmtsFlags(ts) & RPMTRANS_FLAG_NOFDIGESTS))
			? 0 : 1;
#define	_tsmask	(RPMTRANS_FLAG_PKGCOMMIT | RPMTRANS_FLAG_COMMIT)
    fsm->commit = ((ts && (rpmtsFlags(ts) & _tsmask) &&
			fsm->goal != IOSM_PKGCOMMIT) ? 0 : 1);
#undef _tsmask

    if (fsm->goal == IOSM_PKGINSTALL || fsm->goal == IOSM_PKGBUILD) {
	void * ptr;
	fi->archivePos = 0;
	ptr = rpmtsNotify(ts, fi->te,
		RPMCALLBACK_INST_START, fi->archivePos, fi->archiveSize);
    }

    /*@-assignexpose@*/
    fsm->archiveSize = archiveSize;
    if (fsm->archiveSize)
	*fsm->archiveSize = 0;
    fsm->failedFile = failedFile;
    if (fsm->failedFile)
	*fsm->failedFile = NULL;
    /*@=assignexpose@*/

    memset(fsm->sufbuf, 0, sizeof(fsm->sufbuf));
    if (fsm->goal == IOSM_PKGINSTALL) {
	if (ts && rpmtsGetTid(ts) != (rpmuint32_t)-1)
	    sprintf(fsm->sufbuf, ";%08x", (unsigned)rpmtsGetTid(ts));
    }

    ec = fsm->rc = 0;
/*@-mods@*/	/* LCL: avoid void * _fsm annotation for now. */
    rc = fsmUNSAFE(fsm, IOSM_CREATE);
/*@=mods@*/
    if (rc && !ec) ec = rc;

/*@-mods@*/	/* LCL: avoid void * _fsm annotation for now. */
    rc = fsmUNSAFE(fsm, fsm->goal);
/*@=mods@*/
    if (rc && !ec) ec = rc;

    if (fsm->archiveSize && ec == 0)
	*fsm->archiveSize = (fdGetCpioPos(fsm->cfd) - pos);

/*@-nullstate@*/ /* FIX: *fsm->failedFile may be NULL */
   return ec;
/*@=nullstate@*/
}

int fsmTeardown(void * _fsm)
{
    IOSM_t fsm = (IOSM_t) _fsm;
    int rc = fsm->rc;

if (fsm->debug < 0)
fprintf(stderr, "--> fsmTeardown(%p)\n", fsm);
    if (!rc)
	rc = fsmUNSAFE(fsm, IOSM_DESTROY);

	(void) rpmswAdd(rpmtsOp(fsmGetTs(fsm), RPMTS_OP_DIGEST),
			&fsm->op_digest);

    fsm->lmtab = _free(fsm->lmtab);
    (void)rpmtsFree(fsm->iter->ts); 
    fsm->iter->ts = NULL;
    fsm->iter = mapFreeIterator(fsm->iter);
    if (fsm->cfd != NULL) {
/*@-refcounttrans@*/	/* FIX: XfdFree annotation */
	fsm->cfd = fdFree(fsm->cfd, "persist (fsm)");
/*@=refcounttrans@*/
	fsm->cfd = NULL;
    }
    fsm->failedFile = NULL;
    return rc;
}

/*
 * Set file security context (if not disabled).
 * @param fsm		file state machine data
 * @return		0 always
 */
static int fsmMapFContext(IOSM_t fsm)
	/*@modifies fsm @*/
{
    fsm->fcontext = NULL;
    if (!fsm->nofcontexts) {
	fsm->fcontext = rpmsxMatch(NULL, fsm->path, fsm->sb.st_mode);
#ifdef	DYING	/* XXX SELinux file contexts not set from package content. */
	{   rpmfi fi = fsmGetFi(fsm);
	    int i = fsm->ix;

	    /* Get file security context from package. */
	    if (fi && i >= 0 && i < (int)fi->fc)
		fsm->fcontext = (fi->fcontexts ? fi->fcontexts[i] : NULL);
	}
#endif
    }
    return 0;
}

int fsmMapPath(IOSM_t fsm)
{
    rpmfi fi = fsmGetFi(fsm);	/* XXX const except for fstates */
    int teAdding = fsm->adding;
    int rc = 0;
    int i = fsm->ix;

assert(fi);	/* XXX coverity #1035927 */
    fsm->osuffix = NULL;
    fsm->nsuffix = NULL;
    fsm->astriplen = 0;
    fsm->action = FA_UNKNOWN;
    fsm->mapFlags = fi->mapflags;

    if (i >= 0 && i < (int)fi->fc) {

	fsm->astriplen = fi->astriplen;
	fsm->action = (fi->actions ? fi->actions[i] : fi->action);
	fsm->fflags = (fi->fflags ? fi->fflags[i] : fi->flags);
	fsm->mapFlags = (fi->fmapflags ? fi->fmapflags[i] : fi->mapflags);

	/* src rpms have simple base name in payload. */
	fsm->dirName = fi->dnl[fi->dil[i]];
	fsm->baseName = fi->bnl[i];

	switch (fsm->action) {
	case FA_SKIP:
	    break;
	case FA_UNKNOWN:
	    break;

	case FA_COPYOUT:
	    break;
	case FA_COPYIN:
	case FA_CREATE:
assert(teAdding);
	    break;

	case FA_SKIPNSTATE:
	    if (fi->fstates && teAdding)
		fi->fstates[i] = RPMFILE_STATE_NOTINSTALLED;
	    break;

	case FA_SKIPNETSHARED:
	    if (fi->fstates && teAdding)
		fi->fstates[i] = RPMFILE_STATE_NETSHARED;
	    break;

	case FA_SKIPCOLOR:
	    if (fi->fstates && teAdding)
		fi->fstates[i] = RPMFILE_STATE_WRONGCOLOR;
	    break;

	case FA_BACKUP:
	    if (!(fsm->fflags & RPMFILE_GHOST)) /* XXX Don't if %ghost file. */
		fsm->osuffix = (teAdding ? SUFFIX_RPMORIG : SUFFIX_RPMSAVE);
	    break;

	case FA_ALTNAME:
assert(teAdding);
	    if (!(fsm->fflags & RPMFILE_GHOST)) /* XXX Don't if %ghost file. */
		fsm->nsuffix = SUFFIX_RPMNEW;
	    break;

	case FA_SAVE:
assert(teAdding);
	    if (!(fsm->fflags & RPMFILE_GHOST)) /* XXX Don't if %ghost file. */
		fsm->osuffix = SUFFIX_RPMSAVE;
	    break;
	case FA_ERASE:
#if 0	/* XXX is this a genhdlist fix? */
	    assert(rpmteType(fi->te) == TR_REMOVED);
#endif
	    /*
	     * XXX TODO: %ghost probably shouldn't be removed, but that changes
	     * legacy rpm behavior.
	     */
	    break;
	default:
	    break;
	}

	if ((fsm->mapFlags & IOSM_MAP_PATH) || fsm->nsuffix) {
	    const struct stat * st = &fsm->sb;
	    fsm->path = _free(fsm->path);
	    fsm->path = fsmFsPath(fsm, st, fsm->subdir,
		(fsm->suffix ? fsm->suffix : fsm->nsuffix));
	}
    }
    return rc;
}

int fsmMapAttrs(IOSM_t fsm)
{
    struct stat * st = &fsm->sb;
    rpmfi fi = fsmGetFi(fsm);
    int i = fsm->ix;

    if (fi && i >= 0 && i < (int) fi->fc) {
	mode_t perms = (S_ISDIR(st->st_mode) ? fi->dperms : fi->fperms);
	mode_t finalMode = (fi->fmodes ? (mode_t)fi->fmodes[i] : perms);
	ino_t finalInode = (fi->finodes ? (ino_t)fi->finodes[i] : 0);
	dev_t finalRdev = (dev_t)(fi->frdevs ? fi->frdevs[i] : 0);
	rpmuint32_t finalMtime = (fi->fmtimes ? fi->fmtimes[i] : 0);
	uid_t uid = fi->uid;
	gid_t gid = fi->gid;

	/* Make sure OpenPKG/Mandriva RPM does not try to set file owner/group on files during
	   installation of _source_ RPMs. Instead, let it use the current
	   run-time owner/group, because most of the time the owner/group in
	   the source RPM (which is the owner/group of the files as staying on
	   the package author system) is not existing on the target system, of
	   course. */
	if (fi->fuser && unameToUid(fi->fuser[i], &uid)) {
	  if (!fi->isSource) {
	    if (fsm->goal == IOSM_PKGINSTALL)
		rpmlog(RPMLOG_WARNING,
		    _("user %s does not exist - using root\n"), fi->fuser[i]);
	    uid = 0;
	    finalMode &= ~S_ISUID;      /* turn off suid bit */
	  }
	}

	if (fi->fgroup && gnameToGid(fi->fgroup[i], &gid)) {
	  if (!fi->isSource) {
	    if (fsm->goal == IOSM_PKGINSTALL)
		rpmlog(RPMLOG_WARNING,
		    _("group %s does not exist - using root\n"), fi->fgroup[i]);
	    gid = 0;
	    finalMode &= ~S_ISGID;	/* turn off sgid bit */
	  }
	}

	if (fsm->mapFlags & IOSM_MAP_MODE)
	    st->st_mode = (st->st_mode & S_IFMT) | (finalMode & ~S_IFMT);
	if (fsm->mapFlags & IOSM_MAP_TYPE) {
	    st->st_mode = (st->st_mode & ~S_IFMT) | (finalMode & S_IFMT);
	    if ((S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
	    && st->st_nlink == 0)
		st->st_nlink = 1;
	    st->st_rdev = finalRdev;
	    st->st_ino = finalInode;
	    st->st_mtime = finalMtime;
	}
	if (fsm->mapFlags & IOSM_MAP_UID)
	    st->st_uid = uid;
	if (fsm->mapFlags & IOSM_MAP_GID)
	    st->st_gid = gid;

	/*
	 * Set file digest (if not disabled).
	 */
	if (!fsm->nofdigests) {
	    fsm->fdigestalgo = fi->digestalgo;
	    fsm->fdigest = (fi->fdigests ? fi->fdigests[i] : NULL);
	    fsm->digestlen = fi->digestlen;
	    fsm->digest = (fi->digests ? (fi->digests + (fsm->digestlen * i)) : NULL);
	} else {
	    fsm->fdigestalgo = 0;
	    fsm->fdigest = NULL;
	    fsm->digestlen = 0;
	    fsm->digest = NULL;
	}
    }
    return 0;
}

/** \ingroup payload
 * Create file from payload stream.
 * @param fsm		file state machine data
 * @return		0 on success
 */
/*@-compdef@*/
static int extractRegular(/*@special@*/ IOSM_t fsm)
	/*@uses fsm->fdigest, fsm->digest, fsm->sb, fsm->wfd  @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    const struct stat * st = &fsm->sb;
    size_t left = (size_t) st->st_size;
    int rc = 0;
    int xx;

    {	const char * fn = fsm->path;
	mode_t mode = st->st_mode;
	uint8_t * b = (uint8_t *)"";
	size_t blen = 1;
	const uint8_t * d = fsm->digest;
	size_t dlen = fsm->digestlen;
	uint32_t dalgo = fsm->fdigestalgo;

	xx = rpmlioCreat(rpmtsGetRdb(fsmGetTs(fsm)), fn, mode, b, blen, d, dlen, dalgo);
    }

    rc = fsmNext(fsm, IOSM_WOPEN);
    if (rc)
	goto exit;

    if (st->st_size > 0 && (fsm->fdigest != NULL || fsm->digest != NULL))
	fdInitDigest(fsm->wfd, fsm->fdigestalgo, 0);

    while (left) {

	fsm->wrlen = (left > fsm->wrsize ? fsm->wrsize : left);
	rc = fsmNext(fsm, IOSM_DREAD);
	if (rc)
	    goto exit;

	rc = fsmNext(fsm, IOSM_WRITE);
	if (rc)
	    goto exit;

	left -= fsm->wrnb;

	/* Notify iff progress, completion is done elsewhere */
	if (!rc && left)
	    (void) fsmNext(fsm, IOSM_NOTIFY);
    }

#ifdef	DYING
/* Measurements from installing kernel-source package:
 * +fsync
 *	total:               1      0.000000 MB    640.854524 secs
 * +fdatasync
 * 	total:               1      0.000000 MB    419.983200 secs
 * w/o fsync/fdsatasync:
 * 	total:               1      0.000000 MB     12.492918 secs
 */
    xx = fsync(Fileno(fsm->wfd));
#endif

    if (st->st_size > 0 && (fsm->fdigest || fsm->digest)) {
	void * digest = NULL;
	int asAscii = (fsm->digest == NULL ? 1 : 0);

	(void) Fflush(fsm->wfd);
	fdFiniDigest(fsm->wfd, fsm->fdigestalgo, &digest, NULL, asAscii);

	if (digest == NULL) {
	    rc = IOSMERR_DIGEST_MISMATCH;
	    goto exit;
	}

	if (fsm->digest != NULL) {
	    if (memcmp(digest, fsm->digest, fsm->digestlen))
		rc = IOSMERR_DIGEST_MISMATCH;
	} else {
	    if (strcmp(digest, fsm->fdigest))
		rc = IOSMERR_DIGEST_MISMATCH;
	}
	digest = _free(digest);
    }

exit:
    (void) fsmNext(fsm, IOSM_WCLOSE);

    return rc;
}
/*@=compdef@*/

/** \ingroup payload
 * Write next item to payload stream.
 * @param fsm		file state machine data
 * @param writeData	should data be written?
 * @return		0 on success
 */
/*@-compdef -compmempass@*/
static int writeFile(/*@special@*/ /*@partial@*/ IOSM_t fsm, int writeData)
	/*@uses fsm->path, fsm->opath, fsm->sb, fsm->osb, fsm->cfd @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    const char * path = fsm->path;
    const char * opath = fsm->opath;
    struct stat * st = &fsm->sb;
    struct stat * ost = &fsm->osb;
    size_t left;
    int xx;
    int rc;

    st->st_size = (writeData ? ost->st_size : 0);

    if (S_ISDIR(st->st_mode)) {
	st->st_size = 0;
    } else if (S_ISLNK(st->st_mode)) {
	/*
	 * While linux puts the size of a symlink in the st_size field,
	 * I don't think that's a specified standard.
	 */
	/* XXX NUL terminated result in fsm->rdbuf, len in fsm->rdnb. */
	rc = fsmUNSAFE(fsm, IOSM_READLINK);
	if (rc) goto exit;
	st->st_size = fsm->rdnb;
	fsm->lpath = xstrdup(fsm->rdbuf);	/* XXX save readlink return. */
    }

    if (fsm->mapFlags & IOSM_MAP_ABSOLUTE) {
	size_t nb= strlen(fsm->dirName) + strlen(fsm->baseName) + sizeof(".");
	char * t = (char *) alloca(nb);
	*t = '\0';
	fsm->path = t;
	if (fsm->mapFlags & IOSM_MAP_ADDDOT)
	    *t++ = '.';
	t = stpcpy( stpcpy(t, fsm->dirName), fsm->baseName);
    } else if (fsm->mapFlags & IOSM_MAP_PATH) {
	rpmfi fi = fsmGetFi(fsm);
	if (fi->apath) {
	    const char * apath = NULL;
	    (void) urlPath(fi->apath[fsm->ix], &apath);
	    fsm->path = apath + fi->striplen;
	} else
	    fsm->path = fi->bnl[fsm->ix];
    }

    rc = fsmNext(fsm, IOSM_HWRITE);
    fsm->path = path;
    if (rc) goto exit;

    if (writeData && S_ISREG(st->st_mode)) {
#if defined(HAVE_MMAP)
	char * rdbuf = NULL;
	void * mapped = (void *)-1;
	size_t nmapped = 0;
	/* XXX 128 Mb resource cap for top(1) scrutiny, MADV_DONTNEED better. */
	int use_mmap = (st->st_size <= 0x07ffffff);
#endif

	rc = fsmNext(fsm, IOSM_ROPEN);
	if (rc) goto exit;

	/* XXX unbuffered mmap generates *lots* of fdio debugging */
#if defined(HAVE_MMAP)
	if (use_mmap) {
	    mapped = mmap(NULL, st->st_size, PROT_READ, MAP_SHARED, Fileno(fsm->rfd), 0);
	    if (mapped != (void *)-1) {
		rdbuf = fsm->rdbuf;
		fsm->rdbuf = (char *) mapped;
		fsm->rdlen = nmapped = st->st_size;
#if defined(HAVE_MADVISE) && defined(MADV_DONTNEED)
		xx = madvise(mapped, nmapped, MADV_DONTNEED);
#endif
	    }
	}
#endif

	left = st->st_size;

	while (left) {
#if defined(HAVE_MMAP)
	  if (mapped != (void *)-1) {
	    fsm->rdnb = nmapped;
	  } else
#endif
	  {
	    fsm->rdlen = (left > fsm->rdsize ? fsm->rdsize : left),
	    rc = fsmNext(fsm, IOSM_READ);
	    if (rc) goto exit;
	  }

	    /* XXX DWRITE uses rdnb for I/O length. */
	    rc = fsmNext(fsm, IOSM_DWRITE);
	    if (rc) goto exit;

	    left -= fsm->wrnb;
	}

#if defined(HAVE_MMAP)
	if (mapped != (void *)-1) {
/* XXX splint misses size_t 2nd arg. */
/*@i@*/	    xx = msync(mapped, nmapped, MS_ASYNC);
#if defined(HAVE_MADVISE) && defined(MADV_DONTNEED)
	    xx = madvise(mapped, nmapped, MADV_DONTNEED);
#endif
	    xx = munmap(mapped, nmapped);
	    fsm->rdbuf = rdbuf;
	} else
#endif
	{   int fdno = Fileno(fsm->rfd);
	    if (fdno >= 0)
		xx = fsync(fdno);	/* XXX coverity #1035827 */
	}
    }

    rc = fsmNext(fsm, IOSM_PAD);
    if (rc) goto exit;

    rc = 0;

exit:
    if (fsm->rfd != NULL)
	(void) fsmNext(fsm, IOSM_RCLOSE);
/*@-dependenttrans@*/
    fsm->opath = opath;
    fsm->path = path;
/*@=dependenttrans@*/
    return rc;
}
/*@=compdef =compmempass@*/

/** \ingroup payload
 * Write set of linked files to payload stream.
 * @param fsm		file state machine data
 * @return		0 on success
 */
static int writeLinkedFile(/*@special@*/ /*@partial@*/ IOSM_t fsm)
	/*@uses fsm->path, fsm->nsuffix, fsm->ix, fsm->li, fsm->failedFile @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    const char * path = fsm->path;
    const char * lpath = fsm->lpath;
    const char * nsuffix = fsm->nsuffix;
    int iterIndex = fsm->ix;
    int ec = 0;
    int rc;
    int i;
    const char * linkpath = NULL;
    int firstfile = 1;

    fsm->path = NULL;
    fsm->lpath = NULL;
    fsm->nsuffix = NULL;
    fsm->ix = -1;

    for (i = fsm->li->nlink - 1; i >= 0; i--) {

	if (fsm->li->filex[i] < 0) continue;

	fsm->ix = fsm->li->filex[i];
/*@-compdef@*/
	rc = fsmNext(fsm, IOSM_MAP);
/*@=compdef@*/

	/* XXX tar and cpio have to do things differently. */
	if (fsm->headerWrite == tarHeaderWrite) {
	    if (firstfile) {
		const char * apath = NULL;
		char *t;
		(void) urlPath(fsm->path, &apath);
		/* Remove the buildroot prefix. */
		t = (char *) xmalloc(sizeof(".") + strlen(apath + fsm->astriplen));
		(void) stpcpy( stpcpy(t, "."), apath + fsm->astriplen);
		linkpath = t;
		firstfile = 0;
	    } else
		fsm->lpath = linkpath;

	    /* Write data after first link for tar. */
	    rc = writeFile(fsm, (fsm->lpath == NULL));
	} else {
	    /* Write data after last link for cpio. */
	    rc = writeFile(fsm, (i == 0));
	}
	if (fsm->failedFile && rc != 0 && *fsm->failedFile == NULL) {
	    ec = rc;
	    *fsm->failedFile = xstrdup(fsm->path);
	}

	fsm->path = _free(fsm->path);
	fsm->li->filex[i] = -1;
    }

/*@-dependenttrans@*/
    linkpath = _free(linkpath);
/*@=dependenttrans@*/
    fsm->ix = iterIndex;
    fsm->nsuffix = nsuffix;
    fsm->lpath = lpath;
    fsm->path = path;
    return ec;
}

/** \ingroup payload
 * Create pending hard links to existing file.
 * @param fsm		file state machine data
 * @return		0 on success
 */
/*@-compdef@*/
static int fsmMakeLinks(/*@special@*/ /*@partial@*/ IOSM_t fsm)
	/*@uses fsm->path, fsm->opath, fsm->nsuffix, fsm->ix, fsm->li @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    const char * path = fsm->path;
    const char * opath = fsm->opath;
    const char * nsuffix = fsm->nsuffix;
    int iterIndex = fsm->ix;
    int ec = 0;
    int rc;
    int i;

    fsm->path = NULL;
    fsm->opath = NULL;
    fsm->nsuffix = NULL;
    fsm->ix = -1;

    fsm->ix = fsm->li->filex[fsm->li->createdPath];
    rc = fsmNext(fsm, IOSM_MAP);
    fsm->opath = fsm->path;
    fsm->path = NULL;
    for (i = 0; i < fsm->li->nlink; i++) {
	if (fsm->li->filex[i] < 0) continue;
	if (fsm->li->createdPath == i) continue;

	fsm->ix = fsm->li->filex[i];
	fsm->path = _free(fsm->path);
	rc = fsmNext(fsm, IOSM_MAP);
	if (iosmFileActionSkipped(fsm->action)) continue;

	rc = fsmUNSAFE(fsm, IOSM_VERIFY);
	if (!rc) continue;
	if (!(rc == IOSMERR_ENOENT)) break;

	/* XXX link(fsm->opath, fsm->path) */
	rc = fsmNext(fsm, IOSM_LINK);
	if (fsm->failedFile && rc != 0 && *fsm->failedFile == NULL) {
	    ec = rc;
	    *fsm->failedFile = xstrdup(fsm->path);
	}

	fsm->li->linksLeft--;
    }
    fsm->path = _free(fsm->path);
    fsm->opath = _free(fsm->opath);

    fsm->ix = iterIndex;
    fsm->nsuffix = nsuffix;
    fsm->path = path;
    fsm->opath = opath;
    return ec;
}
/*@=compdef@*/

/** \ingroup payload
 * Commit hard linked file set atomically.
 * @param fsm		file state machine data
 * @return		0 on success
 */
/*@-compdef@*/
static int fsmCommitLinks(/*@special@*/ /*@partial@*/ IOSM_t fsm)
	/*@uses fsm->path, fsm->nsuffix, fsm->ix, fsm->sb,
		fsm->li, fsm->links @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    const char * path = fsm->path;
    const char * nsuffix = fsm->nsuffix;
    int iterIndex = fsm->ix;
    struct stat * st = &fsm->sb;
    int rc = 0;
    int i;

    fsm->path = NULL;
    fsm->nsuffix = NULL;
    fsm->ix = -1;

    for (fsm->li = fsm->links; fsm->li; fsm->li = fsm->li->next) {
	if (fsm->li->sb.st_ino == st->st_ino && fsm->li->sb.st_dev == st->st_dev)
	    break;
    }

assert(fsm->li);	/* XXX coverity #1035780 */
    for (i = 0; i < fsm->li->nlink; i++) {
	if (fsm->li->filex[i] < 0) continue;
	fsm->ix = fsm->li->filex[i];
	rc = fsmNext(fsm, IOSM_MAP);
	if (!iosmFileActionSkipped(fsm->action))
	    rc = fsmNext(fsm, IOSM_COMMIT);
	fsm->path = _free(fsm->path);
	fsm->li->filex[i] = -1;
    }

    fsm->ix = iterIndex;
    fsm->nsuffix = nsuffix;
    fsm->path = path;
    return rc;
}
/*@=compdef@*/

/**
 * Remove (if created) directories not explicitly included in package.
 * @param fsm		file state machine data
 * @return		0 on success
 */
static int fsmRmdirs(/*@special@*/ /*@partial@*/ IOSM_t fsm)
	/*@uses fsm->path, fsm->dnlx, fsm->ldn, fsm->rdbuf, fsm->iter @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    const char * path = fsm->path;
    void * dnli = dnlInitIterator(fsm, 1);
    char * dn = fsm->rdbuf;
    int dc = dnlCount(dnli);
    int rc = 0;

    fsm->path = NULL;
    dn[0] = '\0';
    /*@-observertrans -dependenttrans@*/
    if (fsm->ldn != NULL && fsm->dnlx != NULL)
    while ((fsm->path = dnlNextIterator(dnli)) != NULL) {
	size_t dnlen = strlen(fsm->path);
	char * te;

	dc = dnlIndex(dnli);
	if (fsm->dnlx[dc] < 1 || (size_t)fsm->dnlx[dc] >= dnlen)
	    continue;

	/* Copy to avoid const on fsm->path. */
	te = stpcpy(dn, fsm->path) - 1;
	fsm->path = dn;

	/* Remove generated directories. */
	/*@-usereleased@*/ /* LCL: te used after release? */
	do {
	    if (*te == '/') {
		*te = '\0';
/*@-compdef@*/
		rc = fsmNext(fsm, IOSM_RMDIR);
/*@=compdef@*/
		*te = '/';
	    }
	    if (rc)
		/*@innerbreak@*/ break;
	    te--;
	} while ((te - fsm->path) > fsm->dnlx[dc]);
	/*@=usereleased@*/
    }
    dnli = dnlFreeIterator(dnli);
    /*@=observertrans =dependenttrans@*/

    fsm->path = path;
    return rc;
}

/**
 * Create (if necessary) directories not explicitly included in package.
 * @param fsm		file state machine data
 * @return		0 on success
 */
static int fsmMkdirs(/*@special@*/ /*@partial@*/ IOSM_t fsm)
	/*@uses fsm->path, fsm->sb, fsm->osb, fsm->rdbuf, fsm->iter,
		fsm->ldn, fsm->ldnlen, fsm->ldnalloc @*/
	/*@defines fsm->dnlx, fsm->ldn @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    struct stat * st = &fsm->sb;
    struct stat * ost = &fsm->osb;
    const char * path = fsm->path;
    mode_t st_mode = st->st_mode;
    void * dnli = dnlInitIterator(fsm, 0);
    char * dn = fsm->rdbuf;
    int dc = dnlCount(dnli);
    int rc = 0;
    size_t i;

    fsm->path = NULL;

    dn[0] = '\0';
    fsm->dnlx = (unsigned short *) (dc ? xcalloc(dc, sizeof(*fsm->dnlx)) : NULL);
    /*@-observertrans -dependenttrans@*/
    if (fsm->dnlx != NULL)
    while ((fsm->path = dnlNextIterator(dnli)) != NULL) {
	size_t dnlen = strlen(fsm->path);
	char * te;

	dc = dnlIndex(dnli);
	if (dc < 0) continue;
	fsm->dnlx[dc] = (unsigned short) dnlen;
	if (dnlen <= 1)
	    continue;

	/*@-compdef -nullpass@*/	/* FIX: fsm->ldn not defined ??? */
	if (dnlen <= fsm->ldnlen && !strcmp(fsm->path, fsm->ldn))
	    continue;
	/*@=compdef =nullpass@*/

	/* Copy to avoid const on fsm->path. */
	(void) stpcpy(dn, fsm->path);
	fsm->path = dn;

	/* Assume '/' directory exists, "mkdir -p" for others if non-existent */
	(void) urlPath(dn, (const char **)&te);
	for (i = 1, te++; *te != '\0'; te++, i++) {
	    if (*te != '/')
		/*@innercontinue@*/ continue;

	    *te = '\0';

	    /* Already validated? */
	    /*@-usedef -compdef -nullpass -nullderef@*/
	    if (i < fsm->ldnlen &&
		(fsm->ldn[i] == '/' || fsm->ldn[i] == '\0') &&
		!strncmp(fsm->path, fsm->ldn, i))
	    {
		*te = '/';
		/* Move pre-existing path marker forward. */
		fsm->dnlx[dc] = (te - dn);
		/*@innercontinue@*/ continue;
	    }
	    /*@=usedef =compdef =nullpass =nullderef@*/

	    /* Validate next component of path. */
	    rc = fsmUNSAFE(fsm, IOSM_LSTAT);
	    *te = '/';

	    /* Directory already exists? */
	    if (rc == 0 && S_ISDIR(ost->st_mode)) {
		/* Move pre-existing path marker forward. */
		fsm->dnlx[dc] = (te - dn);
	    } else if (rc == IOSMERR_ENOENT) {
		rpmfi fi = fsmGetFi(fsm);
		*te = '\0';
		st->st_mode = S_IFDIR | (fi->dperms & 07777);
		rc = fsmNext(fsm, IOSM_MKDIR);
		if (!rc) {
		    /* XXX FIXME? only new dir will have context set. */
		    /* Get file security context from patterns. */
		    if (!fsm->nofcontexts) {
			fsm->fcontext =
				rpmsxMatch(NULL, fsm->path, st->st_mode);
			if (fsm->fcontext != NULL)
			    rc = fsmNext(fsm, IOSM_LSETFCON);
		    } else
			fsm->fcontext = NULL;
		    rpmlog(RPMLOG_DEBUG,
			D_("%s directory created with perms %04o, context %s.\n"),
			fsm->path, (unsigned)(st->st_mode & 07777),
			(fsm->fcontext ? fsm->fcontext : "(no context)"));
		    fsm->fcontext = _free(fsm->fcontext);
		}
		*te = '/';
	    }
	    if (rc)
		/*@innerbreak@*/ break;
	}
	if (rc) break;

	/* Save last validated path. */
/*@-compdef@*/ /* FIX: ldn/path annotations ? */
	if (fsm->ldnalloc < (dnlen + 1)) {
	    fsm->ldnalloc = dnlen + 100;
	    fsm->ldn = (char *) xrealloc(fsm->ldn, fsm->ldnalloc);
	}
	if (fsm->ldn != NULL) {	/* XXX can't happen */
	    strcpy(fsm->ldn, fsm->path);
 	    fsm->ldnlen = dnlen;
	}
/*@=compdef@*/
    }
    dnli = dnlFreeIterator(dnli);
    /*@=observertrans =dependenttrans@*/

    fsm->path = path;
    st->st_mode = st_mode;		/* XXX restore st->st_mode */
/*@-compdef@*/ /* FIX: ldn/path annotations ? */
    return rc;
/*@=compdef@*/
}

#ifdef	NOTYET
/**
 * Check for file on disk.
 * @param fsm		file state machine data
 * @return		0 on success
 */
static int fsmStat(/*@special@*/ /*@partial@*/ IOSM_t fsm)
	/*@globals fileSystem, internalState @*/
	/*@modifies fsm, fileSystem, internalState @*/
{
    int rc = 0;

    if (fsm->path != NULL) {
	int saveernno = errno;
	rc = fsmUNSAFE(fsm, (!(fsm->mapFlags & IOSM_FOLLOW_SYMLINKS)
			? IOSM_LSTAT : IOSM_STAT));
	if (rc == IOSMERR_ENOENT) {
	    errno = saveerrno;
	    rc = 0;
	    fsm->exists = 0;
	} else if (rc == 0) {
	    fsm->exists = 1;
	}
    } else {
	/* Skip %ghost files on build. */
	fsm->exists = 0;
    }
    return rc;
}
#endif

#define	IS_DEV_LOG(_x)	\
	((_x) != NULL && strlen(_x) >= (sizeof("/dev/log")-1) && \
	!strncmp((_x), "/dev/log", sizeof("/dev/log")-1) && \
	((_x)[sizeof("/dev/log")-1] == '\0' || \
	 (_x)[sizeof("/dev/log")-1] == ';'))

/*@-compmempass@*/
int fsmStage(IOSM_t fsm, iosmFileStage stage)
{
#ifdef	NOTUSED
    iosmFileStage prevStage = fsm->stage;
    const char * const prev = iosmFileStageString(prevStage);
#endif
    const char * const cur = iosmFileStageString(stage);
    struct stat * st = &fsm->sb;
    struct stat * ost = &fsm->osb;
    int saveerrno = errno;
    int rc = fsm->rc;
    int i;

#define	_fafilter(_a)	\
    (!((_a) == FA_CREATE || (_a) == FA_ERASE || (_a) == FA_COPYIN || (_a) == FA_COPYOUT) \
	? iosmFileActionString(_a) : "")

    if (stage & IOSM_DEAD) {
	/* do nothing */
    } else if (stage & IOSM_INTERNAL) {
	if (fsm->debug && !(stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s %06o%3d (%4d,%4d)%12lu %s %s\n",
		cur,
		(unsigned)st->st_mode, (int)st->st_nlink,
		(int)st->st_uid, (int)st->st_gid, (unsigned long)st->st_size,
		(fsm->path ? fsm->path : ""),
		_fafilter(fsm->action));
    } else {
	const char * apath = NULL;
	if (fsm->path)
	    (void) urlPath(fsm->path, &apath);
	fsm->stage = stage;
	if (fsm->debug || !(stage & IOSM_VERBOSE))
	    rpmlog(RPMLOG_DEBUG, "%-8s  %06o%3d (%4d,%4d)%12lu %s %s\n",
		cur,
		(unsigned)st->st_mode, (int)st->st_nlink,
		(int)st->st_uid, (int)st->st_gid, (unsigned long)st->st_size,
		(apath ? apath + fsm->astriplen : ""),
		_fafilter(fsm->action));
    }
#undef	_fafilter

    switch (stage) {
    case IOSM_UNKNOWN:
	break;
    case IOSM_PKGINSTALL:
	while (1) {
	    /* Clean fsm, free'ing memory. Read next archive header. */
	    rc = fsmUNSAFE(fsm, IOSM_INIT);

	    /* Exit on end-of-payload. */
	    if (rc == IOSMERR_HDR_TRAILER) {
		rc = 0;
		/*@loopbreak@*/ break;
	    }

	    /* Exit on error. */
	    if (rc) {
		fsm->postpone = 1;
		(void) fsmNext(fsm, IOSM_UNDO);
		/*@loopbreak@*/ break;
	    }

	    /* Extract file from archive. */
	    rc = fsmNext(fsm, IOSM_PROCESS);
	    if (rc) {
		(void) fsmNext(fsm, IOSM_UNDO);
		/*@loopbreak@*/ break;
	    }

	    /* Notify on success. */
	    (void) fsmNext(fsm, IOSM_NOTIFY);

	    rc = fsmNext(fsm, IOSM_FINI);
	    if (rc) {
		/*@loopbreak@*/ break;
	    }
	}
	break;
    case IOSM_PKGERASE:
    case IOSM_PKGCOMMIT:
	while (1) {
	    /* Clean fsm, free'ing memory. */
	    rc = fsmUNSAFE(fsm, IOSM_INIT);

	    /* Exit on end-of-payload. */
	    if (rc == IOSMERR_HDR_TRAILER) {
		rc = 0;
		/*@loopbreak@*/ break;
	    }

	    /* Rename/erase next item. */
	    if (fsmNext(fsm, IOSM_FINI))
		/*@loopbreak@*/ break;
	}
	break;
    case IOSM_PKGBUILD:
	while (1) {

	    rc = fsmUNSAFE(fsm, IOSM_INIT);

	    /* Exit on end-of-payload. */
	    if (rc == IOSMERR_HDR_TRAILER) {
		rc = 0;
		/*@loopbreak@*/ break;
	    }

	    /* Exit on error. */
	    if (rc) {
		fsm->postpone = 1;
		(void) fsmNext(fsm, IOSM_UNDO);
		/*@loopbreak@*/ break;
	    }

	    /* Copy file into archive. */
	    rc = fsmNext(fsm, IOSM_PROCESS);
	    if (rc) {
		(void) fsmNext(fsm, IOSM_UNDO);
		/*@loopbreak@*/ break;
	    }

	    /* Notify on success. */
	    (void) fsmNext(fsm, IOSM_NOTIFY);

	    if (fsmNext(fsm, IOSM_FINI))
		/*@loopbreak@*/ break;
	}

	/* Flush partial sets of hard linked files. */
	if (!(fsm->mapFlags & IOSM_ALL_HARDLINKS)) {
	    int nlink, j;
	    while ((fsm->li = fsm->links) != NULL) {
		fsm->links = fsm->li->next;
		fsm->li->next = NULL;

		/* Re-calculate link count for archive header. */
		for (j = -1, nlink = 0, i = 0; i < fsm->li->nlink; i++) {
		    if (fsm->li->filex[i] < 0)
			/*@innercontinue@*/ continue;
		    nlink++;
		    if (j == -1) j = i;
		}
		/* XXX force the contents out as well. */
		if (j != 0) {
		    fsm->li->filex[0] = fsm->li->filex[j];
		    fsm->li->filex[j] = -1;
		}
		fsm->li->sb.st_nlink = nlink;

		fsm->sb = fsm->li->sb;	/* structure assignment */
		fsm->osb = fsm->sb;	/* structure assignment */

		if (!rc) rc = writeLinkedFile(fsm);

		fsm->li = freeHardLink(fsm->li);
	    }
	}

	if (!rc)
	    rc = fsmNext(fsm, IOSM_TRAILER);

	break;
    case IOSM_CREATE:
	fsm->path = _free(fsm->path);
	fsm->lpath = _free(fsm->lpath);
	fsm->opath = _free(fsm->opath);
	fsm->dnlx = _free(fsm->dnlx);

	fsm->ldn = _free(fsm->ldn);
	fsm->ldnalloc = fsm->ldnlen = 0;

	fsm->rdsize = fsm->wrsize = 0;
	fsm->rdbuf = fsm->rdb = _free(fsm->rdb);
	fsm->wrbuf = fsm->wrb = _free(fsm->wrb);
	if (fsm->goal == IOSM_PKGINSTALL || fsm->goal == IOSM_PKGBUILD) {
	    fsm->rdsize = 16 * BUFSIZ;
	    fsm->rdbuf = fsm->rdb = (char *) xmalloc(fsm->rdsize);
	    fsm->wrsize = 16 * BUFSIZ;
	    fsm->wrbuf = fsm->wrb = (char *) xmalloc(fsm->wrsize);
	}

	fsm->mkdirsdone = 0;
	fsm->ix = -1;
	fsm->links = NULL;
	fsm->li = NULL;
	errno = 0;	/* XXX get rid of EBADF */

	/* Detect and create directories not explicitly in package. */
	if (fsm->goal == IOSM_PKGINSTALL) {
/*@-compdef@*/
	    rc = fsmNext(fsm, IOSM_MKDIRS);
/*@=compdef@*/
	    if (!rc) fsm->mkdirsdone = 1;
	}

	break;
    case IOSM_INIT:
	fsm->path = _free(fsm->path);
	fsm->lpath = _free(fsm->lpath);
	fsm->postpone = 0;
	fsm->diskchecked = fsm->exists = 0;
	fsm->subdir = NULL;
	fsm->suffix = (fsm->sufbuf[0] != '\0' ? fsm->sufbuf : NULL);
	fsm->action = FA_UNKNOWN;
	fsm->osuffix = NULL;
	fsm->nsuffix = NULL;

	if (fsm->goal == IOSM_PKGINSTALL) {
	    /* Read next header from payload, checking for end-of-payload. */
	    rc = fsmUNSAFE(fsm, IOSM_NEXT);
	}
	if (rc) break;

	/* Identify mapping index. */
	fsm->ix = ((fsm->goal == IOSM_PKGINSTALL)
		? mapFind(fsm->iter, fsm->path) : mapNextIterator(fsm->iter));

if (!(fsmGetFi(fsm)->mapflags & IOSM_PAYLOAD_LIST)) {
	/* Detect end-of-loop and/or mapping error. */
if (!(fsmGetFi(fsm)->mapflags & IOSM_PAYLOAD_EXTRACT)) {
	if (fsm->ix < 0) {
	    if (fsm->goal == IOSM_PKGINSTALL) {
#if 0
		rpmlog(RPMLOG_WARNING,
		    _("archive file %s was not found in header file list\n"),
			fsm->path);
#endif
		if (fsm->failedFile && *fsm->failedFile == NULL)
		    *fsm->failedFile = xstrdup(fsm->path);
		rc = IOSMERR_UNMAPPED_FILE;
	    } else {
		rc = IOSMERR_HDR_TRAILER;
	    }
	    break;
	}
}

	/* On non-install, mode must be known so that dirs don't get suffix. */
	if (fsm->goal != IOSM_PKGINSTALL) {
	    rpmfi fi = fsmGetFi(fsm);
	    st->st_mode = fi->fmodes[fsm->ix];
	}
}

	/* Generate file path. */
	rc = fsmNext(fsm, IOSM_MAP);
	if (rc) break;

	/* Perform lstat/stat for disk file. */
#ifdef	NOTYET
	rc = fsmStat(fsm);
#else
	if (fsm->path != NULL &&
	    !(fsm->goal == IOSM_PKGINSTALL && S_ISREG(st->st_mode)))
	{
	    rc = fsmUNSAFE(fsm, (!(fsm->mapFlags & IOSM_FOLLOW_SYMLINKS)
			? IOSM_LSTAT : IOSM_STAT));
	    if (rc == IOSMERR_ENOENT) {
		errno = saveerrno;
		rc = 0;
		fsm->exists = 0;
	    } else if (rc == 0) {
		fsm->exists = 1;
	    }
	} else {
	    /* Skip %ghost files on build. */
	    fsm->exists = 0;
	}
#endif
	fsm->diskchecked = 1;
	if (rc) break;

	/* On non-install, the disk file stat is what's remapped. */
	if (fsm->goal != IOSM_PKGINSTALL)
	    *st = *ost;			/* structure assignment */

	/* Remap file perms, owner, and group. */
	rc = fsmMapAttrs(fsm);
	if (rc) break;

	fsm->postpone = iosmFileActionSkipped(fsm->action);
	if (fsm->goal == IOSM_PKGINSTALL || fsm->goal == IOSM_PKGBUILD) {
	    /*@-evalorder@*/ /* FIX: saveHardLink can modify fsm */
	    if (S_ISREG(st->st_mode) && st->st_nlink > 1)
		fsm->postpone = saveHardLink(fsm);
	    /*@=evalorder@*/
	}
if (fsmGetFi(fsm)->mapflags & IOSM_PAYLOAD_LIST) fsm->postpone = 1;
	break;
    case IOSM_PRE:
	break;
    case IOSM_MAP:
	rc = fsmMapPath(fsm);
	break;
    case IOSM_MKDIRS:
	rc = fsmMkdirs(fsm);
	break;
    case IOSM_RMDIRS:
	if (fsm->dnlx)
	    rc = fsmRmdirs(fsm);
	break;
    case IOSM_PROCESS:
	if (fsm->postpone) {
	    if (fsm->goal == IOSM_PKGINSTALL) {
		/* XXX Skip over file body, archive headers already done. */
		if (S_ISREG(st->st_mode))
		    rc = fsmNext(fsm, IOSM_EAT);
	    }
	    break;
	}

	if (fsm->goal == IOSM_PKGBUILD) {
	    if (fsm->fflags & RPMFILE_GHOST) /* XXX Don't if %ghost file. */
		break;
	    if (S_ISREG(st->st_mode) && st->st_nlink > 1) {
		struct hardLink_s * li, * prev;

if (!(fsm->mapFlags & IOSM_ALL_HARDLINKS)) break;
		rc = writeLinkedFile(fsm);
		if (rc) break;	/* W2DO? */

		for (li = fsm->links, prev = NULL; li; prev = li, li = li->next)
		     if (li == fsm->li)
			/*@loopbreak@*/ break;

		if (prev == NULL)
		    fsm->links = fsm->li->next;
		else
		    prev->next = fsm->li->next;
		fsm->li->next = NULL;
		fsm->li = freeHardLink(fsm->li);
	    } else {
		rc = writeFile(fsm, 1);
	    }
	    break;
	}

	if (fsm->goal != IOSM_PKGINSTALL)
	    break;

	if (S_ISREG(st->st_mode) && fsm->lpath != NULL) {
	    const char * opath = fsm->opath;
	    char * t = (char *) xmalloc(strlen(fsm->lpath+1) + strlen(fsm->suffix) + 1);
	    (void) stpcpy(t, fsm->lpath+1);
	     fsm->opath = t;
	    /* XXX link(fsm->opath, fsm->path) */
	    rc = fsmNext(fsm, IOSM_LINK);
	    if (fsm->failedFile && rc != 0 && *fsm->failedFile == NULL) {
		*fsm->failedFile = xstrdup(fsm->path);
	    }
	    fsm->opath = _free(fsm->opath);
	    fsm->opath = opath;
	    break;	/* XXX so that delayed hard links get skipped. */
	}
	if (S_ISREG(st->st_mode)) {
	    const char * path = fsm->path;
	    if (fsm->osuffix)
		fsm->path = fsmFsPath(fsm, st, NULL, NULL);
	    rc = fsmUNSAFE(fsm, IOSM_VERIFY);

	    if (rc == 0 && fsm->osuffix) {
		const char * opath = fsm->opath;
		fsm->opath = fsm->path;
		fsm->path = fsmFsPath(fsm, st, NULL, fsm->osuffix);
		rc = fsmNext(fsm, IOSM_RENAME);
		if (!rc)
		    rpmlog(RPMLOG_WARNING,
			_("%s saved as %s\n"),
				(fsm->opath ? fsm->opath : ""),
				(fsm->path ? fsm->path : ""));
		fsm->path = _free(fsm->path);
		fsm->opath = opath;
	    }

	    /*@-dependenttrans@*/
	    fsm->path = path;
	    /*@=dependenttrans@*/
	    if (!(rc == IOSMERR_ENOENT)) return rc;
	    rc = extractRegular(fsm);
	} else if (S_ISDIR(st->st_mode)) {
	    mode_t st_mode = st->st_mode;
	    rc = fsmUNSAFE(fsm, IOSM_VERIFY);
	    if (rc == IOSMERR_ENOENT) {
		st->st_mode &= ~07777; 		/* XXX abuse st->st_mode */
		st->st_mode |=  00700;
		rc = fsmNext(fsm, IOSM_MKDIR);
		st->st_mode = st_mode;		/* XXX restore st->st_mode */
	    }
	} else if (S_ISLNK(st->st_mode)) {
assert(fsm->lpath != NULL);
	    /*@=dependenttrans@*/
	    rc = fsmUNSAFE(fsm, IOSM_VERIFY);
	    if (rc == IOSMERR_ENOENT)
		rc = fsmNext(fsm, IOSM_SYMLINK);
	} else if (S_ISFIFO(st->st_mode)) {
	    mode_t st_mode = st->st_mode;
	    /* This mimics cpio S_ISSOCK() behavior but probably isnt' right */
	    rc = fsmUNSAFE(fsm, IOSM_VERIFY);
	    if (rc == IOSMERR_ENOENT) {
		st->st_mode = 0000;		/* XXX abuse st->st_mode */
		rc = fsmNext(fsm, IOSM_MKFIFO);
		st->st_mode = st_mode;		/* XXX restore st->st_mode */
	    }
	} else if (S_ISCHR(st->st_mode) ||
		   S_ISBLK(st->st_mode) ||
    /*@-unrecog@*/ S_ISSOCK(st->st_mode) /*@=unrecog@*/)
	{
	    rc = fsmUNSAFE(fsm, IOSM_VERIFY);
	    if (rc == IOSMERR_ENOENT)
		rc = fsmNext(fsm, IOSM_MKNOD);
	} else {
	    /* XXX Repackaged payloads may be missing files. */
	    if (fsm->repackaged)
		break;

	    /* XXX Special case /dev/log, which shouldn't be packaged anyways */
	    if (!IS_DEV_LOG(fsm->path))
		rc = IOSMERR_UNKNOWN_FILETYPE;
	}
	if (S_ISREG(st->st_mode) && st->st_nlink > 1) {
	    fsm->li->createdPath = fsm->li->linkIndex;
	    rc = fsmMakeLinks(fsm);
	}
	break;
    case IOSM_POST:
	break;
    case IOSM_MKLINKS:
	rc = fsmMakeLinks(fsm);
	break;
    case IOSM_NOTIFY:		/* XXX move from fsm to psm -> tsm */
	if (fsm->goal == IOSM_PKGINSTALL || fsm->goal == IOSM_PKGBUILD) {
	    rpmts ts = fsmGetTs(fsm);
	    rpmfi fi = fsmGetFi(fsm);
	    void * ptr;
	    rpmuint64_t archivePos = fdGetCpioPos(fsm->cfd);
	    if (archivePos > fi->archivePos) {
		fi->archivePos = (unsigned long long) archivePos;
		ptr = rpmtsNotify(ts, fi->te, RPMCALLBACK_INST_PROGRESS,
			fi->archivePos, fi->archiveSize);
	    }
	}
	break;
    case IOSM_UNDO:
	if (fsm->postpone)
	    break;
	if (fsm->goal == IOSM_PKGINSTALL) {
	    /* XXX only erase if temp fn w suffix is in use */
	    if (fsm->sufbuf[0] != '\0')
		(void) fsmNext(fsm,
		    (S_ISDIR(st->st_mode) ? IOSM_RMDIR : IOSM_UNLINK));

#ifdef	NOTYET	/* XXX remove only dirs just created, not all. */
	    if (fsm->dnlx)
		(void) fsmNext(fsm, IOSM_RMDIRS);
#endif
	    errno = saveerrno;
	}
	if (fsm->failedFile && *fsm->failedFile == NULL)
	    *fsm->failedFile = xstrdup(fsm->path);
	break;
    case IOSM_FINI:
	if (!fsm->postpone && fsm->commit) {
	    if (fsm->goal == IOSM_PKGINSTALL)
		rc = ((S_ISREG(st->st_mode) && st->st_nlink > 1)
			? fsmCommitLinks(fsm) : fsmNext(fsm, IOSM_COMMIT));
	    if (fsm->goal == IOSM_PKGCOMMIT)
		rc = fsmNext(fsm, IOSM_COMMIT);
	    if (fsm->goal == IOSM_PKGERASE)
		rc = fsmNext(fsm, IOSM_COMMIT);
	}
	fsm->path = _free(fsm->path);
	fsm->lpath = _free(fsm->lpath);
	fsm->opath = _free(fsm->opath);
	memset(st, 0, sizeof(*st));
	memset(ost, 0, sizeof(*ost));
	break;
    case IOSM_COMMIT:
	/* Rename pre-existing modified or unmanaged file. */
	if (fsm->osuffix && fsm->diskchecked &&
	  (fsm->exists || (fsm->goal == IOSM_PKGINSTALL && S_ISREG(st->st_mode))))
	{
	    const char * opath = fsm->opath;
	    const char * path = fsm->path;
	    fsm->opath = fsmFsPath(fsm, st, NULL, NULL);
	    fsm->path = fsmFsPath(fsm, st, NULL, fsm->osuffix);
	    rc = fsmNext(fsm, IOSM_RENAME);
	    if (!rc) {
		rpmlog(RPMLOG_WARNING, _("%s saved as %s\n"),
				(fsm->opath ? fsm->opath : ""),
				(fsm->path ? fsm->path : ""));
	    }
	    fsm->path = _free(fsm->path);
	    fsm->path = path;
	    fsm->opath = _free(fsm->opath);
	    fsm->opath = opath;
	}

	/* Remove erased files. */
	if (fsm->goal == IOSM_PKGERASE) {
	    if (fsm->action == FA_ERASE) {
		rpmfi fi = fsmGetFi(fsm);
		if (S_ISDIR(st->st_mode)) {
		    rc = fsmNext(fsm, IOSM_RMDIR);
		    if (!rc) break;
		    switch (rc) {
		    case IOSMERR_ENOENT: /* XXX rmdir("/") linux 2.2.x kernel hack */
		    case IOSMERR_ENOTEMPTY:
	/* XXX make sure that build side permits %missingok on directories. */
			if (fsm->fflags & RPMFILE_MISSINGOK)
			    /*@innerbreak@*/ break;

			/* XXX common error message. */
			rpmlog(
			    (fsm->strict_erasures ? RPMLOG_ERR : RPMLOG_DEBUG),
			    _("%s rmdir of %s failed: Directory not empty\n"), 
				rpmfiTypeString(fi), fsm->path);
			/*@innerbreak@*/ break;
		    default:
			rpmlog(
			    (fsm->strict_erasures ? RPMLOG_ERR : RPMLOG_DEBUG),
				_("%s rmdir of %s failed: %s\n"),
				rpmfiTypeString(fi), fsm->path, strerror(errno));
			/*@innerbreak@*/ break;
		    }
		} else {
		    rc = fsmNext(fsm, IOSM_UNLINK);
		    if (!rc) break;
		    switch (rc) {
		    case IOSMERR_ENOENT:
			if (fsm->fflags & RPMFILE_MISSINGOK)
			    /*@innerbreak@*/ break;
			/*@fallthrough@*/
		    default:
			rpmlog(
			    (fsm->strict_erasures ? RPMLOG_ERR : RPMLOG_DEBUG),
				_(" %s: unlink of %s failed: %s\n"),
				rpmfiTypeString(fi), fsm->path, strerror(errno));
			/*@innerbreak@*/ break;
		    }
		}
	    }
	    /* XXX Failure to remove is not (yet) cause for failure. */
	    if (!fsm->strict_erasures) rc = 0;
	    break;
	}

	/* XXX Special case /dev/log, which shouldn't be packaged anyways */
if (!(fsmGetFi(fsm)->mapflags & IOSM_PAYLOAD_EXTRACT)) {
	if (!S_ISSOCK(st->st_mode) && !IS_DEV_LOG(fsm->path)) {
	    /* Rename temporary to final file name. */
	    if (!S_ISDIR(st->st_mode) &&
		(fsm->subdir || fsm->suffix || fsm->nsuffix))
	    {
		fsm->opath = fsm->path;
		fsm->path = fsmFsPath(fsm, st, NULL, fsm->nsuffix);
		rc = fsmNext(fsm, IOSM_RENAME);
		if (rc)
			(void) Unlink(fsm->opath);
		else if (fsm->nsuffix) {
		    const char * opath = fsmFsPath(fsm, st, NULL, NULL);
		    rpmlog(RPMLOG_WARNING, _("%s created as %s\n"),
				(opath ? opath : ""),
				(fsm->path ? fsm->path : ""));
		    opath = _free(opath);
		}
		fsm->opath = _free(fsm->opath);
	    }
	    /*
	     * Set file security context (if not disabled).
	     */
	    if (!rc && !getuid()) {
		rc = fsmMapFContext(fsm);
		if (!rc)
		    rc = fsmNext(fsm, IOSM_LSETFCON);
/*@-dependenttrans -observertrans @*/	/* FIX: use the SELinux free wrapper */
		fsm->fcontext = _free(fsm->fcontext);
/*@=dependenttrans =observertrans @*/
	    }
	    if (S_ISLNK(st->st_mode)) {
		if (!rc && !getuid())
		    rc = fsmNext(fsm, IOSM_LCHOWN);
	    } else {
		if (!rc && !getuid())
		    rc = fsmNext(fsm, IOSM_CHOWN);
		if (!rc)
		    rc = fsmNext(fsm, IOSM_CHMOD);
		if (!rc) {
		    time_t mtime = st->st_mtime;
		    rpmfi fi = fsmGetFi(fsm);
		    if (fi->fmtimes)
			st->st_mtime = fi->fmtimes[fsm->ix];
		    rc = fsmNext(fsm, IOSM_UTIME);
		    st->st_mtime = mtime;
		}
	    }
	}
}

	/* Notify on success. */
	if (!rc)		rc = fsmNext(fsm, IOSM_NOTIFY);
	else if (fsm->failedFile && *fsm->failedFile == NULL) {
	    *fsm->failedFile = fsm->path;
	    fsm->path = NULL;
	}
	break;
    case IOSM_DESTROY:
	fsm->path = _free(fsm->path);

	/* Check for hard links missing from payload. */
	while ((fsm->li = fsm->links) != NULL) {
	    fsm->links = fsm->li->next;
	    fsm->li->next = NULL;
	    if (fsm->goal == IOSM_PKGINSTALL &&
			fsm->commit && fsm->li->linksLeft)
	    {
		for (i = 0 ; i < fsm->li->linksLeft; i++) {
		    if (fsm->li->filex[i] < 0)
			/*@innercontinue@*/ continue;
		    rc = IOSMERR_MISSING_HARDLINK;
		    if (fsm->failedFile && *fsm->failedFile == NULL) {
			fsm->ix = fsm->li->filex[i];
			if (!fsmNext(fsm, IOSM_MAP)) {
			    *fsm->failedFile = fsm->path;
			    fsm->path = NULL;
			}
		    }
		    /*@loopbreak@*/ break;
		}
	    }
	    if (fsm->goal == IOSM_PKGBUILD &&
		(fsm->mapFlags & IOSM_ALL_HARDLINKS))
	    {
		rc = IOSMERR_MISSING_HARDLINK;
            }
	    fsm->li = freeHardLink(fsm->li);
	}
	fsm->ldn = _free(fsm->ldn);
	fsm->ldnalloc = fsm->ldnlen = 0;
	fsm->rdbuf = fsm->rdb = _free(fsm->rdb);
	fsm->wrbuf = fsm->wrb = _free(fsm->wrb);
	break;
    case IOSM_VERIFY:
	if (fsm->diskchecked && !fsm->exists) {
	    rc = IOSMERR_ENOENT;
	    break;
	}
	if (S_ISREG(st->st_mode)) {
	    char * path = (char *) alloca(strlen(fsm->path) + sizeof("-RPMDELETE"));
	    (void) stpcpy( stpcpy(path, fsm->path), "-RPMDELETE");
	    /*
	     * XXX HP-UX (and other os'es) don't permit unlink on busy
	     * XXX files.
	     */
	    fsm->opath = fsm->path;
	    fsm->path = path;
	    rc = fsmNext(fsm, IOSM_RENAME);
	    if (!rc)
		    (void) fsmNext(fsm, IOSM_UNLINK);
	    else
		    rc = IOSMERR_UNLINK_FAILED;
	    fsm->path = fsm->opath;
	    fsm->opath = NULL;
	    return (rc ? rc : IOSMERR_ENOENT);	/* XXX HACK */
	    /*@notreached@*/ break;
	} else if (S_ISDIR(st->st_mode)) {
	    if (S_ISDIR(ost->st_mode))		return 0;
	    if (S_ISLNK(ost->st_mode)) {
		rc = fsmUNSAFE(fsm, IOSM_STAT);
		if (rc == IOSMERR_ENOENT) rc = 0;
		if (rc) break;
		errno = saveerrno;
		if (S_ISDIR(ost->st_mode))	return 0;
	    }
	} else if (S_ISLNK(st->st_mode)) {
	    if (S_ISLNK(ost->st_mode)) {
	/* XXX NUL terminated result in fsm->rdbuf, len in fsm->rdnb. */
		rc = fsmUNSAFE(fsm, IOSM_READLINK);
		errno = saveerrno;
		if (rc) break;
		if (!strcmp(fsm->lpath, fsm->rdbuf))	return 0;
	    }
	} else if (S_ISFIFO(st->st_mode)) {
	    if (S_ISFIFO(ost->st_mode))		return 0;
	} else if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode)) {
	    if ((S_ISCHR(ost->st_mode) || S_ISBLK(ost->st_mode)) &&
		(ost->st_rdev == st->st_rdev))	return 0;
	} else if (S_ISSOCK(st->st_mode)) {
	    if (S_ISSOCK(ost->st_mode))		return 0;
	}
	    /* XXX shouldn't do this with commit/undo. */
	rc = 0;
	if (fsm->stage == IOSM_PROCESS) rc = fsmNext(fsm, IOSM_UNLINK);
	if (rc == 0)	rc = IOSMERR_ENOENT;
	return (rc ? rc : IOSMERR_ENOENT);	/* XXX HACK */
	/*@notreached@*/ break;

    case IOSM_UNLINK:
    {	const char * fn = fsm->path;
	uint8_t * b = (uint8_t *)"";
	size_t blen = 0;
	uint8_t * d = NULL;
	size_t dlen = 0;
	uint32_t dalgo = 0;
	FD_t fd = NULL;
	struct stat sb;
	mode_t mode;
	sb.st_mode = 0;
	if (!Lstat(fn, &sb) && S_ISREG(sb.st_mode)) {
	    fd = Fopen(fn, "r.fdio");
	    blen = sb.st_size;
	    b = mmap(NULL, blen, PROT_READ, MAP_SHARED, Fileno(fd), 0);
	}
	mode = sb.st_mode;
	rc = rpmlioUnlink(rpmtsGetRdb(fsmGetTs(fsm)), fn, mode, b, blen, d, dlen, dalgo);
	if (fd != NULL) {
/*@-observertrans@*/	/* FIX: b should be initialized to NULL, not "" */
	    (void)munmap(b, blen);
/*@=observertrans@*/
	    (void) Fclose(fd);
	    fd = NULL;
	}
    }	goto iosmcall;
    case IOSM_RENAME:
    {	const char * ofn = fsm->opath;
	const char * fn = fsm->path;
	uint8_t * b = NULL;
	size_t blen = 0;
	uint8_t * d = NULL;
	size_t dlen = 0;
	uint32_t dalgo = 0;
	FD_t fd = NULL;
	struct stat sb;
	mode_t mode;
	sb.st_mode = 0;
	if (!Lstat(fn, &sb) && S_ISREG(sb.st_mode)) {
	    fd = Fopen(fn, "r.fdio");
	    blen = sb.st_size;
	    b = mmap(NULL, blen, PROT_READ, MAP_SHARED, Fileno(fd), 0);
	}
	mode = sb.st_mode;
	rc = rpmlioRename(rpmtsGetRdb(fsmGetTs(fsm)), ofn, fn, mode, b, blen, d, dlen, dalgo);
	if (fd != NULL) {
	    (void)munmap(b, blen);
	    (void) Fclose(fd);
	    fd = NULL;
	}
    }	goto iosmcall;
    case IOSM_MKDIR:
	rc = rpmlioMkdir(rpmtsGetRdb(fsmGetTs(fsm)), fsm->path, st->st_mode);
	goto iosmcall;
    case IOSM_RMDIR:
	rc = rpmlioRmdir(rpmtsGetRdb(fsmGetTs(fsm)), fsm->path, st->st_mode);
	goto iosmcall;
    case IOSM_LSETFCON:
	/* Log iff lsetfilecon() will actually be called. */
	if (fsm->fcontext && *fsm->fcontext
	 && strcmp(fsm->fcontext, "<<none>>"))
	    rc = rpmlioLsetfilecon(rpmtsGetRdb(fsmGetTs(fsm)),
			fsm->path, fsm->fcontext);
	goto iosmcall;
    case IOSM_CHOWN:
	rc = rpmlioChown(rpmtsGetRdb(fsmGetTs(fsm)), fsm->path, st->st_uid, st->st_gid);
	goto iosmcall;
    case IOSM_LCHOWN:
	rc = rpmlioLchown(rpmtsGetRdb(fsmGetTs(fsm)), fsm->path, st->st_uid, st->st_gid);
	goto iosmcall;
    case IOSM_CHMOD:
	rc = rpmlioChmod(rpmtsGetRdb(fsmGetTs(fsm)), fsm->path, st->st_mode);
	goto iosmcall;
    case IOSM_UTIME:
	rc = rpmlioUtime(rpmtsGetRdb(fsmGetTs(fsm)), fsm->path, st->st_mtime, st->st_mtime);
	goto iosmcall;
    case IOSM_SYMLINK:
	rc = rpmlioSymlink(rpmtsGetRdb(fsmGetTs(fsm)), fsm->lpath, fsm->path);
	goto iosmcall;
    case IOSM_LINK:
	rc = rpmlioLink(rpmtsGetRdb(fsmGetTs(fsm)), fsm->opath, fsm->path);
	goto iosmcall;
    case IOSM_MKFIFO:
	rc = rpmlioMkfifo(rpmtsGetRdb(fsmGetTs(fsm)), fsm->path, st->st_mode);
	goto iosmcall;
    case IOSM_MKNOD:
	rc = rpmlioMknod(rpmtsGetRdb(fsmGetTs(fsm)), fsm->path, st->st_mode, st->st_rdev);
	goto iosmcall;
    case IOSM_LSTAT:
    case IOSM_STAT:
    case IOSM_READLINK:
    case IOSM_CHROOT:
iosmcall:
	rc = iosmStage(fsm, stage);
	break;

    case IOSM_NEXT:
    case IOSM_EAT:
    case IOSM_POS:
    case IOSM_PAD:
    case IOSM_TRAILER:
    case IOSM_HREAD:
    case IOSM_HWRITE:
    case IOSM_DREAD:
    case IOSM_DWRITE:
	rc = iosmStage(fsm, stage);
	break;

    case IOSM_ROPEN:
    case IOSM_READ:
    case IOSM_RCLOSE:
	rc = iosmStage(fsm, stage);
	break;
    case IOSM_WOPEN:
    case IOSM_WRITE:
    case IOSM_WCLOSE:
	rc = iosmStage(fsm, stage);
	break;

    default:
	break;
    }

    if (!(stage & IOSM_INTERNAL)) {
	fsm->rc = (rc == IOSMERR_HDR_TRAILER ? 0 : rc);
    }
    return rc;
}
/*@=compmempass@*/
