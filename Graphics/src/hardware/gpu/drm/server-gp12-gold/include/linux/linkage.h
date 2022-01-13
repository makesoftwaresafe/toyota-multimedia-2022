/*
 * offset from main entry -- for debug
 */
extern int main(int ac, char **av);
#define main_offset(func)	({	\
		unsigned long	fp = (uintptr_t)(func);	\
		fp = (fp < 0x100000000) ? fp : _THIS_IP_;	\
		(int)(fp - (uintptr_t)main); })
#define caller_main_offset()	main_offset(__builtin_return_address(0))

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/linkage.h $ $Rev: 836322 $")
#endif
