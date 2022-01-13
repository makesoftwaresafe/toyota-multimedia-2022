/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /arch/x86/include/asm/uaccess.h.
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

#ifndef _QNX_LINUX_UACCESS_H
#define _QNX_LINUX_UACCESS_H

#include <sys/mman.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/memmsg.h>
#include <sys/types.h>
#include <assert.h>


#define VERIFY_READ	0
#define VERIFY_WRITE	1

unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);
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
#ifndef __QNXNTO__
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
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/x86/asm/uaccess.h $ $Rev: 838597 $")
#endif
