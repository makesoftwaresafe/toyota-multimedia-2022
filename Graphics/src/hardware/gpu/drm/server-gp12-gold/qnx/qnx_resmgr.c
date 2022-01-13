/*
 *  drm.c
 *
 *  This module contains the source code for a resource manager
 *  with multi-threaded handling of client requests using
 *  a thread pool.
 *
 */

struct _drm_device;
struct _drm_ocb;

/* Define our overrides before including <sys/iofunc.h>  */
#undef IOFUNC_ATTR_T
#undef IOFUNC_OCB_T
#undef THREAD_POOL_PARAM_T
#define IOFUNC_ATTR_T       struct _drm_device
#define IOFUNC_OCB_T        struct _drm_ocb
#define THREAD_POOL_PARAM_T dispatch_context_t

#include <libgen.h>
#include <sys/neutrino.h>
#include <sys/resource.h>
#include <sys/procfs.h>
#include <linux/mman.h>

#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/qnx_resmgr.h>
#include <linux/sched.h>

#include <string.h>
#include <devctl.h>
#include <signal.h>
#include <sys/resmgr.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <wordexp.h>

/* For ELF dynamic loader support */
#include <sys/link.h>

#include "drmP.h"

#include "libelf.h"
#include "mmap_trace.h"

#include <iotg_build.h>

int qnx_i915_init(void);
void qnx_i915_exit(void);

int qnx_pci_init(void);
void qnx_pci_fini(void);

int qnx_wordexp(const char *words, wordexp_t* pwordexp, int flags);
void qnx_wordfree(wordexp_t* pwordexp);
int __libc_argc;
char **__libc_argv;
#define DRM_CMDLINE_MAX_SIZE 4096

int debugfs_enable = 0;
static int resmgr_link_id = -1;
dispatch_t* dpp = NULL;
static thread_pool_t* tpp = NULL;

// globals
int last_inode;
unsigned long _jiffies;
enum system_states system_state = SYSTEM_RUNNING;
struct attr_list minors_list;   /**< Linked list of minors */
struct attr_list groups_list;   /**< Linked list of groups */

//  I/O functions
static int io_read(resmgr_context_t *ctp, io_read_t *msg, drm_ocb_t *func);
static int io_write(resmgr_context_t *ctp, io_write_t *msg, drm_ocb_t *tocb);
static int io_stat(resmgr_context_t *ctp, io_stat_t *msg, drm_ocb_t *func);
static int io_devctl(resmgr_context_t *ctp, io_devctl_t *msg,drm_ocb_t *ocb);
static int io_open(resmgr_context_t *ctp, io_open_t *msg, drm_device_t *attr, void *extra);
static int io_close(resmgr_context_t *ctp, void* reserved, drm_ocb_t *ocb);
static int io_unblock(resmgr_context_t *ctp, io_pulse_t *msg, drm_ocb_t *ocb);
static int io_mmap(resmgr_context_t *ctp, io_mmap_t *msg, drm_ocb_t* ocb);
static int io_notify(resmgr_context_t* ctp, io_notify_t* msg, drm_ocb_t* ocb);

// resmgr state
static resmgr_connect_funcs_t  drm_connect_funcs;
static resmgr_io_funcs_t       drm_io_funcs;
static resmgr_attr_t           rattr;
static drm_device_t            drm_attrs;
static drm_device_ext_data_t*  attr_priv;
#define DRM_DIR "/dev/drm"

// miscellaneous
static char *progname = "[drm-intel]";          // for diagnostic messages
static void options_debug(int argc, char **argv);
static int options(int argc, char **argv);
static void* parse_modinfo(int argc, char **argv);
void drm_resmgr_stop(void);
void cpu_init();

#define DRM_SERVER_PARMS_COUNT 256

static struct _module_options {
    char* parameter_name;
    char* parameter_desc;
    char* parameter_type;
    uintptr_t parameter_addr;
    uintptr_t parameter_size;
} module_options[DRM_SERVER_PARMS_COUNT];

static int module_options_count = 0;

extern char firmware_path[PATH_MAX];

#define DIRENT_ALIGN(x) (((x) + 3) & ~3)

/******************************************************************************/
#define MAX_SIMULTANEOUS_TASKS 512
static struct task_struct tasks[MAX_SIMULTANEOUS_TASKS];
static struct task_struct main_task;
__thread struct task_struct* current;

static pthread_mutex_t tasks_mutex = PTHREAD_MUTEX_INITIALIZER;

#define TASK_COUNT_DEBUG
#undef  TASK_COUNT_DEBUG
#ifdef TASK_COUNT_DEBUG
static inline void qnx_task_count_debug(struct task_struct *p, const char *txt)
{
	printk(KERN_INFO "%s[%d] %-.*s %d:%d cnt=%d opened=%d:%d cnt=%d\n",
			txt, pthread_self(),
			(int)sizeof(p->comm), p->comm,
			p->pid, p->attachment.user_tid, p->usage_count,
			p->attachment.opened ? p->attachment.opened->pid : -1,
			p->attachment.opened ? p->attachment.opened->attachment.user_tid : -1,
			p->attachment.opened ? p->attachment.opened->usage_count : -1);
}
#else
#define qnx_task_count_debug(p,c)
#endif

static inline void qnx_get_task_locked(struct task_struct* task)
{
	task->usage_count++;
}
void qnx_get_task(struct task_struct* task)
{
	pthread_mutex_lock(&tasks_mutex);
	qnx_get_task_locked(task);
	pthread_mutex_unlock(&tasks_mutex);
}

static bool qnx_put_task_locked(struct task_struct* task)
{
	task->usage_count--;
	if (!task->usage_count) {
		if (task->attachment.opened) {
			qnx_task_count_debug(task, "put_task, closed");
		}
		memset(task, 0, sizeof(*task));
		qnx_destroy_task_sched(task);

		return true;
	}
	return false;
}
bool qnx_put_task(struct task_struct* task)
{
	bool	ret;

	pthread_mutex_lock(&tasks_mutex);
	ret = qnx_put_task_locked(task);
	pthread_mutex_unlock(&tasks_mutex);
	return ret;
}

struct task_struct* qnx_find_create_task(resmgr_context_t *ctp, int* need_init)
{
	int it;
	struct task_struct	*p, *opened, *vacant;

	opened = NULL;
	vacant = NULL;
	pthread_mutex_lock(&tasks_mutex);

	for (it=0; it<MAX_SIMULTANEOUS_TASKS; it++) {
		p = &tasks[it];

		if (!vacant && !p->pid) {
			vacant = p;
		}
		if (p->pid != ctp->info.pid) {
			continue;
		}
		if (p->attachment.opened && p->attachment.opened == p) {
			/*
			 * opened task
			 *
			 * Note: when multiple thread opened,
			 * mark first opened task
			 * when closed the opened task,
			 * check other opened thread
			 */
			opened = p;
		}
		if (p->attachment.user_tid != ctp->info.tid) {
			continue;
		}
		/*
		 * found the same task
		 */
		if (need_init) {
			*need_init = 0;
		}
		pthread_mutex_unlock(&tasks_mutex);
		return p;
	}

	if (vacant) {
		/*
		 * create new task
		 */
		p = vacant;

		memset(p, 0, sizeof(*p));
		p->pid = ctp->info.pid;
		p->attachment.user_tid = ctp->info.tid;
		p->usage_count = 0;

		if (opened) {
			p->attachment.opened = opened;
			qnx_get_task_locked(p);	/* child also opened */
			qnx_task_count_debug(p, __func__);
		}
		if (need_init) {
			*need_init = 1;
		}
		pthread_mutex_unlock(&tasks_mutex);
		return p;
	}

	fprintf(stderr, "%s(): can't find/create task for a client %d[%d]!\n",
			__FUNCTION__, ctp->info.pid, ctp->info.tid);
	pthread_mutex_unlock(&tasks_mutex);

	return NULL;
}

struct task_struct* qnx_find_task(pid_t pid, _Int32t tid)
{
	int it;

	pthread_mutex_lock(&tasks_mutex);

	for (it=0; it<MAX_SIMULTANEOUS_TASKS; it++) {
		if (tasks[it].pid == pid) {
			if (tasks[it].attachment.user_tid != tid) {
				continue;
			}

			pthread_mutex_unlock(&tasks_mutex);
			return &tasks[it];
		}
	}

	pthread_mutex_unlock(&tasks_mutex);

	return NULL;
}

static void qnx_close_tasks_locked(struct task_struct *opener,
		pid_t closed_pid)
{
	struct task_struct	*p, *opened;
	int	it, follows;

	opened = NULL;
	follows = 0;
	/*
	 * at first, scan all tasks
	 * - search other opened task
	 * - count followed tasks
	 */
	for (it = 0; it < MAX_SIMULTANEOUS_TASKS; it++) {
		p = &tasks[it];
		if (p->pid == closed_pid &&
				p->attachment.opened == p && opener != p) {
			/*
			 * another opened task exists
			 */
			opened = p;
		}
		if (p->attachment.opened == opener) {
			follows++;
		}
	}
#ifdef TASK_COUNT_DEBUG
	printk(KERN_INFO "%s[%d] pid=%d other opened=:%d "
			"%d follows child\n",
			__func__, pthread_self(), closed_pid,
			opened ? opened->attachment.user_tid : 0,
			follows);
#endif
	/*
	 * close tasks when owner=closed task
	 * however other opened task exist, move owner
	 */
	for (it = 0; follows && it < MAX_SIMULTANEOUS_TASKS; it++) {
		p = &tasks[it];
		if (p->attachment.opened == opener) {
			follows--;
			if (opened) {
				/*
				 * replace owner
				 */
				p->attachment.opened = opened;

				qnx_task_count_debug(p,
					"close_tasks, replace owner");
			} else {
				qnx_task_count_debug(p,
					"close_tasks, followed owner");
				qnx_put_task_locked(p);
			}
		}
	}
}

#define	system_call(syscall)	({ \
	int r = syscall; \
	if (r) { \
		qnx_error("%s[%d] %s = %d",	\
			__func__, pthread_self(),	\
			#syscall, r);	\
	} \
	r; })
#define	system_call_errno(syscall)	({ \
	int r = syscall; \
	if (r < 0) { \
		qnx_error("%s[%d] %s = %d\n",	\
			__func__, pthread_self(),	\
			#syscall, errno);	\
		r = -errno; \
	} \
	r; })

int qnx_current_task_sched(struct task_struct *task)
{
	int err;

	if (	(err = system_call(
			pthread_getschedparam(pthread_self(),
				&task->sched_policy,
				&task->sched_param))) ) {
		goto err;
	}
	task->sched_max_priority = system_call_errno(
			sched_get_priority_max(task->sched_policy));
	if (task->sched_max_priority < 0) {
		err = -EINVAL;
		goto err;
	}
	task->sched_high_priority = qnx_get_priority(QNX_PRTY_HIGH, task);
#ifdef DEBUG_SCHED_PARAM
	printk(KERN_INFO "DEBUG %s[%d] %s policy=%d prio=%d:%d:%d\n",
			__func__, pthread_self(), task->comm,
			task->sched_policy,
			task->sched_param.sched_priority,
			task->sched_max_priority,
			task->sched_high_priority);
#endif
err:
	return err;
}

/* These two functions are just condvar mutex creation */
/* helpers, they do not depend on any locks.           */
int qnx_create_task_sched(struct task_struct* task)
{
	pthread_condattr_t cond_attr;
	int err = EOK;

	if (	(err = system_call(
			pthread_mutex_init(&task->sched_mutex, NULL)))) {
		goto err;
	}
	if (	(err = system_call(
			pthread_condattr_init(&cond_attr)))) {
		goto err_mutex;
	}
	if (	(err = system_call(
			pthread_condattr_setclock(&cond_attr,
				CLOCK_MONOTONIC))) ||
		(err = system_call(
			pthread_cond_init(&task->sched_cond,
				&cond_attr))) ) {
		goto err_condattr;
	}
	if (	(err = qnx_current_task_sched(task)) ) {
		goto err_cond;
	}

	return 0;

err_cond:
	pthread_cond_destroy(&task->sched_cond);
err_condattr:
	pthread_condattr_destroy(&cond_attr);
err_mutex:
	pthread_mutex_destroy(&task->sched_mutex);
err:
	return err;
}

void qnx_destroy_task_sched(struct task_struct* task)
{
	pthread_cond_destroy(&task->sched_cond);
	pthread_mutex_destroy(&task->sched_mutex);
}

/******************************************************************************/

void task_structure_set_signal_locked(resmgr_context_t *ctp, struct task_struct* task)
{
	struct _msg_info info;

	/* Set pending signal only if it was raised */
	task->signal = false;

	/* If rcvid is zero this means "fake" io_close() call */
	if (ctp->rcvid != 0) {
		if (MsgInfo(ctp->rcvid, &info) != -1) {
			if (info.flags & _NTO_MI_UNBLOCK_REQ) {
				task->signal = true;
			}
		} else {
			/* Connection to client is broken, assume it as */
			/* fatal signal / unblock request.              */
			task->signal = true;
		}
	}
}

/* Currently we distinguish all connections from clients by pid only. */
/* This means that all connections from same process sharing linux    */
/* synchronization primitives. If event happens on one connection     */
/* it wakes up blocked clients on other connections which share the   */
/* same pid. Performance impact of this solution is very minimal and  */
/* currently we don't have multiple rendering clients allowed from    */
/* one process due to libkhronos/Mesa3D limitation.                   */

static void set_comm(struct task_struct *task, resmgr_context_t *ctp)
{
	char process_path[PATH_MAX];
	struct {
		procfs_debuginfo info;
		char name[PATH_MAX];
	} debuginfo;
	int fd, ret;

	snprintf(task->comm, sizeof(task->comm),
			"?:%d:%d", ctp->info.pid, ctp->info.tid);
	task->comm[sizeof(task->comm) - 1] = '\0';

	snprintf(process_path, sizeof(process_path) - 1,
			"/proc/%d/as", ctp->info.pid);
	fd = open(process_path, O_RDONLY);
	if (fd < 0) {
		return;	/* cannot set comm */
	}

	debuginfo.info.path[0] = '0';
	ret = devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &debuginfo,
			sizeof(debuginfo), NULL);
	if (ret != EOK || *debuginfo.info.path == '\0') {
		;
	} else {
		snprintf(task->comm, sizeof(task->comm),
				"%s:%d",
				basename(debuginfo.info.path),
				ctp->info.tid);
		task->comm[sizeof(task->comm) - 1] = '\0';
	}
	close(fd);
}

int claim_task_structure(resmgr_context_t *ctp, int inopen)
{
	struct task_struct* task;
	int ret;

	if (!ctp) {
		return -1;
	}

	task = qnx_find_create_task(ctp, &ret);
	if (!task) {
		return -1;
	}
	if (!ret) {
		/*
		 * set sched_param at first, even no initial request
		 */
		ret = qnx_current_task_sched(task);
		if (ret) {
			return -1;
		}
	} else {
		struct _client_info cinfo;

		ret = qnx_create_task_sched(task);
		if (ret) {
			/* Error output is done inside qnx_create_task_sched() function */
			return -1;
		}

		task->task_state = TASK_RUNNING;
		task->spid.pid = task->pid;
		task->spid.tid = task->attachment.user_tid;
		ret = ConnectClientInfo_r(ctp->info.scoid, &cinfo, NGROUPS_MAX);
		if (ret == EOK) {
			task->real_cred = &task->cred_vault;
			task->cred = &task->cred_vault;
			task->cred_vault.uid.val = cinfo.cred.ruid;
			task->cred_vault.gid.val = cinfo.cred.rgid;
			task->cred_vault.suid.val = cinfo.cred.suid;
			task->cred_vault.sgid.val = cinfo.cred.sgid;
			task->cred_vault.euid.val = cinfo.cred.euid;
			task->cred_vault.egid.val = cinfo.cred.egid;
			task->cred_vault.fsuid.val = cinfo.cred.euid;
			task->cred_vault.fsgid.val = cinfo.cred.egid;
		} else {
			fprintf(stderr, "%s(): can't get client info for scoid %d, errno: %d\n", __FUNCTION__, ctp->info.scoid, errno);
		}
		set_comm(task, ctp);
	}

	/*
	 * change current priority, if too high
	 */
	task->sched_requestor_priority = 0;
	if (task->sched_param.sched_priority >=
			qnx_get_priority(QNX_PRTY_DRM_MIN, task)) {
		task->sched_requestor_priority =
			task->sched_param.sched_priority;
		task->sched_param.sched_priority =
			qnx_get_priority(QNX_PRTY_USER_MAX, task);
		ret = system_call(
			pthread_setschedprio(
				pthread_self(),
				task->sched_param.sched_priority));
		if (ret) {
			return -1;
		}
	}

	pthread_mutex_lock(&tasks_mutex);
	task_structure_set_signal_locked(ctp, task);
	pthread_mutex_unlock(&tasks_mutex);

	pthread_mutex_lock(&tasks_mutex);
	qnx_get_task_locked(task);

	current = task;

	task->attachment.uaddr_cache_id = uaddr_get(
			task->attachment.uaddr_cache_id, task->pid);
	if (inopen) {
		if (!task->attachment.opened) {
			/*
			 * mark opened, at first open
			 */
			task->attachment.opened = task;

			task->attachment.uaddr_cache_id = uaddr_get(
					task->attachment.uaddr_cache_id,
					task->pid);
		}

		/*
		 * Increase usage count for every open
		 * -> increase opener, 2nd open might by other thread
		 */
		qnx_get_task_locked(task->attachment.opened);
		qnx_task_count_debug(task, "==open==");
	}
	pthread_mutex_unlock(&tasks_mutex);

	task->attachment.copy_to_user_memcpy = 0;

	return 0;
}

int unclaim_task_structure(resmgr_context_t *ctp, int inclose)
{
	struct task_struct* task;
	int ret;

	if (!ctp) {
		return -1;
	}

	task = qnx_find_task(ctp->info.pid, ctp->info.tid);
	if (!task) {
		return -1;
	}

	if (task->sched_requestor_priority) {
		ret = system_call(
			pthread_setschedprio(
				pthread_self(),
				task->sched_requestor_priority));
		if (ret) {
			return -1;
		}
	}
	pthread_mutex_lock(&tasks_mutex);
	task_structure_set_signal_locked(ctp, task);
	qnx_put_task_locked(task);

	task->attachment.uaddr_cache_id =
		uaddr_put(task->attachment.uaddr_cache_id);

	if (inclose) {
		pid_t	closed_pid = task->pid;
		struct task_struct	*owner = task->attachment.opened;

		qnx_task_count_debug(task, "==close(before)==");
		/* Decrease usage count for every close */
		if (!owner) {
			set_comm(task, ctp);
			printk(KERN_ERR "## %s[%d] "
					"close not opened thread "
					"%-.*s\n",
					__func__, pthread_self(),
					(int)sizeof(task->comm),
					task->comm);
		} else
		if (qnx_put_task_locked(owner)) {
			/*
			 * closed last opened
			 * owner may not be self task
			 */
			qnx_close_tasks_locked(owner, closed_pid);

			task->attachment.uaddr_cache_id =
				uaddr_put(task->attachment.uaddr_cache_id);
		}
	}
	pthread_mutex_unlock(&tasks_mutex);

	current = NULL;

	return 0;
}

static int set_task_struct()
{
	int err;

	/* Setup main thread as kernel control thread, user level access is not allowed */
	/* int this context, since resmgr_context is NULL, any access should produce    */
	/* SIGSEGV, which is equal to kernel panic in Linux.                            */
	memset(&main_task, 0, sizeof(main_task));
	main_task.pid = -1;
	main_task.spid.pid = -1;
	strncpy(main_task.comm, "kernel", sizeof(main_task.comm));
	main_task.real_cred = &main_task.cred_vault;
	main_task.cred = &main_task.cred_vault;
	main_task.cred_vault.uid.val = getuid();
	main_task.cred_vault.gid.val = getgid();
	main_task.cred_vault.euid.val = geteuid();
	main_task.cred_vault.egid.val = getegid();
	main_task.cred_vault.suid.val = geteuid();
	main_task.cred_vault.sgid.val = getegid();
	main_task.cred_vault.fsuid.val = geteuid();
	main_task.cred_vault.fsgid.val = getegid();
	err = qnx_create_task_sched(&main_task);
	if (err) {
		/* Error output is done inside qnx_create_task_sched() function */
		return err;
	}
	current = &main_task;
	return err;
}

static int globals_init()
{
	int err = EOK;
	struct timespec now;

	/* clear clients task structures */
	memset(tasks, 0, sizeof(tasks));

	/* MG_TODO: Make a piece of code below as a function, since it is used at 8 */
	/* different places right now in same form. */

	main_task.attachment.user_tid = pthread_self();
	main_task.attachment.copy_to_user_memcpy = 1;

	cpu_init();

	// make sure CLOCK_MONOTONIC is available
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
		err = errno;
		qnx_error("clock_gettime failed");
		return err;
	}

	err = qnx_pci_init();
	if (err) {
		qnx_error("qnx_pci_init() failed");
	}

	return err;
}

static void globals_fini()
{
	qnx_pci_fini();

	if (current != NULL) {
		qnx_destroy_task_sched(current);
		current = NULL;
	}
}

int gpiolib_debugfs_init();
int buses_init();
int devices_init();
int fbmem_init();
int i2c_init();
void backlight_class_exit();
int backlight_class_init();
int classes_init();
void radix_tree_init();
int dma_buf_init();
void dma_buf_deinit();

static int drm_intel_drv_preinit()
{
	int err = EOK;

	sched_init();
	init_timers();
	radix_tree_init();
	if (!init_wq_system()) {
		fprintf(stderr, "%s: Couldn't initialize workqueue system! errno %d\n", progname, errno);
		return errno;
	}

	devices_init();
	buses_init();
	gpiolib_debugfs_init();
	fbmem_init();
	i2c_init();
	qnx_mipi_dsi_bus_init();
	backlight_class_init();
	dma_buf_init();

	qnx_drm_core_init();
	qnx_drm_kms_helper_init();
	fbmem_init();

	return err;
}

static int drm_intel_drv_init()
{
	int err = EOK;

	DRM_DEBUG("Initializing Intel i915 DRM driver\n");
	err = qnx_i915_init();
	if (err == EOK) {
		DRM_DEBUG("Intel i915 DRM driver has been successfully initialized\n");
	} else {
		fprintf(stderr, "%s: Couldn't initialize Intel i915 driver, errno %d\n", progname, errno);
	}
	return err;
}

int drm_resmgr_start()
{
	int err = EOK;

	if ((err = set_task_struct())) {
		return err;
	}
	preempt_disable();	/* boost up startup */

	ThreadCtl(_NTO_TCTL_IO_PRIV, 0);

	INIT_LIST_HEAD(&minors_list.list);
	INIT_LIST_HEAD(&groups_list.list);

	/* This must be one of the first calls, since it allocates memory for global linux kernel object */
	classes_init();

	err = mlockall(MCL_CURRENT | MCL_FUTURE);
	if (err != EOK) {
	    fprintf(stderr, "Can't lock all pages!\n");
	}

	err = globals_init();
	if(err != EOK){
		globals_fini();
		return err;
	}

	/* Create a dispatch handle, which is used by debugfs too */
	dpp = dispatch_create();
	if (dpp == NULL) {
		fprintf(stderr, "Cannot allocate resmgr dispatch handle!\n");
		goto out;
	}

	err = drm_intel_drv_preinit();
	if(err != EOK){
		fprintf(stderr, "Preinit of DRM drivers failed!\n");
		goto out;
	}

	err = drm_intel_drv_init();
	if(err != EOK){
		fprintf(stderr, "Startup of i915 driver failed!\n");
		goto out;
	}

	memset(&rattr, 0, sizeof (rattr));

	iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &drm_connect_funcs,
					  _RESMGR_IO_NFUNCS, &drm_io_funcs);
	drm_io_funcs.read = io_read;
	drm_io_funcs.write = io_write;
	drm_io_funcs.devctl = io_devctl;
	drm_io_funcs.close_ocb = io_close;
	drm_io_funcs.unblock = io_unblock;
	drm_io_funcs.stat = io_stat;
	drm_io_funcs.mmap = io_mmap;
	drm_io_funcs.notify = io_notify;
	drm_connect_funcs.open = io_open;

	iofunc_attr_init(&drm_attrs.attr, _S_IFDIR | 0777, NULL, NULL);
	drm_attrs.attr.inode = ++last_inode;
	drm_attrs.attr.nbytes = 0;
	attr_priv = calloc(1, sizeof(drm_device_ext_data_t));
	if (attr_priv) {
		attr_priv->nodes = &minors_list;
		attr_priv->iofunc_attr = &drm_attrs.attr;
		struct sysfs_dirent *sd = calloc(1, sizeof(struct sysfs_dirent));
		if (sd) {
			sd->s_name = DRM_DIR;
			sd->s_mode = attr_priv->iofunc_attr->mode;
			sd->s_flags = SYSFS_DIR;
			attr_priv->sd = sd;
		}

	} else {
		fprintf(stderr, "Not enough memory for attributes allocation!\n");
	}
	drm_attrs.device_ext_data = (void *)attr_priv;

	DRM_DEBUG("Attaching pathname %s\n", DRM_DIR);

	resmgr_link_id = resmgr_attach(
			dpp,                 /* dispatch handle        */
			&rattr,              /* resource manager attrs */
			DRM_DIR,             /* device name            */
			_FTYPE_ANY,          /* open type              */
			_RESMGR_FLAG_DIR,    /* flags                  */
			&drm_connect_funcs,  /* connect routines       */
			&drm_io_funcs,       /* I/O routines           */
			&drm_attrs);         /* handle                 */
	if (resmgr_link_id == -1) {
		err = errno;
		fprintf(stderr, "%s:  couldn't attach pathname %s, errno %d\n", progname, DRM_DIR, errno);
		goto out;
	}

	thread_pool_attr_t    pool_attr;

	memset (&pool_attr, 0, sizeof(pool_attr));
	pool_attr.handle = dpp;
	pool_attr.block_func = dispatch_block;
	pool_attr.unblock_func = dispatch_unblock;
	pool_attr.handler_func = dispatch_handler;
	pool_attr.context_alloc = dispatch_context_alloc;
	pool_attr.context_free = dispatch_context_free;
	pool_attr.lo_water = 3;
	pool_attr.hi_water = 6;
	pool_attr.increment = 2;
	pool_attr.maximum = 10;
	pool_attr.tid_name = "resmgr_thread_pool";
	pool_attr.attr = calloc(1, sizeof(pthread_attr_t));
	if (pool_attr.attr) {
		err = pthread_attr_init(pool_attr.attr);
		if (err != 0) {
			free(pool_attr.attr);
			pool_attr.attr = NULL;
			goto out;
		}
		err = pthread_attr_setstacksize(pool_attr.attr, THREAD_STACK_SIZE);
		if (err != 0) {
			free(pool_attr.attr);
			pool_attr.attr = NULL;
			goto out;
		}
		err = pthread_attr_setguardsize(pool_attr.attr, THREAD_STACK_GUARD_SIZE);
		if (err != 0) {
			free(pool_attr.attr);
			pool_attr.attr = NULL;
			goto out;
		}
	}

	DRM_DEBUG("Configuring and launching DRM thread pool\n");
	preempt_enable();

	if ((tpp = thread_pool_create(&pool_attr, 0)) == NULL) {
		err = errno;
		fprintf(stderr, "%s: thread_pool_create failed: %s\n", progname, strerror(err));
		goto out;
	}

	errno = 0;
	if (thread_pool_start(tpp) != 0) {
		err = errno ? errno : ESRVRFAULT;
		goto out;
	}

out:
	if (err) {
		drm_resmgr_stop();
	}

	return err;
}

void drm_resmgr_stop()
{
	DRM_DEBUG("Exiting from DRM. Cleaning resources\n");

	/* Destroy the dispatch handle */
	if (dpp != NULL) {
		if (dispatch_destroy(dpp) == -1) {
			fprintf(stderr, "Dispatch wasn't destroyed, bad dispatch handle: %p.\n", dpp);
			/* Continue cleanup procedure even if we can't destroy dispatch handle */
		}
		dpp = NULL;
	}

	/* Destroy thread pool */
	if (tpp != NULL) {
		if (thread_pool_destroy(tpp) == -1) {
			fprintf(stderr, "Thread pool can't be destroyed, tpp handle is: %p.\n", tpp);
			/* Continue cleanup procedure even if we can't destroy thread pool */
		}
	}

	/* Disconnect all /dev/drm/ entries and close all connections. */
	if (resmgr_link_id != -1) {
		if (resmgr_detach(dpp, resmgr_link_id, _RESMGR_DETACH_ALL) == -1) {
			fprintf(stderr, "Resmgr wasn't detached, bad resmgr link id: %d.\n", resmgr_link_id);
			/* Continue cleanup procedure even if we can't detach resmgr link id */
		}
		resmgr_link_id = -1;
	}

	qnx_i915_exit();

	free(attr_priv);
	attr_priv = NULL;

	destroy_wq_system();
	fbmem_exit();
	qnx_drm_kms_helper_exit();
	qnx_drm_core_exit();

	dma_buf_deinit();
	qnx_mipi_dsi_bus_exit();

	globals_fini();

#if defined(CONFIG_DRM_DEBUG_MM)
	/* This function is empty in non-debug mode */
	mmap_trace_dump();
	/* This one is not */
	kmalloc_trace_dump();
#endif /* CONFIG_DRM_DEBUG_MM */
#if defined(CONFIG_TRACE_QNX_MUTEX)
	__mutex_debug_dump();
#endif /* CONFIG_TRACE_QNX_MUTEX */
}

#ifndef VARIANT_dll
int main(int argc, char **argv)
{
	int ret;
	int err = EOK;
	bool started_drm = false;
	sigset_t sigset;
	struct rlimit rl;
	int it;
	wordexp_t env_cmdargs;
	char* env_cmdline = NULL;
	char* we_cmdline;

	/* Parse process cmdline arguments passed through environment variable */
	env_cmdline = getenv("DRM_CMDLINE");
	if (env_cmdline != NULL) {
		if (strlen(env_cmdline) > (DRM_CMDLINE_MAX_SIZE - 1)) {
			fprintf(stderr, "DRM_CMDLINE length exceeds %d bytes! Giving up...\n",
				(int)(DRM_CMDLINE_MAX_SIZE - strlen("process ") - 1));
			return -1;
		}
		we_cmdline = calloc(1, DRM_CMDLINE_MAX_SIZE);
		if (we_cmdline == NULL) {
			fprintf(stderr, "Can't allocate memory for environment variable string!\n");
			return -1;
		}
		strncpy(we_cmdline, "process ", DRM_CMDLINE_MAX_SIZE);
		strncat(we_cmdline, env_cmdline, DRM_CMDLINE_MAX_SIZE);

		__libc_argc = argc;
		__libc_argv = argv;

		ret = qnx_wordexp(we_cmdline, &env_cmdargs, WRDE_SHOWERR | WRDE_UNDEF);
		switch (ret) {
			case 0:
				options_debug(env_cmdargs.we_wordc, env_cmdargs.we_wordv);
				if (options(env_cmdargs.we_wordc, env_cmdargs.we_wordv) == -1) {
					return 0;
				}
				qnx_wordfree(&env_cmdargs);
				break;
			case WRDE_BADCHAR:
				fprintf(stderr, "DRM_CMDLINE: Illegal occurrence of newline or one of |, &, ;, <, >, (, ), {, }.\n");
				break;
			case WRDE_BADVAL:
				fprintf(stderr, "DRM_CMDLINE: An undefined shell variable was referenced.\n");
				break;
			case WRDE_CMDSUB:
				fprintf(stderr, "DRM_CMDLINE: Command substitution requested.\n");
				break;
			case WRDE_NOSPACE:
				fprintf(stderr, "DRM_CMDLINE: Out of memory.\n");
				break;
			case WRDE_SYNTAX:
				fprintf(stderr, "DRM_CMDLINE: Shell syntax error or unbalanced parentheses or unmatched quotes.\n");
				break;
			case WRDE_NOSYS:
				fprintf(stderr, "DRM_CMDLINE: Parser function is not supported by system.\n");
				break;
			default:
				fprintf(stderr, "DRM_CMDLINE: Unknown error occured during parsing.\n");
				break;
		}
		free(we_cmdline);
	}

	/* Parse generic cmdline arguments */
	options_debug(argc, argv);
	if (options(argc, argv) == -1) {
		return 0;
	}

	/* Cleanup memory allocated for options, if any */
	for (it = 0; it < module_options_count; it++) {
		free(module_options[it].parameter_name);
		free(module_options[it].parameter_type);
		free(module_options[it].parameter_desc);
		module_options[it].parameter_addr = 0ULL;
		module_options[it].parameter_size = 0ULL;
	}

	rl.rlim_cur = 32767;
	rl.rlim_max = 32767;
#if __OFF_BITS__ == 32
	rl.rlim_cur_hi = 0;
	rl.rlim_max_hi = 0;
#endif

	ret = setrlimit(RLIMIT_NOFILE, &rl);
	if (ret) {
		qnx_error("could not increase maximum number of fds ");
	}

	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGQUIT);

	err = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (err) {
		fprintf(stderr, "pthread_sigmask: %s\n", strerror(err));
		goto out;
	}
	err = drm_resmgr_start();
	if (err) goto out;
	started_drm = true;

	pthread_setname_np(0, "drm-monitor");

	// Put the DRM process into the background.
	if (procmgr_daemon(EXIT_SUCCESS, PROCMGR_DAEMON_KEEPUMASK |
		PROCMGR_DAEMON_NODEVNULL | PROCMGR_DAEMON_NOCLOSE) < 0) {
		err = ENOSYS;  // XXX: does procmgr_daemon set errno? JI#656638
		fprintf(stderr, "procmgr_daemon failed\n");
		goto out;
	}

	for(;;) {
		int sig = SignalWaitinfo_r(&sigset, NULL);
		if (sig < 0 && sig != -EINTR) {
			err = -sig;
			break;
		} else if (sig == SIGTERM || sig == SIGQUIT) {
			// shut down
			break;
		}
	}

out:
	if (started_drm) {
		drm_resmgr_stop();
	}
	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
#endif

int dirent_size(char *fname)
{
	return (DIRENT_ALIGN(sizeof(struct dirent) + strlen(fname)));
}

struct dirent *dirent_fill(struct dirent *dp, int inode, int offset, char *fname)
{
	dp->d_ino = inode;
	dp->d_offset = offset;
	strcpy(dp->d_name, fname);
	dp->d_namelen = strlen(dp->d_name)+1;
	dp->d_reclen = DIRENT_ALIGN(sizeof(struct dirent) + dp->d_namelen);
	return ((struct dirent *)((char *)dp + dp->d_reclen));
}

static int my_read_dir(resmgr_context_t *ctp, io_read_t *msg, drm_ocb_t *ocb)
{
	int nbytes;
	int nleft;
	struct  dirent *dp;
	char    *reply_msg;
	struct drm_node_list *entry;

	DRM_DEBUG("Enter reading dir\n");

	// allocate a buffer for the reply
	reply_msg = calloc (1, msg->i.nbytes);
	if (reply_msg == NULL) {
		return (ENOMEM);
	}

	// assign output buffer
	dp = (struct dirent *) reply_msg;

	// we have "nleft" bytes left
	nleft = msg->i.nbytes;

	drm_device_ext_data_t *attr = (drm_device_ext_data_t *)ocb->hdr.attr->device_ext_data;

	if (attr) {
		DRM_DEBUG("Reading dir %s\n", attr->sd->s_name);
		list_for_each_entry(entry, &attr->nodes->list, head) {
			if ( ocb->hdr.offset >= attr->nodes->count ){
				DRM_DEBUG("offset = nodes count\n");
				break;
			}
			// see how big the result is
			nbytes = dirent_size ((char *)entry->name);

			// do we have room for it?
			if (nleft - nbytes >= 0) {

				// fill the dirent, and advance the dirent pointer
				dp = dirent_fill (dp, entry->device.attr.inode, ocb->hdr.offset, (char *)entry->name);

				// move the OCB offset
				ocb->hdr.offset++;

				// account for the bytes we just used up
				nleft -= nbytes;
			} else {

				// don't have any more room, stop
				break;
			}
		}
	} else {
		DRM_DEBUG("Device ext data pointer is null!\n");
	}

	// return info back to the client
	MsgReply (ctp->rcvid, (char *) dp - reply_msg, reply_msg,
	          (char *) dp - reply_msg);

	// release our buffer
	free (reply_msg);

	DRM_DEBUG("Exit reading dir\n");

	// tell resource manager library we already did the reply
	return (_RESMGR_NOREPLY);
}

static int my_read_file(resmgr_context_t *ctp, io_read_t *msg, drm_ocb_t *ocb)
{
	int     nbytes;
	int     nleft;
	const char *buf;
	DRM_DEBUG("Enter reading file\n");
	drm_device_ext_data_t *attr_priv = (drm_device_ext_data_t *)ocb->hdr.attr->device_ext_data;
	drm_ocb_ext_data_t *ocb_priv = (drm_ocb_ext_data_t *)ocb->ocb_ext_data;

	// we don't do any xtypes here...
	if ((msg -> i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
		return (ENOSYS);
	}

	if (!ocb->hdr.offset){
		unsigned int type = sysfs_type(attr_priv->sd);
		attr_priv->buf = calloc(1, PAGE_SIZE);

		claim_task_structure(ctp, 0);
		switch (type){
		case SYSFS_KOBJ_BIN_ATTR:
			DRM_DEBUG("Reading file %s\n", attr_priv->battr.attr.name);
			if (attr_priv->battr.read) {
				int count = attr_priv->battr.read(&ocb_priv->filp, &attr_priv->dev->kobj, &attr_priv->battr, attr_priv->buf, ocb->hdr.offset, PAGE_SIZE);
				ocb->hdr.attr->attr.nbytes = count;
			}
			else {
				attr_priv->buf = (char *)attr_priv->battr.attr.name;
			}
			break;
		case SYSFS_KOBJ_ATTR:
		default:
			DRM_DEBUG("Reading file %s\n", attr_priv->dattr.attr.name);
			if (attr_priv->dattr.show) {
				attr_priv->dattr.show(attr_priv->dev, &attr_priv->dattr, attr_priv->buf);
				ocb->hdr.attr->attr.nbytes = strlen(attr_priv->buf);
			}
			else {
				attr_priv->buf = (char *)attr_priv->dattr.attr.name;
			}
		}
		unclaim_task_structure(ctp, 0);
	}
	buf = attr_priv->buf;

	// figure out how many bytes are left
	nleft = ocb->hdr.attr->attr.nbytes - ocb->hdr.offset;

	// and how many we can return to the client
	nbytes = min (nleft, msg->i.nbytes);

	if (nbytes) {
		// return it to the client
		MsgReply (ctp->rcvid, nbytes, buf + ocb->hdr.offset, nbytes);

		// update flags and offset
		ocb->hdr.attr->attr.flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;
		ocb->hdr.offset += nbytes;
	} else {
		// nothing to return, indicate End Of File
		MsgReply (ctp->rcvid, EOK, NULL, 0);
	}
	DRM_DEBUG("Exit reading file\n");

	// already done the reply ourselves
	return (_RESMGR_NOREPLY);
}

static int io_read_event(resmgr_context_t* ctp, io_read_t* msg, drm_ocb_t* ocb, int nonblock)
{
	int total = msg->i.nbytes;
	ssize_t nbytes;
	int rc;

	drm_ocb_ext_data_t *ocb_priv = (drm_ocb_ext_data_t *)ocb->ocb_ext_data;
	if (nonblock) {
		ocb_priv->filp.f_flags |= O_NONBLOCK;
	} else {
		ocb_priv->filp.f_flags &= ~(O_NONBLOCK);
	}

	if (total <= 0) {
		return EINVAL;
	} else {
		if (total > PAGE_SIZE) {
			total = PAGE_SIZE;
		}
	}

	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
		return ENOSYS;
	}

	claim_task_structure(ctp, 0);
	current->attachment.copy_to_user_memcpy = 1;

	iofunc_attr_unlock(&ocb->hdr.attr->attr);

	if ((drm_poll(&ocb_priv->filp, NULL) & (POLLRDNORM | POLLIN)) == (POLLRDNORM | POLLIN)) {
		nbytes = drm_read(&ocb_priv->filp, ocb->filedata, total, NULL);
		if (nbytes > 0) {
			SETIOV(ctp->iov, ocb->filedata, nbytes);
			_IO_SET_READ_NBYTES(ctp, nbytes);
			rc = _RESMGR_NPARTS(1);
		} else {
			rc = -nbytes;
		}
	} else {
		rc = 0;
	}

	iofunc_attr_lock(&ocb->hdr.attr->attr);

	/* Do not keep O_NONBLOCK flag always set, set it only when it is really needed */
	ocb_priv->filp.f_flags &= ~(O_NONBLOCK);

	unclaim_task_structure(ctp, 0);

	return rc;
}
/*
 *  io_read
 *
 *  At this point, the client has called their library "read"
 *  function, and expects zero or more bytes.
 */

int io_read(resmgr_context_t *ctp, io_read_t *msg, drm_ocb_t * ocb)
{
	int status;
	int nonblock;

	DRM_DEBUG("in io_read(pid=%d nbytes=%d)\n", ctp->info.pid, msg->i.nbytes);
	if ((status = iofunc_read_verify(ctp, msg, &ocb->hdr, &nonblock)) != EOK) {
		fprintf(stderr, "%s: no read permissions, status = %d\n", progname, status);
		return (status);
	}

	// decide if we should perform the "file" or "dir" read
	if (S_ISDIR (ocb->hdr.attr->attr.mode)) {
		return (my_read_dir(ctp, msg, ocb));
	} else if (S_ISREG (ocb->hdr.attr->attr.mode)) {
		return (my_read_file(ctp, msg, ocb));
	} else if (S_ISCHR (ocb->hdr.attr->attr.mode)) {
		return io_read_event(ctp, msg, ocb, nonblock);
	} else {
		return (EBADF);
	}
}

static int io_unblock(resmgr_context_t *ctp, io_pulse_t *msg, drm_ocb_t *ocb)
{
	DRM_DEBUG("Enter to io_unblock: ocb=%p filp=%p\n", ocb, ocb->ocb_ext_data);

	if (iofunc_unblock_default(ctp, msg, &ocb->hdr) != _RESMGR_DEFAULT) {
		return _RESMGR_NOREPLY;
	}

	/* claim task structure, if we have UNBLOCK requested */
	/* it will be set in the task structure */
	claim_task_structure(ctp, 0);

	/* kick scheduler waiters if any */
	pthread_mutex_lock(&current->sched_mutex);
	set_bit(QNX_SCHED_SIGNALLED, &current->sched_flags);
	pthread_cond_broadcast(&current->sched_cond);
	pthread_mutex_unlock(&current->sched_mutex);

	unclaim_task_structure(ctp, 0);

	return _RESMGR_DEFAULT;
}

static int io_stat(resmgr_context_t *ctp, io_stat_t *msg, drm_ocb_t *ocb)
{
	return iofunc_stat_default(ctp, msg, &ocb->hdr);
}

static int my_write_dir(resmgr_context_t *ctp, io_write_t *msg, drm_ocb_t *ocb)
{
	_IO_SET_WRITE_NBYTES (ctp, msg->i.nbytes);
	return EOK;
}

static int my_write_file(resmgr_context_t *ctp, io_write_t *msg, drm_ocb_t *ocb)
{
	int         count = 0;
	int         nbytes;
	char        *data;
	int         off;
	int         start_data_offset;
	int         xtype;
	struct _xtype_offset *xoffset;

	DRM_DEBUG("Enter writing file\n");
	drm_device_ext_data_t *attr_priv = (drm_device_ext_data_t *)ocb->hdr.attr->device_ext_data;
	drm_ocb_ext_data_t *ocb_priv = (drm_ocb_ext_data_t *)ocb->ocb_ext_data;

	// we don't do any xtypes here...
	xtype = msg -> i.xtype & _IO_XTYPE_MASK;
	if (xtype == _IO_XTYPE_OFFSET) {
		xoffset = (struct _xtype_offset *) (&msg -> i + 1);
		start_data_offset = sizeof (msg -> i) + sizeof (*xoffset);
		off = xoffset -> offset;
	} else if (xtype == _IO_XTYPE_NONE) {
		off = ocb->hdr.offset;
		start_data_offset = sizeof (msg->i);
	} else {   // unknown, fail it
		return (ENOSYS);
	}

	nbytes = msg->i.nbytes;

	/* go to the client's send buffer and get the data */
	data = calloc(1, nbytes);      /* one extra for the NULL */
	if (data == NULL)
		return (errno);

	if (resmgr_msgread (ctp, data, nbytes, start_data_offset) == -1) {
		free (data);
		return (errno);
	}

	unsigned int type = sysfs_type(attr_priv->sd);

	claim_task_structure(ctp, 0);
	switch (type){
	case SYSFS_KOBJ_BIN_ATTR:
		DRM_DEBUG("Writing file %s\n", attr_priv->battr.attr.name);
		if (attr_priv->battr.write) {
			count = attr_priv->battr.write(&ocb_priv->filp, &attr_priv->dev->kobj, &attr_priv->battr, data, off, nbytes);
		}
		break;
	case SYSFS_KOBJ_ATTR:
	default:
		DRM_DEBUG("Writing file %s\n", attr_priv->dattr.attr.name);
		if (attr_priv->dattr.store) {
			count = attr_priv->dattr.store(attr_priv->dev, &attr_priv->dattr, data, nbytes);
		}
	}
	unclaim_task_structure(ctp, 0);

	free(data);

	_IO_SET_WRITE_NBYTES (ctp, count);

	if (count) {
		ocb->hdr.attr->attr.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_DIRTY_TIME;

		if (xtype == _IO_XTYPE_NONE) {
			ocb->hdr.offset += nbytes;
		}
	}

	DRM_DEBUG("Exit writing file\n");

	return (count < 0) ? -(count) : (EOK);
}

/*
 *  io_write
 *
 *  At this point, the client has called their library "write"
 *  function.  We act like /dev/null.
 */

int io_write(resmgr_context_t *ctp, io_write_t *msg,
			 drm_ocb_t *iofunc_ocb)
{
	int status;
	drm_ocb_t *ocb = (drm_ocb_t *)iofunc_ocb;

	DRM_DEBUG("in io_write(pid=%d nbytes=%d)\n", ctp->info.pid, msg->i.nbytes);
	if ((status = iofunc_write_verify(ctp, msg, &ocb->hdr, NULL)) != EOK) {
		fprintf(stderr, "%s: no write permissions, status = %d\n", progname, status);
		return (status);
	}

	// decide if we should perform the "file" or "dir" read
	if (S_ISDIR (ocb->hdr.attr->attr.mode)) {
		return (my_write_dir(ctp, msg, ocb));
	} else if (S_ISREG (ocb->hdr.attr->attr.mode)) {
		return (my_write_file(ctp, msg, ocb));
	} else {
		return (EBADF);
	}
}

int io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, drm_ocb_t* ocb)
{
	int status;
	void *rx_data;
	int nbytes = 0;
	drm_ocb_ext_data_t *ocb_priv = (drm_ocb_ext_data_t *)ocb->ocb_ext_data;

	DRM_DEBUG("in io_devctl() pid=%d nbytes=%d dcmd=0x%x ocb=%p, filp=%p\n", ctp->info.pid, msg->i.nbytes, msg->i.dcmd, ocb, ocb->ocb_ext_data);

	if ((status = iofunc_devctl_default(ctp, msg, &ocb->hdr)) !=  _RESMGR_DEFAULT)
	{
		DRM_DEBUG("pid=%d dcmd=0x%x handled by default\n", ctp->info.pid, msg->i.dcmd);
		return (status);
	}

	nbytes = msg->i.nbytes;

	iofunc_attr_unlock(&ocb->hdr.attr->attr);

	if ( (( msg->i.dcmd < 0x000 ) || ( msg->i.dcmd > 0xfff )) && (ocb_priv->filp.private_data != NULL) )
	{
		rx_data = _DEVCTL_DATA(msg->i);
		claim_task_structure(ctp, 0);
		status = drm_ioctl(&ocb_priv->filp, msg->i.dcmd, (unsigned long)rx_data);
		unclaim_task_structure(ctp, 0);
		/* From drm_ioctl() docs, it returns zero on success, negative error code on failure. */
		/* Positive value must be accepted as a special return code on success.*/
		if (status < 0) {
			/* Warn about unsuccessful call */
			DRM_DEBUG("pid=%d dcmd=0x%x handled by DRM. retcode=%d\n", ctp->info.pid, msg->i.dcmd, status);
		}
	}
	else
	{
		DRM_DEBUG("pid=%d dcmd=0x%x unhandled!\n", ctp->info.pid, msg->i.dcmd);
	}

	iofunc_attr_lock(&ocb->hdr.attr->attr);

	msg->o.zero = 0;
	msg->o.zero2 = 0;
	msg->o.nbytes = nbytes;
	msg->o.ret_val = status;
	_RESMGR_STATUS(ctp, EOK);

	return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + nbytes));
}

