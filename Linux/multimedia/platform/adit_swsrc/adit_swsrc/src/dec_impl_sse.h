/**
 * \file: dec_impl_sse.h
 *
 * SSE optimized DEC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __DEC_IMPL_SSE_H__
#define __DEC_IMPL_SSE_H__


#include <xmmintrin.h>     //SSE
#include <emmintrin.h>     //SSE2
#include <pmmintrin.h>     //SSE3
//#include <tmmintrin.h>     //SSSE3


/*
1ch dec: 4samples + coefficients in each vector

s0_1		c0		sum_1
s1_1	*	c1	->	sum_1	->	sum
s2_1		c2		sum_1
s3_1		c3		sum_1
*/

#define decX_filt_1ch_sse(result, samples, coeffs, len)				\
{										\
	int i;									\
	__m128 _samp0123;							\
	__m128 _coeff0123;							\
	__m128 _dummy = _mm_set_ps(0, 0, 0, 0);					\
	__m128 _sum = _mm_set_ps(0, 0, 0, 0);					\
	/*each loop processes 4 frames*/					\
	for (i = 0; i < len; i += 4) {						\
		_samp0123 = _mm_loadu_ps(samples);				\
		samples += 4;							\
		_coeff0123 = _mm_load_ps(coeffs);				\
		coeffs += 4;							\
		_sum = _mm_add_ps(_sum, _mm_mul_ps(_samp0123, _coeff0123));	\
	}									\
	_sum = _mm_hadd_ps(_sum, _dummy);					\
	_sum = _mm_hadd_ps(_sum, _dummy);					\
	_mm_store_ss(result, _sum);						\
}


/*
2ch dec: 

s0_1		c0		sum_1
s0_2	*	c0	->	sum_2
s1_1		c1		sum_1
s1_2		c1		sum_2		sum_1
					-> 	sum_2	->sum_1, sum2
s2_1		c2		sum_1		sum_1
s2_2	*	c2	->	sum_2		sum_2
s3_1		c3		sum_1
s3_2		c3		sum_2
*/
#define decX_filt_2ch_sse(result, samples, coeffs, len)				\
{										\
	int i;									\
	__m128 _samp01;								\
	__m128 _samp23;								\
	__m128 _coeff;								\
	__m128 _coeff01;							\
	__m128 _coeff23;							\
	__m128 _dummy =_mm_set_ps(0, 0, 0, 0);					\
	__m128 _sum1 = _mm_set_ps(0, 0, 0, 0);					\
	__m128 _sum2 = _mm_set_ps(0, 0, 0, 0);					\
										\
	/*#each loop processes 4 frames*/					\
	for (i = 0; i < len; i += 4) {						\
		_samp01 = _mm_loadu_ps(samples);				\
		samples += 4;							\
		_samp23 = _mm_loadu_ps(samples);				\
		samples += 4;							\
		_coeff = _mm_load_ps(coeffs);					\
		coeffs += 4;							\
		_coeff = _mm_shuffle_ps(_coeff, _coeff, _MM_SHUFFLE(3, 1, 2, 0));\
		_coeff01 = _mm_moveldup_ps(_coeff);				\
		_coeff23 = _mm_movehdup_ps(_coeff);				\
										\
		_sum1 = _mm_add_ps(_sum1, _mm_mul_ps(_samp01, _coeff01));	\
		_sum2 = _mm_add_ps(_sum2, _mm_mul_ps(_samp23, _coeff23));	\
	}									\
										\
	_sum1 = _mm_add_ps(_sum1, _sum2);					\
	_sum1 = _mm_shuffle_ps(_sum1, _sum1, _MM_SHUFFLE(3, 1, 2, 0));		\
	_sum1 = _mm_hadd_ps(_sum1, _dummy);					\
	_mm_store_ss(result + 1, _sum1);					\
	_sum1 = _mm_shuffle_ps(_sum1, _sum1, _MM_SHUFFLE(0, 3, 2, 1));		\
	_mm_store_ss(result, _sum1);						\
}

