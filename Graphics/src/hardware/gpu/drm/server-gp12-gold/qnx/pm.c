#include <linux/qnx.h>
#include <linux/linux.h>

#ifdef CONFIG_PM
enum pm_qos_flags_status __dev_pm_qos_flags(struct device *dev, s32 mask)
{
	return PM_QOS_FLAGS_UNDEFINED; 
}

enum pm_qos_flags_status dev_pm_qos_flags(struct device *dev, s32 mask)
{
	return PM_QOS_FLAGS_UNDEFINED; 
}

s32 __dev_pm_qos_read_value(struct device *dev)
{
	return 0;
}

s32 dev_pm_qos_read_value(struct device *dev)
{
	return 0;
}

int dev_pm_qos_add_request(struct device *dev, struct dev_pm_qos_request *req,
	enum dev_pm_qos_req_type type, s32 value)
{
	return 0;
}

int dev_pm_qos_update_request(struct dev_pm_qos_request *req,
							  s32 new_value)
{
	return 0;
}

int dev_pm_qos_remove_request(struct dev_pm_qos_request *req)
{
	return 0;
}

int dev_pm_qos_add_notifier(struct device *dev, struct notifier_block *notifier)
{
	return 0;
}

int dev_pm_qos_remove_notifier(struct device *dev, struct notifier_block *notifier)
{
	return 0;
}

int dev_pm_qos_add_global_notifier(struct notifier_block *notifier)
{
	return 0;
}

int dev_pm_qos_remove_global_notifier(struct notifier_block *notifier)
{
	return 0;
}

void dev_pm_qos_constraints_init(struct device *dev)
{
	dev->power.power_state = PMSG_ON;
}

void dev_pm_qos_constraints_destroy(struct device *dev)
{
	dev->power.power_state = PMSG_INVALID;
}

int dev_pm_qos_add_ancestor_request(struct device *dev,
				    struct dev_pm_qos_request *req,
				    enum dev_pm_qos_req_type type, s32 value)
{
	return 0;
}

int pm_generic_runtime_suspend(struct device *dev)
{
	dev->power.runtime_status = RPM_SUSPENDED;
	return 0;
}

int pm_generic_runtime_resume(struct device *dev)
{
	dev->power.runtime_status = RPM_ACTIVE;
	return 0;
}

int dev_pm_domain_attach(struct device *dev, bool power_on)
{
	return 0;
}

void dev_pm_domain_detach(struct device *dev, bool power_off)
{
}

int dev_pm_set_wake_irq(struct device *dev, int irq)
{
	return 0;
}

int dev_pm_set_dedicated_wake_irq(struct device *dev, int irq)
{
	return 0;
}

void dev_pm_clear_wake_irq(struct device *dev)
{
}

void dev_pm_enable_wake_irq(struct device *dev)
{
}

void dev_pm_disable_wake_irq(struct device *dev)
{
}

void pm_runtime_no_callbacks(struct device *dev)
{
}

const char power_group_name[] = "power";

/**
 * pm_qos_remove_request - modifies an existing qos request
 * @req: handle to request list element
 *
 * Will remove pm qos request from the list of constraints and
 * recompute the current target value for the pm_qos_class.  Call this
 * on slow code paths.
 */
void pm_qos_remove_request(struct pm_qos_request *req)
{
}


void pm_qos_add_request(struct pm_qos_request *req,
			int pm_qos_class, s32 value)
{
}

void pm_qos_update_request(struct pm_qos_request *req,
			   s32 new_value)
{
}

/**
 * rpm_idle - Notify device bus type if the device can be suspended.
 * @dev: Device to notify the bus type about.
 * @rpmflags: Flag bits.
 *
 * Check if the device's runtime PM status allows it to be suspended.  If
 * another idle notification has been started earlier, return immediately.  If
 * the RPM_ASYNC flag is set then queue an idle-notification request; otherwise
 * run the ->runtime_idle() callback directly. If the ->runtime_idle callback
 * doesn't exist or if it returns 0, call rpm_suspend with the RPM_AUTO flag.
 *
 * This function must be called under dev->power.lock with interrupts disabled.
 */
static int rpm_idle(struct device *dev, int rpmflags)
{
	return 0;
}

/**
 * pm_runtime_allow - Unblock runtime PM of a device.
 * @dev: Device to handle.
 *
 * Decrease the device's usage count and set its power.runtime_auto flag.
 */
void pm_runtime_allow(struct device *dev)
{
	if (atomic_dec_and_test(&dev->power.usage_count))
		rpm_idle(dev, RPM_AUTO | RPM_ASYNC);
}

/**
 * pm_runtime_forbid - Block runtime PM of a device.
 * @dev: Device to handle.
 *
 * Increase the device's usage count and clear its power.runtime_auto flag,
 * so that it cannot be suspended at run time until pm_runtime_allow() is called
 * for it.
 */
void pm_runtime_forbid(struct device *dev)
{
	atomic_inc(&dev->power.usage_count);
}

