#include <linux/qnx.h>
#include <linux/linux.h>

#include <drm/drmP.h>
#include <linux/qnx_resmgr.h>

#include "base.h"

static DEFINE_SPINLOCK(fasync_lock);
static struct kmem_cache *fasync_cache;

struct module __this_module = {
	.name = "i915",
	.async_probe_requested = false,
};

/////////////////////////
// internal functions //
/////////////////////////

/*
 * allocate a file descriptor, mark it busy.
 */
int alloc_fd(unsigned start, unsigned flags)
{
	BUG();
	fprintf(stderr, "alloc_fd(): function is not implemented!\n");

	return -1;
}

void dump_stack(void)
{
	pid_t		pid = getpid();
	pthread_t	tid = pthread_self();
	const char	*path = "/log/coredumps";
	char	cmd[100];

	snprintf(cmd, sizeof(cmd),
		"dumper -nmz9 -p %d -d %s", pid, path);

	printk(KERN_ERR "## %s[%d] start pid=%d\n",
			__func__, tid, pid);
	if (system("/mnt/drm_warning.sh")) {
		printk(KERN_ERR "## %s[%d]  /mnt/drm_warning.sh is not found. nothing to do.\n",
			__func__, tid);
		//system(cmd);
	}
	printk(KERN_ERR "## %s[%d] finish\n",
			__func__, tid);
}

struct device *next_device(struct klist_iter *i);

/**
 * device_for_each_child - device child iterator.
 * @parent: parent struct device.
 * @fn: function to be called for each device.
 * @data: data for the callback.
 *
 * Iterate over @parent's child devices, and call @fn for each,
 * passing it @data.
 *
 * We check the return of @fn each time. If it returns anything
 * other than 0, we break out and return that value.
 */
int device_for_each_child(struct device *parent, void *data,
			  int (*fn)(struct device *dev, void *data))
{
	struct klist_iter i;
	struct device *child;
	int error = 0;

	if (!parent->p)
		return 0;

	klist_iter_init(&parent->p->klist_children, &i);
	while ((child = next_device(&i)) && !error)
		error = fn(child, data);
	klist_iter_exit(&i);
	return error;
}

#define to_device_private_parent(obj) container_of(obj, struct device_private, knode_parent)

static void klist_children_get(struct klist_node *n)
{
    struct device_private *p = to_device_private_parent(n);
    struct device *dev = p->device;

    get_device(dev);
}

static void klist_children_put(struct klist_node *n)
{
    struct device_private *p = to_device_private_parent(n);
    struct device *dev = p->device;

    put_device(dev);
}

int device_private_init(struct device *dev)
{
    dev->p = kzalloc(sizeof(*dev->p), GFP_KERNEL);
    if (!dev->p)
        return -ENOMEM;
    dev->p->device = dev;
    klist_init(&dev->p->klist_children, klist_children_get, klist_children_put);
    INIT_LIST_HEAD(&dev->p->deferred_probe);
    return 0;
}

/**
 * device_register - register a device with the system.
 * @dev: pointer to the device structure
 *
 * This happens in two clean steps - initialize the device
 * and add it to the system. The two steps can be called
 * separately, but this is the easiest and most common.
 * I.e. you should only call the two helpers separately if
 * have a clearly defined need to use and refcount the device
 * before it is added to the hierarchy.
 *
 * For more information, see the kerneldoc for device_initialize()
 * and device_add().
 *
 * NOTE: _Never_ directly free @dev after calling this function, even
 * if it returned an error! Always use put_device() to give up the
 * reference initialized in this function instead.
 */
int device_register(struct device *dev)
{
	device_initialize(dev);
	return device_add(dev);
}
EXPORT_SYMBOL_GPL(device_register);

static void device_create_release(struct device *dev)
{
        DRM_DEBUG("device: '%s': %s\n", dev_name(dev), __func__);
        kfree(dev);
}


#define to_dev(obj) container_of(obj, struct device, kobj)

struct device *get_device(struct device *dev)
{
        return dev ? to_dev(kobject_get(&dev->kobj)) : NULL;
}

void put_device(struct device *dev)
{
         /* might_sleep(); */
         if (dev)
                kobject_put(&dev->kobj);
}

struct device *device_create_vargs(struct class *class, struct device *parent,
    dev_t devt, void *drvdata, const char *fmt, va_list args)
{
    struct device *dev = NULL;
    int retval = -ENODEV;

    if (class == NULL || IS_ERR(class)) {
        DRM_ERROR("bad class!\n");
        goto error;
    }

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev) {
        retval = -ENOMEM;
        DRM_ERROR("can't allocate device structure!\n");
        goto error;
    }

    dev->devt = devt;
    dev->class = class;
    dev->parent = parent;
    dev->release = device_create_release;
    dev_set_drvdata(dev, drvdata);

    /* Initialize power interface properly to SUSPENDED state by default */
    dev->power.runtime_status = RPM_SUSPENDED;
    atomic_set(&dev->power.usage_count, 0);

    retval = kobject_set_name_vargs(&dev->kobj, fmt, args);
    if (retval) {
        DRM_ERROR("device creation failed!\n");
        goto error;
    }

    retval = device_register(dev);
    if (retval) {
        DRM_ERROR("device registration failed!\n");
        goto error;
    }

    return dev;

error:
    put_device(dev);
    return ERR_PTR(retval);
}

struct device *device_create(struct class *class, struct device *parent,
                             dev_t devt, void *drvdata, const char *fmt, ...)
{
	va_list vargs;
	struct device *dev;

	va_start(vargs, fmt);
	dev = device_create_vargs(class, parent, devt, drvdata, fmt, vargs);
	va_end(vargs);
	return dev;
}

void device_destroy(struct class *class, dev_t devt)
{
        struct device *dev = NULL;

        //TODO: add searching and destroying device
//        dev = class_find_device(class, NULL, &devt, __match_devt);
        if (dev) {
                put_device(dev);
                device_unregister(dev);
        }
}

static void fasync_free_rcu(struct rcu_head *head)
{
	kmem_cache_free(fasync_cache, container_of(head, struct fasync_struct, fa_rcu));
}

/*
 * Remove a fasync entry. If successfully removed, return
 * positive and clear the FASYNC flag. If no entry exists,
 * do nothing and return 0.
 *
 * NOTE! It is very important that the FASYNC flag always
 * match the state "is the filp on a fasync list".
 *
 */
int fasync_remove_entry(struct file *filp, struct fasync_struct **fapp)
{
	struct fasync_struct *fa, **fp;
	int result = 0;

	spin_lock(&filp->f_lock);
	spin_lock(&fasync_lock);
	for (fp = fapp; (fa = *fp) != NULL; fp = &fa->fa_next) {
		if (fa->fa_file != filp)
			continue;

		spin_lock_irq(&fa->fa_lock);
		fa->fa_file = NULL;
		spin_unlock_irq(&fa->fa_lock);

		*fp = fa->fa_next;
		call_rcu(&fa->fa_rcu, fasync_free_rcu);
		filp->f_flags &= ~FASYNC;
		result = 1;
		break;
	}
	spin_unlock(&fasync_lock);
	spin_unlock(&filp->f_lock);
	return result;
}

struct fasync_struct *fasync_alloc(void)
{
	return kmem_cache_alloc(fasync_cache, GFP_KERNEL);
}

/*
 * Insert a new entry into the fasync list.  Return the pointer to the
 * old one if we didn't use the new one.
 *
 * NOTE! It is very important that the FASYNC flag always
 * match the state "is the filp on a fasync list".
 */
struct fasync_struct *fasync_insert_entry(int fd, struct file *filp, struct fasync_struct **fapp, struct fasync_struct *new)
{
        struct fasync_struct *fa, **fp;

