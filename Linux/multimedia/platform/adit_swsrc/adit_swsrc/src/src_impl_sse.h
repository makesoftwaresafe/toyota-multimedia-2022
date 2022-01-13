/**
 * \file: src_impl_sse.h
 *
 * SSE optimized SRC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __SRC_IMPL_SSE__
#define __SRC_IMPL_SSE__


#include <xmmintrin.h>     //SSE
#include <emmintrin.h>     //SSE2
#include <pmmintrin.h>     //SSE3
//#include <tmmintrin.h>     //SSSE3

/*'samples' has highest channel first*/

static inline void src_filt_1ch_sse(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float *ipf)
{
	__m128 _samp0123;
	__m128 _coeff1_0123;
	__m128 _coeff2_0123;
	__m128 _sum_low = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum_up = _mm_set_ps(0, 0, 0, 0);
	__m128 _dummy = _mm_set_ps(0, 0, 0, 0);
	__m128 _tmp1;
	__m128 _tmp2;
	__m128 _ipf = _mm_set_ps(*ipf, *ipf, *ipf, *ipf);
	__m128 _fac2 = _mm_set_ps(2, 2, 2, 2);
	int i;
	for (i = 0; i < FILT_LEN_SRC; i += 4) {
		_samp0123 = _mm_loadu_ps(samples);
		samples += 4;
		_coeff1_0123 = _mm_load_ps(coeffs);
		coeffs += 4;
		_coeff2_0123 = _mm_load_ps(coeffs2);
		coeffs2 += 4;
		_sum_low = _mm_add_ps(_sum_low, _mm_mul_ps(_samp0123, _coeff1_0123));
		_sum_up = _mm_add_ps(_sum_up, _mm_mul_ps(_samp0123, _coeff2_0123));
	}

	_sum_low = _mm_hadd_ps(_sum_low, _dummy);
	_sum_low = _mm_hadd_ps(_sum_low, _dummy);

	_sum_up = _mm_hadd_ps(_sum_up, _dummy);
	_sum_up = _mm_hadd_ps(_sum_up, _dummy);

	/*for 1 ch this could be done more easily w/o vectors:*/
	/**res1 = (c1_low + (c1_up-c1_low) * *ipf)*2;*/
	_tmp1 = _mm_sub_ps(_sum_up, _sum_low);
	_tmp2 = _mm_mul_ps(_tmp1, _ipf);
	_tmp1 = _mm_add_ps(_tmp2, _sum_low);
	_tmp1 = _mm_mul_ps(_tmp1, _fac2);
	_mm_store_ss(res1, _tmp1);
}

static inline void src_filt_2ch_sse(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float *ipf)
{
	__m128 _samp01;
	__m128 _samp23;
	__m128 _coeff1;
	__m128 _coeff2;
	__m128 _coeff1_01;
	__m128 _coeff1_23;
	__m128 _coeff2_01;
	__m128 _coeff2_23;
	__m128 _dummy =_mm_set_ps(0, 0, 0, 0);
	__m128 _sum1_low = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum2_low = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum1_up = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum2_up = _mm_set_ps(0, 0, 0, 0);
 	__m128 _tmp1;
	__m128 _tmp2;
	__m128 _ipf = _mm_set_ps(*ipf, *ipf, *ipf, *ipf);
	__m128 _fac2 = _mm_set_ps(2, 2, 2, 2);

	int i;
	for (i = 0; i < FILT_LEN_SRC; i += 4) {
		_samp01 = _mm_loadu_ps(samples);
		samples += 4;
		_samp23 = _mm_loadu_ps(samples);
		samples += 4;
		_coeff1 = _mm_load_ps(coeffs);
		coeffs += 4;
		_coeff2 = _mm_load_ps(coeffs2);
		coeffs2 += 4;

		_coeff1 = _mm_shuffle_ps(_coeff1, _coeff1, _MM_SHUFFLE(3, 1, 2, 0));
		_coeff1_01 = _mm_moveldup_ps(_coeff1);
		_coeff1_23 = _mm_movehdup_ps(_coeff1);

		_coeff2 = _mm_shuffle_ps(_coeff2,_coeff2, _MM_SHUFFLE(3, 1, 2, 0));
		_coeff2_01 = _mm_moveldup_ps(_coeff2);
		_coeff2_23 = _mm_movehdup_ps(_coeff2);

		_sum1_low = _mm_add_ps(_sum1_low, _mm_mul_ps(_samp01, _coeff1_01));
		_sum2_low = _mm_add_ps(_sum2_low, _mm_mul_ps(_samp23, _coeff1_23));

		_sum1_up = _mm_add_ps(_sum1_up, _mm_mul_ps(_samp01, _coeff2_01));
		_sum2_up = _mm_add_ps(_sum2_up, _mm_mul_ps(_samp23, _coeff2_23));
	}
	_sum1_low = _mm_add_ps(_sum1_low, _sum2_low);
	_sum1_low = _mm_shuffle_ps(_sum1_low, _sum1_low, _MM_SHUFFLE(3, 1, 2, 0));
	_sum1_low = _mm_hadd_ps(_sum1_low, _dummy);

	_sum1_up = _mm_add_ps(_sum1_up, _sum2_up);
	_sum1_up = _mm_shuffle_ps(_sum1_up, _sum1_up, _MM_SHUFFLE(3, 1, 2, 0));
	_sum1_up = _mm_hadd_ps(_sum1_up, _dummy);

	_tmp1 = _mm_sub_ps(_sum1_up, _sum1_low);
	_tmp2 = _mm_mul_ps(_tmp1, _ipf);
	_tmp1 = _mm_add_ps(_tmp2, _sum1_low);
	_tmp1 = _mm_mul_ps(_tmp1, _fac2);
	_mm_store_ss(res1 + 1, _tmp1);
	_tmp1 = _mm_shuffle_ps(_tmp1, _tmp1, _MM_SHUFFLE(0, 3, 2, 1));
	_mm_store_ss(res1, _tmp1);
}


