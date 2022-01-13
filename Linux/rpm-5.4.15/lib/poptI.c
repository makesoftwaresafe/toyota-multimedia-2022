/** \ingroup rpmcli
 * \file lib/poptI.c
 *  Popt tables for install modes.
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmlog.h>

#include <rpmtag.h>
#include <rpmcli.h>

#include "debug.h"

/*@-redecl@*/
extern time_t get_date(const char * p, void * now);	/* XXX expedient lies */
/*@=redecl@*/

/*@-fullinitblock@*/
/*@unchecked@*/
struct rpmQVKArguments_s rpmIArgs = {
#if defined(RPM_VENDOR_MANDRIVA) /* dont-filter-install-file-conflicts */
    .probFilter = RPMPROB_FILTER_NONE,
#else
    .probFilter = (RPMPROB_FILTER_REPLACEOLDFILES | RPMPROB_FILTER_REPLACENEWFILES),
#endif
};
/*@=fullinitblock@*/

#define	POPT_RELOCATE		-1021
#define	POPT_EXCLUDEPATH	-1022
#define	POPT_ROLLBACK		-1023
#define	POPT_ROLLBACK_EXCLUDE	-1024
/* -1025 thrugh -1033 are common in rpmcli.h. */
#define	POPT_AUTOROLLBACK_GOAL	-1036

#define	alloca_strdup(_s)	strcpy((char *)alloca(strlen(_s)+1), (_s))

/**
 * Print a message and exit.
 * @todo (CLI embedding) Use rpmlog/rpmlog instead of fprintf, remove exit.
 * @param desc		message	
 */
static void argerror(const char * desc) __attribute__((__noreturn__));
/*@exits@*/
static void argerror(const char * desc)
	/*@globals stderr, fileSystem @*/
	/*@modifies stderr, fileSystem @*/
{
    /*@-modfilesys -globs @*/
    fprintf(stderr, _("%s: %s\n"), __progname, desc);
    /*@=modfilesys =globs @*/
    exit(EXIT_FAILURE);
}

/**
 */
