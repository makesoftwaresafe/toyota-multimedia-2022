#include "system.h"
extern const char *__progname;

/* Copyright (C) 1998-2002 - Red Hat, Inc. */

#define	_AUTOHELP

#if defined(IAM_RPM) || defined(__LCLINT__)
#define	IAM_RPMBT
#define	IAM_RPMDB
#define	IAM_RPMEIU
#define	IAM_RPMQV
#define	IAM_RPMK
#endif

#if defined(RPM_VENDOR_OPENPKG) /* integrity-checking */
#define	_RPMIOB_INTERNAL	/* XXX rpmiobSlurp */
#include "rpmio_internal.h"
#endif

#include <rpmio.h>
#include <rpmiotypes.h>
#include <poptIO.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include "rpmdb.h"

#if defined(IAM_RPMBT) || defined(IAM_RPMK)
#include "signature.h"
#endif

#if defined(RPM_VENDOR_OPENPKG) /* integrity-checking */
#include "rpmns.h"
#define _RPMLUA_INTERNAL
#include "rpmlua.h"
#include "rpmluaext.h"
#endif

#include "rpmversion.h"
#include "rpmps.h"
#include "rpmts.h"

#include "fs.h"		/* XXX for rpmFreeFilesystems() */

#include <rpmbuild.h>

#ifdef	IAM_RPMBT
#include "build.h"
#define GETOPT_REBUILD		1003
#define GETOPT_RECOMPILE	1004
#endif

#include <rpmcli.h>
#include <rpmrollback.h>

#include "debug.h"


#ifdef __cplusplus

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))
#define QVA_SET(_qvaflags, _FLAG) { \
	unsigned ui = (unsigned)(_qvaflags); ui |= (VERIFY_##_FLAG); \
	(_qvaflags) = (rpmVerifyFlags) ui; \
	}
#define QVA_CLR(_qvaflags, _FLAG) { \
	unsigned ui = (unsigned)(_qvaflags); ui &= ~(VERIFY_##_FLAG); \
	(_qvaflags) = (rpmVerifyFlags) ui; \
	}

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG) { \
	unsigned ui = (unsigned)(_vsflags); ui |= (RPMVSF_##_FLAG); \
	(_vsflags) = (pgpVSFlags) ui; \
	}
#define VSF_CLR(_vsflags, _FLAG) { \
	unsigned ui = (unsigned)(_vsflags); ui &= ~(RPMVSF_##_FLAG); \
	(_vsflags) = (pgpVSFlags) ui; \
	}

#define TSF_ISSET(_tsflags, _FLAG)	((_tsflags) & (RPMTRANS_FLAG_##_FLAG))
#define TSF_SET(_tsflags, _FLAG) { \
	unsigned ui = (unsigned)(_tsflags); ui |= (RPMTRANS_FLAG_##_FLAG); \
	(_tsflags) = (rpmtransFlags) ui; \
	}
#define TSF_CLR(_tsflags, _FLAG) { \
	unsigned ui = (unsigned)(_tsflags); ui &= ~(RPMTRANS_FLAG_##_FLAG); \
	(_tsflags) = (rpmtransFlags) ui; \
	}

#define IIF_ISSET(_iflags, _FLAG)	((_iflags) & (INSTALL_##_FLAG))
#define IIF_SET(_iflags, _FLAG)	{ \
	unsigned ui = (unsigned)(_iflags); ui |= (INSTALL_##_FLAG); \
	(_iflags) = (rpmInstallInterfaceFlags) ui; \
	}
#define IIF_CLR(_iflags, _FLAG)	{ \
	unsigned ui = (unsigned)(_iflags); ui &= ~(INSTALL_##_FLAG); \
	(_iflags) = (rpmInstallInterfaceFlags) ui; \
	}

#define PFF_ISSET(_pfflags, _FLAG)	((_pfflags) & (RPMPROB_FILTER_##_FLAG))
#define PFF_SET(_pfflags, _FLAG) { \
	unsigned ui = (unsigned)(_pfflags); ui |= (RPMPROB_FILTER_##_FLAG); \
	(_pfflags) = (rpmprobFilterFlags) ui; \
	}
#define PFF_CLR(_pfflags, _FLAG) { \
	unsigned ui = (unsigned)(_pfflags); ui &= ~(RPMPROB_FILTER_##_FLAG); \
	(_pfflags) = (rpmprobFilterFlags) ui; \
	}

#else	/* __cplusplus */

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))
#define QVA_SET(_qvaflags, _FLAG)	(_qvaflags) |= (VERIFY_##_FLAG)
#define QVA_CLR(_qvaflags, _FLAG)	(_qvaflags) &= ~(VERIFY_##_FLAG)

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#define TSF_ISSET(_tsflags, _FLAG)	((_tsflags) &  (RPMTRANS_FLAG_##_FLAG))
#define TSF_SET(_tsflags, _FLAG)	(_tsflags) |=  (RPMTRANS_FLAG_##_FLAG)
#define TSF_CLR(_tsflags, _FLAG)	(_tsflags) &= ~(RPMTRANS_FLAG_##_FLAG)

#define IIF_ISSET(_iflags, _FLAG)	((_iflags) &  (INSTALL_##_FLAG))
#define IIF_SET(_iflags, _FLAG)		(_iflags) |=  (INSTALL_##_FLAG)
#define IIF_CLR(_iflags, _FLAG)		(_iflags) &= ~(INSTALL_##_FLAG)

#define PFF_ISSET(_pfflags, _FLAG)	((_pfflags) &  (RPMPROB_FILTER_##_FLAG))
#define PFF_SET(_pfflags, _FLAG)	(_pfflags) |=  (RPMPROB_FILTER_##_FLAG)
#define PFF_CLR(_pfflags, _FLAG)	(_pfflags) &= ~(RPMPROB_FILTER_##_FLAG)

#endif	/* __cplusplus */

enum modes {
    MODE_UNKNOWN	= 0,

    MODE_QUERY		= (1 <<  0),
    MODE_VERIFY		= (1 <<  3),
#define	MODES_QV (MODE_QUERY | MODE_VERIFY)

    MODE_INSTALL	= (1 <<  1),
    MODE_ERASE		= (1 <<  2),
#define	MODES_IE (MODE_INSTALL | MODE_ERASE)

    MODE_BUILD		= (1 <<  4),
    MODE_REBUILD	= (1 <<  5),
    MODE_RECOMPILE	= (1 <<  8),
    MODE_TARBUILD	= (1 << 11),
#define	MODES_BT (MODE_BUILD | MODE_TARBUILD | MODE_REBUILD | MODE_RECOMPILE)

    MODE_CHECKSIG	= (1 <<  6),
    MODE_RESIGN		= (1 <<  7),
#define	MODES_K	 (MODE_CHECKSIG | MODE_RESIGN)

    MODE_REBUILDDB	= (1 << 12),
#define	MODES_DB (MODE_REBUILDDB)
};

#define	MODES_FOR_DBPATH	(MODES_BT | MODES_IE | MODES_QV | MODES_DB)
#define	MODES_FOR_NODEPS	(MODES_BT | MODES_IE | MODE_VERIFY)
#define	MODES_FOR_TEST		(MODES_BT | MODES_IE)
#define	MODES_FOR_ROOT		(MODES_BT | MODES_IE | MODES_QV | MODES_DB | MODES_K)

/* the structure describing the options we take and the defaults */
/*@unchecked@*/
static struct poptOption optionsTable[] = {

#ifdef	IAM_RPMQV
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmQueryPoptTable, 0,
	N_("Query options (with -q or --query):"),
	NULL },
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmVerifyPoptTable, 0,
	N_("Verify options (with -V or --verify):"),
	NULL },
#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliQVSourcePoptTable, 0,
        N_("Source options (with --query or --verify):"),
        NULL },
#endif
#endif	/* IAM_RPMQV */

#if defined(IAM_RPMQV) || defined(IAM_RPMEIU)
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliDepFlagsPoptTable, 0,
        N_("Dependency check/order options:"),
        NULL },
#endif	/* IAM_RPMQV */

#ifdef	IAM_RPMQV
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioFtsPoptTable, 0,
        N_("File tree walk options (with --ftswalk):"),
        NULL },
#endif	/* IAM_RPMQV */

#ifdef	IAM_RPMK
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmSignPoptTable, 0,
	N_("Signature options:"),
	NULL },
#endif	/* IAM_RPMK */

#ifdef	IAM_RPMDB
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmDatabasePoptTable, 0,
	N_("Database options:"),
	NULL },
#endif	/* IAM_RPMDB */

#ifdef	IAM_RPMBT
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmBuildPoptTable, 0,
	N_("Build options with [ <specfile> | <tarball> | <source package> ]:"),
	NULL },
#endif	/* IAM_RPMBT */

#ifdef	IAM_RPMEIU
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmInstallPoptTable, 0,
	N_("Install/Upgrade/Erase options:"),
	NULL },
#endif	/* IAM_RPMEIU */

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
	N_("Common options:"),
	NULL },

   POPT_AUTOALIAS
   POPT_AUTOHELP
   POPT_TABLEEND
};

#ifdef __MINT__
/* MiNT cannot dynamically increase the stack.  */
long _stksize = 64 * 1024L;
#endif

/*@exits@*/ static void argerror(const char * desc)
	/*@globals __assert_program_name, fileSystem @*/
	/*@modifies fileSystem @*/
{
    fprintf(stderr, _("%s: %s\n"), __progname, desc);
    exit(EXIT_FAILURE);
}

#ifdef	DYING /* XXX rpmIsVerbose alone stops usage spewage with every --eval */
static void printVersion(FILE * fp)
	/*@globals rpmEVR, fileSystem @*/
	/*@modifies *fp, fileSystem @*/
{
    fprintf(fp, "%s (" RPM_NAME ") %s\n", __progname, rpmEVR);
    if (rpmIsVerbose())
	fprintf(fp, "rpmlib 0x%08x,0x%08x,0x%08x\n",
	    rpmlibVersion(), rpmlibTimestamp(), rpmlibVendor());
}

static void printUsage(poptContext con, FILE * fp, int flags)
	/*@globals rpmEVR, fileSystem, internalState @*/
	/*@modifies *fp, fileSystem, internalState @*/
{
    printVersion(fp);
    fprintf(fp, "\n");

    if (rpmIsVerbose())
	poptPrintHelp(con, fp, flags);
    else
	poptPrintUsage(con, fp, flags);
}
#endif

#if defined(RPM_VENDOR_OPENPKG) /* integrity-checking */

#if !defined(RPM_INTEGRITY_FP)
#error required RPM_INTEGRITY_FP (fingerprint of public key of integrity authority) not defined!
#endif

enum {
    INTEGRITY_OK      = 0,
    INTEGRITY_WARNING = 1,
    INTEGRITY_ERROR   = 2
};

static void integrity_check_message(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "rpm: ATTENTION: INTEGRITY CHECKING DETECTED AN ENVIRONMENT ANOMALY!\nrpm: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    return;
}

