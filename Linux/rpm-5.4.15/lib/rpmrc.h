#ifndef H_RPMRC
#define	H_RPMRC

/**
 * \file lib/rpmrc.h
 */

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmrc
 * Read macro configuration file(s) for a target.
 * @param file		NULL always
 * @param target	target platform (NULL uses default)
 * @return		0 on success, -1 on error
 */
int rpmReadConfigFiles(/*@null@*/ const char * file,
		/*@null@*/ const char * target)
	/*@globals rpmGlobalMacroContext, rpmCLIMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, rpmCLIMacroContext,
		fileSystem, internalState @*/;

/*@only@*/ /*@null@*/ /*@unchecked@*/
extern void * platpat;
/*@unchecked@*/
extern int nplatpat;

/** \ingroup rpmrc
 * Return score of a platform string.
 * A platform score measures the "nearness" of a platform string wrto
 * configured platform patterns. The returned score is the line number
 * of the 1st pattern in /etc/rpm/platform that matches the input string.
 *
 * @param platform	cpu-vendor-os platform string
 * @param mi_re		pattern array (NULL uses /etc/rpm/platform patterns)
 * @param mi_nre	no. of patterns
 * @return		platform score (0 is no match, lower is preferred)
 */
int rpmPlatformScore(const char * platform, /*@null@*/ void * mi_re, int mi_nre)
	/*@modifies mi_re @*/;

/** \ingroup rpmrc
 * Display current rpmrc (and macro) configuration.
 * @param fp		output file handle
 * @return		0 always
 */
int rpmShowRC(FILE * fp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *fp, rpmGlobalMacroContext, fileSystem, internalState  @*/;

/** \ingroup rpmrc
 * @todo	Eliminate in rpm-5.1.
 * Destroy rpmrc arch/os compatibility tables.
 * @todo Eliminate from API.
 */
void rpmFreeRpmrc(void)
	/*@globals platpat, nplatpat, internalState @*/
	/*@modifies platpat, nplatpat, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMRC */
