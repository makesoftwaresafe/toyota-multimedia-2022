#ifndef _UAPI_ASM_X86_UNISTD_H
#define _UAPI_ASM_X86_UNISTD_H

/* x32 syscall flag bit */
#define __X32_SYSCALL_BIT	0x40000000

#ifndef __KERNEL__
# ifdef __i386__
#  include <asm/unistd_32.h>
# elif defined(__ILP32__)
#  include <asm/unistd_x32.h>
# else
#  include <asm/unistd_64.h>
# endif
#endif

#endif /* _UAPI_ASM_X86_UNISTD_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/uapi/asm/unistd.h $ $Rev: 836322 $")
#endif