static void installArgCallback(/*@unused@*/ poptContext con,
		/*@unused@*/ enum poptCallbackReason reason,
		const struct poptOption * opt, const char * arg,
		/*@unused@*/ const void * data)
	/*@globals rpmIArgs, stderr @*/
	/*@modifies rpmIArgs, stderr @*/
{
    QVA_t ia = &rpmIArgs;
    int xx;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {

    case 'i':
	ia->installInterfaceFlags = (rpmInstallInterfaceFlags)
		(ia->installInterfaceFlags | INSTALL_INSTALL);
	break;

    case POPT_EXCLUDEPATH:
	if (arg == NULL || *arg != '/') 
	    argerror(_("exclude paths must begin with a /"));
	xx = rpmfiAddRelocation(&ia->relocations, &ia->nrelocations, arg, NULL);
	break;
    case POPT_RELOCATE:
      { char * oldPath = NULL;
	char * newPath = NULL;
	
	if (arg == NULL) 
	    argerror(_("Option --relocate needs /old/path=/new/path argument"));
	if (*arg != '/') 
	    argerror(_("relocations must begin with a /"));
	oldPath = xstrdup(arg);
	if (!(newPath = strchr(oldPath, '=')))
	    argerror(_("relocations must contain a ="));
	*newPath++ = '\0';
	if (*newPath != '/') 
	    argerror(_("relocations must have a / following the ="));
	xx = rpmfiAddRelocation(&ia->relocations, &ia->nrelocations,
			oldPath, newPath);
	oldPath = _free(oldPath);
      }	break;

    case POPT_ROLLBACK_EXCLUDE:
    {	rpmuint32_t tid;
	char *t, *te;

	/* Make sure we were given the proper number of args */
	if (arg == NULL)
	    argerror(_("Option --rbexclude needs transaction id argument(s)"));

	te = alloca_strdup(arg);
	while (*te != '\0' && strchr(" \t\n,", *te) != NULL)
	    *te++ = '\0';
	while ((t = te++) != NULL && *t != '\0') {
	    /* Find next tid. */
	    while (*te != '\0' && strchr(" \t\n,", *te) == NULL)
		te++;
	    while (*te != '\0' && strchr(" \t\n,", *te) != NULL)
		*te++ = '\0';

	    /* Convert arg to TID which happens to be time_t */
	    /* XXX: Need check for arg to be an integer      */
	    tid = (rpmuint32_t) strtol(t, NULL, 0);

	    /* Allocate space for new exclude tid */
	    ia->rbtidExcludes = (rpmuint32_t *) xrealloc(ia->rbtidExcludes, 
		sizeof(*ia->rbtidExcludes) * (ia->numrbtidExcludes + 1));

	    /* Add it to the list and iterate count*/
/*@-temptrans@*/
	    ia->rbtidExcludes[ia->numrbtidExcludes] = tid;
/*@=temptrans@*/
	    ia->numrbtidExcludes++;
	}
    } break;

    case POPT_ROLLBACK:
      {	time_t tid;
	if (arg == NULL)
	    argerror(_("Option --rollback needs a time/date stamp argument"));

	/*@-moduncon@*/
	tid = get_date(arg, NULL);
	rpmlog(RPMLOG_INFO, _("Rollback goal:  %-24.24s (0x%08x)\n"), ctime(&tid), (int)tid);
	/*@=moduncon@*/

	if (tid == (time_t)-1 || tid == (time_t)0)
	    argerror(_("malformed rollback time/date stamp argument"));
	ia->rbtid = (rpmuint32_t)tid;
      }	break;
    
    case POPT_AUTOROLLBACK_GOAL:
      {	time_t tid;
	if (arg == NULL)
	    argerror(_("arbgoal takes a time/date stamp argument"));

	/*@-moduncon@*/
	tid = get_date(arg, NULL);
	/*@=moduncon@*/

	if (tid == (time_t)-1 || tid == (time_t)0)
	    argerror(_("malformed arbgoal time/date stamp argument"));
	ia->arbtid = (rpmuint32_t)tid;
      }	break;

    case RPMCLI_POPT_NODIGEST:
	ia->qva_flags = (rpmQueryFlags) (ia->qva_flags | VERIFY_DIGEST);
	break;

    case RPMCLI_POPT_NOSIGNATURE:
	ia->qva_flags = (rpmQueryFlags) (ia->qva_flags | VERIFY_SIGNATURE);
	break;

    case RPMCLI_POPT_NOHDRCHK:
	ia->qva_flags = (rpmQueryFlags) (ia->qva_flags | VERIFY_HDRCHK);
	break;

    case RPMCLI_POPT_NODEPS:
	ia->noDeps = 1;
	break;

    case RPMCLI_POPT_NOCONTEXTS:
	ia->transFlags = (rpmtransFlags)
		(ia->transFlags | RPMTRANS_FLAG_NOCONTEXTS);
	break;

    case RPMCLI_POPT_NOSCRIPTS:
	ia->transFlags = (rpmtransFlags)
		(ia->transFlags | (_noTransScripts|_noTransTriggers));
	break;

    case RPMCLI_POPT_NOFDIGESTS:
	ia->transFlags = (rpmtransFlags)
		(ia->transFlags | RPMTRANS_FLAG_NOFDIGESTS);
	break;

    }
}

/**
 */
/*@-bitwisesigned -compmempass @*/
/*@unchecked@*/
struct poptOption rpmInstallPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	(void *) installArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "allfiles", '\0', POPT_BIT_SET,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_ALLFILES,
  N_("install all files, even configurations which might otherwise be skipped"),
	NULL},
 { "apply", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &rpmIArgs.transFlags,
	(_noTransScripts|_noTransTriggers|
		RPMTRANS_FLAG_APPLYONLY|RPMTRANS_FLAG_PKGCOMMIT),
	N_("do not execute package scriptlet(s)"), NULL },
 { "dirstash", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_DIRSTASH,
	N_("save erased package files by renaming into sub-directory"), NULL},
 { "excludeconfigs", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOCONFIGS,
	N_("do not install configuration files"), NULL},
 { "excludedocs", '\0', POPT_BIT_SET,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NODOCS,
	N_("do not install documentation"), NULL},
 { "justdb", '\0', POPT_BIT_SET, &rpmIArgs.transFlags, RPMTRANS_FLAG_JUSTDB,
	N_("update the database, but do not modify the filesystem"), NULL},
 { "noconfigs", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOCONFIGS,
	N_("do not install configuration files"), NULL},
 { "nodocs", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NODOCS,
	N_("do not install documentation"), NULL},
