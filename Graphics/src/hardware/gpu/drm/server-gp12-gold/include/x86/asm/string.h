#ifndef _QNX_LINUX_ASM_STRING_H
#define _QNX_LINUX_ASM_STRING_H

#ifdef CONFIG_X86_32
# include <asm/string_32.h>
#else
# include <asm/string_64.h>
#endif

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/string.h $ $Rev: 836322 $")
#endif
