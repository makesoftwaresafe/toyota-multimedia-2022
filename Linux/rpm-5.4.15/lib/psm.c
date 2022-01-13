/** \ingroup rpmts payload
 * \file lib/psm.c
 * Package state machine to handle a package from a transaction set.
 */

#include "system.h"

#define	_MIRE_INTERNAL	/* XXX mireApply doesn't tell which pattern matched. */

#include <rpmio_internal.h>	/* XXX FDSTAT_READ */
#include <rpmcb.h>		/* XXX fnpyKey */
#include <rpmsx.h>
#include <rpmmacro.h>
#include <rpmurl.h>

#include <rpmaug.h>
#include <rpmficl.h>
#include <rpmjni.h>
#include <rpmjs.h>
#include <rpmlua.h>
#include <rpmmrb.h>
#include <rpmperl.h>
#include <rpmpython.h>
#include <rpmruby.h>
#include <rpmsm.h>
#include <rpmsql.h>
#include <rpmsquirrel.h>
#include <rpmtcl.h>

#if defined(WITH_LUA) || defined(WITH_AUGEAS) || defined(WITH_FICL) || defined(WITH_GPSEE) || defined(WITH_MRBEMBED) || defined(WITH_PERLEMBED) || defined(WITH_PYTHONEMBED) || defined(WITH_RUBYEMBED) || defined(WITH_SEMANAGE) || defined(WITH_SQLITE) || defined(WITH_SQUIRREL) || defined(WITH_TCL)
#define	_WITH_EMBEDDED
#else
#undef _WITH_ENBEDDED
#endif

#include <rpmtag.h>
#include <rpmtypes.h>
#include <pkgio.h>
#define	_RPMDB_INTERNAL
#include <rpmdb.h>		/* XXX for db_chrootDone */
#include <rpmtxn.h>
#include "signature.h"		/* signature constants */
#include <rpmlib.h>

#define	_RPMFI_INTERNAL
#include "rpmfi.h"
#include "fsm.h"		/* XXX CPIO_FOO/IOSM_FOO constants */
#define	_RPMSQ_INTERNAL
#define	_RPMPSM_INTERNAL
#include "psm.h"
#define F_ISSET(_psm, _FLAG)	((_psm)->flags & (RPMPSM_FLAGS_##_FLAG))
#define F_SET(_psm, _FLAG) { \
	unsigned ui = (unsigned)(_psm)->flags; ui |= (RPMPSM_FLAGS_##_FLAG); \
	(_psm)->flags = (rpmpsmFlags) ui; \
	}
#define F_CLR(_psm, _FLAG) { \
	unsigned ui = (unsigned)(_psm)->flags; ui &= ~(RPMPSM_FLAGS_##_FLAG); \
	(_psm)->flags = (rpmpsmFlags) ui; \
	}

#define	_RPMEVR_INTERNAL
#include "rpmds.h"

#define	_RPMTE_INTERNAL
#include "rpmte.h"

#define	_RPMTS_INTERNAL		/* XXX ts->notify */
#include "rpmts.h"

#include "misc.h"		/* XXX rpmMkdirPath, makeTempFile, doputenv */

#include <rpmcli.h>

#include "debug.h"

#define	_PSM_DEBUG	0
/*@unchecked@*/
int _psm_debug = _PSM_DEBUG;
/*@unchecked@*/
int _psm_threads = 0;

/*@access FD_t @*/		/* XXX void * arg */
/*@access Header @*/		/* XXX void * arg */
/*@access miRE @*/

/*@access rpmpsm @*/

/*@access rpmfi @*/
/*@access rpmte @*/	/* XXX rpmInstallSourcePackage */
/*@access rpmts @*/	/* XXX ts->notify */

/*@access rpmluav @*/

#ifdef __cplusplus
GENfree(HE_t)
GENfree(int *)
GENfree(const struct stat *)
#endif	/* __cplusplus */

#ifdef	DYING
/**
 * Mark files in database shared with this package as "replaced".
 * @param psm		package state machine data
 * @return		0 always
 */
static rpmRC markReplacedFiles(const rpmpsm psm)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies psm, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const rpmts ts = psm->ts;
    rpmte te = psm->te;
    rpmfi fi = psm->fi;
    sharedFileInfo replaced = (te ? te->replaced : NULL);
    sharedFileInfo sfi;
    rpmmi mi;
    Header h;
    uint32_t * offsets;
    rpmuint32_t prev;
    int num;
    int xx;

    if (!(rpmfiFC(fi) > 0 && replaced != NULL))
	return RPMRC_OK;

    num = prev = 0;
    for (sfi = replaced; sfi->otherPkg; sfi++) {
	if (prev && prev == sfi->otherPkg)
	    continue;
	prev = sfi->otherPkg;
	num++;
    }
    if (num == 0)
	return RPMRC_OK;

    offsets = (uint32_t *) alloca(num * sizeof(*offsets));
    offsets[0] = 0;
    num = prev = 0;
    for (sfi = replaced; sfi->otherPkg; sfi++) {
	if (prev && prev == sfi->otherPkg)
	    continue;
	prev = sfi->otherPkg;
	offsets[num++] = sfi->otherPkg;
    }

    mi = rpmtsInitIterator(ts, RPMDBI_PACKAGES, NULL, 0);
    xx = rpmmiGrow(mi, offsets, num);
    xx = rpmmiSetRewrite(mi, 1);

    sfi = replaced;
    while ((h = rpmmiNext(mi)) != NULL) {
	int modified;

	modified = 0;

	/* XXX FIXME: not correct yet, but headerGetEntry needs to die now! */
	he->tag = RPMTAG_FILESTATES;
	xx = headerGet(h, he, 0);
	if (!xx)
	    continue;
	
	prev = rpmmiInstance(mi);
	num = 0;
	while (sfi->otherPkg && sfi->otherPkg == prev) {
assert(sfi->otherFileNum < he->c);
	    if (he->p.ui8p[sfi->otherFileNum] != RPMFILE_STATE_REPLACED) {
		he->p.ui8p[sfi->otherFileNum] = RPMFILE_STATE_REPLACED;
		if (modified == 0) {
		    /* Modified header will be rewritten. */
		    modified = 1;
		    xx = rpmmiSetModified(mi, modified);
		}
		num++;
	    }
	    sfi++;
	}
	he->p.ptr = _free(he->p.ptr);
    }
    mi = rpmmiFree(mi);

    return RPMRC_OK;
}
#endif

static rpmRC createDir(rpmts ts, rpmfi fi, const char ** fn, const char * name)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies *fn, rpmGlobalMacroContext @*/
{
    const char * N = rpmGenPath(rpmtsRootDir(ts), name, "");
    char * t = xstrdup(name+2);
    rpmRC rc;

    t[strlen(t)-1] = '\0';

    rc = rpmMkdirPath(N, t+1);
    if (rc != RPMRC_OK) {
    	if (Access(N, W_OK))
    	    rpmlog(RPMLOG_ERR, _("cannot write to %%%s %s\n"), t, N);
	else if (fi && Chown(N, fi->uid, fi->gid)) /* XXX coverity #1035724 */
    	    rpmlog(RPMLOG_ERR, _("cannot chown %%%s %s\n"), t, N);
    }

    if (fn)
	*fn = N;
    else
	N = _free(N);
    t = _free(t);

    return rc;
}

rpmRC rpmInstallSourcePackage(rpmts ts, void * _fd,
		const char ** specFilePtr, const char ** cookie)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    FD_t fd = (FD_t) _fd;
    rpmfi fi = NULL;
    rpmte p = NULL;
    rpmpsm psm = NULL;
    Header h = NULL;
    int isSource;
    rpmRC rc;
    int i;

/*@-mods@*/	/* Avoid void * _fd annotations for now. */
    rc = rpmReadPackageFile(ts, fd, __FUNCTION__, &h);
/*@=mods@*/
    switch (rc) {
    case RPMRC_NOTTRUSTED:
    case RPMRC_NOKEY:
    case RPMRC_OK:
	break;
    default:
	goto exit;
	/*@notreached@*/ break;
    }
    if (h == NULL)
	goto exit;

    rc = RPMRC_FAIL;		/* assume failure */

    isSource =
	(headerIsEntry(h, RPMTAG_SOURCERPM) == 0 &&
	 headerIsEntry(h, RPMTAG_ARCH) != 0);

    if (!isSource) {
	rpmlog(RPMLOG_ERR, _("source package expected, binary found\n"));
	goto exit;
    }

    (void) rpmtsAddInstallElement(ts, h, NULL, 0, NULL);

    p = rpmtsElement(ts, 0);
assert(p->h == NULL);
    (void) rpmteSetHeader(p, h);
/*@-mods@*/	/* LCL: avoid void * _fd annotation for now. */
/*@-assignexpose -castexpose -temptrans @*/
    p->fd = fdLink(fd, __FUNCTION__);
/*@=assignexpose =castexpose =temptrans @*/
/*@=mods@*/

    fi = rpmteFI(p, RPMTAG_BASENAMES);
    fi->h = headerLink(h);
/*@-onlytrans@*/	/* FIX: te reference */
    fi->te = p;
/*@=onlytrans@*/

    /* XXX FIXME: don't do per-file mapping, force global flags. */
    fi->fmapflags = _free(fi->fmapflags);
    fi->mapflags = IOSM_MAP_PATH | IOSM_MAP_MODE | IOSM_MAP_UID | IOSM_MAP_GID;

    fi->uid = getuid();
    fi->gid = getgid();
#if defined(RPM_VENDOR_OPENPKG) /* switch-from-susr-to-musr-on-srpm-install */
    /* If running as the OpenPKG "susr", do not unpack source RPM
       packages with "susr" file ownerships as the OpenPKG Set-UID
       wrapper switches from "musr" to "susr" on "openpkg rpm -Uvh
       *.src.rpm". As a result the installed files could be never
       removed again by "musr". It is more consistent to always unpack
       as "musr" if possible. */
    if (fi->uid == 0) {
        char *muid_str;
        char *mgid_str;
        uid_t muid;
        gid_t mgid;
        if ((muid_str = rpmExpand("%{l_muid}", NULL)) != NULL)
            if ((muid = (uid_t)strtol(muid_str, (char **)NULL, 10)) > 0)
                fi->uid = muid;
        if ((mgid_str = rpmExpand("%{l_mgid}", NULL)) != NULL)
            if ((mgid = (gid_t)strtol(mgid_str, (char **)NULL, 10)) > 0)
                fi->gid = mgid;
    }
#endif
    for (i = 0; i < (int)fi->fc; i++)
	fi->actions[i] = FA_CREATE;

    /* Load relative (in a *.src.rpm) file paths as an argv array. */
    fi->astriplen = 0;
    fi->striplen = 0;
    he->tag = RPMTAG_FILEPATHS;
    if (!headerGet(h, he, 0) || he->p.argv == NULL || he->p.argv[0] == NULL)
	goto exit;
    fi->apath = he->p.argv;

    (void) headerMacrosLoad(h);

#if defined(RPM_VENDOR_OPENPKG) /* switch-from-susr-to-musr-on-srpm-install */
    if (createDir(ts, fi, NULL, "%{_topdir}")
     || createDir(ts, fi, NULL, "%{_builddir}")
     || createDir(ts, fi, NULL, "%{_rpmdir}")
     || createDir(ts, fi, NULL, "%{_srcrpmdir}")
     || createDir(ts, fi, NULL, "%{_sourcedir}")
     || createDir(ts, fi, NULL, "%{_specdir}"))
#else
    if (createDir(ts, NULL, NULL, "%{_topdir}")
     || createDir(ts, NULL, NULL, "%{_builddir}")
     || createDir(ts, NULL, NULL, "%{_rpmdir}")
     || createDir(ts, NULL, NULL, "%{_srcrpmdir}")
     || createDir(ts, NULL, NULL, "%{_sourcedir}")
     || createDir(ts, NULL, NULL, "%{_specdir}"))
#endif
	goto exit;

    /* Retrieve build cookie. */
    if (cookie) {
	*cookie = NULL;
	he->tag = RPMTAG_COOKIE;
	if (headerGet(h, he, 0)) *cookie = he->p.str;
    }

    /* Find spec file path. */
    if (specFilePtr) {
	*specFilePtr = NULL;
	fi = rpmfiInit(fi, 0);
	while ((i = rpmfiNext(fi)) >= 0) {
	    if (!(rpmfiFFlags(fi) & RPMFILE_SPECFILE))
		continue;
	    *specFilePtr = xstrdup(rpmfiFN(fi));
	    break;
	}
	if (*specFilePtr == NULL) {
	    rpmlog(RPMLOG_ERR, _("source package contains no .spec file\n"));
	    goto exit;
	}
    }

    /* Unpack the SRPM contents. */
    psm = rpmpsmNew(ts, p, fi);
    psm->goal = PSM_PKGINSTALL;
    rc = rpmpsmStage(psm, PSM_PROCESS);
    (void) rpmpsmStage(psm, PSM_FINI);
    psm = rpmpsmFree(psm, __FUNCTION__);

exit:
    if (rc != RPMRC_OK) {
	if (specFilePtr) *specFilePtr = _free(*specFilePtr);
	if (cookie) *cookie = _free(*cookie);
    }

    if (fi)
	fi->te = NULL;

    if (p) {
	(void) rpmteSetHeader(p, NULL);
/*@-mods@*/	/* Avoid void * _fd annotations for now. */
	if (p->fd != NULL)
	    (void) Fclose(p->fd);
/*@=mods@*/
	p->fd = NULL;
    }

    /* XXX nuke the added package(s). */
    rpmtsClean(ts);

    (void) headerFree(h);
    h = NULL;

    return rc;
}

/*@observer@*/ /*@unchecked@*/
static const char * SCRIPT_PATH =
	"PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin";

/**
 * Return scriptlet name from tag.
 * @param tag		scriptlet tag
 * @return		name of scriptlet
 */
static /*@observer@*/ const char * tag2sln(rpmTag tag)
	/*@*/
{
    switch (tag) {
    case RPMTAG_PRETRANS:	return "%pretrans";
    case RPMTAG_TRIGGERPREIN:	return "%triggerprein";
    case RPMTAG_PREIN:		return "%pre";
    case RPMTAG_POSTIN:		return "%post";
    case RPMTAG_TRIGGERIN:	return "%triggerin";
    case RPMTAG_TRIGGERUN:	return "%triggerun";
    case RPMTAG_PREUN:		return "%preun";
    case RPMTAG_POSTUN:		return "%postun";
    case RPMTAG_POSTTRANS:	return "%posttrans";
    case RPMTAG_TRIGGERPOSTUN:	return "%triggerpostun";
    case RPMTAG_VERIFYSCRIPT:	return "%verify";
    case RPMTAG_SANITYCHECK:	return "%sanitycheck";
    case RPMTAG_BUILDPREP:	return "%prep";
    case RPMTAG_BUILDBUILD:	return "%build";
    case RPMTAG_BUILDINSTALL:	return "%install";
    case RPMTAG_BUILDCHECK:	return "%check";
    default:	break;
    }
    return "%unknownscript";
}

