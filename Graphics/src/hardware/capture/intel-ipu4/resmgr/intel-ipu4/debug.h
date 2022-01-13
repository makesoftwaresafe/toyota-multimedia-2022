#ifndef _QNX_DEBUG_H
#define _QNX_DEBUG_H

#include <sys/neutrino.h>
#include <sys/trace.h>
#include <sys/slogcodes.h>
#include <errno.h>

#define in_dbg_master() (0)

#define _SLOG_MAJOR _SLOGC_GRAPHICS
#define _SLOG_MINOR 915

#define SLOGC_IGPU _SLOG_SETCODE(_SLOG_MAJOR, _SLOG_MINOR)
#define SLOGC_SELF SLOGC_IGPU

/* trace routine */
static inline
void dbg_noop(const char * fmt, ...){
}

#define qnx_trace_log(format,...)						   \
	trace_logf(_NTO_TRACE_USERFIRST+915, " drm_intel [%s:%d]"#format, \
			   __func__, __LINE__, ##__VA_ARGS__)

#define qnx_slog(format,...)								\
	do {													\
		slogf(SLOGC_SELF, _SLOG_WARNING, "[%s:%d]"#format,	\
			  __func__, __LINE__, ##__VA_ARGS__);			\
	} while(0)


#ifdef DRM_PROFILE
#define qnx_trace_profile(format, ...)								\
	do {															\
	     qnx_trace_log(format, ##__VA_ARGS__);						\
		 trace_logf(_NTO_TRACE_USERFIRST+916, "[%s:%d]"#format,		\
					__func__, __LINE__, ##__VA_ARGS__);				\
		 slogf(SLOGC_SELF, _SLOG_INFO, "[%s:%d]"#format,			\
			   __func__, __LINE__, ##__VA_ARGS__);					\
	}while(0)

#else
#define qnx_trace_profile(format, ...)		\
	dbg_noop(format, ##__VA_ARGS__)
#endif

#define qnx_warning(format,...)								\
	do {													\
		slogf(SLOGC_SELF, _SLOG_WARNING, "[%s:%d]"#format,	\
			  __func__, __LINE__, ##__VA_ARGS__);			\
		qnx_trace_log(format, ##__VA_ARGS__);				\
	} while(0)

#define qnx_error(format, ...)											\
	do {																\
		slogf(SLOGC_SELF, _SLOG_ERROR, "[errno:%d @ %s:%d]"#format,		\
			  errno, __func__, __LINE__, ##__VA_ARGS__);				\
		qnx_trace_log("[errno:%d @ %s:%d]"#format,						\
					  errno, __func__, __LINE__, ##__VA_ARGS__);		\
	}while(0)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/intel-ipu4/debug.h $ $Rev: 838597 $")
#endif
