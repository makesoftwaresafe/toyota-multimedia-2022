/** \ingroup payload
 * \file rpmio/iosm.c
 * File state machine to handle a payload from a package.
 */

#include "system.h"

#include <rpmio_internal.h>	/* XXX urlPath, fdGetCpioPos */
#include <rpmcb.h>		/* XXX fnpyKey */
#include <ugid.h>		/* XXX unameToUid() and gnameToGid() */

#include <rpmsq.h>		/* XXX rpmsqJoin()/rpmsqThread() */
#include <rpmsw.h>		/* XXX rpmswAdd() */
#include <rpmsx.h>

#include "../rpmdb/rpmtag.h"

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmts_s * rpmts;
typedef /*@abstract@*/ struct rpmte_s * rpmte;

#define	_IOSM_INTERNAL
#include <iosm.h>
#define	iosmUNSAFE	iosmStage

#include "cpio.h"
#include "tar.h"
#include "ar.h"

typedef iosmMapFlags cpioMapFlags;
#define	_RPMFI_INTERNAL
#define	_RPMFI_NOMETHODS
#include "../lib/rpmfi.h"

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmds_s * rpmds;
typedef struct rpmRelocation_s * rpmRelocation;
#undef	_USE_RPMTE
#if defined(_USE_RPMTE)
typedef /*@abstract@*/ void * alKey;
#include "rpmte.h"
#endif

#undef	_USE_RPMSX
#if defined(_USE_RPMSX)
#include "../lib/rpmsx.h"		/* XXX rpmsx rpmsxFContext rpmsxFree */
#endif

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmdb_s * rpmdb;
typedef /*@abstract@*/ struct rpmmi_s * rpmmi;
typedef struct rpmPRCO_s * rpmPRCO;
typedef struct Spec_s * Spec;
#undef	_USE_RPMTS
#if defined(_USE_RPMTS)
#include "rpmts.h"
#endif

#include "debug.h"

/*@access FD_t @*/	/* XXX void ptr args */
/*@access IOSMI_t @*/
/*@access IOSM_t @*/

/*@access rpmfi @*/

#ifdef __cplusplus
GENfree(unsigned short *)
GENfree(int *)
GENfree(IOSM_t)
GENfree(iosmFileAction *)
#endif	/* __cplusplus */

#define	alloca_strdup(_s)	strcpy((char *)alloca(strlen(_s)+1), (_s))

#define	_IOSM_DEBUG	0
/*@unchecked@*/
int _iosm_debug = _IOSM_DEBUG;

/*@-exportheadervar@*/
/*@unchecked@*/
int _iosm_threads = 0;
/*@=exportheadervar@*/

/*@-redecl@*/
int (*_iosmNext) (IOSM_t iosm, iosmFileStage nstage)
        /*@modifies iosm @*/ = &iosmNext;
/*@=redecl@*/

#if defined(_USE_RPMTS)
void * iosmGetTs(const IOSM_t iosm)
{
    const IOSMI_t iter = iosm->iter;
    /*@-compdef -refcounttrans -retexpose -usereleased @*/
    return (iter ? iter->ts : NULL);
    /*@=compdef =refcounttrans =retexpose =usereleased @*/
}
#endif

void * iosmGetFi(const IOSM_t iosm)
{
    const IOSMI_t iter = iosm->iter;
    /*@-compdef -refcounttrans -retexpose -usereleased @*/
    return (iter ? iter->fi : NULL);
    /*@=compdef =refcounttrans =retexpose =usereleased @*/
}

#define	SUFFIX_RPMORIG	".rpmorig"
#define	SUFFIX_RPMSAVE	".rpmsave"
#define	SUFFIX_RPMNEW	".rpmnew"

/** \ingroup payload
 * Build path to file from file info, ornamented with subdir and suffix.
 * @param iosm		file state machine data
 * @param st		file stat info
 * @param subdir	subdir to use (NULL disables)
 * @param suffix	suffix to use (NULL disables)
 * @retval		path to file
 */
