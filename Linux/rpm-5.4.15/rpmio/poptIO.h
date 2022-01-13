#ifndef H_POPTIO
#define	H_POPTIO

/** \ingroup rpmio
 * \file rpmio/poptIO.h
 */

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmcb.h>
#include <rpmmacro.h>
#include <rpmmg.h>
#include <rpmpgp.h>
#include <rpmsw.h>
#include <rpmurl.h>
#include <argv.h>
#include <fts.h>
#include <mire.h>
#include <popt.h>

/*@unchecked@*/
extern int __debug;	/* XXX I know I'm gonna regret __debug ... */

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmio
 */

/*@unchecked@*/
extern int _rpmio_popt_context_flags;	/* XXX POPT_CONTEXT_POSIXMEHARDER */

/*@unchecked@*/
extern pgpHashAlgo rpmioDigestHashAlgo;

/** \ingroup rpmio
 * Popt option table for options to select digest algorithm.
 */
/*@unchecked@*/ /*@observer@*/
extern struct poptOption		rpmioDigestPoptTable[];

/** \ingroup rpmio
 * Popt option table for options shared by all modes and executables.
 */
/*@unchecked@*/
extern struct poptOption		rpmioAllPoptTable[];

/*@unchecked@*/
extern int rpmioFtsOpts;

/** \ingroup rpmio
 * Popt option table for options to set Fts(3) options.
 */
/*@unchecked@*/
extern struct poptOption		rpmioFtsPoptTable[];

/*@unchecked@*/ /*@observer@*/ /*@null@*/
extern const char * rpmioPipeOutput;

/*@unchecked@*/ /*@observer@*/ /*@null@*/
extern const char * rpmioRootDir;

/**
 * Initialize most everything needed by an rpmio executable context.
 * @param argc			no. of args
 * @param argv			arg array
 * @param optionsTable		popt option table
 * @return			popt context (or NULL)
 */
/*@null@*/
poptContext
rpmioInit(int argc, char *const argv[], struct poptOption * optionsTable)
	/*@globals stderr, fileSystem, internalState @*/
	/*@modifies stderr, fileSystem, internalState @*/;

/**
 * Make sure that rpm configuration has been read.
 * @warning Options like --rcfile and --verbose must precede callers option.
 */
/*@mayexit@*/
void rpmioConfigured(void)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/**
 * Destroy most everything needed by an rpm CLI executable context.
 * @param optCon		popt context
 * @return			NULL always
 */
poptContext
rpmioFini(/*@only@*/ /*@null@*/ poptContext optCon)
	/*@globals rpmGlobalMacroContext,
		fileSystem, internalState @*/
	/*@modifies optCon, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_POPTIO */