/*
3ch dec: 

s0_1		c0		sum_1		sum_1
s0_2	*	c0	->	sum_2		sum_1
s0_3		c0		sum_3		sum_1
s1_1		c1		sum_1		sum_1

s1_2		c1		sum_2		sum_2
s1_3	*	c1	->	sum_3	->	sum_2	->	sum1, sum2, sum3
s2_1		c2		sum_1		sum_2
s2_2		c2		sum_2		sum_2

s2_3		c2		sum_3		sum_3
s3_1	*	c3	->	sum_1		sum_3
s3_2		c3		sum_2		sum_3
s3_3		c3		sum_3		sum_3
*/
#define decX_filt_3ch_sse(result, samples, coeffs, len)				\
{										\
	int i;									\
	__m128 _samp01;								\
	__m128 _samp12;								\
	__m128 _samp23;								\
	__m128 _coeff;								\
	__m128 _coeff01;							\
	__m128 _coeff12;							\
	__m128 _coeff23;							\
	__m128 _dummy =_mm_set_ps(0, 0, 0, 0);					\
	__m128 _sum1 = _mm_set_ps(0, 0, 0, 0);					\
	__m128 _sum2 = _mm_set_ps(0, 0, 0, 0);					\
	__m128 _sum3 = _mm_set_ps(0, 0, 0, 0);					\
	__m128 _sum1_;								\
	__m128 _sum2_;								\
	__m128 _sum3_;								\
	/*#each loop processes 4 frames*/					\
	for (i = 0; i < len; i += 4) {						\
		_samp01 = _mm_loadu_ps(samples);				\
		samples += 4;							\
		_samp12 = _mm_loadu_ps(samples);				\
		samples += 4;							\
		_samp23 = _mm_loadu_ps(samples);				\
		samples += 4;							\
		_coeff = _mm_load_ps(coeffs);					\
		coeffs += 4;							\
		_coeff01 = _mm_shuffle_ps(_coeff, _coeff, _MM_SHUFFLE(1, 0, 0, 0));\
		_coeff12 = _mm_shuffle_ps(_coeff, _coeff, _MM_SHUFFLE(2, 2, 1, 1));\
		_coeff23 = _mm_shuffle_ps(_coeff, _coeff, _MM_SHUFFLE(3, 3, 3, 2));\
										\
		_sum1 = _mm_add_ps(_sum1, _mm_mul_ps(_samp01, _coeff01));	\
		_sum2 = _mm_add_ps(_sum2, _mm_mul_ps(_samp12, _coeff12));	\
		_sum3 = _mm_add_ps(_sum3, _mm_mul_ps(_samp23, _coeff23));	\
	}									\
	/*collect ch1 to _sum1_*/						\
	_sum1 = _mm_shuffle_ps(_sum1, _sum1, _MM_SHUFFLE(2, 1, 3, 0));/*1 3 2 1 -> 3 2 1 1 */\
	_sum1_ = _mm_shuffle_ps(_sum2, _sum3, _MM_SHUFFLE(1, 0, 3, 2));/*1 3 2 1*/\
	_sum1_ = _mm_shuffle_ps(_sum1, _sum1_, _MM_SHUFFLE(3, 0, 1, 0));/*1 1 1 1*/\
	/*collect ch2 to _sum2_*/						\
	_sum2 = _mm_shuffle_ps(_sum2, _sum2, _MM_SHUFFLE(2, 1, 3, 0));/*2 1 3 2 -> 1 3 2 2 */\
	_sum2_ = _mm_shuffle_ps(_sum1, _sum3, _MM_SHUFFLE(3, 2, 3, 2));/*3 2 3 2 */\
	_sum2_ = _mm_shuffle_ps(_sum2, _sum2_, _MM_SHUFFLE(2, 0, 1, 0));/*2 2 2 2 */\
	/*collect ch3 to _sum3_*/						\
	_sum3 = _mm_shuffle_ps(_sum3, _sum3, _MM_SHUFFLE(2, 1, 3, 0));/*3 2 1 3 -> 2 1 3 3 */\
	_sum3_ = _mm_shuffle_ps(_sum1, _sum2, _MM_SHUFFLE(3, 2, 3, 2));/*1 3 3 2*/\
	_sum3_ = _mm_shuffle_ps(_sum3, _sum3_, _MM_SHUFFLE(2, 1, 1, 0));/*3 3 3 3*/\
	/*TODO: optimize w/o _dummy by doing hadd in form 11 22 X 33 11, etc ??*/\
	_sum1_ = _mm_hadd_ps(_sum1_, _dummy);					\
	_sum1_ = _mm_hadd_ps(_sum1_, _dummy);					\
	_mm_store_ss(result + 2, _sum1_);					\
	_sum2_ = _mm_hadd_ps(_sum2_, _dummy);					\
	_sum2_ = _mm_hadd_ps(_sum2_, _dummy);					\
	_mm_store_ss(result + 1, _sum2_);					\
	_sum3_ = _mm_hadd_ps(_sum3_, _dummy);					\
	_sum3_ = _mm_hadd_ps(_sum3_, _dummy);					\
	_mm_store_ss(result, _sum3_);						\
}


