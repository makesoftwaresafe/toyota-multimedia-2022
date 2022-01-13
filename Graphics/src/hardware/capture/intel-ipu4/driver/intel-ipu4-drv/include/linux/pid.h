/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/pid.h.
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

#ifndef _QNX_LINUX_PID_H
#define _QNX_LINUX_PID_H

#include <linux/uidgid.h>

#ifdef __QNXNTO__
#include <stdlib.h>
#include <string.h>
#endif /* __QNXNTO__ */

enum pid_type
{
	PIDTYPE_PID,
	PIDTYPE_PGID,
	PIDTYPE_SID,
	PIDTYPE_MAX
};

struct pid {
	pid_t pid;
};

static inline void put_pid(struct pid *pid)
{
#ifndef __QNXNTO__
	struct pid_namespace *ns;

	if (!pid)
		return;

	ns = pid->numbers[pid->level].ns;
	if ((atomic_read(&pid->count) == 1) ||
	     atomic_dec_and_test(&pid->count)) {
		kmem_cache_free(ns->pid_cachep, pid);
		put_pid_ns(ns);
	}
#else
	if(pid){
		free(pid);
	}
#endif
}
static inline struct pid * get_pid(struct pid *pid)
{
#ifndef __QNXNTO__
	if (pid)
		atomic_inc(&pid->count);
#endif
	struct pid * p;
	p = malloc(sizeof(*p));
	if(p){
		*p = *pid;
	}
	return p;
}

#define current_user_ns()	0

static inline pid_t
pid_vnr(struct pid *pid)
{
	return pid->pid; //pid_nr_ns(pid, task_active_pid_ns(current));
}

static inline uid_t from_kuid(struct user_namespace *to, kuid_t kuid)
{
	return __kuid_val(kuid);
}

static inline gid_t from_kgid(struct user_namespace *to, kgid_t kgid)
{
	return __kgid_val(kgid);
}

extern int overflowuid;
extern int overflowgid;

static inline uid_t from_kuid_munged(struct user_namespace *to, kuid_t kuid)
{
	uid_t uid = from_kuid(to, kuid);
	if (uid == (uid_t)-1)
		uid = overflowuid;
	return uid;
}

extern void put_pid(struct pid *pid);
extern struct task_struct *get_pid_task(struct pid *pid, enum pid_type);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/pid.h $ $Rev: 838597 $")
#endif
