#include <linux/qnx.h>
#include <linux/list.h>
#include <drm/drmP.h>

static pthread_mutex_t irq_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * NOP functions
 */
static void noop(struct irq_data* data)
{
}

static unsigned int noop_ret(struct irq_data *data)
{
	return 0;
}

/*
 * Generic no controller implementation
 */
struct irq_chip no_irq_chip = {
	.name		= "none",
	.irq_startup	= noop_ret,
	.irq_shutdown	= noop,
	.irq_enable	= noop,
	.irq_disable	= noop,
	.irq_ack	= noop,
};

/*
 * Generic dummy implementation which can be used for
 * real dumb interrupt sources
 */
struct irq_chip dummy_irq_chip = {
	.name		= "dummy",
	.irq_startup	= noop_ret,
	.irq_shutdown	= noop,
	.irq_enable	= noop,
	.irq_disable	= noop,
	.irq_ack	= noop,
	.irq_mask	= noop,
	.irq_unmask	= noop,
	.flags		= IRQCHIP_SKIP_SET_WAKE,
};
EXPORT_SYMBOL_GPL(dummy_irq_chip);

enum isr_flag_bits {
	ISRFLAG_INIT_THREAD,
	ISRFLAG_STOPPING,
	ISRFLAG_WORKING,
	ISRFLAG_DISABLED,
};
#define ISR_PULSE (_PULSE_CODE_MINAVAIL+1)

// We're using a linked list (instead of a tree) for simplicity;
// we only expect one interrupt to be attached anyway.
static struct irq_desc irq_handler_list = {
	.list_entry = LIST_HEAD_INIT(irq_handler_list.list_entry),
};

struct irq_desc* irq_to_desc(unsigned int irq)
{
	struct irq_desc* irqinfo;

	pthread_mutex_lock(&irq_mutex);

	/*TODO. FIXME. add mutex for handling multi-irq */
	list_for_each_entry(irqinfo, &irq_handler_list.list_entry, list_entry) {
		if (irqinfo->irq == irq) {
			pthread_mutex_unlock(&irq_mutex);
			return irqinfo;
		}
	}

	pthread_mutex_unlock(&irq_mutex);

	return NULL;
}

static int irq_invoke(struct irq_desc* irqinfo)
{
	if (!irqinfo->handler) {
		return -1;	/* no handler */
	}

	if (irqs_disabled() ||
		test_bit(ISRFLAG_DISABLED, &irqinfo->atomic_flags)) {
		/*
		 * already disabled, then pending irq
		 */
		atomic_set(&irqinfo->irq_pend, 1);
		return 0;
	}

	set_bit(ISRFLAG_WORKING, &irqinfo->atomic_flags);
	irq_enter();

	pthread_mutex_lock(&irq_mutex);
	current->preempt.qnx_irq_thread++;
		/*
		 * invoke handler serialized
		 * When irqs_disabled, not invoke handler, but
		 * set pending.
		 * When releasing irqs_disable, call irq_invoke
		 * from arch_local_irq_enable() on other thread.
		 * While IRQ thread can also call irq_invoke
		 * Now hander must run serialized
		 */
	irqinfo->handler(irqinfo->irq, irqinfo->dev_id);

	current->preempt.qnx_irq_thread--;
	pthread_mutex_unlock(&irq_mutex);

	irq_exit();
	clear_bit(ISRFLAG_WORKING, &irqinfo->atomic_flags);

	return 0;
}

/*
 * local irq flag control
 *
 * -- not treat QNX interrupt control, but only control scheduling handler
 * copyright DENSO corporation
 */
#if 0
static intrspin_t __spinlock;
static inline void native_irq_enable()
{
	InterruptUnlock(&__spinlock);
}

static inline void native_irq_disable()
{
	InterruptLock(&__spinlock);
}

void disable_irq(unsigned int irq)
{
	struct irq_desc* irqinfo = irq_to_desc(irq);
	if (irqinfo) {
		InterruptMask(irqinfo->irq, irqinfo->intr_id);
	}
}

void enable_irq(unsigned int irq)
{
	struct irq_desc * irqinfo = irq_to_desc(irq);
	if (irqinfo) {
		InterruptUnmask(irqinfo->irq, irqinfo->intr_id);
	}
}
#endif


