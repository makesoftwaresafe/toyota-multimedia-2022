/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/mm.h with modifications.
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

#ifndef _QNX_LINUX_MM_H
#define _QNX_LINUX_MM_H

#include <asm/page.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <asm/processor.h>

#define nth_page(page,n) pfn_to_page(page_to_pfn((page)) + (n))


#define VM_READ         0x00000001      /* currently active flags */
#define VM_WRITE		0x00000002
#define VM_EXEC         0x00000004
#define VM_SHARED       0x00000008
#define VM_MAYWRITE		0x00000020
#define VM_MAYSHARE     0x00000080
#define VM_PFNMAP       0x00000400      /* Page-ranges managed without "struct page", just pure PFN */
#define VM_LOCKED       0x00002000
#define VM_IO           0x00004000	/* Memory mapped I/O or similar */
#define VM_DONTEXPAND	0x00040000	/* Cannot expand with mremap() */
#define VM_RESERVED		0x00080000	/* Count as reserved_vm like IO */
#define VM_NORESERVE    0x00200000      /* should the VM suppress accounting */
#define VM_DONTDUMP	    0x04000000	/* Do not include in the core dump */


#define VM_FAULT_MINOR	0 /* For backwards compat. Remove me quickly. */

#define FAULT_FLAG_WRITE	0x01	/* Fault was a write access */
#define FAULT_FLAG_NONLINEAR	0x02	/* Fault was via a nonlinear mapping */
#define FAULT_FLAG_MKWRITE	0x04	/* Fault was mkwrite of existing pte */
#define FAULT_FLAG_ALLOW_RETRY	0x08	/* Retry fault if blocking */
#define FAULT_FLAG_RETRY_NOWAIT	0x10	/* Don't drop mmap_sem and wait when retrying */
#define FAULT_FLAG_KILLABLE	0x20	/* The fault task is in SIGKILL killable region */
#define FAULT_FLAG_TRIED	0x40	/* second try */
#define FAULT_FLAG_USER		0x80	/* The fault originated in userspace */

#define VM_FAULT_OOM	0x0001
#define VM_FAULT_SIGBUS	0x0002
#define VM_FAULT_MAJOR	0x0004
#define VM_FAULT_WRITE	0x0008	/* Special case for get_user_pages */
#define VM_FAULT_HWPOISON 0x0010	/* Hit poisoned small page */
#define VM_FAULT_HWPOISON_LARGE 0x0020  /* Hit poisoned large page. Index encoded in upper bits */

#define VM_FAULT_NOPAGE	0x0100	/* ->fault installed the pte, not return page */
#define VM_FAULT_LOCKED	0x0200	/* ->fault locked the returned page */
#define VM_FAULT_RETRY	0x0400	/* ->fault blocked, must retry */
#define VM_FAULT_FALLBACK 0x0800	/* huge page fault failed, fall back to small */

#define VM_FAULT_HWPOISON_LARGE_MASK 0xf000 /* encodes hpage index for large hwpoison */

#define VM_FAULT_ERROR	(VM_FAULT_OOM | VM_FAULT_SIGBUS | VM_FAULT_HWPOISON | \
			 VM_FAULT_FALLBACK | VM_FAULT_HWPOISON_LARGE)

/* Encode hstate index for a hwpoisoned large page */
#define VM_FAULT_SET_HINDEX(x) ((x) << 12)
#define VM_FAULT_GET_HINDEX(x) (((x) >> 12) & 0xf)

#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

typedef unsigned long vm_flags_t;

struct vm_fault {
	unsigned int flags;		/* FAULT_FLAG_xxx flags */
	pgoff_t pgoff;			/* Logical page offset based on vma */
	void __user *virtual_address;	/* Faulting virtual address */

	struct page *page;		/* ->fault handlers should return a
					 * page here, unless VM_FAULT_NOPAGE
					 * is set (which is also implied by
					 * VM_FAULT_ERROR).
					 */
};

struct vm_region {
	unsigned long	vm_flags;	/* VMA vm_flags */
	unsigned long	vm_start;	/* start address of region */
	unsigned long	vm_end;		/* region initialised to here */
	unsigned long	vm_top;		/* region allocated to here */
	unsigned long	vm_pgoff;	/* the offset in vm_file corresponding to vm_start */
	struct file	*vm_file;	/* the backing file or NULL */

	int		vm_usage;	/* region usage count (access under nommu_region_sem) */
	bool		vm_icache_flushed : 1; /* true if the icache has been flushed for
						* this region */
};

