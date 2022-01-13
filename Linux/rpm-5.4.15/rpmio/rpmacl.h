#ifndef	H_RPMACL
#define	H_RPMACL

/** \ingroup rpmio
 * \file rpmio/rpmacl.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

#ifdef __cplusplus
extern "C" {
#endif

rpmRC rpmaclCopyFd(FD_t ifd, FD_t ofd)
	/*@*/;

rpmRC rpmaclCopyDir(const char * sdn, const char * tdn, mode_t mode)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMACL */
