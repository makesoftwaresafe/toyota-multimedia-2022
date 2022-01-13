#ifndef _LINUX_IRQHANDLER_H
#define _LINUX_IRQHANDLER_H

/*
 * Interrupt flow handler typedefs are defined here to avoid circular
 * include dependencies.
 */

struct irq_desc;
struct irq_data;
typedef	void (*irq_flow_handler_t)(unsigned int irq, struct irq_desc *desc);
typedef	void (*irq_preflow_handler_t)(struct irq_data *data);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/irqhandler.h $ $Rev: 836322 $")
#endif
