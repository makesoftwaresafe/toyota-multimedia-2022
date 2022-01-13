#ifndef H_RPMCLI
#define	H_RPMCLI

/** \ingroup rpmcli rpmbuild
 * \file lib/rpmcli.h
 */

#include <popt.h>
#include <rpmmacro.h>
#include <rpmtypes.h>
#include <rpmtag.h>
#include <rpmps.h>
#include <rpmrc.h>
#include <rpmfi.h>	/* XXX rpmfileAttrs */
#include <rpmts.h>	/* XXX rpmdepFlags */

/**
 * Table of query format extensions.
 * @note Chains *headerCompoundFormats -> *headerDefaultFormats.
 */
/*@-redecl@*/
/*@unchecked@*/
extern headerSprintfExtension rpmHeaderFormats;
/*@=redecl@*/

/** \ingroup rpmcli
 * Should version 3 packages be produced?
 */
/*@-redecl@*/
/*@unchecked@*/
extern int _noDirTokens;
/*@=redecl@*/

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmcli
 * Popt option table for options shared by all modes and executables.
 */
/*@unchecked@*/
extern struct poptOption		rpmcliAllPoptTable[];

/*@unchecked@*/
extern int global_depFlags;

/*@unchecked@*/
extern struct poptOption		rpmcliDepFlagsPoptTable[];

/*@unchecked@*/ /*@observer@*/ /*@null@*/
extern const char * rpmcliTargets;
/*@=redecl@*/

/**
 * Initialize most everything needed by an rpm CLI executable context.
 * @param argc			no. of args
 * @param argv			arg array
 * @param optionsTable		popt option table
 * @return			popt context (or NULL)
 */
/*@null@*/
poptContext
rpmcliInit(int argc, char *const argv[], struct poptOption * optionsTable)
	/*@globals rpmCLIMacroContext, rpmGlobalMacroContext, h_errno, stderr, 
		fileSystem, internalState @*/
	/*@modifies rpmCLIMacroContext, rpmGlobalMacroContext, stderr, 
		fileSystem, internalState @*/;

/**
 * Make sure that rpm configuration has been read.
 * @warning Options like --rcfile and --verbose must precede callers option.
 */
/*@mayexit@*/
void rpmcliConfigured(void)
	/*@globals rpmCLIMacroContext,
		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmCLIMacroContext, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/**
 * Destroy most everything needed by an rpm CLI executable context.
 * @param optCon		popt context
 * @return			NULL always
 */
poptContext
rpmcliFini(/*@only@*/ /*@null@*/ poptContext optCon)
	/*@globals rpmTags, rpmGlobalMacroContext,
		fileSystem, internalState @*/
	/*@modifies optCon, rpmTags, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/**
 * Common/global popt tokens used for command line option tables.
 */
#define	RPMCLI_POPT_NODEPS		-1026
#define	RPMCLI_POPT_NOFDIGESTS		-1027
#define	RPMCLI_POPT_NOSCRIPTS		-1028
#define	RPMCLI_POPT_NOSIGNATURE		-1029
#define	RPMCLI_POPT_NODIGEST		-1030
#define	RPMCLI_POPT_NOHDRCHK		-1031
#define	RPMCLI_POPT_NOCONTEXTS		-1032
#define	RPMCLI_POPT_TARGETPLATFORM	-1033
#define	RPMCLI_POPT_NOHMACS		-1034

/* ==================================================================== */
/** \name RPMQV */
/*@{*/

/** \ingroup rpmcli
 * Query/Verify argument qualifiers.
 * @todo Reassign to tag values.
 */
