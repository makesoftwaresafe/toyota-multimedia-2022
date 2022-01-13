/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/uidgid.h.
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

#ifndef _LINUX_UIDGID_H
#define _LINUX_UIDGID_H

/*
 * A set of types for the internal kernel types representing uids and gids.
 *
 * The types defined in this header allow distinguishing which uids and gids in
 * the kernel are values used by userspace and which uid and gid values are
 * the internal kernel values.  With the addition of user namespaces the values
 * can be different.  Using the type system makes it possible for the compiler
 * to detect when we overlook these differences.
 *
 */

struct user_namespace;
#ifndef __QNXNTO__
extern struct user_namespace init_user_ns;
#endif

typedef struct {
	uid_t val;
} kuid_t;


typedef struct {
	gid_t val;
} kgid_t;

#define KUIDT_INIT(value) (kuid_t){ value }
#define KGIDT_INIT(value) (kgid_t){ value }


static inline uid_t __kuid_val(kuid_t uid)
{
	return uid.val;
}

static inline gid_t __kgid_val(kgid_t gid)
{
	return gid.val;
}

#define GLOBAL_ROOT_UID KUIDT_INIT(0)
#define GLOBAL_ROOT_GID KGIDT_INIT(0)

static inline kuid_t make_kuid(struct user_namespace *from, uid_t uid)
{
	return KUIDT_INIT(uid);
}

static inline kgid_t make_kgid(struct user_namespace *from, gid_t gid)
{
	return KGIDT_INIT(gid);
}

#endif /* _LINUX_UIDGID_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/uidgid.h $ $Rev: 838597 $")
#endif