#define HARDIRQ_DISABLE_OFFSET	(2 * HARDIRQ_OFFSET)
void arch_local_irq_enable()
{
	__preempt_count_sub_ip(HARDIRQ_DISABLE_OFFSET - 1, _RET_IP_);
		/* keep preempt_disable */
	/*
	 * schedule pending irq handler
	 */
	for(;;) {
		struct irq_desc* irqinfo;
		int	found_pend = 0;

		pthread_mutex_lock(&irq_mutex);

		list_for_each_entry(irqinfo, &irq_handler_list.list_entry,
				list_entry) {
			if (test_bit(ISRFLAG_DISABLED,
						&irqinfo->atomic_flags)) {
				/*
				 * not execute pending irq,
				 * because disabled
				 */
				continue;
			}
			if (atomic_add_return(-1, &irqinfo->irq_pend)) {
				atomic_add(1, &irqinfo->irq_pend);
			} else {
				found_pend = 1;
				break;
			}
		}
		pthread_mutex_unlock(&irq_mutex);

		if (!found_pend) {
			break;
		}
		/*
		 * execute handler here
		 */
		irq_invoke(irqinfo);
	}
	__preempt_count_sub_ip(1, _RET_IP_);
}
void arch_local_irq_disable()
{
	__preempt_count_add_ip(HARDIRQ_DISABLE_OFFSET, _RET_IP_);
}

void disable_irq(unsigned int irq)
{
	struct irq_desc* irqinfo = irq_to_desc(irq);

	set_bit(ISRFLAG_DISABLED, &irqinfo->atomic_flags);
	/*
	 * disable_irq not in_atomic, but preemptable
	 * see i915_reset
	 */
}
void enable_irq(unsigned int irq)
{
	struct irq_desc* irqinfo = irq_to_desc(irq);

	clear_bit(ISRFLAG_DISABLED, &irqinfo->atomic_flags);
	/*
	 * invoke pending irq
	 */
	local_irq_disable();
	local_irq_enable();
}

/**
 *	synchronize_irq - wait for pending IRQ handlers (on other CPUs)
 *	@irq: interrupt number to wait for
 *
 *	This function waits for any pending IRQ handlers for this interrupt
 *	to complete before returning. If you use this function while
 *	holding a resource the IRQ handler may need you will deadlock.
 *
 *	This function may be called - with care - from IRQ context.
 */
void synchronize_irq(unsigned int irq)
{
	struct irq_desc* irqinfo = irq_to_desc(irq);

	if (irqinfo) {
		while(test_bit(ISRFLAG_WORKING, &irqinfo->atomic_flags)) {
			usleep(1000);
		}
	}
}

static void* irq_thread(void *arg)
{
	struct irq_desc *irqinfo = arg;
	struct task_struct this_task, *qnx_irq_task;
	char namestr[_NTO_THREAD_NAME_MAX+1];
	int err = EOK;

	/* set thread name */
	snprintf(namestr, sizeof namestr, "isr%s%s", irqinfo->name ? "-" : "", irqinfo->name ? irqinfo->name : "");
	namestr[sizeof namestr - 1] = '\0';
	pthread_setname_np(0, namestr);

	/* Setup interrupt task context, task should have interrupted task */
	/* context, but we emulate interrupted kernel thread.              */
	qnx_irq_task = &this_task;
	memset(qnx_irq_task, 0, sizeof(*qnx_irq_task));
		/*
		 * not consider to fail create task_struct
		 */
	qnx_irq_task->pid = -1;
	qnx_irq_task->spid.pid = -1;
	strncpy(qnx_irq_task->comm, namestr, sizeof(qnx_irq_task->comm) - 1);
	qnx_irq_task->real_cred = &qnx_irq_task->cred_vault;
	qnx_irq_task->cred = &qnx_irq_task->cred_vault;
	err = qnx_create_task_sched(qnx_irq_task);
	if (err) {
		/* Error output is done inside qnx_create_task_sched() function */
		return NULL;
	}
	qnx_irq_task->preempt.qnx_irq_thread = 1;
	current = qnx_irq_task;
	qnx_irq_task->attachment.user_tid = pthread_self();
	qnx_irq_task->attachment.copy_to_user_memcpy = 1;

	clear_bit(ISRFLAG_INIT_THREAD, &irqinfo->atomic_flags);

	for(;;) {
		struct _pulse pulse;
		int rcvid;

		rcvid = MsgReceivePulse_r(irqinfo->chid, &pulse, sizeof pulse, NULL);
		if (test_bit(ISRFLAG_STOPPING, &irqinfo->atomic_flags)) {
			break;
		}
		if (rcvid < 0) {
			err = -rcvid;
			if (err == EINTR) {
				continue;
			}
			errno = err;
			qnx_error("MsgReceivePulse_r() returned an error");
			break;
		}

		if (pulse.code == ISR_PULSE) {
			if (irq_invoke(irqinfo)) {
				break;
			}

			// unmask the interrupt, so we can get the next event
			InterruptUnmask(irqinfo->irq, irqinfo->intr_id);
		}
	}

	qnx_destroy_task_sched(current);
	current = NULL;

	return (void*)(uintptr_t)err;
}

