/**
 * \file: src_impl_avx.h
 *
 * AVX optimized SRC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __SRC_IMPL_AVX__
#define __SRC_IMPL_AVX__


#include <immintrin.h>

/*'samples' has highest channel first*/

static inline void src_filt_1ch_avx(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float *ipf)
{
	int i;
	__m256 _samp01234567;
	__m256 _coeff1_01234567;
	__m256 _coeff2_01234567;
	__m256 _tmp1;
	__m256 _tmp2;
	__m256 _sum_low = _mm256_setzero_ps();
	__m256 _sum_up = _mm256_setzero_ps();
	__m256i mask = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 1);
	__m256 _ipf = _mm256_set1_ps(*ipf);
	__m256 _fac2 = _mm256_set1_ps(2);

	/*each loop processes 8 frames*/
	for (i = 0; i < FILT_LEN_SRC; i += 8) {
		_samp01234567 = _mm256_loadu_ps(samples);
		samples += 8;
		_coeff1_01234567 = _mm256_load_ps(coeffs);
		coeffs += 8;
		_sum_low = _mm256_add_ps(_sum_low, _mm256_mul_ps(_samp01234567, _coeff1_01234567));

		_coeff2_01234567 = _mm256_load_ps(coeffs2);
		coeffs2 += 8;
		_sum_up = _mm256_add_ps(_sum_up, _mm256_mul_ps(_samp01234567, _coeff2_01234567));
	}
	_sum_low = _mm256_hadd_ps(_sum_low, _sum_low);	/*sum in 0:31, 32:63, 128:159, 160:191*/
	_sum_low = _mm256_hadd_ps(_sum_low, _sum_low);	/*sum in 0:31, 128:159*/
	_tmp1 = _mm256_permute2f128_ps(_sum_low, _sum_low, 0x01);/*copy of sum with Xch positions*/
	_sum_low = _mm256_add_ps(_sum_low, _tmp1);

	_sum_up = _mm256_hadd_ps(_sum_up, _sum_up);	/*sum in 0:31, 32:63, 128:159, 160:191*/
	_sum_up = _mm256_hadd_ps(_sum_up, _sum_up);	/*sum in 0:31, 128:159*/
	_tmp1 = _mm256_permute2f128_ps(_sum_up, _sum_up, 0x01);/*copy of sum with Xch positions*/
	_sum_up = _mm256_add_ps(_sum_up, _tmp1);

	/*for 1 ch this could be done more easily w/o vectors:*/
	/**res1 = (c1_low + (c1_up-c1_low) * *ipf)*2;*/
	_tmp1 = _mm256_sub_ps(_sum_up, _sum_low);
	_tmp2 = _mm256_mul_ps(_tmp1, _ipf);
	_tmp1 = _mm256_add_ps(_tmp2, _sum_low);
	_tmp1 = _mm256_mul_ps(_tmp1, _fac2);
	_mm256_maskstore_ps(res1, mask, _tmp1);
}

static inline void src_filt_2ch_avx(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float *ipf)
{
	int i;
	__m256 _samp0123;
	__m256 _samp4567;
	__m256 _coeff;
	__m256 _coeff1_0123;
	__m256 _coeff2_0123;
	__m256 _coeff1_4567;
	__m256 _coeff2_4567;
	__m256 _tmp1;
	__m256 _tmp2;
	__m256 _sum1_low = _mm256_setzero_ps();
	__m256 _sum1_up = _mm256_setzero_ps();
	__m256i mask = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 1, 1);
	__m256 _ipf = _mm256_set1_ps(*ipf);
	__m256 _fac2 = _mm256_set1_ps(2);

	/*#each loop processes 8 frames*/
	for (i = 0; i < FILT_LEN_SRC; i += 8) {
		_samp0123 = _mm256_loadu_ps(samples);
		samples += 8;
		_samp4567 = _mm256_loadu_ps(samples);
		samples += 8;
		_coeff = _mm256_load_ps(coeffs);
		coeffs += 8;
		_coeff1_0123 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff1_4567 = _mm256_unpackhi_ps(_coeff,_coeff);
		_coeff = _mm256_load_ps(coeffs2);
		coeffs2 += 8;
		_coeff2_0123 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff2_4567 = _mm256_unpackhi_ps(_coeff,_coeff);

		_sum1_low = _mm256_add_ps(_sum1_low, _mm256_mul_ps(_samp0123, _coeff1_0123));
		_sum1_low = _mm256_add_ps(_sum1_low, _mm256_mul_ps(_samp4567, _coeff1_4567));
		_sum1_up = _mm256_add_ps(_sum1_up, _mm256_mul_ps(_samp0123, _coeff2_0123));
		_sum1_up = _mm256_add_ps(_sum1_up, _mm256_mul_ps(_samp4567, _coeff2_4567));
	}
	_sum1_low = _mm256_permute_ps(_sum1_low, _MM_SHUFFLE(3, 1, 2, 0));
	_sum1_low = _mm256_hadd_ps(_sum1_low, _sum1_low);
	_sum1_low = _mm256_permute_ps(_sum1_low, _MM_SHUFFLE(3, 1, 2, 0));
	_sum1_low = _mm256_hadd_ps(_sum1_low, _sum1_low);

	_sum1_up = _mm256_permute_ps(_sum1_up, _MM_SHUFFLE(3, 1, 2, 0));
	_sum1_up = _mm256_hadd_ps(_sum1_up, _sum1_up);
	_sum1_up = _mm256_permute_ps(_sum1_up, _MM_SHUFFLE(3, 1, 2, 0));
	_sum1_up = _mm256_hadd_ps(_sum1_up, _sum1_up);

	_tmp1 = _mm256_sub_ps(_sum1_up, _sum1_low);
	_tmp2 = _mm256_mul_ps(_tmp1, _ipf);
	_tmp1 = _mm256_add_ps(_tmp2, _sum1_low);
	_tmp1 = _mm256_mul_ps(_tmp1, _fac2);

	_mm256_maskstore_ps(res1, mask,_tmp1);/*wrong channel order !!*/
}


static inline void src_filt_3ch_avx(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float *ipf)
{
	int i;
	__m256 _samp01;
	__m256 _samp23;
	__m256 _samp45;
	__m256 _samp67;
	__m256 _coeff;
	__m256 _coeff1_01;
	__m256 _coeff1_23;
	__m256 _coeff1_45;
	__m256 _coeff1_67;
	__m256 _coeff2_01;
	__m256 _coeff2_23;
	__m256 _coeff2_45;
	__m256 _coeff2_67;

	__m256 _sum_low = _mm256_setzero_ps();
	__m256 _sum_up = _mm256_setzero_ps();
	__m256 _tmp1;
	__m256 _tmp2;
	__m256i _mask = _mm256_set_epi32(0, 1, 1, 1, 0, 1, 1, 1);
	__m256 _ipf = _mm256_set1_ps(*ipf);
	__m256 _fac2 = _mm256_set1_ps(2);

	/*#each loop processes 8 frames*/
	for (i = 0; i < FILT_LEN_SRC; i += 8) {
		_samp01 = _mm256_maskload_ps(samples, _mask);
		samples += 6;
		_samp23 = _mm256_maskload_ps(samples, _mask);
		samples += 6;
		_samp45 = _mm256_maskload_ps(samples, _mask);
		samples += 6;
		_samp67 = _mm256_maskload_ps(samples, _mask);
		samples += 6;

		_coeff = _mm256_load_ps(coeffs);	/*=76543210*/
		coeffs += 8;
		_coeff1_01 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff1_01 = _mm256_unpacklo_ps(_coeff1_01,_coeff1_01);
		_coeff1_23 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff1_23 = _mm256_unpacklo_ps(_coeff1_23,_coeff1_23);
		_coeff1_45 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff1_45 = _mm256_unpacklo_ps(_coeff1_45,_coeff1_45);
		_coeff1_67 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff1_67 = _mm256_unpacklo_ps(_coeff1_67,_coeff1_67);

		_coeff = _mm256_load_ps(coeffs2);	/*=76543210*/
		coeffs2 += 8;
		_coeff2_01 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff2_01 = _mm256_unpacklo_ps(_coeff2_01,_coeff2_01);
		_coeff2_23 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff2_23 = _mm256_unpacklo_ps(_coeff2_23,_coeff2_23);
		_coeff2_45 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff2_45 = _mm256_unpacklo_ps(_coeff2_45,_coeff2_45);
		_coeff2_67 = _mm256_unpacklo_ps(_coeff,_coeff);
		_coeff2_67 = _mm256_unpacklo_ps(_coeff2_67,_coeff2_67);

		_sum_low = _mm256_add_ps(_sum_low, _mm256_mul_ps(_samp01, _coeff1_01));
		_sum_low = _mm256_add_ps(_sum_low, _mm256_mul_ps(_samp23, _coeff1_23));
		_sum_low = _mm256_add_ps(_sum_low, _mm256_mul_ps(_samp45, _coeff1_45));
		_sum_low = _mm256_add_ps(_sum_low, _mm256_mul_ps(_samp67, _coeff1_67));

		_sum_up = _mm256_add_ps(_sum_up, _mm256_mul_ps(_samp01, _coeff2_01));
		_sum_up = _mm256_add_ps(_sum_up, _mm256_mul_ps(_samp23, _coeff2_23));
		_sum_up = _mm256_add_ps(_sum_up, _mm256_mul_ps(_samp45, _coeff2_45));
		_sum_up = _mm256_add_ps(_sum_up, _mm256_mul_ps(_samp67, _coeff2_67));
	}
	_sum_low = _mm256_shuffle_ps(_sum_low, _sum_low, _MM_SHUFFLE(0, 1, 0, 1));
	_tmp1 =_mm256_permute2f128_ps(_sum_low, _sum_low, 0x01);/*high part of sum to low of sum1*/
	_sum_low = _mm256_add_ps(_sum_low, _tmp1);	/*only low 128bit relevant !*/

	_sum_up = _mm256_shuffle_ps(_sum_up, _sum_up, _MM_SHUFFLE(0, 1, 0, 1));
	_tmp1 =_mm256_permute2f128_ps(_sum_up, _sum_up, 0x01);/*high part of sum to low of sum1*/
	_sum_up = _mm256_add_ps(_sum_up, _tmp1);	/*only low 128bit relevant !*/

	_tmp1 = _mm256_sub_ps(_sum_up, _sum_low);
	_tmp2 = _mm256_mul_ps(_tmp1, _ipf);
	_tmp1 = _mm256_add_ps(_tmp2, _sum_low);
	_tmp1 = _mm256_mul_ps(_tmp1, _fac2);

	_mask = _mm256_set_epi32(0, 0, 0, 0, 0, 1, 1, 1);
	_mm256_maskstore_ps(res1, _mask, _tmp1);/*channel order!!*/
}

#define src_filt_3ch_avx SRC_NOT_IMPL

#endif /*SRC_IMPL_AVX*/