        spin_lock(&filp->f_lock);
        spin_lock(&fasync_lock);
        for (fp = fapp; (fa = *fp) != NULL; fp = &fa->fa_next) {
                if (fa->fa_file != filp)
                        continue;

                spin_lock_irq(&fa->fa_lock);
                fa->fa_fd = fd;
                spin_unlock_irq(&fa->fa_lock);
                goto out;
        }
        // TODO: required spin destroy
        spin_lock_init(&new->fa_lock);
        new->magic = FASYNC_MAGIC;
        new->fa_file = filp;
        new->fa_fd = fd;
        new->fa_next = *fapp;
        rcu_assign_pointer(*fapp, new);
        filp->f_flags |= FASYNC;

out:
        spin_unlock(&fasync_lock);
        spin_unlock(&filp->f_lock);
        return fa;
}

/*
 * NOTE! This can be used only for unused fasync entries:
 * entries that actually got inserted on the fasync list
 * need to be released by rcu - see fasync_remove_entry.
 */
void fasync_free(struct fasync_struct *new)
{
        kmem_cache_free(fasync_cache, new);
}

/*
 * Add a fasync entry. Return negative on error, positive if
 * added, and zero if did nothing but change an existing one.
 */
static int fasync_add_entry(int fd, struct file *filp, struct fasync_struct **fapp)
{
        struct fasync_struct *new;

        new = fasync_alloc();
        if (!new)
                return -ENOMEM;

        /*
         * fasync_insert_entry() returns the old (update) entry if
         * it existed.
         *
         * So free the (unused) new entry and return 0 to let the
         * caller know that we didn't add any new fasync entries.
         */
         if (fasync_insert_entry(fd, filp, fapp, new)) {
                fasync_free(new);
                return 0;
         }

         return 1;
}

/*
 * fasync_helper() is used by almost all character device drivers
 * to set up the fasync queue, and for regular files by the file
 * lease code. It returns negative on error, 0 if it did no changes
 * and positive if it added/deleted the entry.
 */
int fasync_helper(int fd, struct file * filp, int on, struct fasync_struct **fapp)
{
        if (!on)
                return fasync_remove_entry(filp, fapp);
        return fasync_add_entry(fd, filp, fapp);
}


/*TODO, remove me */
int
i915_gem_userptr_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	return 0;
}
int
i915_gem_init_userptr(struct drm_device *dev)
{
	return 0;
}

struct rcu_state {
	const char *name;
};

/* Index values for nxttail array in struct rcu_data. */
#define RCU_DONE_TAIL		0	/* Also RCU_WAIT head. */
#define RCU_WAIT_TAIL		1	/* Also RCU_NEXT_READY head. */
#define RCU_NEXT_READY_TAIL	2	/* Also RCU_NEXT head. */
#define RCU_NEXT_TAIL		3
#define RCU_NEXT_SIZE		4

/*
 * Definition for node within the RCU grace-period-detection hierarchy.
 */
