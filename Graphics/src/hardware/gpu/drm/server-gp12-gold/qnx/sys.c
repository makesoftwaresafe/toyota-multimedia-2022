#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/highuid.h>

int overflowuid = DEFAULT_OVERFLOWUID;
int overflowgid = DEFAULT_OVERFLOWGID;

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/sys.c $ $Rev: 855874 $")
#endif
