# ifdef CONFIG_X86_32
#  include <uapi/asm/posix_types_32.h>
# else
#  include <uapi/asm/posix_types_64.h>
# endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/posix_types.h $ $Rev: 836322 $")
#endif