static void integrity_check(const char *progname, enum modes progmode_num)
{
    rpmts ts = NULL;
    rpmlua lua = NULL;
    char *spec_fn = NULL;
    char *proc_fn = NULL;
    char *pkey_fn = NULL;
    char *spec = NULL;
    char *proc = NULL;
    rpmiob spec_iob = NULL;
    rpmiob proc_iob = NULL;
    const char *result = NULL;
    const char *error = NULL;
    int xx;
    const char *progmode;
    int rc = INTEGRITY_ERROR;

    /* determine paths of integrity checking related files */
    spec_fn = rpmExpand("%{?_integrity_spec_cfg}%{!?_integrity_spec_cfg:scripts/integrity.cfg}", NULL);
    if (spec_fn == NULL || spec_fn[0] == '\0') {
        integrity_check_message("ERROR: Integrity Configuration Specification file not configured.\n"
            "rpm: HINT: macro %%{_integrity_spec_cfg} not configured correctly.\n");
        goto failure;
    }
    proc_fn = rpmExpand("%{?_integrity_proc_lua}%{!?_integrity_proc_lua:scripts/integrity.lua}", NULL);
    if (proc_fn == NULL || proc_fn[0] == '\0') {
        integrity_check_message("ERROR: Integrity Validation Processor file not configured.\n"
            "rpm: HINT: macro %%{_integrity_proc_lua} not configured correctly.\n");
        goto failure;
    }
    pkey_fn = rpmExpand("%{?_integrity_pkey_pgp}%{!?_integrity_pkey_pgp:scripts/integrity.pgp}", NULL);
    if (pkey_fn == NULL || pkey_fn[0] == '\0') {
        integrity_check_message("ERROR: Integrity Authority Public-Key file not configured.\n"
            "rpm: HINT: macro %%{_integrity_pkey_pgp} not configured correctly.\n");
        goto failure;
    }

    /* create RPM transaction environment and open RPM database */
    ts = rpmtsCreate();
    (void)rpmtsOpenDB(ts, O_RDONLY);

    /* check signature on integrity configuration specification file */
    if (rpmnsProbeSignature(ts, spec_fn, NULL, pkey_fn, RPM_INTEGRITY_FP, 0) != RPMRC_OK) {
        integrity_check_message("ERROR: Integrity Configuration Specification file contains invalid signature.\n"
            "rpm: HINT: Check file \"%s\".\n", spec_fn);
        goto failure;
    }

    /* check signature on integrity validation processor file */
    if (rpmnsProbeSignature(ts, proc_fn, NULL, pkey_fn, RPM_INTEGRITY_FP, 0) != RPMRC_OK) {
        integrity_check_message("ERROR: Integrity Validation Processor file contains invalid signature.\n"
            "rpm: HINT: Check file \"%s\".\n", proc_fn);
        goto failure;
    }

    /* load integrity configuration specification file */
    xx = rpmiobSlurp(spec_fn, &spec_iob);
    if (!(xx == 0 && spec_iob != NULL)) {
        integrity_check_message("ERROR: Unable to load Integrity Configuration Specification file.\n"
            "rpm: HINT: Check file \"%s\".\n", spec_fn);
        goto failure;
    }
    spec = rpmiobStr(spec_iob);

    /* load integrity validation processor file */
    xx = rpmiobSlurp(proc_fn, &proc_iob);
    if (!(xx == 0 && proc_iob != NULL)) {
        integrity_check_message("ERROR: Unable to load Integrity Validation Processor file.\n"
            "rpm: HINT: Check file \"%s\".\n", proc_fn);
        goto failure;
    }
    proc = rpmiobStr(proc_iob);

    /* provision program name and mode */
    if (progname == NULL || progname[0] == '\0')
        progname = "rpm";
    switch (progmode_num) {
        case MODE_QUERY:     progmode = "query";     break;
        case MODE_VERIFY:    progmode = "verify";    break;
        case MODE_CHECKSIG:  progmode = "checksig";  break;
        case MODE_RESIGN:    progmode = "resign";    break;
        case MODE_INSTALL:   progmode = "install";   break;
        case MODE_ERASE:     progmode = "erase";     break;
        case MODE_BUILD:     progmode = "build";     break;
        case MODE_REBUILD:   progmode = "rebuild";   break;
        case MODE_RECOMPILE: progmode = "recompile"; break;
        case MODE_TARBUILD:  progmode = "tarbuild";  break;
        case MODE_REBUILDDB: progmode = "rebuilddb"; break;
        case MODE_UNKNOWN:   progmode = "unknown";   break;
        default:             progmode = "unknown";   break;
    }

    /* execute Integrity Validation Processor via Lua glue code */
    lua = rpmluaNew();
    rpmluaSetPrintBuffer(lua, 1);
    rpmluaextActivate(lua);
    lua_getfield(lua->L, LUA_GLOBALSINDEX, "integrity");
    lua_getfield(lua->L, -1, "processor");
    lua_remove(lua->L, -2);
    lua_pushstring(lua->L, progname);
    lua_pushstring(lua->L, progmode);
    lua_pushstring(lua->L, spec_fn);
    lua_pushstring(lua->L, spec);
    lua_pushstring(lua->L, proc_fn);
    lua_pushstring(lua->L, proc);
#ifdef RPM_INTEGRITY_MV
    lua_pushstring(lua->L, RPM_INTEGRITY_MV);
#else
    lua_pushstring(lua->L, "0");
#endif
    if (lua_pcall(lua->L, 7, 1, 0) != 0) {
        error = lua_isstring(lua->L, -1) ? lua_tostring(lua->L, -1) : "unknown error";
        lua_pop(lua->L, 1);
        integrity_check_message("ERROR: Failed to execute Integrity Validation Processor.\n"
            "rpm: ERROR: Lua: %s.\n"
            "rpm: HINT: Check file \"%s\".\n", error, proc_fn);
        goto failure;
    }

    /* check Integrity Validation Processor results */
    if (!lua_isstring(lua->L, -1)) {
        integrity_check_message("ERROR: Failed to fetch Integrity Validation Processor results.\n"
            "rpm: HINT: Check file \"%s\".\n", proc_fn);
        goto failure;
    }
    result = lua_tostring(lua->L, -1);
    if (strcmp(result, "OK") == 0)
        rc = INTEGRITY_OK;
    else if (strncmp(result, "WARNING:", 8) == 0) {
        rc = INTEGRITY_WARNING;
        integrity_check_message("%s\n", result);
    }
    else {
        rc = INTEGRITY_ERROR;
        integrity_check_message("%s\n", result);
    }

    /* cleanup processing */
    failure:
    if (lua != NULL)
	rpmluaFree(lua);
    if (ts != NULL)
	(void)rpmtsFree(ts);
    ts = NULL;
    if (spec_iob != NULL)
	spec_iob = rpmiobFree(spec_iob);
    if (proc_iob != NULL)
	proc_iob = rpmiobFree(proc_iob);

    /* final result handling */
    if (rc != INTEGRITY_OK) {
        if (isatty(STDIN_FILENO) || isatty(STDOUT_FILENO))
            sleep(4);
        if (rc == INTEGRITY_ERROR)
            exit(42);
    }
    return;
}
#endif

