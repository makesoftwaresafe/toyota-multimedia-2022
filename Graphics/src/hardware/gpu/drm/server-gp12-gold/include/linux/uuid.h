/*
 * UUID/GUID definition
 *
 * Copyright (C) 2010, Intel Corp.
 *	Huang Ying <ying.huang@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _LINUX_UUID_H_
#define _LINUX_UUID_H_

#include <uapi/linux/uuid.h>

#define	UUID_STRING_LEN		36

static inline int uuid_le_cmp(const uuid_le u1, const uuid_le u2)
{
	return memcmp(&u1, &u2, sizeof(uuid_le));
}

static inline int uuid_be_cmp(const uuid_be u1, const uuid_be u2)
{
	return memcmp(&u1, &u2, sizeof(uuid_be));
}

extern void uuid_le_gen(uuid_le *u);
extern void uuid_be_gen(uuid_be *u);

bool uuid_is_valid(const char *uuid);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/uuid.h $ $Rev: 852681 $")
#endif
