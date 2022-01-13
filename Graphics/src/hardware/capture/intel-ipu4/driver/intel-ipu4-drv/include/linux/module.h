/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/module.h with modifications.
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

#ifndef _LINUX_MODULE_H
#define _LINUX_MODULE_H

/*
 * Dynamic loading of modules into the kernel.
 *
 * Rewritten by Richard Henderson <rth@tamu.edu> Dec 1996
 * Rewritten again by Rusty Russell, 2002
 */

#include <linux/moduleparam.h>
#include <linux/export.h>
#include <linux/kmod.h>

typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

#define MODULE_INFO(tag, info) __MODULE_INFO(tag, tag, info)

#if 0
#define MODULE_AUTHOR(_author) MODULE_INFO(author, _author)
#define MODULE_DESCRIPTION(_description) MODULE_INFO(description, _description)
#define MODULE_LICENSE(_license) MODULE_INFO(license, _license)
#else
#define MODULE_AUTHOR(_author)
#define MODULE_DESCRIPTION(_description)
#define MODULE_LICENSE(_license)
#endif

#ifdef MODULE
/* Creates an alias so file2alias.c can find device table. */
#define MODULE_DEVICE_TABLE(type, name)					\
extern const typeof(name) __mod_##type##__##name##_device_table		\
  __attribute__ ((unused, alias(__stringify(name))))
#else  /* !MODULE */
#define MODULE_DEVICE_TABLE(type, name)
#endif

/* TODO. Get/put a kernel symbol (calls should be symmetric) */
#define symbol_get(x) ({ NULL; })
#define symbol_put(x) do { } while (0)
#define symbol_put_addr(x) do { } while (0)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/module.h $ $Rev: 838597 $")
#endif