/**
 * Return scriptlet id from tag.
 * @param tag		scriptlet tag
 * @return		id of scriptlet
 */
static rpmScriptID tag2slx(rpmTag tag)
	/*@*/
{
    switch (tag) {
    case RPMTAG_PRETRANS:	return RPMSCRIPT_PRETRANS;
    case RPMTAG_TRIGGERPREIN:	return RPMSCRIPT_TRIGGERPREIN;
    case RPMTAG_PREIN:		return RPMSCRIPT_PREIN;
    case RPMTAG_POSTIN:		return RPMSCRIPT_POSTIN;
    case RPMTAG_TRIGGERIN:	return RPMSCRIPT_TRIGGERIN;
    case RPMTAG_TRIGGERUN:	return RPMSCRIPT_TRIGGERUN;
    case RPMTAG_PREUN:		return RPMSCRIPT_PREUN;
    case RPMTAG_POSTUN:		return RPMSCRIPT_POSTUN;
    case RPMTAG_POSTTRANS:	return RPMSCRIPT_POSTTRANS;
    case RPMTAG_TRIGGERPOSTUN:	return RPMSCRIPT_TRIGGERPOSTUN;
    case RPMTAG_VERIFYSCRIPT:	return RPMSCRIPT_VERIFY;
    case RPMTAG_SANITYCHECK:	return RPMSCRIPT_SANITYCHECK;
    case RPMTAG_BUILDPREP:	return RPMSCRIPT_PREP;
    case RPMTAG_BUILDBUILD:	return RPMSCRIPT_BUILD;
    case RPMTAG_BUILDINSTALL:	return RPMSCRIPT_INSTALL;
    case RPMTAG_BUILDCHECK:	return RPMSCRIPT_CHECK;
    default:	break;
    }
    return RPMSCRIPT_MAX;
}

/**
 * Wait for child process to be reaped.
 * @param psm		package state machine data
 * @return		
 */
static pid_t psmWait(rpmpsm psm)
	/*@globals fileSystem, internalState @*/
	/*@modifies psm, fileSystem, internalState @*/
{
    const rpmts ts = psm->ts;
    rpmtime_t msecs;

    (void) rpmsqWait(&psm->sq);
    msecs = psm->sq.op.usecs/1000;
    (void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_SCRIPTLETS), &psm->sq.op);

    rpmlog(RPMLOG_DEBUG,
	D_("%s: waitpid(%d) rc %d status %x secs %u.%03u\n"),
	psm->stepName, (unsigned)psm->sq.child,
	(unsigned)psm->sq.reaped, psm->sq.status,
	(unsigned)msecs/1000, (unsigned)msecs%1000);

    if (psm->sstates != NULL)
    {	rpmuint32_t * ssp = psm->sstates + tag2slx(psm->scriptTag);
	*ssp &= ~0xffff;
	*ssp |= (psm->sq.status & 0xffff);
	*ssp |= RPMSCRIPT_STATE_REAPED;
    }

    return psm->sq.reaped;
}

#ifdef WITH_LUA
/**
 * Run internal Lua script.
 * @param psm		package state machine data
 * @param sln		name of scriptlet section
 * @param Phe		scriptlet args, Phe->p.argv[0] is interpreter to use
 * @param script	scriptlet body
 * @param arg1		no. instances of package installed after scriptlet exec
 *			(-1 is no arg)
 * @param arg2		ditto, but for the target package
 * @return		RPMRC_OK on success
 */
static rpmRC runLuaScript(rpmpsm psm, const char * sln, HE_t Phe,
		   const char *script, int arg1, int arg2)
	/*@globals fileSystem, internalState @*/
	/*@modifies psm, fileSystem, internalState @*/
{
    rpmRC rc = RPMRC_OK;
    int xx;
    rpmlua lua = NULL;	/* Global state. */
    rpmluav var;

    /* Create arg variable */
    rpmluaPushTable(lua, "arg");
    var = rpmluavNew();
    rpmluavSetListMode(var, 1);
/*@+relaxtypes@*/
    if (Phe->p.argv) {
	int i;
	for (i = 0; i < (int)Phe->c && Phe->p.argv[i]; i++) {
	    rpmluavSetValue(var, RPMLUAV_STRING, Phe->p.argv[i]);
	    rpmluaSetVar(lua, var);
	}
    }
    if (arg1 >= 0) {
	rpmluavSetValueNum(var, arg1);
	rpmluaSetVar(lua, var);
    }
    if (arg2 >= 0) {
	rpmluavSetValueNum(var, arg2);
	rpmluaSetVar(lua, var);
    }
/*@=relaxtypes@*/
/*@-moduncon@*/
    var = (rpmluav) rpmluavFree(var);
/*@=moduncon@*/
    rpmluaPop(lua);

    {	char buf[BUFSIZ];
	xx = snprintf(buf, BUFSIZ, "%s(%s)", sln, psm->NVRA);
	xx = rpmluaRunScript(lua, script, buf);
	if (xx == -1) {
	    void * ptr = rpmtsNotify(psm->ts, psm->te, RPMCALLBACK_SCRIPT_ERROR,
				 psm->scriptTag, 1);
	    ptr = ptr;	/* XXX keep gcc happy. */
	    rc = RPMRC_FAIL;
	} else
	    rc = RPMRC_OK;
    }
    rpmluaDelVar(lua, "arg");

    return rc;
}
#endif	/* WITH_LUA */

#if defined(_WITH_EMBEDDED)
static int enterChroot(rpmpsm psm, int * pwdFdnop, int * rootFdnop)
	/*@globals fileSystem, internalState @*/
	/*@modifies *pwdFdnop, *rootFdnop, fileSystem, internalState @*/
{
    const rpmts ts = psm->ts;
    int inChroot;
    int xx;

    /* Save the current working directory. */
    if (pwdFdnop)
	(*pwdFdnop) = open(".", O_RDONLY, 0);

    /* Save the current root directory. */
    if (rootFdnop)
	(*rootFdnop) = open("/", O_RDONLY, 0);

    /* Get into the chroot. */
    if (!rpmtsChrootDone(ts)) {
	const char *rootDir = rpmtsRootDir(ts);
	inChroot = 0;
	/*@-modobserver @*/
	if (rootDir != NULL && strcmp(rootDir, "/") && *rootDir == '/') {
	    xx = Chroot(rootDir);
	/*@=modobserver @*/
	    xx = rpmtsSetChrootDone(ts, 1);
	}
    } else
       inChroot = 1;

    /* All embedded scriptlets run with CWD == "/". */
    xx = Chdir("/");

    return inChroot;
}

static int exitChroot(rpmpsm psm, int inChroot, int pwdFdno, int rootFdno)
	/*@globals fileSystem, internalState @*/
	/*@modifies psm, fileSystem, internalState @*/
{
    const rpmts ts = psm->ts;
    const char *rootDir = rpmtsRootDir(ts);
    int xx;

    if (rpmtsChrootDone(ts) && !inChroot) {
	xx = fchdir(rootFdno);
/*@-modobserver@*/
	if (rootDir != NULL && strcmp(rootDir, "/") && *rootDir == '/') {
	    xx = Chroot(".");
/*@=modobserver@*/
	    xx = rpmtsSetChrootDone(ts, 0);
	}
	xx = fchdir(pwdFdno);
    } else
	xx = fchdir(pwdFdno);

    xx = close(rootFdno);
    xx = close(pwdFdno);

    return 0;
}

/**
 * Run embedded interpreter script.
 * @param psm		package state machine data
 * @param sln		name of scriptlet section
 * @param Phe		scriptlet args, Phe->p.argv[0] is interpreter to use
 * @param script	scriptlet body
 * @param arg1		no. instances of package installed after scriptlet exec
 *			(-1 is no arg)
 * @param arg2		ditto, but for the target package
 * @return		RPMRC_OK on success
 */
