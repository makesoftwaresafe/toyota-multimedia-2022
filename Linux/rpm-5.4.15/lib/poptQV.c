/** \ingroup rpmcli
 * \file lib/poptQV.c
 *  Popt tables for query/verify modes.
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmcb.h>

#include <rpmtag.h>
#include <rpmcli.h>
#include <rpmgi.h>		/* XXX for giFlags */

#include "debug.h"

/*@unchecked@*/
struct rpmQVKArguments_s rpmQVKArgs;

/*@unchecked@*/
int specedit = 0;

#define POPT_QUERYFORMAT	-1000
#define POPT_WHATREQUIRES	-1001
#define POPT_WHATPROVIDES	-1002
#define POPT_QUERYBYNUMBER	-1003
#define POPT_TRIGGEREDBY	-1004
#define POPT_DUMP		-1005
#define POPT_SPECFILE		-1006
#define POPT_QUERYBYPKGID	-1007
#define POPT_QUERYBYHDRID	-1008
#define POPT_QUERYBYFILEID	-1009
#define POPT_QUERYBYTID		-1010
#define POPT_HDLIST		-1011
#define POPT_FTSWALK		-1012

/* -1025 thrugh -1033 are common in rpmcli.h. */
#define	POPT_TRUST		-1037
#define	POPT_WHATNEEDS		-1038
#define	POPT_SPECSRPM		-1039
#define POPT_QUERYBYSOURCEPKGID	-1040
#define	POPT_WHATCONFLICTS	-1041
#define	POPT_WHATOBSOLETES	-1042
#define	POPT_NOPASSWORD		-1043

/* ========== Query/Verify/Signature source args */
static void rpmQVSourceArgCallback( /*@unused@*/ poptContext con,
		/*@unused@*/ enum poptCallbackReason reason,
		const struct poptOption * opt,
		/*@unused@*/ const char * arg,
		/*@unused@*/ const void * data)
	/*@globals rpmQVKArgs @*/
	/*@modifies rpmQVKArgs @*/
{
    QVA_t qva = &rpmQVKArgs;
    unsigned * _sourcep = (unsigned *)&qva->qva_source;

    switch (opt->val) {
    case 'q':	/* from --query, -q */
    case 'Q':	/* from --querytags (handled by poptALL) */
    case 'V':	/* from --verify, -V */
    case 'A':	/* from --addsign */
    case 'D':	/* from --delsign */
    case 'I':	/* from --import */
    case 'K':	/* from --checksig, -K */
    case 'R':	/* from --resign */
	if (qva->qva_mode == '\0' || strchr("qQ ", qva->qva_mode)) {
	    qva->qva_mode = opt->val;
	    qva->qva_char = ' ';
	}
	break;
    case 'a': *_sourcep |= RPMQV_ALL; qva->qva_sourceCount++; break;
    case 'f': *_sourcep |= RPMQV_PATH; qva->qva_sourceCount++; break;
    case 'g': *_sourcep |= RPMQV_GROUP; qva->qva_sourceCount++; break;
    case 'p': *_sourcep |= RPMQV_RPM; qva->qva_sourceCount++; break;
    case POPT_WHATNEEDS: *_sourcep |= RPMQV_WHATNEEDS;
				qva->qva_sourceCount++; break;
    case POPT_WHATPROVIDES: *_sourcep |= RPMQV_WHATPROVIDES;
				qva->qva_sourceCount++; break;
    case POPT_WHATREQUIRES: *_sourcep |= RPMQV_WHATREQUIRES;
				qva->qva_sourceCount++; break;
    case POPT_WHATCONFLICTS: *_sourcep |= RPMQV_WHATCONFLICTS;
				qva->qva_sourceCount++; break;
    case POPT_WHATOBSOLETES: *_sourcep |= RPMQV_WHATOBSOLETES;
				qva->qva_sourceCount++; break;
    case POPT_TRIGGEREDBY: *_sourcep |= RPMQV_TRIGGEREDBY;
				qva->qva_sourceCount++; break;
    case POPT_QUERYBYSOURCEPKGID: *_sourcep |= RPMQV_SOURCEPKGID;
				qva->qva_sourceCount++; break;
    case POPT_QUERYBYPKGID: *_sourcep |= RPMQV_PKGID;
				qva->qva_sourceCount++; break;
    case POPT_QUERYBYHDRID: *_sourcep |= RPMQV_HDRID;
				qva->qva_sourceCount++; break;
    case POPT_QUERYBYFILEID: *_sourcep |= RPMQV_FILEID;
				qva->qva_sourceCount++; break;
    case POPT_QUERYBYTID: *_sourcep |= RPMQV_TID;
				qva->qva_sourceCount++; break;
    case POPT_HDLIST: *_sourcep |= RPMQV_HDLIST;
				qva->qva_sourceCount++; break;
    case POPT_FTSWALK: *_sourcep |= RPMQV_FTSWALK;
				qva->qva_sourceCount++; break;

/* XXX SPECFILE is not verify sources */
    case POPT_SPECFILE:
	*_sourcep |= RPMQV_SPECFILE;
	qva->qva_sourceCount++;
	break;
/* XXX SPECSRPM is not verify sources */
    case POPT_SPECSRPM:
	*_sourcep |= RPMQV_SPECSRPM;
	qva->qva_sourceCount++;
	break;
    case POPT_QUERYBYNUMBER:
	*_sourcep |= RPMQV_DBOFFSET;
	qva->qva_sourceCount++;
	break;

    case POPT_NOPASSWORD:
	qva->nopassword = 1;
	break;

    }
}

