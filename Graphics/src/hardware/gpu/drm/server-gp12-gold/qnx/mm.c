#include <linux/qnx.h>
#include <linux/linux.h>
#include <asm/cacheflush.h>

#if defined(CONFIG_64BIT)
pteval_t __supported_pte_mask __read_mostly = ~(0);
#else
pteval_t __supported_pte_mask __read_mostly = ~(_PAGE_NX | _PAGE_GLOBAL);
#endif /* CONFIG_64BIT */

/* description of effects of mapping type and prot in current implementation.
 * this is due to the limited x86 page protection hardware.  The expected
 * behavior is in parens:
 *
 * map_type	prot
 *		PROT_NONE	PROT_READ	PROT_WRITE	PROT_EXEC
 * MAP_SHARED	r: (no) no	r: (yes) yes	r: (no) yes	r: (no) yes
 *		w: (no) no	w: (no) no	w: (yes) yes	w: (no) no
 *		x: (no) no	x: (no) yes	x: (no) yes	x: (yes) yes
 *		
 * MAP_PRIVATE	r: (no) no	r: (yes) yes	r: (no) yes	r: (no) yes
 *		w: (no) no	w: (no) no	w: (copy) copy	w: (no) no
 *		x: (no) no	x: (no) yes	x: (no) yes	x: (yes) yes
 *
 */
pgprot_t protection_map[16] = {
	__P000, __P001, __P010, __P011, __P100, __P101, __P110, __P111,
	__S000, __S001, __S010, __S011, __S100, __S101, __S110, __S111
};


pgprot_t vm_get_page_prot(unsigned long vm_flags)
{
       return __pgprot(pgprot_val(protection_map[vm_flags &
                               (VM_READ|VM_WRITE|VM_EXEC|VM_SHARED)]) |
                       pgprot_val(arch_vm_get_page_prot(vm_flags)));
}

int set_memory_uc(unsigned long addr, int numpages)
{
	/* MG_TODO: Not implemented in QNX */
	BUG();
	return 0;
}

int set_memory_wb(unsigned long addr, int numpages)
{
	/* MG_TODO: Not implemented in QNX */
	BUG();
	return 0;
}

int set_pages_array_uc(struct page **pages, int addrinarray)
{
	/* MG_TODO: Not implemented in QNX */
	BUG();
	return 0;
}

int set_pages_array_wb(struct page **pages, int addrinarray)
{
	/* MG_TODO: Not implemented in QNX */
	BUG();
	return 0;
}

int set_pages_array_wc(struct page **pages, int addrinarray)
{
	/* MG_TODO: Not implemented in QNX */
	BUG();
	return 0;
}

int set_pages_uc(struct page *page, int numpages)
{
	unsigned long addr = (unsigned long)page_address(page);

	BUG();
	fprintf(stderr, "***ERROR***: set_pages_uc(): function is not implemented!\n");

	return set_memory_uc(addr, numpages);
}
EXPORT_SYMBOL(set_pages_uc);

int set_pages_wb(struct page *page, int numpages)
{
	unsigned long addr = (unsigned long)page_address(page);

	BUG();
	fprintf(stderr, "***ERROR***: set_pages_wb(): function is not implemented!\n");

	return set_memory_wb(addr, numpages);
}
EXPORT_SYMBOL(set_pages_wb);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/mm.c $ $Rev: 837534 $")
#endif