static /*@only@*//*@null@*/
const char * iosmFsPath(/*@special@*/ /*@null@*/ const IOSM_t iosm,
		/*@null@*/ const struct stat * st,
		/*@null@*/ const char * subdir,
		/*@null@*/ const char * suffix)
	/*@uses iosm->dirName, iosm->baseName */
	/*@*/
{
    const char * s = NULL;

    if (iosm) {
	char * t;
	int nb;
	nb = strlen(iosm->dirName) +
	    (st && !S_ISDIR(st->st_mode) ? (subdir ? strlen(subdir) : 0) : 0) +
	    (st && !S_ISDIR(st->st_mode) ? (suffix ? strlen(suffix) : 0) : 0) +
	    strlen(iosm->baseName) + 1;
	s = t = (char *) xmalloc(nb);
	t = stpcpy(t, iosm->dirName);
	if (st && !S_ISDIR(st->st_mode))
	    if (subdir) t = stpcpy(t, subdir);
	t = stpcpy(t, iosm->baseName);
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
	/*@modifies p @*/
{
    IOSMI_t iter = (IOSMI_t) _iter;
    if (iter) {
#if !defined(_RPMFI_NOMETHODS)
	iter->fi = rpmfiUnlink(iter->fi, "mapIterator");
#endif
	iter->fi = NULL;
    }
    return _free(iter);
}

/** \ingroup payload
 * Create file info iterator.
 * @param fi		transaction element file info
 * @param reverse	iterate in reverse order?
 * @return		file info iterator
 */
/*@-mustmod@*/
static void *
mapInitIterator(rpmfi fi, int reverse)
	/*@modifies fi @*/
{
    IOSMI_t iter = NULL;

    iter = (IOSMI_t) xcalloc(1, sizeof(*iter));
#if !defined(_RPMFI_NOMETHODS)
    iter->fi = rpmfiLink(fi, "mapIterator");
#else
/*@i@*/ iter->fi = fi;
#endif
    iter->reverse = reverse;
    iter->i = (iter->reverse ? (fi->fc - 1) : 0);
    iter->isave = iter->i;
    return iter;
}
/*@=mustmod@*/

/** \ingroup payload
 * Return next index into file info.
 * @param _iter		file info iterator
 * @return		next index, -1 on termination
 */
static int mapNextIterator(/*@null@*/ void * _iter)
	/*@*/
{
    IOSMI_t iter = (IOSMI_t) _iter;
    int i = -1;

    if (iter) {
	if (iter->reverse) {
	    if (iter->i >= 0)	i = iter->i--;
	} else {
    	    if (iter->i < (int) ((rpmfi)iter->fi)->fc)	i = iter->i++;
	}
	iter->isave = i;
    }
    return i;
}

/** \ingroup payload
 */
static int iosmStrCmp(const void * a, const void * b)
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
 * @param iosmPath	archive path
 * @return		index into file info, -1 if archive path was not found
 */
static int mapFind(/*@null@*/ IOSMI_t iter, const char * iosmPath)
	/*@modifies iter @*/
{
    int ix = -1;

    if (iter) {
/*@-onlytrans@*/
	const rpmfi fi = (rpmfi) iter->fi;
/*@=onlytrans@*/
#if !defined(_RPMFI_NOMETHODS)
	size_t fc = rpmfiFC(fi);
#else
	size_t fc = (fi ? fi->fc : 0);
#endif
	if (fi && fc > 0 && fi->apath && iosmPath && *iosmPath) {
	    const char ** p = NULL;

	    if (fi->apath != NULL)
		p = (const char **)
			bsearch(&iosmPath, fi->apath, fc, sizeof(iosmPath),
				iosmStrCmp);
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
static /*@null@*/ void * dnlFreeIterator(/*@only@*//*@null@*/ const void * _dnli)
	/*@modifies _dnli @*/
{
    if (_dnli) {
	DNLI_t dnli = (DNLI_t)_dnli;
	if (dnli->active) free(dnli->active);
    }
    return _free((void *)_dnli);
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
 * @param iosm		file state machine data
 * @param reverse	traverse directory names in reverse order?
 * @return		directory name iterator
 */
/*@-usereleased@*/
static /*@only@*/ /*@null@*/
void * dnlInitIterator(/*@special@*/ const IOSM_t iosm,
		int reverse)
	/*@uses iosm->iter @*/ 
	/*@*/
{
    rpmfi fi = (rpmfi) iosmGetFi(iosm);
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
#if !defined(_RPMFI_NOMETHODS)
	if ((fi = rpmfiInit(fi, 0)) != NULL)
	while ((i = rpmfiNext(fi)) >= 0)
#else
	for (i = 0; i < (int)fi->fc; i++)
#endif
	{
            if (!iosmFileActionSkipped((iosmFileAction)fi->actions[i]))
		dnli->active[fi->dil[i]] = (char)1;
	}

	/* Exclude parent directories that are explicitly included. */
#if !defined(_RPMFI_NOMETHODS)
	if ((fi = rpmfiInit(fi, 0)) != NULL)
	while ((i = rpmfiNext(fi)) >= 0)
#else
	for (i = 0; i < (int)fi->fc; i++)
#endif
	{
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
		dnli->active[j] = (char)0;
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
static void * iosmThread(void * _iosm)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies _iosm, fileSystem, internalState @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
/*@-unqualifiedtrans@*/
    return ((void *) ((long)iosmStage(iosm, iosm->nstage)));
/*@=unqualifiedtrans@*/
}
#endif

int iosmNext(IOSM_t iosm, iosmFileStage nstage)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    iosm->nstage = nstage;
#if defined(WITH_PTHREADS)
    if (iosm->multithreaded)
	return rpmsqJoin( rpmsqThread(iosmThread, iosm) );
#endif
    return iosmStage(iosm, iosm->nstage);
}

/** \ingroup payload
 * Save hard link in chain.
 * @param iosm		file state machine data
 * @return		Is chain only partially filled?
 */
static int saveHardLink(/*@special@*/ /*@partial@*/ IOSM_t iosm)
	/*@uses iosm->links, iosm->ix, iosm->sb, iosm->goal, iosm->nsuffix @*/
	/*@defines iosm->li @*/
	/*@releases iosm->path @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    struct stat * st = &iosm->sb;
    int rc = 0;
    int ix = -1;
    int j;

    /* Find hard link set. */
    for (iosm->li = iosm->links; iosm->li; iosm->li = iosm->li->next) {
	if (iosm->li->sb.st_ino == st->st_ino && iosm->li->sb.st_dev == st->st_dev)
	    break;
    }

    /* New hard link encountered, add new link to set. */
    if (iosm->li == NULL) {
	iosm->li = (struct hardLink_s *) xcalloc(1, sizeof(*iosm->li));
	iosm->li->next = NULL;
	iosm->li->sb = *st;	/* structure assignment */
	iosm->li->nlink = (int) st->st_nlink;
	iosm->li->linkIndex = iosm->ix;
	iosm->li->createdPath = -1;

	iosm->li->filex = (int *) xcalloc(st->st_nlink, sizeof(iosm->li->filex[0]));
	memset(iosm->li->filex, -1, (st->st_nlink * sizeof(iosm->li->filex[0])));
	iosm->li->nsuffix = (const char **) xcalloc(st->st_nlink, sizeof(*iosm->li->nsuffix));

	if (iosm->goal == IOSM_PKGBUILD)
	    iosm->li->linksLeft = (int) st->st_nlink;
	if (iosm->goal == IOSM_PKGINSTALL)
	    iosm->li->linksLeft = 0;

	/*@-kepttrans@*/
	iosm->li->next = iosm->links;
	/*@=kepttrans@*/
	iosm->links = iosm->li;
    }

    if (iosm->goal == IOSM_PKGBUILD) --iosm->li->linksLeft;
    iosm->li->filex[iosm->li->linksLeft] = iosm->ix;
    /*@-observertrans -dependenttrans@*/
    iosm->li->nsuffix[iosm->li->linksLeft] = iosm->nsuffix;
    /*@=observertrans =dependenttrans@*/
    if (iosm->goal == IOSM_PKGINSTALL) iosm->li->linksLeft++;

    if (iosm->goal == IOSM_PKGBUILD)
	return (iosm->li->linksLeft > 0);

    if (iosm->goal != IOSM_PKGINSTALL)
	return 0;

    if (!(st->st_size || iosm->li->linksLeft == (int) st->st_nlink))
	return 1;

    /* Here come the bits, time to choose a non-skipped file name. */
    {	rpmfi fi = (rpmfi) iosmGetFi(iosm);

	for (j = iosm->li->linksLeft - 1; j >= 0; j--) {
	    ix = iosm->li->filex[j];
	    if (ix < 0 || iosmFileActionSkipped((iosmFileAction)fi->actions[ix]))
		continue;
	    break;
	}
    }

    /* Are all links skipped or not encountered yet? */
    if (ix < 0 || j < 0)
	return 1;	/* XXX W2DO? */

    /* Save the non-skipped file name and map index. */
    iosm->li->linkIndex = j;
    iosm->path = _free(iosm->path);
    iosm->ix = ix;
    rc = iosmNext(iosm, IOSM_MAP);
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

IOSM_t newIOSM(void)
{
    IOSM_t iosm = (IOSM_t) xcalloc(1, sizeof(*iosm));
    return iosm;
}

IOSM_t freeIOSM(IOSM_t iosm)
{
    if (iosm) {
	iosm->path = _free(iosm->path);
	while ((iosm->li = iosm->links) != NULL) {
	    iosm->links = iosm->li->next;
	    iosm->li->next = NULL;
	    iosm->li = (struct hardLink_s *) freeHardLink((struct hardLink_s *) iosm->li);
	}
	iosm->dnlx = _free(iosm->dnlx);
	iosm->ldn = _free(iosm->ldn);
	iosm->iter = (IOSMI_t) mapFreeIterator((IOSMI_t)iosm->iter);
    }
    return _free(iosm);
}

static int arSetup(IOSM_t iosm, rpmfi fi)
	/*@modifies iosm @*/
{
    const char * path;
    char * t;
    size_t lmtablen = 0;
    size_t nb;

    /* Calculate size of ar(1) long member table. */
#if !defined(_RPMFI_NOMETHODS)
    if ((fi = rpmfiInit(fi, 0)) != NULL)
    while (rpmfiNext(fi) >= 0)
#else
    int i;
    if (fi != NULL)
    for (i = 0; i < (int)fi->fc; i++)
#endif
    {
#ifdef	NOTYET
	if (fi->apath) {
	    const char * apath = NULL;
	    (void) urlPath(fi->apath[ix], &apath);
	    path = apath + fi->striplen;
	} else
#endif
#if !defined(_RPMFI_NOMETHODS)
	    path = rpmfiBN(fi);
#else
	    path = fi->bnl[i];
#endif
	if ((nb = strlen(path)) < 15)
	    continue;
	lmtablen += nb + 1;	/* trailing \n */
    }

    /* Anything to do? */
    if (lmtablen == 0)
	return 0;

    /* Create and load ar(1) long member table. */
    iosm->lmtab = t = (char *) xmalloc(lmtablen + 1);	/* trailing \0 */
    iosm->lmtablen = lmtablen;
    iosm->lmtaboff = 0;
#if !defined(_RPMFI_NOMETHODS)
    if ((fi = rpmfiInit(fi, 0)) != NULL)
    while (rpmfiNext(fi) >= 0)
#else
    if (fi != NULL)
    for (i = 0; i < (int)fi->fc; i++)
#endif
    {
#ifdef	NOTYET
	if (fi->apath) {
	    const char * apath = NULL;
	    (void) urlPath(fi->apath[ix], &apath);
	    path = apath + fi->striplen;
	} else
#endif
#if !defined(_RPMFI_NOMETHODS)
	    path = rpmfiBN(fi);
#else
	    path = fi->bnl[i];
#endif
	if ((nb = strlen(path)) < 15)
	    continue;
	t = stpcpy(t, path);
	*t++ = '\n';
    }
    *t = '\0';
    
    return 0;
}

int iosmSetup(IOSM_t iosm, iosmFileStage goal, const char * afmt,
		const void * _ts, const void * _fi, FD_t cfd,
		unsigned int * archiveSize, const char ** failedFile)
{
#if defined(_USE_RPMTS)
    const rpmts ts = (const rpmts) _ts;
#endif
/*@i@*/ const rpmfi fi = (const rpmfi) _fi;
#if defined(_USE_RPMTE)
    int reverse = (rpmteType(fi->te) == TR_REMOVED && fi->action != FA_COPYOUT);
    int adding = (rpmteType(fi->te) == TR_ADDED);
#else
    int reverse = 0;	/* XXX HACK: devise alternative means */
    int adding = 1;	/* XXX HACK: devise alternative means */
#endif
    size_t pos = 0;
    int rc, ec = 0;

    iosm->debug = _iosm_debug;
    iosm->multithreaded = _iosm_threads;
    iosm->adding = adding;

/*@+voidabstract -nullpass@*/
if (iosm->debug < 0)
fprintf(stderr, "--> iosmSetup(%p, 0x%x, \"%s\", %p, %p, %p, %p, %p)\n", iosm, goal, afmt, (void *)_ts, _fi, cfd, archiveSize, failedFile);
/*@=voidabstract =nullpass@*/

    _iosmNext = &iosmNext;
    if (iosm->headerRead == NULL) {
	if (afmt != NULL && (!strcmp(afmt, "tar") || !strcmp(afmt, "ustar"))) {
if (iosm->debug < 0)
fprintf(stderr, "\ttar vectors set\n");
	    iosm->headerRead = &tarHeaderRead;
	    iosm->headerWrite = &tarHeaderWrite;
	    iosm->trailerWrite = &tarTrailerWrite;
	    iosm->blksize = TAR_BLOCK_SIZE;
	} else
	if (afmt != NULL && !strcmp(afmt, "ar")) {
if (iosm->debug < 0)
fprintf(stderr, "\tar vectors set\n");
	    iosm->headerRead = &arHeaderRead;
	    iosm->headerWrite = &arHeaderWrite;
	    iosm->trailerWrite = &arTrailerWrite;
	    iosm->blksize = 2;
	    if (goal == IOSM_PKGBUILD || goal == IOSM_PKGERASE)
		(void) arSetup(iosm, fi);
	} else
	{
if (iosm->debug < 0)
fprintf(stderr, "\tcpio vectors set\n");
	    iosm->headerRead = &cpioHeaderRead;
	    iosm->headerWrite = &cpioHeaderWrite;
	    iosm->trailerWrite = &cpioTrailerWrite;
	    iosm->blksize = 4;
	}
    }

    iosm->goal = goal;
    if (cfd != NULL) {
/*@-assignexpose@*/
	iosm->cfd = fdLink(cfd, "persist (iosm)");
/*@=assignexpose@*/
	pos = fdGetCpioPos(iosm->cfd);
	fdSetCpioPos(iosm->cfd, 0);
    }
/*@-mods@*/	/* WTF? */
    iosm->iter = (IOSMI_t) mapInitIterator(fi, reverse);
/*@=mods@*/
#if defined(_USE_RPMTS)
    iosm->iter->ts = rpmtsLink(ts, "mapIterator");
    iosm->nofcontexts = (rpmtsFlags(ts) & RPMTRANS_FLAG_NOCONTEXTS);
    iosm->nofdigests =
	(ts != NULL && !(rpmtsFlags(ts) & RPMTRANS_FLAG_NOFDIGESTS))
			? 0 : 1;
#define	_tsmask	(RPMTRANS_FLAG_PKGCOMMIT | RPMTRANS_FLAG_COMMIT)
    iosm->commit = ((ts && (rpmtsFlags(ts) & _tsmask) &&
			iosm->goal != IOSM_PKGCOMMIT) ? 0 : 1);
#undef _tsmask
#else
/*@-assignexpose -temptrans @*/
    iosm->iter->ts = (void *)_ts;
/*@=assignexpose =temptrans @*/
    iosm->nofcontexts = 1;
    iosm->nofdigests = 1;
    iosm->commit = 1;
#endif

#if defined(_USE_RPMTS)
    if (iosm->goal == IOSM_PKGINSTALL || iosm->goal == IOSM_PKGBUILD) {
	fi->archivePos = 0;
	(void) rpmtsNotify(ts, fi->te,
		RPMCALLBACK_INST_START, fi->archivePos, fi->archiveSize);
    }
#endif

    /*@-assignexpose@*/
    iosm->archiveSize = archiveSize;
    if (iosm->archiveSize)
	*iosm->archiveSize = 0;
    iosm->failedFile = failedFile;
    if (iosm->failedFile)
	*iosm->failedFile = NULL;
    /*@=assignexpose@*/

    memset(iosm->sufbuf, 0, sizeof(iosm->sufbuf));
    if (iosm->goal == IOSM_PKGINSTALL) {
	rpmuint32_t tid;
#if defined(_USE_RPMTS)
	tid = (ts != NULL ? rpmtsGetTid(ts) : 0);
#else
	static time_t now = 0;
	if (now == 0) now = time(NULL);
	tid = (rpmuint32_t) now;
#endif
	if (tid > 0 && tid < 0xffffffff)
	    sprintf(iosm->sufbuf, ";%08x", (unsigned)tid);
    }

    ec = iosm->rc = 0;
    rc = iosmUNSAFE(iosm, IOSM_CREATE);
    if (rc && !ec) ec = rc;

    rc = iosmUNSAFE(iosm, iosm->goal);
    if (rc && !ec) ec = rc;

    if (iosm->archiveSize && ec == 0)
	*iosm->archiveSize = (fdGetCpioPos(iosm->cfd) - pos);

/*@-nullstate@*/ /* FIX: *iosm->failedFile may be NULL */
   return ec;
/*@=nullstate@*/
}

int iosmTeardown(IOSM_t iosm)
{
    int rc = iosm->rc;

if (iosm->debug < 0)
fprintf(stderr, "--> iosmTeardown(%p)\n", iosm);
    if (!rc)
	rc = iosmUNSAFE(iosm, IOSM_DESTROY);

    iosm->lmtab = _free(iosm->lmtab);

    if (iosm->iter != NULL) {
#if defined(_USE_RPMTS)
	(void) rpmswAdd(rpmtsOp(iosmGetTs(iosm), RPMTS_OP_DIGEST),
			&iosm->op_digest);
	(void)rpmtsFree(iosm->iter->ts); 
#endif
	iosm->iter->ts = NULL;
	mapFreeIterator(iosm->iter); iosm->iter = NULL;
    }
    if (iosm->cfd != NULL) {
	iosm->cfd = fdFree(iosm->cfd, "persist (iosm)");
	iosm->cfd = NULL;
    }
    iosm->failedFile = NULL;
    return rc;
}

/*
 * Set file security context (if not disabled).
 * @param iosm		file state machine data
 * @return		0 always
 */
static int iosmMapFContext(IOSM_t iosm)
	/*@modifies iosm @*/
{
    /*
     * Find file security context (if not disabled).
     */
    iosm->fcontext = NULL;
    if (!iosm->nofcontexts) {
	iosm->fcontext = rpmsxMatch(NULL, iosm->path, iosm->sb.st_mode);
#ifdef	DYING	/* XXX SELinux file contexts not set from package content. */
	{   rpmfi fi = (rpmfi) iosmGetFi(iosm);
	    int i = iosm->ix;

	    /* Get file security context from package. */
	    if (fi && i >= 0 && i < (int)fi->fc)
		iosm->fcontext = (fi->fcontexts ? fi->fcontexts[i] : NULL);
	}
#endif
/*@=moduncon@*/
    }
    return 0;
}

int iosmMapPath(IOSM_t iosm)
{
    rpmfi fi = (rpmfi) iosmGetFi(iosm);	/* XXX const except for fstates */
    int teAdding = iosm->adding;
    int rc = 0;
    int i = iosm->ix;

    iosm->osuffix = NULL;
    iosm->nsuffix = NULL;
    iosm->astriplen = 0;
    iosm->action = FA_UNKNOWN;
    iosm->mapFlags = (iosmMapFlags) (fi ? fi->mapflags : 0);

    if (fi && i >= 0 && i < (int)fi->fc) {

	iosm->astriplen = fi->astriplen;
	iosm->action = (iosmFileAction) (fi->actions ? fi->actions[i] : fi->action);
	iosm->fflags = (fi->fflags ? fi->fflags[i] : fi->flags);
	iosm->mapFlags = (iosmMapFlags) (fi->fmapflags ? fi->fmapflags[i] : fi->mapflags);

	/* src rpms have simple base name in payload. */
	iosm->dirName = fi->dnl[fi->dil[i]];
	iosm->baseName = fi->bnl[i];

	switch (iosm->action) {
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
		fi->fstates[i] = (rpmuint8_t)RPMFILE_STATE_NOTINSTALLED;
	    break;

	case FA_SKIPNETSHARED:
	    if (fi->fstates && teAdding)
		fi->fstates[i] = (rpmuint8_t)RPMFILE_STATE_NETSHARED;
	    break;

	case FA_SKIPCOLOR:
	    if (fi->fstates && teAdding)
		fi->fstates[i] = (rpmuint8_t)RPMFILE_STATE_WRONGCOLOR;
	    break;

	case FA_BACKUP:
	    if (!(iosm->fflags & RPMFILE_GHOST)) /* XXX Don't if %ghost file. */
		iosm->osuffix = (teAdding ? SUFFIX_RPMORIG : SUFFIX_RPMSAVE);
	    break;

	case FA_ALTNAME:
assert(teAdding);
	    if (!(iosm->fflags & RPMFILE_GHOST)) /* XXX Don't if %ghost file. */
		iosm->nsuffix = SUFFIX_RPMNEW;
	    break;

	case FA_SAVE:
assert(teAdding);
	    if (!(iosm->fflags & RPMFILE_GHOST)) /* XXX Don't if %ghost file. */
		iosm->osuffix = SUFFIX_RPMSAVE;
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

	if ((iosm->mapFlags & IOSM_MAP_PATH) || iosm->nsuffix) {
	    const struct stat * st = &iosm->sb;
	    iosm->path = _free(iosm->path);
	    iosm->path = iosmFsPath(iosm, st, iosm->subdir,
		(iosm->suffix ? iosm->suffix : iosm->nsuffix));
	}
    }
    return rc;
}

int iosmMapAttrs(IOSM_t iosm)
{
    struct stat * st = &iosm->sb;
    rpmfi fi = (rpmfi) iosmGetFi(iosm);
    int i = iosm->ix;

    if (fi && i >= 0 && i < (int)fi->fc) {
	mode_t perms = (S_ISDIR(st->st_mode) ? fi->dperms : fi->fperms);
	mode_t finalMode = (fi->fmodes ? (mode_t)fi->fmodes[i] : perms);
	ino_t finalInode = (fi->finodes ? (ino_t)fi->finodes[i] : 0);
	dev_t finalRdev = (fi->frdevs ? fi->frdevs[i] : 0);
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
	    if (iosm->goal == IOSM_PKGINSTALL)
		rpmlog(RPMLOG_WARNING,
		    _("user %s does not exist - using root\n"), fi->fuser[i]);
	    uid = 0;
	    finalMode &= ~S_ISUID;      /* turn off suid bit */
	  }
	}

	if (fi->fgroup && gnameToGid(fi->fgroup[i], &gid)) {
	  if (!fi->isSource) {
	    if (iosm->goal == IOSM_PKGINSTALL)
		rpmlog(RPMLOG_WARNING,
		    _("group %s does not exist - using root\n"), fi->fgroup[i]);
	    gid = 0;
	    finalMode &= ~S_ISGID;	/* turn off sgid bit */
	  }
	}

	if (iosm->mapFlags & IOSM_MAP_MODE)
	    st->st_mode = (st->st_mode & S_IFMT) | (finalMode & ~S_IFMT);
	if (iosm->mapFlags & IOSM_MAP_TYPE) {
	    st->st_mode = (st->st_mode & ~S_IFMT) | (finalMode & S_IFMT);
	    if ((S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
	    && st->st_nlink == 0)
		st->st_nlink = 1;
	    st->st_rdev = finalRdev;
	    st->st_ino = finalInode;
	    st->st_mtime = finalMtime;
	}
	if (iosm->mapFlags & IOSM_MAP_UID)
	    st->st_uid = uid;
	if (iosm->mapFlags & IOSM_MAP_GID)
	    st->st_gid = gid;

	/*
	 * Set file digest (if not disabled).
	 */
	if (!iosm->nofdigests) {
	    iosm->fdigestalgo = fi->digestalgo;
	    iosm->fdigest = (fi->fdigests ? fi->fdigests[i] : NULL);
	    iosm->digestlen = fi->digestlen;
	    iosm->digest = (fi->digests ? (fi->digests + (iosm->digestlen * i)) : NULL);
	} else {
	    iosm->fdigestalgo = 0;
	    iosm->fdigest = NULL;
	    iosm->digestlen = 0;
	    iosm->digest = NULL;
	}
    }
    return 0;
}

/** \ingroup payload
 * Create file from payload stream.
 * @param iosm		file state machine data
 * @return		0 on success
 */
/*@-compdef@*/
static int extractRegular(/*@special@*/ IOSM_t iosm)
	/*@uses iosm->fdigest, iosm->digest, iosm->sb, iosm->wfd  @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    const struct stat * st = &iosm->sb;
    size_t left = (size_t) st->st_size;
    int rc = 0;
    int xx;

    rc = iosmNext(iosm, IOSM_WOPEN);
    if (rc)
	goto exit;

    if (st->st_size > 0 && (iosm->fdigest != NULL || iosm->digest != NULL))
	fdInitDigest(iosm->wfd, (pgpHashAlgo)iosm->fdigestalgo, 0);

    while (left) {

	iosm->wrlen = (left > iosm->wrsize ? iosm->wrsize : left);
	rc = iosmNext(iosm, IOSM_DREAD);
	if (rc)
	    goto exit;

	rc = iosmNext(iosm, IOSM_WRITE);
	if (rc)
	    goto exit;

	left -= iosm->wrnb;

	/* Notify iff progress, completion is done elsewhere */
	if (!rc && left)
	    (void) iosmNext(iosm, IOSM_NOTIFY);
    }

    xx = fsync(Fileno(iosm->wfd));

    if (st->st_size > 0 && (iosm->fdigest || iosm->digest)) {
	void * digest = NULL;
	int asAscii = (iosm->digest == NULL ? 1 : 0);

	(void) Fflush(iosm->wfd);
	fdFiniDigest(iosm->wfd, (pgpHashAlgo)iosm->fdigestalgo, &digest, NULL, asAscii);

	if (digest == NULL) {
	    rc = IOSMERR_DIGEST_MISMATCH;
	    goto exit;
	}

	if (iosm->digest != NULL) {
	    if (memcmp(digest, iosm->digest, iosm->digestlen))
		rc = IOSMERR_DIGEST_MISMATCH;
	} else {
	    if (strcmp((char *)digest, iosm->fdigest))
		rc = IOSMERR_DIGEST_MISMATCH;
	}
	digest = _free(digest);
    }

exit:
    (void) iosmNext(iosm, IOSM_WCLOSE);
    return rc;
}
/*@=compdef@*/

/** \ingroup payload
 * Write next item to payload stream.
 * @param iosm		file state machine data
 * @param writeData	should data be written?
 * @return		0 on success
 */
/*@-compdef -compmempass@*/
static int writeFile(/*@special@*/ /*@partial@*/ IOSM_t iosm, int writeData)
	/*@uses iosm->path, iosm->opath, iosm->sb, iosm->osb, iosm->cfd @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    const char * path = iosm->path;
    const char * opath = iosm->opath;
    struct stat * st = &iosm->sb;
    struct stat * ost = &iosm->osb;
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
	/* XXX NUL terminated result in iosm->rdbuf, len in iosm->rdnb. */
	rc = iosmUNSAFE(iosm, IOSM_READLINK);
	if (rc) goto exit;
	st->st_size = iosm->rdnb;
	iosm->lpath = xstrdup(iosm->rdbuf);	/* XXX save readlink return. */
    }

    if (iosm->mapFlags & IOSM_MAP_ABSOLUTE) {
	size_t nb=strlen(iosm->dirName) + strlen(iosm->baseName) + sizeof(".");
	char * t = (char *) alloca(nb);
	*t = '\0';
	iosm->path = t;
	if (iosm->mapFlags & IOSM_MAP_ADDDOT)
	    *t++ = '.';
	t = stpcpy( stpcpy(t, iosm->dirName), iosm->baseName);
    } else if (iosm->mapFlags & IOSM_MAP_PATH) {
	rpmfi fi = (rpmfi) iosmGetFi(iosm);
	if (fi->apath) {
	    const char * apath = NULL;
	    (void) urlPath(fi->apath[iosm->ix], &apath);
	    iosm->path = apath + fi->striplen;
	} else
	    iosm->path = fi->bnl[iosm->ix];
    }

    rc = iosmNext(iosm, IOSM_HWRITE);
    iosm->path = path;
    if (rc) goto exit;

    if (writeData && S_ISREG(st->st_mode)) {
#if defined(HAVE_MMAP)
	char * rdbuf = NULL;
	void * mapped = (void *)-1;
	size_t nmapped = 0;
	/* XXX 128 Mb resource cap for top(1) scrutiny, MADV_DONTNEED better. */
	int use_mmap = (st->st_size <= 0x07ffffff);
#endif

	rc = iosmNext(iosm, IOSM_ROPEN);
	if (rc) goto exit;

	/* XXX unbuffered mmap generates *lots* of fdio debugging */
#if defined(HAVE_MMAP)
	if (use_mmap) {
	    mapped = mmap(NULL, st->st_size, PROT_READ, MAP_SHARED, Fileno(iosm->rfd), 0);
	    if (mapped != (void *)-1) {
		rdbuf = iosm->rdbuf;
		iosm->rdbuf = (char *) mapped;
		iosm->rdlen = nmapped = st->st_size;
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
	    iosm->rdnb = nmapped;
	  } else
#endif
	  {
	    iosm->rdlen = (left > iosm->rdsize ? iosm->rdsize : left),
	    rc = iosmNext(iosm, IOSM_READ);
	    if (rc) goto exit;
	  }

	    /* XXX DWRITE uses rdnb for I/O length. */
	    rc = iosmNext(iosm, IOSM_DWRITE);
	    if (rc) goto exit;

	    left -= iosm->wrnb;
	}

#if defined(HAVE_MMAP)
	if (mapped != (void *)-1) {
/* XXX splint misses size_t 2nd arg. */
/*@i@*/	    xx = msync(mapped, nmapped, MS_ASYNC);
#if defined(HAVE_MADVISE) && defined(MADV_DONTNEED)
	    xx = madvise(mapped, nmapped, MADV_DONTNEED);
#endif
	    xx = munmap(mapped, nmapped);
	    iosm->rdbuf = rdbuf;
	} else
#endif
	{   int fdno = Fileno(iosm->rfd);
	    if (fdno >= 0)
		xx = fsync(fdno);	/* XXX coverity #1035827 */
	}

    }

    rc = iosmNext(iosm, IOSM_PAD);
    if (rc) goto exit;

    rc = 0;

exit:
    if (iosm->rfd != NULL)
	(void) iosmNext(iosm, IOSM_RCLOSE);
/*@-dependenttrans@*/
    iosm->opath = opath;
    iosm->path = path;
/*@=dependenttrans@*/
    return rc;
}
/*@=compdef =compmempass@*/

/** \ingroup payload
 * Write set of linked files to payload stream.
 * @param iosm		file state machine data
 * @return		0 on success
 */
static int writeLinkedFile(/*@special@*/ /*@partial@*/ IOSM_t iosm)
	/*@uses iosm->path, iosm->nsuffix, iosm->ix, iosm->li, iosm->failedFile @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    const char * path = iosm->path;
    const char * lpath = iosm->lpath;
    const char * nsuffix = iosm->nsuffix;
    int iterIndex = iosm->ix;
    int ec = 0;
    int rc;
    int i;
    const char * linkpath = NULL;
    int firstfile = 1;

    iosm->path = NULL;
    iosm->lpath = NULL;
    iosm->nsuffix = NULL;
    iosm->ix = -1;

    for (i = iosm->li->nlink - 1; i >= 0; i--) {

	if (iosm->li->filex[i] < 0) continue;

	iosm->ix = iosm->li->filex[i];
/*@-compdef@*/
	rc = iosmNext(iosm, IOSM_MAP);
/*@=compdef@*/

	/* XXX tar and cpio have to do things differently. */
	if (iosm->headerWrite == tarHeaderWrite) {
	    if (firstfile) {
		const char * apath = NULL;
		char *t;
		(void) urlPath(iosm->path, &apath);
		/* Remove the buildroot prefix. */
		t = (char *) xmalloc(sizeof(".") + strlen(apath + iosm->astriplen));
		(void) stpcpy( stpcpy(t, "."), apath + iosm->astriplen);
		linkpath = t;
		firstfile = 0;
	    } else
		iosm->lpath = linkpath;

	    /* Write data after first link for tar. */
	    rc = writeFile(iosm, (iosm->lpath == NULL));
	} else {
	    /* Write data after last link for cpio. */
	    rc = writeFile(iosm, (i == 0));
	}
	if (iosm->failedFile && rc != 0 && *iosm->failedFile == NULL) {
	    ec = rc;
	    *iosm->failedFile = xstrdup(iosm->path);
	}

	iosm->path = _free(iosm->path);
	iosm->li->filex[i] = -1;
    }

/*@-dependenttrans@*/
    linkpath = _free(linkpath);
/*@=dependenttrans@*/
    iosm->ix = iterIndex;
    iosm->nsuffix = nsuffix;
    iosm->lpath = lpath;
    iosm->path = path;
    return ec;
}

