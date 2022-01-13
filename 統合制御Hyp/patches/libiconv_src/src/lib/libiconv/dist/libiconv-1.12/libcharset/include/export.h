
#if @HAVE_VISIBILITY@ && BUILDING_LIBCHARSET
#define LIBCHARSET_DLL_EXPORTED __attribute__((__visibility__("default")))
#else
#define LIBCHARSET_DLL_EXPORTED
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/lib/libiconv/dist/libiconv-1.12/libcharset/include/export.h $ $Rev: 680336 $")
#endif