/**
 * Common query/verify mode options.
 */
/*@unchecked@*/
struct poptOption rpmQVSourcePoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA,
	(void *) rpmQVSourceArgCallback, 0, NULL, NULL },
/*@=type@*/
 { "all", 'a', 0, NULL, 'a',
	N_("query/verify all packages"), NULL },
 { "checksig", 'K', POPT_ARGFLAG_DOC_HIDDEN, NULL, 'K',
	N_("rpm checksig mode"), NULL },
 { "file", 'f', 0, NULL, 'f',
	N_("query/verify package(s) owning file"), "FILE" },
 { "group", 'g', 0, NULL, 'g',
	N_("query/verify package(s) in group"), "GROUP" },
 { "package", 'p', 0, NULL, 'p',
	N_("query/verify a package file"), NULL },

 { "ftswalk", 'W', 0, NULL, POPT_FTSWALK,
	N_("query/verify package(s) from TOP file tree walk"), "TOP" },
 { "hdlist", 'H', POPT_ARGFLAG_DOC_HIDDEN, 0, POPT_HDLIST,
	N_("query/verify package(s) from system HDLIST"), "HDLIST" },

 { "sourcepkgid", '\0', 0, NULL, POPT_QUERYBYSOURCEPKGID,
	N_("query/verify package(s) with source package identifier"), "MD5" },
 { "pkgid", '\0', 0, NULL, POPT_QUERYBYPKGID,
	N_("query/verify package(s) with package identifier"), "MD5" },
 { "hdrid", '\0', 0, NULL, POPT_QUERYBYHDRID,
	N_("query/verify package(s) with header identifier"), "SHA1" },
 { "fileid", '\0', 0, NULL, POPT_QUERYBYFILEID,
	N_("query/verify package(s) with file identifier"), "MD5" },

 { "query", 'q', POPT_ARGFLAG_DOC_HIDDEN, NULL, 'q',
	N_("rpm query mode"), NULL },
 { "querybynumber", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_QUERYBYNUMBER,
	N_("query/verify a header instance"), "HDRNUM" },
 { "specfile", '\0', 0, NULL, POPT_SPECFILE,
	N_("query a spec file"), N_("<spec>") },
 { "specsrpm", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_SPECSRPM,
	N_("query source metadata from spec file parse"), N_("<spec>") },
 { "tid", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_QUERYBYTID,
	N_("query/verify package(s) from install transaction"), "TID" },
 { "triggeredby", '\0', 0, NULL, POPT_TRIGGEREDBY,
	N_("query the package(s) triggered by the package"), "PACKAGE" },
 { "verify", 'V', POPT_ARGFLAG_DOC_HIDDEN, NULL, 'V',
	N_("rpm verify mode"), NULL },
 { "whatrequires", '\0', 0, NULL, POPT_WHATREQUIRES,
	N_("query/verify the package(s) which require a dependency"), "CAPABILITY" },
 { "whatneeds", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_WHATNEEDS,
	N_("query/verify the package(s) which require any contained provide"),
	"CAPABILITY" },

 { "whatprovides", '\0', 0, NULL, POPT_WHATPROVIDES,
	N_("query/verify the package(s) which provide a dependency"), "CAPABILITY" },
 { "whatconflicts", '\0', 0, NULL, POPT_WHATCONFLICTS,
	N_("query/verify the package(s) which conflict with a dependency"), "CAPABILITY" },
 { "whatobsoletes", '\0', 0, NULL, POPT_WHATOBSOLETES,
	N_("query/verify the package(s) which obsolete a dependency"), "CAPABILITY" },

 { "transaction", 'T', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &giFlags, (RPMGI_TSADD|RPMGI_TSORDER),
	N_("create transaction set"), NULL},