static int io_open(resmgr_context_t *ctp, io_open_t *msg, drm_device_t *pattr, void *extra)
{
	drm_ocb_t  *ocb;
	iofunc_attr_t *attr = NULL;
	const char *pname = NULL;
	int retcode = 0;
	struct drm_node_list    *entry;
	struct drm_node_list    *attr_entry;
	struct drm_node_list    *attr_sub_entry;
	drm_device_ext_data_t   *attr_priv = NULL;
	drm_ocb_ext_data_t      *ocb_priv = NULL;

	DRM_DEBUG("Enter io_open\n");

	if (msg->connect.path_len > 1)
		DRM_DEBUG("pid=%d, path=%s/%s\n", ctp->info.pid, DRM_DIR, msg->connect.path);
	else
		DRM_DEBUG("pid=%d, path=%s\n", ctp->info.pid, DRM_DIR);

	if (msg->connect.path[0] == 0) {   // the directory (/dev/drm)
		attr = &pattr->attr;
		attr_priv = (drm_device_ext_data_t *)pattr->device_ext_data;
		pname = DRM_DIR;
	} else if (msg->connect.path[0] > 0) {   // the file (/dev/drm/...)
		char name[256];
		char *where = strchr( msg->connect.path, '/' );
		if (where == NULL) {
			list_for_each_entry(entry, &minors_list.list, head){
				if(!strcmp(entry->name, msg->connect.path)){
					attr = &entry->device.attr;
					attr_priv = (drm_device_ext_data_t*)entry->device.device_ext_data;
					pname = entry->name;
					break;
				}
			}
		} else {
			list_for_each_entry(entry, &minors_list.list, head){
				if (!list_empty(&entry->attrs_list.list)) {
					list_for_each_entry(attr_entry, &entry->attrs_list.list, head){
						sprintf(name, "%s/%s", entry->name, attr_entry->name);
						if (!strcmp(name, msg->connect.path)){
							attr = &attr_entry->device.attr;
							attr_priv = (drm_device_ext_data_t *)attr_entry->device.device_ext_data;
							pname = attr_entry->name;
							break;
						}
						if (attr_entry->count && !list_empty(&attr_entry->attrs_list.list)) {
							list_for_each_entry(attr_sub_entry, &attr_entry->attrs_list.list, head){
								sprintf(name, "%s/%s/%s", entry->name, attr_entry->name, attr_sub_entry->name);
								if (!strcmp(name, msg->connect.path)){
									attr = &attr_sub_entry->device.attr;
									attr_priv = (drm_device_ext_data_t *)attr_sub_entry->device.device_ext_data;
									pname = attr_sub_entry->name;
									break;
								}
							}
							if (attr) {
								break;
							}
						}
					}
					if (attr) {
						break;
					}
				}
			}
		}
	} else {
		return (ENOENT);
	}

	if (attr_priv) {
		attr_priv->pid = ctp->info.pid;
		unsigned int type = sysfs_type(attr_priv->sd);
		switch (type) {
			case SYSFS_KOBJ_BIN_ATTR:
				attr_priv->battr.attr.name = pname;
				break;
			case SYSFS_KOBJ_ATTR:
			default:
				attr_priv->dattr.attr.name = pname;
				break;
		}
		DRM_DEBUG("rdev = 0x%08X\n", attr_priv->inode.i_rdev);

	} else {
		qnx_error("Node %s has not been found!\n", msg->connect.path);
		return ENOENT;
	}

	iofunc_attr_lock(attr);

	if ((retcode = iofunc_open(ctp, msg, attr, 0, 0)) != EOK) {
		qnx_error("no open permissions, status = %d\n", retcode);
		iofunc_attr_unlock(attr);
		return (retcode);
	}

	if ((ocb = calloc(1, sizeof(*ocb))) == NULL) {
		qnx_error("Not enough memory for OCB allocation!\n");
		iofunc_attr_lock(attr);
		return ENOMEM;
	}
	if ((ocb->filedata = calloc(1, PAGE_SIZE)) == NULL) {
		qnx_error("Not enough memory for internal buffer allocation inside an OCB!\n");
		free(ocb);
		iofunc_attr_unlock(attr);
		return ENOMEM;
	}

	ocb_priv = calloc(1, sizeof(drm_ocb_ext_data_t));
	ocb->ocb_ext_data = ocb_priv;
	if (ocb->ocb_ext_data == NULL) {
		qnx_error("Not enough memory for internal buffer allocation inside an OCB!\n");
		free(ocb->filedata);
		free(ocb);
		iofunc_attr_unlock(attr);
		return ENOMEM;
	}

	ocb_priv->filp.drm_attr = (void *)attr_priv;

	if ((retcode = iofunc_ocb_attach(ctp, msg, &ocb->hdr, attr, 0)) != EOK) {
		DRM_DEBUG("iofunc_ocb_attach(ctp=%p, msg=%p, ocb=%p, attr=%p, io_funcs=0) failed, errno=%d!\n", ctp, msg, &ocb->hdr, attr, retcode);
		free(ocb->ocb_ext_data);
		free(ocb->filedata);
		free(ocb);
		iofunc_attr_unlock(attr);
		return (retcode);
	}

	if (attr_priv->inode.i_rdev != 0) {
		claim_task_structure(ctp, 1);
		retcode = drm_open(&attr_priv->inode, &ocb_priv->filp);
		unclaim_task_structure(ctp, 0);
		if (retcode != EOK) {
			DRM_ERROR("drm_open call failed, retcode=%d\n", retcode);
			free(ocb->ocb_ext_data);
			free(ocb->filedata);
			free(ocb);
			iofunc_attr_unlock(attr);
			return -retcode;
		}
		ocb_priv->filp_inited = 1;
		atomic_long_inc(&ocb_priv->filp.f_count);
	}

	/* Initialize notify queues */
	if (ocb_priv->filp.private_data) {
		IOFUNC_NOTIFY_INIT(((struct drm_file*)ocb_priv->filp.private_data)->notify);
	}

	iofunc_attr_unlock(attr);
	DRM_DEBUG("Exit io_open\n");
	return (EOK);
}

static int io_close(resmgr_context_t *ctp, void* reserved, drm_ocb_t *ocb)
{
	int retcode = EOK;

	DRM_DEBUG("Enter to io_close: ocb=%p filp=%p\n", ocb, ocb->ocb_ext_data);
	drm_ocb_ext_data_t *ocb_priv = (drm_ocb_ext_data_t *)ocb->ocb_ext_data;
	drm_device_ext_data_t *attr_priv = (drm_device_ext_data_t *)ocb_priv->filp.drm_attr;

	/* Remove notify queues at exit and unblock all waiting clients */
	if (ocb_priv->filp.private_data) {
		iofunc_notify_trigger_strict(ctp, ((struct drm_file*)ocb_priv->filp.private_data)->notify, INT_MAX, IOFUNC_NOTIFY_INPUT);
		iofunc_notify_trigger_strict(ctp, ((struct drm_file*)ocb_priv->filp.private_data)->notify, INT_MAX, IOFUNC_NOTIFY_OUTPUT);
		iofunc_notify_trigger_strict(ctp, ((struct drm_file*)ocb_priv->filp.private_data)->notify, INT_MAX, IOFUNC_NOTIFY_OBAND);
		iofunc_notify_remove(ctp, ((struct drm_file*)ocb_priv->filp.private_data)->notify);
	}

	if (attr_priv->inode.i_rdev != 0) {
		claim_task_structure(ctp, 0);
		if (ocb_priv->filp_inited) {
			retcode = drm_release(&attr_priv->inode, &ocb_priv->filp);
			if (retcode != EOK){
				DRM_ERROR("drm_release call failed, retcode=%d\n", retcode);
			}

		}

		/* kick scheduler waiters if any */
		pthread_mutex_lock(&current->sched_mutex);
		set_bit(QNX_SCHED_SIGNALLED, &current->sched_flags);
		pthread_cond_broadcast(&current->sched_cond);
		pthread_mutex_unlock(&current->sched_mutex);

		unclaim_task_structure(ctp, 1);
	}

	free(ocb->ocb_ext_data);
	free(ocb->filedata);

	retcode = iofunc_close_ocb_default(ctp, NULL, (iofunc_ocb_t*)ocb);
	if (retcode != EOK){
		DRM_ERROR("iofunc_close_ocb_default call failed, retcode=%d\n", retcode);
	}

	DRM_DEBUG("Exit from io_close, retcode=%d\n", retcode);
	return retcode;
}

static int io_mmap(resmgr_context_t *ctp, io_mmap_t *msg, drm_ocb_t* ocb)
{
	qnx_error("mmap() is called for DRM process directly, aborting execution!");
	abort();

	msg->o.allowed_prot = 0;
	msg->o.coid = -1;
	msg->o.fd = -1;
	msg->o.offset = 0;

	return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o)));
}

static int io_notify(resmgr_context_t* ctp, io_notify_t* msg, drm_ocb_t* ocb)
{
	drm_ocb_ext_data_t *ocb_priv = (drm_ocb_ext_data_t *)ocb->ocb_ext_data;
	int trigger = 0;
	unsigned int mask = 0;

	if (ocb_priv->filp.private_data) {

		claim_task_structure(ctp, 0);

		mask = drm_poll(&ocb_priv->filp, NULL);
		if (mask & POLLIN) {
			trigger |= _NOTIFY_COND_INPUT;
		}

		unclaim_task_structure(ctp, 0);

		return iofunc_notify(ctp, msg, ((struct drm_file*)ocb_priv->filp.private_data)->notify, trigger, NULL, NULL);
	}

	return EINVAL;
}

#define ELFREAD_ERROR (void*)(uintptr_t)(-1)

static void* retrieve_elf_section(char* filename, char* section, size_t* size)
{
    int fd;
    size_t shstrndx;
    void* section_data = NULL;
    char* name;

    Elf* e;
    Elf_Scn* scn = NULL;
#if defined(CONFIG_X86_64)
    Elf64_Shdr* shdr;
#else
    Elf32_Shdr* shdr;
#endif /* __CONFIG_X86_64 */

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "%s: ELF library iinitialization failed: %s\n", progname, elf_errmsg(-1));
        return ELFREAD_ERROR;
    }

    DRM_DEBUG("Retrieving ELF sections from %s\n", filename);

    if ((fd = open(filename, O_RDONLY, 0)) < 0) {
        fprintf(stderr, "%s: open \"%s\" failed\n", progname, filename);
        return ELFREAD_ERROR;
    }

    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
        fprintf(stderr, "%s: elf_begin() failed: %s\n", progname, elf_errmsg(-1));
        return ELFREAD_ERROR;
    }

    // Retrieve the section index of the ELF section containing the string table of section names
    if (elf_getshdrstrndx(e, &shstrndx) != 0) {
        fprintf(stderr, "%s: elf_getshdrstrndx() failed: %s\n", progname, elf_errmsg(-1));
        return ELFREAD_ERROR;
    }

    // Loop over all sections in the ELF object
    while((scn = elf_nextscn(e, scn)) != NULL) {
        // Given a Elf Scn pointer, retrieve the associated section header
#if defined(CONFIG_X86_64)
        if ((shdr = elf64_getshdr(scn)) == NULL) {
#else
        if ((shdr = elf32_getshdr(scn)) == NULL) {
#endif /* __CONFIG_X86_64 */
            fprintf(stderr, "%s: getshdr() failed: %s\n", progname, elf_errmsg(-1));
            return ELFREAD_ERROR;
        }

        // Retrieve the name of the section name
        if ((name = elf_strptr(e, shstrndx, shdr->sh_name)) == NULL) {
            fprintf(stderr, "%s: elf_strptr() failed: %s\n", progname, elf_errmsg(-1));
            return ELFREAD_ERROR;
        }

        // If the section is the one we want... (in my case, it is one of the main file sections)
        if (!strcmp(name, section)) {
            // We can use the section adress as a pointer, since it corresponds to the actual
            // adress where the section is placed in the virtual memory
            section_data = (void*)shdr->sh_addr;
            *size = shdr->sh_size;

            // TODO: If section_data is NULL we should check for shdr->sh_offset and read it from
            // file directly.

            // End the loop (if we only need this section)
            break;
        }
    }

    elf_end(e);
    close(fd);

    return section_data;
}

static int search_parm(char* parm)
{
    int it=0;

    for (it = 0; it < DRM_SERVER_PARMS_COUNT; it++) {
        if (module_options[it].parameter_name != NULL) {
            if (strcmp(module_options[it].parameter_name, parm) == 0) {
                return it;
            }
        }
    }

    return it;
}

static int first_segments_scan = 1;
static char* filename = NULL;
static uint64_t filename_loadaddr = 0;

static int seg_scan_callback(const struct dl_phdr_info *info, size_t size, void *data)
{
    int j;

    if (first_segments_scan) {
        first_segments_scan = 0;
        filename = strdup(info->dlpi_name);
        filename_loadaddr = info->dlpi_addr;
    }

    return -1;
}

static void* parse_modinfo(int argc, char **argv)
{
    void* modinfo_org;
    void* modinfo;
    size_t mod_size = 0;
    void* tag_start;
    void* tag_end;
    char* tempname;
    int it;

    dl_iterate_phdr(seg_scan_callback, NULL);

    modinfo = retrieve_elf_section(filename, ".modinfo", &mod_size);
    modinfo += filename_loadaddr;
    free(filename);

    if ((modinfo == ELFREAD_ERROR) || (modinfo == NULL)) {
        return NULL;
    }
    modinfo_org = modinfo;

    DRM_DEBUG(".modinfo virtual address is %p\n", modinfo);

    do {
        if (memcmp(modinfo, "parm=", 0x05) == 0) {
            tag_start = tag_end = modinfo + 0x05;
            do {
                if (*((char*)tag_end) == ':') {
                    tempname = calloc(tag_end - tag_start + 1, 1);
                    memcpy(tempname, tag_start, tag_end - tag_start);

                    /* Search for existing parm name to update it's fields */
                    it = search_parm(tempname);
                    if (it == DRM_SERVER_PARMS_COUNT) {
                        /* Initialize all fields */
                        module_options[module_options_count].parameter_name = tempname;
                        module_options[module_options_count].parameter_desc = NULL;
                        module_options[module_options_count].parameter_type = NULL;
                        module_options[module_options_count].parameter_addr = 0;
                        module_options[module_options_count].parameter_size = 0;
                        it = module_options_count;
                        module_options_count++;
                    } else {
                        free(tempname);
                    }

                    module_options[it].parameter_desc = calloc(strlen(tag_end + 1) + 1, 1);
                    memcpy(module_options[it].parameter_desc, tag_end + 1, strlen(tag_end + 1));
                    tag_end += strlen(tag_end + 1);
                    modinfo += strlen(modinfo);
                    break;
                }
                if (*((char*)tag_end) == 0x00) {
                    break;
                }
                tag_end++;
            } while(1);
        }

        if (memcmp(modinfo, "parmtype=", 0x09) == 0) {
            tag_start = tag_end = modinfo + 0x09;
            do {
                if (*(char*)tag_end == ':') {
                    tempname = calloc(tag_end - tag_start + 1, 1);
                    memcpy(tempname, tag_start, tag_end - tag_start);


                    /* Search for existing parm name to update it's fields */
                    it = search_parm(tempname);
                    if (it == DRM_SERVER_PARMS_COUNT) {
                        /* Initialize all fields */
                        module_options[module_options_count].parameter_name = tempname;
                        module_options[module_options_count].parameter_desc = NULL;
                        module_options[module_options_count].parameter_type = NULL;
                        module_options[module_options_count].parameter_addr = 0;
                        module_options[module_options_count].parameter_size = 0;
                        it = module_options_count;
                        module_options_count++;
                    } else {
                        free(tempname);
                    }

                    module_options[it].parameter_type = calloc(strlen(tag_end + 1) + 1, 1);
                    memcpy(module_options[it].parameter_type, tag_end + 1, strlen(tag_end + 1));
                    tag_end += strlen(tag_end + 1);
                    modinfo += strlen(modinfo);
                    break;
                }
                if (*((char*)tag_end) == 0x00) {
                    break;
                }
                tag_end++;
            } while(1);
        }

        if (memcmp(modinfo, "parmaddr=", 0x09) == 0) {
            tag_start = tag_end = modinfo + 0x09;
            do {
                if (*(char*)tag_end == ':') {
                    tempname = calloc(tag_end - tag_start + 1, 1);
                    memcpy(tempname, tag_start, tag_end - tag_start);

                    /* Search for existing parm name to update it's fields */
                    it = search_parm(tempname);
                    if (it == DRM_SERVER_PARMS_COUNT) {
                        /* Initialize all fields */
                        module_options[module_options_count].parameter_name = tempname;
                        module_options[module_options_count].parameter_desc = NULL;
                        module_options[module_options_count].parameter_type = NULL;
                        module_options[module_options_count].parameter_addr = 0;
                        module_options[module_options_count].parameter_size = 0;
                        it = module_options_count;
                        module_options_count++;
                    } else {
                        free(tempname);
                    }

                    module_options[it].parameter_addr = *((uintptr_t*)(modinfo + 64)); // offsetof( _infox_qnx, parmaddr );
                    module_options[it].parameter_size = *((uintptr_t*)(modinfo + sizeof(uintptr_t) + 64)); // offsetof( _infox_qnx, parmsize );
                    tag_end += 64 + 2 * sizeof(uintptr_t) - 1;
                    modinfo += 64 + 2 * sizeof(uintptr_t) - 1;
                    break;
                }
                if (*((char*)tag_end) == 0x00) {
                    break;
                }
                tag_end++;
            } while(1);
        }

        modinfo++;
        if ((modinfo - modinfo_org) >= mod_size) {
            break;
        }
    } while(1);

    /* Post-processing of options, do sanity checks */
    for (it = 0; it < module_options_count; it++) {
        if (module_options[it].parameter_name != NULL) {
            if ((module_options[it].parameter_type == NULL) || (module_options[it].parameter_addr == 0ULL)) {
                /* Can't handle option without a type and address */
                free(module_options[it].parameter_name);
                free(module_options[it].parameter_type);
                free(module_options[it].parameter_desc);
                module_options[it].parameter_name = NULL;
                module_options[it].parameter_type = NULL;
                module_options[it].parameter_desc = NULL;
                module_options[it].parameter_addr = 0ULL;
            }
            if (module_options[it].parameter_desc == NULL) {
               module_options[it].parameter_desc = strdup("No description available for this option!");
            }
       }
    }

    return modinfo;
}

int video_setup(char *options);

void options_debug(int argc, char **argv)
{
	int opt;

	optind = 1;
	while ((opt = getopt(argc, argv, "ed:pstlao:vf:w:")) != -1) {
		switch (opt) {
		case 'e':
			debugfs_enable = 1;
			break;
		case 'd':
			drm_debug &= ~0x000000FF;
			drm_debug |= strtoul(optarg, NULL, 0);
			break;
		case 'p':
			drm_debug |= DRM_UT_QNX_PRINTF;
			break;
		case 's':
			drm_debug |= DRM_UT_QNX_SLOGF;
			break;
		case 't':
			drm_debug |= DRM_UT_QNX_TRACELOG;
			break;
		default :
			break;
		}
	}

	/* If debug output is enabled, do it via slogf by default. */
	if (drm_debug & 0x000000FF) {
		if ((drm_debug & (DRM_UT_QNX_PRINTF | DRM_UT_QNX_SLOGF | DRM_UT_QNX_TRACELOG)) == 0) {
			drm_debug |= DRM_UT_QNX_SLOGF;
		}
	}

	/* if core messages are enabled, enable QNX info messages too */
	if ((drm_debug & DRM_UT_CORE) == DRM_UT_CORE) {
		drm_debug |= DRM_UT_QNX_INFO;
	}
}

