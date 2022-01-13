#ifndef __QNX_LINUX_LIMITS_H
#define __QNX_LINUX_LIMITS_H

#define NR_OPEN	        1024

#ifndef NGROUPS_MAX
#define NGROUPS_MAX    65536	/* supplemental group IDs are available */
#endif
#ifndef ARG_MAX
#define ARG_MAX       131072	/* # bytes of args + environ for exec() */
#endif
#ifndef LINK_MAX
#define LINK_MAX         127	/* # links a file may have */
#endif
#define MAX_CANON        255	/* size of the canonical input queue */
#define MAX_INPUT        255	/* size of the type-ahead buffer */
#define NAME_MAX         255	/* # chars in a file name */
#ifndef PATH_MAX
#define PATH_MAX        4096	/* # chars in a path name including nul */
#endif
#ifndef PIPE_BUF
#define PIPE_BUF        4096	/* # bytes in atomic write to a pipe */
#endif
#define XATTR_NAME_MAX   255	/* # chars in an extended attribute name */
#define XATTR_SIZE_MAX 65536	/* size of an extended attribute value (64k) */
#define XATTR_LIST_MAX 65536	/* size of extended attribute namelist (64k) */

#define RTSIG_MAX	  32


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/limits.h $ $Rev: 836322 $")
#endif