typedef enum rpmQVSources_e {
    RPMQV_PACKAGE = 0,	/*!< ... from package name db search. */
    RPMQV_PATH,		/*!< ... from file path db search. */
    RPMQV_ALL,		/*!< ... from each installed package. */
    RPMQV_RPM, 		/*!< ... from reading binary rpm package. */
    RPMQV_GROUP		= RPMTAG_GROUP,
    RPMQV_WHATPROVIDES,	/*!< ... from provides db search. */
    RPMQV_WHATREQUIRES,	/*!< ... from requires db search. */
    RPMQV_TRIGGEREDBY	= RPMTAG_TRIGGERNAME,
    RPMQV_DBOFFSET,	/*!< ... from database header instance. */
    RPMQV_SPECFILE,	/*!< ... from spec file parse (query only). */
    RPMQV_PKGID,	/*!< ... from package id (header+payload MD5). */
    RPMQV_HDRID,	/*!< ... from header id (immutable header SHA1). */
    RPMQV_FILEID,	/*!< ... from file id (file digest, usually MD5). */
    RPMQV_TID,		/*!< ... from install transaction id (time stamp). */
    RPMQV_HDLIST,	/*!< ... from system hdlist. */
    RPMQV_FTSWALK,	/*!< ... from fts(3) walk. */
    RPMQV_WHATNEEDS,	/*!< ... from requires using contained provides. */
    RPMQV_SPECSRPM,	/*!< ... srpm from spec file parse (query only). */
    RPMQV_SOURCEPKGID,	/*!< ... from source package id (header+payload MD5). */
    RPMQV_WHATCONFLICTS	= RPMTAG_CONFLICTNAME,
    RPMQV_WHATOBSOLETES	= RPMTAG_OBSOLETENAME
} rpmQVSources;

/** \ingroup rpmcli
 * Bit(s) for rpmVerifyFile() attributes and result.
 */
typedef enum rpmVerifyAttrs_e {
    RPMVERIFY_NONE	= 0,		/*!< */
    RPMVERIFY_FDIGEST	= (1 << 0),	/*!< from %verify(digest) */
    RPMVERIFY_FILESIZE	= (1 << 1),	/*!< from %verify(size) */
    RPMVERIFY_LINKTO	= (1 << 2),	/*!< from %verify(link) */
    RPMVERIFY_USER	= (1 << 3),	/*!< from %verify(user) */
    RPMVERIFY_GROUP	= (1 << 4),	/*!< from %verify(group) */
    RPMVERIFY_MTIME	= (1 << 5),	/*!< from %verify(mtime) */
    RPMVERIFY_MODE	= (1 << 6),	/*!< from %verify(mode) */
    RPMVERIFY_RDEV	= (1 << 7),	/*!< from %verify(rdev) */
    RPMVERIFY_CAPS	= (1 << 8),	/*!< from %verify(caps) (unimplemented) */
	/* bits 9-13 unused, reserved for rpmVerifyAttrs */
    RPMVERIFY_HMAC	= (1 << 14),
    RPMVERIFY_CONTEXTS	= (1 << 15),	/*!< verify: from --nocontexts */
	/* bits 16-22 used in rpmVerifyFlags */
	/* bits 23-27 used in rpmQueryFlags */
    RPMVERIFY_READLINKFAIL= (1 << 28),	/*!< readlink failed */
    RPMVERIFY_READFAIL	= (1 << 29),	/*!< file read failed */
    RPMVERIFY_LSTATFAIL	= (1 << 30),	/*!< lstat failed */
    RPMVERIFY_LGETFILECONFAIL	= (1 << 31)	/*!< lgetfilecon failed */
} rpmVerifyAttrs;
#define	RPMVERIFY_ALL		((rpmVerifyAttrs)~(RPMVERIFY_NONE))
#define	RPMVERIFY_FAILURES	\
  (RPMVERIFY_LSTATFAIL|RPMVERIFY_READFAIL|RPMVERIFY_READLINKFAIL|RPMVERIFY_LGETFILECONFAIL)

/** \ingroup rpmcli
 * Bit(s) to control rpmQuery() operation, stored in qva_flags.
 * @todo Merge rpmQueryFlags, rpmVerifyFlags, and rpmVerifyAttrs?.
 */
