/*
 * Xtensa built-in interrupt controller
 *
 * Copyright (C) 2002 - 2013 Tensilica, Inc.
 * Copyright (C) 1992, 1998 Linus Torvalds, Ingo Molnar
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef __LINUX_IRQCHIP_XTENSA_PIC_H
#define __LINUX_IRQCHIP_XTENSA_PIC_H

struct device_node;
int xtensa_pic_init_legacy(struct device_node *interrupt_parent);

#endif /* __LINUX_IRQCHIP_XTENSA_PIC_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/irqchip/xtensa-pic.h $ $Rev: 836322 $")
#endif
