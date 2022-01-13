#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/bug.h>

int oops_in_progress;
EXPORT_SYMBOL(oops_in_progress);

static u64 oops_id = 0;

static int init_oops_id(void)
{
        oops_id++;

        return 0;
}

void print_oops_end_marker(void)
{
        init_oops_id();
        pr_warn("---[ end trace %016llx ]---\n", (unsigned long long)oops_id);
}

struct warn_args {
        const char *fmt;
        va_list args;
};

void __warn(const char *file, int line, void *caller, unsigned taint,
            struct pt_regs *regs, struct warn_args *args)
{
	pid_t	pid;
	pthread_t	tid, this_tid;
	const char	*comm, *file_last;

	int	preempt_owner, preempt_prty;
	struct sched_param	sp;
	char	buf[200];

	if (current) {
		pid = current->pid;
		tid = current->attachment.user_tid;
		comm = current->comm;
		preempt_owner = current->preempt.owner;
		preempt_prty = current->preempt.prio;
	} else {
		pid = -2;
		tid = -2;
		comm = "<no current>";
		preempt_owner = 0;
		preempt_prty =0;
	}
	this_tid = pthread_self();
	pthread_getschedparam(this_tid, NULL, &sp);

	if (file) {
		file_last = strrchr(file, '/');
		if (file_last) {
			file_last++;
		} else {
			file_last = file;
		}
	} else {
		file_last = "";
	}

        pr_warn("------------[ cut here ]------------\n");

	snprintf(buf, sizeof(buf),
			"WARNING[%d]: PID: %d:%d %-.16s "
			"preempt=%#x irq=%d owner=%#x prty=%d(%d) "
			"caller=%#x",
			this_tid, pid, tid, comm,
			preempt_count(), irqs_disabled(),
			preempt_owner,
			sp.sched_curpriority, preempt_prty,
			main_offset(caller));
        if (file)
		pr_warn("%s at %s:%d\n", buf, file_last, line);
	else
		pr_warn("%s\n", buf);

        if (args)
                vprintk(args->fmt, args->args);

	do {
		static struct timespec	prev;
		struct timespec	now;

		clock_gettime(CLOCK_MONOTONIC, &now);
		if ( (prev.tv_sec || prev.tv_nsec) &&
				now.tv_sec - prev.tv_sec < 10) {
			pr_warn("..not dumpstack so often\n");
			break;
		}
		prev = now;
		dump_stack();
	} while (0);
        print_oops_end_marker();
}

void warn_slowpath_fmt(const char *file, int line, const char *fmt, ...)
{
        struct warn_args args;

        args.fmt = fmt;
        va_start(args.args, fmt);
        __warn(file, line, __builtin_return_address(0), TAINT_WARN, NULL,
               &args);
        va_end(args.args);
}
EXPORT_SYMBOL(warn_slowpath_fmt);

void warn_slowpath_fmt_taint(const char *file, int line,
                             unsigned taint, const char *fmt, ...)
{
        struct warn_args args;

        args.fmt = fmt;
        va_start(args.args, fmt);
        __warn(file, line, __builtin_return_address(0), taint, NULL, &args);
        va_end(args.args);
}
EXPORT_SYMBOL(warn_slowpath_fmt_taint);

void warn_slowpath_null(const char *file, int line)
{
        __warn(file, line, __builtin_return_address(0), TAINT_WARN, NULL, NULL);
}
EXPORT_SYMBOL(warn_slowpath_null);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/printk.c $ $Rev: 841047 $")
#endif