/*@-bounds@*/ /* LCL: segfault */
/*@-mods@*/ /* FIX: shrug */
#if !defined(__GLIBC__) && !defined(__LCLINT__)
int main(int argc, const char ** argv, /*@unused@*/ char ** envp)
#else
int main(int argc, const char ** argv)
#endif
	/*@globals rpmEVR, RPMVERSION,
		rpmGlobalMacroContext, rpmCLIMacroContext,
		h_errno, fileSystem, internalState@*/
	/*@modifies fileSystem, internalState@*/
{
    poptContext optCon = rpmcliInit(argc, (char *const *)argv, optionsTable);

    rpmts ts = NULL;
    enum modes bigMode = MODE_UNKNOWN;

#if defined(IAM_RPMQV)
    QVA_t qva = &rpmQVKArgs;
#endif

#ifdef	IAM_RPMBT
    BTA_t ba = &rpmBTArgs;
#endif

#ifdef	IAM_RPMEIU
    QVA_t ia = &rpmIArgs;
#endif

#if defined(IAM_RPMDB)
    QVA_t da = &rpmDBArgs;
#endif

#if defined(IAM_RPMK)
    QVA_t ka = &rpmQVKArgs;
#endif

#if defined(IAM_RPMBT) || defined(IAM_RPMK)
    char * passPhrase = (char *) "";
#endif

    pid_t pipeChild = 0;
    int ec = 0;
    int status;
    int p[2];
#ifdef	IAM_RPMEIU
    int xx;
#endif
	
#if !defined(__GLIBC__) && !defined(__LCLINT__)
    environ = envp;
#else
/* XXX limit the fiddle up to linux for now. */
#if !defined(HAVE_SETPROCTITLE) && defined(__linux__)
    (void) initproctitle(argc, (char **)argv, environ);
#endif
#endif  

    /* Set the major mode based on argv[0] */
    /*@-nullpass@*/
#ifdef	IAM_RPMBT
    if (!strcmp(__progname, "rpmb"))	bigMode = MODE_BUILD;
    if (!strcmp(__progname, "lt-rpmb"))	bigMode = MODE_BUILD;
    if (!strcmp(__progname, "rpmt"))	bigMode = MODE_TARBUILD;
    if (!strcmp(__progname, "rpmbuild"))	bigMode = MODE_BUILD;
#endif
#ifdef	IAM_RPMQV
    if (!strcmp(__progname, "rpmq"))	bigMode = MODE_QUERY;
    if (!strcmp(__progname, "lt-rpmq"))	bigMode = MODE_QUERY;
    if (!strcmp(__progname, "rpmv"))	bigMode = MODE_VERIFY;
    if (!strcmp(__progname, "rpmquery"))	bigMode = MODE_QUERY;
    if (!strcmp(__progname, "rpmverify"))	bigMode = MODE_VERIFY;
#endif
#ifdef	RPMEIU
    if (!strcmp(__progname, "rpme"))	bigMode = MODE_ERASE;
    if (!strcmp(__progname, "rpmi"))	bigMode = MODE_INSTALL;
    if (!strcmp(__progname, "lt-rpmi"))	bigMode = MODE_INSTALL;
    if (!strcmp(__progname, "rpmu"))	bigMode = MODE_INSTALL;
#endif
    /*@=nullpass@*/

#if defined(IAM_RPMQV)
    /* Jumpstart option from argv[0] if necessary. */
    switch (bigMode) {
    case MODE_QUERY:	qva->qva_mode = 'q';	break;
    case MODE_VERIFY:	qva->qva_mode = 'V';	break;
    case MODE_CHECKSIG:	qva->qva_mode = 'K';	break;
    case MODE_RESIGN:	qva->qva_mode = 'R';	break;
    case MODE_INSTALL:
    case MODE_ERASE:
    case MODE_BUILD:
    case MODE_REBUILD:
    case MODE_RECOMPILE:
    case MODE_TARBUILD:
    case MODE_REBUILDDB:
    case MODE_UNKNOWN:
    default:
	break;
    }
#endif

    rpmcliConfigured();

#ifdef	IAM_RPMBT
    switch (ba->buildMode) {
    case 'b':	bigMode = MODE_BUILD;		break;
    case 't':	bigMode = MODE_TARBUILD;	break;
    case 'B':	bigMode = MODE_REBUILD;		break;
    case 'C':	bigMode = MODE_RECOMPILE;	break;
    }

    if ((ba->buildAmount & RPMBUILD_RMSOURCE) && bigMode == MODE_UNKNOWN)
	bigMode = MODE_BUILD;

    if ((ba->buildAmount & RPMBUILD_RMSPEC) && bigMode == MODE_UNKNOWN)
	bigMode = MODE_BUILD;
#endif	/* IAM_RPMBT */
    
#ifdef	IAM_RPMDB
  if (bigMode == MODE_UNKNOWN || (bigMode & MODES_DB)) {
    if (da->rebuild) {
	if (bigMode != MODE_UNKNOWN) 
	    argerror(_("only one major mode may be specified"));
	else
	    bigMode = MODE_REBUILDDB;
    }
  }
#endif	/* IAM_RPMDB */

#ifdef	IAM_RPMQV
  if (bigMode == MODE_UNKNOWN || (bigMode & MODES_QV)) {
    switch (qva->qva_mode) {
    case 'q':	bigMode = MODE_QUERY;		break;
    case 'V':	bigMode = MODE_VERIFY;		break;
    }

    if (qva->qva_sourceCount) {
	if (qva->qva_sourceCount > 2)
	    argerror(_("one type of query/verify may be performed at a "
			"time"));
    }
    if (qva->qva_flags && (bigMode & ~MODES_QV)) 
	argerror(_("unexpected query flags"));

    if (qva->qva_queryFormat && (bigMode & ~MODES_QV)) 
	argerror(_("unexpected query format"));

    if (qva->qva_source != RPMQV_PACKAGE && (bigMode & ~MODES_QV)) 
	argerror(_("unexpected query source"));
  }
#endif	/* IAM_RPMQV */

#ifdef	IAM_RPMEIU
  if (bigMode == MODE_UNKNOWN || (bigMode & MODES_IE))
    {	int iflags = (ia->installInterfaceFlags &
		(INSTALL_UPGRADE|INSTALL_FRESHEN|INSTALL_INSTALL));
	int eflags = (ia->installInterfaceFlags & INSTALL_ERASE);

	if (iflags & eflags)
	    argerror(_("only one major mode may be specified"));
	else if (iflags)
	    bigMode = MODE_INSTALL;
	else if (eflags)
	    bigMode = MODE_ERASE;
    }
#endif	/* IAM_RPMEIU */

#ifdef	IAM_RPMK
  if (bigMode == MODE_UNKNOWN || (bigMode & MODES_K)) {
	switch (ka->qva_mode) {
	case RPMSIGN_NONE:
	    ka->sign = 0;
	    break;
	case RPMSIGN_IMPORT_PUBKEY:
	case RPMSIGN_CHK_SIGNATURE:
	    bigMode = MODE_CHECKSIG;
	    ka->sign = 0;
	    break;
	case RPMSIGN_ADD_SIGNATURE:
	case RPMSIGN_NEW_SIGNATURE:
	case RPMSIGN_DEL_SIGNATURE:
	    bigMode = MODE_RESIGN;
	    ka->sign = (ka->qva_mode != RPMSIGN_DEL_SIGNATURE);
	    break;
	}
  }
#endif	/* IAM_RPMK */

#if defined(IAM_RPMEIU)
    if (!( bigMode == MODE_INSTALL ) &&
(ia->probFilter & (RPMPROB_FILTER_REPLACEPKG | RPMPROB_FILTER_OLDPACKAGE)))
	argerror(_("only installation, upgrading, rmsource and rmspec may be forced"));
    if (bigMode != MODE_INSTALL && (ia->probFilter & RPMPROB_FILTER_FORCERELOCATE))
	argerror(_("files may only be relocated during package installation"));

    if (ia->relocations && ia->qva_prefix)
	argerror(_("cannot use --prefix with --relocate or --excludepath"));

    if (bigMode != MODE_INSTALL && ia->relocations)
	argerror(_("--relocate and --excludepath may only be used when installing new packages"));

    if (bigMode != MODE_INSTALL && ia->qva_prefix)
	argerror(_("--prefix may only be used when installing new packages"));

    if (ia->qva_prefix && ia->qva_prefix[0] != '/') 
	argerror(_("arguments to --prefix must begin with a /"));

    if (bigMode != MODE_INSTALL && (ia->installInterfaceFlags & INSTALL_HASH))
	argerror(_("--hash (-h) may only be specified during package "
			"installation"));

    if (bigMode != MODE_INSTALL && (ia->installInterfaceFlags & INSTALL_PERCENT))
	argerror(_("--percent may only be specified during package "
			"installation"));

    if (bigMode != MODE_INSTALL && (ia->probFilter & RPMPROB_FILTER_REPLACEPKG))
	argerror(_("--replacepkgs may only be specified during package "
			"installation"));

    if (bigMode != MODE_INSTALL && (ia->transFlags & RPMTRANS_FLAG_NODOCS))
	argerror(_("--excludedocs may only be specified during package "
		   "installation"));

    if (bigMode != MODE_INSTALL && ia->incldocs)
	argerror(_("--includedocs may only be specified during package "
		   "installation"));

    if (ia->incldocs && (ia->transFlags & RPMTRANS_FLAG_NODOCS))
	argerror(_("only one of --excludedocs and --includedocs may be "
		 "specified"));
  
    if (bigMode != MODE_INSTALL && (ia->probFilter & RPMPROB_FILTER_IGNOREARCH))
	argerror(_("--ignorearch may only be specified during package "
		   "installation"));

    if (bigMode != MODE_INSTALL && (ia->probFilter & RPMPROB_FILTER_IGNOREOS))
	argerror(_("--ignoreos may only be specified during package "
		   "installation"));

    if ((ia->installInterfaceFlags & INSTALL_ALLMATCHES) && bigMode != MODE_ERASE)
	argerror(_("--allmatches may only be specified during package "
		   "erasure"));

    if ((ia->transFlags & RPMTRANS_FLAG_ALLFILES) && bigMode != MODE_INSTALL)
	argerror(_("--allfiles may only be specified during package "
		   "installation"));

    if ((ia->transFlags & RPMTRANS_FLAG_JUSTDB) &&
	bigMode != MODE_INSTALL && bigMode != MODE_ERASE)
	argerror(_("--justdb may only be specified during package "
		   "installation and erasure"));

    if (bigMode != MODE_INSTALL && bigMode != MODE_ERASE &&
	(ia->transFlags & (RPMTRANS_FLAG_NOSCRIPTS | _noTransScripts | _noTransTriggers)))
	argerror(_("script disabling options may only be specified during "
		   "package installation and erasure"));

    if (bigMode != MODE_INSTALL && bigMode != MODE_ERASE &&
	(ia->transFlags & (RPMTRANS_FLAG_NOTRIGGERS | _noTransTriggers)))
	argerror(_("trigger disabling options may only be specified during "
		   "package installation and erasure"));

    if (ia->noDeps & (bigMode & ~MODES_FOR_NODEPS))
	argerror(_("--nodeps may only be specified during package "
		   "building, rebuilding, recompilation, installation, "
		   "erasure, and verification"));

    if ((ia->transFlags & RPMTRANS_FLAG_TEST) && (bigMode & ~MODES_FOR_TEST))
	argerror(_("--test may only be specified during package installation, "
		 "erasure, and building"));
#endif	/* IAM_RPMEIU */

    if (rpmioRootDir && rpmioRootDir[1] && (bigMode & ~MODES_FOR_ROOT))
	argerror(_("--root (-r) may only be specified during "
		 "installation, erasure, querying, and "
		 "database rebuilds"));

    if (rpmioRootDir) {
	switch (urlIsURL(rpmioRootDir)) {
	default:
	    if (bigMode & MODES_FOR_ROOT)
		break;
	    /*@fallthrough@*/
	case URL_IS_UNKNOWN:
	    if (rpmioRootDir[0] != '/')
		argerror(_("arguments to --root (-r) must begin with a /"));
	    break;
	}
    }

#if defined(RPM_VENDOR_OPENPKG) /* integrity-checking */
    integrity_check(__progname, bigMode);
#endif

#if defined(IAM_RPMBT) || defined(IAM_RPMK)
    if (0
#if defined(IAM_RPMBT)
    || ba->sign 
#endif
#if defined(IAM_RPMK)
    || ka->sign
#endif
    )
    /*@-branchstate@*/
    {
        if (bigMode == MODE_REBUILD || bigMode == MODE_BUILD ||
	    bigMode == MODE_RESIGN || bigMode == MODE_TARBUILD)
	{
	    const char ** av;
	    struct stat sb;
	    int errors = 0;

	    if ((av = poptGetArgs(optCon)) == NULL) {
		fprintf(stderr, _("no files to sign\n"));
		errors++;
	    } else
	    while (*av) {
		if (Stat(*av, &sb)) {
		    fprintf(stderr, _("cannot access file %s\n"), *av);
		    errors++;
		}
		av++;
	    }

	    if (errors) {
		ec = errors;
		goto exit;
	    }

	
            if (poptPeekArg(optCon)
#if defined(IAM_RPMBT)
	    && !ba->nopassword 
#endif
#if defined(IAM_RPMK)
	    && !ka->nopassword
#endif
	    )
	    {
		passPhrase = Getpass(_("Enter pass phrase: "));
		if (rpmCheckPassPhrase(passPhrase)) {
		    fprintf(stderr, _("Pass phrase check failed\n"));
		    ec = EXIT_FAILURE;
		    goto exit;
		}
		fprintf(stderr, _("Pass phrase is good.\n"));
		/* XXX Getpass() should realloc instead. */
		passPhrase = xstrdup(passPhrase);
	    }
	}
    }
    /*@=branchstate@*/
#endif	/* IAM_RPMBT || IAM_RPMK */

    if (rpmioPipeOutput) {
	if (pipe(p) < 0) {
	    fprintf(stderr, _("creating a pipe for --pipe failed: %m\n"));
	    goto exit;
	}

	if (!(pipeChild = fork())) {
	    (void) close(p[1]);
	    (void) dup2(p[0], STDIN_FILENO);
	    (void) close(p[0]);
	    (void) execl("/bin/sh", "/bin/sh", "-c", rpmioPipeOutput, NULL);
	    fprintf(stderr, _("exec failed\n"));
	}

	(void) close(p[0]);
	(void) dup2(p[1], STDOUT_FILENO);
	(void) close(p[1]);
    }
	
    ts = rpmtsCreate();
    (void) rpmtsSetRootDir(ts, rpmioRootDir);
    switch (bigMode) {
#ifdef	IAM_RPMDB
    case MODE_REBUILDDB:
    {   rpmVSFlags vsflags;
	rpmVSFlags ovsflags;

	vsflags = (rpmVSFlags) rpmExpandNumeric("%{?_vsflags_rebuilddb}");
	vsflags = (rpmVSFlags) 0; /* XXX FIXME: ignore default disablers. */
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
	VSF_CLR(vsflags, NEEDPAYLOAD);
#endif

	ovsflags = rpmtsSetVSFlags(ts, vsflags);
	ec = rpmtsRebuildDB(ts);
	vsflags = rpmtsSetVSFlags(ts, ovsflags);
    }	break;
#endif	/* IAM_RPMDB */

#ifdef	IAM_RPMBT
    case MODE_REBUILD:
    case MODE_RECOMPILE:
    {	const char * pkg;
	int nbuilds = 0;

        while (!rpmIsVerbose())
	    rpmIncreaseVerbosity();

	if (!poptPeekArg(optCon))
	    argerror(_("no packages files given for rebuild"));

	ba->buildAmount =
	    RPMBUILD_PREP | RPMBUILD_BUILD | RPMBUILD_INSTALL | RPMBUILD_CHECK;
	if (bigMode == MODE_REBUILD) {
	    ba->buildAmount |= RPMBUILD_PACKAGEBINARY;
	    ba->buildAmount |= RPMBUILD_RMSOURCE;
	    ba->buildAmount |= RPMBUILD_RMSPEC;
	    ba->buildAmount |= RPMBUILD_CLEAN;
	    ba->buildAmount |= RPMBUILD_RMBUILD;
	}

	while ((pkg = poptGetArg(optCon))) {
	    if (nbuilds++ > 0) {
		rpmFreeMacros(NULL);
		rpmFreeRpmrc();
		(void) rpmReadConfigFiles(NULL, NULL);
	    }
	    ba->specFile = NULL;
	    ba->cookie = NULL;
	    ec = rpmInstallSource(ts, pkg, &ba->specFile, &ba->cookie);
	    if (ec == 0) {
		ba->rootdir = rpmioRootDir;
		ba->passPhrase = passPhrase;
		ec = build(ts, ba, NULL);
	    }
	    ba->cookie = _free(ba->cookie);
	    ba->specFile = _free(ba->specFile);

	    if (ec)
		/*@loopbreak@*/ break;
	}

    }	break;

    case MODE_BUILD:
    case MODE_TARBUILD:
    {	int nbuilds = 0;

#if defined(RPM_VENDOR_OPENPKG) /* no-auto-verbose-increase-for-track-and-fetch */
	if (ba->buildChar != 't' && ba->buildChar != 'f')
#endif
        while (!rpmIsVerbose())
	    rpmIncreaseVerbosity();
       
	switch (ba->buildChar) {
	case 'a':
	    ba->buildAmount |= RPMBUILD_PACKAGESOURCE;
	    /*@fallthrough@*/
	case 'b':
	    ba->buildAmount |= RPMBUILD_PACKAGEBINARY;
	    ba->buildAmount |= RPMBUILD_CLEAN;
#if defined(RPM_VENDOR_MANDRIVA)
	    if ((ba->buildChar == 'a' || ba->buildChar == 'b') && ba->shortCircuit)
#else
	    if ((ba->buildChar == 'b') && ba->shortCircuit)
#endif
		/*@innerbreak@*/ break;
	    /*@fallthrough@*/
	case 'i':
	    ba->buildAmount |= RPMBUILD_INSTALL;
	    ba->buildAmount |= RPMBUILD_CHECK;
	    if ((ba->buildChar == 'i') && ba->shortCircuit)
		/*@innerbreak@*/ break;
	    /*@fallthrough@*/
	case 'c':
	    ba->buildAmount |= RPMBUILD_BUILD;
	    if ((ba->buildChar == 'c') && ba->shortCircuit)
		/*@innerbreak@*/ break;
	    /*@fallthrough@*/
	case 'p':
	    ba->buildAmount |= RPMBUILD_PREP;
	    /*@innerbreak@*/ break;
	    
	case 'l':
	    ba->buildAmount |= RPMBUILD_FILECHECK;
	    /*@innerbreak@*/ break;
	case 's':
	    ba->buildAmount |= RPMBUILD_PACKAGESOURCE;
#if defined(RPM_VENDOR_OPENPKG) || defined(RPM_VENDOR_MANDRIVA) || defined(RPM_VENDOR_ARK) /* no-deps-on-building-srpms */
	    /* enforce no dependency checking when rolling a source RPM */
	    ba->noDeps = 1;
#endif
	    /*@innerbreak@*/ break;
	case 't':	/* support extracting the "%track" script/section */
	    ba->buildAmount |= RPMBUILD_TRACK;
	    /* enforce no dependency checking and expansion of %setup, %patch and %prep macros */
	    ba->noDeps = 1;
	    rpmDefineMacro(NULL, "setup #", RMIL_CMDLINE);
	    rpmDefineMacro(NULL, "patch #", RMIL_CMDLINE);
	    rpmDefineMacro(NULL, "prep %%prep", RMIL_CMDLINE);
	    /*@innerbreak@*/ break;
	case 'f':
	    ba->buildAmount |= RPMBUILD_FETCHSOURCE;
	    ba->noDeps = 1;
	    /*@innerbreak@*/ break;
	}

	if (!poptPeekArg(optCon)) {
	    if (bigMode == MODE_BUILD)
		argerror(_("no spec files given for build"));
	    else
		argerror(_("no tar files given for build"));
	}

	while ((ba->specFile = poptGetArg(optCon))) {
	    if (nbuilds++ > 0) {
		rpmFreeMacros(NULL);
		rpmFreeRpmrc();
		(void) rpmReadConfigFiles(NULL, NULL);
	    }
	    ba->rootdir = rpmioRootDir;
	    ba->passPhrase = passPhrase;
	    ba->cookie = NULL;
	    ec = build(ts, ba, NULL);
	    if (ec)
		/*@loopbreak@*/ break;
	}
    }	break;
#endif	/* IAM_RPMBT */

#ifdef	IAM_RPMEIU
    case MODE_ERASE:
	ia->depFlags = (rpmdepFlags) global_depFlags;
	if (ia->noDeps) IIF_SET(ia->installInterfaceFlags, NODEPS);

	if (!poptPeekArg(optCon)) {
	    if (ia->rbtid == 0)
		argerror(_("no packages given for erase"));
TSF_SET(ia->transFlags, NOFDIGESTS);
PFF_SET(ia->probFilter, OLDPACKAGE);
ia->rbCheck = rpmcliInstallCheck;
ia->rbOrder = rpmcliInstallOrder;
ia->rbRun = rpmcliInstallRun;
	    ec += rpmRollback(ts, ia, NULL);
	} else {
	    ec += rpmErase(ts, ia, (const char **) poptGetArgs(optCon));
	}
	break;

    case MODE_INSTALL:

	/* RPMTRANS_FLAG_KEEPOBSOLETE */

	ia->depFlags = (rpmdepFlags) global_depFlags;
	if (!ia->incldocs) {
	    if (ia->transFlags & RPMTRANS_FLAG_NODOCS) {
		;
	    } else if (rpmExpandNumeric("%{_excludedocs}"))
		TSF_SET(ia->transFlags, NODOCS);
	}

	if (ia->noDeps) IIF_SET(ia->installInterfaceFlags, NODEPS);

	/* we've already ensured !(!ia->prefix && !ia->relocations) */
	/*@-branchstate@*/
	if (ia->qva_prefix) {
	    xx = rpmfiAddRelocation(&ia->relocations, &ia->nrelocations,
			NULL, ia->qva_prefix);
	    xx = rpmfiAddRelocation(&ia->relocations, &ia->nrelocations,
			NULL, NULL);
	} else if (ia->relocations) {
	    xx = rpmfiAddRelocation(&ia->relocations, &ia->nrelocations,
			NULL, NULL);
	}
	/*@=branchstate@*/

	if (!poptPeekArg(optCon)) {
	    if (ia->rbtid == 0)
		argerror(_("no packages given for install"));
TSF_SET(ia->transFlags, NOFDIGESTS);
PFF_SET(ia->probFilter, OLDPACKAGE);
ia->rbCheck = rpmcliInstallCheck;
ia->rbOrder = rpmcliInstallOrder;
ia->rbRun = rpmcliInstallRun;
/*@i@*/	    ec += rpmRollback(ts, ia, NULL);
	} else {
	    /*@-compdef -compmempass@*/ /* FIX: ia->relocations[0].newPath undefined */
	    ec += rpmcliInstall(ts, ia, (const char **)poptGetArgs(optCon));
	    /*@=compdef =compmempass@*/
	}
	break;

#endif	/* IAM_RPMEIU */

#ifdef	IAM_RPMQV
    case MODE_QUERY:
	if (!poptPeekArg(optCon)
	 && !(qva->qva_source == RPMQV_ALL || qva->qva_source == RPMQV_HDLIST))
	    argerror(_("no arguments given for query"));

	qva->depFlags = (rpmdepFlags) global_depFlags;
	qva->qva_specQuery = rpmspecQuery;
	ec = rpmcliQuery(ts, qva, (const char **) poptGetArgs(optCon));
	qva->qva_specQuery = NULL;
	break;

    case MODE_VERIFY:
    {	rpmVerifyFlags vflags = (rpmVerifyFlags) ~RPMVERIFY_NONE;
	unsigned ui;

	qva->depFlags = (rpmdepFlags) global_depFlags;
	ui = (unsigned) vflags;
	ui &= ~((unsigned)qva->qva_flags);
	qva->qva_flags = (rpmQueryFlags) ui;

	if (!poptPeekArg(optCon)
	 && !(qva->qva_source == RPMQV_ALL || qva->qva_source == RPMQV_HDLIST))
	    argerror(_("no arguments given for verify"));
	ec = rpmcliVerify(ts, qva, (const char **) poptGetArgs(optCon));
    }	break;
#endif	/* IAM_RPMQV */

#ifdef IAM_RPMK
    case MODE_CHECKSIG:
    {	rpmVerifyFlags vflags = (rpmVerifyFlags) 0;
	unsigned ui;

	QVA_SET(vflags, FDIGEST);
	QVA_SET(vflags, HDRCHK);
	QVA_SET(vflags, DIGEST);
	QVA_SET(vflags, SIGNATURE);
	ui = (unsigned) vflags;
	ui &= ~((unsigned)ka->qva_flags);
	ka->qva_flags = (rpmQueryFlags) ui;
    }   /*@fallthrough@*/
    case MODE_RESIGN:
	if (!poptPeekArg(optCon))
	    argerror(_("no arguments given"));
	ka->passPhrase = passPhrase;
	ec = rpmcliSign(ts, ka, (const char **)poptGetArgs(optCon));
    	break;
#endif	/* IAM_RPMK */
	
#if !defined(IAM_RPMQV)
    case MODE_QUERY:
    case MODE_VERIFY:
#endif
#if !defined(IAM_RPMK)
    case MODE_CHECKSIG:
    case MODE_RESIGN:
#endif
#if !defined(IAM_RPMDB)
    case MODE_REBUILDDB:
#endif
#if !defined(IAM_RPMBT)
    case MODE_BUILD:
    case MODE_REBUILD:
    case MODE_RECOMPILE:
    case MODE_TARBUILD:
#endif
#if !defined(IAM_RPMEIU)
    case MODE_INSTALL:
    case MODE_ERASE:
#endif
    case MODE_UNKNOWN:
#ifdef	DYING /* XXX rpmIsVerbose alone stops usage spewage with every --eval */
	if (poptPeekArg(optCon) != NULL || argc <= 1 || rpmIsVerbose()) {
	    printUsage(optCon, stderr, 0);
	    ec = argc;
	}
#endif
	break;
    }

#if defined(IAM_RPMBT) || defined(IAM_RPMK)
exit:
#endif	/* IAM_RPMBT || IAM_RPMK */

    (void)rpmtsFree(ts); 
    ts = NULL;

    if (pipeChild) {
	(void) fclose(stdout);
	(void) waitpid(pipeChild, &status, 0);
    }

#ifdef	IAM_RPMQV
    qva->qva_queryFormat = _free(qva->qva_queryFormat);
#endif

#ifdef	IAM_RPMBT
    freeNames();
    /* XXX _specPool/_pkgPool teardown should be done somewhere else. */
    {	extern rpmioPool _pkgPool;
	extern rpmioPool _specPool;
	_pkgPool = rpmioFreePool(_pkgPool);
	_specPool = rpmioFreePool(_specPool);
    }
#endif

#ifdef	IAM_RPMEIU
    ia->relocations = rpmfiFreeRelocations(ia->relocations);
#endif

    optCon = rpmcliFini(optCon);

/* XXX limit the fiddle up to linux for now. */
#if !defined(HAVE_SETPROCTITLE) && defined(__linux__)
    (void) finiproctitle();
#endif

    /* XXX don't overflow single byte exit status */
    /* XXX status 255 is special to xargs(1) */
    if (ec > 254) ec = 254;

     rpmlog(RPMLOG_DEBUG, D_("exit code: %d\n"), ec);

    /*@-globstate@*/
    return ec;
    /*@=globstate@*/
}
/*@=mods@*/
/*@=bounds@*/
