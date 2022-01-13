
#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/sntp/libevent/test/tinytest_local.h $ $Rev: 777476 $")
#endif
