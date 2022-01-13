/**
 * \file: rate_core_if.h
 *
 * Interface to ADIT sample rate converter SRC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __RATE_CORE_IF_H__
#define __RATE_CORE_IF_H__


struct rate_core_init_side_info{
	unsigned int freq;
	unsigned int period_size;
	signed int pitch_frames_max;
};

struct rate_core_init_info{
	struct rate_core_init_side_info in;
	struct rate_core_init_side_info out;
	unsigned int channels;
};
#define RATE_CORE_INIT_INFO_INITIALIZER {.in={0},.out={0},.channels =0}

struct rate_core_convert_side_info{
	int16_t* buf;
	unsigned int frames;
	signed int pitch_frames;
};

struct rate_core_convert_info{
	struct rate_core_convert_side_info in;
	struct rate_core_convert_side_info out;
};

struct adit_swsrc_core_ops{
	int (*set_log)(void *ctx, int is_err, void(*log_fkt)(const char *file, int line, const char *function, int err, const char *fmt, ...));
	int (*init)(void *ctx, struct rate_core_init_info *info);
	int (*info)(void *ctx, char* buf, int len);
	int (*close)(void*ctx);
	int (*convert_s16)(void *ctx, struct rate_core_convert_info *info);
	unsigned int (*in_frames)(void *ctx, unsigned int frames);
	unsigned int (*out_frames)(void *ctx, unsigned int frames);
};



#define ADIT_SWSRC_LIB_NAME "libadit-swsrc.so"
#define ADIT_SWSRC_CORE_ENTRY adit_swsrc_core_open

/** open function type */
typedef int (*adit_swsrc_core_open_func_t)(unsigned int version, void **objp, struct adit_swsrc_core_ops **ops);

/** interface version */
#define ADIT_RATE_CORE_IF_VERSION 0x000102


#endif /* __RATE_CORE_IF_H__*/


