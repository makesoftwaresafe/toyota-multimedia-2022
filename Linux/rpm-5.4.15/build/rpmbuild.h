#ifndef	_H_RPMBUILD_
#define	_H_RPMBUILD_

/** \ingroup rpmbuild
 * \file build/rpmbuild.h
 *  This is the *only* module users of librpmbuild should need to include.
 */

#include <rpmiotypes.h>
#include <rpmmacro.h>
#include <rpmtypes.h>
#include <rpmtag.h>

#include <rpmfi.h>

#include <rpmcli.h>

#include "rpmspec.h"

/** \ingroup rpmbuild
 * Bit(s) to control buildSpec() operation.
 */
/*@-typeuse@*/
typedef enum rpmBuildFlags_e {
/*@-enummemuse@*/
    RPMBUILD_NONE	= 0,
/*@=enummemuse@*/
    RPMBUILD_PREP	= (1 <<  0),	/*!< Execute %%prep. */
    RPMBUILD_BUILD	= (1 <<  1),	/*!< Execute %%build. */
    RPMBUILD_INSTALL	= (1 <<  2),	/*!< Execute %%install. */
    RPMBUILD_CHECK	= (1 <<  3),	/*!< Execute %%check. */
    RPMBUILD_CLEAN	= (1 <<  4),	/*!< Execute %%clean. */
    RPMBUILD_FILECHECK	= (1 <<  5),	/*!< Check %%files manifest. */
    RPMBUILD_PACKAGESOURCE = (1 <<  6),	/*!< Create source package. */
    RPMBUILD_PACKAGEBINARY = (1 <<  7),	/*!< Create binary package(s). */
    RPMBUILD_RMSOURCE	= (1 <<  8),	/*!< Remove source(s) and patch(s). */
    RPMBUILD_RMBUILD	= (1 <<  9),	/*!< Remove build sub-tree. */
    RPMBUILD_STRINGBUF	= (1 << 10),	/*!< only for doScript() */
    RPMBUILD_TRACK	= (1 << 11),	/*!< Execute %%track. */
    RPMBUILD_RMSPEC	= (1 << 12),	/*!< Remove spec file. */
    RPMBUILD_FETCHSOURCE= (1 << 13)	/*!< Fetch source(s) and patch(s). */
} rpmBuildFlags;
/*@=typeuse@*/

#define SKIPSPACE(s) { while (*(s) && xisspace(*(s))) (s)++; }
#define SKIPNONSPACE(s) { while (*(s) && !xisspace(*(s))) (s)++; }

#define PART_SUBNAME  0
#define PART_NAME     1

/** \ingroup rpmbuild
 * Spec file parser states.
 */
#define	PART_BASE	100
typedef enum rpmParseState_e {
    PART_NONE		=  0+PART_BASE,	/*!< */
    /* leave room for RPMRC_NOTFOUND returns. */
    PART_PREAMBLE	= 11+PART_BASE,	/*!< */
    PART_PREP		= 12+PART_BASE,	/*!< */
    PART_BUILD		= 13+PART_BASE,	/*!< */
    PART_INSTALL	= 14+PART_BASE,	/*!< */
    PART_CHECK		= 15+PART_BASE,	/*!< */
    PART_CLEAN		= 16+PART_BASE,	/*!< */
    PART_FILES		= 17+PART_BASE,	/*!< */
    PART_PRE		= 18+PART_BASE,	/*!< */
    PART_POST		= 19+PART_BASE,	/*!< */
    PART_PREUN		= 20+PART_BASE,	/*!< */
    PART_POSTUN		= 21+PART_BASE,	/*!< */
    PART_PRETRANS	= 22+PART_BASE,	/*!< */
    PART_POSTTRANS	= 23+PART_BASE,	/*!< */
    PART_DESCRIPTION	= 24+PART_BASE,	/*!< */
    PART_CHANGELOG	= 25+PART_BASE,	/*!< */
    PART_TRIGGERIN	= 26+PART_BASE,	/*!< */
    PART_TRIGGERUN	= 27+PART_BASE,	/*!< */
    PART_VERIFYSCRIPT	= 28+PART_BASE,	/*!< */
    PART_BUILDARCHITECTURES= 29+PART_BASE,/*!< */
    PART_TRIGGERPOSTUN	= 30+PART_BASE,	/*!< */
    PART_TRIGGERPREIN	= 31+PART_BASE,	/*!< */
    /* support "%sanitycheck" script */
    PART_SANITYCHECK	= 32+PART_BASE, /*!< */
    PART_ARBITRARY	= 33+PART_BASE, /*!< */
    PART_LAST		= 34+PART_BASE  /*!< */
} rpmParseState;

