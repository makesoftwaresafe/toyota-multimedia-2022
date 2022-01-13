/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Yocto Linux original taken from GP101 release; __QNXNTO__
* takes code from /linux/signal.h and merges it into the code from
* /asm-generic/siginfo.h
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

#ifndef _ASM_GENERIC_SIGINFO_H
#define _ASM_GENERIC_SIGINFO_H

#include <uapi/asm-generic/siginfo.h>

#define __SI_MASK	0xffff0000u
#define __SI_KILL	(0 << 16)
#define __SI_TIMER	(1 << 16)
#define __SI_POLL	(2 << 16)
#define __SI_FAULT	(3 << 16)
#define __SI_CHLD	(4 << 16)
#define __SI_RT		(5 << 16)
#define __SI_MESGQ	(6 << 16)
#define __SI_SYS	(7 << 16)
#define __SI_CODE(T,N)	((T) | ((N) & 0xffff))

struct siginfo;
void do_schedule_next_timer(struct siginfo *info);

#ifndef __QNXNTO__
#ifndef HAVE_ARCH_COPY_SIGINFO

#include <linux/string.h>

static inline void copy_siginfo(struct siginfo *to, struct siginfo *from)
{
	if (from->si_code < 0)
		memcpy(to, from, sizeof(*to));
	else
		/* _sigchld is currently the largest know union member */
		memcpy(to, from, __ARCH_SI_PREAMBLE_SIZE + sizeof(from->_sifields._sigchld));
}

#endif
#endif // __QNXNTO__

extern int copy_siginfo_to_user(struct siginfo __user *to, const struct siginfo *from);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/asm-generic/siginfo.h $ $Rev: 838597 $")
#endif
