/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software.   Free development licenses are
 * available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include <linux/qnx.h>
#include <linux/linux.h>
#include <drm/drmP.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/trace.h>
#include <time.h>

/* Map memory to client using shared memory file descriptor */
void* peer_map_buffer_fd(pid_t pid, int fd, unsigned long prot, size_t len)
{
	void* ptr;

	ptr = mmap64_peer(pid, 0, len, prot, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		qnx_error("%s: could not map vaddr [errno %d]", __FUNCTION__, errno);
	}

	return ptr;
}

/* Map memory to client using physical addresses */
void* peer_map_buffer(pid_t pid, void *pvaddr, unsigned long prot, size_t len)
{
	void *vaddr = (void *)((uintptr_t)pvaddr & ~(__PAGESIZE-1));
	void *cvaddr;

	cvaddr = mmap64_peer(pid, NULL, len, prot, MAP_LAZY | MAP_SHARED, NOFD, 0);
	if (cvaddr != MAP_FAILED) {
		void *ptr = cvaddr;
		size_t bytes = len;
		off64_t off;
		size_t contig;

		while (bytes > 0) {
			if (mem_offset64(vaddr, NOFD, bytes, &off, &contig)) {
				qnx_error("%s: could not get paddr [errno %d]", __FUNCTION__, errno);
				munmap_peer(pid, cvaddr, len);
				cvaddr = MAP_FAILED;
				break;
			}

			ptr = mmap64_peer(pid, ptr, contig, prot, MAP_FIXED | MAP_PHYS | MAP_SHARED, NOFD, off);
			if (ptr == MAP_FAILED) {
				qnx_error("%s: could not map vaddr [errno %d]", __FUNCTION__, errno);
				munmap_peer(pid, cvaddr, len);
				cvaddr = MAP_FAILED;
				break;
			}

			bytes -= contig;
			ptr += contig;
			vaddr += contig;
		}
	} else {
		qnx_error("%s: could not get vaddr [errno %d]", __FUNCTION__, errno);
	}

	return cvaddr;
}

unsigned long
vm_mmap(struct file *filp, unsigned long addr,
	unsigned long size, unsigned long prot,
	unsigned long flag, unsigned long offset)
{
	void* cvaddr;

	(void)flag;

	if (unlikely(offset + PAGE_ALIGN(size) < offset))
		return -EINVAL;
	if (unlikely(offset & ~PAGE_MASK))
		return -EINVAL;

	pid_t pid = task_pid_nr(current);
	if (flag & MAP_LAZY) {
		/* We do not support offset mapping right now for WC, but it seems   */
		/* it is very rare case, we implement it when we have a real client. */
		BUG_ON(offset);

		/* Use fast way to map same memory to client as write-combine via shm_ interface */
		cvaddr = peer_map_buffer_fd(pid, filp->file_descr, prot, size);
	} else {
		void* vaddr;

		vaddr = page_address(sg_page(filp->f_path.dentry->d_inode->i_mapping->sg->sgl));
		vaddr += offset;

		/* Use slow path to remap this memory as write-back */
		cvaddr = peer_map_buffer(pid, vaddr, prot, size);
	}

	if (cvaddr == MAP_FAILED) {
		return -ENOMEM;
	}

	return (unsigned long)cvaddr;
}

unsigned long
copy_from_msg(void *to, const void *from, unsigned long n)
{
	memcpy(to, from, n);
	return 0;
}

unsigned long
copy_to_msg(void *to, const void *from, unsigned long n)
{
	memcpy(to, from, n);
	return 0;
}

void schedule(void)
{
	schedule_timeout(MAX_SCHEDULE_TIMEOUT);
}

extern int preempt_owner;
long schedule_timeout(long _jiffies)
{
	uint64_t nsec;
	uint64_t end_ns;
	struct timespec now, end;
	int err = EOK;
	struct task_struct *self = current;

	if (self->task_state == TASK_RUNNING) {
#ifdef DEBUG_SCHEDULE
		printk(KERN_WARNING "%s:%s(%d) schedule at TASK_RUNNING, "
				"no wait %ld jiffies "
				"caller-main-offset=%#x\n",
				__func__, self->comm, pthread_self(),
				_jiffies == MAX_SCHEDULE_TIMEOUT ?
					-1 : _jiffies,
				caller_main_offset());
#endif
		return _jiffies;
	}

	if (in_atomic()) {
		printk(KERN_ERR "%s:%s(%d) ## BUG: "
				"scheduling while atomic:%#x "
				"caller=%#x owner=%#x\n",
				__func__, self->comm, pthread_self(),
				preempt_count(),
				caller_main_offset(), preempt_owner);
	}
	if (_jiffies == MAX_SCHEDULE_TIMEOUT) {
		nsec = 0;
	} else {
		nsec = jiffies_to_nsec(_jiffies);
	}

	clock_gettime(CLOCK_MONOTONIC, &now);
	end_ns = timespec2nsec(&now) + nsec;
	nsec2timespec(&end, end_ns);

	pthread_mutex_lock(&self->sched_mutex);
	/* Special case if we have missed condvar broadcast, we check for its last status here */
	while (!test_bit(QNX_SCHED_SIGNALLED, &self->sched_flags) && err == EOK) {
		if (nsec == 0) {
			err = pthread_cond_wait(&self->sched_cond, &self->sched_mutex);
		} else {
			err = pthread_cond_timedwait(&self->sched_cond, &self->sched_mutex, &end);
		}
	}
	clear_bit(QNX_SCHED_SIGNALLED, &self->sched_flags);
	set_current_state(TASK_RUNNING);
		/*
		 * Linux implementation
		 * try_to_wake_up(p) -> ... -> ttwu_do_wakeup(p)
		 * -> p->state = TASK_RUNNING
		 *
		 * QNX implementation
		 * try_to_wake_up(p) ->
		 *    p->sched_flags |= QNX_SHED_SIGNALLED
		 */

	pthread_mutex_unlock(&self->sched_mutex);

	clock_gettime(CLOCK_MONOTONIC, &now);
	{
		uint64_t now_ns = timespec2nsec(&now);
		long remain = 0;

		if (_jiffies == MAX_SCHEDULE_TIMEOUT) {
			return _jiffies;
		} else {
			if (end_ns > now_ns) {
				u64 nsecs = end_ns - now_ns;
				remain = nsec_to_jiffies(nsecs);
			}
		}
		return remain;
	}
}

void fput(struct file* file)
{
	if (atomic_long_dec_and_test(&file->f_count)) {
		kfree(file);
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/qnx-util.c $ $Rev: 851843 $")
#endif