#ifdef	DYING	/* XXX breaks --noorder in poptI.c */
 { "noorder", '\0', POPT_BIT_CLR|POPT_ARGFLAG_DOC_HIDDEN, &giFlags, RPMGI_TSORDER,
	N_("do not order transaction set"), NULL},
#endif
 { "noglob", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &giFlags, RPMGI_NOGLOB,
	N_("do not glob arguments"), NULL},
 { "nomanifest", '\0', POPT_BIT_SET, &giFlags, RPMGI_NOMANIFEST,
	N_("do not process non-package files as manifests"), NULL},
 { "noheader", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &giFlags, RPMGI_NOHEADER,
	N_("do not read headers"), NULL},


   POPT_TABLEEND
};

/* ========== Query specific popt args */

static void queryArgCallback(poptContext con,
		/*@unused@*/ enum poptCallbackReason reason,
		const struct poptOption * opt, const char * arg,
		/*@unused@*/ const void * data)
	/*@globals rpmQVKArgs @*/
	/*@modifies con, rpmQVKArgs @*/
{
    QVA_t qva = &rpmQVKArgs;
    unsigned * _flagsp = (unsigned *)&qva->qva_flags;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'c': *_flagsp |= QUERY_FOR_CONFIG | QUERY_FOR_LIST; break;
    case 'd': *_flagsp |= QUERY_FOR_DOCS | QUERY_FOR_LIST; break;
    case 'l': *_flagsp |= QUERY_FOR_LIST; break;
    case 's': *_flagsp |= QUERY_FOR_STATE | QUERY_FOR_LIST;
	break;
    case POPT_DUMP: *_flagsp |= QUERY_FOR_DUMPFILES | QUERY_FOR_LIST;
	break;

    case POPT_QUERYFORMAT:
	if (arg) {
	    char * qf = (char *)qva->qva_queryFormat;
	    char * b = NULL;
	    size_t nb = 0;

	    /* Read queryformat from file. */
	    if (arg[0] == '/') {
		const char * fn = arg;
		int rc;

		rc = poptReadFile(fn, &b, &nb, POPT_READFILE_TRIMNEWLINES);
		if (rc != 0)
		    goto _qfexit;
		if (b == NULL || nb == 0)	/* XXX can't happen */
		    goto _qfexit;
		/* XXX trim double quotes */
		if (*b == '"') {
		    while (nb > 0 && b[nb] != '"')
			b[nb--] = '\0';
		    b[nb] = '\0';
		    arg = b + 1;
		} else
		    arg = b;
	    }

	    /* Append to existing queryformat. */
	    if (qf) {
		size_t len = strlen(qf) + strlen(arg) + 1;
		qf = (char *) xrealloc(qf, len);
		strcat(qf, arg);
	    } else {
		qf = (char *) xmalloc(strlen(arg) + 1);
		strcpy(qf, arg);
	    }
	    qva->qva_queryFormat = qf;

	_qfexit:
	    b = _free(b);
	}
	break;

    case 'i':
	if (qva->qva_mode == 'q') {
	    /*@-nullassign -readonlytrans@*/
	    const char * infoCommand[] = { "--info", NULL };
	    /*@=nullassign =readonlytrans@*/
	    (void) poptStuffArgs(con, infoCommand);
	}
	break;

    case RPMCLI_POPT_NODIGEST:
	*_flagsp |= VERIFY_DIGEST;
	break;

    case RPMCLI_POPT_NOSIGNATURE:
	*_flagsp |= VERIFY_SIGNATURE;
	break;

    case RPMCLI_POPT_NOHDRCHK:
	*_flagsp |= VERIFY_HDRCHK;
	break;

    case RPMCLI_POPT_NODEPS:
	*_flagsp |= VERIFY_DEPS;
	break;

    case RPMCLI_POPT_NOFDIGESTS:
	*_flagsp |= VERIFY_FDIGEST;
	break;

    case RPMCLI_POPT_NOCONTEXTS:
	*_flagsp |= VERIFY_CONTEXTS;
	break;

    case RPMCLI_POPT_NOSCRIPTS:
	*_flagsp |= VERIFY_SCRIPT;
	break;

    case RPMCLI_POPT_NOHMACS:
	*_flagsp |= VERIFY_HMAC;
	break;

    /* XXX perhaps POPT_ARG_INT instead of callback. */
    case POPT_TRUST:
    {	char * end = NULL;
	long trust = (int) strtol(arg, &end, 0);
	/* XXX range checks on trust. */
	/* XXX if (end && *end) argerror(_("non-numeric trust metric.")); */
	qva->trust = trust;
    }	break;
    }
}