/** \ingroup payload
 * Create pending hard links to existing file.
 * @param iosm		file state machine data
 * @return		0 on success
 */
/*@-compdef@*/
static int iosmMakeLinks(/*@special@*/ /*@partial@*/ IOSM_t iosm)
	/*@uses iosm->path, iosm->opath, iosm->nsuffix, iosm->ix, iosm->li @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    const char * path = iosm->path;
    const char * opath = iosm->opath;
    const char * nsuffix = iosm->nsuffix;
    int iterIndex = iosm->ix;
    int ec = 0;
    int rc;
    int i;

    iosm->path = NULL;
    iosm->opath = NULL;
    iosm->nsuffix = NULL;
    iosm->ix = -1;

    iosm->ix = iosm->li->filex[iosm->li->createdPath];
    rc = iosmNext(iosm, IOSM_MAP);
    iosm->opath = iosm->path;
    iosm->path = NULL;
    for (i = 0; i < iosm->li->nlink; i++) {
	if (iosm->li->filex[i] < 0) continue;
	if (iosm->li->createdPath == i) continue;

	iosm->ix = iosm->li->filex[i];
	iosm->path = _free(iosm->path);
	rc = iosmNext(iosm, IOSM_MAP);
	if (iosmFileActionSkipped(iosm->action)) continue;

	rc = iosmUNSAFE(iosm, IOSM_VERIFY);
	if (!rc) continue;
	if (!(rc == IOSMERR_ENOENT)) break;

	/* XXX link(iosm->opath, iosm->path) */
	rc = iosmNext(iosm, IOSM_LINK);
	if (iosm->failedFile && rc != 0 && *iosm->failedFile == NULL) {
	    ec = rc;
	    *iosm->failedFile = xstrdup(iosm->path);
	}

	iosm->li->linksLeft--;
    }
    iosm->path = _free(iosm->path);
    iosm->opath = _free(iosm->opath);

    iosm->ix = iterIndex;
    iosm->nsuffix = nsuffix;
    iosm->path = path;
    iosm->opath = opath;
    return ec;
}
/*@=compdef@*/

