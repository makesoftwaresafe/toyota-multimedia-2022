/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/moduleparam.h with modifications.
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

#ifndef _LINUX_MODULE_PARAMS_H
#define _LINUX_MODULE_PARAMS_H
/* (C) Copyright 2001, 2002 Rusty Russell IBM Corporation */

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
	param_check_##type(name, &(value));				   \
	module_param_cb(name, &param_ops_##type, &value, perm);		   \
	__MODULE_PARM_TYPE(name, #type)

#define module_param_named_unsafe(name, value, type, perm)		\
	param_check_##type(name, &(value));				\
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
#if 0
#define module_param_string(name, string, len, perm)			\
	static const struct kparam_string __param_string_##name		\
		= { len, string };					\
	__module_param_call(MODULE_PARAM_PREFIX, name,			\
			    &param_ops_string,				\
			    .str = &__param_string_##name, perm, -1, 0);\
	__MODULE_PARM_TYPE(name, "string")

#else
#define module_param_string(name, string, len, perm)	

#endif

#define module_param_cb(name, ops, arg, perm)				      \
	__module_param_call(MODULE_PARAM_PREFIX, name, ops, arg, perm, -1, 0)

#define module_param_cb_unsafe(name, ops, arg, perm)			      \
	__module_param_call(MODULE_PARAM_PREFIX, name, ops, arg, perm, -1,    \
			    KERNEL_PARAM_FL_UNSAFE)

#define __MODULE_PARM_TYPE(name, _type)					  \
  __MODULE_INFO(parmtype, name##type, #name ":" _type)

#define MODULE_PARM_DESC(_parm, desc) \
	__MODULE_INFO(parm, _parm, #_parm ":" desc)

#define __param_check(name, p, type) \
	static inline type *__check_##name(void) { return(p); }

#define param_check_byte(name, p) __param_check(name, p, unsigned char)
#define param_check_short(name, p) __param_check(name, p, short)
#define param_check_ushort(name, p) __param_check(name, p, unsigned short)
#define param_check_int(name, p) __param_check(name, p, int)
#define param_check_uint(name, p) __param_check(name, p, unsigned int)
#define param_check_long(name, p) __param_check(name, p, long)
#define param_check_ulong(name, p) __param_check(name, p, unsigned long)
#define param_check_charp(name, p) __param_check(name, p, char *)
#define param_check_bool(name, p) __param_check(name, p, bool)
#define param_check_invbool(name, p) __param_check(name, p, bool)

#if 0
#define __MODULE_INFO(tag, name, info)					  \
  struct __UNIQUE_ID(name) {}
#else
#define __MODULE_INFO(tag, name, info)					  \
static const char __UNIQUE_ID(name)[]					  \
  __used __attribute__((section(".modinfo"), unused, aligned(1)))	  \
  = __stringify(tag) "=" info
#endif

#define __module_param_call(prefix, name, ops, arg, perm, level, flags)
//TODO. add sys interface on qnx 

#if 0
	/* Default value instead of permissions? */			\
	static int __param_perm_check_##name __attribute__((unused)) =	\
	BUILD_BUG_ON_ZERO((perm) < 0 || (perm) > 0777 || ((perm) & 2))	\
	+ BUILD_BUG_ON_ZERO(sizeof(""prefix) > MAX_PARAM_PREFIX_LEN);	\
	static const char __param_str_##name[] = prefix #name;		\
	static struct kernel_param __moduleparam_const __param_##name	\
	__used								\
    __attribute__ ((unused,__section__ ("__param"),aligned(sizeof(void *)))) \
	= { __param_str_##name, ops, perm, level, { arg } }
#endif

#endif //_QNX_LINUX_MODULE_PARAMS_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/moduleparam.h $ $Rev: 838597 $")
#endif
