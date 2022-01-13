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

#ifndef _QNX_LINUX_TIMEKEEPING_H
#define _QNX_LINUX_TIMEKEEPING_H

#include <assert.h>

static inline ktime_t ktime_get_real(void)
{
	ktime_t kt;
	struct timespec t;
	if( clock_gettime(CLOCK_REALTIME, &t) == -1 ) {
		perror( "Monolitic clock gettime faied" );
	}
	kt.tv.sec = t.tv_sec;
	kt.tv.nsec = t.tv_nsec;
	return kt;
}

static inline ktime_t ktime_mono_to_real(ktime_t mono)
{
    /* usually we have monotonic clock identical to realtime */
    /* for scanout position determination difference between realtime */
    /* monotonic doesn't matter. */
    return mono;
}

static inline u64 ktime_get_ns(void)
{
    return ktime_get_raw_ns();
}

#endif //_QNX_LINUX_TIMEKEEPING_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/timekeeping.h $ $Rev: 838597 $")
#endif