/** \ingroup payload
 * Commit hard linked file set atomically.
 * @param iosm		file state machine data
 * @return		0 on success
 */
/*@-compdef@*/
static int iosmCommitLinks(/*@special@*/ /*@partial@*/ IOSM_t iosm)
	/*@uses iosm->path, iosm->nsuffix, iosm->ix, iosm->sb,
		iosm->li, iosm->links @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    const char * path = iosm->path;
    const char * nsuffix = iosm->nsuffix;
    int iterIndex = iosm->ix;
    struct stat * st = &iosm->sb;
    int rc = 0;
    int i;

    iosm->path = NULL;
    iosm->nsuffix = NULL;
    iosm->ix = -1;

    for (iosm->li = iosm->links; iosm->li; iosm->li = iosm->li->next) {
	if (iosm->li->sb.st_ino == st->st_ino && iosm->li->sb.st_dev == st->st_dev)
	    break;
    }

assert(iosm->li);	/* XXX coverity #1035793 */
    for (i = 0; i < iosm->li->nlink; i++) {
	if (iosm->li->filex[i] < 0) continue;
	iosm->ix = iosm->li->filex[i];
	rc = iosmNext(iosm, IOSM_MAP);
	if (!iosmFileActionSkipped(iosm->action))
	    rc = iosmNext(iosm, IOSM_COMMIT);
	iosm->path = _free(iosm->path);
	iosm->li->filex[i] = -1;
    }

    iosm->ix = iterIndex;
    iosm->nsuffix = nsuffix;
    iosm->path = path;
    return rc;
}
/*@=compdef@*/

/**
 * Remove (if created) directories not explicitly included in package.
 * @param iosm		file state machine data
 * @return		0 on success
 */
static int iosmRmdirs(/*@special@*/ /*@partial@*/ IOSM_t iosm)
	/*@uses iosm->path, iosm->dnlx, iosm->ldn, iosm->rdbuf, iosm->iter @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    const char * path = iosm->path;
    void * dnli = dnlInitIterator(iosm, 1);
    char * dn = iosm->rdbuf;
    int dc = dnlCount((DNLI_t)dnli);
    int rc = 0;

    iosm->path = NULL;
    dn[0] = '\0';
    /*@-observertrans -dependenttrans@*/
    if (iosm->ldn != NULL && iosm->dnlx != NULL)
    while ((iosm->path = dnlNextIterator((DNLI_t)dnli)) != NULL) {
	size_t dnlen = strlen(iosm->path);
	char * te;

	dc = dnlIndex((DNLI_t)dnli);
	if (iosm->dnlx[dc] < 1 || (size_t)iosm->dnlx[dc] >= dnlen)
	    continue;

	/* Copy to avoid const on iosm->path. */
	te = stpcpy(dn, iosm->path) - 1;
	iosm->path = dn;

	/* Remove generated directories. */
	/*@-usereleased@*/ /* LCL: te used after release? */
	do {
	    if (*te == '/') {
		*te = '\0';
/*@-compdef@*/
		rc = iosmNext(iosm, IOSM_RMDIR);
/*@=compdef@*/
		*te = '/';
	    }
	    if (rc)
		/*@innerbreak@*/ break;
	    te--;
	} while ((te - iosm->path) > iosm->dnlx[dc]);
	/*@=usereleased@*/
    }
    dnli = dnlFreeIterator(dnli);
    /*@=observertrans =dependenttrans@*/

    iosm->path = path;
    return rc;
}

/**
 * Create (if necessary) directories not explicitly included in package.
 * @param iosm		file state machine data
 * @return		0 on success
 */
static int iosmMkdirs(/*@special@*/ /*@partial@*/ IOSM_t iosm)
	/*@uses iosm->path, iosm->sb, iosm->osb, iosm->rdbuf, iosm->iter,
		iosm->ldn, iosm->ldnlen, iosm->ldnalloc @*/
	/*@defines iosm->dnlx, iosm->ldn @*/
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    struct stat * st = &iosm->sb;
    struct stat * ost = &iosm->osb;
    const char * path = iosm->path;
    mode_t st_mode = st->st_mode;
    void * dnli = dnlInitIterator(iosm, 0);
    char * dn = iosm->rdbuf;
    int dc = dnlCount((DNLI_t)dnli);
    int rc = 0;
    size_t i;

    iosm->path = NULL;

    dn[0] = '\0';
    iosm->dnlx = (unsigned short *) (dc ? xcalloc(dc, sizeof(*iosm->dnlx)) : NULL);
    /*@-observertrans -dependenttrans@*/
    if (iosm->dnlx != NULL)
    while ((iosm->path = dnlNextIterator((DNLI_t)dnli)) != NULL) {
	size_t dnlen = strlen(iosm->path);
	char * te;

	dc = dnlIndex((DNLI_t)dnli);
	if (dc < 0) continue;
	iosm->dnlx[dc] = (unsigned short) dnlen;
	if (dnlen <= 1)
	    continue;

	/*@-compdef -nullpass@*/	/* FIX: iosm->ldn not defined ??? */
	if (dnlen <= iosm->ldnlen && !strcmp(iosm->path, iosm->ldn))
	    continue;
	/*@=compdef =nullpass@*/

	/* Copy to avoid const on iosm->path. */
	(void) stpcpy(dn, iosm->path);
	iosm->path = dn;

	/* Assume '/' directory exists, "mkdir -p" for others if non-existent */
	(void) urlPath(dn, (const char **)&te);
	for (i = 1, te++; *te != '\0'; te++, i++) {
	    if (*te != '/')
		/*@innercontinue@*/ continue;

	    *te = '\0';

	    /* Already validated? */
	    /*@-usedef -compdef -nullpass -nullderef@*/
	    if (i < iosm->ldnlen &&
		(iosm->ldn[i] == '/' || iosm->ldn[i] == '\0') &&
		!strncmp(iosm->path, iosm->ldn, i))
	    {
		*te = '/';
		/* Move pre-existing path marker forward. */
		iosm->dnlx[dc] = (te - dn);
		/*@innercontinue@*/ continue;
	    }
	    /*@=usedef =compdef =nullpass =nullderef@*/

	    /* Validate next component of path. */
	    rc = iosmUNSAFE(iosm, IOSM_LSTAT);
	    *te = '/';

	    /* Directory already exists? */
	    if (rc == 0 && S_ISDIR(ost->st_mode)) {
		/* Move pre-existing path marker forward. */
		iosm->dnlx[dc] = (te - dn);
	    } else if (rc == IOSMERR_ENOENT) {
		rpmfi fi = (rpmfi) iosmGetFi(iosm);
		*te = '\0';
		st->st_mode = S_IFDIR | (fi->dperms & 07777);
		rc = iosmNext(iosm, IOSM_MKDIR);
		if (!rc) {
#if defined(_USE_RPMSX)
		    /* XXX FIXME? only new dir will have context set. */
		    /* Get file security context from patterns. */
		    if (!fsm->nofcontexts) {
			iosm->fcontext =
				rpmsxMatch(NULL, iosm->path, st->st_mode);
			if (iosm->fcontext != NULL)
			    rc = iosmNext(iosm, IOSM_LSETFCON);
		    } else
#endif
			iosm->fcontext = NULL;
		    rpmlog(RPMLOG_DEBUG,
			D_("%s directory created with perms %04o, context %s.\n"),
			iosm->path, (unsigned)(st->st_mode & 07777),
			(iosm->fcontext ? iosm->fcontext : "(no context)"));
#if defined(_USE_RPMSX)
		    iosm->fcontext = _free(iosm->fcontext);
#endif
		}
		*te = '/';
	    }
	    if (rc)
		/*@innerbreak@*/ break;
	}
	if (rc) break;

	/* Save last validated path. */
/*@-compdef@*/ /* FIX: ldn/path annotations ? */
	if (iosm->ldnalloc < (dnlen + 1)) {
	    iosm->ldnalloc = dnlen + 100;
	    iosm->ldn = (char *) xrealloc(iosm->ldn, iosm->ldnalloc);
	}
	if (iosm->ldn != NULL) {	/* XXX can't happen */
	    strcpy(iosm->ldn, iosm->path);
 	    iosm->ldnlen = dnlen;
	}
/*@=compdef@*/
    }
    dnli = dnlFreeIterator(dnli);
    /*@=observertrans =dependenttrans@*/

    iosm->path = path;
    st->st_mode = st_mode;		/* XXX restore st->st_mode */
/*@-compdef@*/ /* FIX: ldn/path annotations ? */
    return rc;
/*@=compdef@*/
}

#ifdef	NOTYET
/**
 * Check for file on disk.
 * @param iosm		file state machine data
 * @return		0 on success
 */