typedef enum rpmQueryFlags_e {
/*@-enummemuse@*/
    QUERY_FOR_DEFAULT	= 0,		/*!< */
    QUERY_FDIGEST	= (1 << 0),	/*!< from --nofdigest */
    QUERY_SIZE		= (1 << 1),	/*!< from --nosize */
    QUERY_LINKTO	= (1 << 2),	/*!< from --nolink */
    QUERY_USER		= (1 << 3),	/*!< from --nouser) */
    QUERY_GROUP		= (1 << 4),	/*!< from --nogroup) */
    QUERY_MTIME		= (1 << 5),	/*!< from --nomtime) */
    QUERY_MODE		= (1 << 6),	/*!< from --nomode) */
    QUERY_RDEV		= (1 << 7),	/*!< from --nodev */
    QUERY_CAPS		= (1 << 8),	/*!< (unimplemented) */
	/* bits 9-13 unused, reserved for rpmVerifyAttrs */
    QUERY_HMAC		= (1 << 14),
    QUERY_CONTEXTS	= (1 << 15),	/*!< verify: from --nocontexts */
    QUERY_FILES		= (1 << 16),	/*!< verify: from --nofiles */
    QUERY_DEPS		= (1 << 17),	/*!< verify: from --nodeps */
    QUERY_SCRIPT	= (1 << 18),	/*!< verify: from --noscripts */
    QUERY_DIGEST	= (1 << 19),	/*!< verify: from --nodigest */
    QUERY_SIGNATURE	= (1 << 20),	/*!< verify: from --nosignature */
    QUERY_PATCHES	= (1 << 21),	/*!< verify: from --nopatches */
    QUERY_HDRCHK	= (1 << 22),	/*!< verify: from --nohdrchk */
/*@=enummemuse@*/
    QUERY_FOR_LIST	= (1 << 23),	/*!< query:  from --list */
    QUERY_FOR_STATE	= (1 << 24),	/*!< query:  from --state */
    QUERY_FOR_DOCS	= (1 << 25),	/*!< query:  from --docfiles */
    QUERY_FOR_CONFIG	= (1 << 26),	/*!< query:  from --configfiles */
    QUERY_FOR_DUMPFILES	= (1 << 27)	/*!< query:  from --dump */
} rpmQueryFlags;

#define	_QUERY_FOR_BITS	\
   (QUERY_FOR_LIST|QUERY_FOR_STATE|QUERY_FOR_DOCS|QUERY_FOR_CONFIG|\
    QUERY_FOR_DUMPFILES)

/** \ingroup rpmcli
 * Bit(s) from common command line options.
 */
/*@unchecked@*/
extern rpmQueryFlags rpmcliQueryFlags;

/** \ingroup rpmcli
 * Bit(s) to control rpmVerify() operation, stored in qva_flags.
 * @todo Merge rpmQueryFlags, rpmVerifyFlags, and rpmVerifyAttrs values?.
 */
typedef enum rpmVerifyFlags_e {
/*@-enummemuse@*/
    VERIFY_DEFAULT	= 0,		/*!< */
/*@=enummemuse@*/
    VERIFY_FDIGEST	= (1 << 0),	/*!< from --nofdigest */
    VERIFY_SIZE		= (1 << 1),	/*!< from --nosize */
    VERIFY_LINKTO	= (1 << 2),	/*!< from --nolinkto */
    VERIFY_USER		= (1 << 3),	/*!< from --nouser */
    VERIFY_GROUP	= (1 << 4),	/*!< from --nogroup */
    VERIFY_MTIME	= (1 << 5),	/*!< from --nomtime */
    VERIFY_MODE		= (1 << 6),	/*!< from --nomode */
    VERIFY_RDEV		= (1 << 7),	/*!< from --nodev */
    VERIFY_CAPS		= (1 << 8),	/*!< (unimplemented) */
	/* bits 9-13 unused, reserved for rpmVerifyAttrs */
    VERIFY_HMAC		= (1 << 14),
    VERIFY_CONTEXTS	= (1 << 15),	/*!< verify: from --nocontexts */
    VERIFY_FILES	= (1 << 16),	/*!< verify: from --nofiles */
    VERIFY_DEPS		= (1 << 17),	/*!< verify: from --nodeps */
    VERIFY_SCRIPT	= (1 << 18),	/*!< verify: from --noscripts */
    VERIFY_DIGEST	= (1 << 19),	/*!< verify: from --nodigest */
    VERIFY_SIGNATURE	= (1 << 20),	/*!< verify: from --nosignature */
    VERIFY_PATCHES	= (1 << 21),	/*!< verify: from --nopatches */
    VERIFY_HDRCHK	= (1 << 22),	/*!< verify: from --nohdrchk */
/*@-enummemuse@*/
    VERIFY_FOR_LIST	= (1 << 23),	/*!< query:  from --list */
    VERIFY_FOR_STATE	= (1 << 24),	/*!< query:  from --state */
    VERIFY_FOR_DOCS	= (1 << 25),	/*!< query:  from --docfiles */
    VERIFY_FOR_CONFIG	= (1 << 26),	/*!< query:  from --configfiles */
    VERIFY_FOR_DUMPFILES= (1 << 27)	/*!< query:  from --dump */
/*@=enummemuse@*/
	/* bits 28-31 used in rpmVerifyAttrs */
} rpmVerifyFlags;