struct rcu_node {
	raw_spinlock_t lock;	/* Root rcu_node's lock protects some */
				/*  rcu_state fields as well as following. */
	unsigned long gpnum;	/* Current grace period for this node. */
				/*  This will either be equal to or one */
				/*  behind the root rcu_node's gpnum. */
	unsigned long completed; /* Last GP completed for this node. */
				/*  This will either be equal to or one */
				/*  behind the root rcu_node's gpnum. */
	unsigned long qsmask;	/* CPUs or groups that need to switch in */
				/*  order for current grace period to proceed.*/
				/*  In leaf rcu_node, each bit corresponds to */
				/*  an rcu_data structure, otherwise, each */
				/*  bit corresponds to a child rcu_node */
				/*  structure. */
	unsigned long expmask;	/* Groups that have ->blkd_tasks */
				/*  elements that need to drain to allow the */
				/*  current expedited grace period to */
				/*  complete (only for PREEMPT_RCU). */
	unsigned long qsmaskinit;
				/* Per-GP initial value for qsmask & expmask. */
				/*  Initialized from ->qsmaskinitnext at the */
				/*  beginning of each grace period. */
	unsigned long qsmaskinitnext;
				/* Online CPUs for next grace period. */
	unsigned long grpmask;	/* Mask to apply to parent qsmask. */
				/*  Only one bit will be set in this mask. */
	int	grplo;		/* lowest-numbered CPU or group here. */
	int	grphi;		/* highest-numbered CPU or group here. */
	u8	grpnum;		/* CPU/group number for next level up. */
	u8	level;		/* root is at level 0. */
	bool	wait_blkd_tasks;/* Necessary to wait for blocked tasks to */
				/*  exit RCU read-side critical sections */
				/*  before propagating offline up the */
				/*  rcu_node tree? */
	struct rcu_node *parent;
	struct list_head blkd_tasks;
				/* Tasks blocked in RCU read-side critical */
				/*  section.  Tasks are placed at the head */
				/*  of this list and age towards the tail. */
	struct list_head *gp_tasks;
				/* Pointer to the first task blocking the */
				/*  current grace period, or NULL if there */
				/*  is no such task. */
	struct list_head *exp_tasks;
				/* Pointer to the first task blocking the */
				/*  current expedited grace period, or NULL */
				/*  if there is no such task.  If there */
				/*  is no current expedited grace period, */
				/*  then there can cannot be any such task. */
	int need_future_gp[2];
				/* Counts of upcoming no-CB GP requests. */
	raw_spinlock_t fqslock;
};

/* Per-CPU data for read-copy update. */
struct rcu_data {
	/* 1) quiescent-state and grace-period handling : */
	unsigned long	completed;	/* Track rsp->completed gp number */
					/*  in order to detect GP end. */
	unsigned long	gpnum;		/* Highest gp number that this CPU */
					/*  is aware of having started. */
	unsigned long	rcu_qs_ctr_snap;/* Snapshot of rcu_qs_ctr to check */
					/*  for rcu_all_qs() invocations. */
	bool		passed_quiesce;	/* User-mode/idle loop etc. */
	bool		qs_pending;	/* Core waits for quiesc state. */
	bool		beenonline;	/* CPU online at least once. */
	bool		gpwrap;		/* Possible gpnum/completed wrap. */
	struct rcu_node *mynode;	/* This CPU's leaf of hierarchy */
	unsigned long grpmask;		/* Mask to apply to leaf qsmask. */

	/* 2) batch handling */
	/*
	 * If nxtlist is not NULL, it is partitioned as follows.
	 * Any of the partitions might be empty, in which case the
	 * pointer to that partition will be equal to the pointer for
	 * the following partition.  When the list is empty, all of
	 * the nxttail elements point to the ->nxtlist pointer itself,
	 * which in that case is NULL.
	 *
	 * [nxtlist, *nxttail[RCU_DONE_TAIL]):
	 *	Entries that batch # <= ->completed
	 *	The grace period for these entries has completed, and
	 *	the other grace-period-completed entries may be moved
	 *	here temporarily in rcu_process_callbacks().
	 * [*nxttail[RCU_DONE_TAIL], *nxttail[RCU_WAIT_TAIL]):
	 *	Entries that batch # <= ->completed - 1: waiting for current GP
	 * [*nxttail[RCU_WAIT_TAIL], *nxttail[RCU_NEXT_READY_TAIL]):
	 *	Entries known to have arrived before current GP ended
	 * [*nxttail[RCU_NEXT_READY_TAIL], *nxttail[RCU_NEXT_TAIL]):
	 *	Entries that might have arrived after current GP ended
	 *	Note that the value of *nxttail[RCU_NEXT_TAIL] will
	 *	always be NULL, as this is the end of the list.
	 */
	struct rcu_head *nxtlist;
	struct rcu_head **nxttail[RCU_NEXT_SIZE];
	unsigned long	nxtcompleted[RCU_NEXT_SIZE];
					/* grace periods for sublists. */
	long		qlen_lazy;	/* # of lazy queued callbacks */
	long		qlen;		/* # of queued callbacks, incl lazy */
	long		qlen_last_fqs_check;
					/* qlen at last check for QS forcing */
	unsigned long	n_cbs_invoked;	/* count of RCU cbs invoked. */
	unsigned long	n_nocbs_invoked; /* count of no-CBs RCU cbs invoked. */
	unsigned long   n_cbs_orphaned; /* RCU cbs orphaned by dying CPU */
	unsigned long   n_cbs_adopted;  /* RCU cbs adopted from dying CPU */
	unsigned long	n_force_qs_snap;
					/* did other CPU force QS recently? */
	long		blimit;		/* Upper limit on a processed batch */