#ifdef	NOTYET	/* XXX multiple identical options forces popt callback. */
 { "nocontexts", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOCONTEXTS,
	N_("don't install file security contexts"), NULL},
 { "nofdigests", '\0', POPT_BIT_SET,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOFDIGESTS,
	N_("don't verify file digests"), NULL },
#else
  { "nocontexts", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, RPMCLI_POPT_NOCONTEXTS,
	N_("don't verify file security contexts"), NULL },
  { "nofdigests", '\0', 0, NULL, RPMCLI_POPT_NOFDIGESTS,
	N_("don't verify file digests"), NULL },
#endif
 { "norpmdb", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NORPMDB,
	N_("don't register headers in rpmdb"), NULL},

#ifdef	NOTYET	/* XXX multiple identical options forces popt callback. */
 { "noscripts", '\0', POPT_BIT_SET,
	&rpmIArgs.transFlags, (_noTransScripts|_noTransTriggers),
	N_("do not execute package scriptlet(s)"), NULL },
#else
  { "noscripts", '\0', 0, NULL, RPMCLI_POPT_NOSCRIPTS,
	N_("do not execute package scriptlet(s)"), NULL },
#endif
 { "nopretrans", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOPRETRANS,
	N_("do not execute %%pretrans scriptlet (if any)"), NULL },
 { "nopre", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOPRE,
	N_("do not execute %%pre scriptlet (if any)"), NULL },
 { "nopost", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOPOST,
	N_("do not execute %%post scriptlet (if any)"), NULL },
 { "nopreun", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOPREUN,
	N_("do not execute %%preun scriptlet (if any)"), NULL },
 { "nopostun", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOPOSTUN,
	N_("do not execute %%postun scriptlet (if any)"), NULL },
 { "noposttrans", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOPOSTTRANS,
	N_("do not execute %%postrans scriptlet (if any)"), NULL },

 { "notriggers", '\0', POPT_BIT_SET, &rpmIArgs.transFlags, _noTransTriggers,
	N_("do not execute any scriptlet(s) triggered by this package"), NULL},
 { "notriggerprein", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOTRIGGERPREIN,
	N_("do not execute any %%triggerprein scriptlet(s)"), NULL},
 { "notriggerin", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOTRIGGERIN,
	N_("do not execute any %%triggerin scriptlet(s)"), NULL},
 { "notriggerun", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOTRIGGERUN,
	N_("do not execute any %%triggerun scriptlet(s)"), NULL},
 { "notriggerpostun", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_NOTRIGGERPOSTUN,
	N_("do not execute any %%triggerpostun scriptlet(s)"), NULL},

 { "repackage", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,
	&rpmIArgs.transFlags, RPMTRANS_FLAG_REPACKAGE,
	N_("save erased package files by repackaging"), NULL},
 { "test", '\0', POPT_BIT_SET, &rpmIArgs.transFlags, RPMTRANS_FLAG_TEST,
	N_("don't install, but tell if it would work or not"), NULL},

 { "allmatches", '\0', POPT_BIT_SET,
	&rpmIArgs.installInterfaceFlags, INSTALL_ALLMATCHES,
	N_("remove all packages which match <package> (normally an error is generated if <package> specified multiple packages)"),
	NULL},

 { "badreloc", '\0', POPT_BIT_SET,
	&rpmIArgs.probFilter, RPMPROB_FILTER_FORCERELOCATE,
	N_("relocate files in non-relocatable package"), NULL},

 { "erase", 'e', POPT_BIT_SET,
	&rpmIArgs.installInterfaceFlags, INSTALL_ERASE,
	N_("erase (uninstall) package"), N_("<package>+") },
 { "excludepath", '\0', POPT_ARG_STRING, NULL, POPT_EXCLUDEPATH,
	N_("skip files with leading component <path> "),
	N_("<path>") },

 { "fileconflicts", '\0', POPT_BIT_CLR, &rpmIArgs.probFilter,
	(RPMPROB_FILTER_REPLACEOLDFILES | RPMPROB_FILTER_REPLACENEWFILES),
	N_("detect file conflicts between packages"), NULL},

 { "freshen", 'F', POPT_BIT_SET, &rpmIArgs.installInterfaceFlags,
	(INSTALL_UPGRADE|INSTALL_FRESHEN|INSTALL_INSTALL),
	N_("upgrade package(s) if already installed"),
	N_("<packagefile>+") },
 { "hash", 'h', POPT_BIT_SET, &rpmIArgs.installInterfaceFlags, INSTALL_HASH,
	N_("print hash marks as package installs (good with -v)"), NULL},
