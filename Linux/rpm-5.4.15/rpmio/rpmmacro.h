#ifndef _H_MACRO_
#define	_H_MACRO_

/** \ingroup rpmio
 * \file rpmio/rpmmacro.h
 */
#include <stdio.h>

typedef /*@abstract@*/ struct MacroEntry_s * MacroEntry;
typedef /*@abstract@*/ struct MacroContext_s * MacroContext;

#if defined(_MACRO_INTERNAL)
#include <rpmiotypes.h>
/*! The structure used to store a macro. */
struct MacroEntry_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    struct MacroEntry_s *prev;	/*!< Macro entry stack. */
    const char *name;		/*!< Macro name. */
    const char *opts;		/*!< Macro parameters (a la getopt) */
    const char *body;		/*!< Macro body. */
    int	used;			/*!< No. of expansions. */
    short level;		/*!< Scoping level. */
    unsigned short flags;	/*!< Flags. */
};

/*! The structure used to store the set of macros in a context. */
struct MacroContext_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@owned@*//*@null@*/
    MacroEntry *macroTable;	/*!< Macro entry table for context. */
    int	macrosAllocated;	/*!< No. of allocated macros. */
    int	firstFree;		/*!< No. of macros. */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif

/*@-redecl@*/
/*@checked@*/
extern MacroContext rpmGlobalMacroContext;

/*@checked@*/
extern MacroContext rpmCLIMacroContext;

/** \ingroup rpmrc
 * List of macro files to read when configuring rpm.
 * This is a colon separated list of files. URI's are permitted as well,
 * identified by the token '://', so file paths must not begin with '//'.
 */
/*@observer@*/ /*@checked@*/
extern const char * rpmMacrofiles;
/*@=redecl@*/

/**
 * Markers for sources of macros added throughout rpm.
 */
#define	RMIL_DEFAULT	-15
#define	RMIL_MACROFILES	-13
#define	RMIL_RPMRC	-11

#define	RMIL_CMDLINE	-7
#define	RMIL_TARBALL	-5
#define	RMIL_SPEC	-3
#define	RMIL_OLDSPEC	-1
#define	RMIL_GLOBAL	0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Print macros to file stream.
 * @param mc		macro context (NULL uses global context).
 * @param fp		file stream (NULL uses stderr).
 */
void rpmDumpMacroTable(/*@null@*/ MacroContext mc, /*@null@*/ FILE * fp)
	/*@globals rpmGlobalMacroContext, fileSystem @*/
	/*@modifies *fp, fileSystem @*/;

/**
 * Return macro entries as string array.
 * @param mc		macro context (NULL uses global context)
 * @param _mire		pattern to match (NULL disables)
 * @param used		macro usage (<0 all, =0 unused, >=1 used count)
 * @retval *avp		macro definitions
 * @return		no. of entries
 */
int
rpmGetMacroEntries(/*@null@*/ MacroContext mc, /*@null@*/ void * _mire,
		int used, /*@null@*/ const char *** avp)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies _mire, *avp @*/;

/**
 * Check whether configuration file is moderately secure to load.
 * @param filename	filename to check
 * @return		1 on success, 0 on failure
 * XXX FIXME: rpmRC return?
 */
int rpmSecuritySaneFile(const char *filename)
	/*@globals internalState @*/;

/**
 * Return URL path(s) from a (URL prefixed) pattern glob.
 * @param patterns	glob pattern
 * @retval *argcPtr	no. of paths
 * @retval *argvPtr	array of paths (malloc'd contiguous blob)
 * @return		0 on success
 * XXX FIXME: rpmRC return?
 */
int rpmGlob(const char * patterns, /*@out@*/ int * argcPtr,
		/*@out@*/ const char *** argvPtr)
	/*@globals fileSystem, internalState @*/
	/*@modifies *argcPtr, *argvPtr, fileSystem, internalState @*/;

/**
 * Expand macro into buffer.
 * @deprecated Use rpmExpand().
 * @todo Eliminate from API.
 * @param spec		cookie (unused)
 * @param mc		macro context (NULL uses global context).
 * @retval sbuf		input macro to expand, output expansion
 * @param slen		size of buffer
 * @return		0 on success
 */
int expandMacros(/*@null@*/ void * spec, /*@null@*/ MacroContext mc,
		/*@in@*/ /*@out@*/ char * sbuf, size_t slen)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *sbuf, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Add macro to context.
 * @deprecated Use rpmDefineMacro().
 * @param mc		macro context (NULL uses global context).
 * @param n		macro name
 * @param o		macro paramaters
 * @param b		macro body
 * @param level		macro recursion level (0 is entry API)
 */
void addMacro(/*@null@*/ MacroContext mc, const char * n,
		/*@null@*/ const char * o, /*@null@*/ const char * b, int level)
	/*@globals rpmGlobalMacroContext, internalState @*/
	/*@modifies mc, rpmGlobalMacroContext, internalState @*/;

/**
 * Delete macro from context.
 * @param mc		macro context (NULL uses global context).
 * @param n		macro name
 */
void delMacro(/*@null@*/ MacroContext mc, const char * n)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies mc, rpmGlobalMacroContext @*/;

/**
 * Define macro in context.
 * @param mc		macro context (NULL uses global context).
 * @param macro		macro name, options, body
 * @param level		macro recursion level (0 is entry API)
 * @return		@todo Document.
 */
int rpmDefineMacro(/*@null@*/ MacroContext mc, const char * macro, int level)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies mc, rpmGlobalMacroContext, internalState @*/;

/**
 * Undefine macro in context.
 * @param mc		macro context (NULL uses global context).
 * @param macro		macro name
 * @return		@todo Document.
 */