	/* 4) reasons this CPU needed to be kicked by force_quiescent_state */
	unsigned long dynticks_fqs;	/* Kicked due to dynticks idle. */
	unsigned long offline_fqs;	/* Kicked due to being offline. */
	unsigned long cond_resched_completed;
					/* Grace period that needs help */
					/*  from cond_resched(). */

	/* 5) __rcu_pending() statistics. */
	unsigned long n_rcu_pending;	/* rcu_pending() calls since boot. */
	unsigned long n_rp_qs_pending;
	unsigned long n_rp_report_qs;
	unsigned long n_rp_cb_ready;
	unsigned long n_rp_cpu_needs_gp;
	unsigned long n_rp_gp_completed;
	unsigned long n_rp_gp_started;
	unsigned long n_rp_nocb_defer_wakeup;
	unsigned long n_rp_need_nothing;

	/* 6) _rcu_barrier() and OOM callbacks. */
	struct rcu_head barrier_head;

	int cpu;
	struct rcu_state *rsp;
};

struct rcu_state rcu_sched_state;

LIST_HEAD(aliases_lookup);

/**
 * struct alias_prop - Alias property in 'aliases' node
 * @link:	List node to link the structure in aliases_lookup list
 * @alias:	Alias property name
 * @np:		Pointer to device_node that the alias stands for
 * @id:		Index value from end of alias name
 * @stem:	Alias string without the index
 *
 * The structure represents one alias property of 'aliases' node as
 * an entry in aliases_lookup list.
 */
struct alias_prop {
	struct list_head link;
	const char *alias;
	struct device_node *np;
	int id;
	char stem[0];
};

/**
 * of_alias_get_highest_id - Get highest alias id for the given stem
 * @stem:	Alias stem to be examined
 *
 * The function travels the lookup table to get the highest alias id for the
 * given alias stem.  It returns the alias id if found.
 */
int of_alias_get_highest_id(const char *stem)
{
	struct alias_prop *app;
	int id = -ENODEV;

	list_for_each_entry(app, &aliases_lookup, link) {
		if (strcmp(app->stem, stem) != 0)
			continue;

		if (app->id > id)
			id = app->id;
	}

	return id;
}
EXPORT_SYMBOL_GPL(of_alias_get_highest_id);

bool static_key_initialized;

static pthread_mutex_t	rcu_lock_mutex = PTHREAD_MUTEX_INITIALIZER;

void __rcu_read_lock(void)
{
	/* MG_TODO: we should provide a real lock here */
	local_irq_disable();
	pthread_mutex_lock(&rcu_lock_mutex);
}

void __rcu_read_unlock(void)
{
	/* MG_TODO: we should provide a real lock here */
	pthread_mutex_unlock(&rcu_lock_mutex);
	local_irq_enable();
}

void call_rcu_sched(struct rcu_head *head, void (*func)(struct rcu_head *rcu))
{
	if (func != NULL) {
		/* MG_TODO: we should provide a real lock here */
		local_irq_disable();
		pthread_mutex_lock(&rcu_lock_mutex);
		func(head);
		pthread_mutex_unlock(&rcu_lock_mutex);
		local_irq_enable();
	}
}
EXPORT_SYMBOL_GPL(call_rcu_sched);

#define DEFINE_RCU_TPS(sname)
#define RCU_STATE_NAME(sname) __stringify(sname)

#define RCU_STATE_INITIALIZER(sname, sabbr, cr) \
DEFINE_RCU_TPS(sname) \
DEFINE_PER_CPU_SHARED_ALIGNED(struct rcu_data, sname##_data); \
struct rcu_state sname##_state = { \
	.name = RCU_STATE_NAME(sname), \
}

RCU_STATE_INITIALIZER(rcu_preempt, 'p', call_rcu);
static struct rcu_state *rcu_state_p = &rcu_preempt_state;

/**
 * rcu_barrier - Wait until all in-flight call_rcu() callbacks complete.
 *
 * Note that this primitive does not necessarily wait for an RCU grace period
 * to complete.  For example, if there are no RCU callbacks queued anywhere
 * in the system, then rcu_barrier() is within its rights to return
 * immediately, without waiting for anything, much less an RCU grace period.
 */
void rcu_barrier(void)
{
}
EXPORT_SYMBOL_GPL(rcu_barrier);

struct page *kmap_to_page(void *vaddr)
{
	/* Under QNX we can't provide map from virtual address to corresponding page, so   */
	/* that's why function is_vmalloc_addr() always returns true, just to avoid usage  */
	/* of this function. */
	fprintf(stderr, "***ERROR***: kmap_to_page() is not implemented by reason and should be avoided!\n");
	BUG();

	return NULL;
}

/*
 * smp_call_function_single - Run a function on a specific CPU
 * @func: The function to run. This must be fast and non-blocking.
 * @info: An arbitrary pointer to pass to the function.
 * @wait: If true, wait until function has completed on other CPUs.
 *
 * Returns 0 on success, else a negative status code.
 */
int smp_call_function_single(int cpuid, smp_call_func_t func, void *info, int wait)
{
	func(info);

	return 0;
}

void kunmap_atomic(void *addr)
{
}

void kunmap(struct page *page)
{
}

void* kmap_atomic(struct page* page)
{
	if (page == NULL) {
		BUG();
		abort();
	}

	return page_address(page);
}

void* kmap(struct page* page)
{
	if (page == NULL) {
		BUG();
		abort();
	}

	return page_address(page);
}

int sprint_symbol(char *buffer, unsigned long addr)
{
	/* MG_TODO: replace this with dladdr() calls to retrieve function name by address */
	sprintf(buffer, "%016llx", (unsigned long long)addr);
	return 0;
}

int sprint_symbol_no_offset(char *buffer, unsigned long addr)
{
	sprintf(buffer, "%016llx", (unsigned long long)addr);
	return 0;
}

int sprint_backtrace(char *buffer, unsigned long addr)
{
	sprintf(buffer, "%016llx", (unsigned long long)addr);
	return 0;
}

/**
  * uuid_is_valid - checks if UUID string valid
  * @uuid:	UUID string to check
  *
  * Description:
  * It checks if the UUID string is following the format:
  *	xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
  * where x is a hex digit.
  *
  * Return: true if input is valid UUID string.
  */
bool uuid_is_valid(const char *uuid)
{
	unsigned int i;

	for (i = 0; i < UUID_STRING_LEN; i++) {
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			if (uuid[i] != '-')
				return false;
		} else if (!isxdigit(uuid[i])) {
			return false;
		}
	}

	return true;
}
EXPORT_SYMBOL(uuid_is_valid);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/stubs.c $ $Rev: 873525 $")
#endif