#define	VERIFY_ATTRS	\
  ( VERIFY_FDIGEST | VERIFY_SIZE | VERIFY_LINKTO | VERIFY_USER | VERIFY_GROUP | \
    VERIFY_MTIME | VERIFY_MODE | VERIFY_RDEV | VERIFY_HMAC | VERIFY_CONTEXTS )
#define	VERIFY_ALL	\
  ( VERIFY_ATTRS | VERIFY_FILES | VERIFY_DEPS | VERIFY_SCRIPT | VERIFY_DIGEST |\
    VERIFY_SIGNATURE | VERIFY_HDRCHK )

/** \ingroup rpmcli
 */
typedef struct rpmQVKArguments_s * QVA_t;

/** \ingroup rpmcli
 * Function to display iterator matches.
 *
 * @param qva		parsed query/verify options
 * @param ts		transaction set
 * @param h		header to use for query/verify
 * @return		0 on success
 */
typedef	int (*QVF_t) (QVA_t qva, rpmts ts, Header h)
	/*@globals fileSystem @*/
	/*@modifies qva, ts, fileSystem @*/;

/** \ingroup rpmcli
 * Function to query spec file.
 *
 * @param ts		transaction set
 * @param qva		parsed query/verify options
 * @param arg		query argument
 * @return		0 on success
 */
typedef	int (*QSpecF_t) (rpmts ts, QVA_t qva, const char * arg)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, qva, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct poptOption rpmQVSourcePoptTable[];

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern int specedit;

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct poptOption rpmQueryPoptTable[];

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct poptOption rpmVerifyPoptTable[];

/** \ingroup rpmcli
 * Common query/verify source interface, called once for each CLI arg.
 *
 * This routine uses:
 *	- qva->qva_mi		rpm database iterator
 *	- qva->qva_showPackage	query/verify display routine
 *
 * @param qva		parsed query/verify options
 * @param ts		transaction set
 * @param arg		name of source to query/verify
 * @return		showPackage() result, 1 if rpmmiInit() is NULL
 */
int rpmQueryVerify(QVA_t qva, rpmts ts, const char * arg)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies qva, ts, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Display results of package query.
 * @todo Devise a meaningful return code.
 * @param qva		parsed query/verify options
 * @param ts		transaction set
 * @param h		header to use for query
 * @return		0 always
 */
int showQueryPackage(QVA_t qva, rpmts ts, Header h)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, h, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Iterate over query/verify arg list.
 * @param ts		transaction set
 * @param qva		parsed query/verify options
 * @param argv		query argument(s) (or NULL)
 * @return		0 on success, else no. of failures
 */
int rpmcliArgIter(rpmts ts, QVA_t qva, /*@null@*/ const char ** argv)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, qva, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Display package information.
 * @todo hack: RPMQV_ALL can pass char ** arglist = NULL, not char * arg. Union?
 * @param ts		transaction set
 * @param qva		parsed query/verify options
 * @param argv		query argument(s) (or NULL)
 * @return		0 on success, else no. of failures
 */
int rpmcliQuery(rpmts ts, QVA_t qva, /*@null@*/ const char ** argv)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, qva, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Display results of package verify.
 * @param qva		parsed query/verify options
 * @param ts		transaction set
 * @param h		header to use for verify
 * @return		result of last non-zero verify return
 */
int showVerifyPackage(QVA_t qva, rpmts ts, Header h)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, h, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Check package and header signatures.
 * @param qva		parsed query/verify options
 * @param ts		transaction set
 * @param _fd		package file handle
 * @param fn		package file name
 * @return		0 on success, 1 on failure
 */
