#ifndef _LINUX_AUXVEC_H
#define _LINUX_AUXVEC_H

#include <uapi/linux/auxvec.h>

#define AT_VECTOR_SIZE_BASE 20 /* NEW_AUX_ENT entries in auxiliary table */
  /* number of "#define AT_.*" above, minus {AT_NULL, AT_IGNORE, AT_NOTELF} */
#endif /* _LINUX_AUXVEC_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/auxvec.h $ $Rev: 836322 $")
#endif