/**
 * Query mode options.
 */
/*@unchecked@*/
struct poptOption rpmQueryPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	(void *) queryArgCallback, 0, NULL, NULL },
/*@=type@*/
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmQVSourcePoptTable, 0,
	NULL, NULL },
 { "configfiles", 'c', 0, 0, 'c',
	N_("list all configuration files"), NULL },
 { "docfiles", 'd', 0, 0, 'd',
	N_("list all documentation files"), NULL },
 { "dump", '\0', 0, 0, POPT_DUMP,
	N_("dump basic file information"), NULL },
 { NULL, 'i', POPT_ARGFLAG_DOC_HIDDEN, 0, 'i',
	NULL, NULL },
 { "list", 'l', 0, 0, 'l',
	N_("list files in package"), NULL },

 { "aid", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.depFlags, RPMDEPS_FLAG_ADDINDEPS,
	N_("add suggested packages to transaction"), NULL },

 /* Duplicate file attr flags from packages into command line options. */
 { "noconfig", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_fflags, RPMFILE_CONFIG,
        N_("skip %%config files"), NULL },
 { "nodoc", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_fflags, RPMFILE_DOC,
        N_("skip %%doc files"), NULL },
 { "noghost", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_fflags, RPMFILE_GHOST,
        N_("skip %%ghost files"), NULL },
#ifdef	NOTEVER		/* XXX there's hardly a need for these */
 { "nolicense", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_fflags, RPMFILE_LICENSE,
        N_("skip %%license files"), NULL },
 { "noreadme", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_fflags, RPMFILE_README,
        N_("skip %%readme files"), NULL },
#endif

 { "qf", '\0', POPT_ARG_STRING | POPT_ARGFLAG_DOC_HIDDEN, 0,
	POPT_QUERYFORMAT, NULL, NULL },
 { "queryformat", '\0', POPT_ARG_STRING, 0, POPT_QUERYFORMAT,
	N_("use the following query format"), N_("QUERYFORMAT") },
 { "specedit", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &specedit, -1,
	N_("substitute i18n sections into spec file"), NULL },
 { "state", 's', 0, 0, 's',
	N_("display the states of the listed files"), NULL },
 { "target", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, 0,  RPMCLI_POPT_TARGETPLATFORM,
        N_("specify target platform"), N_("CPU-VENDOR-OS") },
   POPT_TABLEEND
};

/**
 * Verify mode options.
 */
struct poptOption rpmVerifyPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	(void *) queryArgCallback, 0, NULL, NULL },
/*@=type@*/
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmQVSourcePoptTable, 0,
	NULL, NULL },

 { "aid", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.depFlags, RPMDEPS_FLAG_ADDINDEPS,
	N_("add suggested packages to transaction"), NULL },

 /* Duplicate file attr flags from packages into command line options. */
 { "noconfig", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_fflags, RPMFILE_CONFIG,
        N_("skip %%config files"), NULL },
 { "nodoc", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_fflags, RPMFILE_DOC,
        N_("skip %%doc files"), NULL },

 /* Duplicate file verify flags from packages into command line options. */
/** @todo Add --nomd5 alias to rpmpopt, eliminate. */
#ifdef	DYING
 { "nomd5", '\0', POPT_BIT_SET, &rpmQVKArgs.qva_flags, VERIFY_FDIGEST,
	N_("don't verify file digests"), NULL },
#else
 { "nomd5", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, RPMCLI_POPT_NOFDIGESTS,
	N_("don't verify file digests"), NULL },
 { "nofdigests", '\0', 0, NULL, RPMCLI_POPT_NOFDIGESTS,
	N_("don't verify file digests"), NULL },
#endif
 { "nosize", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_SIZE,
        N_("don't verify size of files"), NULL },
 { "nolinkto", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_LINKTO,
        N_("don't verify symlink path of files"), NULL },
 { "nouser", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_USER,
        N_("don't verify owner of files"), NULL },
 { "nogroup", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_GROUP,
        N_("don't verify group of files"), NULL },
 { "nomtime", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_MTIME,
        N_("don't verify modification time of files"), NULL },
 { "nomode", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_MODE,
        N_("don't verify mode of files"), NULL },
 { "nordev", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_RDEV,
        N_("don't verify mode of files"), NULL },

 { "nohmacs", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, RPMCLI_POPT_NOHMACS,
	N_("don't verify file HMAC's"), NULL },
 { "nocontexts", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, RPMCLI_POPT_NOCONTEXTS,
	N_("don't verify file security contexts"), NULL },
 { "nofiles", '\0', POPT_BIT_SET, &rpmQVKArgs.qva_flags, VERIFY_FILES,
	N_("don't verify files in package"), NULL},