static rpmRC runEmbeddedScript(rpmpsm psm, const char * sln, HE_t Phe,
		   const char *script, int arg1, int arg2)
	/*@globals fileSystem, internalState @*/
	/*@modifies psm, fileSystem, internalState @*/
{
    char * av[] = { NULL, NULL, NULL, NULL };
    int pwdFdno = -1;
    int rootFdno = -1;
    rpmRC rc = RPMRC_OK;
    int xx = 0;
    rpmuint32_t * ssp = NULL;
    int inChroot = enterChroot(psm, &pwdFdno, &rootFdno);

    if (psm->sstates != NULL)
	ssp = psm->sstates + tag2slx(psm->scriptTag);
    if (ssp != NULL)
	*ssp |= (RPMSCRIPT_STATE_EMBEDDED|RPMSCRIPT_STATE_EXEC);

    av[0] = (char *) Phe->p.argv[0];
    /* XXX coverity #1035772 */
    if (arg1 >= 0) {
	av[1] = (char *) alloca(32);
	(void) sprintf(av[1], "%d", arg1);
    }
    if (arg2 >= 0) {
	av[2] = (char *) alloca(32);
	(void) sprintf(av[2], "%d", arg2);
     }

#if defined(WITH_LUA)
    if (!strcmp(Phe->p.argv[0], "<lua>")) {
	rc = runLuaScript(psm, sln, Phe, script, arg1, arg2);
    } else
#endif
#if defined(WITH_AUGEAS)
    if (!strcmp(Phe->p.argv[0], "<augeas>")) {
	/* XXX change rpmaugNew() to common embedded interpreter API */
	rpmaug aug = NULL;
	rc = rpmaugRun(aug, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	aug = rpmaugFree(aug);
    } else
#endif
#if defined(WITH_FICL)
    if (!strcmp(Phe->p.argv[0], "<ficl>")) {
	rpmficl ficl = rpmficlNew((char **)av, 0);
	rc = rpmficlRun(ficl, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	ficl = rpmficlFree(ficl);
    } else
#endif
#if defined(WITH_JNIEMBED)
    if (!strcmp(Phe->p.argv[0], "<java>")) {
	rpmjni jni = rpmjniNew((char **)av, 0);
	rc = rpmjniRun(jni, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	jni = rpmjniFree(jni);
    } else
#endif
#if defined(WITH_GPSEE)
    if (!strcmp(Phe->p.argv[0], "<js>")) {
	rpmjs js = rpmjsNew((char **)av, 0);
	rc = rpmjsRun(js, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	js = rpmjsFree(js);
    } else
#endif
#if defined(WITH_MRBEMBED)
    if (!strcmp(Phe->p.argv[0], "<mruby>")) {
	rpmmrb mrb = rpmmrbNew((char **)av, 0);
	rc = rpmmrbRun(mrb, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	mrb = rpmmrbFree(mrb);
    } else
#endif
#if defined(WITH_PERLEMBED)
    if (!strcmp(Phe->p.argv[0], "<perl>")) {
	rpmperl perl = rpmperlNew((char **)av, 0);
	rc = rpmperlRun(perl, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	perl = rpmperlFree(perl);
    } else
#endif
#if defined(WITH_PYTHONEMBED)
    if (!strcmp(Phe->p.argv[0], "<python>")) {
	rpmpython python = rpmpythonNew((char **)av, 0);
	rc = rpmpythonRun(python, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	python = rpmpythonFree(python);
    } else
#endif
#if defined(WITH_RUBYEMBED)
    if (!strcmp(Phe->p.argv[0], "<ruby>")) {
	rpmruby ruby = rpmrubyNew((char **)av, 0);
	rc = rpmrubyRun(ruby, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	ruby = rpmrubyFree(ruby);
    } else
#endif
#if defined(WITH_SEMANAGE)
    if (!strcmp(Phe->p.argv[0], "<spook>")) {
	/* XXX change rpmsmNew() to common embedded interpreter API */
	rpmsm sm = NULL;
	/* XXX HACK: use an argv for now. */
	const char * av[2];
	av[0] = script;
	av[1] = NULL;
	rc = rpmsmRun(sm, (char **)av, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	sm = rpmsmFree(sm);
    } else
#endif
#if defined(WITH_SQLITE)
    if (!strcmp(Phe->p.argv[0], "<sql>")) {
	int Pac = Phe->c;
	const char ** Pav = (const char **) xmalloc((Pac + 1) * sizeof(*Pav));
	const char * result = NULL;
	rpmsql sql;
	int i;

	/* XXX ignore $1/$2, copy the tag array instead. */
	/* XXX no NULL sentinel in tag arrays. */
	for (i = 0; i < Pac; i++)
	    Pav[i] = rpmExpand(Phe->p.argv[i], NULL);
	Pav[Pac] = NULL;

	sql = rpmsqlNew((char **)Pav, 0);
	rc = rpmsqlRun(sql, script, &result) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	sql = rpmsqlFree(sql);
	Pav = argvFree(Pav);
    } else
#endif
#if defined(WITH_SQUIRREL)
    if (!strcmp(Phe->p.argv[0], "<squirrel>")) {
	rpmsquirrel squirrel = rpmsquirrelNew((char **)av, 0);
	rc = rpmsquirrelRun(squirrel, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	squirrel = rpmsquirrelFree(squirrel);
    } else
#endif
#if defined(WITH_TCL)
    if (!strcmp(Phe->p.argv[0], "<tcl>")) {
	rpmtcl tcl = rpmtclNew((char **)av, 0);
	rc = rpmtclRun(tcl, script, NULL) == RPMRC_OK
	    ? RPMRC_OK : RPMRC_FAIL;
	tcl = rpmtclFree(tcl);
    } else
#endif
	rc = RPMRC_NOTFOUND;

    if (ssp != NULL) {
	*ssp &= ~0xffff;
	*ssp |= (xx & 0xffff);
	*ssp |= RPMSCRIPT_STATE_REAPED;
    }

    xx = exitChroot(psm, inChroot, pwdFdno, rootFdno);

    return rc;
}
#endif

/**
 */
/*@unchecked@*/
static int ldconfig_done = 0;

/*@unchecked@*/ /*@observer@*/ /*@null@*/
static const char * ldconfig_path = "/sbin/ldconfig";

/**
 * Run scriptlet with args.
 *
 * Run a script with an interpreter. If the interpreter is not specified,
 * /bin/sh will be used.
 *
 * @param psm		package state machine data
 * @param h		header
 * @param sln		name of scriptlet section
 * @param Phe		scriptlet args, Phe->p.argv[0] is interpreter to use
 * @param script	scriptlet body
 * @param arg1		no. instances of package installed after scriptlet exec
 *			(-1 is no arg)
 * @param arg2		ditto, but for the target package
 * @return		RPMRC_OK on success
 */
static rpmRC runScript(rpmpsm psm, Header h, const char * sln, HE_t Phe,
		const char * script, int arg1, int arg2)
	/*@globals ldconfig_done, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState@*/
	/*@modifies psm, ldconfig_done, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    const rpmts ts = psm->ts;
    const char * NVRA = psm->NVRA;
    HE_t IPhe = psm->IPhe;
    const char ** argv = NULL;
    int argc = 0;
    const char ** IP = NULL;
    int nIP;
    size_t maxPrefixLength;
    size_t len;
    char * prefixBuf = NULL;
    const char * fn = NULL;
    FD_t scriptFd = NULL;
    FD_t out = NULL;		/* exit: expects this to be initialized. */
    rpmRC rc = RPMRC_FAIL;	/* assume failure */
    const char * body = NULL;
    rpmop op = (rpmop) memset(alloca(sizeof(*op)), 0, sizeof(*op));
    int ix = tag2slx(psm->scriptTag);
    rpmuint32_t * ssp = NULL;
    pid_t pid;
    int xx;
    int i;

    if (psm->sstates != NULL && ix >= 0 && ix < RPMSCRIPT_MAX)
	ssp = psm->sstates + ix;
    if (ssp != NULL)
	*ssp = RPMSCRIPT_STATE_UNKNOWN;

    if (Phe->p.argv == NULL && script == NULL)
	return RPMRC_OK;

    /* Macro expand all scriptlets. */
    body = rpmExpand(script, NULL);

    /* XXX Load NVRA lazily. This should be done elsewhere ... */
    if (NVRA == NULL) {
	HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
	he->tag = RPMTAG_NVRA;
	xx = headerGet(h, he, 0);
assert(he->p.str != NULL);
	psm->NVRA = NVRA = he->p.str;
    }

    if (op != NULL)
	(void) rpmswEnter(op, 0);
    
    if (Phe->p.argv && Phe->p.argv[0])
    if (!strcmp(Phe->p.argv[0], "<lua>")
     || !strcmp(Phe->p.argv[0], "<augeas>")
     || !strcmp(Phe->p.argv[0], "<ficl>")
     || !strcmp(Phe->p.argv[0], "<java>")
     || !strcmp(Phe->p.argv[0], "<js>")
     || !strcmp(Phe->p.argv[0], "<mruby>")
     || !strcmp(Phe->p.argv[0], "<perl>")
     || !strcmp(Phe->p.argv[0], "<python>")
     || !strcmp(Phe->p.argv[0], "<ruby>")
     || !strcmp(Phe->p.argv[0], "<sql>")
     || !strcmp(Phe->p.argv[0], "<squirrel>")
     || !strcmp(Phe->p.argv[0], "<tcl>"))
    {
#if defined(_WITH_EMBEDDED)
	rpmlog(RPMLOG_DEBUG,
		D_("%s: %s(%s) running %s scriptlet.\n"),
		psm->stepName, tag2sln(psm->scriptTag), NVRA, Phe->p.argv[0]);
	rc = runEmbeddedScript(psm, sln, Phe, body, arg1, arg2);
#endif
	goto exit;
    }

    psm->sq.reaper = 1;

    /*
     * If a successor node, and ldconfig was just run, don't bother.
     */
    if (ldconfig_path && Phe->p.argv != NULL && F_ISSET(psm, UNORDERED)) {
 	if (ldconfig_done && !strcmp(Phe->p.argv[0], ldconfig_path)) {
	    rpmlog(RPMLOG_DEBUG,
		D_("%s: %s(%s) skipping redundant \"%s\".\n"),
		psm->stepName, tag2sln(psm->scriptTag), NVRA,
		Phe->p.argv[0]);
	    rc = RPMRC_OK;
	    goto exit;
	}
    }

    rpmlog(RPMLOG_DEBUG,
		D_("%s: %s(%s) %ssynchronous scriptlet start\n"),
		psm->stepName, tag2sln(psm->scriptTag), NVRA,
		(F_ISSET(psm, UNORDERED) ? "a" : ""));

    if (Phe->p.argv == NULL) {
	argv = (const char **) alloca(5 * sizeof(*argv));
	argv[0] = "/bin/sh";
	argc = 1;
	ldconfig_done = 0;
    } else {
	argv = (const char **) alloca((Phe->c + 4) * sizeof(*argv));
	memcpy(argv, Phe->p.argv, Phe->c * sizeof(*argv));
	argc = Phe->c;
	ldconfig_done = (ldconfig_path && !strcmp(argv[0], ldconfig_path)
		? 1 : 0);
    }

    /* XXX Load INSTPREFIXES lazily. This should be done elsewhere ... */
    if (IPhe->tag == 0) {
	IPhe->tag = RPMTAG_INSTPREFIXES;
	xx = headerGet(h, IPhe, 0);
	if (!xx) {
	    IPhe->p.ptr = _free(IPhe->p.ptr);
	    IPhe->tag = RPMTAG_INSTALLPREFIX;
	    xx = headerGet(h, IPhe, 0);
	    if (xx) {
		const char ** av = (const char **)
			xmalloc(sizeof(*av) + strlen(IPhe->p.argv[0]) + 1);
		char * t = (char *) &av[1];

		av[0] = t;
		t = stpcpy(t, IPhe->p.argv[0]);
		*t = '\0';
		IPhe->p.ptr = _free(IPhe->p.ptr);
		IPhe->t = RPM_STRING_ARRAY_TYPE;
		IPhe->p.argv = av;
		IPhe->c = 1;
	    } else {
		IPhe->p.argv = NULL;
		IPhe->c = 0;
	    }
	}
    }
    IP = IPhe->p.argv;
    nIP = IPhe->c;

    maxPrefixLength = 0;
    if (IP != NULL)
    for (i = 0; i < nIP; i++) {
	len = strlen(IP[i]);
	if (len > maxPrefixLength) maxPrefixLength = len;
    }
    prefixBuf = (char *) alloca(maxPrefixLength + 50);

    if (script) {
	const char * rootDir = rpmtsRootDir(ts);
	FD_t fd;
	size_t nw;

	if (rpmTempFile((!rpmtsChrootDone(ts) ? rootDir : "/"), &fn, &fd))
	    goto exit;

	if (rpmIsDebug() &&
	    (!strcmp(argv[0], "/bin/sh") || !strcmp(argv[0], "/bin/bash")))
	{
	    static const char set_x[] = "set -x\n";
	    nw = Fwrite(set_x, sizeof(set_x[0]), sizeof(set_x)-1, fd);
	}

	if (ldconfig_path && strstr(body, ldconfig_path) != NULL)
	    ldconfig_done = 1;

	nw = Fwrite(body, sizeof(body[0]), strlen(body), fd);
	xx = Fclose(fd);

	{   const char * sn = fn;
	    if (!rpmtsChrootDone(ts) && rootDir != NULL &&
		!(rootDir[0] == '/' && rootDir[1] == '\0'))
	    {
		sn += strlen(rootDir)-1;
	    }
	    argv[argc++] = sn;
	}

	if (arg1 >= 0) {
	    char *av = (char *) alloca(20);
	    sprintf(av, "%d", arg1);
	    argv[argc++] = av;
	}
	if (arg2 >= 0) {
	    char *av = (char *) alloca(20);
	    sprintf(av, "%d", arg2);
	    argv[argc++] = av;
	}
    }

    argv[argc] = NULL;

    /* Log the scriptlet to be exec'd. */
    switch (psm->scriptTag) {
    default:
	break;
    case RPMTAG_PREIN:
	(void) rpmlioPrein(rpmtsGetRdb(ts), argv, body);
	break;
    case RPMTAG_POSTIN:
	(void) rpmlioPostin(rpmtsGetRdb(ts), argv, body);
	break;
    case RPMTAG_PREUN:
	(void) rpmlioPreun(rpmtsGetRdb(ts), argv, body);
	break;
    case RPMTAG_POSTUN:
	(void) rpmlioPostun(rpmtsGetRdb(ts), argv, body);
	break;
    }

    scriptFd = rpmtsScriptFd(ts);
    if (scriptFd != NULL) {
	if (rpmIsVerbose()) {
	    out = fdDup(Fileno(scriptFd));
	} else {
	    out = Fopen("/dev/null", "w.fdio");
	    if (Ferror(out)) {
		out = fdDup(Fileno(scriptFd));
	    }
	}
    } else {
	out = fdDup(STDOUT_FILENO);
    }
    if (out == NULL)	/* XXX can't happen */
	goto exit;

    pid = rpmsqFork(&psm->sq);
    if (psm->sq.child == 0) {
	int pipes[2];
	int flag;
	int fdno;

	pipes[0] = pipes[1] = 0;
	/* Make stdin inaccessible */
	xx = pipe(pipes);
	xx = close(pipes[1]);
	xx = dup2(pipes[0], STDIN_FILENO);
	xx = close(pipes[0]);

	/* XXX Force FD_CLOEXEC on 1st 100 inherited fdno's. */
	for (fdno = 3; fdno < 100; fdno++) {
	    flag = fcntl(fdno, F_GETFD);
	    if (flag == -1 || (flag & FD_CLOEXEC))
		continue;
	    rpmlog(RPMLOG_DEBUG,
			D_("%s: %s(%s)\tfdno(%d) missing FD_CLOEXEC\n"),
			psm->stepName, sln, NVRA,
			fdno);
	    xx = fcntl(fdno, F_SETFD, FD_CLOEXEC);
	    /* XXX W2DO? debug msg for inheirited fdno w/o FD_CLOEXEC */
	}

	if (scriptFd != NULL) {
	    int sfdno = Fileno(scriptFd);
	    int ofdno = Fileno(out);
	    if (sfdno != STDERR_FILENO)
		xx = dup2(sfdno, STDERR_FILENO);
	    if (ofdno != STDOUT_FILENO)
		xx = dup2(ofdno, STDOUT_FILENO);
	    /* make sure we don't close stdin/stderr/stdout by mistake! */
	    if (ofdno > STDERR_FILENO && ofdno != sfdno)
		xx = Fclose (out);
	    if (sfdno > STDERR_FILENO && ofdno != sfdno)
		xx = Fclose (scriptFd);
	}

	{   const char *ipath = rpmExpand("PATH=%{_install_script_path}", NULL);
	    const char *path = SCRIPT_PATH;

	    if (ipath && ipath[5] != '%')
		path = ipath;

	    xx = doputenv(path);
	    /*@-modobserver@*/
	    ipath = _free(ipath);
	    /*@=modobserver@*/
	}

	if (IP != NULL)
	for (i = 0; i < nIP; i++) {
	    sprintf(prefixBuf, "RPM_INSTALL_PREFIX%d=%s", i, IP[i]);
	    xx = doputenv(prefixBuf);

	    /* backwards compatibility */
	    if (i == 0) {
		sprintf(prefixBuf, "RPM_INSTALL_PREFIX=%s", IP[i]);
		xx = doputenv(prefixBuf);
	    }
	}

	{   const char * rootDir = rpmtsRootDir(ts);
	    if (!rpmtsChrootDone(ts) && rootDir != NULL &&
		!(rootDir[0] == '/' && rootDir[1] == '\0'))
	    {
		/*@-modobserver@*/
		xx = Chroot(rootDir);
		/*@=modobserver@*/
	    }
	    xx = Chdir("/");
	    rpmlog(RPMLOG_DEBUG, D_("%s: %s(%s)\texecv(%s) pid %d\n"),
			psm->stepName, sln, NVRA,
			argv[0], (unsigned)getpid());

	    /* XXX Don't mtrace into children. */
	    unsetenv("MALLOC_CHECK_");

	    if (ssp != NULL)
		*ssp |= RPMSCRIPT_STATE_EXEC;

	    /* Permit libselinux to do the scriptlet exec. */
	    if (rpmtsSELinuxEnabled(ts) == 1) {	
		if (ssp != NULL)
		    *ssp |= RPMSCRIPT_STATE_SELINUX;
		xx = rpmsxExec(NULL, 0, argv);
	    } else {
/*@-nullstate@*/
		xx = execv(argv[0], (char *const *)argv);
/*@=nullstate@*/
	    }
	}

	if (ssp != NULL)
	    *ssp &= ~RPMSCRIPT_STATE_EXEC;

 	_exit(-1);
	/*@notreached@*/
    }

    if (psm->sq.child == (pid_t)-1) {
        rpmlog(RPMLOG_ERR, _("Couldn't fork %s: %s\n"), sln, strerror(errno));
        goto exit;
    }

    (void) psmWait(psm);

  /* XXX filter order dependent multilib "other" arch helper error. */
  if (!(psm->sq.reaped >= 0 && !strcmp(argv[0], "/usr/sbin/glibc_post_upgrade") && WEXITSTATUS(psm->sq.status) == 110)) {
    void *ptr = NULL;
    if (psm->sq.reaped < 0) {
	rpmlog(RPMLOG_ERR,
		_("%s(%s) scriptlet failed, waitpid(%d) rc %d: %s\n"),
		 sln, NVRA, (int)psm->sq.child, (int)psm->sq.reaped,
		strerror(errno));
	goto exit;
    } else
    if (!WIFEXITED(psm->sq.status) || WEXITSTATUS(psm->sq.status)) {
	if (WIFSIGNALED(psm->sq.status)) {
	    ptr = rpmtsNotify(ts, psm->te, RPMCALLBACK_SCRIPT_ERROR,
				 psm->scriptTag, WTERMSIG(psm->sq.status));
	    rpmlog(RPMLOG_ERR,
                 _("%s(%s) scriptlet failed, signal %d\n"),
                 sln, NVRA, WTERMSIG(psm->sq.status));
	} else {
	    ptr = rpmtsNotify(ts, psm->te, RPMCALLBACK_SCRIPT_ERROR,
				 psm->scriptTag, WEXITSTATUS(psm->sq.status));
	    rpmlog(RPMLOG_ERR,
		_("%s(%s) scriptlet failed, exit status %d\n"),
		sln, NVRA, WEXITSTATUS(psm->sq.status));
	}
	goto exit;
    }
  }

    rc = RPMRC_OK;

exit:
    if (op != NULL) {
	static unsigned int scale = 1000;
	(void) rpmswExit(op, 0);
        if (ix >= 0 && ix < RPMSCRIPT_MAX)
            psm->smetrics[ix] += op->usecs / scale;
    }

    if (out)
	xx = Fclose(out);	/* XXX dup'd STDOUT_FILENO */

    if (script) {
	if (!rpmIsDebug() && fn != NULL)
	    xx = Unlink(fn);
	fn = _free(fn);
    }

    body = _free(body);

    return rc;
}

/**
 * Retrieve and run scriptlet from header.
 * @param psm		package state machine data
 * @return		rpmRC return code
 */
static rpmRC runInstScript(rpmpsm psm)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies psm, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t Phe = (HE_t) memset(alloca(sizeof(*Phe)), 0, sizeof(*Phe));
    HE_t She = (HE_t) memset(alloca(sizeof(*She)), 0, sizeof(*She));
    rpmfi fi = psm->fi;
    const char * argv0 = NULL;
    rpmRC rc = RPMRC_OK;

assert(fi->h != NULL);
    She->tag = psm->scriptTag;
    if (!headerGet(fi->h, She, 0))
	goto exit;

    Phe->tag = psm->progTag;
    if (!headerGet(fi->h, Phe, 0))
	goto exit;

    /* Coerce strings into header argv return. */
    if (Phe->t == RPM_STRING_TYPE) {
	const char * s = Phe->p.str;
	char * t;
	Phe->p.argv = (const char **)
		xmalloc(sizeof(Phe->p.argv[0]) + strlen(s) + 1);
	Phe->p.argv[0] = t = (char *) &Phe->p.argv[1];
	t = stpcpy(t, s);
	*t = '\0';
	s = _free(s);
    }

    /* Expand "%script -p %%{interpreter}" macros. */
    if (Phe->p.argv[0][0] == '%')
	Phe->p.argv[0] = argv0 = rpmExpand(Phe->p.argv[0], NULL);

    rc = runScript(psm, fi->h, tag2sln(psm->scriptTag), Phe,
		She->p.str, psm->scriptArg, -1);

exit:
    argv0 = _free(argv0);
    Phe->p.ptr = _free(Phe->p.ptr);
    She->p.ptr = _free(She->p.ptr);
    return rc;
}

/*@unchecked@*/
static rpmTag _trigger_tag;

/**
 * Execute triggers.
 * @param psm		package state machine data
 * @param sourceH
 * @param triggeredH
 * @param arg2
 * @return		RPMRC_OK on success
 */
static rpmRC handleOneTrigger(const rpmpsm psm,
			Header sourceH, Header triggeredH, int arg2)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState@*/
	/*@modifies psm, sourceH, triggeredH,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    static int scareMem = 0;
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    HE_t Ihe = (HE_t) memset(alloca(sizeof(*Ihe)), 0, sizeof(*Ihe));
    HE_t She = (HE_t) memset(alloca(sizeof(*She)), 0, sizeof(*She));
    HE_t Phe = (HE_t) memset(alloca(sizeof(*Phe)), 0, sizeof(*Phe));
    miRE mire = NULL;
    const rpmts ts = psm->ts;
    rpmds Tds = NULL;
    rpmds Fds = NULL;
    rpmds Dds = NULL;
    rpmds Pds = NULL;
    const char * sourceName;
    const char * triggerName;
    rpmRC rc = RPMRC_OK;
    int arg1;
    int xx;
    int i;

    he->tag = RPMTAG_NAME;
    xx = headerGet(sourceH, he, 0);
    sourceName = he->p.str;

    he->tag = RPMTAG_NAME;
    xx = headerGet(triggeredH, he, 0);
    triggerName = he->p.str;

    arg1 = rpmdbCountPackages(rpmtsGetRdb(ts), triggerName);
    if (arg1 < 0) {
	/* XXX W2DO? fails as "execution of script failed" */
	rc = RPMRC_FAIL;
	goto exit;
    }
    arg1 += psm->countCorrection;

    Tds = rpmdsNew(triggeredH, RPMTAG_TRIGGERNAME, scareMem);
    if (Tds == NULL)
	goto exit;
    xx = rpmdsSetNoPromote(Tds, 1);

    Ihe->tag = RPMTAG_TRIGGERINDEX;
    if (!headerGet(triggeredH, Ihe, 0))
	goto exit;

    She->tag = RPMTAG_TRIGGERSCRIPTS;
    if (!headerGet(triggeredH, She, 0))
	goto exit;

    Phe->tag = RPMTAG_TRIGGERSCRIPTPROG;
    if (!headerGet(triggeredH, Phe, 0))
	goto exit;

    if ((Tds = rpmdsInit(Tds)) != NULL)
    while ((i = rpmdsNext(Tds)) >= 0) {
	rpmuint32_t Flags = rpmdsFlags(Tds);
	char * depName;
	int bingo;

	/* Skip triggers that are not in this context. */
	if (!(Flags & psm->sense))
	    continue;

	bingo = 0;		/* no trigger to fire. */
	depName = (char *) rpmdsN(Tds);
	if (depName[0] == '/') {
	    size_t nb = strlen(depName);
	    if (Glob_pattern_p(depName, 0)) {
		rpmds ds = NULL;
		if (depName[nb-1] == '/') {
		    /* XXX Dirnames w trailing "/" needed. */
		    if (Dds == NULL)
			Dds = rpmdsNew(sourceH, RPMTAG_DIRNAMES, 0x2);
		    ds = rpmdsLink(Dds, "Triggers");
		} else {
		    if (Fds == NULL)
			Fds = rpmdsNew(sourceH, RPMTAG_BASENAMES, 0);
		    ds = rpmdsLink(Fds, "Triggers");
		}
		if (mire == NULL)
		    mire = mireNew(RPMMIRE_GLOB, 0);

		xx = mireRegcomp(mire, depName);
		if ((ds = rpmdsInit(ds)) != NULL)
		while (rpmdsNext(ds) >= 0) {
		    const char * N = rpmdsN(ds);
		    xx = mireRegexec(mire, N, 0);
		    if (xx < 0)
			/*@innercontinue@*/ continue;
		    bingo = 1;
		    /*@innerbreak@*/ break;
		}
		(void)rpmdsFree(ds);
		ds = NULL;
		xx = mireClean(mire);
	    }

	    /* If not matched, and directory trigger, try dir names. */
	    if (!bingo && depName[nb-1] == '/') {
		/* XXX Dirnames w trailing "/" needed. */
		if (Dds == NULL)
		    Dds = rpmdsNew(sourceH, RPMTAG_DIRNAMES, 0x2);
		bingo = rpmdsMatch(Tds, Dds);
	    }

	    /* If not matched, try file paths. */
	    if (!bingo) {
		if (Fds == NULL)
		    Fds = rpmdsNew(sourceH, RPMTAG_BASENAMES, 0);
		bingo = rpmdsMatch(Tds, Fds);
	    }
	}

	/* If trigger not fired yet, try provided dependency match. */
	if (!bingo) {
	    if (Pds == NULL)
		Pds = rpmdsNew(sourceH, RPMTAG_PROVIDENAME, 0);
	    bingo = rpmdsMatch(Tds, Pds);
	    bingo = rpmdsNegateRC(Tds, bingo);
	}
	if (!bingo)
	    continue;

	/* Coerce strings into header argv return. */
	/* XXX FIXME: permit trigger scripts with arguments. */
	{   int index = Ihe->p.ui32p[i];
	    const char * s = Phe->p.argv[index];
	    char * t;
	    
	    he->tag = Phe->tag;
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->c = 1;
	    he->p.argv = (const char **)
		xmalloc(sizeof(Phe->p.argv[0]) + strlen(s) + 1);
	    he->p.argv[0] = t = (char *) &he->p.argv[1];
	    t = stpcpy(t, s);
	    *t = '\0';

	    if (runScript(psm, triggeredH, "%trigger", he,
			She->p.argv[index], arg1, arg2))
		rc = RPMRC_FAIL;

	    he->p.ptr = _free(he->p.ptr);
	}
    }

    mire = mireFree(mire);
    (void)rpmdsFree(Pds);
    Pds = NULL;
    (void)rpmdsFree(Dds);
    Dds = NULL;
    (void)rpmdsFree(Fds);
    Fds = NULL;
    (void)rpmdsFree(Tds);
    Tds = NULL;

exit:
    Ihe->p.ptr = _free(Ihe->p.ptr);
    She->p.ptr = _free(She->p.ptr);
    Phe->p.ptr = _free(Phe->p.ptr);
    triggerName = _free(triggerName);
    sourceName = _free(sourceName);

    return rc;
}

/* Retrieve trigger patterns from rpmdb. */
static int rpmdbTriggerGlobs(rpmpsm psm)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies psm, rpmGlobalMacroContext @*/
{
    const rpmts ts = psm->ts;
    ARGV_t keys = NULL;
    int xx = rpmdbMireApply(rpmtsGetRdb(ts), RPMTAG_TRIGGERNAME,
		RPMMIRE_STRCMP, NULL, &keys);
    int nkeys = argvCount(keys);
    int i;
    
    if (keys)
    for (i = 0; i < nkeys; i++) {
	char * t = (char *) keys[i];
	if (!Glob_pattern_p(t, 0))
	    continue;
	xx = mireAppend(RPMMIRE_GLOB, 0, t, NULL,
		(miRE *)&psm->Tmires, &psm->nTmires);
	xx = argvAdd(&psm->Tpats, t);
    }
    keys = argvFree(keys);
    return 0;
}

/**
 * Run a dependency set loop against rpmdb triggers.
 * @param psm		package state machine data
 * @param tagno		dependency set to run against rpmdb
 * @param arg2		scriptlet arg2
 * @return		RPMRC_OK on success
 */
static rpmRC runTriggersLoop(rpmpsm psm, rpmTag tagno, int arg2)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies psm, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    static int scareMem = 0;
    const rpmts ts = psm->ts;
    rpmfi fi = psm->fi;
    rpmds ds = rpmdsNew(fi->h, tagno, scareMem);
    char * depName = NULL;
    ARGI_t instances = NULL;
    rpmmi mi;
    Header triggeredH;
    rpmRC rc = RPMRC_OK;
    int i;
    int xx;

    /* Fire elements against rpmdb trigger strings. */
    if ((ds = rpmdsInit(ds)) != NULL)
    while ((i = rpmdsNext(ds)) >= 0) {
	const char * Name = rpmdsN(ds);
	size_t nName = strlen(Name);
	unsigned prev, instance;
	unsigned nvals;
	ARGint_t vals;

	depName = _free(depName);
	depName = (char *) xmalloc(nName + 1 + 1);
	(void) stpcpy(depName, Name);
	/* XXX re-add the pesky trailing '/' to dirnames. */
	depName[nName] = (tagno == RPMTAG_DIRNAMES ? '/' : '\0');
	depName[nName+1] = '\0';

	if (depName[0] == '/' && psm->Tmires != NULL) {
	    miRE mire;
	    int j;

	    /* XXX mireApply doesn't tell which pattern matched. */
	    for (j = 0, mire = (miRE)psm->Tmires; j < psm->nTmires; j++, mire++) {
		const char * pattern = psm->Tpats[j];
		if (depName[nName-1] != '/') {
		    size_t npattern = strlen(pattern);
		    depName[nName] = (pattern[npattern-1] == '/') ? '/' : '\0';
		}
		if (mireRegexec(mire, depName, 0) < 0)
		    /*@innercontinue@*/ continue;

		/* Reset the primary retrieval key to the pattern. */
		depName = _free(depName);
		depName = xstrdup(pattern);
		/*@innerbreak@*/ break;
	    }
	}

	/* Retrieve triggered header(s) by key. */
	mi = rpmtsInitIterator(ts, RPMTAG_TRIGGERNAME, depName, 0);

	nvals = argiCount(instances);
	vals = argiData(instances);
	if (nvals > 0)
	    xx = rpmmiPrune(mi, (uint32_t *)vals, nvals, 1);

	prev = 0;
	while((triggeredH = rpmmiNext(mi)) != NULL) {
	    instance = rpmmiInstance(mi);
	    if (prev == instance)
		/*@innercontinue@*/ continue;
	    if (handleOneTrigger(psm, fi->h, triggeredH, arg2))
		rc = RPMRC_FAIL;
	    prev = instance;
	    xx = argiAdd(&instances, -1, instance);
	    xx = argiSort(instances, NULL);
	}

	mi = rpmmiFree(mi);
    }

    instances = argiFree(instances);
    depName = _free(depName);
    (void)rpmdsFree(ds);
    ds = NULL;

    return rc;
}

/**
 * Run trigger scripts in the database that are fired by this header.
 * @param psm		package state machine data
 * @return		0 on success
 */
static rpmRC runTriggers(rpmpsm psm)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies psm, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    const rpmts ts = psm->ts;
    rpmfi fi = psm->fi;
    int numPackage;
    rpmTag tagno;
    rpmRC rc = RPMRC_OK;

    /* Select RPMTAG_NAME or RPMTAG_PROVIDENAME index for triggering. */
    if (_trigger_tag == 0) {
	const char * t = rpmExpand("%{?_trigger_tag}", NULL);
/*@-mods@*/
	_trigger_tag = (!strcmp(t, "name") ? RPMTAG_NAME : RPMTAG_PROVIDENAME);
/*@=mods@*/
	t = _free(t);
    }
    tagno = _trigger_tag;

assert(psm->te != NULL);
    {	const char * N = rpmteN(psm->te);
assert(N != NULL);
	numPackage = rpmdbCountPackages(rpmtsGetRdb(ts), N);
	numPackage += psm->countCorrection;
	if (numPackage < 0)
	    return RPMRC_NOTFOUND;
    }
assert(fi != NULL);
assert(fi->h != NULL);

    /* XXX Save/restore count correction. */
    {	int countCorrection = psm->countCorrection;

	psm->countCorrection = 0;

	/* Try name/providename triggers first. */
	if (runTriggersLoop(psm, tagno, numPackage))
	    rc = RPMRC_FAIL;

	/* If not limited to NEVRA triggers, also try file/dir path triggers. */
	if (tagno != RPMTAG_NAME) {
	    int xx;
	    /* Retrieve trigger patterns from rpmdb. */
	    xx = rpmdbTriggerGlobs(psm);

	    if (runTriggersLoop(psm, RPMTAG_BASENAMES, numPackage))
		rc = RPMRC_FAIL;
	    if (runTriggersLoop(psm, RPMTAG_DIRNAMES, numPackage))
		rc = RPMRC_FAIL;

	    psm->Tpats = argvFree(psm->Tpats);
	    psm->Tmires = mireFreeAll((miRE)psm->Tmires, psm->nTmires);
	    psm->nTmires = 0;
	}

	psm->countCorrection = countCorrection;
    }

    return rc;
}

/**
 * Run triggers from this header that are fired by headers in the database.
 * @param psm		package state machine data
 * @return		0 on success
 */
static rpmRC runImmedTriggers(rpmpsm psm)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies psm, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    HE_t Ihe = (HE_t) memset(alloca(sizeof(*Ihe)), 0, sizeof(*Ihe));
    const rpmts ts = psm->ts;
    rpmfi fi = psm->fi;
    rpmds triggers = NULL;
    rpmmi mi;
    ARGV_t keys = NULL;
    ARGI_t instances = NULL;
    Header sourceH = NULL;
    const char * Name;
    rpmTag tagno;
    rpmRC rc = RPMRC_OK;
    int i;
    int xx;

assert(fi->h != NULL);

    /* Select RPMTAG_NAME or RPMTAG_PROVIDENAME index for triggering. */
    if (_trigger_tag == 0) {
	const char * t = rpmExpand("%{?_trigger_tag}", NULL);
/*@-mods@*/
	_trigger_tag = (!strcmp(t, "name") ? RPMTAG_NAME : RPMTAG_PROVIDENAME);
/*@=mods@*/
	t = _free(t);
    }
    tagno = _trigger_tag;

/*@-castexpose@*/
    triggers = rpmdsLink(psm->triggers, "ImmedTriggers");
/*@=castexpose@*/
    if (triggers == NULL)
	goto exit;

    Ihe->tag = RPMTAG_TRIGGERINDEX;
    xx = headerGet(fi->h, Ihe, 0);
    if (!(xx && Ihe->p.ui32p && Ihe->c)) goto exit;

    /* Collect primary trigger keys, expanding globs as needed. */
    triggers = rpmdsInit(triggers);
    if (triggers != NULL)
    while ((i = rpmdsNext(triggers)) >= 0) {
	evrFlags Flags = rpmdsFlags(triggers);
	const char * N = rpmdsN(triggers);
	const char * EVR = rpmdsEVR(triggers);

	/* Skip triggers that are not in this context. */
	if (!(Flags & psm->sense))
	    continue;

	/* If not limited to NEVRA triggers, use file/dir index. */
	if (tagno != RPMTAG_NAME) {
	    /* XXX if trigger name ends with '/', use dirnames instead. */
	    if (N[0] == '/') 
		tagno = (N[strlen(N)-1] == '/')
			? RPMTAG_DIRNAMES : RPMTAG_FILEPATHS;
	}
	/* XXX For now, permit globs only in unversioned triggers. */
	if ((EVR == NULL || *EVR == '\0') && Glob_pattern_p(N, 0))
	    xx = rpmdbMireApply(rpmtsGetRdb(ts), tagno, RPMMIRE_GLOB, N, &keys);
	else
	    xx = argvAdd(&keys, N);
    }
    (void)rpmdsFree(triggers);
    triggers = NULL;

    /* For all primary keys, retrieve headers and fire triggers. */
    if (keys != NULL)
    for (i = 0; (Name = keys[i]) != NULL; i++) {
	unsigned prev, instance;
	unsigned nvals;
	ARGint_t vals;

	/* If not limited to NEVRA triggers, use file/dir index. */
	if (tagno != RPMTAG_NAME) {
	    /* XXX if trigger name ends with '/', use dirnames instead. */
	    if (Name[0] == '/') 
		tagno = (Name[strlen(Name)-1] == '/')
			? RPMTAG_DIRNAMES : RPMTAG_FILEPATHS;
	}

	mi = rpmtsInitIterator(ts, tagno, Name, 0);

	/* Don't retrieve headers that have already been processed. */
	nvals = argiCount(instances);
	vals = argiData(instances);
	if (nvals > 0)
	    xx = rpmmiPrune(mi, (uint32_t *)vals, nvals, 1);

	prev = 0;
	while((sourceH = rpmmiNext(mi)) != NULL) {

	    /* Skip headers that have already been processed. */
	    instance = rpmmiInstance(mi);
	    if (prev == instance)
		/*@innercontinue@*/ continue;

	    if (handleOneTrigger(psm, sourceH, fi->h, rpmmiCount(mi)))
		rc = RPMRC_FAIL;

	    /* Mark header instance as processed. */
	    prev = instance;
	    xx = argiAdd(&instances, -1, instance);
	    xx = argiSort(instances, NULL);
	}

	mi = rpmmiFree(mi);
    }

exit:
    instances = argiFree(instances);
    keys = argvFree(keys);
    Ihe->p.ptr = _free(Ihe->p.ptr);
    return rc;
}

/*@observer@*/
static const char * pkgStageString(pkgStage a)
	/*@*/
{
    switch(a) {
    case PSM_UNKNOWN:		return "unknown";

    case PSM_PKGINSTALL:	return "  install";
    case PSM_PKGERASE:		return "    erase";
    case PSM_PKGCOMMIT:		return "   commit";
    case PSM_PKGSAVE:		return "repackage";

    case PSM_INIT:		return "init";
    case PSM_PRE:		return "pre";
    case PSM_PROCESS:		return "process";
    case PSM_POST:		return "post";
    case PSM_UNDO:		return "undo";
    case PSM_FINI:		return "fini";

    case PSM_CREATE:		return "create";
    case PSM_NOTIFY:		return "notify";
    case PSM_DESTROY:		return "destroy";
    case PSM_COMMIT:		return "commit";

    case PSM_CHROOT_IN:		return "chrootin";
    case PSM_CHROOT_OUT:	return "chrootout";
    case PSM_SCRIPT:		return "script";
    case PSM_TRIGGERS:		return "triggers";
    case PSM_IMMED_TRIGGERS:	return "immedtriggers";

    case PSM_RPMIO_FLAGS:	return "rpmioflags";

    case PSM_RPMDB_LOAD:	return "rpmdbload";
    case PSM_RPMDB_ADD:		return "rpmdbadd";
    case PSM_RPMDB_REMOVE:	return "rpmdbremove";

    default:			return "???";
    }
    /*@noteached@*/
}

void rpmpsmSetAsync(rpmpsm psm, int async)
{
    assert(psm != NULL);
#ifdef	REFERENCE
    psm->unorderedSuccessor = async;
#else
    if (async) {
	F_SET(psm, UNORDERED);
    } else {
	F_CLR(psm, UNORDERED);
    }
#endif
}

rpmRC rpmpsmScriptStage(rpmpsm psm, rpmTag scriptTag, rpmTag progTag)
{
assert(psm != NULL);
    psm->scriptTag = scriptTag;
    psm->progTag = progTag;
    /* XXX other tags needed? */
    switch (scriptTag) {
    default:	break;
    case RPMTAG_SANITYCHECK:	psm->stepName = "sanitycheck";	break;
    case RPMTAG_VERIFYSCRIPT:	psm->stepName = "verify";	break;
    case RPMTAG_PRETRANS:	psm->stepName = "pretrans";	break;
    case RPMTAG_POSTTRANS:	psm->stepName = "posttrans";	break;
    }
    return rpmpsmStage(psm, PSM_SCRIPT);
}

/*@-mustmod@*/
static void rpmpsmFini(void * _psm)
	/*@modifies _psm @*/
{
    rpmpsm psm = (rpmpsm) _psm;

/*@-nullstate@*/
    psm->fi = rpmfiFree(psm->fi);
#ifdef	NOTYET
    psm->te = rpmteFree(psm->te);
#else
    psm->te = NULL;
#endif
/*@-internalglobs@*/
    (void)rpmtsFree(psm->ts); 
    psm->ts = NULL;
/*@=internalglobs@*/

    psm->IPhe->p.ptr = _free(psm->IPhe->p.ptr);
    psm->IPhe = _free(psm->IPhe);
    psm->NVRA = _free(psm->NVRA);
    (void)rpmdsFree(psm->triggers);
    psm->triggers = NULL;
/*@=nullstate@*/
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _psmPool;

static rpmpsm rpmpsmGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _psmPool, fileSystem, internalState @*/
	/*@modifies pool, _psmPool, fileSystem, internalState @*/
{
    rpmpsm psm;

    if (_psmPool == NULL) {
	_psmPool = rpmioNewPool("psm", sizeof(*psm), -1, _psm_debug,
			NULL, NULL, rpmpsmFini);
	pool = _psmPool;
    }
    psm = (rpmpsm) rpmioGetPool(pool, sizeof(*psm));
    memset(((char *)psm)+sizeof(psm->_item), 0, sizeof(*psm)-sizeof(psm->_item));
    return psm;
}

rpmpsm rpmpsmNew(rpmts ts, rpmte te, rpmfi fi)
{
    static const char msg[] = "rpmpsmNew";
    rpmpsm psm = rpmpsmGetPool(_psmPool);

/*@-assignexpose -castexpose @*/
    if (ts)	psm->ts = rpmtsLink(ts, msg);
#ifdef	NOTYET
    if (te)	psm->te = rpmteLink(te, msg);
#else
/*@-temptrans @*/
    if (te)	psm->te = te;
/*@=temptrans @*/
#endif
    if (fi)	psm->fi = rpmfiLink(fi, msg);
/*@=assignexpose =castexpose @*/

    psm->triggers = NULL;
    psm->NVRA = NULL;
    psm->IPhe = (HE_t) xcalloc(1, sizeof(*psm->IPhe));
    memset(psm->sstates, 0, sizeof(psm->sstates));
    memset(psm->smetrics, 0, sizeof(psm->smetrics));

    return rpmpsmLink(psm, msg);
}

/**
 * Load a transaction id from a header.
 * @param h		header
 * @param tag		tag to load
 * @return		tag value (0 on failure)
 */
static rpmuint32_t hLoadTID(Header h, rpmTag tag)
	/*@globals internalState @*/
	/*@modifies internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmuint32_t val;
    int xx;

    he->tag = tag;
    xx = headerGet(h, he, 0);
    val = (xx && he->p.ui32p ? he->p.ui32p[0] : 0);
    he->p.ptr = _free(he->p.ptr);
    return val;
}

/**
 * Copy a tag from a source to a target header.
 * @param sh		source header
 * @param th		target header
 * @param tag		tag to copy
 * @return		0 always
 */
static int hCopyTag(Header sh, Header th, rpmTag tag)
	/*@globals internalState @*/
	/*@modifies th, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int xx = 1;

    he->tag = tag;
    if (headerGet(sh, he, 0) && he->c > 0)
	xx = headerPut(th, he, 0);
    he->p.ptr = _free(he->p.ptr);
    return 0;
}

/**
 * Save backward link(s) of an upgrade chain into a header.
 * @param h		header
 * @param *blink	backward links
 * @return		0 always
 */
static int hSaveBlinks(Header h, const struct rpmChainLink_s * blink)
	/*@modifies h @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
/*@observer@*/
    static const char * chain_end = RPMTE_CHAIN_END;
    int ac;
    int xx = 1;

    /* Save forward links into header upgrade chain. */
    he->tag = RPMTAG_BLINKNEVRA;
    he->t = RPM_STRING_ARRAY_TYPE;
    ac = argvCount(blink->NEVRA);
    if (ac > 0) {
	he->p.argv = argvData(blink->NEVRA);
	he->c = ac;
    } else {    /* XXX Add an explicit chain terminator on 1st install. */
	he->p.argv = &chain_end;
	he->c = 1;
    }
    xx = headerPut(h, he, 0);
    
    he->tag = RPMTAG_BLINKPKGID;
    he->t = RPM_STRING_ARRAY_TYPE;
    ac = argvCount(blink->Pkgid);
    if (ac > 0) {
	he->p.argv = argvData(blink->Pkgid);
	he->c = ac;
    } else {	/* XXX Add an explicit chain terminator on 1st install. */
	he->p.argv = &chain_end;
	he->c = 1;
    }
    xx = headerPut(h, he, 0);

    he->tag = RPMTAG_BLINKHDRID;
    he->t = RPM_STRING_ARRAY_TYPE;
    ac = argvCount(blink->Hdrid);
    if (ac > 0) {
	he->p.argv = argvData(blink->Hdrid);
	he->c = ac;
    } else {	/* XXX Add an explicit chain terminator on 1st install. */
	he->p.argv = &chain_end;
	he->c = 1;
    }
    xx = headerPut(h, he, 0);

    return 0;
}

/**
 * Save forward link(s) of an upgrade chain into a header.
 * @param h		header
 * @param *flink	forward links
 * @return		0 always
 */
static int hSaveFlinks(Header h, const struct rpmChainLink_s * flink)
	/*@modifies h @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
#ifdef	NOTYET
    /*@observer@*/
    static const char * chain_end = RPMTE_CHAIN_END;
#endif
    int ac;
    int xx = 1;

    /* Save forward links into header upgrade chain. */
    he->tag = RPMTAG_FLINKNEVRA;
    he->t = RPM_STRING_ARRAY_TYPE;
    ac = argvCount(flink->NEVRA);
    if (ac > 0) {
	he->p.argv = argvData(flink->NEVRA);
	he->c = ac;
    }
#ifdef	NOTYET	/* XXX is an explicit flink terminator needed? */
    else {	/* XXX Add an explicit chain terminator on 1st install. */
	he->p.argv = &chain_end;
	he->c = 1;
    }
#endif
    xx = headerPut(h, he, 0);

    he->tag = RPMTAG_FLINKPKGID;
    he->t = RPM_STRING_ARRAY_TYPE;
    ac = argvCount(flink->Pkgid);
    if (ac > 0) {
	he->p.argv = argvData(flink->Pkgid);
	he->c = ac;
    }
#ifdef	NOTYET	/* XXX is an explicit flink terminator needed? */
    else {	/* XXX Add an explicit chain terminator on 1st install. */
	he->p.argv = &chain_end;
	he->c = 1;
    }
#endif
    xx = headerPut(h, he, 0);

    he->tag = RPMTAG_FLINKHDRID;
    he->t = RPM_STRING_ARRAY_TYPE;
    ac = argvCount(flink->Hdrid);
    if (ac > 0) {
	he->p.argv = argvData(flink->Hdrid);
	he->c = ac;
    }
#ifdef	NOTYET	/* XXX is an explicit flink terminator needed? */
    else {	/* XXX Add an explicit chain terminator on 1st install. */
	he->p.argv = &chain_end;
	he->c = 1;
    }
#endif
    xx = headerPut(h, he, 0);

    return 0;
}

/**
 * Add per-transaction data to an install header.
 * @param ts		transaction set
 * @param te		transaction element
 * @param fi		file info set
 * @return		0 always
 */
static int populateInstallHeader(const rpmts ts, const rpmte te, rpmfi fi)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fi, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmuint32_t tscolor = rpmtsColor(ts);
    rpmuint32_t tecolor = rpmteColor(te);
    rpmuint32_t * uip;
    rpmuint32_t installTime[2];
    rpmuint32_t originTime[2];
    rpmuint32_t originTid[2];
    int xx = 1;

assert(fi->h != NULL);

    {	struct timeval tv;
	xx = gettimeofday(&tv, NULL);
	installTime[0] = (rpmuint32_t) tv.tv_sec;
	installTime[1] = (rpmuint32_t) tv.tv_usec;
    }
    he->tag = RPMTAG_INSTALLTIME;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &installTime[0];
    he->c = 2;
    xx = headerPut(fi->h, he, 0);

    /* Propagate the tid & time that the package was first installed. */
    if ((uip = rpmteOriginTime(te)) != NULL)
	memcpy(originTime, uip, sizeof(originTime));
    if (originTime[0] == 0)
	memcpy(originTime, installTime, sizeof(originTime));
    he->tag = RPMTAG_ORIGINTIME;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = originTime;
    he->c = 2;
    xx = headerPut(fi->h, he, 0);

    if ((uip = rpmteOriginTid(te)) != NULL)
	memcpy(originTid, uip, sizeof(originTid));
    if (originTid[0] == 0)
	memcpy(originTid, ts->tid, sizeof(originTid));
    he->tag = RPMTAG_ORIGINTID;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = originTid;
    he->c = 2;
    xx = headerPut(fi->h, he, 0);

    he->tag = RPMTAG_INSTALLCOLOR;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &tscolor;
    he->c = 1;
    xx = headerPut(fi->h, he, 0);

    /* XXX FIXME: add preferred color at install. */

    he->tag = RPMTAG_PACKAGECOLOR;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &tecolor;
    he->c = 1;
    xx = headerPut(fi->h, he, 0);

    /* Add the header's origin/digest/stat (i.e. URL) */
    {	const char * fn = headerGetOrigin(fi->h);
	const char * digest = headerGetDigest(fi->h);
	struct stat * st = headerGetStatbuf(fi->h);

	if (fn != NULL) {
	    he->tag = RPMTAG_PACKAGEORIGIN;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = xstrdup(fn);
	    he->c = 1;
	    xx = headerPut(fi->h, he, 0);
	    he->p.ptr = _free(he->p.ptr);

	    if (digest != NULL) {
		he->tag = RPMTAG_PACKAGEDIGEST;
		he->t = RPM_STRING_TYPE;
		he->p.str = headerGetDigest(fi->h);
		he->c = 1;
		xx = headerPut(fi->h, he, 0);
	    }
	    if (st != NULL) {
/* XXX Fstat(2) in pkgio.c should set *st. Verify st->st_mode w assert(3). */
#ifndef	DYING
		int ut = urlPath(fn, NULL);
		/* XXX URI is active, so avoid the lazy Stat(2) for now. */
		if (!(ut == URL_IS_HTTP || ut == URL_IS_HTTPS))
		if (st->st_mode == 0 && st->st_mtime == 0 && st->st_size == 0)
		    xx = Stat(fn, st);
#endif
		if (st->st_mode != 0) {
		    he->tag = RPMTAG_PACKAGESTAT;
		    he->t = RPM_BIN_TYPE;
		    he->p.ptr = (void *)st;
		    he->c = (rpmTagCount) sizeof(*st);
		    xx = headerPut(fi->h, he, 0);
		}
	    }
	}
    }

    /* XXX Don't clobber forward/backward upgrade chain on rollbacks */
    if (rpmtsType(ts) != RPMTRANS_TYPE_ROLLBACK)
	xx = hSaveBlinks(fi->h, &te->blink);

    return 0;
}

/**
 * Add fi->states to an install header.
 * @param ts		transaction set
 * @param psm		package state machine
 * @param fi		file info set
 * @return		0 always
 */
static int postPopulateInstallHeader(/*@unused@*/ const rpmts ts,
		const rpmpsm psm, rpmfi fi)
	/*@modifies psm, fi @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int fc = rpmfiFC(fi);
    int xx = 1;

    /* Add the (install) scriptlet status/metrics. */
    he->tag = RPMTAG_SCRIPTSTATES;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = psm->sstates;
    he->c = RPMSCRIPT_MAX;
    xx = headerPut(fi->h, he, 0);
    he->tag = RPMTAG_SCRIPTMETRICS;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = psm->smetrics;
    he->c = RPMSCRIPT_MAX;
    xx = headerPut(fi->h, he, 0);

    /* Add file states to install header. */
    if (fi->fstates != NULL && fc > 0) {
	he->tag = RPMTAG_FILESTATES;
	he->t = RPM_UINT8_TYPE;
	he->p.ui8p = fi->fstates;
	he->c = fc;
	xx = headerPut(fi->h, he, 0);
    }

    return 0;
}

