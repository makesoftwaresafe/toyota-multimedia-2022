#ifndef _ASM_X86_POSIX_TYPES_32_H
#define _ASM_X86_POSIX_TYPES_32_H

/*
 * This file is generally used by user-level software, so you need to
 * be a little careful about namespace pollution etc.  Also, we cannot
 * assume GCC is being used.
 */

typedef unsigned short	__kernel_mode_t;
#define __kernel_mode_t __kernel_mode_t

typedef unsigned short	__kernel_ipc_pid_t;
#define __kernel_ipc_pid_t __kernel_ipc_pid_t

typedef unsigned short	__kernel_uid_t;
typedef unsigned short	__kernel_gid_t;
#define __kernel_uid_t __kernel_uid_t

typedef unsigned short	__kernel_old_dev_t;
#define __kernel_old_dev_t __kernel_old_dev_t

#include <asm-generic/posix_types.h>

#endif /* _ASM_X86_POSIX_TYPES_32_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/uapi/asm/posix_types_32.h $ $Rev: 836322 $")
#endif