#ifdef	DYING
 { "nodeps", '\0', POPT_BIT_SET, &rpmQVKArgs.qva_flags, VERIFY_DEPS,
	N_("don't verify package dependencies"), NULL },
#else
 { "nodeps", '\0', 0, NULL, RPMCLI_POPT_NODEPS,
	N_("don't verify package dependencies"), NULL },
#endif

#ifdef	DYING
 { "noscript", '\0', POPT_BIT_SET,&rpmQVKArgs.qva_flags, VERIFY_SCRIPT,
        N_("don't execute verify script(s)"), NULL },
 /* XXX legacy had a trailing s on --noscript */
 { "noscripts", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_SCRIPT,
        N_("don't execute verify script(s)"), NULL },
#else
 { "noscript", '\0', 0, NULL, RPMCLI_POPT_NOSCRIPTS,
        N_("don't execute verify script(s)"), NULL },
 /* XXX legacy had a trailing s on --noscript */
 { "noscripts", '\0', POPT_ARGFLAG_DOC_HIDDEN, NULL, RPMCLI_POPT_NOSCRIPTS,
        N_("don't execute verify script(s)"), NULL },
#endif

#ifdef	DYING
 { "nodigest", '\0', POPT_BIT_SET, &rpmQVKArgs.qva_flags, VERIFY_DIGEST,
        N_("don't verify package digest(s)"), NULL },
 { "nohdrchk", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_HDRCHK,
        N_("don't verify database header(s) when retrieved"), NULL },
 { "nosignature", '\0', POPT_BIT_SET,
	&rpmQVKArgs.qva_flags, VERIFY_SIGNATURE,
        N_("don't verify package signature(s)"), NULL },
#else
 { "nodigest", '\0', POPT_ARGFLAG_DOC_HIDDEN, 0, RPMCLI_POPT_NODIGEST,
        N_("don't verify package digest(s)"), NULL },
 { "nohdrchk", '\0', POPT_ARGFLAG_DOC_HIDDEN, 0, RPMCLI_POPT_NOHDRCHK,
        N_("don't verify database header(s) when retrieved"), NULL },
 { "nosignature", '\0', POPT_ARGFLAG_DOC_HIDDEN, 0, RPMCLI_POPT_NOSIGNATURE,
        N_("don't verify package signature(s)"), NULL },
#endif

    POPT_TABLEEND
};

/**
 * Signature mode options.
 */
/*@unchecked@*/
struct poptOption rpmSignPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	(void *) rpmQVSourceArgCallback, 0, NULL, NULL },
/*@=type@*/
 { "addsign", '\0', 0, NULL, 'A',
	N_("sign package(s) (identical to --resign)"), NULL },
 { "checksig", 'K', 0, NULL, 'K',
	N_("verify package signature(s)"), NULL },
 { "delsign", '\0', 0, NULL, 'D',
	N_("delete package signatures"), NULL },
 { "import", '\0', 0, NULL, 'I',
	N_("import an armored public key"), NULL },
 { "resign", '\0', 0, NULL, 'R',
	N_("sign package(s) (identical to --addsign)"), NULL },
 { "sign", '\0', POPT_ARGFLAG_DOC_HIDDEN, &rpmQVKArgs.sign, 0,
	N_("generate signature"), NULL },

 { "trust", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, 0,  POPT_TRUST,
        N_("specify trust metric"), N_("TRUST") },
 { "trusted", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.trust, 1,
        N_("set ultimate trust when importing pubkey(s)"), NULL },
 { "untrusted", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.trust, -1,
        N_("unset ultimate trust when importing pubkey(s)"), NULL },
 { "nopassword", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, 0,  POPT_NOPASSWORD,
        N_("disable password challenge"), NULL },
 /* XXX perhaps POPT_ARG_INT instead of callback. */

 { "nodigest", '\0', POPT_BIT_SET, &rpmQVKArgs.qva_flags, VERIFY_DIGEST,
        N_("don't verify package digest(s)"), NULL },
 { "nohdrchk", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,
	&rpmQVKArgs.qva_flags, VERIFY_HDRCHK,
        N_("don't verify database header(s) when retrieved"), NULL },
 { "nosignature", '\0', POPT_BIT_SET, &rpmQVKArgs.qva_flags, VERIFY_SIGNATURE,
        N_("don't verify package signature(s)"), NULL },

   POPT_TABLEEND
};
