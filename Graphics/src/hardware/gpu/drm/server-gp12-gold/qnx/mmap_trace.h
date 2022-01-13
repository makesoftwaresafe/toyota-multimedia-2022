#ifndef __MMAP_TRACE_H__
#define __MMAP_TRACE_H__

#include <stdlib.h>

int mmap_trace_dump();
int kmalloc_trace_dump();

int mmap_trace_check(void* addr);
void* mmap_trace_add(void* addr);
void* mmap_trace_add_range(void* addr, size_t size);
void* mmap_trace_del(void* addr);
void* mmap_trace_del_range(void* addr, size_t size);

#endif /* __MMAP_TRACE_H__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/mmap_trace.h $ $Rev: 850317 $")
#endif
