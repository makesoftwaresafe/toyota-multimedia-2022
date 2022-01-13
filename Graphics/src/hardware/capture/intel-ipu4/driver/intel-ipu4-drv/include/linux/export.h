/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/export.h - takes small portion of this file.
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

#ifndef _LINUX_EXPORT_H
#define _LINUX_EXPORT_H

#define EXPORT_SYMBOL(sym) extern typeof(sym) sym;
#define EXPORT_SYMBOL_GPL(sym) extern typeof(sym) sym;
#define EXPORT_SYMBOL_GPL_FUTURE(sym) extern typeof(sym) sym;
#define EXPORT_UNUSED_SYMBOL(sym) extern typeof(sym) sym;
#define EXPORT_UNUSED_SYMBOL_GPL(sym) extern typeof(sym) sym;

#define THIS_MODULE ((void *)0)


#endif /* _LINUX_EXPORT_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/export.h $ $Rev: 838597 $")
#endif
