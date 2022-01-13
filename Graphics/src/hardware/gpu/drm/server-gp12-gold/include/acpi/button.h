#ifndef ACPI_BUTTON_H
#define ACPI_BUTTON_H

#include <linux/kconfig.h>
#include <linux/notifier.h>

#if IS_ENABLED(CONFIG_ACPI_BUTTON)
extern int acpi_lid_notifier_register(struct notifier_block *nb);
extern int acpi_lid_notifier_unregister(struct notifier_block *nb);
extern int acpi_lid_open(void);
#else
static inline int acpi_lid_notifier_register(struct notifier_block *nb)
{
	return 0;
}
static inline int acpi_lid_notifier_unregister(struct notifier_block *nb)
{
	return 0;
}
static inline int acpi_lid_open(void)
{
	return 1;
}
#endif /* IS_ENABLED(CONFIG_ACPI_BUTTON) */

#endif /* ACPI_BUTTON_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/acpi/button.h $ $Rev: 836322 $")
#endif
