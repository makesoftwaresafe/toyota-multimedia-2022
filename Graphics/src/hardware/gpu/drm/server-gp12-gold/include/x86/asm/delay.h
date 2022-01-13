#ifndef _ASM_X86_DELAY_H
#define _ASM_X86_DELAY_H

#include <asm-generic/delay.h>

void use_tsc_delay(void);
void use_mwaitx_delay(void);

#endif /* _ASM_X86_DELAY_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/delay.h $ $Rev: 836322 $")
#endif