#if defined(WITH_PTHREADS)
static void * rpmpsmThread(void * _psm)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies arg, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmpsm psm = (rpmpsm) _psm;
/*@-unqualifiedtrans@*/
    return ((void *) rpmpsmStage(psm, psm->nstage));
/*@=unqualifiedtrans@*/
}
#endif

static int rpmpsmNext(rpmpsm psm, pkgStage nstage)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies psm, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    psm->nstage = nstage;
#if defined(WITH_PTHREADS)
    if (_psm_threads)
	return rpmsqJoin( rpmsqThread(rpmpsmThread, psm) );
#endif
    return rpmpsmStage(psm, psm->nstage);
}

/**
 * @todo Packages w/o files never get a callback, hence don't get displayed
 * on install with -v.
 */
/*@-nullpass@*/ /* FIX: testing null annotation for fi->h */
rpmRC rpmpsmStage(rpmpsm psm, pkgStage stage)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const rpmts ts = psm->ts;
    rpmuint32_t tscolor = rpmtsColor(ts);
    rpmfi fi = psm->fi;
    rpmRC rc = psm->rc;
    int saveerrno;
    int xx;

/* XXX hackery to assert(!scareMem) in rpmfiNew. */
/*@-castexpose@*/
if (fi->h == NULL && fi->te && ((rpmte)fi->te)->h != NULL) fi->h = headerLink(((rpmte)fi->te)->h);
/*@=castexpose@*/

    switch (stage) {
    case PSM_UNKNOWN:
	break;
    case PSM_INIT:
	rpmlog(RPMLOG_DEBUG, D_("%s: %s has %d files, test = %d\n"),
		psm->stepName, rpmteNEVR(psm->te),
		rpmfiFC(fi), (rpmtsFlags(ts) & RPMTRANS_FLAG_TEST));

	/*
	 * When we run scripts, we pass an argument which is the number of
	 * versions of this package that will be installed when we are
	 * finished.
	 */
	psm->npkgs_installed = rpmdbCountPackages(rpmtsGetRdb(ts), rpmteN(psm->te));
	if (psm->npkgs_installed < 0) {
	    rc = RPMRC_FAIL;
	    break;
	}

	/* Adjust package count on rollback downgrade. */
assert(psm->te != NULL);
	if (rpmtsType(ts) == RPMTRANS_TYPE_AUTOROLLBACK &&
	    (psm->goal & ~(PSM_PKGSAVE|PSM_PKGERASE)))
	{
	    if (psm->te->downgrade)
		psm->npkgs_installed--;
	}

	if (psm->goal == PSM_PKGINSTALL) {
	    int fc = rpmfiFC(fi);
	    const char * hdrid;

	    /* Add per-transaction data to install header. */
	    xx = populateInstallHeader(ts, psm->te, fi);

	    psm->scriptArg = psm->npkgs_installed + 1;

assert(psm->mi == NULL);
	    hdrid = rpmteHdrid(psm->te);
	    if (hdrid != NULL) {
		/* XXX should use RPMTAG_HDRID not RPMTAG_SHA1HEADER */
		psm->mi = rpmtsInitIterator(ts, RPMTAG_SHA1HEADER, hdrid, 0);
	    } else {
		psm->mi = rpmtsInitIterator(ts, RPMTAG_NAME, rpmteN(psm->te),0);
		xx = rpmmiAddPattern(psm->mi, RPMTAG_EPOCH, RPMMIRE_STRCMP,
			rpmteE(psm->te));
		xx = rpmmiAddPattern(psm->mi, RPMTAG_VERSION, RPMMIRE_STRCMP,
			rpmteV(psm->te));
		xx = rpmmiAddPattern(psm->mi, RPMTAG_RELEASE, RPMMIRE_STRCMP,
			rpmteR(psm->te));
#ifdef	RPM_VENDOR_MANDRIVA
		xx = rpmmiAddPattern(psm->mi, RPMTAG_DISTEPOCH, RPMMIRE_STRCMP,
			rpmteD(psm->te));
#endif
		if (tscolor) {
		    xx = rpmmiAddPattern(psm->mi,RPMTAG_ARCH, RPMMIRE_STRCMP,
			rpmteA(psm->te));
		    xx = rpmmiAddPattern(psm->mi, RPMTAG_OS, RPMMIRE_STRCMP,
			rpmteO(psm->te));
		}
	    }

	    while ((psm->oh = rpmmiNext(psm->mi)) != NULL) {
		fi->record = rpmmiInstance(psm->mi);
		psm->oh = NULL;
		/*@loopbreak@*/ break;
	    }
	    psm->mi = rpmmiFree(psm->mi);

	    rc = RPMRC_OK;

	    /* XXX lazy alloc here may need to be done elsewhere. */
	    if (fi->fstates == NULL && fc > 0) {
		fi->fstates = (rpmuint8_t *) xmalloc(sizeof(*fi->fstates) * fc);
		memset(fi->fstates, RPMFILE_STATE_NORMAL, fc);
	    }

	    xx = rpmtxnBegin(rpmtsGetRdb(ts), ts->txn, &psm->te->txn);

	    if (rpmtsFlags(ts) & RPMTRANS_FLAG_JUSTDB)	break;
	    if (fc <= 0)				break;
	
	    /*
	     * Old format relocatable packages need the entire default
	     * prefix stripped to form the cpio list, while all other packages
	     * need the leading / stripped.
	     */
	    he->tag = RPMTAG_DEFAULTPREFIX;
	    xx = headerGet(fi->h, he, 0);
	    fi->striplen = (xx && he->p.str ? strlen(he->p.str) + 1 : 1);
	    he->p.ptr = _free(he->p.ptr);
	    fi->mapflags =
		IOSM_MAP_PATH | IOSM_MAP_MODE | IOSM_MAP_UID | IOSM_MAP_GID | (fi->mapflags & IOSM_SBIT_CHECK);
	
	    if (headerIsEntry(fi->h, RPMTAG_ORIGBASENAMES))
		he->tag = RPMTAG_ORIGPATHS;
	    else
		he->tag = RPMTAG_FILEPATHS;
	    xx = headerGet(fi->h, he, 0);
assert(he->p.argv != NULL);
	    fi->apath = he->p.argv;
	
	    if (fi->fuser == NULL) {
		he->tag = RPMTAG_FILEUSERNAME;
		xx = headerGet(fi->h, he, 0);
		fi->fuser = he->p.argv;
	    }
	    if (fi->fgroup == NULL) {
		he->tag = RPMTAG_FILEGROUPNAME;
		xx = headerGet(fi->h, he, 0);
		fi->fgroup = he->p.argv;
	    }
	    rc = RPMRC_OK;
	}
	if (psm->goal == PSM_PKGERASE || psm->goal == PSM_PKGSAVE) {
	    psm->scriptArg = psm->npkgs_installed - 1;
	
	    /* XXX FIXME: PSM_PKGSAVE needs to be transactionally protected. */
	    if (psm->goal == PSM_PKGERASE)
		xx = rpmtxnBegin(rpmtsGetRdb(ts), ts->txn, &psm->te->txn);

	    /* Retrieve installed header. */
	    rc = (rpmRC) rpmpsmNext(psm, PSM_RPMDB_LOAD);
	    if (rc == RPMRC_OK && psm->te)
		(void) rpmteSetHeader(psm->te, fi->h);
	}
	if (psm->goal == PSM_PKGSAVE) {
	    /* Open output package for writing. */
	    {	char tiddn[32];
		const char * bfmt;
		const char * pkgdn;
		const char * pkgbn;
		char * pkgdn_buf;

		xx = snprintf(tiddn, sizeof(tiddn), "%d", rpmtsGetTid(ts));
		bfmt = rpmGetPath(tiddn, "/", "%{_repackage_name_fmt}", NULL);
		pkgbn = headerSprintf(fi->h, bfmt,
					NULL, rpmHeaderFormats, NULL);
		bfmt = _free(bfmt);
		psm->pkgURL = rpmGenPath("%{?_repackage_root}",
					 "%{?_repackage_dir}",
					pkgbn);
		pkgbn = _free(pkgbn);
		(void) urlPath(psm->pkgURL, &psm->pkgfn);
		pkgdn_buf = xstrdup(psm->pkgfn);
/*@-moduncon@*/
		pkgdn = dirname(pkgdn_buf);
/*@=moduncon@*/
		rc = rpmMkdirPath(pkgdn, "_repackage_dir");
		pkgdn_buf = _free(pkgdn_buf);
		if (rc == RPMRC_FAIL)
		    break;
		psm->fd = Fopen(psm->pkgfn, "w.fdio");
		if (psm->fd == NULL || Ferror(psm->fd)) {
		    rc = RPMRC_FAIL;
		    break;
		}
	    }
	}
	break;
    case PSM_PRE:
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_TEST)	break;

