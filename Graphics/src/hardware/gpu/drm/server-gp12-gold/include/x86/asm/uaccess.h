#ifndef _QNX_LINUX_UACCESS_H
#define _QNX_LINUX_UACCESS_H

#include <sys/mman.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/memmsg.h>
#include <sys/types.h>
#include <stdint.h>
#include <assert.h>

#include <linux/types.h>

#ifndef __user
#define __user
#endif /* __user */

#ifndef __always_inline
#define __always_inline inline
#endif /* __always_inline */

#ifndef  __must_check
#define  __must_check
#endif /* __QNXNTO__ */

#define VERIFY_READ	0
#define VERIFY_WRITE	1

int memmgr_peer_sendnc( pid_t pid, int coid, void *smsg, size_t sbytes, void *rmsg, size_t rbytes );

void * _mmap2_peer(pid_t pid, void *addr, size_t len, int prot, int flags, int fd, off64_t off,
			unsigned align, unsigned pre_load, void **base, size_t *size);

// Make an unsigned version of the 'off_t' type so that we get a zero
// extension down below.
/* #if __OFF_BITS__ == 32 */
/* 	typedef _Uint32t uoff_t; */
/* #elif __OFF_BITS__ == 64 */
/* 	typedef _Uint64t uoff_t; */
/* #else */
/* 	#error Do not know what size to make uoff_t */
/* #endif */

static inline void *
mmap_peer(pid_t pid, void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	return _mmap2_peer(pid, addr, len, prot, flags, fd, off, 0, 0, 0, 0);
}

static inline int
munmap_flags_peer(pid_t pid, void *addr, size_t len, unsigned flags)
{
	mem_ctrl_t msg;

	msg.i.type = _MEM_CTRL;
	msg.i.subtype = _MEM_CTRL_UNMAP;
	msg.i.addr = (uintptr_t)addr;
	msg.i.len = len;
	msg.i.flags = flags;
	return memmgr_peer_sendnc(pid, MEMMGR_COID, &msg.i, sizeof msg.i, 0, 0);
}

static inline int
munmap_peer(pid_t pid, void *addr, size_t len)
{
	return munmap_flags_peer(pid, addr, len, 0);
}

static inline void *
mmap64_peer(pid_t pid, void *addr, size_t len, int prot, int flags, int fd, off64_t off) {
	return _mmap2_peer(pid, addr, len, prot, flags, fd, off, 0, 0, 0, 0);
}
static inline int
munmap64_peer(pid_t pid, void *addr, size_t len) {
	return munmap_peer(pid, addr, len);
}

void * peer_register_buffer(pid_t pid, int id, void *pcvaddr, size_t len, int * cached);
int mem_offset64_peer(pid_t pid, const uintptr_t addr, size_t len, off64_t *offset, size_t *contig_len);

struct uaddr {
	const void *uaddr;	/* I - user space address */
	void *vaddr;	/* O - current space mapped address */
	size_t size;	/* I/O - maximum accesssable size from vaddr */
};
extern int uaddr_get(int id, pid_t pid);	/* get uaddr cache */
extern int uaddr_put(int id);			/* put uaddr cache */
extern int uaddr_to_vaddr(int id, pid_t pid, struct uaddr *uaddr);
extern void uaddr_forget(int id, pid_t pid, struct uaddr *uaddr);

extern atomic_t uaddr_cache_validseq;
#define uaddr_invalidate() do { \
	atomic_add(1, &uaddr_cache_validseq); \
} while(0)

unsigned long copy_from_pid(pid_t pid, void *to, const void __user *from, unsigned long n);
unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);
unsigned long copy_to_pid(pid_t pid, void __user *to, const void *from, unsigned long n);
unsigned long copy_to_user(void __user *to, const void *from, unsigned long n);

#define __get_user_case(VAR, BITS) { uint##BITS##_t __gu_tmp; \
    __gu_remain = copy_from_user(&__gu_tmp, __gu_ptr, sizeof __gu_tmp); \
    if (__gu_remain == 0) (VAR) = *(__typeof__(*__gu_ptr) *)&__gu_tmp; \
}
#define __get_user(VAR, PTR) ({ \
    __typeof__(*(PTR)) *__gu_ptr = (PTR); \
    int __gu_remain = -1; \
    switch (sizeof *(__gu_ptr)) { \
    case 1: __get_user_case((VAR),8); break; \
    case 2: __get_user_case((VAR),16); break; \
    case 4: __get_user_case((VAR),32); break; \
    case 8: __get_user_case((VAR),64); break; \
    } \
    __gu_remain ? -EFAULT : 0;  /* return value */ \
})
#define get_user(VAR, PTR) __get_user(VAR, PTR)

#define __put_user(VAR, PTR) ({ \
    __typeof__(*(PTR)) __pu_tmp = (VAR); \
    int __pu_remain; \
    __pu_remain = __copy_to_user((PTR), &__pu_tmp, sizeof __pu_tmp); \
    __pu_remain ? -EFAULT : 0;  /* return value */ \
})
#define put_user(VAR, PTR) __put_user(VAR, PTR)

#define  __copy_to_user_inatomic(to, from, n)  copy_to_user(to, from, n)
#define  __copy_to_user(to, from, n)  copy_to_user(to, from, n)
#define  __copy_from_user(to, from, n)  copy_from_user(to, from, n)
#define __copy_from_user_inatomic(to, from, n)  copy_from_user(to, from, n)
#define __copy_from_user_inatomic_nocache(to, from, n)  copy_from_user(to, from, n)
#define  __copy_to_user_ll(to, from, n)  copy_to_user(to, from, n)


#define access_ok(type, addr, size) \
	(likely(__range_not_ok(addr, size, user_addr_max()) == 0))

/*
 * Test whether a block of memory is a valid user space address.
 * Returns 0 if the range is valid, nonzero otherwise.
 *
 * This is equivalent to the following test:
 * (u33)addr + (u33)size > (u33)current->addr_limit.seg (u65 for x86_64)
 *
 * This needs 33-bit (65-bit for x86_64) arithmetic. We have a carry...
 */
#if 0
#define __range_not_ok(addr, size, limit)				\
({									\
	unsigned long flag, roksum;					\
	__chk_user_ptr(addr);						\
	asm("add %3,%1 ; sbb %0,%0 ; cmp %1,%4 ; sbb $0,%0"		\
	    : "=&r" (flag), "=r" (roksum)				\
	    : "1" (addr), "g" ((long)(size)),				\
	      "rm" (limit));						\
	flag;								\
})
#else
//TODO
#define __range_not_ok(addr, size, limit)	0
#endif


static __always_inline __must_check
int __copy_in_user(void __user *dst, const void __user *src, unsigned size)
{
	assert(0 && "__copy_in_user no imp yet!");
	return 0;
}

static inline void pagefault_disable(void) {}
static inline void pagefault_enable(void) {}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/uaccess.h $ $Rev: 836935 $")
#endif
