#ifndef _LINUX_IRQNR_H
#define _LINUX_IRQNR_H

extern int nr_irqs;

extern struct irq_desc *irq_to_desc(unsigned int irq);
unsigned int irq_get_next_irq(unsigned int offset);

#endif /* _LINUX_IRQNR_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/irqnr.h $ $Rev: 836322 $")
#endif
