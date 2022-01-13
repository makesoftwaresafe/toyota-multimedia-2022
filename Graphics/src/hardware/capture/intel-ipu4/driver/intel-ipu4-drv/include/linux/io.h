/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/io.h, /include/linux/io-mapping.h, taking portion
* of these files and making local modifications.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _QNX_LINUX_IO_H
#define _QNX_LINUX_IO_H

#include <assert.h>
#include <sys/mman.h>
#include <linux/compiler.h>

#include <hw/inout.h>
#define outb(x,y)  out8(y,x)
#define inb(x)  in8(x)

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

static inline void *
qnx_phys_to_virt(phys_addr_t address){
	// TODO.
	assert( 0 && "qnx_phys_to_virt no imp yet!");
	return 0;
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

/** io mapping */
struct io_mapping {
	resource_size_t base;
	unsigned long size;
	pgprot_t prot;
	void * vaddr;
};

/**
 * iounmap - Free a IO remapping
 * @addr: virtual address from ioremap_*
 *
 * Caller must ensure there is only one unmapping for the same pointer.
 */
static inline void iounmap(volatile void *addr, unsigned long size)
{
	munmap_device_memory((void*)addr, size);
//	munmap_device_io((_Uintptrt)addr, size);
}
static inline void
io_mapping_unmap(void *vaddr, unsigned long size )
{
	munmap_device_memory(vaddr, size);
}

static inline void
io_mapping_unmap_atomic(void *vaddr, unsigned long size )
{
	//munmap_device_memory(vaddr, size);
}

static inline void *ioremap_wc(resource_size_t phys_addr, unsigned long size)
{
	return mmap_device_memory( 0, size, PROT_NOCACHE | PROT_READ|PROT_WRITE, 0, phys_addr );
}

static inline void *ioremap(resource_size_t phys_addr, unsigned long size)
{
	return mmap_device_memory( 0, size, PROT_NOCACHE | PROT_READ|PROT_WRITE, 0, phys_addr );
}
static inline void *ioremap_nocache(resource_size_t phys_addr, unsigned long size)
{
	return mmap_device_memory( 0, size, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, phys_addr );
}

static inline void *io_mapping_map_wc(struct io_mapping *mapping, unsigned long offset)
{
	resource_size_t phys_addr;
	if(offset >= mapping->size) return NULL;
	phys_addr = mapping->base + offset;
	return ioremap_wc(phys_addr, PAGE_SIZE);
}
/* Atomic map/unmap */
static inline void  *
io_mapping_map_atomic_wc(struct io_mapping *mapping,
			 unsigned long offset)
{
	//resource_size_t phys_addr;
	if(offset >= mapping->size) return NULL;
#if 0
	phys_addr = mapping->base + offset;
	return ioremap_wc(phys_addr, PAGE_SIZE);
#else
	return mapping->vaddr + offset;
#endif
}

#define fault_in_multipages_readable(...)  false
#define fault_in_multipages_writeable(...)  false

/* Create the io_mapping object*/
static inline struct io_mapping *
io_mapping_create_wc(resource_size_t base, unsigned long size)
{
	struct io_mapping *iomap;
	pgprot_t prot;

	prot.pgprot = 0;
	iomap = malloc(sizeof(*iomap));
	if (!iomap)
		return NULL;
	memset(iomap,0,sizeof(*iomap));
	iomap->base = base;
	iomap->size = size;
	iomap->prot = prot;
	return iomap;
}

static inline void
io_mapping_free(struct io_mapping *mapping)
{
	iounmap((void *) mapping->base , mapping->size);
	free(mapping);
}


#endif //_QNX_LINUX_IO_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/io.h $ $Rev: 838597 $")
#endif
