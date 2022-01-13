/*
 * pm_runtime.h - Device run-time power management helper functions.
 *
 * Copyright (C) 2009 Rafael J. Wysocki <rjw@sisk.pl>
 * Some modifications Copyright (c) 2017 QNX Software Systems.
 *
 * This file is released under the GPLv2.
 */

#ifndef _LINUX_PM_RUNTIME_H
#define _LINUX_PM_RUNTIME_H

#ifdef __QNXNTO__
#include <linux/pm.h>
#include <linux/workqueue.h>
#include <errno.h>
#else
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/pm.h>

#include <linux/jiffies.h>
#endif

/* Runtime PM flag argument bits */
#define RPM_ASYNC		0x01	/* Request is asynchronous */
#define RPM_NOWAIT		0x02	/* Don't wait for concurrent
					    state change */
#define RPM_GET_PUT		0x04	/* Increment/decrement the
					    usage_count */
#define RPM_AUTO		0x08	/* Use autosuspend_delay */

static inline bool queue_pm_work(struct work_struct *work) { return false; }

#ifndef __QNXNTO__
static inline int pm_generic_runtime_suspend(struct device *dev) { return 0; }
static inline int pm_generic_runtime_resume(struct device *dev) { return 0; }
#else
int pm_generic_runtime_suspend(struct device *dev);
int pm_generic_runtime_resume(struct device *dev);
#endif
static inline int pm_runtime_force_suspend(struct device *dev) { return 0; }
static inline int pm_runtime_force_resume(struct device *dev) { return 0; }

static inline int __pm_runtime_idle(struct device *dev, int rpmflags)
{
	return -ENOSYS;
}
static inline int __pm_runtime_suspend(struct device *dev, int rpmflags)
{
	return -ENOSYS;
}
static inline int __pm_runtime_resume(struct device *dev, int rpmflags)
{
	return 1;
}
static inline int pm_schedule_suspend(struct device *dev, unsigned int delay)
{
	return -ENOSYS;
}
static inline int __pm_runtime_set_status(struct device *dev,
					    unsigned int status) { return 0; }
static inline int pm_runtime_barrier(struct device *dev) { return 0; }
static inline void pm_runtime_enable(struct device *dev) {}
static inline void __pm_runtime_disable(struct device *dev, bool c) {}
static inline void pm_runtime_allow(struct device *dev) {}
static inline void pm_runtime_forbid(struct device *dev) {}

static inline bool pm_children_suspended(struct device *dev) { return false; }
static inline void pm_runtime_get_noresume(struct device *dev) {}
static inline void pm_runtime_put_noidle(struct device *dev) {}
static inline bool device_run_wake(struct device *dev) { return false; }
static inline void device_set_run_wake(struct device *dev, bool enable) {}
static inline bool pm_runtime_suspended(struct device *dev) { return false; }
static inline bool pm_runtime_active(struct device *dev) { return true; }
static inline bool pm_runtime_status_suspended(struct device *dev) { return false; }
static inline bool pm_runtime_suspended_if_enabled(struct device *dev) { return false; }
static inline bool pm_runtime_enabled(struct device *dev) { return false; }

static inline void pm_runtime_no_callbacks(struct device *dev) {}
static inline void pm_runtime_irq_safe(struct device *dev) {}
static inline bool pm_runtime_is_irq_safe(struct device *dev) { return false; }

static inline bool pm_runtime_callbacks_present(struct device *dev) { return false; }
static inline void pm_runtime_mark_last_busy(struct device *dev) {}
static inline void __pm_runtime_use_autosuspend(struct device *dev,
						bool use) {}
static inline void pm_runtime_set_autosuspend_delay(struct device *dev,
						int delay) {}
static inline unsigned long pm_runtime_autosuspend_expiration(
				struct device *dev) { return 0; }
static inline void pm_runtime_set_memalloc_noio(struct device *dev,
						bool enable){}

static inline int pm_runtime_idle(struct device *dev)
{
	return __pm_runtime_idle(dev, 0);
}

static inline int pm_runtime_suspend(struct device *dev)
{
	return __pm_runtime_suspend(dev, 0);
}

static inline int pm_runtime_autosuspend(struct device *dev)
{
	return __pm_runtime_suspend(dev, RPM_AUTO);
}

static inline int pm_runtime_resume(struct device *dev)
{
	return __pm_runtime_resume(dev, 0);
}

static inline int pm_request_idle(struct device *dev)
{
	return __pm_runtime_idle(dev, RPM_ASYNC);
}

static inline int pm_request_resume(struct device *dev)
{
	return __pm_runtime_resume(dev, RPM_ASYNC);
}

static inline int pm_request_autosuspend(struct device *dev)
{
	return __pm_runtime_suspend(dev, RPM_ASYNC | RPM_AUTO);
}

static inline int pm_runtime_get(struct device *dev)
{
	return __pm_runtime_resume(dev, RPM_GET_PUT | RPM_ASYNC);
}

static inline int pm_runtime_get_sync(struct device *dev)
{
	return __pm_runtime_resume(dev, RPM_GET_PUT);
}

static inline int pm_runtime_put(struct device *dev)
{
	return __pm_runtime_idle(dev, RPM_GET_PUT | RPM_ASYNC);
}

static inline int pm_runtime_put_autosuspend(struct device *dev)
{
	return __pm_runtime_suspend(dev,
	    RPM_GET_PUT | RPM_ASYNC | RPM_AUTO);
}

static inline int pm_runtime_put_sync(struct device *dev)
{
	return __pm_runtime_idle(dev, RPM_GET_PUT);
}

static inline int pm_runtime_put_sync_suspend(struct device *dev)
{
	return __pm_runtime_suspend(dev, RPM_GET_PUT);
}

static inline int pm_runtime_put_sync_autosuspend(struct device *dev)
{
	return __pm_runtime_suspend(dev, RPM_GET_PUT | RPM_AUTO);
}

static inline int pm_runtime_set_active(struct device *dev)
{
	return __pm_runtime_set_status(dev, RPM_ACTIVE);
}

static inline void pm_runtime_set_suspended(struct device *dev)
{
	__pm_runtime_set_status(dev, RPM_SUSPENDED);
}

static inline void pm_runtime_disable(struct device *dev)
{
	__pm_runtime_disable(dev, true);
}

static inline void pm_runtime_use_autosuspend(struct device *dev)
{
	__pm_runtime_use_autosuspend(dev, true);
}

static inline void pm_runtime_dont_use_autosuspend(struct device *dev)
{
	__pm_runtime_use_autosuspend(dev, false);
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/pm_runtime.h $ $Rev: 838597 $")
#endif