/*
4ch dec: 

s0_1		c0		sum_1
s0_2	*	c0	->	sum_2
s0_3		c0		sum_3
s0_4		c0		sum_4

s1_1		c1		sum_1
s1_2	*	c1	->	sum_2
s1_3		c1		sum_3
s1_4		c1		sum_4			sum_1
						->	sum_2
s2_1		c2		sum_1			sum_3
s2_2	*	c2	->	sum_2			sum_4
s2_3		c2		sum_3
s2_4		c2		sum_4

s3_1		c3		sum_1
s3_2	*	c3	->	sum_2
s3_3		c3		sum_3
s3_4		c3		sum_4
*/

#define decX_filt_4ch_sse(result, samples, coeffs, len)				\
{										\
	int i;									\
	__m128 _samp0;								\
	__m128 _samp1;								\
	__m128 _samp2;								\
	__m128 _samp3;								\
	__m128 _coeff;								\
	__m128 _coeff0;								\
	__m128 _coeff1;								\
	__m128 _coeff2;								\
	__m128 _coeff3;								\
	__m128 _sum = _mm_set_ps(0, 0, 0, 0);					\
	/*each loop processes 4 frames*/					\
	for (i = 0; i < len; i += 4) {						\
		_samp0 = _mm_loadu_ps(samples);					\
		samples += 4;							\
		_samp1 = _mm_loadu_ps(samples);					\
		samples += 4;							\
		_samp2 = _mm_loadu_ps(samples);					\
		samples += 4;							\
		_samp3 = _mm_loadu_ps(samples);					\
		samples += 4;							\
		_coeff = _mm_load_ps(coeffs);					\
		coeffs += 4;							\
										\
		_coeff0 = _mm_shuffle_ps(_coeff, _coeff, _MM_SHUFFLE(0, 0, 0, 0));\
		_coeff1 = _mm_shuffle_ps(_coeff, _coeff, _MM_SHUFFLE(1, 1, 1, 1));\
		_coeff2 = _mm_shuffle_ps(_coeff, _coeff, _MM_SHUFFLE(2, 2, 2, 2));\
		_coeff3 = _mm_shuffle_ps(_coeff, _coeff, _MM_SHUFFLE(3, 3, 3, 3));\
										\
		_sum = _mm_add_ps(_sum, _mm_add_ps(_mm_mul_ps(_samp0, _coeff0), _mm_mul_ps(_samp1, _coeff1)));\
		_sum = _mm_add_ps(_sum, _mm_add_ps(_mm_mul_ps(_samp2, _coeff2), _mm_mul_ps(_samp3, _coeff3)));\
	}									\
	_sum = _mm_shuffle_ps(_sum, _sum, _MM_SHUFFLE(0, 1, 2, 3));		\
	_mm_storeu_ps(result, _sum);						\
}

/*'samples' has highest channel first*/

static inline void dec2_filt_1ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_sse(result, samples, coeffs, FILT_LEN_DEC2);
}

static inline void dec2_filt_2ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_sse(result, samples, coeffs, FILT_LEN_DEC2);
}

static inline void dec2_filt_3ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_sse(result, samples, coeffs, FILT_LEN_DEC2);
}

static inline void dec2_filt_4ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_sse(result, samples, coeffs, FILT_LEN_DEC2);
}


static inline void dec3_filt_1ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_sse(result, samples, coeffs, FILT_LEN_DEC3);
}

static inline void dec3_filt_2ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_sse(result, samples, coeffs, FILT_LEN_DEC3);
}

static inline void dec3_filt_3ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_sse(result, samples, coeffs, FILT_LEN_DEC3);
}

static inline void dec3_filt_4ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_sse(result, samples, coeffs, FILT_LEN_DEC3);
}


static inline void dec4_filt_1ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_sse(result, samples, coeffs, FILT_LEN_DEC4);
}

static inline void dec4_filt_2ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_sse(result, samples, coeffs, FILT_LEN_DEC4);
}

static inline void dec4_filt_3ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_sse(result, samples, coeffs, FILT_LEN_DEC4);
}

static inline void dec4_filt_4ch_sse(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_sse(result, samples, coeffs, FILT_LEN_DEC4);
}

#endif /*__DEC_IMPL_SSE_H__*/
