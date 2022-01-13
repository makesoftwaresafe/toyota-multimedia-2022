#ifndef H_FILETRIGGERS
#define H_FILETRIGGERS

/**
 * \file lib/filetriggers.h
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 */
__attribute__ ((visibility("hidden")))
int mayAddToFilesAwaitingFiletriggers(const char *rootDir, rpmfi fi,
		int install_or_erase)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies fi, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 */
void rpmRunFileTriggers(const char *rootDir)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_FILETRIGGERS */