/* XXX insure that trigger index is opened before entering chroot. */
#ifdef	NOTYET
 { static int oneshot = 0;
   dbiIndex dbi;
   if (!oneshot) {
     dbi = dbiOpen(rpmtsGetRdb(ts), RPMTAG_TRIGGERNAME, 0);
     oneshot++;
   }
 }
#endif

	/* Change root directory if requested and not already done. */
	rc = (rpmRC) rpmpsmNext(psm, PSM_CHROOT_IN);

	if (psm->goal == PSM_PKGINSTALL) {
	    psm->scriptTag = RPMTAG_PREIN;
	    psm->progTag = RPMTAG_PREINPROG;
	    psm->sense = RPMSENSE_TRIGGERPREIN;
	    psm->countCorrection = 0;	/* XXX is this correct?!? */

	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_NOTRIGGERPREIN)) {

		/* Run triggers in other package(s) this package sets off. */
		rc = (rpmRC) rpmpsmNext(psm, PSM_TRIGGERS);
		if (rc) break;

		/* Run triggers in this package other package(s) set off. */
		rc = (rpmRC) rpmpsmNext(psm, PSM_IMMED_TRIGGERS);
		if (rc) break;
	    }

	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_NOPRE)) {
		rc = (rpmRC) rpmpsmNext(psm, PSM_SCRIPT);
		if (rc != RPMRC_OK) {
		    rpmlog(RPMLOG_ERR,
			_("%s: %s scriptlet failed (%d), skipping %s\n"),
			psm->stepName, tag2sln(psm->scriptTag), rc,
			rpmteNEVR(psm->te));
		    break;
		}
	    }
	}

	if (psm->goal == PSM_PKGERASE) {
	    psm->scriptTag = RPMTAG_PREUN;
	    psm->progTag = RPMTAG_PREUNPROG;
	    psm->sense = RPMSENSE_TRIGGERUN;
	    psm->countCorrection = -1;

	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_NOTRIGGERUN)) {
		/* Run triggers in this package other package(s) set off. */
		rc = (rpmRC) rpmpsmNext(psm, PSM_IMMED_TRIGGERS);
		if (rc) break;

		/* Run triggers in other package(s) this package sets off. */
		rc = (rpmRC) rpmpsmNext(psm, PSM_TRIGGERS);
		if (rc) break;
	    }

	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_NOPREUN))
		rc = (rpmRC) rpmpsmNext(psm, PSM_SCRIPT);
	}
	if (psm->goal == PSM_PKGSAVE) {
	    int noArchiveSize = 0;
	    const char * origin = NULL;
	    const char * digest = NULL;
	    const struct stat * st = NULL;
	    size_t nstbytes = 0;

	    /* Regenerate original header. */
	    {	void * uh = NULL;

		/* Save original header's origin/digest/stat (i.e. URL) */
		he->tag = RPMTAG_PACKAGEORIGIN;
		xx = headerGet(fi->h, he, 0);
		origin = he->p.str;
		he->tag = RPMTAG_PACKAGEDIGEST;
		xx = headerGet(fi->h, he, 0);
		if (xx && he->p.str != NULL)
		    digest = he->p.str;
		he->tag = RPMTAG_PACKAGESTAT;
		xx = headerGet(fi->h, he, 0);
		if (xx && he->p.ptr != NULL && (size_t)he->c == sizeof(*st)) {
		    st = (struct stat *) he->p.ptr;
		    nstbytes = he->c;
		}

		/* Retrieve original header blob. */
		he->tag = RPMTAG_HEADERIMMUTABLE;
		xx = headerGet(fi->h, he, 0);
		uh = he->p.ptr;
		if (xx && uh != NULL) {
		    psm->oh = headerCopyLoad(uh);
		    uh = _free(uh);
		} else {
		    he->tag = RPMTAG_HEADERIMAGE;
		    xx = headerGet(fi->h, he, 0);
		    uh = he->p.ptr;
		    if (xx && uh != NULL) {
			HeaderIterator hi;
			Header oh;

			/* Load the original header from the blob. */
			oh = headerCopyLoad(uh);

			/* XXX this is headerCopy w/o headerReload() */
			psm->oh = headerNew();

			for (hi = headerInit(oh);
			     headerNext(hi, he, 0);
			     he->p.ptr = _free(he->p.ptr))
			{
			    if (he->tag == RPMTAG_ARCHIVESIZE)
				noArchiveSize = 1;
			    xx = headerPut(psm->oh, he, 0);
			}
			hi = headerFini(hi);

			(void)headerFree(oh);
			oh = NULL;
			uh = _free(uh);
		    } else
			break;	/* XXX shouldn't ever happen */
		}
	    }

	    /* Retrieve type of payload compression. */
	    /*@-nullstate@*/	/* FIX: psm->oh may be NULL */
	    rc = (rpmRC) rpmpsmNext(psm, PSM_RPMIO_FLAGS);
	    /*@=nullstate@*/

	    /* Write the lead section into the package. */
	    {	static const char item[] = "Lead";
		const char * NEVR = rpmteNEVR(psm->te);
		const char * msg = xstrdup(NEVR); /* XXX passed to wrLead() */
		size_t nb = rpmpkgSizeof(item, NULL);
	
		if (nb == 0)
		    rc = RPMRC_FAIL;
		else {
		    void * l = memset(alloca(nb), 0, nb);
		    rc = rpmpkgWrite(item, psm->fd, l, &msg);
		}
		if (rc != RPMRC_OK) {
		    rpmlog(RPMLOG_ERR, _("Unable to write package: %s\n"),
				Fstrerror(psm->fd));
		    msg = _free(msg);
		    break;
		}
		msg = _free(msg);
	    }

	    /* Write the signature section into the package. */
	    /* XXX rpm-4.1 and later has archive size in signature header. */
	    {	static const char item[] = "Signature";
		Header sigh = headerRegenSigHeader(fi->h, noArchiveSize);
		/* Reallocate the signature into one contiguous region. */
		sigh = headerReload(sigh, RPMTAG_HEADERSIGNATURES);
		if (sigh == NULL) {
		    rpmlog(RPMLOG_ERR, _("Unable to reload signature header\n"));
		    rc = RPMRC_FAIL;
		    break;
		}
		rc = rpmpkgWrite(item, psm->fd, sigh, NULL);
		(void)headerFree(sigh);
		sigh = NULL;
		if (rc != RPMRC_OK) {
		    break;
		}
	    }

	    /* Add remove transaction id to header. */
	    if (psm->oh != NULL)
	    {	rpmuint32_t tid = rpmtsGetTid(ts);

		he->tag = RPMTAG_REMOVETID;
		he->t = RPM_UINT32_TYPE;
		he->p.ui32p = &tid;
		he->c = 1;
		xx = headerPut(psm->oh, he, 0);

		/* Add original header's origin/digest/stat (i.e. URL) */
		if (origin != NULL) {
		    he->tag = RPMTAG_PACKAGEORIGIN;
		    he->t = RPM_STRING_TYPE;
		    he->p.str = origin;
		    he->c = 1;
		    xx = headerPut(psm->oh, he, 0);
		    origin = _free(origin);
		}
		if (digest != NULL) {
		    he->tag = RPMTAG_PACKAGEDIGEST;
		    he->t = RPM_STRING_TYPE;
		    he->p.str = digest;
		    he->c = 1;
		    xx = headerPut(psm->oh, he, 0);
		    digest = _free(digest);
		}
		if (st != NULL) {
		    he->tag = RPMTAG_PACKAGESTAT;
		    he->t = RPM_BIN_TYPE;
		    he->p.ptr = (void *)st;
		    he->c = (rpmTagCount)nstbytes;
		    xx = headerPut(psm->oh, he, 0);
		    st = _free(st);
		}

		/* Copy upgrade chain link tags. */
		xx = hCopyTag(fi->h, psm->oh, RPMTAG_INSTALLTID);
		xx = hCopyTag(fi->h, psm->oh, RPMTAG_BLINKPKGID);
		xx = hCopyTag(fi->h, psm->oh, RPMTAG_BLINKHDRID);
		xx = hCopyTag(fi->h, psm->oh, RPMTAG_BLINKNEVRA);

assert(psm->te != NULL);
		xx = hSaveFlinks(psm->oh, &psm->te->flink);
	    }

	    /* Write the metadata section into the package. */
	    {	const char item[] = "Header";
		const char * msg = NULL;
		rc = rpmpkgWrite(item, psm->fd, psm->oh, &msg);
		if (rc != RPMRC_OK) {
		    rpmlog(RPMLOG_ERR, "%s: %s: %s", psm->pkgfn, item,
			(msg && *msg ? msg : "write failed\n"));
		    msg = _free(msg);
		}
	    }
	}
	break;
    case PSM_PROCESS:
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_TEST)	break;

	if (psm->goal == PSM_PKGINSTALL) {

	    if (rpmtsFlags(ts) & RPMTRANS_FLAG_JUSTDB)	break;

	    /* XXX Synthesize callbacks for packages with no files. */
	    if (rpmfiFC(fi) <= 0) {
		void * ptr;
		ptr = rpmtsNotify(ts, (rpmte)fi->te, RPMCALLBACK_INST_START, 0, 100);
		ptr = rpmtsNotify(ts, (rpmte)fi->te, RPMCALLBACK_INST_PROGRESS, 100, 100);
		break;
	    }

	    /* Retrieve type of payload compression. */
	    rc = (rpmRC) rpmpsmNext(psm, PSM_RPMIO_FLAGS);

	    if (rpmteFd((rpmte)fi->te) == NULL) {	/* XXX can't happen */
		rc = RPMRC_FAIL;
		break;
	    }

	    /*@-nullpass@*/	/* LCL: fi->fd != NULL here. */
	    psm->cfd = Fdopen(fdDup(Fileno(rpmteFd((rpmte)fi->te))), psm->rpmio_flags);
	    /*@=nullpass@*/
	    if (psm->cfd == NULL) {	/* XXX can't happen */
		rc = RPMRC_FAIL;
		break;
	    }

	    xx = rpmtxnBegin(rpmtsGetRdb(ts), psm->te->txn, NULL);

	    rc = (rpmRC) fsmSetup(fi->fsm, IOSM_PKGINSTALL, psm->payload_format,
			ts, fi, psm->cfd, NULL, &psm->failedFile);
	    (void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_UNCOMPRESS),
			fdstat_op(psm->cfd, FDSTAT_READ));
	    (void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DIGEST),
			fdstat_op(psm->cfd, FDSTAT_DIGEST));
	    xx = fsmTeardown(fi->fsm);

	    saveerrno = errno; /* XXX FIXME: Fclose with libio destroys errno */
	    xx = Fclose(psm->cfd);
	    psm->cfd = NULL;
	    /*@-mods@*/
	    errno = saveerrno; /* XXX FIXME: Fclose with libio destroys errno */
	    /*@=mods@*/

	    if (!rc)
		rc = (rpmRC) rpmpsmNext(psm, PSM_COMMIT);

	    /* Commit/abort the SRPM install transaction. */
	    /* XXX move into the PSM package state machine w PSM_COMMIT */
	{   rpmdb db = rpmtsGetRdb(ts);
	    rpmtxn _txn = (db ? db->db_txn : NULL);
	    if (_txn != NULL) {
		if (rc)
		    xx = rpmtxnAbort(_txn);
		else
		    xx = rpmtxnCommit(_txn);
		db->db_txn = NULL;
	    }
	}

	    /* XXX make sure progress is closed out */
	    psm->what = RPMCALLBACK_INST_PROGRESS;
	    psm->amount = (fi->archiveSize ? fi->archiveSize : 100);
	    psm->total = psm->amount;
	    xx = rpmpsmNext(psm, PSM_NOTIFY);

	    if (rc) {
		const char * msg = iosmStrerror(rc);
		rpmlog(RPMLOG_ERR,
			_("unpacking of archive failed%s%s: %s\n"),
			(psm->failedFile != NULL ? _(" on file ") : ""),
			(psm->failedFile != NULL ? psm->failedFile : ""),
			msg);
		msg = _free(msg);
		rc = RPMRC_FAIL;

		/* XXX notify callback on error. */
		psm->what = RPMCALLBACK_UNPACK_ERROR;
		psm->amount = 0;
		psm->total = 0;
		xx = rpmpsmNext(psm, PSM_NOTIFY);

		break;
	    }
	}
	if (psm->goal == PSM_PKGERASE) {
	    int fc = rpmfiFC(fi);

	    if (rpmtsFlags(ts) & RPMTRANS_FLAG_JUSTDB)	break;
	    if (rpmtsFlags(ts) & RPMTRANS_FLAG_APPLYONLY)	break;

	    psm->what = RPMCALLBACK_UNINST_START;
	    psm->amount = fc;
	    psm->total = (fc ? fc : 100);
	    xx = rpmpsmNext(psm, PSM_NOTIFY);

	    if (fc > 0) {
		rc = (rpmRC) fsmSetup(fi->fsm, IOSM_PKGERASE, psm->payload_format,
			ts, fi, NULL, NULL, &psm->failedFile);
		xx = fsmTeardown(fi->fsm);
	    }

	    psm->what = RPMCALLBACK_UNINST_STOP;
	    psm->amount = (fc ? fc : 100);
	    psm->total = (fc ? fc : 100);
	    xx = rpmpsmNext(psm, PSM_NOTIFY);

	}
	if (psm->goal == PSM_PKGSAVE) {
	    iosmFileAction * actions = (iosmFileAction *) fi->actions;
	    iosmFileAction action = (iosmFileAction) fi->action;

	    fi->action = FA_COPYOUT;
	    fi->actions = NULL;

	    if (psm->fd == NULL) {	/* XXX can't happen */
		rc = RPMRC_FAIL;
		break;
	    }
	    /*@-nullpass@*/	/* FIX: fdDup mey return NULL. */
	    xx = Fflush(psm->fd);
	    psm->cfd = Fdopen(fdDup(Fileno(psm->fd)), psm->rpmio_flags);
	    /*@=nullpass@*/
	    if (psm->cfd == NULL) {	/* XXX can't happen */
		rc = RPMRC_FAIL;
		break;
	    }

	    rc = (rpmRC) fsmSetup(fi->fsm, IOSM_PKGBUILD, psm->payload_format,
			ts, fi, psm->cfd, NULL, &psm->failedFile);
	    (void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_COMPRESS),
			fdstat_op(psm->cfd, FDSTAT_WRITE));
	    (void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DIGEST),
			fdstat_op(psm->cfd, FDSTAT_DIGEST));
	    xx = fsmTeardown(fi->fsm);

	    saveerrno = errno; /* XXX FIXME: Fclose with libio destroys errno */
	    xx = Fclose(psm->cfd);
	    psm->cfd = NULL;
	    /*@-mods@*/
	    errno = saveerrno;
	    /*@=mods@*/

	    /* XXX make sure progress is closed out */
	    psm->what = RPMCALLBACK_INST_PROGRESS;
	    psm->amount = (fi->archiveSize ? fi->archiveSize : 100);
	    psm->total = psm->amount;
	    xx = rpmpsmNext(psm, PSM_NOTIFY);

	    fi->action = (int) action;
	    fi->actions = (int *) actions;
	}
	break;
    case PSM_POST:
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_TEST)	break;

	if (psm->goal == PSM_PKGINSTALL) {

	    psm->scriptTag = RPMTAG_POSTIN;
	    psm->progTag = RPMTAG_POSTINPROG;
	    psm->sense = RPMSENSE_TRIGGERIN;
	    psm->countCorrection = 1;

	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_NOPOST)) {
		rc = (rpmRC) rpmpsmNext(psm, PSM_SCRIPT);
		if (rc) break;
	    }
	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_NOTRIGGERIN)) {
		/* Run triggers in other package(s) this package sets off. */
		rc = (rpmRC) rpmpsmNext(psm, PSM_TRIGGERS);
		if (rc) break;

		/* Run triggers in this package other package(s) set off. */
		rc = (rpmRC) rpmpsmNext(psm, PSM_IMMED_TRIGGERS);
		if (rc) break;
	    }

	    /*
	     * If this header has already been installed, remove it from
	     * the database before adding the new header.
	     */
	    if (fi->record && !(rpmtsFlags(ts) & RPMTRANS_FLAG_APPLYONLY)) {
		rc = (rpmRC) rpmpsmNext(psm, PSM_RPMDB_REMOVE);
		if (rc) break;
	    }

	    /* Add scriptlet/file states to install header. */
	    xx = postPopulateInstallHeader(ts, psm, fi);

	    rc = (rpmRC) rpmpsmNext(psm, PSM_RPMDB_ADD);
	    if (rc) break;

#ifdef	DYING
	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_APPLYONLY))
		rc = markReplacedFiles(psm);
#endif

	}
	if (psm->goal == PSM_PKGERASE) {

	    psm->scriptTag = RPMTAG_POSTUN;
	    psm->progTag = RPMTAG_POSTUNPROG;
	    psm->sense = RPMSENSE_TRIGGERPOSTUN;
	    psm->countCorrection = -1;

	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_NOPOSTUN)) {
		rc = (rpmRC) rpmpsmNext(psm, PSM_SCRIPT);
		if (rc) break;
	    }

	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_NOTRIGGERPOSTUN)) {
		/* Run triggers in other package(s) this package sets off. */
		rc = (rpmRC) rpmpsmNext(psm, PSM_TRIGGERS);
		if (rc) break;

		/* Run triggers in this package other package(s) set off. */
		rc = (rpmRC) rpmpsmNext(psm, PSM_IMMED_TRIGGERS);
		if (rc) break;
	    }

	    if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_APPLYONLY))
		rc = (rpmRC) rpmpsmNext(psm, PSM_RPMDB_REMOVE);
	}
	if (psm->goal == PSM_PKGSAVE) {
	}

	/* Restore root directory if changed. */
	xx = rpmpsmNext(psm, PSM_CHROOT_OUT);
	break;
    case PSM_UNDO:
	break;
    case PSM_FINI:
	/* Restore root directory if changed. */
	xx = rpmpsmNext(psm, PSM_CHROOT_OUT);

	if (psm->fd != NULL) {
	    saveerrno = errno; /* XXX FIXME: Fclose with libio destroys errno */
	    xx = Fclose(psm->fd);
	    psm->fd = NULL;
	    /*@-mods@*/
	    errno = saveerrno;
	    /*@=mods@*/
	}

	if (psm->goal == PSM_PKGSAVE) {
	    if (!rc && ts && ts->notify == NULL) {
		rpmlog(RPMLOG_INFO, _("Wrote: %s\n"),
			(psm->pkgURL ? psm->pkgURL : "???"));
	    }
	}

	if (rc) {
	    const char * msg = iosmStrerror(rc);
	    if (psm->failedFile)
		rpmlog(RPMLOG_ERR,
			_("%s failed on file %s: %s\n"),
			psm->stepName, psm->failedFile, msg);
	    else
		rpmlog(RPMLOG_ERR, _("%s failed: %s\n"),
			psm->stepName, msg);
	    msg = _free(msg);

	    /* XXX notify callback on error. */
	    psm->what = RPMCALLBACK_CPIO_ERROR;
	    psm->amount = 0;
	    psm->total = 0;
	    /*@-nullstate@*/ /* FIX: psm->fd may be NULL. */
	    xx = rpmpsmNext(psm, PSM_NOTIFY);
	    /*@=nullstate@*/
	    if (psm->te->txn != NULL) {
		xx = rpmtxnAbort(psm->te->txn);
		psm->te->txn = NULL;
	    }
	} else {
	    if (psm->te->txn != NULL) {
		xx = rpmtxnCommit(psm->te->txn);
		psm->te->txn = NULL;
	    }
	}

	if (psm->goal == PSM_PKGERASE || psm->goal == PSM_PKGSAVE) {
	    if (psm->te != NULL)
		(void) rpmteSetHeader(psm->te, NULL);
	    if (fi->h != NULL) {
		(void)headerFree(fi->h);
		fi->h = NULL;
	    }
 	}
	(void)headerFree(psm->oh);
	psm->oh = NULL;
	psm->pkgURL = _free(psm->pkgURL);
	psm->rpmio_flags = _free(psm->rpmio_flags);
	psm->payload_format = _free(psm->payload_format);
	psm->failedFile = _free(psm->failedFile);

	fi->fgroup = _free(fi->fgroup);
	fi->fuser = _free(fi->fuser);
	fi->apath = _free(fi->apath);
	fi->fstates = _free(fi->fstates);
	break;

    case PSM_PKGINSTALL:
    case PSM_PKGERASE:
    case PSM_PKGSAVE:
	psm->goal = stage;
	psm->rc = RPMRC_OK;
	psm->stepName = pkgStageString(stage);

	rc = (rpmRC) rpmpsmNext(psm, PSM_INIT);
	if (!rc) rc = (rpmRC) rpmpsmNext(psm, PSM_PRE);
	if (!rc) rc = (rpmRC) rpmpsmNext(psm, PSM_PROCESS);
	if (!rc) rc = (rpmRC) rpmpsmNext(psm, PSM_POST);
	xx = rpmpsmNext(psm, PSM_FINI);
	break;
    case PSM_PKGCOMMIT:
	break;

    case PSM_CREATE:
	break;
    case PSM_NOTIFY:
    {	void * ptr;
/*@-nullpass@*/ /* FIX: psm->te may be NULL */
	ptr = rpmtsNotify(ts, psm->te, psm->what, psm->amount, psm->total);
/*@-nullpass@*/
    }	break;
    case PSM_DESTROY:
	break;
    case PSM_COMMIT:
	if (!(rpmtsFlags(ts) & RPMTRANS_FLAG_PKGCOMMIT)) break;
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_APPLYONLY) break;

	rc = (rpmRC) fsmSetup(fi->fsm, IOSM_PKGCOMMIT, psm->payload_format,
			ts, fi, NULL, NULL, &psm->failedFile);
	xx = fsmTeardown(fi->fsm);
	break;

    case PSM_CHROOT_IN:
    {	const char * rootDir = rpmtsRootDir(ts);
	/* Change root directory if requested and not already done. */
	if (rootDir != NULL && !(rootDir[0] == '/' && rootDir[1] == '\0')
	 && !rpmtsChrootDone(ts) && !F_ISSET(psm, CHROOTDONE))
	{
	    static int _pw_loaded = 0;
	    static int _gr_loaded = 0;

	    if (!_pw_loaded) {
		(void)getpwnam("root");
		endpwent();
		_pw_loaded++;
	    }
	    if (!_gr_loaded) {
		(void)getgrnam("root");
		endgrent();
		_gr_loaded++;
	    }

	    xx = Chdir("/");
	    /*@-modobserver@*/
	    if (rootDir != NULL && strcmp(rootDir, "/") && *rootDir == '/')
		rc = (rpmRC) Chroot(rootDir);
	    /*@=modobserver@*/
	    F_SET(psm, CHROOTDONE);
	    (void) rpmtsSetChrootDone(ts, 1);
	}
    }	break;
    case PSM_CHROOT_OUT:
	/* Restore root directory if changed. */
	if (F_ISSET(psm, CHROOTDONE)) {
	    const char * rootDir = rpmtsRootDir(ts);
	    const char * currDir = rpmtsCurrDir(ts);
	    /*@-modobserver@*/
	    if (rootDir != NULL && strcmp(rootDir, "/") && *rootDir == '/')
		rc = (rpmRC) Chroot(".");
	    /*@=modobserver@*/
	    F_CLR(psm, CHROOTDONE);
	    (void) rpmtsSetChrootDone(ts, 0);
	    if (currDir != NULL)	/* XXX can't happen */
		xx = Chdir(currDir);
	}
	break;
    case PSM_SCRIPT:	/* Run current package scriptlets. */
	/* XXX running %verifyscript/%sanitycheck doesn't have psm->te */
    {	rpmtxn _parent = (psm && psm->te ? psm->te->txn : NULL);
	xx = rpmtxnBegin(rpmtsGetRdb(ts), _parent, NULL);
	rc = runInstScript(psm);
	if (rc)
	     xx = rpmtxnAbort(rpmtsGetRdb(ts)->db_txn);
	else
	     xx = rpmtxnCommit(rpmtsGetRdb(ts)->db_txn);
	rpmtsGetRdb(ts)->db_txn = NULL;
    }	break;
    case PSM_TRIGGERS:
	/* Run triggers in other package(s) this package sets off. */
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_TEST)	break;
	rc = runTriggers(psm);
	break;
    case PSM_IMMED_TRIGGERS:
	/* Run triggers in this package other package(s) set off. */
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_TEST)	break;
	if (!F_ISSET(psm, GOTTRIGGERS)) {
	    psm->triggers = rpmdsNew(fi->h, RPMTAG_TRIGGERNAME, 0);
	    F_SET(psm, GOTTRIGGERS);
	}
	if (psm->triggers != NULL)
	    rc = runImmedTriggers(psm);
	break;

    case PSM_RPMIO_FLAGS:
    {	const char * payload_compressor = NULL;
	const char * payload_format = NULL;
	char * t;

	he->tag = RPMTAG_PAYLOADCOMPRESSOR;
	xx = headerGet(fi->h, he, 0);
	payload_compressor = he->p.str;
	if (payload_compressor == NULL)
	    payload_compressor = xstrdup("gzip");

	psm->rpmio_flags = t = (char *) xmalloc(sizeof("w9.gzdio"));
	*t = '\0';
	t = stpcpy(t, ((psm->goal == PSM_PKGSAVE) ? "w9" : "r"));
	if (!strcmp(payload_compressor, "gzip"))
	    t = stpcpy(t, ".gzdio");
	if (!strcmp(payload_compressor, "bzip2"))
	    t = stpcpy(t, ".bzdio");
	if (!strcmp(payload_compressor, "lzma"))
	    t = stpcpy(t, ".lzdio");
	if (!strcmp(payload_compressor, "xz"))
	    t = stpcpy(t, ".xzdio");
	payload_compressor = _free(payload_compressor);

	he->tag = RPMTAG_PAYLOADFORMAT;
	xx = headerGet(fi->h, he, 0);
	payload_format = he->p.str;
	if (!xx || payload_format == NULL || !(
	  !strcmp(payload_format, "tar") || !strcmp(payload_format, "ustar")
#if defined(SUPPORT_AR_PAYLOADS)
	 || !strcmp(payload_format, "ar")
#endif
	   ))
	{
	    payload_format = _free(payload_format);
	    payload_format = xstrdup("cpio");
	}
	psm->payload_format = _free(psm->payload_format);
	psm->payload_format = payload_format;
	rc = RPMRC_OK;
    }	break;

    case PSM_RPMDB_LOAD:
