#ifndef H_RPMDPKG
#define H_RPMDPKG

/** \ingroup rpmds
 * \file rpmdb/rpmdpkg.h
 * Structure(s) and routine(s) used for dpkg EVR parsing and comparison.
 */

#include "rpmevr.h"

/**
 */
/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmdpkg_debug;
/*@=exportlocal@*/

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmds
 * Segmented string compare.
 * @param a		1st string
 * @param b		2nd string
 * @return		+1 if a is "newer", 0 if equal, -1 if b is "newer"
 */
int dpkgEVRcmp(/*@null@*/ const char *a, /*@null@*/ const char *b)
	/*@*/;

/** \ingroup rpmds
 * Split EVR string into epoch, version, and release components.
 * @param evrstr	[epoch:]version[-release] string
 * @retval *evr		parse results
 * @return		0 always
 */
int dpkgEVRparse(const char * evrstr, EVR_t evr)
	/*@modifies evrstr, evr @*/;

/** \ingroup rpmds
 * Compare EVR containers.
 * @param a		1st EVR container
 * @param b		2nd EVR container
 * @return		+1 if a is "newer", 0 if equal, -1 if b is "newer"
 */
int dpkgEVRcompare(const EVR_t a, const EVR_t b)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMDPKG */
