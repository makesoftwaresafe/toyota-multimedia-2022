#ifndef __linux_video_edid_h__
#define __linux_video_edid_h__

#include <uapi/video/edid.h>

#ifdef CONFIG_X86
extern struct edid_info edid_info;
#endif
#endif /* __linux_video_edid_h__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/video/edid.h $ $Rev: 836322 $")
#endif