int rpmVerifySignatures(QVA_t qva, rpmts ts, void * _fd, const char * fn)
	/*@globals fileSystem, internalState @*/
	/*@modifies qva, ts, fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Verify package install.
 * @todo hack: RPMQV_ALL can pass char ** arglist = NULL, not char * arg. Union?
 * @param ts		transaction set
 * @param qva		parsed query/verify options
 * @param argv		verify argument(s) (or NULL)
 * @return		0 on success, else no. of failures
 */
int rpmcliVerify(rpmts ts, QVA_t qva, /*@null@*/ const char ** argv)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, qva, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/*@}*/
/* ==================================================================== */
/** \name RPMEIU */
/*@{*/
/* --- install/upgrade/erase modes */

/** \ingroup rpmcli
 * Bit(s) to control rpmcliInstall() and rpmErase() operation.
 */
typedef enum rpmInstallInterfaceFlags_e {
    INSTALL_NONE	= 0,
    INSTALL_PERCENT	= (1 <<  0),	/*!< from --percent */
    INSTALL_HASH	= (1 <<  1),	/*!< from --hash */
    INSTALL_NODEPS	= (1 <<  2),	/*!< from --nodeps */
    INSTALL_NOORDER	= (1 <<  3),	/*!< from --noorder */
    INSTALL_LABEL	= (1 <<  4),	/*!< from --verbose (notify) */
    INSTALL_UPGRADE	= (1 <<  5),	/*!< from --upgrade */
    INSTALL_FRESHEN	= (1 <<  6),	/*!< from --freshen */
    INSTALL_INSTALL	= (1 <<  7),	/*!< from --install */
    INSTALL_ERASE	= (1 <<  8),	/*!< from --erase */
    INSTALL_ALLMATCHES	= (1 <<  9)	/*!< from --allmatches (erase) */
} rpmInstallInterfaceFlags;

/*@-redecl@*/
/*@unchecked@*/
extern int rpmcliPackagesTotal;
/*@=redecl@*/
/*@unchecked@*/
extern int rpmcliHashesCurrent;
/*@unchecked@*/
extern int rpmcliHashesTotal;
/*@unchecked@*/
extern rpmuint64_t rpmcliProgressCurrent;
/*@unchecked@*/
extern rpmuint64_t rpmcliProgressTotal;

/** \ingroup rpmcli
 * The rpm CLI generic transaction callback handler.
 * @todo Remove headerSprintf() from the progress callback.
 * @warning This function's args have changed, so the function cannot be
 * used portably
 * @deprecated Transaction callback arguments need to change, so don't rely on
 * this routine in the rpmcli API.
 *
 * @param arg		per-callback private data (e.g. an rpm header)
 * @param what		callback identifier
 * @param amount	per-callback progress info
 * @param total		per-callback progress info
 * @param key		opaque header key (e.g. file name or PyObject)
 * @param data		private data (e.g. rpmInstallInterfaceFlags)
 * @return		per-callback data (e.g. an opened FD_t)
 */
/*@null@*/
void * rpmShowProgress(/*@null@*/ const void * arg,
		const rpmCallbackType what,
		const rpmuint64_t amount,
		const rpmuint64_t total,
		/*@null@*/ fnpyKey key,
		/*@null@*/ void * data)
	/*@globals rpmcliHashesCurrent,
		rpmcliProgressCurrent, rpmcliProgressTotal,
		h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies arg, rpmcliHashesCurrent,
		rpmcliProgressCurrent, rpmcliProgressTotal,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Install source rpm package.
 * @param ts		transaction set
 * @param arg		source rpm file name
 * @retval *specFilePtr	(installed) spec file name
 * @retval *cookie
 * @return		0 on success
 */
int rpmInstallSource(rpmts ts, const char * arg,
		/*@null@*/ /*@out@*/ const char ** specFilePtr,
		/*@null@*/ /*@out@*/ const char ** cookie)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState@*/
	/*@modifies ts, *specFilePtr, *cookie, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Report package problems (if any).
 * @param ts		transaction set
 * @param msg		problem context string to display
 * @param rc		result of a tranbsaction operation
 * @return		no. of (added) packages
 */
