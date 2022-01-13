
#if @HAVE_VISIBILITY@ && BUILDING_LIBICONV
#define LIBICONV_DLL_EXPORTED __attribute__((__visibility__("default")))
#else
#define LIBICONV_DLL_EXPORTED
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/lib/libiconv/dist/libiconv-1.12/include/export.h $ $Rev: 680336 $")
#endif