static inline void src_filt_3ch_sse(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float *ipf)
{
	__m128 _samp01;
	__m128 _samp12;
	__m128 _samp23;
	__m128 _coeff1;
	__m128 _coeff2;
	__m128 _coeff1_01;
	__m128 _coeff1_12;
	__m128 _coeff1_23;
	__m128 _coeff2_01;
	__m128 _coeff2_12;
	__m128 _coeff2_23;
	__m128 _dummy =_mm_set_ps(0, 0, 0, 0);
	__m128 _sum1_low = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum2_low = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum3_low = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum1_up = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum2_up = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum3_up = _mm_set_ps(0, 0, 0, 0);
	__m128 _sum1_low_;
	__m128 _sum2_low_;
	__m128 _sum3_low_;
	__m128 _sum1_up_;
	__m128 _sum2_up_;
	__m128 _sum3_up_;
	__m128 _tmp1;
	__m128 _tmp2;
	__m128 _ipf = _mm_set_ps(*ipf, *ipf, *ipf, *ipf);
	__m128 _fac2 = _mm_set_ps(2, 2, 2, 2);

	int i;
	for (i = 0; i < FILT_LEN_SRC; i += 4) {
		_samp01 = _mm_loadu_ps(samples);
		samples += 4;
		_samp12 = _mm_loadu_ps(samples);
		samples += 4;
		_samp23 = _mm_loadu_ps(samples);
		samples += 4;
		_coeff1 = _mm_load_ps(coeffs);
		coeffs += 4;
		_coeff2 = _mm_load_ps(coeffs2);
		coeffs2 += 4;

		_coeff1_01 = _mm_shuffle_ps(_coeff1, _coeff1, _MM_SHUFFLE(1, 0, 0, 0));
		_coeff1_12 = _mm_shuffle_ps(_coeff1, _coeff1, _MM_SHUFFLE(2, 2, 1, 1));
		_coeff1_23 = _mm_shuffle_ps(_coeff1, _coeff1, _MM_SHUFFLE(3, 3, 3, 2));
		_coeff2_01 = _mm_shuffle_ps(_coeff2, _coeff2, _MM_SHUFFLE(1, 0, 0, 0));
		_coeff2_12 = _mm_shuffle_ps(_coeff2, _coeff2, _MM_SHUFFLE(2, 2, 1, 1));
		_coeff2_23 = _mm_shuffle_ps(_coeff2, _coeff2, _MM_SHUFFLE(3, 3, 3, 2));

		_sum1_low = _mm_add_ps(_sum1_low, _mm_mul_ps(_samp01, _coeff1_01));
		_sum2_low = _mm_add_ps(_sum2_low, _mm_mul_ps(_samp12, _coeff1_12));
		_sum3_low = _mm_add_ps(_sum3_low, _mm_mul_ps(_samp23, _coeff1_23));

		_sum1_up = _mm_add_ps(_sum1_up, _mm_mul_ps(_samp01, _coeff2_01));
		_sum2_up = _mm_add_ps(_sum2_up, _mm_mul_ps(_samp12, _coeff2_12));
		_sum3_up = _mm_add_ps(_sum3_up, _mm_mul_ps(_samp23, _coeff2_23));
	}


	/*collect ch1 to _sum1_*/
	_sum1_low = _mm_shuffle_ps(_sum1_low, _sum1_low, _MM_SHUFFLE(2, 1, 3, 0));/*1 3 2 1 -> 3 2 1 1 */
	_sum1_low_ = _mm_shuffle_ps(_sum2_low, _sum3_low, _MM_SHUFFLE(1, 0, 3, 2));/*1 3 2 1*/
	_sum1_low_ = _mm_shuffle_ps(_sum1_low, _sum1_low_, _MM_SHUFFLE(3, 0, 1, 0));/*1 1 1 1*/

	_sum1_up = _mm_shuffle_ps(_sum1_up, _sum1_up, _MM_SHUFFLE(2, 1, 3, 0));/*1 3 2 1 -> 3 2 1 1 */
	_sum1_up_ = _mm_shuffle_ps(_sum2_up, _sum3_up, _MM_SHUFFLE(1, 0, 3, 2));/*1 3 2 1*/
	_sum1_up_ = _mm_shuffle_ps(_sum1_up, _sum1_up_, _MM_SHUFFLE(3, 0, 1, 0));/*1 1 1 1*/

	/*collect ch2 to _sum2_*/
	_sum2_low = _mm_shuffle_ps(_sum2_low, _sum2_low, _MM_SHUFFLE(2, 1, 3, 0));/*2 1 3 2 -> 1 3 2 2 */
	_sum2_low_ = _mm_shuffle_ps(_sum1_low, _sum3_low, _MM_SHUFFLE(3, 2, 3, 2));/*3 2 3 2 */
	_sum2_low_ = _mm_shuffle_ps(_sum2_low, _sum2_low_, _MM_SHUFFLE(2, 0, 1, 0));/*2 2 2 2 */

	_sum2_up = _mm_shuffle_ps(_sum2_up, _sum2_up, _MM_SHUFFLE(2, 1, 3, 0));/*2 1 3 2 -> 1 3 2 2 */
	_sum2_up_ = _mm_shuffle_ps(_sum1_up, _sum3_up, _MM_SHUFFLE(3, 2, 3, 2));/*3 2 3 2 */
	_sum2_up_ = _mm_shuffle_ps(_sum2_up, _sum2_up_, _MM_SHUFFLE(2, 0, 1, 0));/*2 2 2 2 */

	/*collect ch3 to _sum3_*/
	_sum3_low = _mm_shuffle_ps(_sum3_low, _sum3_low, _MM_SHUFFLE(2, 1, 3, 0));/*3 2 1 3 -> 2 1 3 3 */
	_sum3_low_ = _mm_shuffle_ps(_sum1_low, _sum2_low, _MM_SHUFFLE(3, 2, 3, 2));/*1 3 3 2*/
	_sum3_low_ = _mm_shuffle_ps(_sum3_low, _sum3_low_, _MM_SHUFFLE(2, 1, 1, 0));/*3 3 3 3*/

	_sum3_up = _mm_shuffle_ps(_sum3_up, _sum3_up, _MM_SHUFFLE(2, 1, 3, 0));/*3 2 1 3 -> 2 1 3 3 */
	_sum3_up_ = _mm_shuffle_ps(_sum1_up, _sum2_up, _MM_SHUFFLE(3, 2, 3, 2));/*1 3 3 2*/
	_sum3_up_ = _mm_shuffle_ps(_sum3_up, _sum3_up_, _MM_SHUFFLE(2, 1, 1, 0));/*3 3 3 3*/

	//collapse vectors
	_sum1_low_ = _mm_hadd_ps(_sum1_low_, _dummy);
	_sum1_low_ = _mm_hadd_ps(_sum1_low_, _dummy);
	_sum1_up_ = _mm_hadd_ps(_sum1_up_, _dummy);
	_sum1_up_ = _mm_hadd_ps(_sum1_up_, _dummy);

	_sum2_low_ = _mm_hadd_ps(_sum2_low_, _dummy);
	_sum2_low_ = _mm_hadd_ps(_sum2_low_, _dummy);
	_sum2_up_ = _mm_hadd_ps(_sum2_up_, _dummy);
	_sum2_up_ = _mm_hadd_ps(_sum2_up_, _dummy);

	_sum3_low_ = _mm_hadd_ps(_sum3_low_, _dummy);
	_sum3_low_ = _mm_hadd_ps(_sum3_low_, _dummy);
	_sum3_up_ = _mm_hadd_ps(_sum3_up_, _dummy);
	_sum3_up_ = _mm_hadd_ps(_sum3_up_, _dummy);

	//interpolate
	_tmp1 = _mm_sub_ps(_sum3_up_, _sum3_low_);
	_tmp2 = _mm_mul_ps(_tmp1, _ipf);
	_tmp1 = _mm_add_ps(_tmp2, _sum3_low_);
	_tmp1 = _mm_mul_ps(_tmp1, _fac2);
	_mm_store_ss(res1++, _tmp1);

	_tmp1 = _mm_sub_ps(_sum2_up_, _sum2_low_);
	_tmp2 = _mm_mul_ps(_tmp1, _ipf);
	_tmp1 = _mm_add_ps(_tmp2, _sum2_low_);
	_tmp1 = _mm_mul_ps(_tmp1, _fac2);
	_mm_store_ss(res1++, _tmp1);

	_tmp1 = _mm_sub_ps(_sum1_up_, _sum1_low_);
	_tmp2 = _mm_mul_ps(_tmp1, _ipf);
	_tmp1 = _mm_add_ps(_tmp2, _sum1_low_);
	_tmp1 = _mm_mul_ps(_tmp1, _fac2);
	_mm_store_ss(res1++, _tmp1);
}

#define src_filt_4ch_sse SRC_NOT_IMPL

#endif /*SRC_IMPL_SSE*/