assert(psm->mi == NULL);
	psm->mi = rpmtsInitIterator(ts, RPMDBI_PACKAGES,
				&fi->record, sizeof(fi->record));
	fi->h = rpmmiNext(psm->mi);
/*@-castexpose@*/
	if (fi->h != NULL)
	    fi->h = headerLink(fi->h);
/*@=castexpose@*/
	psm->mi = rpmmiFree(psm->mi);

	if (fi->h != NULL) {
	    (void) headerSetInstance(fi->h, fi->record);
	    rc = RPMRC_OK;
	} else
	    rc = RPMRC_FAIL;
	break;
    case PSM_RPMDB_ADD:
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_TEST)	break;
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_NORPMDB)	break;
	if (fi->isSource)	break;	/* XXX never add SRPM's */
	if (fi->h == NULL)	break;	/* XXX can't happen */

	xx = rpmtxnBegin(rpmtsGetRdb(ts), psm->te->txn, NULL);

	/* Add header to db, doing header check if requested */
	/* XXX rollback headers propagate the previous transaction id. */
	{   rpmuint32_t tid = ((rpmtsType(ts) == RPMTRANS_TYPE_ROLLBACK)
		? hLoadTID(fi->h, RPMTAG_INSTALLTID) : rpmtsGetTid(ts));
	    (void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_DBADD), 0);
	    if (!(rpmtsVSFlags(ts) & RPMVSF_NOHDRCHK))
		rc = (rpmRC) rpmdbAdd(rpmtsGetRdb(ts), tid, fi->h, ts);
	    else
		rc = (rpmRC) rpmdbAdd(rpmtsGetRdb(ts), tid, fi->h, NULL);
	    (void) rpmswExit(rpmtsOp(ts, RPMTS_OP_DBADD), 0);