static int iosmStat(/*@special@*/ /*@partial@*/ IOSM_t iosm)
	/*@globals fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/
{
    int rc = 0;

    if (iosm->path != NULL) {
	int saveernno = errno;
	rc = iosmUNSAFE(iosm, (!(iosm->mapFlags & IOSM_FOLLOW_SYMLINKS)
			? IOSM_LSTAT : IOSM_STAT));
	if (rc == IOSMERR_ENOENT) {
	    errno = saveerrno;
	    rc = 0;
	    iosm->exists = 0;
	} else if (rc == 0) {
	    iosm->exists = 1;
	}
    } else {
	/* Skip %ghost files on build. */
	iosm->exists = 0;
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
int iosmStage(IOSM_t iosm, iosmFileStage stage)
{
#ifdef	NOTUSED
    iosmFileStage prevStage = iosm->stage;
    const char * const prev = iosmFileStageString(prevStage);
#endif
    const char * const cur = iosmFileStageString(stage);
    struct stat * st = &iosm->sb;
    struct stat * ost = &iosm->osb;
    int saveerrno = errno;
    int rc = iosm->rc;
    size_t left;
    int i;

#define	_fafilter(_a)	\
    (!((_a) == FA_CREATE || (_a) == FA_ERASE || (_a) == FA_COPYIN || (_a) == FA_COPYOUT) \
	? iosmFileActionString(_a) : "")

    if (stage & IOSM_DEAD) {
	/* do nothing */
    } else if (stage & IOSM_INTERNAL) {
	if (iosm->debug && !(stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s %06o%3d (%4d,%4d)%12lu %s %s\n",
		cur,
		(unsigned)st->st_mode, (int)st->st_nlink,
		(int)st->st_uid, (int)st->st_gid, (unsigned long)st->st_size,
		(iosm->path ? iosm->path : ""),
		_fafilter(iosm->action));
    } else {
	const char * apath = NULL;
	if (iosm->path)
	    (void) urlPath(iosm->path, &apath);
	iosm->stage = stage;
	if (iosm->debug || !(stage & IOSM_VERBOSE))
	    rpmlog(RPMLOG_DEBUG, "%-8s  %06o%3d (%4d,%4d)%12lu %s %s\n",
		cur,
		(unsigned)st->st_mode, (int)st->st_nlink,
		(int)st->st_uid, (int)st->st_gid, (unsigned long)st->st_size,
		(apath ? apath + iosm->astriplen : ""),
		_fafilter(iosm->action));
    }
#undef	_fafilter

    switch (stage) {
    case IOSM_UNKNOWN:
	break;
    case IOSM_PKGINSTALL:
	while (1) {
	    /* Clean iosm, free'ing memory. Read next archive header. */
	    rc = iosmUNSAFE(iosm, IOSM_INIT);

	    /* Exit on end-of-payload. */
	    if (rc == IOSMERR_HDR_TRAILER) {
		rc = 0;
		/*@loopbreak@*/ break;
	    }

	    /* Exit on error. */
	    if (rc) {
		iosm->postpone = 1;
		(void) iosmNext(iosm, IOSM_UNDO);
		/*@loopbreak@*/ break;
	    }

	    /* Extract file from archive. */
	    rc = iosmNext(iosm, IOSM_PROCESS);
	    if (rc) {
		(void) iosmNext(iosm, IOSM_UNDO);
		/*@loopbreak@*/ break;
	    }

	    /* Notify on success. */
	    (void) iosmNext(iosm, IOSM_NOTIFY);

	    rc = iosmNext(iosm, IOSM_FINI);
	    if (rc) {
		/*@loopbreak@*/ break;
	    }
	}
	break;
    case IOSM_PKGERASE:
    case IOSM_PKGCOMMIT:
	while (1) {
	    /* Clean iosm, free'ing memory. */
	    rc = iosmUNSAFE(iosm, IOSM_INIT);

	    /* Exit on end-of-payload. */
	    if (rc == IOSMERR_HDR_TRAILER) {
		rc = 0;
		/*@loopbreak@*/ break;
	    }

	    /* Rename/erase next item. */
	    if (iosmNext(iosm, IOSM_FINI))
		/*@loopbreak@*/ break;
	}
	break;
    case IOSM_PKGBUILD:
	while (1) {

	    rc = iosmUNSAFE(iosm, IOSM_INIT);

	    /* Exit on end-of-payload. */
	    if (rc == IOSMERR_HDR_TRAILER) {
		rc = 0;
		/*@loopbreak@*/ break;
	    }

	    /* Exit on error. */
	    if (rc) {
		iosm->postpone = 1;
		(void) iosmNext(iosm, IOSM_UNDO);
		/*@loopbreak@*/ break;
	    }

	    /* Copy file into archive. */
	    rc = iosmNext(iosm, IOSM_PROCESS);
	    if (rc) {
		(void) iosmNext(iosm, IOSM_UNDO);
		/*@loopbreak@*/ break;
	    }

	    /* Notify on success. */
	    (void) iosmNext(iosm, IOSM_NOTIFY);

	    if (iosmNext(iosm, IOSM_FINI))
		/*@loopbreak@*/ break;
	}

	/* Flush partial sets of hard linked files. */
	if (!(iosm->mapFlags & IOSM_ALL_HARDLINKS)) {
	    int nlink, j;
	    while ((iosm->li = iosm->links) != NULL) {
		iosm->links = iosm->li->next;
		iosm->li->next = NULL;

		/* Re-calculate link count for archive header. */
		for (j = -1, nlink = 0, i = 0; i < iosm->li->nlink; i++) {
		    if (iosm->li->filex[i] < 0)
			/*@innercontinue@*/ continue;
		    nlink++;
		    if (j == -1) j = i;
		}
		/* XXX force the contents out as well. */
		if (j != 0) {
		    iosm->li->filex[0] = iosm->li->filex[j];
		    iosm->li->filex[j] = -1;
		}
		iosm->li->sb.st_nlink = nlink;

		iosm->sb = iosm->li->sb;	/* structure assignment */
		iosm->osb = iosm->sb;	/* structure assignment */

		if (!rc) rc = writeLinkedFile(iosm);

	        iosm->li = (struct hardLink_s *) freeHardLink((struct hardLink_s *) iosm->li);
	    }
	}

	if (!rc)
	    rc = iosmNext(iosm, IOSM_TRAILER);

	break;
    case IOSM_CREATE:
	iosm->path = _free(iosm->path);
	iosm->lpath = _free(iosm->lpath);
	iosm->opath = _free(iosm->opath);
	iosm->dnlx = _free(iosm->dnlx);

	iosm->ldn = _free(iosm->ldn);
	iosm->ldnalloc = iosm->ldnlen = 0;

	iosm->rdsize = iosm->wrsize = 0;
	iosm->rdbuf = iosm->rdb = _free(iosm->rdb);
	iosm->wrbuf = iosm->wrb = _free(iosm->wrb);
	if (iosm->goal == IOSM_PKGINSTALL || iosm->goal == IOSM_PKGBUILD) {
	    iosm->rdsize = 16 * BUFSIZ;
	    iosm->rdbuf = iosm->rdb = (char *) xmalloc(iosm->rdsize);
	    iosm->wrsize = 16 * BUFSIZ;
	    iosm->wrbuf = iosm->wrb = (char *) xmalloc(iosm->wrsize);
	}

	iosm->mkdirsdone = 0;
	iosm->ix = -1;
	iosm->links = NULL;
	iosm->li = NULL;
	errno = 0;	/* XXX get rid of EBADF */

	/* Detect and create directories not explicitly in package. */
	if (iosm->goal == IOSM_PKGINSTALL) {
/*@-compdef@*/
	    rc = iosmNext(iosm, IOSM_MKDIRS);
/*@=compdef@*/
	    if (!rc) iosm->mkdirsdone = 1;
	}

	break;
    case IOSM_INIT:
	iosm->path = _free(iosm->path);
	iosm->lpath = _free(iosm->lpath);
	iosm->postpone = 0;
	iosm->diskchecked = iosm->exists = 0;
	iosm->subdir = NULL;
	iosm->suffix = (iosm->sufbuf[0] != '\0' ? iosm->sufbuf : NULL);
	iosm->action = FA_UNKNOWN;
	iosm->osuffix = NULL;
	iosm->nsuffix = NULL;

	if (iosm->goal == IOSM_PKGINSTALL) {
	    /* Read next header from payload, checking for end-of-payload. */
	    rc = iosmUNSAFE(iosm, IOSM_NEXT);
	}
	if (rc) break;

	/* Identify mapping index. */
	iosm->ix = ((iosm->goal == IOSM_PKGINSTALL)
		? mapFind(iosm->iter, iosm->path) : mapNextIterator(iosm->iter));

    {	rpmfi fi = (rpmfi) iosmGetFi(iosm);
	if (fi != NULL && !(fi->mapflags & IOSM_PAYLOAD_LIST)) {
	/* Detect end-of-loop and/or mapping error. */
	    if (!(fi->mapflags & IOSM_PAYLOAD_EXTRACT)) {
		if (iosm->ix < 0) {
		    if (iosm->goal == IOSM_PKGINSTALL) {
#if 0
			rpmlog(RPMLOG_WARNING,
			    _("archive file %s was not found in header\n"),
			    iosm->path);
#endif
			if (iosm->failedFile && *iosm->failedFile == NULL)
			    *iosm->failedFile = xstrdup(iosm->path);
			rc = IOSMERR_UNMAPPED_FILE;
		    } else
			rc = IOSMERR_HDR_TRAILER;
		    break;
		}
	    }

	    /* On non-install, mode must be known so dirs don't get suffixed. */
	    if (iosm->goal != IOSM_PKGINSTALL)
		st->st_mode = fi->fmodes[iosm->ix];
	}
    }

	/* Generate file path. */
	rc = iosmNext(iosm, IOSM_MAP);
	if (rc) break;

	/* Perform lstat/stat for disk file. */
#ifdef	NOTYET
	rc = iosmStat(iosm);
#else
	if (iosm->path != NULL &&
	    !(iosm->goal == IOSM_PKGINSTALL && S_ISREG(st->st_mode)))
	{
	    rc = iosmUNSAFE(iosm, (!(iosm->mapFlags & IOSM_FOLLOW_SYMLINKS)
			? IOSM_LSTAT : IOSM_STAT));
	    if (rc == IOSMERR_ENOENT) {
		errno = saveerrno;
		rc = 0;
		iosm->exists = 0;
	    } else if (rc == 0) {
		iosm->exists = 1;
	    }
	} else {
	    /* Skip %ghost files on build. */
	    iosm->exists = 0;
	}
#endif
	iosm->diskchecked = 1;
	if (rc) break;

	/* On non-install, the disk file stat is what's remapped. */
	if (iosm->goal != IOSM_PKGINSTALL)
	    *st = *ost;			/* structure assignment */

	/* Remap file perms, owner, and group. */
	rc = iosmMapAttrs(iosm);
	if (rc) break;

	iosm->postpone = iosmFileActionSkipped(iosm->action);
	if (iosm->goal == IOSM_PKGINSTALL || iosm->goal == IOSM_PKGBUILD) {
	    /*@-evalorder@*/ /* FIX: saveHardLink can modify iosm */
	    if (S_ISREG(st->st_mode) && st->st_nlink > 1)
		iosm->postpone = saveHardLink(iosm);
	    /*@=evalorder@*/
	}
    {	rpmfi fi = (rpmfi) iosmGetFi(iosm);
	if (fi != NULL && (fi->mapflags & IOSM_PAYLOAD_LIST))
	    iosm->postpone = 1;
    }
	break;
    case IOSM_PRE:
	break;
    case IOSM_MAP:
	rc = iosmMapPath(iosm);
	break;
    case IOSM_MKDIRS:
	rc = iosmMkdirs(iosm);
	break;
    case IOSM_RMDIRS:
	if (iosm->dnlx)
	    rc = iosmRmdirs(iosm);
	break;
    case IOSM_PROCESS:
	if (iosm->postpone) {
	    if (iosm->goal == IOSM_PKGINSTALL) {
		/* XXX Skip over file body, archive headers already done. */
		if (S_ISREG(st->st_mode))
		    rc = iosmNext(iosm, IOSM_EAT);
	    }
	    break;
	}

	if (iosm->goal == IOSM_PKGBUILD) {
	    if (iosm->fflags & RPMFILE_GHOST) /* XXX Don't if %ghost file. */
		break;
	    if (S_ISREG(st->st_mode) && st->st_nlink > 1) {
		struct hardLink_s * li, * prev;

if (!(iosm->mapFlags & IOSM_ALL_HARDLINKS)) break;
		rc = writeLinkedFile(iosm);
		if (rc) break;	/* W2DO? */

		for (li = iosm->links, prev = NULL; li; prev = li, li = li->next)
		     if (li == iosm->li)
			/*@loopbreak@*/ break;

		if (prev == NULL)
		    iosm->links = iosm->li->next;
		else
		    prev->next = iosm->li->next;
		iosm->li->next = NULL;
	        iosm->li = (struct hardLink_s *) freeHardLink((struct hardLink_s *) iosm->li);
	    } else {
		rc = writeFile(iosm, 1);
	    }
	    break;
	}

	if (iosm->goal != IOSM_PKGINSTALL)
	    break;

	if (S_ISREG(st->st_mode) && iosm->lpath != NULL) {
	    const char * opath = iosm->opath;
	    char * t = (char *) xmalloc(strlen(iosm->lpath+1) + strlen(iosm->suffix) + 1);
	    (void) stpcpy(t, iosm->lpath+1);
	     iosm->opath = t;
	    /* XXX link(iosm->opath, iosm->path) */
	    rc = iosmNext(iosm, IOSM_LINK);
	    if (iosm->failedFile && rc != 0 && *iosm->failedFile == NULL) {
		*iosm->failedFile = xstrdup(iosm->path);
	    }
	    iosm->opath = _free(iosm->opath);
	    iosm->opath = opath;
	    break;	/* XXX so that delayed hard links get skipped. */
	}
	if (S_ISREG(st->st_mode)) {
	    const char * path = iosm->path;
	    if (iosm->osuffix)
		iosm->path = iosmFsPath(iosm, st, NULL, NULL);
	    rc = iosmUNSAFE(iosm, IOSM_VERIFY);

	    if (rc == 0 && iosm->osuffix) {
		const char * opath = iosm->opath;
		iosm->opath = iosm->path;
		iosm->path = iosmFsPath(iosm, st, NULL, iosm->osuffix);
		rc = iosmNext(iosm, IOSM_RENAME);
		if (!rc)
		    rpmlog(RPMLOG_WARNING,
			_("%s saved as %s\n"),
				(iosm->opath ? iosm->opath : ""),
				(iosm->path ? iosm->path : ""));
		iosm->path = _free(iosm->path);
		iosm->opath = opath;
	    }

	    /*@-dependenttrans@*/
	    iosm->path = path;
	    /*@=dependenttrans@*/
	    if (!(rc == IOSMERR_ENOENT)) return rc;
	    rc = extractRegular(iosm);
	} else if (S_ISDIR(st->st_mode)) {
	    mode_t st_mode = st->st_mode;
	    rc = iosmUNSAFE(iosm, IOSM_VERIFY);
	    if (rc == IOSMERR_ENOENT) {
		st->st_mode &= ~07777; 		/* XXX abuse st->st_mode */
		st->st_mode |=  00700;
		rc = iosmNext(iosm, IOSM_MKDIR);
		st->st_mode = st_mode;		/* XXX restore st->st_mode */
	    }
	} else if (S_ISLNK(st->st_mode)) {
assert(iosm->lpath != NULL);
	    rc = iosmUNSAFE(iosm, IOSM_VERIFY);
	    if (rc == IOSMERR_ENOENT)
		rc = iosmNext(iosm, IOSM_SYMLINK);
	} else if (S_ISFIFO(st->st_mode)) {
	    mode_t st_mode = st->st_mode;
	    /* This mimics cpio S_ISSOCK() behavior but probably isnt' right */
	    rc = iosmUNSAFE(iosm, IOSM_VERIFY);
	    if (rc == IOSMERR_ENOENT) {
		st->st_mode = 0000;		/* XXX abuse st->st_mode */
		rc = iosmNext(iosm, IOSM_MKFIFO);
		st->st_mode = st_mode;		/* XXX restore st->st_mode */
	    }
	} else if (S_ISCHR(st->st_mode) ||
		   S_ISBLK(st->st_mode) ||
    /*@-unrecog@*/ S_ISSOCK(st->st_mode) /*@=unrecog@*/)
	{
	    rc = iosmUNSAFE(iosm, IOSM_VERIFY);
	    if (rc == IOSMERR_ENOENT)
		rc = iosmNext(iosm, IOSM_MKNOD);
	} else {
	    /* XXX Repackaged payloads may be missing files. */
	    if (iosm->repackaged)
		break;

	    /* XXX Special case /dev/log, which shouldn't be packaged anyways */
	    if (!IS_DEV_LOG(iosm->path))
		rc = IOSMERR_UNKNOWN_FILETYPE;
	}
	if (S_ISREG(st->st_mode) && st->st_nlink > 1) {
	    iosm->li->createdPath = iosm->li->linkIndex;
	    rc = iosmMakeLinks(iosm);
	}
	break;
    case IOSM_POST:
	break;
    case IOSM_MKLINKS:
	rc = iosmMakeLinks(iosm);
	break;
    case IOSM_NOTIFY:		/* XXX move from iosm to psm -> tsm */
#if defined(_USE_RPMTS)
	if (iosm->goal == IOSM_PKGINSTALL || iosm->goal == IOSM_PKGBUILD) {
	    rpmfi fi = iosmGetFi(iosm);
	    rpmuint64_t archivePos = fdGetCpioPos(iosm->cfd);
	    if (archivePos > fi->archivePos) {
		fi->archivePos = (unsigned long long) archivePos;
		(void) rpmtsNotify(iosmGetTs(iosm), fi->te,RPMCALLBACK_INST_PROGRESS,
			fi->archivePos, fi->archiveSize);
	    }
	}
#endif
	break;
    case IOSM_UNDO:
	if (iosm->postpone)
	    break;
	if (iosm->goal == IOSM_PKGINSTALL) {
	    /* XXX only erase if temp fn w suffix is in use */
	    if (iosm->sufbuf[0] != '\0')
		(void) iosmNext(iosm,
		    (S_ISDIR(st->st_mode) ? IOSM_RMDIR : IOSM_UNLINK));

#ifdef	NOTYET	/* XXX remove only dirs just created, not all. */
	    if (iosm->dnlx)
		(void) iosmNext(iosm, IOSM_RMDIRS);
#endif
	    errno = saveerrno;
	}
	if (iosm->failedFile && *iosm->failedFile == NULL)
	    *iosm->failedFile = xstrdup(iosm->path);
	break;
    case IOSM_FINI:
	if (!iosm->postpone && iosm->commit) {
	    if (iosm->goal == IOSM_PKGINSTALL)
		rc = ((S_ISREG(st->st_mode) && st->st_nlink > 1)
			? iosmCommitLinks(iosm) : iosmNext(iosm, IOSM_COMMIT));
	    if (iosm->goal == IOSM_PKGCOMMIT)
		rc = iosmNext(iosm, IOSM_COMMIT);
	    if (iosm->goal == IOSM_PKGERASE)
		rc = iosmNext(iosm, IOSM_COMMIT);
	}
	iosm->path = _free(iosm->path);
	iosm->lpath = _free(iosm->lpath);
	iosm->opath = _free(iosm->opath);
	memset(st, 0, sizeof(*st));
	memset(ost, 0, sizeof(*ost));
	break;
    case IOSM_COMMIT:
	/* Rename pre-existing modified or unmanaged file. */
	if (iosm->osuffix && iosm->diskchecked &&
	  (iosm->exists || (iosm->goal == IOSM_PKGINSTALL && S_ISREG(st->st_mode))))
	{
	    const char * opath = iosm->opath;
	    const char * path = iosm->path;
	    iosm->opath = iosmFsPath(iosm, st, NULL, NULL);
	    iosm->path = iosmFsPath(iosm, st, NULL, iosm->osuffix);
	    rc = iosmNext(iosm, IOSM_RENAME);
	    if (!rc) {
		rpmlog(RPMLOG_WARNING, _("%s saved as %s\n"),
				(iosm->opath ? iosm->opath : ""),
				(iosm->path ? iosm->path : ""));
	    }
	    iosm->path = _free(iosm->path);
	    iosm->path = path;
	    iosm->opath = _free(iosm->opath);
	    iosm->opath = opath;
	}

	/* Remove erased files. */
	if (iosm->goal == IOSM_PKGERASE) {
	    if (iosm->action == FA_ERASE) {
		if (S_ISDIR(st->st_mode)) {
		    rc = iosmNext(iosm, IOSM_RMDIR);
		    if (!rc) break;
		    switch (rc) {
		    case IOSMERR_ENOENT: /* XXX rmdir("/") linux 2.2.x kernel hack */
		    case IOSMERR_ENOTEMPTY:
	/* XXX make sure that build side permits %missingok on directories. */
			if (iosm->fflags & RPMFILE_MISSINGOK)
			    /*@innerbreak@*/ break;

			/* XXX common error message. */
			rpmlog(
			    (iosm->strict_erasures ? RPMLOG_ERR : RPMLOG_DEBUG),
			    _("rmdir of %s failed: Directory not empty\n"), 
				iosm->path);
			/*@innerbreak@*/ break;
		    default:
			rpmlog(
			    (iosm->strict_erasures ? RPMLOG_ERR : RPMLOG_DEBUG),
				_("rmdir of %s failed: %s\n"),
				iosm->path, strerror(errno));
			/*@innerbreak@*/ break;
		    }
		} else {
		    rc = iosmNext(iosm, IOSM_UNLINK);
		    if (!rc) break;
		    switch (rc) {
		    case IOSMERR_ENOENT:
			if (iosm->fflags & RPMFILE_MISSINGOK)
			    /*@innerbreak@*/ break;
			/*@fallthrough@*/
		    default:
			rpmlog(
			    (iosm->strict_erasures ? RPMLOG_ERR : RPMLOG_DEBUG),
				_("unlink of %s failed: %s\n"),
				iosm->path, strerror(errno));
			/*@innerbreak@*/ break;
		    }
		}
	    }
	    /* XXX Failure to remove is not (yet) cause for failure. */
	    if (!iosm->strict_erasures) rc = 0;
	    break;
	}

	/* XXX Special case /dev/log, which shouldn't be packaged anyways */
    { rpmfi fi = (rpmfi) iosmGetFi(iosm);
      if (!(fi->mapflags & IOSM_PAYLOAD_EXTRACT)) {
	if (!S_ISSOCK(st->st_mode) && !IS_DEV_LOG(iosm->path)) {
	    /* Rename temporary to final file name. */
	    if (!S_ISDIR(st->st_mode) &&
		(iosm->subdir || iosm->suffix || iosm->nsuffix))
	    {
		iosm->opath = iosm->path;
		iosm->path = iosmFsPath(iosm, st, NULL, iosm->nsuffix);
		rc = iosmNext(iosm, IOSM_RENAME);
		if (rc)
			(void) Unlink(iosm->opath);
		else if (iosm->nsuffix) {
		    const char * opath = iosmFsPath(iosm, st, NULL, NULL);
		    rpmlog(RPMLOG_WARNING, _("%s created as %s\n"),
				(opath ? opath : ""),
				(iosm->path ? iosm->path : ""));
		    opath = _free(opath);
		}
		iosm->opath = _free(iosm->opath);
	    }
	    /*
	     * Set file security context (if not disabled).
	     */
	    if (!rc && !getuid()) {
		rc = iosmMapFContext(iosm);
		if (!rc)
		    rc = iosmNext(iosm, IOSM_LSETFCON);
		iosm->fcontext = NULL;
	    }
	    if (S_ISLNK(st->st_mode)) {
		if (!rc && !getuid())
		    rc = iosmNext(iosm, IOSM_LCHOWN);
	    } else {
		if (!rc && !getuid())
		    rc = iosmNext(iosm, IOSM_CHOWN);
		if (!rc)
		    rc = iosmNext(iosm, IOSM_CHMOD);
		if (!rc) {
		    time_t mtime = st->st_mtime;
		    if (fi->fmtimes)
			st->st_mtime = fi->fmtimes[iosm->ix];
		    rc = iosmNext(iosm, IOSM_UTIME);
		    st->st_mtime = mtime;
		}
	    }
	}
      }
    }

	/* Notify on success. */
	if (!rc)		rc = iosmNext(iosm, IOSM_NOTIFY);
	else if (iosm->failedFile && *iosm->failedFile == NULL) {
	    *iosm->failedFile = iosm->path;
	    iosm->path = NULL;
	}
	break;
    case IOSM_DESTROY:
	iosm->path = _free(iosm->path);

	/* Check for hard links missing from payload. */
	while ((iosm->li = iosm->links) != NULL) {
	    iosm->links = iosm->li->next;
	    iosm->li->next = NULL;
	    if (iosm->goal == IOSM_PKGINSTALL &&
			iosm->commit && iosm->li->linksLeft)
	    {
		for (i = 0 ; i < iosm->li->linksLeft; i++) {
		    if (iosm->li->filex[i] < 0)
			/*@innercontinue@*/ continue;
		    rc = IOSMERR_MISSING_HARDLINK;
		    if (iosm->failedFile && *iosm->failedFile == NULL) {
			iosm->ix = iosm->li->filex[i];
			if (!iosmNext(iosm, IOSM_MAP)) {
			    *iosm->failedFile = iosm->path;
			    iosm->path = NULL;
			}
		    }
		    /*@loopbreak@*/ break;
		}
	    }
	    if (iosm->goal == IOSM_PKGBUILD &&
		(iosm->mapFlags & IOSM_ALL_HARDLINKS))
	    {
		rc = IOSMERR_MISSING_HARDLINK;
            }
	    freeHardLink(iosm->li); iosm->li = NULL;
	}
	iosm->ldn = _free(iosm->ldn);
	iosm->ldnalloc = iosm->ldnlen = 0;
	iosm->rdbuf = iosm->rdb = _free(iosm->rdb);
	iosm->wrbuf = iosm->wrb = _free(iosm->wrb);
	break;
    case IOSM_VERIFY:
	if (iosm->diskchecked && !iosm->exists) {
	    rc = IOSMERR_ENOENT;
	    break;
	}
	if (S_ISREG(st->st_mode)) {
	    char * path = (char *) alloca(strlen(iosm->path) + sizeof("-RPMDELETE"));
	    (void) stpcpy( stpcpy(path, iosm->path), "-RPMDELETE");
	    /*
	     * XXX HP-UX (and other os'es) don't permit unlink on busy
	     * XXX files.
	     */
	    iosm->opath = iosm->path;
	    iosm->path = path;
	    rc = iosmNext(iosm, IOSM_RENAME);
	    if (!rc)
		    (void) iosmNext(iosm, IOSM_UNLINK);
	    else
		    rc = IOSMERR_UNLINK_FAILED;
	    iosm->path = iosm->opath;
	    iosm->opath = NULL;
	    return (rc ? rc : IOSMERR_ENOENT);	/* XXX HACK */
	    /*@notreached@*/ break;
	} else if (S_ISDIR(st->st_mode)) {
	    if (S_ISDIR(ost->st_mode))		return 0;
	    if (S_ISLNK(ost->st_mode)) {
		rc = iosmUNSAFE(iosm, IOSM_STAT);
		if (rc == IOSMERR_ENOENT) rc = 0;
		if (rc) break;
		errno = saveerrno;
		if (S_ISDIR(ost->st_mode))	return 0;
	    }
	} else if (S_ISLNK(st->st_mode)) {
	    if (S_ISLNK(ost->st_mode)) {
	/* XXX NUL terminated result in iosm->rdbuf, len in iosm->rdnb. */
		rc = iosmUNSAFE(iosm, IOSM_READLINK);
		errno = saveerrno;
		if (rc) break;
		if (!strcmp(iosm->lpath, iosm->rdbuf))	return 0;
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
	if (iosm->stage == IOSM_PROCESS) rc = iosmNext(iosm, IOSM_UNLINK);
	if (rc == 0)	rc = IOSMERR_ENOENT;
	return (rc ? rc : IOSMERR_ENOENT);	/* XXX HACK */
	/*@notreached@*/ break;

    case IOSM_UNLINK:
	/* XXX Remove setuid/setgid bits on possibly hard linked files. */
	if (iosm->mapFlags & IOSM_SBIT_CHECK) {
	    struct stat stb;
	    if (Lstat(iosm->path, &stb) == 0
	     && S_ISREG(stb.st_mode) && (stb.st_mode & 06000) != 0)
	    {
		/* XXX rc = iosmNext(iosm, IOSM_CHMOD); instead */
		(void)Chmod(iosm->path, stb.st_mode & 0777);
	    }
	}
	rc = Unlink(iosm->path);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s) %s\n", cur,
		iosm->path, (rc < 0 ? strerror(errno) : ""));
	if (rc < 0)
	    rc = (errno == ENOENT ? IOSMERR_ENOENT : IOSMERR_UNLINK_FAILED);
	break;
    case IOSM_RENAME:
	/* XXX Remove setuid/setgid bits on possibly hard linked files. */
	if (iosm->mapFlags & IOSM_SBIT_CHECK) {
	    struct stat stb;
	    if (Lstat(iosm->path, &stb) == 0
	     && S_ISREG(stb.st_mode) && (stb.st_mode & 06000) != 0)
	    {
		/* XXX rc = iosmNext(iosm, IOSM_CHMOD); instead */
		(void)Chmod(iosm->path, stb.st_mode & 0777);
	    }
	}
	rc = Rename(iosm->opath, iosm->path);
	/* XXX Repackaged payloads may be missing files. */
	if (iosm->repackaged)
	    rc = 0;
#if defined(ETXTBSY)
	if (rc && errno == ETXTBSY) {
	    char * path = (char *) alloca(strlen(iosm->path) + sizeof("-RPMDELETE"));
	    (void) stpcpy( stpcpy(path, iosm->path), "-RPMDELETE");
	    /*
	     * XXX HP-UX (and other os'es) don't permit rename to busy
	     * XXX files.
	     */
	    rc = Rename(iosm->path, path);
	    if (!rc) rc = Rename(iosm->opath, iosm->path);
	}
#endif
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, %s) %s\n", cur,
		iosm->opath, iosm->path, (rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_RENAME_FAILED;
	break;
    case IOSM_MKDIR:
	rc = Mkdir(iosm->path, (st->st_mode & 07777));
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, 0%04o) %s\n", cur,
		iosm->path, (unsigned)(st->st_mode & 07777),
		(rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_MKDIR_FAILED;
	break;
    case IOSM_RMDIR:
	rc = Rmdir(iosm->path);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s) %s\n", cur,
		iosm->path, (rc < 0 ? strerror(errno) : ""));
	if (rc < 0)
	    switch (errno) {
	    case ENOENT:        rc = IOSMERR_ENOENT;    /*@switchbreak@*/ break;
	    case ENOTEMPTY:     rc = IOSMERR_ENOTEMPTY; /*@switchbreak@*/ break;
	    default:            rc = IOSMERR_RMDIR_FAILED; /*@switchbreak@*/ break;
	    }
	break;
    case IOSM_LSETFCON:
      {	const char * iosmpath = NULL;
	if (iosm->fcontext == NULL || *iosm->fcontext == '\0'
	 || !strcmp(iosm->fcontext, "<<none>>"))
	    break;
	(void) urlPath(iosm->path, &iosmpath);	/* XXX iosm->path */
	rc = rpmsxLsetfilecon(NULL, iosmpath, 0, iosm->fcontext);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, %s) %s\n", cur,
		iosm->path, iosm->fcontext,
		(rc < 0 ? strerror(errno) : ""));
	if (rc < 0) rc = (errno == EOPNOTSUPP ? 0 : IOSMERR_LSETFCON_FAILED);
      }	break;
    case IOSM_CHOWN:
	rc = Chown(iosm->path, st->st_uid, st->st_gid);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, %d, %d) %s\n", cur,
		iosm->path, (int)st->st_uid, (int)st->st_gid,
		(rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_CHOWN_FAILED;
	break;
    case IOSM_LCHOWN:
#if ! CHOWN_FOLLOWS_SYMLINK
	rc = Lchown(iosm->path, st->st_uid, st->st_gid);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, %d, %d) %s\n", cur,
		iosm->path, (int)st->st_uid, (int)st->st_gid,
		(rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_CHOWN_FAILED;
#endif
	break;
    case IOSM_CHMOD:
	rc = Chmod(iosm->path, (st->st_mode & 07777));
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, 0%04o) %s\n", cur,
		iosm->path, (unsigned)(st->st_mode & 07777),
		(rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_CHMOD_FAILED;
	break;
    case IOSM_UTIME:
	{   struct utimbuf stamp;
	    stamp.actime = st->st_mtime;
	    stamp.modtime = st->st_mtime;
	    rc = Utime(iosm->path, &stamp);
	    if (iosm->debug && (stage & IOSM_SYSCALL))
		rpmlog(RPMLOG_DEBUG, " %8s (%s, 0x%x) %s\n", cur,
			iosm->path, (unsigned)st->st_mtime,
			(rc < 0 ? strerror(errno) : ""));
	    if (rc < 0)	rc = IOSMERR_UTIME_FAILED;
	}
	break;
    case IOSM_SYMLINK:
	rc = Symlink(iosm->lpath, iosm->path);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, %s) %s\n", cur,
		iosm->lpath, iosm->path, (rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_SYMLINK_FAILED;
	break;
    case IOSM_LINK:
	rc = Link(iosm->opath, iosm->path);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, %s) %s\n", cur,
		iosm->opath, iosm->path, (rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_LINK_FAILED;
	break;
    case IOSM_MKFIFO:
	rc = Mkfifo(iosm->path, (st->st_mode & 07777));
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, 0%04o) %s\n", cur,
		iosm->path, (unsigned)(st->st_mode & 07777),
		(rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_MKFIFO_FAILED;
	break;
    case IOSM_MKNOD:
	/*@-unrecog -portability @*/ /* FIX: check S_IFIFO or dev != 0 */
	rc = Mknod(iosm->path, (st->st_mode & ~07777), st->st_rdev);
	/*@=unrecog =portability @*/
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, 0%o, 0x%x) %s\n", cur,
		iosm->path, (unsigned)(st->st_mode & ~07777),
		(unsigned)st->st_rdev,
		(rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_MKNOD_FAILED;
	break;
    case IOSM_LSTAT:
	rc = Lstat(iosm->path, ost);
	if (iosm->debug && (stage & IOSM_SYSCALL) && rc && errno != ENOENT)
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, ost) %s\n", cur,
		iosm->path, (rc < 0 ? strerror(errno) : ""));
	if (rc < 0) {
	    rc = (errno == ENOENT ? IOSMERR_ENOENT : IOSMERR_LSTAT_FAILED);
	    memset(ost, 0, sizeof(*ost));	/* XXX s390x hackery */
	}
	break;
    case IOSM_STAT:
	rc = Stat(iosm->path, ost);
	if (iosm->debug && (stage & IOSM_SYSCALL) && rc && errno != ENOENT)
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, ost) %s\n", cur,
		iosm->path, (rc < 0 ? strerror(errno) : ""));
	if (rc < 0) {
	    rc = (errno == ENOENT ? IOSMERR_ENOENT : IOSMERR_STAT_FAILED);
	    memset(ost, 0, sizeof(*ost));	/* XXX s390x hackery */
	}
	break;
    case IOSM_READLINK:
	/* XXX NUL terminated result in iosm->rdbuf, len in iosm->rdnb. */
	rc = Readlink(iosm->path, iosm->rdbuf, iosm->rdsize - 1);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, rdbuf, %d) %s\n", cur,
		iosm->path, (int)(iosm->rdsize -1), (rc < 0 ? strerror(errno) : ""));
	if (rc < 0)	rc = IOSMERR_READLINK_FAILED;
	else {
	    iosm->rdnb = rc;
	    iosm->rdbuf[iosm->rdnb] = '\0';
	    rc = 0;
	}
	break;
    case IOSM_CHROOT:
	break;

    case IOSM_NEXT:
	rc = iosmUNSAFE(iosm, IOSM_HREAD);
	if (rc) break;
	if (!strcmp(iosm->path, CPIO_TRAILER)) { /* Detect end-of-payload. */
	    iosm->path = _free(iosm->path);
	    rc = IOSMERR_HDR_TRAILER;
	}
	if (!rc)
	    rc = iosmNext(iosm, IOSM_POS);
	break;
    case IOSM_EAT:
	for (left = st->st_size; left > 0; left -= iosm->rdnb) {
	    iosm->wrlen = (left > iosm->wrsize ? iosm->wrsize : left);
	    rc = iosmNext(iosm, IOSM_DREAD);
	    if (rc)
		/*@loopbreak@*/ break;
	}
	break;
    case IOSM_POS:
	left = (iosm->blksize - (fdGetCpioPos(iosm->cfd) % iosm->blksize)) % iosm->blksize;
	if (left) {
	    iosm->wrlen = left;
	    (void) iosmNext(iosm, IOSM_DREAD);
	}
	break;
    case IOSM_PAD:
	left = (iosm->blksize - (fdGetCpioPos(iosm->cfd) % iosm->blksize)) % iosm->blksize;
	if (left) {
	    if (iosm->blksize == 2)
		iosm->rdbuf[0] = '\n';	/* XXX ar(1) pads with '\n' */
	    else
		memset(iosm->rdbuf, 0, left);
	    /* XXX DWRITE uses rdnb for I/O length. */
	    iosm->rdnb = left;
	    (void) iosmNext(iosm, IOSM_DWRITE);
	}
	break;
    case IOSM_TRAILER:
	rc = (*iosm->trailerWrite) (iosm);	/* Write payload trailer. */
	break;
    case IOSM_HREAD:
	rc = iosmNext(iosm, IOSM_POS);
	if (!rc)
	    rc = (*iosm->headerRead) (iosm, st);/* Read next payload header. */
	break;
    case IOSM_HWRITE:
	rc = (*iosm->headerWrite) (iosm, st);	/* Write next payload header. */
	break;
    case IOSM_DREAD:
	iosm->rdnb = Fread(iosm->wrbuf, sizeof(*iosm->wrbuf), iosm->wrlen, iosm->cfd);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, %d, cfd)\trdnb %d\n",
		cur, (iosm->wrbuf == iosm->wrb ? "wrbuf" : "mmap"),
		(int)iosm->wrlen, (int)iosm->rdnb);
	if (iosm->rdnb != iosm->wrlen || Ferror(iosm->cfd))
	    rc = IOSMERR_READ_FAILED;
	if (iosm->rdnb > 0)
	    fdSetCpioPos(iosm->cfd, fdGetCpioPos(iosm->cfd) + iosm->rdnb);
	break;
    case IOSM_DWRITE:
	iosm->wrnb = Fwrite(iosm->rdbuf, sizeof(*iosm->rdbuf), iosm->rdnb, iosm->cfd);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, %d, cfd)\twrnb %d\n",
		cur, (iosm->rdbuf == iosm->rdb ? "rdbuf" : "mmap"),
		(int)iosm->rdnb, (int)iosm->wrnb);
	if (iosm->rdnb != iosm->wrnb || Ferror(iosm->cfd))
	    rc = IOSMERR_WRITE_FAILED;
	if (iosm->wrnb > 0)
	    fdSetCpioPos(iosm->cfd, fdGetCpioPos(iosm->cfd) + iosm->wrnb);
	break;

    case IOSM_ROPEN:
	iosm->rfd = Fopen(iosm->path, "r.fdio");
	if (iosm->rfd == NULL || Ferror(iosm->rfd)) {
	    if (iosm->rfd != NULL)	(void) iosmNext(iosm, IOSM_RCLOSE);
	    iosm->rfd = NULL;
	    rc = IOSMERR_OPEN_FAILED;
	    break;
	}