static void deinit_irq_locked(struct irq_desc *irqinfo)
{
	if (!irqinfo) {
		return;
	}

	if (irqinfo->intr_id >= 0) {
		InterruptDetach_r(irqinfo->intr_id);
		irqinfo->intr_id = -1;
	}

	irqinfo->handler = NULL;
	set_bit(ISRFLAG_STOPPING, &irqinfo->atomic_flags);
	MsgSendPulse_r(irqinfo->coid, -1, ISR_PULSE, 0);
	pthread_join(irqinfo->tid, NULL);

	if (irqinfo->coid >= 0) {
		ConnectDetach_r(irqinfo->coid);
		irqinfo->coid = -1;
	}
	if (irqinfo->chid >= 0) {
		ChannelDestroy_r(irqinfo->chid);
		irqinfo->chid = -1;
	}

	list_del(&irqinfo->list_entry);

	kfree(irqinfo);
}

// Returns EOK or a negative errno value.
int request_irq(unsigned int irq, irq_handler_t handler,
	unsigned long flags, const char *name, void *dev)
{
	struct irq_desc *irqinfo = NULL;
	unsigned int channel_flags = 0;
	struct sched_param param;
	int err = 0;

	pthread_attr_t	task_attr;

	DRM_DEBUG("Register %s int #%d\n", name, irq);

	err = pthread_mutex_lock(&irq_mutex);
	if (err) goto out_nolock;

	irqinfo = kmalloc(sizeof(*irqinfo), GFP_KERNEL);
	if (!irqinfo) {
		err = ENOMEM;
		goto out;
	}
	*irqinfo = (struct irq_desc) {
		.dev_id = dev,
		.irq = irq,
		.handler = handler,
		.linux_flags = flags,
		.name = name,
		.intr_id = -1,
		.chid = -1,
		.coid = -1,
		.irq_pend = ATOMIC_INIT(0),
	};

	INIT_LIST_HEAD(&irqinfo->list_entry);

#ifndef _NTO_TCTL_IO_PRIV
#define _NTO_TCTL_IO_PRIV _NTO_TCTL_IO
#endif
	ThreadCtl(_NTO_TCTL_IO_PRIV, 0);

#if defined(_NTO_CHF_PRIVATE)
	channel_flags |= _NTO_CHF_PRIVATE;
#endif /* _NTO_CHF_PRIVATE */

	irqinfo->chid = ChannelCreate_r(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK | channel_flags);
	if (irqinfo->chid < 0) {
		err = -irqinfo->chid;
		qnx_error("ChannelCreate_r() call failed!");
		goto out;
	}

	irqinfo->coid = ConnectAttach_r(0, 0, irqinfo->chid, _NTO_SIDE_CHANNEL, 0);
	if (irqinfo->coid < 0) {
		err = -irqinfo->coid;
		qnx_error("ConnectAttach_r() call failed!");
		goto out;
	}

	if ((err = qnx_taskattr_init(&task_attr, QNX_PRTY_MAX))) {
		/* err report is in qnx_taskattr_init */
		goto out;
	}

	if ((err = pthread_attr_getschedparam(&task_attr, &param))) {
		qnx_error("pthread_attr_getschedparam");
		goto out;
	}
	SIGEV_PULSE_INIT(&irqinfo->event, irqinfo->coid,
			param.sched_priority, ISR_PULSE, 0);

	irqinfo->intr_id = InterruptAttachEvent_r(irqinfo->irq, &irqinfo->event,
			_NTO_INTR_FLAGS_TRK_MSK | _NTO_INTR_FLAGS_PROCESS);
	if (irqinfo->intr_id < 0) {
		err = -irqinfo->intr_id;
		qnx_error("InterruptAttachEvent_r() call failed!");
		goto out;
	}

	err = pthread_create(&irqinfo->tid, &task_attr, irq_thread, irqinfo);
	if (err) {
		pthread_attr_destroy(&task_attr);
		qnx_error("pthread_create() call failed!");
		goto out;
	}

	set_bit(ISRFLAG_INIT_THREAD, &irqinfo->atomic_flags);

out:
	if (!err) {
		list_add(&irqinfo->list_entry, &irq_handler_list.list_entry);
	} else {
		deinit_irq_locked(irqinfo);
	}
	pthread_mutex_unlock(&irq_mutex);

out_nolock:
	return -err;
}