#if defined(HAVE_SYSLOG_H) && defined(RPM_VENDOR_MANDRIVA) /* log-install-remove-to-syslog */
    	    {
		char *s, fmt;
		fmt = rpmExpand("%{___NVRA}", NULL);
		s = headerSprintf(fi->h, fmt,
			rpmTagTable, rpmHeaderFormats, NULL);
		syslog(LOG_NOTICE, "[RPM] %s installed\n", s);
		fmt = _free(fmt);
		s = _free(s);
    	    }
#endif
	}

	if (rc != RPMRC_OK) {
	    xx = rpmtxnAbort(rpmtsGetRdb(ts)->db_txn);
	    rpmtsGetRdb(ts)->db_txn = NULL;
	    break;
	} else
	    xx = rpmtxnCommit(rpmtsGetRdb(ts)->db_txn);
	rpmtsGetRdb(ts)->db_txn = NULL;

assert(psm->te != NULL);
	/* Mark non-rollback elements as installed. */
	if (rpmtsType(ts) != RPMTRANS_TYPE_ROLLBACK)
	    psm->te->installed = 1;

	/* Set the database instance for (possible) rollbacks. */
	rpmteSetDBInstance(psm->te, headerGetInstance(fi->h));

	break;
    case PSM_RPMDB_REMOVE:
    {	
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_TEST)	break;
	if (rpmtsFlags(ts) & RPMTRANS_FLAG_NORPMDB)	break;

	xx = rpmtxnBegin(rpmtsGetRdb(ts), psm->te->txn, NULL);

	(void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_DBREMOVE), 0);
	rc = (rpmRC) rpmdbRemove(rpmtsGetRdb(ts), rpmtsGetTid(ts), fi->record, NULL);
	(void) rpmswExit(rpmtsOp(ts, RPMTS_OP_DBREMOVE), 0);
#if defined(HAVE_SYSLOG_H) && defined(RPM_VENDOR_MANDRIVA) /* log-install-remove-to-syslog */
        {
	    char *s, fmt;
	    fmt = rpmExpand("%{___NVRA}", NULL);
	    s = headerSprintf(fi->h, fmt,
		    rpmTagTable, rpmHeaderFormats, NULL);
	    syslog(LOG_NOTICE, "[RPM] %s removed\n", s);
	    fmt = _free(fmt);
	    s = _free(s);
	}
#endif

	if (rc != RPMRC_OK) {
	    xx = rpmtxnAbort(rpmtsGetRdb(ts)->db_txn);
	    rpmtsGetRdb(ts)->db_txn = NULL;
	    break;
	} else
	    xx = rpmtxnCommit(rpmtsGetRdb(ts)->db_txn);
	rpmtsGetRdb(ts)->db_txn = NULL;

	/* Forget the offset of a successfully removed header. */
	if (psm->te != NULL)	/* XXX can't happen */
	    psm->te->u.removed.dboffset = 0;

    }	break;

    default:
	break;
/*@i@*/    }

/*@-nullstate@*/	/* FIX: psm->oh and psm->fi->h may be NULL. */
    return rc;
/*@=nullstate@*/
}
/*@=nullpass@*/