#if defined(POSIX_FADV_WILLNEED)
	(void) Fadvise(iosm->rfd, 0, 0, POSIX_FADV_WILLNEED);
#endif
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, \"r\") rfd %p rdbuf %p\n", cur,
		iosm->path, iosm->rfd, iosm->rdbuf);
	break;
    case IOSM_READ:
	iosm->rdnb = Fread(iosm->rdbuf, sizeof(*iosm->rdbuf), iosm->rdlen, iosm->rfd);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (rdbuf, %d, rfd)\trdnb %d\n",
		cur, (int)iosm->rdlen, (int)iosm->rdnb);
	if (iosm->rdnb != iosm->rdlen || Ferror(iosm->rfd))
	    rc = IOSMERR_READ_FAILED;
	break;
    case IOSM_RCLOSE:
	if (iosm->rfd != NULL) {
	    if (iosm->debug && (stage & IOSM_SYSCALL))
		rpmlog(RPMLOG_DEBUG, " %8s (%p)\n", cur, iosm->rfd);
	    (void) rpmswAdd(&iosm->op_digest,
			fdstat_op(iosm->rfd, FDSTAT_DIGEST));
	    (void) Fclose(iosm->rfd);
	    errno = saveerrno;
	}
	iosm->rfd = NULL;
	break;
    case IOSM_WOPEN:
	iosm->wfd = Fopen(iosm->path, "w.fdio");
	if (iosm->wfd == NULL || Ferror(iosm->wfd)) {
	    if (iosm->wfd != NULL)	(void) iosmNext(iosm, IOSM_WCLOSE);
	    iosm->wfd = NULL;
	    rc = IOSMERR_OPEN_FAILED;
	}
