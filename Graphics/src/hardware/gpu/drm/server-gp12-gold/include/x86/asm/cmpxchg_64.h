#ifndef _ASM_X86_CMPXCHG_64_H
#define _ASM_X86_CMPXCHG_64_H

static inline void set_64bit(volatile u64 *ptr, u64 val)
{
	*ptr = val;
}

#define cmpxchg64(ptr, o, n)						\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);				\
	cmpxchg((ptr), (o), (n));					\
})

#endif /* _ASM_X86_CMPXCHG_64_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/cmpxchg_64.h $ $Rev: 836935 $")
#endif