struct vm_area_struct {
	unsigned long vm_start;		/* Our start address within vm_mm. */
	unsigned long vm_end;		/* The first byte after our end address
					   within vm_mm. */

	/* linked list of VM areas per task, sorted by address */
	struct vm_area_struct *vm_next, *vm_prev;

	pgprot_t vm_page_prot;		/* Access permissions of this VMA. */
	unsigned long vm_flags;		/* Flags, see mm.h. */

	/* Function pointers to deal with this struct. */
	const struct vm_operations_struct *vm_ops;

	/* Information about our backing store: */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
					   units, *not* PAGE_CACHE_SIZE */
	struct file * vm_file;		/* File we map to (can be NULL). */
	void * vm_private_data;		/* was vm_pte (shared mem) */

#ifndef CONFIG_MMU
	struct vm_region *vm_region;	/* NOMMU mapping region */
#endif
};

// FIXME: VM
struct vm_operations_struct {
	void (*open)(struct vm_area_struct * area);
	void (*close)(struct vm_area_struct * area);
	int (*fault)(struct vm_area_struct *vma, struct vm_fault *vmf);

	/* notification that a previously read-only page is about to become
	 * writable, if an error is returned it will cause a SIGBUS */
	int (*page_mkwrite)(struct vm_area_struct *vma, struct vm_fault *vmf);

	/* called by access_process_vm when get_user_pages() fails, typically
	 * for use by special VMAs that can switch between memory and hardware
	 */
	int (*access)(struct vm_area_struct *vma, unsigned long addr,
		      void *buf, int len, int write);
};

struct address_space;
static inline void unmap_mapping_range(struct address_space *mapping,
		loff_t const holebegin, loff_t const holelen, int even_cows)
{
	int status = 0;
	unsigned long hba;
	unsigned long hlen;
	hba = holebegin >> PAGE_SHIFT;
	hlen = (holelen + PAGE_SIZE - 1) >> PAGE_SHIFT;
	/* Check for overflow. */
	if (sizeof(holelen) > sizeof(hlen)) {
		long long holeend =
			(holebegin + holelen + PAGE_SIZE - 1) >> PAGE_SHIFT;
		if (holeend & ~(long long)ULONG_MAX)
			hlen = ULONG_MAX - hba + 1;
	}
	status = munmap((void *)hba,(size_t)hlen);
	(void)status;
}

static inline unsigned long vma_pages(struct vm_area_struct *vma)
{
	return (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
}

pgprot_t vm_get_page_prot(unsigned long vm_flags);


static inline int
remap_pfn_range(struct vm_area_struct *vm_area, unsigned long addr,
			unsigned long pfn, unsigned long size, pgprot_t prot)
{
//	DRM_DEBUG("Stub is called!\n");
	return 0;
}


/**
 * vm_insert_pfn - insert single pfn into user vma
 * @vma: user vma to map to
 * @addr: target user address of this page
 * @pfn: source kernel pfn
 *
 * Similar to vm_insert_page, this allows drivers to insert individual pages
 * they've allocated into a user vma. Same comments apply.
 *
 * This function should only be called from a vm_ops->fault handler, and
 * in that case the handler should return NULL.
 *
 * vma cannot be a COW mapping.
 *
 * As this is called only for pages that do not currently exist, we
 * do not need to flush old virtual caches or the TLB.
 */
static inline int vm_insert_pfn(struct vm_area_struct *vma, unsigned long addr,
			unsigned long pfn)
{
	int ret = 0;
//	pgprot_t pgprot = vma->vm_page_prot;
	/*
	 * Technically, architectures with pte_special can avoid all these
	 * restrictions (same for remap_pfn_range).  However we would like
	 * consistency in testing and feature parity among all, so we should
	 * try to keep these invariants in place for everybody.
	 */

	if (addr < vma->vm_start || addr >= vma->vm_end)
		return -EFAULT;
//	if (track_pfn_insert(vma, &pgprot, pfn))
//		return -EINVAL;
	// TODO: Insert page into user process memory
//	ret = insert_pfn(vma, addr, pfn, pgprot);

	return ret;
}

extern unsigned long vm_mmap(struct file *, unsigned long,
		unsigned long, unsigned long,
		unsigned long, unsigned long);

#endif //_QNX_LINUX_MM_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/mm.h $ $Rev: 838597 $")
#endif