int rpmUndefineMacro(/*@null@*/ MacroContext mc, const char * macro)
	/*@globals rpmGlobalMacroContext, internalState @*/
	/*@modifies mc, rpmGlobalMacroContext, internalState @*/;

/**
 * Load macros from specific context into global context.
 * @param mc		macro context (NULL does nothing).
 * @param level		macro recursion level (0 is entry API)
 */
void rpmLoadMacros(/*@null@*/ MacroContext mc, int level)
	/*@globals rpmGlobalMacroContext, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/**
 * Load macro context from a macro file.
 * @param mc		(unused)
 * @param fn		macro file name
 * @param nesting	max load recursion depth, 0 disables.
 */
int rpmLoadMacroFile(/*@null@*/ MacroContext mc, const char * fn, int nesting)
	/*@globals rpmGlobalMacroContext,
		h_errno, fileSystem, internalState @*/
	/*@modifies mc, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Initialize macro context from set of macrofile(s).
 * @param mc		macro context
 * @param macrofiles	colon separated list of macro files (NULL does nothing)
 */
void rpmInitMacros(/*@null@*/ MacroContext mc, const char * macrofiles)
	/*@globals rpmGlobalMacroContext, rpmCLIMacroContext,
		h_errno, fileSystem, internalState @*/
	/*@modifies mc, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Destroy macro context.
 * @param mc		macro context (NULL uses global context).
 */
void rpmFreeMacros(/*@null@*/ MacroContext mc)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies mc, rpmGlobalMacroContext @*/;

typedef enum rpmCompressedMagic_e {
    COMPRESSED_NOT		= 0,	/*!< not compressed */
    COMPRESSED_OTHER		= 1,	/*!< gzip can handle */
    COMPRESSED_BZIP2		= 2,	/*!< bzip2 can handle */
    COMPRESSED_ZIP		= 3,	/*!< unzip can handle */
    COMPRESSED_LZOP		= 4,	/*!< lzop can handle */
    COMPRESSED_LZMA		= 5,	/*!< lzma can handle */
    COMPRESSED_XZ		= 6,	/*!< xz can handle */
    COMPRESSED_LRZIP		= 7,	/*!< lrzip can handle */
    COMPRESSED_LZIP		= 8,	/*!< lzip can handle */
    COMPRESSED_7ZIP		= 9	/*!< 7zip can handle */
} rpmCompressedMagic;

/**
 * Return type of compression used in file.
 * @param file		name of file
 * @retval compressed	address of compression type
 * @return		0 on success, 1 on I/O error
 */
int isCompressed(const char * file, /*@out@*/ rpmCompressedMagic * compressed)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies *compressed, fileSystem, internalState @*/;

/**
 * Return (malloc'ed) concatenated macro expansion(s).
 * @param arg		macro(s) to expand (NULL terminates list)
 * @return		macro expansion (malloc'ed)
 */
char * rpmExpand(/*@null@*/ const char * arg, ...)
#if defined(__GNUC__) && __GNUC__ >= 4
	/* issue a warning if the list is not  NULL-terminated */
	__attribute__((sentinel))
#endif
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/**
 * Return (malloc'ed) concatenated macro expansion(s) in a context.
 * @param mc		macro context
 * @param arg		macro(s) to expand (NULL terminates list)
 * @return		macro expansion (malloc'ed)
 */
char * rpmMCExpand(/*@null@*/ MacroContext mc, /*@null@*/ const char * arg, ...)
#if defined(__GNUC__) && __GNUC__ >= 4
	/* issue a warning if the list is not  NULL-terminated */
	__attribute__((sentinel))
#endif
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/**
 * Canonicalize file path.
 * @note Path canonicalization always removes the pesky trailing '/'.
 * @param path		path to canonicalize (in-place)
 * @return		pointer to path
 */
/*@null@*/
char * rpmCleanPath(/*@returned@*/ /*@null@*/ char * path)
	/*@modifies *path @*/;

/**
 * Return (malloc'ed) expanded, canonicalized, file path.
 * @note A final "/" argument will force append a pesky trailing '/'.
 * @param path		macro(s) to expand (NULL terminates list)
 * @return		canonicalized path (malloc'ed)
 */
/*@-redecl@*/ /* LCL: shrug */
char * rpmGetPath(/*@null@*/ const char * path, ...)
#if defined(__GNUC__) && __GNUC__ >= 4
	/* issue a warning if the list is not  NULL-terminated */
	 __attribute__((sentinel))
#endif
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;
/*@=redecl@*/

/**
 * Merge 3 args into path, any or all of which may be a url.
 * The leading part of the first URL encountered is used
 * for the result, other URL prefixes are discarded, permitting
 * a primitive form of URL inheiritance.
 * @param urlroot	root URL (often path to chroot, or NULL)
 * @param urlmdir	directory URL (often a directory, or NULL)
 * @param urlfile	file URL (often a file, or NULL)
 * @return		expanded, merged, canonicalized path (malloc'ed)
 */
/*@-redecl@*/ /* LCL: shrug */
const char * rpmGenPath(/*@null@*/ const char * urlroot,
			/*@null@*/ const char * urlmdir,
			/*@null@*/ const char * urlfile)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;
/*@=redecl@*/

/**
 * Return macro expansion as a numeric value.
 * Boolean values ('Y' or 'y' returns 1, 'N' or 'n' returns 0)
 * are permitted as well. An undefined macro returns 0.
 * @param arg		macro to expand
 * @return		numeric value
 */
int rpmExpandNumeric (const char * arg)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* _H_ MACRO_ */
