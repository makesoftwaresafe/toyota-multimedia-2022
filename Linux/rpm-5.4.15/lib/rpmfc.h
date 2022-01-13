#ifndef _H_RPMFC_
#define _H_RPMFC_

/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmfc_debug;
/*@=exportlocal@*/

/**
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmfc_s * rpmfc;

/**
 */
typedef struct rpmfcTokens_s * rpmfcToken;

/**
 */
typedef enum FCOLOR_e {
    RPMFC_BLACK			= 0,
    RPMFC_ELF32			= (1 <<  0),
    RPMFC_ELF64			= (1 <<  1),
    RPMFC_ELFMIPSN32		= (1 <<  2),
#define	RPMFC_ELF	(RPMFC_ELF32|RPMFC_ELF64|RPMFC_ELFMIPSN32)
	/* (1 << 3) leaks into package headers, reserved */

	/* bit 4 unused */
    RPMFC_TYPELIB		= (1 <<  5),
    RPMFC_HASKELL		= (1 <<  6),
    RPMFC_RUBY			= (1 <<  7),
    RPMFC_PKGCONFIG		= (1 <<  8),
    RPMFC_LIBTOOL		= (1 <<  9),
    RPMFC_BOURNE		= (1 << 10),
    RPMFC_MONO			= (1 << 11),

    RPMFC_SCRIPT		= (1 << 12),
    RPMFC_STATIC		= (1 << 13),
    RPMFC_NOTSTRIPPED		= (1 << 14),
	/* bit 15 unused */

	/* bits 16-19 are enumerated, not bits */
    RPMFC_DIRECTORY		= (1 << 16),
    RPMFC_SYMLINK		= (2 << 16),
    RPMFC_DEVICE		= (3 << 16),
    RPMFC_LIBRARY		= (4 << 16),
    RPMFC_FONT			= (5 << 16),
    RPMFC_IMAGE			= (6 << 16),
    RPMFC_MANPAGE		= (7 << 16),
    RPMFC_TEXT			= (8 << 16),
    RPMFC_DOCUMENT		= (9 << 16),

    RPMFC_ARCHIVE		= (1 << 20),
    RPMFC_COMPRESSED		= (1 << 21),
    RPMFC_MODULE		= (1 << 22),
    RPMFC_EXECUTABLE		= (1 << 23),

    RPMFC_PERL			= (1 << 24),
    RPMFC_JAVA			= (1 << 25),
    RPMFC_PYTHON		= (1 << 26),
    RPMFC_PHP			= (1 << 27),
    RPMFC_TCL			= (1 << 28),

    RPMFC_WHITE			= (1 << 29),
    RPMFC_INCLUDE		= (1 << 30),
    RPMFC_ERROR			= (1 << 31)
} FCOLOR_t;

#if defined(_RPMFC_INTERNAL)
/**
 */
struct rpmfc_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    size_t nfiles;	/*!< no. of files */
    size_t fknown;	/*!< no. of classified files */
    size_t fwhite;	/*!< no. of "white" files */
    size_t ix;		/*!< current file index */
    int skipProv;	/*!< Don't auto-generate Provides:? */
    int skipReq;	/*!< Don't auto-generate Requires:? */
    int tracked;	/*!< Versioned Provides: tracking dependency added? */
    size_t brlen;	/*!< strlen(spec->buildRoot) */

    ARGV_t fn;		/*!< (no. files) file names */
    ARGI_t fcolor;	/*!< (no. files) file colors */
    ARGI_t fcdictx;	/*!< (no. files) file class dictionary indices */
    ARGI_t fddictx;	/*!< (no. files) file depends dictionary start */
    ARGI_t fddictn;	/*!< (no. files) file depends dictionary no. entries */
    ARGV_t cdict;	/*!< (no. classes) file class dictionary */
    ARGV_t ddict;	/*!< (no. dependencies) file depends dictionary */
    ARGI_t ddictx;	/*!< (no. dependencies) file->dependency mapping */

/*@relnull@*/
    rpmds provides;	/*!< (no. provides) package provides */
/*@relnull@*/
    rpmds requires;	/*!< (no. requires) package requires */

    rpmiob iob_java;	/*!< concatenated list of java colored files. */
    rpmiob iob_perl;	/*!< concatenated list of perl colored files. */
    rpmiob iob_python;	/*!< concatenated list of python colored files. */
    rpmiob iob_php;	/*!< concatenated list of php colored files. */

