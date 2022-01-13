#ifndef _QNX_LINUX_MODULE_PARAMS_H
#define _QNX_LINUX_MODULE_PARAMS_H

#include <stdint.h>
#include <linux/stringify.h>

enum {
	KERNEL_PARAM_FL_UNSAFE = (1 << 0)
};

#define module_param(name, type, perm)				\
	module_param_named(name, name, type, perm)

/**
 * module_param_unsafe - same as module_param but taints kernel
 */
#define module_param_unsafe(name, type, perm)			\
	module_param_named_unsafe(name, name, type, perm)

#define module_param_named(name, value, type, perm)			   \
	param_check_##type(name, &value);				   \
	module_param_cb(name, &param_ops_##type, &value, perm);		   \
	__MODULE_PARM_TYPE(name, #type)

#define module_param_named_unsafe(name, value, type, perm)		\
	param_check_##type(name, &value);				\
	module_param_cb_unsafe(name, &param_ops_##type, &value, perm);	\
	__MODULE_PARM_TYPE(name, #type)

/**
 * module_param_string - a char array parameter
 * @name: the name of the parameter
 * @string: the string variable
 * @len: the maximum length of the string, incl. terminator
 * @perm: visibility in sysfs.
 *
 * This actually copies the string when it's set (unlike type charp).
 * @len is usually just sizeof(string).
 */
#define module_param_string(name, string, len, perm)			\
	param_check_string(name, &string, len);				\
	__MODULE_PARM_TYPE(name, "string")

#define module_param_cb(name, ops, arg, perm)
#define module_param_cb_unsafe(name, ops, arg, perm)

#define __MODULE_PARM_TYPE(name, _type)					  \
	__MODULE_INFO(parmtype, name##type, #name ":" _type)

#define MODULE_PARM_DESC(_parm, desc) \
	__MODULE_INFO(parm, _parm, #_parm ":" desc)

#define __param_check(name, p, type, len) \
	__MODULE_INFOX(parmname, name##id, name, p, len)

#define param_check_byte(name, p) __param_check(name, p, unsigned char, 0)
#define param_check_short(name, p) __param_check(name, p, short, 0)
#define param_check_ushort(name, p) __param_check(name, p, unsigned short, 0)
#define param_check_int(name, p) __param_check(name, p, int, 0)
#define param_check_uint(name, p) __param_check(name, p, unsigned int, 0)
#define param_check_long(name, p) __param_check(name, p, long, 0)
#define param_check_ulong(name, p) __param_check(name, p, unsigned long, 0)
#define param_check_ullong(name, p) __param_check(name, p, unsigned long long, 0)
#define param_check_charp(name, p) __param_check(name, p, char *, 0)
#define param_check_bool(name, p) __param_check(name, p, bool, 0)
#define param_check_invbool(name, p) __param_check(name, p, bool, 0)
#define param_check_string(name, p, len) __param_check(name, p, bool, len)

#define __MODULE_INFO(tag, name, info)					  \
static const char __UNIQUE_ID(name)[]					  \
  __used __attribute__((section(".modinfo"), unused, aligned(1)))	  \
  = __stringify(tag) "=" info

struct _infox_qnx {
    char parmaddr_str[64];
    uintptr_t parmaddr;
    uintptr_t parmsize;
};

#define __MODULE_INFOX(tag, name, name2, info, len)		 	\
static const struct _infox_qnx __UNIQUE_ID(name)			\
    __used __attribute__((section(".modinfo"), unused, aligned(1))) = {	\
    .parmaddr = (uintptr_t)(info),					\
    .parmsize = (uintptr_t)(len),					\
    .parmaddr_str = "parmaddr=" #name2 ":" };

#define MAX_PARAM_PREFIX_LEN (64 - sizeof(unsigned long))

#endif /* _QNX_LINUX_MODULE_PARAMS_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/moduleparam.h $ $Rev: 841111 $")
#endif
