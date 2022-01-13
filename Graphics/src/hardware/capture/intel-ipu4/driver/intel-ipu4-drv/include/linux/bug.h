/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/asm-generic/bug.h.
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

#ifndef _QNX_LINUX_BUG_H
#define _QNX_LINUX_BUG_H

#define WARN_ON(condition) ({						\
			int __ret_warn_on = !!(condition);		\
			if (unlikely(__ret_warn_on)){			\
				qnx_warning("warning");				\
			}										\
			unlikely(__ret_warn_on);				\
		})

#define WARN(condition, format...) ({				\
			int __ret_warn_on = !!(condition);		\
			if (unlikely(__ret_warn_on)){			\
				qnx_warning(format);				\
			}										\
			unlikely(__ret_warn_on);				\
		})

#define WARN_ONCE(condition, format...)	({				\
			static bool __section(.data.unlikely) __warned;	\
			int __ret_warn_once = !!(condition);			\
			if (unlikely(__ret_warn_once))					\
				if (WARN(!__warned, format))				\
					__warned = true;						\
			unlikely(__ret_warn_once);						\
		})


#define WARN_ON_ONCE(condition)	({				\
			static bool __section(.data.unlikely) __warned;	\
			int __ret_warn_once = !!(condition);			\
			if (unlikely(__ret_warn_once)){					\
				if (WARN_ON(!__warned))						\
					__warned = true;						\
			}												\
			unlikely(__ret_warn_once);						\
		})

# define WARN_ON_SMP(x)			WARN_ON(x)


#ifndef __OPTIMIZE__
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#else
#define BUILD_BUG_ON(condition) \
	BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)
#endif

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON_NULL(e) ((void *)sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)			\
	BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))
#define BUILD_BUG_ON_INVALID(e) ((void)(sizeof((__force long)(e))))
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)


#endif //_QNX_LINUX_BUG_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/bug.h $ $Rev: 838597 $")
#endif
