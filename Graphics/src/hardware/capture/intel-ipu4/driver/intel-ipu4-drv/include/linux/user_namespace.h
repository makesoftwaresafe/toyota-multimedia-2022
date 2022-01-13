/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/user_namespace.h.
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

#ifndef _LINUX_USER_NAMESPACE_H
#define _LINUX_USER_NAMESPACE_H

#include <linux/ns_common.h>
#include <linux/uidgid.h>

#define UID_GID_MAP_MAX_EXTENTS 5

struct uid_gid_map {	/* 64 bytes -- 1 cache line */
	u32 nr_extents;
	struct uid_gid_extent {
		u32 first;
		u32 lower_first;
		u32 count;
	} extent[UID_GID_MAP_MAX_EXTENTS];
};

#define USERNS_SETGROUPS_ALLOWED 1UL

#define USERNS_INIT_FLAGS USERNS_SETGROUPS_ALLOWED

struct user_namespace {
	struct uid_gid_map	uid_map;
	struct uid_gid_map	gid_map;
	struct uid_gid_map	projid_map;
	atomic_t		count;
	struct user_namespace	*parent;
	int			level;
	kuid_t			owner;
	kgid_t			group;
	struct ns_common	ns;
	unsigned long		flags;

	/* Register of per-UID persistent keyrings for this namespace */
#ifdef CONFIG_PERSISTENT_KEYRINGS
	struct key		*persistent_keyring_register;
	struct rw_semaphore	persistent_keyring_register_sem;
#endif
};

extern struct user_namespace init_user_ns;

#endif /* _LINUX_USER_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/user_namespace.h $ $Rev: 838597 $")
#endif