#ifndef	DIEDIEDIE
 { "ignorearch", '\0', POPT_BIT_SET,
	&rpmIArgs.probFilter, RPMPROB_FILTER_IGNOREARCH,
	N_("don't verify package architecture"), NULL},
 { "ignoreos", '\0', POPT_BIT_SET,
	&rpmIArgs.probFilter, RPMPROB_FILTER_IGNOREOS,
	N_("don't verify package operating system"), NULL},
#endif
 { "ignoresize", '\0', POPT_BIT_SET, &rpmIArgs.probFilter,
	(RPMPROB_FILTER_DISKSPACE|RPMPROB_FILTER_DISKNODES),
	N_("don't check disk space before installing"), NULL},
 { "includedocs", '\0', POPT_ARGFLAG_DOC_HIDDEN, &rpmIArgs.incldocs, 0,
	N_("install documentation"), NULL},

 { "install", 'i', 0, NULL, 'i',
	N_("install package(s)"), N_("<packagefile>+") },

 { "nodeps", '\0', 0, NULL, RPMCLI_POPT_NODEPS,
	N_("do not verify package dependencies"), NULL },

 { "noorder", '\0', POPT_BIT_SET,
	&rpmIArgs.installInterfaceFlags, INSTALL_NOORDER,
	N_("do not reorder package installation to satisfy dependencies"),
	NULL},

 { "nodigest", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, RPMCLI_POPT_NODIGEST,
	N_("don't verify package digest(s)"), NULL },
 { "nohdrchk", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, RPMCLI_POPT_NOHDRCHK,
	N_("don't verify database header(s) when retrieved"), NULL },
 { "nosignature", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, RPMCLI_POPT_NOSIGNATURE,
	N_("don't verify package signature(s)"), NULL },

 { "oldpackage", '\0', POPT_BIT_SET,
	&rpmIArgs.probFilter, RPMPROB_FILTER_OLDPACKAGE,
	N_("upgrade to an old version of the package (--force on upgrades does this automatically)"),
	NULL},
 { "percent", '\0', POPT_BIT_SET,
	&rpmIArgs.installInterfaceFlags, INSTALL_PERCENT,
	N_("print percentages as package installs"), NULL},
 { "prefix", '\0', POPT_ARG_STRING, &rpmIArgs.qva_prefix, 0,
	N_("relocate the package to <dir>, if relocatable"),
	N_("<dir>") },
 { "relocate", '\0', POPT_ARG_STRING, NULL, POPT_RELOCATE,
	N_("relocate files from path <old> to <new>"),
	N_("<old>=<new>") },
 { "replacefiles", '\0', POPT_BIT_SET, &rpmIArgs.probFilter,
	(RPMPROB_FILTER_REPLACEOLDFILES | RPMPROB_FILTER_REPLACENEWFILES),
	N_("ignore file conflicts between packages"), NULL},
 { "replacepkgs", '\0', POPT_BIT_SET,
	&rpmIArgs.probFilter, RPMPROB_FILTER_REPLACEPKG,
	N_("reinstall if the package is already present"), NULL},
 { "rollback", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_ROLLBACK,
	N_("deinstall new, reinstall old, package(s), back to <date>"),
	N_("<date>") },
 { "arbgoal", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_AUTOROLLBACK_GOAL,
	N_("If transaction fails rollback to <date>"),
	N_("<date>") },
 { "rbexclude", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_ROLLBACK_EXCLUDE,
	N_("Exclude Transaction I.D. from rollback"),
	N_("<tid>") },
 { "upgrade", 'U', POPT_BIT_SET,
	&rpmIArgs.installInterfaceFlags, (INSTALL_UPGRADE|INSTALL_INSTALL),
	N_("upgrade package(s)"),
	N_("<packagefile>+") },

   POPT_TABLEEND
};
/*@=bitwisesigned =compmempass @*/
