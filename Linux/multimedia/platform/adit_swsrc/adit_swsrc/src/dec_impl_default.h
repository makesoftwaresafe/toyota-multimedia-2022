/**
 * \file: dec_impl_default.h
 *
 * Non optimized DEC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __DEC_IMPL_DEFAULT_H__
#define __DEC_IMPL_DEFAULT_H__



#define decX_filt_1ch_default(result, samples, coeffs, len) \
{\
	float c1 = 0;\
	int i;\
	for (i = 0; i < len; i++) {\
		c1 += *samples * *coeffs;\
		samples++;\
		coeffs++;\
	}\
	*result = c1;\
}

#define decX_filt_2ch_default(result, samples, coeffs, len)\
{\
	float c1 = 0;\
	float c2 = 0;\
	int i;\
	for (i = 0; i < len; i++) {\
		c1 += *samples * *coeffs;\
		samples++;\
		c2 += *samples * *coeffs;\
		samples++;\
		coeffs++;\
	}\
	*result = c2;\
	result++;\
	*result = c1;\
}

#define decX_filt_3ch_default(result, samples, coeffs, len)\
{\
	float c1 = 0;\
	float c2 = 0;\
	float c3 = 0;\
	int i;\
	for (i = 0; i < len; i++) {\
		c1 += *samples * *coeffs;\
		samples++;\
		c2 += *samples * *coeffs;\
		samples++;\
		c3 += *samples * *coeffs;\
		samples++;\
		coeffs++;\
	}\
	*result = c3;\
	result++;\
	*result = c2;\
	result++;\
	*result = c1;\
}

#define decX_filt_4ch_default(result, samples, coeffs, len)\
{\
	float c1 = 0;\
	float c2 = 0;\
	float c3 = 0;\
	float c4 = 0;\
	int i;\
	for (i = 0; i < len; i++) {\
		c1 += *samples * *coeffs;\
		samples++;\
		c2 += *samples * *coeffs;\
		samples++;\
		c3 += *samples * *coeffs;\
		samples++;\
		c4 += *samples * *coeffs;\
		samples++;\
		coeffs++;\
	}\
	*result = c4;\
	result++;\
	*result = c3;\
	result++;\
	*result = c2;\
	result++;\
	*result = c1;\
}

#define FILT_ALIGN_DEFAULT 1

static inline void dec2_filt_1ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_default(result, samples, coeffs, FILT_LEN_DEC(2,FILT_ALIGN_DEFAULT));
}

static inline void dec2_filt_2ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_default(result, samples, coeffs, FILT_LEN_DEC(2,FILT_ALIGN_DEFAULT));
}

static inline void dec2_filt_3ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_default(result, samples, coeffs, FILT_LEN_DEC(2,FILT_ALIGN_DEFAULT));
}

static inline void dec2_filt_4ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_default(result, samples, coeffs, FILT_LEN_DEC(2,FILT_ALIGN_DEFAULT));
}


static inline void dec3_filt_1ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_default(result, samples, coeffs, FILT_LEN_DEC(3,FILT_ALIGN_DEFAULT));
}

static inline void dec3_filt_2ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_default(result, samples, coeffs, FILT_LEN_DEC(3,FILT_ALIGN_DEFAULT));
}

static inline void dec3_filt_3ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_default(result, samples, coeffs, FILT_LEN_DEC(3,FILT_ALIGN_DEFAULT));
}

static inline void dec3_filt_4ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_default(result, samples, coeffs, FILT_LEN_DEC(3,FILT_ALIGN_DEFAULT));
}



static inline void dec4_filt_1ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_default(result, samples, coeffs, FILT_LEN_DEC(4,FILT_ALIGN_DEFAULT));
}

static inline void dec4_filt_2ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_default(result, samples, coeffs, FILT_LEN_DEC(4,FILT_ALIGN_DEFAULT));
}

static inline void dec4_filt_3ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_default(result, samples, coeffs, FILT_LEN_DEC(4,FILT_ALIGN_DEFAULT));
}

static inline void dec4_filt_4ch_default(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_default(result, samples, coeffs, FILT_LEN_DEC(4,FILT_ALIGN_DEFAULT));
}

#endif /*__DEC_IMPL_DEFAULT_H__*/