#if defined(POSIX_FADV_DONTNEED)
	else
	    (void) Fadvise(iosm->wfd, 0, 0, POSIX_FADV_DONTNEED);
#endif
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (%s, \"w\") wfd %p wrbuf %p\n", cur,
		iosm->path, iosm->wfd, iosm->wrbuf);
	break;
    case IOSM_WRITE:
	iosm->wrnb = Fwrite(iosm->wrbuf, sizeof(*iosm->wrbuf), iosm->rdnb, iosm->wfd);
	if (iosm->debug && (stage & IOSM_SYSCALL))
	    rpmlog(RPMLOG_DEBUG, " %8s (wrbuf, %d, wfd)\twrnb %d\n",
		cur, (int)iosm->rdnb, (int)iosm->wrnb);
	if (iosm->rdnb != iosm->wrnb || Ferror(iosm->wfd))
	    rc = IOSMERR_WRITE_FAILED;
	break;
    case IOSM_WCLOSE:
	if (iosm->wfd != NULL) {
	    if (iosm->debug && (stage & IOSM_SYSCALL))
		rpmlog(RPMLOG_DEBUG, " %8s (%p)\n", cur, iosm->wfd);
	    (void) rpmswAdd(&iosm->op_digest,
			fdstat_op(iosm->wfd, FDSTAT_DIGEST));
	    (void) Fclose(iosm->wfd);
	    errno = saveerrno;
	}
	iosm->wfd = NULL;
	break;

    default:
	break;
    }

    if (!(stage & IOSM_INTERNAL)) {
	iosm->rc = (rc == IOSMERR_HDR_TRAILER ? 0 : rc);
    }
    return rc;
}
/*@=compmempass@*/

