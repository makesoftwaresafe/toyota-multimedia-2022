#ifndef _UAPI__ASM_GENERIC_PARAM_H
#define _UAPI__ASM_GENERIC_PARAM_H

#ifndef HZ
#define HZ 100
#endif

#ifndef EXEC_PAGESIZE
#define EXEC_PAGESIZE	4096
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */


#endif /* _UAPI__ASM_GENERIC_PARAM_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/asm-generic/param.h $ $Rev: 838597 $")
#endif
