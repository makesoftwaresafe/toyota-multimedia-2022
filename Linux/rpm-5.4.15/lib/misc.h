#ifndef H_MISC
#define H_MISC

/**
 * \file lib/misc.h
 */

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create directory if it does not exist, and make sure path is writable.
 * @note This will only create last component of directory path.
 * @param dpath		directory path
 * @param dname		directory use string
 * @return		rpmRC return code
 */
rpmRC rpmMkdirPath (const char * dpath, const char * dname)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Like the libc function, but malloc()'s the space needed.
 * @deprecated Use setenv(3) instead.
 * @param name		variable name
 * @param value		variable value
 * @param overwrite	should an existing variable be changed?
 * @return		0 on success
 */
int dosetenv(const char * name, const char * value, int overwrite)
	/*@globals environ@*/
	/*@modifies *environ @*/;

/**
 * Like the libc function, but malloc()'s the space needed.
 * @deprecated Use setenv(3) instead.
 * @param str		"name=value" string
 * @return		0 on success
 */
int doputenv(const char * str)
	/*@globals environ@*/
	/*@modifies *environ @*/;

/**
 * Return (malloc'd) current working directory.
 * @return		current working directory (malloc'ed)
 */
/*@only@*/ char * currentDirectory(void)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_MISC */
