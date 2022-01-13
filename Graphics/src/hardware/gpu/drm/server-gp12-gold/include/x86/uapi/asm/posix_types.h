#ifndef __KERNEL__
# ifdef __i386__
#  include <asm/posix_types_32.h>
# elif defined(__ILP32__)
#  include <asm/posix_types_x32.h>
# else
#  include <asm/posix_types_64.h>
# endif
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/uapi/asm/posix_types.h $ $Rev: 836322 $")
#endif