int rpmcliInstallProblems(rpmts ts, /*@null@*/ const char * msg, int rc)
	/*@globals fileSystem, internalState @*/
	/*@modifies ts, fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Report packages(if any) that satisfy unresolved dependencies.
 * @param ts		transaction set
 * @return		0 always
 */
int rpmcliInstallSuggests(rpmts ts)
	/*@globals internalState @*/
	/*@modifies ts, internalState @*/;

/** \ingroup rpmcli
 * Check package element dependencies in a transaction set, reporting problems.
 * @param ts		transaction set
 * @return		no. of (added) packages
 */
int rpmcliInstallCheck(rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Order package elements in a transaction set, reporting problems.
 * @param ts		transaction set
 * @return		no. of (added) packages
 */
int rpmcliInstallOrder(rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Install/erase package elements in a transaction set, reporting problems.
 * @param ts		transaction set
 * @param okProbs	previously known problems (or NULL)
 * @param ignoreSet	bits to filter problem types
 * @return		0 on success, -1 on error, >0 no, of failed elements
 */
int rpmcliInstallRun(rpmts ts, rpmps okProbs, rpmprobFilterFlags ignoreSet)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Install/upgrade/freshen binary rpm package.
 * @todo Use rpmdsCompare rather than rpmVersionCompare.
 * @param ts		transaction set
 * @param ia		mode flags and parameters
 * @param argv		array of package file names (NULL terminated)
 * @return		0 on success
 */
int rpmcliInstall(rpmts ts, QVA_t ia, /*@null@*/ const char ** argv)
	/*@globals rpmcliPackagesTotal, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState@*/
	/*@modifies ts, ia, rpmcliPackagesTotal, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Erase binary rpm package.
 * @param ts		transaction set
 * @param ia		control args/bits
 * @param argv		array of package names (NULL terminated)
 * @return		0 on success
 */
int rpmErase(rpmts ts, QVA_t ia, /*@null@*/ const char ** argv)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, ia, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct poptOption rpmInstallPoptTable[];

/*@}*/
/* ==================================================================== */
/** \name RPMDB */
/*@{*/

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct poptOption rpmDatabasePoptTable[];

/*@}*/
/* ==================================================================== */
/** \name RPMK */
/*@{*/

/** \ingroup rpmcli
 * Import public key packet(s).
 * @todo Implicit --update policy for gpg-pubkey headers.
 * @param ts		transaction set
 * @param pkt		pgp pubkey packet(s)
 * @param pktlen	pgp pubkey length
 * @return		RPMRC_OK/RPMRC_FAIL
 */
rpmRC rpmcliImportPubkey(const rpmts ts,
		const unsigned char * pkt, ssize_t pktlen)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Bit(s) to control rpmReSign() operation.
 */
/*@-typeuse@*/
#if !defined(SWIG)
typedef enum rpmSignFlags_e {
    RPMSIGN_NONE		= 0,
    RPMSIGN_CHK_SIGNATURE	= 'K',	/*!< from --checksig */
    RPMSIGN_NEW_SIGNATURE	= 'R',	/*!< from --resign */
    RPMSIGN_ADD_SIGNATURE	= 'A',	/*!< from --addsign */
    RPMSIGN_DEL_SIGNATURE	= 'D',	/*!< from --delsign */
    RPMSIGN_IMPORT_PUBKEY	= 'I',	/*!< from --import */
} rpmSignFlags;
#endif
/*@=typeuse@*/

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct poptOption rpmSignPoptTable[];

/** \ingroup rpmcli
 * Create/Modify/Check elements from signature header.
 * @param ts		transaction set
 * @param qva		mode flags and parameters
 * @param argv		array of arguments (NULL terminated)
 * @return		0 on success
 */
int rpmcliSign(rpmts ts, QVA_t qva, /*@null@*/ const char ** argv)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, qva, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/*@}*/

/** \ingroup rpmcli
 * Command line option information.
 */
#if !defined(SWIG)
struct rpmQVKArguments_s {
    rpmQVSources qva_source;	/*!< Identify CLI arg type. */
    int 	qva_sourceCount;/*!< Exclusive option check (>1 is error). */
    rpmQueryFlags qva_flags;	/*!< Bit(s) to control operation. */
    rpmfileAttrs qva_fflags;	/*!< Bit(s) to filter on attribute. */
/*@only@*/ /*@null@*/
    rpmmi qva_mi;		/*!< Match iterator on selected headers. */
/*@refccounted@*/ /*@relnull@*/
    rpmgi qva_gi;		/*!< Generalized iterator on args. */
    rpmRC qva_rc;		/*!< Current return code. */

/*@null@*/
    QVF_t qva_showPackage;	/*!< Function to display iterator matches. */
    int qva_showOK;		/*!< No. of successes. */
    int qva_showFAIL;		/*!< No. of failures. */
/*@null@*/
    QSpecF_t qva_specQuery;	/*!< Function to query spec file. */
/*@unused@*/
    int qva_verbose;		/*!< (unused) */
/*@only@*/ /*@null@*/
    const char * qva_queryFormat;/*!< Format for headerSprintf(). */
    int sign;			/*!< Is a passphrase needed? */
    int nopassword;
    int trust;			/*!< Trust metric when importing pubkeys. */
/*@observer@*/
    const char * passPhrase;	/*!< Pass phrase. */
/*@owned@*/ /*@null@*/
    const char * qva_prefix;	/*!< Path to top of install tree. */
    char qva_mode;
		/*!<
		- 'q'	from --query, -q
		- 'Q'	from --querytags
		- 'V'	from --verify, -V
		- 'A'	from --addsign
		- 'I'	from --import
		- 'K'	from --checksig, -K
		- 'R'	from --resign
		*/
    char qva_char;		/*!< (unused) always ' ' */

    /* install/erase mode arguments */
    rpmdepFlags depFlags;
    rpmtransFlags transFlags;
    rpmprobFilterFlags probFilter;
    rpmInstallInterfaceFlags installInterfaceFlags;
    rpmuint32_t arbtid;		/*!< from --arbgoal */
    rpmuint32_t rbtid;		/*!< from --rollback */
    rpmuint32_t *rbtidExcludes;	/*!< from --rollback */
    int numrbtidExcludes;	/*!< from --rollback */
    int noDeps;
    int incldocs;
    int no_rollback_links;
/*@owned@*/ /*@relnull@*/
    rpmRelocation relocations;
    int nrelocations;

    /* database mode arguments */
    int rebuild;		/*!< from --rebuilddb */

    /* rollback vectors */
    int (*rbCheck) (rpmts ts);
    int (*rbOrder) (rpmts ts);
    int (*rbRun) (rpmts ts, rpmps okProbs, rpmprobFilterFlags ignoreSet);
};
#endif

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct rpmQVKArguments_s rpmQVKArgs;

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct rpmQVKArguments_s rpmIArgs;

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct rpmQVKArguments_s rpmDBArgs;

/* ==================================================================== */
/** \name RPMBT */
/*@{*/

/** \ingroup rpmcli
 * Describe build command line request.
 */
struct rpmBuildArguments_s {
    rpmQueryFlags qva_flags;	/*!< Bit(s) to control verification. */
    int buildAmount;		/*!< Bit(s) to control operation. */
/*@observer@*/
    const char * passPhrase;	/*!< Pass phrase. */
/*@only@*/ /*@null@*/
    const char * cookie;	/*!< NULL for binary, ??? for source, rpm's */
    const char * specFile;	/*!< from --rebuild/--recompile build */
    int noBuild;		/*!< from --nobuild */
    int noDeps;			/*!< from --nodeps */
    int noLang;			/*!< from --nolang */
    int shortCircuit;		/*!< from --short-circuit */
    int sign;			/*!< from --sign */
    int nopassword;
    char buildMode;		/*!< Build mode (one of "btBC") */
    char buildChar;		/*!< Build stage (one of "abcilps ") */
/*@observer@*/ /*@null@*/
    const char * rootdir;
};

/** \ingroup rpmcli
 */
typedef	struct rpmBuildArguments_s *	BTA_t;

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct rpmBuildArguments_s	rpmBTArgs;

/** \ingroup rpmcli
 */
/*@unchecked@*/
extern struct poptOption		rpmBuildPoptTable[];

/*@}*/

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMCLI */
