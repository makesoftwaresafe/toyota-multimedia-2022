#ifndef _LINUX_SWAB_H
#define _LINUX_SWAB_H

#include <uapi/linux/swab.h>

# define swab16 __swab16
# define swab32 __swab32
# define swab64 __swab64
# define swahw32 __swahw32
# define swahb32 __swahb32
# define swab16p __swab16p
# define swab32p __swab32p
# define swab64p __swab64p
# define swahw32p __swahw32p
# define swahb32p __swahb32p
# define swab16s __swab16s
# define swab32s __swab32s
# define swab64s __swab64s
# define swahw32s __swahw32s
# define swahb32s __swahb32s
#endif /* _LINUX_SWAB_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/swab.h $ $Rev: 838597 $")
#endif