int options(int argc, char **argv)
{
	int opt;
	int it, jt;
	char* ptr;

	optind = 1;
	while ((opt = getopt(argc, argv, "ed:pstlao:vf:w:")) != -1) {
		switch (opt) {
		case 'l':
			if (!module_options_count) {
				parse_modinfo(argc, argv);
			}
			fprintf(stdout, "Linux kernel modules options:\n");
			for (it = 0; it < module_options_count; it++) {
				int length;
				if (module_options[it].parameter_name != NULL) {
					length = strlen(module_options[it].parameter_name);
					fprintf(stdout, "%s", module_options[it].parameter_name);
					for (jt = 25 - length; jt >= 0; jt--) {
						fprintf(stdout, " ");
					}
				}
				if (module_options[it].parameter_type != NULL) {
					length = strlen(module_options[it].parameter_type);
					fprintf(stdout, "%s", module_options[it].parameter_type);
					for (jt = 8 - length; jt >= 0; jt--) {
						fprintf(stdout, " ");
					}
				}
				if (module_options[it].parameter_desc != NULL) {
					length = strlen(module_options[it].parameter_desc);
					if (length>41) {
						for (jt = 0; jt < 41; jt++) {
							fprintf(stdout, "%c", module_options[it].parameter_desc[jt]);
						}
						fprintf(stdout, "...");
					} else {
						fprintf(stdout, "%s", module_options[it].parameter_desc);
					}
					}
				fprintf(stdout, "\n"); fflush(stdout);
			}
			return -1;
		case 'a':
			fprintf(stdout, "Linux kernel modules options:\n");
			if (!module_options_count) {
				parse_modinfo(argc, argv);
			}
			for (it = 0; it < module_options_count; it++) {
				int length;
				if (module_options[it].parameter_name != NULL) {
					length = strlen(module_options[it].parameter_name);
					fprintf(stdout, "%s", module_options[it].parameter_name);
					for (jt = 25 - length; jt >= 0; jt--) {
						fprintf(stdout, " ");
					}
				}
				if (module_options[it].parameter_type != NULL) {
					length = strlen(module_options[it].parameter_type);
					fprintf(stdout, "%s", module_options[it].parameter_type);
					for (jt = 8 - length; jt >= 0; jt--) {
						fprintf(stdout, " ");
					}
				}
				if (module_options[it].parameter_desc != NULL) {
					fprintf(stdout, "%s", module_options[it].parameter_desc);
				}
				fprintf(stdout, "\n\n"); fflush(stdout);
			}
			return -1;
		case 'o':
			if (!module_options_count) {
				parse_modinfo(argc, argv);
			}
			ptr = strchr(optarg, '=');
			if (ptr == NULL) {
				fprintf(stderr, "Option -o should be in format key=value, for example, debug=1\n");
				return -1;
			} else {
				char* tempname;

				tempname = calloc(ptr - optarg + 1, 1);
				memcpy(tempname, optarg, ptr - optarg);
				it = search_parm(tempname);
				if (it == DRM_SERVER_PARMS_COUNT) {
					fprintf(stderr, "Can't find option %s in a options list (check with -l or -a)\n", tempname);
					free(tempname);
					return -1;
				}
				free(tempname);
			}
			ptr++;
			if (strcmp(module_options[it].parameter_type, "int") == 0) {
				char* strptr = NULL;
				long value;

				value = strtol(ptr, &strptr, 0);
				if ((strptr != NULL) && (*strptr!=0x00)) {
					fprintf(stderr, "Can't parse int argument of %s!\n", optarg);
					return -1;
				}
				*((int*)(module_options[it].parameter_addr)) = (int)value;
				DRM_DEBUG("Module parameter %s is set to %d (0x%x) type: %s\n", module_options[it].parameter_name,
					(int)value, (int)value, module_options[it].parameter_type);
			} else if (strcmp(module_options[it].parameter_type, "uint") == 0) {
				char* strptr = NULL;
				unsigned long value;

				value = strtoul(ptr, &strptr, 0);
				if ((strptr != NULL) && (*strptr!=0x00)) {
					fprintf(stderr, "Can't parse uint argument of %s!\n", optarg);
					return -1;
				}
				*((uint*)(module_options[it].parameter_addr)) = (uint)value;
				DRM_DEBUG("Module parameter %s is set to %u (0x%x) type: %s\n", module_options[it].parameter_name,
					(uint)value, (uint)value, module_options[it].parameter_type);
			} else if (strcmp(module_options[it].parameter_type, "ullong") == 0) {
				char* strptr = NULL;
				unsigned long long value;

				value = strtoull(ptr, &strptr, 0);
				if ((strptr != NULL) && (*strptr!=0x00)) {
					fprintf(stderr, "Can't parse ullong argument of %s!\n", optarg);
					return -1;
				}
				*((unsigned long long*)(module_options[it].parameter_addr)) = (unsigned long long)value;
				DRM_DEBUG("Module parameter %s is set to %llu (0x%llx) type: %s\n", module_options[it].parameter_name,
					(unsigned long long)value, (unsigned long long)value, module_options[it].parameter_type);
			} else if (strcmp(module_options[it].parameter_type, "bool") == 0) {
				if (sscanf(ptr, "%d", &jt) != 1) {
					if (strcasecmp(ptr, "true") == 0) {
						jt = 1;
					} else if (strcasecmp(ptr, "false") == 0) {
						jt = 0;
					} else {
						fprintf(stderr, "Can't parse bool argument of %s!\n", optarg);
						return -1;
					}
				}
				*((bool*)(module_options[it].parameter_addr)) = jt;
				DRM_DEBUG("Module parameter %s is set to %d type: %s\n", module_options[it].parameter_name, jt, module_options[it].parameter_type);
			} else if (strcmp(module_options[it].parameter_type, "string") == 0) {
				strncpy((char*)module_options[it].parameter_addr, ptr, module_options[it].parameter_size);
				DRM_DEBUG("Module parameter %s is set to %s\n", module_options[it].parameter_name, (char*)module_options[it].parameter_addr);
			} else {
				fprintf(stderr, "Type '%s' is not currently supported by driver! Please report.\n", module_options[it].parameter_type);
				return -1;
			}
			break;
		case 'v':
			fprintf(stdout, "IOTG i915 forklift 2017-05-16 (4.11 final, GP1.2-Gold, EC1750-RC3, BKC32_EC1750_RC4)\n");
			fprintf(stdout, "IOTG i915 build " IOTG_BUILD_ID "\n");
			return -1;
		case 'f':
			video_setup(strdup(optarg));
			break;
		case 'w':
			/* Do not perform path validation if it exist or not, it is up to user to provide correct path */
			strncpy(firmware_path, optarg, sizeof(firmware_path) - 1);
			break;
		default :
			break;
		}
	}
	return 0;
}

/*
 * qnx_get_priority
 * get real priority from QNX_PRTY_*
 */
int qnx_get_priority(enum qnx_priority prio,
		struct task_struct *task)
{
	int	newprio, ret;
	struct task_struct temp_task;

	if (!task) {
		task = &temp_task;
		ret = qnx_current_task_sched(task);
		if (ret) {
			return ret;	/* minus value */
		}
	}

	if (prio > 0) {
		return (int)prio;
	}

	newprio = task->sched_max_priority + prio;
	if (prio <= 0 && newprio > 0) {
		return newprio;
	}
	return task->sched_param.sched_priority;
}

/*
 * qnx_taskattr_init
 * normalize pthread_attr_init for qnx tasks
 *
 * copyright DENSO corporation
 */
int qnx_taskattr_init(pthread_attr_t *attr,
		enum qnx_priority prio)
{
	int	ret;
	struct task_struct temp_task;

	memset(attr, 0, sizeof(*attr));
	if ((ret = system_call(pthread_attr_init(attr)))) {
		return ret;
	}
	if (	(ret = system_call(pthread_attr_setstacksize(
				attr, THREAD_STACK_SIZE))) ||
		(ret = system_call(pthread_attr_setguardsize(
				attr, THREAD_STACK_GUARD_SIZE))) ||
		(ret = system_call(pthread_attr_setinheritsched(
				attr, PTHREAD_EXPLICIT_SCHED))) ) {
		goto destroy_init;
	}

	if (	(ret = qnx_current_task_sched(&temp_task))) {
		goto destroy_init;
	}

		/*
		 * QNX_PRTY_DEFAULT :
		 * 	no change policy / prio from parent thread
		 * QNX_PRTY_HIGH, QNX_PRTY_KERNEL
		 *	SCHED_RR, max - _DOWN
		 * QNX_PRTY_MAX :
		 * 	SCHED_RR, max
		 */

	if (	prio != QNX_PRTY_DEFAULT &&
		temp_task.sched_policy != SCHED_RR &&
		temp_task.sched_policy != SCHED_FIFO ) {
		/*
		 * not realtime, change to RT
		 */
		int policy, max_prio;

		policy = SCHED_RR;
		max_prio = system_call_errno(
				sched_get_priority_max(policy));
		if (max_prio < 0) {
			ret = errno;
			goto destroy_init;
		}
		temp_task.sched_policy = policy;
		temp_task.sched_max_priority = max_prio;
		temp_task.sched_high_priority =
			qnx_get_priority(QNX_PRTY_HIGH, &temp_task);
	}

	temp_task.sched_param.sched_priority =
			qnx_get_priority(prio, &temp_task);

	if (	(ret = system_call(pthread_attr_setschedpolicy(
				attr, temp_task.sched_policy))) ||
		(ret = system_call(pthread_attr_setschedparam(
				attr, &temp_task.sched_param))) ) {
		goto destroy_init;
	}
#ifdef DEBUG_SCHED_PARAM
	printk(KERN_INFO "DEBUG %s[%d] policy=%d prio=%d caller=%#x\n",
			__func__, pthread_self(),
			policy, sp.sched_priority,
			caller_main_offset());
#endif
	return 0;
destroy_init:
	pthread_attr_destroy(attr);
	return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/qnx_resmgr.c $ $Rev: 864420 $")
#endif
