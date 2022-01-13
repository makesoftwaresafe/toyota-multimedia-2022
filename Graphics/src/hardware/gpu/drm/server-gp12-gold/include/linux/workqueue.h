#ifndef _QNX_LINUX_WORKQUEUE_H
#define _QNX_LINUX_WORKQUEUE_H

#include <linux/wait.h>
#include <linux/timer.h>
#include <assert.h>

#define WORKQ_VALID     0x20122012
#define WORKQUEUE_WORKER_EXIT_TIMEOUT  4

#define WORK_BUSY_PENDING (1 << 0)

struct workqueue_struct;
extern struct workqueue_struct *system_wq;
extern struct workqueue_struct *system_highpri_wq;
extern struct workqueue_struct *system_unbound_wq;
extern struct workqueue_struct *system_long_wq;
extern struct workqueue_struct *system_nrt_wq;
extern struct workqueue_struct *tasklet_wq;

struct work_struct;
typedef struct work_struct {
	struct work_struct *next;
	void *data;
	void (*func)(struct work_struct *work);
	struct workqueue_struct* wq;
	struct list_head list;
	const char* func_name;
	struct {
		unsigned long long	start_time;
		int	caller;
		pthread_t	tid;
	} debug;
	unsigned long work_data_bits;
} work_struct_t;

enum {
	WORK_STRUCT_PENDING_BIT = 0,
	WORK_STRUCT_TIMERED_BIT,
};

typedef struct workqueue_struct {
    pthread_t id;
    pthread_mutex_t     mutex;
    pthread_cond_t      cv;             /* wait for work */
    work_struct_t      *first, *last;   /* work queue */
    int                 valid;          /* set when valid */
    int                 quit;           /* set when wq should quit */
#if WQ_MULTI_THREAD /*TODO add thread pool for wq */
    int                 parallelism;    /* number of threads required */
    int                 counter;        /* current number of threads */
    int                 idle;           /* number of idle threads */
#endif
    char                name[80];       /* I: workqueue name */
    work_struct_t*      current;        /* current work under execution */
} workqueue_struct_t;

struct delayed_work {
	struct work_struct work;
	struct timer_list timer;
};

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 *///FIXME: remove duplicate here. defined in kernel.h
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

static inline struct delayed_work *to_delayed_work(struct work_struct *work)
{
	return container_of(work, struct delayed_work, work);
}


workqueue_struct_t *init_workqueue(const char *name, int threads);
void destroy_workqueue(workqueue_struct_t *wq);
void flush_workqueue(workqueue_struct_t *wq);
void drain_workqueue(workqueue_struct_t *wq);

void drain_workqueue(workqueue_struct_t *wq);
bool queue_work(workqueue_struct_t *wq, work_struct_t *data);
bool mod_delayed_work_on(int cpu, struct workqueue_struct *wq,
						 struct delayed_work *dwork, unsigned long delay);

#define INIT_WORK(_work, _func)						\
	do {								\
		(_work)->data = _work;					\
		(_work)->func = (_func);				\
		(_work)->next = NULL;					\
		(_work)->wq = NULL;					\
		(_work)->func_name = #_func;				\
		INIT_LIST_HEAD(&(_work)->list);				\
		(_work)->work_data_bits = 0; \
	} while (0)

#define INIT_WORK_ONSTACK(_work, _func) INIT_WORK(_work, _func)

#define INIT_DELAYED_WORK(_dwork, _func)				\
	do {								\
		INIT_WORK(&(_dwork)->work, _func);			\
		__setup_timer(&(_dwork)->timer, delayed_work_timer_fn,	\
			(unsigned long)(_dwork), 0);			\
	} while (0)

#define create_singlethread_workqueue(name)				\
	    init_workqueue((name), 1)

#define alloc_workqueue(name,flags,threads) init_workqueue((name), threads)
#define alloc_ordered_workqueue(name,flags) init_workqueue((name), 1)

int init_wq_system();
void destroy_wq_system();

bool cancel_delayed_work_sync(struct delayed_work *dwork);
bool cancel_delayed_work(struct delayed_work *dwork);
bool cancel_work_sync(struct work_struct *dwork);

/**
 * mod_delayed_work - modify delay of or queue a delayed work
 * @wq: workqueue to use
 * @dwork: work to queue
 * @delay: number of jiffies to wait before queueing
 *
 * mod_delayed_work_on() on local CPU.
 */
static inline bool mod_delayed_work(struct workqueue_struct *wq,
				    struct delayed_work *dwork,
				    unsigned long delay)
{
	return mod_delayed_work_on(0, wq, dwork, delay);
}

bool queue_delayed_work(workqueue_struct_t *wq, struct delayed_work *dwork,
						unsigned long delay);

static inline void schedule_work(struct work_struct *work)
{
	assert(system_wq);
	queue_work(system_wq, work);
}

static inline bool schedule_delayed_work(struct delayed_work *dwork, unsigned long delay)
{
	return queue_delayed_work(system_wq, dwork, delay);
}

static inline void flush_scheduled_work(void)
{
	flush_workqueue(system_wq);
}

bool flush_work(struct work_struct *work);
bool flush_delayed_work(struct delayed_work *dwork);

/* These are specified by iBCS2 */
#define POLLIN          0x0001
/* The rest seem to be more-or-less nonstandard. Check them! */
#define POLLRDNORM      0x0040

struct poll_table_struct;
struct file;
/*
 * structures and helpers for f_op->poll implementations
 */
typedef void (*poll_queue_proc)(struct file *f, wait_queue_head_t *wq, struct poll_table_struct *pt);

/*
 * Do not touch the structure directly, use the access functions
 * poll_does_not_wait() and poll_requested_events() instead.
 */
typedef struct poll_table_struct {
        poll_queue_proc _qproc;
        unsigned long _key;
} poll_table;

static inline void poll_wait(struct file *filp, wait_queue_head_t *wait_address, poll_table *p)
{
	if (p && p->_qproc && wait_address)
		p->_qproc(filp, wait_address, p);
}

/**
 * work_pending - Find out whether a work item is currently pending
 * @work: The work item in question
 */
#define work_pending(work) (!!(work_busy(work) & WORK_BUSY_PENDING))

/**
 * delayed_work_pending - Find out whether a delayable work item is currently
 * pending
 * @w: The work item in question
 */
#define delayed_work_pending(dwork) (work_pending(&(dwork)->work))

extern unsigned int work_busy(struct work_struct *work);

#define __WORK_INITIALIZER(n, f) {                 \
        .data = &n,                                \
        .next = NULL,                              \
        .func = (f),                               \
}

#define DECLARE_WORK(n, f)                         \
       struct work_struct n = __WORK_INITIALIZER(n, f)

void delayed_work_timer_fn(unsigned long __data);

/* Linux kernel 4.11 stuff */
static inline void destroy_work_on_stack(struct work_struct *work) { }

#endif /* _WQ_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/workqueue.h $ $Rev: 873525 $")
#endif
