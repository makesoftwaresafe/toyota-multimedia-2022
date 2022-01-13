#ifndef _QNX_LINUX_PROPERTY_H_
#define _QNX_LINUX_PROPERTY_H_

enum fwnode_type {
	FWNODE_INVALID = 0,
	FWNODE_OF,
	FWNODE_ACPI,
};

struct fwnode_handle {
	enum fwnode_type type;
};

#endif /* _QNX_LINUX_PROPERTY_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/property.h $ $Rev: 836322 $")
#endif
