#ifndef H_RPMLIB
#define	H_RPMLIB

/** \ingroup rpmcli rpmrc rpmts rpmte rpmds rpmfi rpmdb lead signature header payload dbi
 * \file lib/rpmlib.h
 *
 * In Memoriam: Steve Taylor <staylor@redhat.com> was here, now he's not.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================== */
/** \name RPMTS */
/*@{*/

/**
 * Install source package.
 * @deprecated	This routine needs to DIE! DIE! DIE!.
 * @todo	Eliminate in rpm-5.1, insturment rpmtsRun() state machine instead.
 * @param ts		transaction set
 * @param _fd		file handle
 * @retval specFilePtr	address of spec file name (or NULL)
 * @retval cookie	address of cookie pointer (or NULL)
 * @return		rpmRC return code
 */
rpmRC rpmInstallSourcePackage(rpmts ts, void * _fd,
			/*@null@*/ /*@out@*/ const char ** specFilePtr,
			/*@null@*/ /*@out@*/ const char ** cookie)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, *specFilePtr, *cookie, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/*@}*/

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMLIB */
