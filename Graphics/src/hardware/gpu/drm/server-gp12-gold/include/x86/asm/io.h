#ifndef __X86_ASM_IO_H__
#define __X86_ASM_IO_H__

#define IO_SPACE_LIMIT 0xffff

extern void iounmap(void *addr);
extern void* ioremap_wc(resource_size_t offset, unsigned long size);
extern void *ioremap(resource_size_t offset, unsigned long size);
extern void* ioremap_nocache(resource_size_t offset, unsigned long size);

static inline void flush_write_buffers(void)
{
#if defined(CONFIG_X86_PPRO_FENCE)
	asm volatile("lock; addl $0,0(%%esp)": : :"memory");
#endif
}

#endif /* __X86_ASM_IO_H__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/io.h $ $Rev: 845340 $")
#endif
