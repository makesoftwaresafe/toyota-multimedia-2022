#ifndef _UAPI__linux_video_edid_h__
#define _UAPI__linux_video_edid_h__

struct edid_info {
	unsigned char dummy[128];
};


#endif /* _UAPI__linux_video_edid_h__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/uapi/video/edid.h $ $Rev: 836322 $")
#endif
