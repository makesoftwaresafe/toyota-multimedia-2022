/*
* Copyright (c) 2017 QNX Software Systems.
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

#ifndef __QNXNTO_QNX_H
#define __QNXNTO_QNX_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <mem.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>
#include <pthread.h>
#include <ioctl.h>
#include <limits.h>
#include <inttypes.h>
#include <gulliver.h>
#include <fcntl.h>
#include <dirent.h>

#ifndef LIBPCI
#include <hw/pci.h>
#include <sys/pci_serv.h>
#else
	#ifndef PCI_IS_IO
		#define PCI_IS_IO(address)				(((address) & 1) &&1)
	#endif
	#ifndef PCI_IS_MEM
		#define PCI_IS_MEM(address)				(!PCI_IS_IO(address))
	#endif
	#ifndef PCI_COMMAND_MASTER
		#define	PCI_COMMAND_MASTER_ENABLE		0x0004
	#endif
	#ifndef PCI_COMMAND_INTX_DISABLE
		#define	PCI_COMMAND_INTX_DISABLE		0x0400
	#endif
#endif
#include <hw/inout.h>

#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/iomgr.h>
#include <sys/iomsg.h>
#include <sys/dispatch.h>
#include <sys/rsrcdbmgr.h>
#include <sys/procmgr.h>
#include <sys/slogcodes.h>
#include <sys/mman.h>
#include <sys/netmgr.h>
#include <sys/memmsg.h>
#include <sys/cache.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/kconfig.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/const.h>

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/qnx.h $ $Rev: 838597 $")
#endif
