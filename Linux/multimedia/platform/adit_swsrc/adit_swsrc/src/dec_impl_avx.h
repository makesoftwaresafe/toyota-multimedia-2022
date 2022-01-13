/**
 * \file: dec_impl_avx.h
 *
 * AVX optimized DEC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __DEC_IMPL_AVX_H__
#define __DEC_IMPL_AVX_H__

#include <immintrin.h>


#define decX_filt_1ch_avx(result, samples, coeffs, len)				\
{										\
	int i;									\
	__m256 _samp01234567;							\
	__m256 _coeff01234567;							\
	__m256 _tmp;								\
	__m256 _sum = _mm256_setzero_ps();					\
	__m256i mask = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 1);		\
	/*each loop processes 8 frames*/					\
	for (i = 0; i < len; i += 8) {						\
		_samp01234567 = _mm256_loadu_ps(samples);			\
		samples += 8;							\
		_coeff01234567 = _mm256_load_ps(coeffs);			\
		coeffs += 8;							\
		_sum = _mm256_add_ps(_sum, _mm256_mul_ps(_samp01234567, _coeff01234567));\
	}									\
	_sum = _mm256_hadd_ps(_sum, _sum);	/*sum in 0:31, 32:63, 128:159, 160:191*/\
	_sum = _mm256_hadd_ps(_sum, _sum);	/*sum in 0:31, 128:159*/	\
	_tmp = _mm256_permute2f128_ps(_sum, _sum, 0x01);/*copy of sum with Xch positions*/\
	_sum = _mm256_add_ps(_sum, _tmp);					\
	_mm256_maskstore_ps(result, mask, _sum);				\
}


/*
OK (change channel order!!)
*/
#define decX_filt_2ch_avx(result, samples, coeffs, len)				\
{										\
	int i;									\
	__m256 _samp0123;							\
	__m256 _samp4567;							\
	__m256 _coeff;								\
	__m256 _coeff0123;							\
	__m256 _coeff4567;							\
	__m256 _sum1 = _mm256_setzero_ps();					\
	__m256i mask = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 1, 1);		\
										\
	/*#each loop processes 8 frames*/					\
	for (i = 0; i < len; i += 8) {						\
		_samp0123 = _mm256_loadu_ps(samples);				\
		samples += 8;							\
		_samp4567 = _mm256_loadu_ps(samples);				\
		samples += 8;							\
		_coeff = _mm256_load_ps(coeffs);				\
		coeffs += 8;							\
		_coeff0123 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff4567 = _mm256_unpackhi_ps(_coeff,_coeff);			\
		_sum1 = _mm256_add_ps(_sum1, _mm256_mul_ps(_samp0123, _coeff0123));\
		_sum1 = _mm256_add_ps(_sum1, _mm256_mul_ps(_samp4567, _coeff4567));\
	}									\
	_sum1 = _mm256_permute_ps(_sum1, _MM_SHUFFLE(3, 1, 2, 0));		\
	_sum1 = _mm256_hadd_ps(_sum1, _sum1);					\
	_sum1 = _mm256_permute_ps(_sum1, _MM_SHUFFLE(3, 1, 2, 0));		\
	_sum1 = _mm256_hadd_ps(_sum1, _sum1);					\
	_mm256_maskstore_ps(result, mask, _sum1);/*wrong channel order !!*/	\
}

/*
3ch dec: 

OK(change channel order!!)
Derived from 4ch dec. -> For 3ch we use only 6 of 8 entries in vector.
Using 8 entries would results in much more complex shifting/reordering of samples + coefficients
*/
#define decX_filt_3ch_avx(result, samples, coeffs, len)				\
{										\
	int i;									\
	__m256 _samp01;								\
	__m256 _samp23;								\
	__m256 _samp45;								\
	__m256 _samp67;								\
	__m256 _coeff;								\
	__m256 _coeff01;							\
	__m256 _coeff23;							\
	__m256 _coeff45;							\
	__m256 _coeff67;							\
	__m256 _sum = _mm256_setzero_ps();					\
	__m256 _sum2;								\
	__m256i _mask = _mm256_set_epi32(0, 1, 1, 1, 0, 1, 1, 1);		\
	/*#each loop processes 8 frames*/					\
	for (i = 0; i < len; i += 8) {						\
		_samp01 = _mm256_maskload_ps(samples, _mask);			\
		samples += 6;							\
		_samp23 = _mm256_maskload_ps(samples, _mask);			\
		samples += 6;							\
		_samp45 = _mm256_maskload_ps(samples, _mask);			\
		samples += 6;							\
		_samp67 = _mm256_maskload_ps(samples, _mask);			\
		samples += 6;							\
		_coeff = _mm256_load_ps(coeffs);	/*=76543210*/		\
		coeffs += 8;							\
										\
		_coeff01 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff01 = _mm256_unpacklo_ps(_coeff01,_coeff01);		\
		_coeff23 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff23 = _mm256_unpacklo_ps(_coeff23,_coeff23);		\
		_coeff45 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff45 = _mm256_unpacklo_ps(_coeff45,_coeff45);		\
		_coeff67 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff67 = _mm256_unpacklo_ps(_coeff67,_coeff67);		\
										\
		_sum = _mm256_add_ps(_sum, _mm256_mul_ps(_samp01, _coeff01));	\
		_sum = _mm256_add_ps(_sum, _mm256_mul_ps(_samp23, _coeff23));	\
		_sum = _mm256_add_ps(_sum, _mm256_mul_ps(_samp45, _coeff45));	\
		_sum = _mm256_add_ps(_sum, _mm256_mul_ps(_samp67, _coeff67));	\
	}									\
	_sum = _mm256_shuffle_ps(_sum, _sum, _MM_SHUFFLE(0, 1, 0, 1));		\
	_sum2 =_mm256_permute2f128_ps(_sum, _sum, 0x01);/*high part of sum to low of sum1*/	\
	_sum = _mm256_add_ps(_sum, _sum2);	/*only low 128bit relevant !*/	\
	_mask = _mm256_set_epi32(0,0,0,0,0,1,1,1);				\
	_mm256_maskstore_ps(result, _mask, _sum);/*channel order!!*/		\
}


/*
4ch dec: 
OK (change channel order!!)
*/

#define decX_filt_4ch_avx(result, samples, coeffs, len)				\
{										\
	int i;									\
	__m256 _samp01;								\
	__m256 _samp23;								\
	__m256 _samp45;								\
	__m256 _samp67;								\
	__m256 _coeff;								\
	__m256 _coeff01;							\
	__m256 _coeff23;							\
	__m256 _coeff45;							\
	__m256 _coeff67;							\
	__m256 _sum = _mm256_setzero_ps();					\
	__m256 _sum2;								\
	__m256i mask = _mm256_set_epi32(0, 0, 0, 0, 1, 1, 1, 1);		\
	/*each loop processes 8 frames*/					\
	for (i = 0; i < len; i += 8) {						\
		_samp01 = _mm256_loadu_ps(samples);				\
		samples += 8;							\
		_samp23 = _mm256_loadu_ps(samples);				\
		samples += 8;							\
		_samp45 = _mm256_loadu_ps(samples);				\
		samples += 8;							\
		_samp67 = _mm256_loadu_ps(samples);				\
		samples += 8;							\
		_coeff = _mm256_load_ps(coeffs);				\
		coeffs += 8;							\
										\
		_coeff01 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff01 = _mm256_unpacklo_ps(_coeff01,_coeff01);		\
		_coeff23 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff23 = _mm256_unpacklo_ps(_coeff23,_coeff23);		\
		_coeff45 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff45 = _mm256_unpacklo_ps(_coeff45,_coeff45);		\
		_coeff67 = _mm256_unpacklo_ps(_coeff,_coeff);			\
		_coeff67 = _mm256_unpacklo_ps(_coeff67,_coeff67);		\
										\
		_sum += _mm256_add_ps(_mm256_mul_ps(_samp01, _coeff01), _mm256_mul_ps(_samp23, _coeff23));\
		_sum += _mm256_add_ps(_mm256_mul_ps(_samp45, _coeff45), _mm256_mul_ps(_samp67, _coeff67));\
	}									\
	_sum = _mm256_shuffle_ps(_sum, _sum, _MM_SHUFFLE(0, 1, 0, 1));		\
	_sum2 =_mm256_permute2f128_ps(_sum, _sum, 0x01);/*high part of sum to low of sum1*/	\
	_sum = _mm256_add_ps(_sum, _sum2);	/*only low 128bit relevant !*/	\
	_mm256_maskstore_ps(result, mask, _sum);/*channel order!!*/				\
}

/*'samples' has highest channel first*/

static inline void dec2_filt_1ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_avx(result, samples, coeffs, FILT_LEN_DEC2);
}

static inline void dec2_filt_2ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_avx(result, samples, coeffs, FILT_LEN_DEC2);
}

static inline void dec2_filt_3ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_avx(result, samples, coeffs, FILT_LEN_DEC2);
}

static inline void dec2_filt_4ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_avx(result, samples, coeffs, FILT_LEN_DEC2);
}


static inline void dec3_filt_1ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_avx(result, samples, coeffs, FILT_LEN_DEC3);
}

static inline void dec3_filt_2ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_avx(result, samples, coeffs, FILT_LEN_DEC3);
}

static inline void dec3_filt_3ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_avx(result, samples, coeffs, FILT_LEN_DEC3);
}

static inline void dec3_filt_4ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_avx(result, samples, coeffs, FILT_LEN_DEC3);
}


static inline void dec4_filt_1ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_1ch_avx(result, samples, coeffs, FILT_LEN_DEC4);
}

static inline void dec4_filt_2ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_2ch_avx(result, samples, coeffs, FILT_LEN_DEC4);
}

static inline void dec4_filt_3ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_3ch_avx(result, samples, coeffs, FILT_LEN_DEC4);
}

static inline void dec4_filt_4ch_avx(float *result, const float *samples, const float *coeffs)
{
	decX_filt_4ch_avx(result, samples, coeffs, FILT_LEN_DEC4);
}

#endif /*__DEC_IMPL_AVX_H__*/
