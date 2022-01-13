#ifndef _QNX_LINUX_IO_H
#define _QNX_LINUX_IO_H

#include <assert.h>
#include <sys/mman.h>
#include <linux/compiler.h>
#include <asm/pgtable_types.h>

static inline int arch_phys_wc_index(int handle)
{
	return -1;
}

#include <hw/inout.h>
#define outb(x,y)  out8(y,x)
#define inb(x)  in8(x)

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

static inline phys_addr_t virt_to_phys(volatile void *address)
{
	phys_addr_t offset = 0;
	if ( mem_offset64((const void *)address, NOFD, PAGE_SIZE, (off64_t *)&offset, 0) == -1 )
	{
		//TODOprintf("%s: error getting physical address [errno %d]\n", __FUNCTION__, errno);
		return (phys_addr_t)0;
	}
	return offset;
}

// x86 io 
/* The "volatile" is due to gcc bugs */
#define barrier() __asm__ __volatile__("": : :"memory")


#define build_mmio_read(name, size, type, reg, barrier) \
static inline type name(const volatile void __iomem *addr) \
{ type ret; asm volatile("mov" size " %1,%0":reg (ret) \
:"m" (*(volatile type __force *)addr) barrier); return ret; }

#define build_mmio_write(name, size, type, reg, barrier) \
static inline void name(type val, volatile void __iomem *addr) \
{ asm volatile("mov" size " %0,%1": :reg (val), \
"m" (*(volatile type __force *)addr) barrier); }

build_mmio_read(readb, "b", unsigned char, "=q", :"memory")
build_mmio_read(readw, "w", unsigned short, "=r", :"memory")
build_mmio_read(readl, "l", unsigned int, "=r", :"memory")

build_mmio_read(__readb, "b", unsigned char, "=q", )
build_mmio_read(__readw, "w", unsigned short, "=r", )
build_mmio_read(__readl, "l", unsigned int, "=r", )

build_mmio_write(writeb, "b", unsigned char, "q", :"memory")
build_mmio_write(writew, "w", unsigned short, "r", :"memory")
build_mmio_write(writel, "l", unsigned int, "r", :"memory")
build_mmio_write(writel_relaxed, "l", unsigned int, "r", :"memory")

build_mmio_write(__writeb, "b", unsigned char, "q", )
build_mmio_write(__writew, "w", unsigned short, "r", )
build_mmio_write(__writel, "l", unsigned int, "r", )

#define readb_relaxed(a) __readb(a)
#define readw_relaxed(a) __readw(a)
#define readl_relaxed(a) __readl(a)
#define __raw_readb __readb
#define __raw_readw __readw
#define __raw_readl __readl

#define __raw_writeb __writeb
#define __raw_writew __writew
#define __raw_writel __writel

#define mmiowb() barrier()

#ifdef CONFIG_X86_64

build_mmio_read(readq, "q", unsigned long, "=r", :"memory")
build_mmio_write(writeq, "q", unsigned long, "r", :"memory")

#define readq_relaxed(a)	readq(a)

#define __raw_readq(a)		readq(a)
#define __raw_writeq(val, addr)	writeq(val, addr)

/* Let people know that we have them */
#define readq			readq
#define writeq			writeq

#endif

static inline unsigned int ioread32(void __iomem *addr)
{
	return readl(addr);
}

static inline void iowrite32(u32 val, void __iomem *addr)
{
	writel(val, addr);
}


static inline int __must_check
arch_phys_wc_add(unsigned long base,
				 unsigned long size){
	return 0;  /* It worked (i.e. did nothing). */
}
static inline void arch_phys_wc_del(int handle){
}

#define fault_in_multipages_readable(...)  false
#define fault_in_multipages_writeable(...)  false

#define IOMEM_ERR_PTR(err) (__force void __iomem *)ERR_PTR(err)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/io.h $ $Rev: 844871 $")
#endif
