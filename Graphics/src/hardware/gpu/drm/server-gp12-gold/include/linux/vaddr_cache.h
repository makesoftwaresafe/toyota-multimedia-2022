#ifndef  __vaddr_cache_h
#define  __vaddr_cache_h

struct vaddr_list_t {
	int pages;
	int off;
	void ** vaddrs;
};

int vaddr_cache_get_pages(pid_t pid, void * pvaddr, size_t len,
						  struct vaddr_list_t * result);


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/vaddr_cache.h $ $Rev: 836322 $")
#endif
