#ifndef _LINUX_STOP_MACHINE
#define _LINUX_STOP_MACHINE

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/list.h>

/*
 * stop_cpu[s]() is simplistic per-cpu maximum priority cpu
 * monopolization mechanism.  The caller can specify a non-sleeping
 * function to be executed on a single or multiple cpus preempting all
 * other processes and monopolizing those cpus until it finishes.
 *
 * Resources for this mechanism are preallocated when a cpu is brought
 * up and requests are guaranteed to be served as long as the target
 * cpus are online.
 */
typedef int (*cpu_stop_fn_t)(void *arg);

#ifdef CONFIG_SMP

struct cpu_stop_work {
	struct list_head	list;		/* cpu_stopper->works */
	cpu_stop_fn_t		fn;
	void			*arg;
	struct cpu_stop_done	*done;
};

int stop_one_cpu(unsigned int cpu, cpu_stop_fn_t fn, void *arg);
int stop_two_cpus(unsigned int cpu1, unsigned int cpu2, cpu_stop_fn_t fn, void *arg);
void stop_one_cpu_nowait(unsigned int cpu, cpu_stop_fn_t fn, void *arg,
			 struct cpu_stop_work *work_buf);
int stop_cpus(const struct cpumask *cpumask, cpu_stop_fn_t fn, void *arg);
int try_stop_cpus(const struct cpumask *cpumask, cpu_stop_fn_t fn, void *arg);

#else	/* CONFIG_SMP */

#include <linux/workqueue.h>

struct cpu_stop_work {
	struct work_struct	work;
	cpu_stop_fn_t		fn;
	void			*arg;
};

static inline int stop_one_cpu(unsigned int cpu, cpu_stop_fn_t fn, void *arg)
{
	int ret = 0;

	ret = fn(arg);

	return ret;
}

static void stop_one_cpu_nowait_workfn(struct work_struct *work)
{
	struct cpu_stop_work *stwork = container_of(work, struct cpu_stop_work, work);
	stwork->fn(stwork->arg);
}

static inline void stop_one_cpu_nowait(unsigned int cpu,
				       cpu_stop_fn_t fn, void *arg,
				       struct cpu_stop_work *work_buf)
{
    INIT_WORK(&work_buf->work, stop_one_cpu_nowait_workfn);
    work_buf->fn = fn;
    work_buf->arg = arg;
    schedule_work(&work_buf->work);
}

static inline int stop_cpus(const struct cpumask *cpumask,
			    cpu_stop_fn_t fn, void *arg)
{
	return stop_one_cpu(raw_smp_processor_id(), fn, arg);
}

static inline int try_stop_cpus(const struct cpumask *cpumask,
				cpu_stop_fn_t fn, void *arg)
{
	return stop_cpus(cpumask, fn, arg);
}

#endif	/* CONFIG_SMP */

/*
 * stop_machine "Bogolock": stop the entire machine, disable
 * interrupts.  This is a very heavy lock, which is equivalent to
 * grabbing every spinlock (and more).  So the "read" side to such a
 * lock is anything which disables preemption.
 */
#if defined(CONFIG_STOP_MACHINE) && defined(CONFIG_SMP) && !defined(__QNXNTO__)

/**
 * stop_machine: freeze the machine on all CPUs and run this function
 * @fn: the function to run
 * @data: the data ptr for the @fn()
 * @cpus: the cpus to run the @fn() on (NULL = any online cpu)
 *
 * Description: This causes a thread to be scheduled on every cpu,
 * each of which disables interrupts.  The result is that no one is
 * holding a spinlock or inside any other preempt-disabled region when
 * @fn() runs.
 *
 * This can be thought of as a very heavy write lock, equivalent to
 * grabbing every spinlock in the kernel. */
int stop_machine(cpu_stop_fn_t fn, void *data, const struct cpumask *cpus);

int stop_machine_from_inactive_cpu(cpu_stop_fn_t fn, void *data,
				   const struct cpumask *cpus);
#else	 /* CONFIG_STOP_MACHINE && CONFIG_SMP */

static inline int stop_machine(cpu_stop_fn_t fn, void *data,
				 const struct cpumask *cpus)
{
	unsigned long flags;
	int ret;
	local_irq_save(flags);
	ret = fn(data);
	local_irq_restore(flags);
	return ret;
}

static inline int stop_machine_from_inactive_cpu(cpu_stop_fn_t fn, void *data,
						 const struct cpumask *cpus)
{
	return stop_machine(fn, data, cpus);
}

#endif	/* CONFIG_STOP_MACHINE && CONFIG_SMP */
#endif	/* _LINUX_STOP_MACHINE */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/stop_machine.h $ $Rev: 836322 $")
#endif
