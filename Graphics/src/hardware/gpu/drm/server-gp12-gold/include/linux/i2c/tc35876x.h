
#ifndef _TC35876X_H
#define _TC35876X_H

struct tc35876x_platform_data {
	int gpio_bridge_reset;
	int gpio_panel_bl_en;
	int gpio_panel_vadd;
};

#endif /* _TC35876X_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/i2c/tc35876x.h $ $Rev: 836322 $")
#endif