/**
 * __pm_runtime_set_status - Set runtime PM status of a device.
 * @dev: Device to handle.
 * @status: New runtime PM status of the device.
 *
 * If runtime PM of the device is disabled or its power.runtime_error field is
 * different from zero, the status may be changed either to RPM_ACTIVE, or to
 * RPM_SUSPENDED, as long as that reflects the actual state of the device.
 * However, if the device has a parent and the parent is not active, and the
 * parent's power.ignore_children flag is unset, the device's status cannot be
 * set to RPM_ACTIVE, so -EBUSY is returned in that case.
 *
 * If successful, __pm_runtime_set_status() clears the power.runtime_error field
 * and the device parent's counter of unsuspended children is modified to
 * reflect the new status.  If the new status is RPM_SUSPENDED, an idle
 * notification request for the parent is submitted.
 */
int __pm_runtime_set_status(struct device *dev, unsigned int status)
{
	if (status != RPM_ACTIVE && status != RPM_SUSPENDED)
		return -EINVAL;

	dev->power.runtime_status = status;

	return 0;
}

/**
 * pm_runtime_enable - Enable runtime PM of a device.
 * @dev: Device to handle.
 */
void pm_runtime_enable(struct device *dev)
{
	/* In our emulation environment this function does nothing */
}

/**
 * __pm_runtime_resume - Entry point for runtime resume operations.
 * @dev: Device to resume.
 * @rpmflags: Flag bits.
 *
 * If the RPM_GET_PUT flag is set, increment the device's usage count.  Then
 * carry out a resume, either synchronous or asynchronous.
 *
 * This routine may be called in atomic context if the RPM_ASYNC flag is set,
 * or if pm_runtime_irq_safe() has been called.
 */
int __pm_runtime_resume(struct device *dev, int rpmflags)
{
	if (rpmflags & RPM_GET_PUT)
		atomic_inc(&dev->power.usage_count);

	dev->power.runtime_status = RPM_ACTIVE;

	return 1;
}

/**
 * pm_runtime_get_if_in_use - Conditionally bump up the device's usage counter.
 * @dev: Device to handle.
 *
 * Return -EINVAL if runtime PM is disabled for the device.
 *
 * If that's not the case and if the device's runtime PM status is RPM_ACTIVE
 * and the runtime PM usage counter is nonzero, increment the counter and
 * return 1.  Otherwise return 0 without changing the counter.
 */
int pm_runtime_get_if_in_use(struct device *dev)
{
	return (dev->power.runtime_status == RPM_ACTIVE &&
		atomic_inc_not_zero(&dev->power.usage_count));
}

/**
 * __pm_runtime_suspend - Entry point for runtime put/suspend operations.
 * @dev: Device to suspend.
 * @rpmflags: Flag bits.
 *
 * If the RPM_GET_PUT flag is set, decrement the device's usage count and
 * return immediately if it is larger than zero.  Then carry out a suspend,
 * either synchronous or asynchronous.
 *
 * This routine may be called in atomic context if the RPM_ASYNC flag is set,
 * or if pm_runtime_irq_safe() has been called.
 */
int __pm_runtime_suspend(struct device *dev, int rpmflags)
{
	if (rpmflags & RPM_GET_PUT) {
		if (!atomic_dec_and_test(&dev->power.usage_count))
			return 0;
	}

	dev->power.runtime_status = RPM_SUSPENDED;

	return 0;
}

/**
 * __pm_runtime_idle - Entry point for runtime idle operations.
 * @dev: Device to send idle notification for.
 * @rpmflags: Flag bits.
 *
 * If the RPM_GET_PUT flag is set, decrement the device's usage count and
 * return immediately if it is larger than zero.  Then carry out an idle
 * notification, either synchronous or asynchronous.
 *
 * This routine may be called in atomic context if the RPM_ASYNC flag is set,
 * or if pm_runtime_irq_safe() has been called.
 */
int __pm_runtime_idle(struct device *dev, int rpmflags)
{
	if (rpmflags & RPM_GET_PUT) {
		if (!atomic_dec_and_test(&dev->power.usage_count))
			return 0;
	}

	/* Pretend we can put device to idle mode */
	return rpm_idle(dev, rpmflags);
}

/**
 * pm_runtime_set_autosuspend_delay - Set a device's autosuspend_delay value.
 * @dev: Device to handle.
 * @delay: Value of the new delay in milliseconds.
 *
 * Set the device's power.autosuspend_delay value.  If it changes to negative
 * and the power.use_autosuspend flag is set, prevent runtime suspends.  If it
 * changes the other way, allow runtime suspends.
 */
void pm_runtime_set_autosuspend_delay(struct device *dev, int delay)
{
	/* Function body is empty because we don't support this mode */
}

/**
 * __pm_runtime_use_autosuspend - Set a device's use_autosuspend flag.
 * @dev: Device to handle.
 * @use: New value for use_autosuspend.
 *
 * Set the device's power.use_autosuspend flag, and allow or prevent runtime
 * suspends as needed.
 */
void __pm_runtime_use_autosuspend(struct device *dev, bool use)
{
	/* Function body is empty because we don't support this mode */
}

#endif /* CONFIG_PM */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/pm.c $ $Rev: 853163 $")
#endif
