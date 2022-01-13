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
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/x86/asm/string.h $ $Rev: 838597 $")
#endif