/*@null@*/
    void * Pmires;	/*!< Filter patterns from %{__noautoprov} */
    int Pnmire;
/*@null@*/
    void * PFmires;	/*!< Filter patterns from %{__noautoprov} */
    int PFnmire;
/*@null@*/
    void * Rmires;	/*!< Filter patterns from %{__noautoreq} */
    int Rnmire;
/*@null@*/
    void * RFmires;	/*!< Filter patterns from %{__noautoreqfile} */
    int RFnmire;

};

/**
 */
struct rpmfcTokens_s {
/*@observer@*/
    const char * token;
    int colors;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return helper output.
 * @param av		helper argv (with possible macros)
 * @param iob_stdin	helper input
 * @retval *iob_stdoutp	helper output
 * @param failnonzero	Is non-zero helper exit status a failure?
 */
int rpmfcExec(const char ** av, rpmiob iob_stdin, /*@out@*/ rpmiob * iob_stdoutp,
		int failnonzero)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *iob_stdoutp, rpmGlobalMacroContext,
		fileSystem, internalState @*/
        /*@requires maxSet(iob_stdoutp) >= 0 @*/;

/**
 * Return file color given file(1) string.
 * @param fmstr		file(1) string
 * @return		file color
 */
/*@-exportlocal@*/
int rpmfcColoring(const char * fmstr)
	/*@*/;
/*@=exportlocal@*/

/**
 * Print results of file classification.
 * @todo Remove debugging routine.
 * @param msg		message prefix (NULL for none)
 * @param fc		file classifier
 * @param fp		output file handle (NULL for stderr)
 */
/*@-exportlocal@*/
void rpmfcPrint(/*@null@*/ const char * msg, rpmfc fc, /*@null@*/ FILE * fp)
	/*@globals fileSystem @*/
	/*@modifies *fp, fc, fileSystem @*/;
/*@=exportlocal@*/

/**
 * Build file class dictionary and mappings.
 * @param fc		file classifier
 * @param argv		files to classify
 * @param fmode		files mode_t array (or NULL)
 * @return		RPMRC_OK on success
 */
/*@-exportlocal@*/
rpmRC rpmfcClassify(rpmfc fc, const char ** argv, /*@null@*/ rpmuint16_t * fmode)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies fc, rpmGlobalMacroContext, fileSystem, internalState @*/;
/*@=exportlocal@*/

/**
 * Build file/package dependency dictionary and mappings.
 * @param fc		file classifier
 * @return		RPMRC_OK on success
 */
/*@-exportlocal@*/
rpmRC rpmfcApply(rpmfc fc)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies fc, rpmGlobalMacroContext, internalState @*/;
/*@=exportlocal@*/

/**
 * Generate package dependencies.
 * @param _spec		spec file control
 * @param _pkg		package control
 * @return		RPMRC_OK on success
 */
rpmRC rpmfcGenerateDepends(void * _spec, void * _pkg)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Unreference a file classifier instance.
 * @param fc		file classifier
 * @return		NULL if free'd
 */
/*@unused@*/ /*@null@*/
rpmfc rpmfcUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmfc fc)
	/*@modifies fc @*/;
#define	rpmfcUnlink(_fc)	\
	((rpmfc)rpmioUnlinkPoolItem((rpmioItem)(_fc), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a file classifier instance.
 * @param fc		file classifier
 * @return		new file classifier reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmfc rpmfcLink (/*@null@*/ rpmfc fc)
	/*@modifies fc @*/;
#define	rpmfcLink(_fc)	\
	((rpmfc)rpmioLinkPoolItem((rpmioItem)(_fc), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a file classifier.
 * @param fc		file classifier
 * @return		NULL if free'd
 */
/*@null@*/
rpmfc rpmfcFree(/*@only@*/ /*@null@*/ rpmfc fc)
	/*@modifies fc @*/;
#define	rpmfcFree(_fc)	\
	((rpmfc)rpmioFreePoolItem((rpmioItem)(_fc), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create a file classifier.
 * @return		new file classifier
 */
/*@-exportlocal@*/
rpmfc rpmfcNew(void)
	/*@*/;
/*@=exportlocal@*/

#ifdef __cplusplus
}
#endif

#endif /* _H_RPMFC_ */
