/* unity_config.h */

#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#define UNITY_INCLUDE_DOUBLE

#ifndef HAVE_STDINT_H
# define UNITY_EXCLUDE_STDINT_H
#endif

#endif /* UNITY_CONFIG_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/sntp/unity/unity_config.h $ $Rev: 799952 $")
#endif