/** \ingroup rpmbuild
 * Spec file parser stripping flags.
 */
typedef enum rpmStripFlags_e {
    STRIP_NOTHING	= 0,
    STRIP_TRAILINGSPACE	= (1 << 0),
    STRIP_COMMENTS	= (1 << 1),
    STRIP_NOEXPAND	= (1 << 2)
} rpmStripFlags;

/*@unchecked@*/
extern int _rpmbuildFlags;

#ifdef __cplusplus
extern "C" {
#endif
/*@-redecl@*/

/** \ingroup rpmbuild
 * Destroy uid/gid caches.
 */
void freeNames(void)
	/*@globals internalState@*/
	/*@modifies internalState */;

/** \ingroup rpmbuild
 * Return cached user name from user id.
 * @todo Implement using hash.
 * @param uid		user id
 * @return		cached user name
 */
extern /*@observer@*/ const char * getUname(uid_t uid)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/** \ingroup rpmbuild
 * Return cached user name.
 * @todo Implement using hash.
 * @param uname		user name
 * @return		cached user name
 */
extern /*@observer@*/ const char * getUnameS(const char * uname)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/** \ingroup rpmbuild
 * Return cached user id.
 * @todo Implement using hash.
 * @param uname		user name
 * @return		cached uid
 */
uid_t getUidS(const char * uname)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/** \ingroup rpmbuild
 * Return cached group name from group id.
 * @todo Implement using hash.
 * @param gid		group id
 * @return		cached group name
 */
extern /*@observer@*/ const char * getGname(gid_t gid)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/** \ingroup rpmbuild
 * Return cached group name.
 * @todo Implement using hash.
 * @param gname		group name
 * @return		cached group name
 */
extern /*@observer@*/ const char * getGnameS(const char * gname)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/** \ingroup rpmbuild
 * Return cached group id.
 * @todo Implement using hash.
 * @param gname		group name
 * @return		cached gid
 */
gid_t getGidS(const char * gname)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/** \ingroup rpmbuild
 * Return build hostname.
 * @return		build hostname
 */
/*@observer@*/
extern const char * buildHost(void)
	/*@*/;

/** \ingroup rpmbuild
 * Return build time stamp.
 * @return		build time stamp
 */
/*@observer@*/
extern rpmuint32_t * getBuildTime(void)
	/*@*/;

/** \ingroup rpmbuild
 * Read next line from spec file.
 * @param spec		spec file control structure
 * @param strip		truncate comments?
 * @return		0 on success, 1 on EOF, <0 on error
 */
int readLine(Spec spec, rpmStripFlags strip)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->fileStack, spec->readStack, spec->line, spec->lineNum,
		spec->lbufPtr,
		spec->nextline, spec->nextpeekc, spec->lbuf, spec->sl,
		rpmGlobalMacroContext, fileSystem, internalState  @*/;

/** \ingroup rpmbuild
 * Stop reading from spec file, freeing resources.
 * @param spec		spec file control structure
 */
void closeSpec(/*@partial@*/ Spec spec)
	/*@globals fileSystem, internalState @*/
	/*@modifies spec->fileStack, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Truncate comment lines.
 * @param s		skip white space, truncate line at '#'
 */
void handleComments(char * s)
	/*@modifies s @*/;

/** \ingroup rpmbuild
 * Check line for section separator, return next parser state.
 * @param spec		spec file control structure
 * @return		next parser state
 */
rpmParseState isPart(Spec spec)
	/*@modifies spec->foo, spec->nfoo @*/;

/** \ingroup rpmbuild
 * Parse a number.
 * @param		line from spec file
 * @retval res		pointer to int
 * @return		0 on success, 1 on failure
 */
int parseNum(/*@null@*/ const char * line, /*@null@*/ /*@out@*/rpmuint32_t * res)
	/*@modifies *res @*/;

/** \ingroup rpmbuild
 * Add changelog entry to header.
 * @todo addChangelogEntry should be static.
 * @param h		header
 * @param time		time of change
 * @param name		person who made the change
 * @param text		description of change
 */
void addChangelogEntry(Header h, time_t time, const char * name,
		const char * text)
	/*@modifies h @*/;

/** \ingroup rpmbuild
 * Parse %%build/%%install/%%clean section(s) of a spec file.
 * @param spec		spec file control structure
 * @param parsePart	current rpmParseState
 * @return		>= 0 next rpmParseState, < 0 on error
 */
int parseBuildInstallClean(Spec spec, rpmParseState parsePart)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->build, spec->install, spec->check, spec->clean,
		spec->macros, spec->foo, spec->nfoo, spec->lbufPtr,
		spec->fileStack, spec->readStack, spec->line, spec->lineNum,
		spec->nextline, spec->nextpeekc, spec->lbuf, spec->sl,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Parse %%changelog section of a spec file.
 * @param spec		spec file control structure
 * @return		>= 0 next rpmParseState, < 0 on error
 */
int parseChangelog(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->fileStack, spec->readStack, spec->line, spec->lineNum,
		spec->foo, spec->nfoo, spec->lbufPtr,
		spec->nextline, spec->nextpeekc, spec->lbuf, spec->sl,
		spec->packages->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Parse %%description section of a spec file.
 * @param spec		spec file control structure
 * @return		>= 0 next rpmParseState, < 0 on error
 */
int parseDescription(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->packages,
		spec->foo, spec->nfoo, spec->lbufPtr,
		spec->fileStack, spec->readStack, spec->line, spec->lineNum,
		spec->nextline, spec->nextpeekc, spec->lbuf, spec->sl,
		spec->st,
		rpmGlobalMacroContext, fileSystem, internalState  @*/;

/** \ingroup rpmbuild
 * Parse %%files section of a spec file.
 * @param spec		spec file control structure
 * @return		>= 0 next rpmParseState, < 0 on error
 */
int parseFiles(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->packages,
		spec->foo, spec->nfoo, spec->lbufPtr,
		spec->fileStack, spec->readStack, spec->line, spec->lineNum,
		spec->nextline, spec->nextpeekc, spec->lbuf, spec->sl,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Parse tags from preamble of a spec file.
 * @param spec		spec file control structure
 * @param initialPackage
 * @return		>= 0 next rpmParseState, < 0 on error
 */
int parsePreamble(Spec spec, int initialPackage)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies spec->packages,
		spec->foo, spec->nfoo, spec->lbufPtr,
		spec->fileStack, spec->readStack, spec->line, spec->lineNum,
		spec->buildSubdir,
		spec->macros, spec->st,
		spec->sources, spec->numSources, spec->noSource,
		spec->sourceHeader, spec->BANames, spec->BACount,
		spec->nextline, spec->nextpeekc, spec->lbuf, spec->sl,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Parse %%prep section of a spec file.
 * @param spec		spec file control structure
 * @param verify	verify existence of sources/patches?
 * @return		>= 0 next rpmParseState, < 0 on error
 */
int parsePrep(Spec spec, int verify)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->prep, spec->buildSubdir, spec->macros,
		spec->foo, spec->nfoo, spec->lbufPtr,
		spec->fileStack, spec->readStack, spec->line, spec->lineNum,
		spec->nextline, spec->nextpeekc, spec->lbuf, spec->sl,
		spec->packages->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Parse dependency relations from spec file and/or autogenerated output buffer.
 * @param spec		spec file control structure
 * @param pkg		package control structure
 * @param field		text to parse (e.g. "foo < 0:1.2-3, bar = 5:6.7")
 * @param tagN		tag, identifies type of dependency
 * @param index		(0 always)
 * @param tagflags	dependency flags already known from context
 * @return		RPMRC_OK on success
 */
rpmRC parseRCPOT(Spec spec, Package pkg, const char * field, rpmTag tagN,
		rpmuint32_t index, rpmsenseFlags tagflags)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/** \ingroup rpmbuild
 * Parse %%pre et al scriptlets from a spec file.
 * @param spec		spec file control structure
 * @param parsePart	current rpmParseState
 * @return		>= 0 next rpmParseState, < 0 on error
 */
int parseScript(Spec spec, int parsePart)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->packages,
		spec->foo, spec->nfoo, spec->lbufPtr,
		spec->fileStack, spec->readStack, spec->line, spec->lineNum,
		spec->nextline, spec->nextpeekc, spec->lbuf, spec->sl,
		rpmGlobalMacroContext, fileSystem, internalState  @*/;

/** \ingroup rpmbuild
 * Evaluate boolean expression.
 * @param spec		spec file control structure
 * @param expr		expression to parse
 * @return
 */
int parseExpressionBoolean(Spec spec, const char * expr)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/** \ingroup rpmbuild
 * Evaluate string expression.
 * @param spec		spec file control structure
 * @param expr		expression to parse
 * @return
 */
/*@unused@*/ /*@null@*/
char * parseExpressionString(Spec spec, const char * expr)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/** \ingroup rpmbuild
 * Run a build script, assembled from spec file scriptlet section.
 *
 * @param spec		spec file control structure
 * @param what		type of script
 * @param name		name of scriptlet section
 * @param iob		lines that compose script body
 * @param test		don't execute scripts or package if testing
 * @return		RPMRC_OK on success, RPMRC_FAIL on failure
 */
rpmRC doScript(Spec spec, int what, /*@null@*/ const char * name,
		/*@null@*/ rpmiob iob, int test)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies spec->macros,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Find sub-package control structure by name.
 * @param spec		spec file control structure
 * @param name		(sub-)package name
 * @param flag		if PART_SUBNAME, then 1st package name is prepended
 * @retval pkg		package control structure
 * @return		RPMRC_OK on success
 */
rpmRC lookupPackage(Spec spec, /*@null@*/ const char * name, int flag,
		/*@out@*/ Package * pkg)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies spec->packages, *pkg, rpmGlobalMacroContext,
		internalState @*/;

/** \ingroup rpmbuild
 * Destroy all packages associated with spec file.
 * @param packages	package control structure chain
 * @return		NULL
 */
/*@null@*/
Package freePackages(/*@only@*/ /*@null@*/ Package packages)
	/*@globals fileSystem @*/
	/*@modifies packages, fileSystem @*/;

/** \ingroup rpmbuild
 * Destroy a package control structure.
 * @todo Rename to pkgFree.
 * @param pkg		package control structure
 * @return		NULL on last dereference
 */
/*@null@*/
Package freePackage(/*@killref@*/ /*@null@*/ Package pkg)
	/*@globals fileSystem @*/
	/*@modifies pkg, fileSystem @*/;
#define	freePackage(_pkg)	\
    ((Package)rpmioFreePoolItem((rpmioItem)(_pkg), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmbuild
 * Create and initialize package control structure.
 * @param spec		spec file control structure
 * @return		package control structure
 */
/*@only@*/
Package newPackage(Spec spec)
	/*@modifies spec->packages, spec->packages->next @*/;

/** \ingroup rpmbuild
 * Add dependency to header, filtering duplicates.
 * @param spec		spec file control structure
 * @param h		header
 * @param tagN		tag, identifies type of dependency
 * @param N		(e.g. Requires: foo < 0:1.2-3, "foo")
 * @param EVR		(e.g. Requires: foo < 0:1.2-3, "0:1.2-3")
 * @param Flags		(e.g. Requires: foo < 0:1.2-3, both "Requires:" and "<")
 * @param index		(0 always)
 * @return		0 always
 */
int addReqProv(/*@unused@*/Spec spec, Header h, rpmTag tagN,
		const char * N, const char * EVR, rpmsenseFlags Flags,
		rpmuint32_t index)
	/*@globals internalState @*/
	/*@modifies h, internalState @*/;

/**
 * Append files (if any) to scriptlet tags.
 * @param spec		spec file control structure
 * @param pkg		package control structure
 * @return		RPMRC_OK on success
 */
rpmRC processScriptFiles(Spec spec, Package pkg)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies pkg->header, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/**
 * Retrofit an explicit Provides: N = E:V-R dependency into package headers.
 * Up to rpm 3.0.4, packages implicitly provided their own name-version-release.
 * @param h             header
 */
void providePackageNVR(Header h)
	/*@globals internalState @*/
	/*@modifies h, internalState @*/;

/** \ingroup rpmbuild
 * Add rpmlib feature dependency.
 * @param h		header
 * @param feature	rpm feature name (i.e. "rpmlib(Foo)" for feature Foo)
 * @param featureEVR	rpm feature epoch/version/release
 * @return		0 always
 */
int rpmlibNeedsFeature(Header h, const char * feature, const char * featureEVR)
	/*@globals internalState @*/
	/*@modifies h, internalState @*/;

/** \ingroup rpmbuild
 * Post-build processing for binary package(s).
 * @param spec		spec file control structure
 * @param installSpecialDoc
 * @param test		don't execute scripts or package if testing
 * @return		RPMRC_OK on success
 */
rpmRC processBinaryFiles(Spec spec, int installSpecialDoc, int test)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->macros, *spec->packages,
		spec->packages->fi, spec->packages->fileList,
		spec->packages->specialDoc, spec->packages->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Create and initialize header for source package.
 * @param spec		spec file control structure
 * @retval *sfp		srpm file list (may be NULL)
 * @return		0 always
 */
int initSourceHeader(Spec spec, /*@null@*/ rpmiob *sfp)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies spec->sourceHeader, spec->sourceHdrInit,
		spec->BANames, *sfp,
		spec->packages->header,
		rpmGlobalMacroContext, internalState @*/;

/** \ingroup rpmbuild
 * Post-build processing for source package.
 * @param spec		spec file control structure
 * @return		0 on success
 */
int processSourceFiles(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->sourceHeader, spec->sourceCpioList,
		spec->BANames, spec->sourceHdrInit,
		spec->packages->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Parse spec file into spec control structure.
 * @param ts		transaction set (spec file control in ts->spec)
 * @param specFile
 * @param rootURL
 * @param recursing	parse is recursive?
 * @param passPhrase
 * @param cookie
 * @param anyarch
 * @param force
 * @param verify
 * @return
 */
int parseSpec(rpmts ts, const char * specFile,
		/*@null@*/ const char * rootURL,
		int recursing,
		/*@null@*/ const char * passPhrase,
		/*@null@*/ const char * cookie,
		int anyarch, int force, int verify)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Build stages state machine driver.
 * @param ts		transaction set
 * @param spec		spec file control structure
 * @param what		bit(s) to enable stages of build
 * @param test		don't execute scripts or package if testing
 * @return		RPMRC_OK on success
 */
rpmRC buildSpec(rpmts ts, Spec spec, int what, int test)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->sourceHeader, spec->sourceCpioList, spec->cookie,
		spec->sourceRpmName, spec->sourcePkgId, spec->sourceHdrInit,
		spec->macros, spec->BASpecs,
		spec->BANames, *spec->packages,
		spec->packages->fi, spec->packages->fileList,
		spec->packages->specialDoc, spec->packages->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Generate binary package(s).
 * @param spec		spec file control structure
 * @return		rpmRC on success
 */
rpmRC packageBinaries(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->packages->header, spec->packages->fi,
		spec->sourceRpmName, spec->cookie, spec->sourcePkgId,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * Generate source package.
 * @param spec		spec file control structure
 * @return		RPMRC_OK on success
 */
rpmRC packageSources(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->sourceHeader, spec->cookie, spec->sourceCpioList,
		spec->sourceRpmName, spec->sourcePkgId, spec->packages->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/*@=redecl@*/
#ifdef __cplusplus
}
#endif

#endif	/* _H_RPMBUILD_ */
