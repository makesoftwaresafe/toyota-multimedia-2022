/**
 * \file: arch_selector.h
 *
 * ADIT SRC core implementation. Selection of arch dependent optimizations.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include "rn_src.h" 
#include "rn_dec.h" 
#include "src_impl_default.h"
#include "dec_impl_default.h"

#if defined(__arm__) || defined(__aarch64__)
#include <sys/auxv.h>
#include <asm/hwcap.h>
#if defined (__ARM_NEON__)
#include "src_impl_neon.h"
#include "dec_impl_neon.h"
#elif defined (__ARM_NEON)
#include "src_impl_asimd.h"
#include "dec_impl_asimd.h"
#endif
#elif defined(__i386__) || defined(__x86_64__)
#if defined (__SSE3__)
#include "src_impl_sse.h"
#include "dec_impl_sse.h"
#endif
#if defined (__AVX__)
#include "src_impl_avx.h"
#include "dec_impl_avx.h"
#endif
#endif


#define link_filters(s,d,i, type) do {\
	*(i) = #type;\
	(s)->src_funcs[0] = src_filt_1ch_##type;\
	(s)->src_funcs[1] = src_filt_2ch_##type;\
	(s)->src_funcs[2] = src_filt_3ch_##type;\
	(s)->src_funcs[3] = src_filt_4ch_##type;\
	(d)->dec2_funcs[0] = dec2_filt_1ch_##type;\
	(d)->dec2_funcs[1] = dec2_filt_2ch_##type;\
	(d)->dec2_funcs[2] = dec2_filt_3ch_##type;\
	(d)->dec2_funcs[3] = dec2_filt_4ch_##type;\
	(d)->dec3_funcs[0] = dec3_filt_1ch_##type;\
	(d)->dec3_funcs[1] = dec3_filt_2ch_##type;\
	(d)->dec3_funcs[2] = dec3_filt_3ch_##type;\
	(d)->dec3_funcs[3] = dec3_filt_4ch_##type;\
	(d)->dec4_funcs[0] = dec4_filt_1ch_##type;\
	(d)->dec4_funcs[1] = dec4_filt_2ch_##type;\
	(d)->dec4_funcs[2] = dec4_filt_3ch_##type;\
	(d)->dec4_funcs[3] = dec4_filt_4ch_##type;\
} while(0)



static inline void select_arch(struct src_funcs *sfuncs, struct dec_funcs *dfuncs, const char** info)
{

	link_filters(sfuncs, dfuncs, info, default);
#if defined (__arm__)
#if defined (__ARM_NEON__)
	unsigned long cap = getauxval(AT_HWCAP);
	if (cap & HWCAP_NEON)
		link_filters(sfuncs, dfuncs, info, neon);
#endif
#elif defined (__aarch64__)
	#if defined (__ARM_NEON)
		unsigned long cap = getauxval(AT_HWCAP);
		if (cap & HWCAP_ASIMD)
			link_filters(sfuncs, dfuncs, info, asimd);
	#endif
#elif defined(__i386__) || defined(__x86_64__)
#ifdef USE_AVX
#if defined (__AVX__)
	if (__builtin_cpu_supports("avx"))
		link_filters(sfuncs, dfuncs, info, avx);
	else
#endif
#endif
#if defined (__SSE3__)
	if (__builtin_cpu_supports("sse3"))
		link_filters(sfuncs, dfuncs, info, sse);
#endif
#endif
}