#define	IOSM_SKIPPING(_a)	\
    ((_a) == FA_SKIP || (_a) == FA_SKIPNSTATE || (_a) == FA_SKIPNETSHARED || (_a) == FA_SKIPCOLOR)

int iosmFileActionSkipped(iosmFileAction action)
{
    return IOSM_SKIPPING(action);
}

/*@observer@*/ const char * iosmFileActionString(iosmFileAction a)
{
    switch (a) {
    case FA_UNKNOWN:	return "unknown";
    case FA_CREATE:	return "create";
    case FA_COPYOUT:	return "copyout";
    case FA_COPYIN:	return "copyin";
    case FA_BACKUP:	return "backup";
    case FA_SAVE:	return "save";
    case FA_SKIP:	return "skip";
    case FA_ALTNAME:	return "altname";
    case FA_ERASE:	return "erase";
    case FA_SKIPNSTATE: return "skipnstate";
    case FA_SKIPNETSHARED: return "skipnetshared";
    case FA_SKIPCOLOR:	return "skipcolor";
    default:		return "???";
    }
    /*@notreached@*/
}

/*@observer@*/ const char * iosmFileStageString(iosmFileStage a) {
    switch(a) {
    case IOSM_UNKNOWN:	return "unknown";

    case IOSM_PKGINSTALL:return "INSTALL";
    case IOSM_PKGERASE:	return "ERASE";
    case IOSM_PKGBUILD:	return "BUILD";
    case IOSM_PKGCOMMIT:	return "COMMIT";
    case IOSM_PKGUNDO:	return "UNDO";

    case IOSM_CREATE:	return "create";
    case IOSM_INIT:	return "init";
    case IOSM_MAP:	return "map";
    case IOSM_MKDIRS:	return "mkdirs";
    case IOSM_RMDIRS:	return "rmdirs";
    case IOSM_PRE:	return "pre";
    case IOSM_PROCESS:	return "process";
    case IOSM_POST:	return "post";
    case IOSM_MKLINKS:	return "mklinks";
    case IOSM_NOTIFY:	return "notify";
    case IOSM_UNDO:	return "undo";
    case IOSM_FINI:	return "fini";
    case IOSM_COMMIT:	return "commit";
    case IOSM_DESTROY:	return "destroy";
    case IOSM_VERIFY:	return "verify";

    case IOSM_UNLINK:	return "Unlink";
    case IOSM_RENAME:	return "Rename";
    case IOSM_MKDIR:	return "Mkdir";
    case IOSM_RMDIR:	return "Rmdir";
    case IOSM_LSETFCON:	return "lsetfcon";
    case IOSM_CHOWN:	return "Chown";
    case IOSM_LCHOWN:	return "Lchown";
    case IOSM_CHMOD:	return "Chmod";
    case IOSM_UTIME:	return "Utime";
    case IOSM_SYMLINK:	return "Symlink";
    case IOSM_LINK:	return "Link";
    case IOSM_MKFIFO:	return "Mkfifo";
    case IOSM_MKNOD:	return "Mknod";
    case IOSM_LSTAT:	return "Lstat";
    case IOSM_STAT:	return "Stat";
    case IOSM_READLINK:	return "Readlink";
    case IOSM_CHROOT:	return "Chroot";

    case IOSM_NEXT:	return "next";
    case IOSM_EAT:	return "eat";
    case IOSM_POS:	return "pos";
    case IOSM_PAD:	return "pad";
    case IOSM_TRAILER:	return "trailer";
    case IOSM_HREAD:	return "hread";
    case IOSM_HWRITE:	return "hwrite";
    case IOSM_DREAD:	return "Fread";
    case IOSM_DWRITE:	return "Fwrite";

    case IOSM_ROPEN:	return "Fopen";
    case IOSM_READ:	return "Fread";
    case IOSM_RCLOSE:	return "Fclose";
    case IOSM_WOPEN:	return "Fopen";
    case IOSM_WRITE:	return "Fwrite";
    case IOSM_WCLOSE:	return "Fclose";

    default:		return "???";
    }
    /*@noteached@*/
}

char * iosmStrerror(int rc)
{
    char msg[256];
    const char *s;
    int l, myerrno = errno;

    strcpy(msg, "cpio: ");
    switch (rc) {
    default:
	s = msg + strlen(msg);
	sprintf((char *)s, _("(error 0x%x)"), (unsigned)rc);
	s = NULL;
	break;
    case IOSMERR_BAD_MAGIC:	s = _("Bad magic");		break;
    case IOSMERR_BAD_HEADER:	s = _("Bad/unreadable header");	break;

    case IOSMERR_OPEN_FAILED:	s = "open";	break;
    case IOSMERR_CHMOD_FAILED:	s = "chmod";	break;
    case IOSMERR_CHOWN_FAILED:	s = "chown";	break;
    case IOSMERR_WRITE_FAILED:	s = "write";	break;
    case IOSMERR_UTIME_FAILED:	s = "utime";	break;
    case IOSMERR_UNLINK_FAILED:	s = "unlink";	break;
    case IOSMERR_RENAME_FAILED:	s = "rename";	break;
    case IOSMERR_SYMLINK_FAILED: s = "symlink";	break;
    case IOSMERR_STAT_FAILED:	s = "stat";	break;
    case IOSMERR_LSTAT_FAILED:	s = "lstat";	break;
    case IOSMERR_MKDIR_FAILED:	s = "mkdir";	break;
    case IOSMERR_RMDIR_FAILED:	s = "rmdir";	break;
    case IOSMERR_MKNOD_FAILED:	s = "mknod";	break;
    case IOSMERR_MKFIFO_FAILED:	s = "mkfifo";	break;
    case IOSMERR_LINK_FAILED:	s = "link";	break;
    case IOSMERR_READLINK_FAILED: s = "readlink";	break;
    case IOSMERR_READ_FAILED:	s = "read";	break;
    case IOSMERR_COPY_FAILED:	s = "copy";	break;
    case IOSMERR_LSETFCON_FAILED: s = "lsetfilecon";	break;

    case IOSMERR_HDR_SIZE:	s = _("Header size too big");	break;
    case IOSMERR_UNKNOWN_FILETYPE: s = _("Unknown file type");	break;
    case IOSMERR_MISSING_HARDLINK: s = _("Missing hard link(s)"); break;
    case IOSMERR_DIGEST_MISMATCH: s = _("File digest mismatch");	break;
    case IOSMERR_INTERNAL:	s = _("Internal error");	break;
    case IOSMERR_UNMAPPED_FILE:	s = _("Archive file not in header"); break;
    case IOSMERR_ENOENT:	s = strerror(ENOENT); break;
    case IOSMERR_ENOTEMPTY:	s = strerror(ENOTEMPTY); break;
    }

    l = sizeof(msg) - strlen(msg) - 1;
    if (s != NULL) {
	if (l > 0) strncat(msg, s, l);
	l -= strlen(s);
    }
    if ((rc & IOSMERR_CHECK_ERRNO) && myerrno) {
	s = _(" failed - ");
	if (l > 0) strncat(msg, s, l);
	l -= strlen(s);
	if (l > 0) strncat(msg, strerror(myerrno), l);
    }
    return xstrdup(msg);
}