void free_irq(unsigned int irq, void *dev_id)
{
	struct irq_desc* irqinfo = NULL;
	struct irq_desc* temp = NULL;
	int err;

	err = pthread_mutex_lock(&irq_mutex);
	if (err) return;

	list_for_each_entry_safe(irqinfo, temp, &irq_handler_list.list_entry, list_entry) {
		if ((irqinfo->dev_id == dev_id) && (irqinfo->irq == irq)) {
			DRM_DEBUG("Remove irq #%d\n", irq);
			deinit_irq_locked(irqinfo);
		}
	}

	pthread_mutex_unlock(&irq_mutex);
}

/**
 *	irq_set_chip_data - set irq chip data for an irq
 *	@irq:	Interrupt number
 *	@data:	Pointer to chip specific data
 *
 *	Set the hardware irq chip data for an irq
 */
int irq_set_chip_data(unsigned int irq, void *data)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc) {
		return -EINVAL;
	}

	desc->irq_data.chip_data = data;

	return 0;
}
EXPORT_SYMBOL(irq_set_chip_data);

void irq_set_chip_and_handler_name(unsigned int irq, struct irq_chip *chip,
	irq_flow_handler_t handle, const char *name)
{
	irq_set_chip(irq, chip);
	__irq_set_handler(irq, handle, 0, name);
}
EXPORT_SYMBOL_GPL(irq_set_chip_and_handler_name);

/**
 *	irq_set_chip - set the irq chip for an irq
 *	@irq:	irq number
 *	@chip:	pointer to irq chip description structure
 */
int irq_set_chip(unsigned int irq, struct irq_chip *chip)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc) {
		return -EINVAL;
	}

	if (!chip) {
		chip = &no_irq_chip;
	}

	desc->irq_data.chip = chip;

	return 0;
}
EXPORT_SYMBOL(irq_set_chip);

/**
 * handle_bad_irq - handle spurious and unhandled irqs
 * @irq:       the interrupt number
 * @desc:      description of the interrupt
 *
 * Handles spurious and unhandled IRQ's. It also prints a debugmessage.
 */
void handle_bad_irq(unsigned int irq, struct irq_desc *desc)
{
}

/*
 * Special, empty irq handler:
 */
irqreturn_t no_action(int cpl, void *dev_id)
{
	return IRQ_NONE;
}
EXPORT_SYMBOL_GPL(no_action);

void __irq_set_handler(unsigned int irq, irq_flow_handler_t handle, int is_chained, const char *name)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc)
		return;

	if (!handle) {
		handle = handle_bad_irq;
	} else {
		struct irq_data *irq_data = &desc->irq_data;
		if (WARN_ON(!irq_data || irq_data->chip == &no_irq_chip))
			return;
	}

	desc->handle_irq = handle;
	desc->name = name;
}
EXPORT_SYMBOL_GPL(__irq_set_handler);

int generic_handle_irq(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc) {
		return -EINVAL;
	}

	generic_handle_irq_desc(desc);
	return 0;
}
EXPORT_SYMBOL_GPL(generic_handle_irq);

irqreturn_t handle_irq_event_percpu(struct irq_desc *desc, struct irqaction *action)
{
	irqreturn_t retval = IRQ_NONE;
	unsigned int flags = 0, irq = desc->irq_data.irq;
	irqreturn_t res;

	res = action->handler(irq, action->dev_id);

	return retval;
}

irqreturn_t handle_irq_event(struct irq_desc *desc)
{
	struct irqaction *action = desc->action;
	irqreturn_t ret = IRQ_NONE;

	if (action) {
		ret = handle_irq_event_percpu(desc, action);
	}

	return ret;
}

/**
 *	handle_simple_irq - Simple and software-decoded IRQs.
 *	@irq:	the interrupt number
 *	@desc:	the interrupt description structure for this irq
 *
 *	Simple interrupts are either sent from a demultiplexing interrupt
 *	handler or come from hardware, where no interrupt hardware control
 *	is necessary.
 *
 *	Note: The caller is expected to handle the ack, clear, mask and
 *	unmask issues if necessary.
 */
void handle_simple_irq(unsigned int irq, struct irq_desc *desc)
{
	handle_irq_event(desc);
}
EXPORT_SYMBOL_GPL(handle_simple_irq);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/qnx_interrupt.c $ $Rev: 856018 $")
#endif
