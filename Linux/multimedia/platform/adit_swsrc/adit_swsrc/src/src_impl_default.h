/**
 * \file: src_impl_default.h
 *
 * Default (not optimized) SRC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __SRC_IMPL_DEFAULT__
#define __SRC_IMPL_DEFAULT__

#define FILT_SRC_ALIGN_DEFAULT 1

static inline void src_filt_1ch_default(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{

	float c1_low = 0;
	float c1_up = 0;
	int i;
	for (i = 0; i < FILT_LEN_SRC_(FILT_SRC_ALIGN_DEFAULT); i++) {
		c1_low += *samples * *coeffs;
		c1_up += *samples * *coeffs2;
		samples++;
		coeffs++;
		coeffs2++;
	}
	*res1 = (c1_low + (c1_up-c1_low) * *ipf)*2;
}

static inline void src_filt_2ch_default(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{

	float c1_low = 0;
	float c1_up = 0;
	float c2_low = 0;
	float c2_up = 0;
	int i;
	for (i = 0; i < FILT_LEN_SRC_(FILT_SRC_ALIGN_DEFAULT); i++) {
		c1_low += *samples * *coeffs;
		c1_up += *samples * *coeffs2;
		samples++;

		c2_low += *samples * *coeffs;
		c2_up += *samples * *coeffs2;
		samples++;

		coeffs++;
		coeffs2++;
	}
	*res1 = (c2_low + (c2_up-c2_low) * *ipf)*2;
	res1++;
	*res1 = (c1_low + (c1_up-c1_low) * *ipf)*2;
}


static inline void src_filt_3ch_default(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{
	float c1_low = 0;
	float c1_up = 0;
	float c2_low = 0;
	float c2_up = 0;
	float c3_low = 0;
	float c3_up = 0;
	int i;
	for (i = 0; i < FILT_LEN_SRC_(FILT_SRC_ALIGN_DEFAULT); i++) {
		c1_low += *samples * *coeffs;
		c1_up += *samples * *coeffs2;
		samples++;

		c2_low += *samples * *coeffs;
		c2_up += *samples * *coeffs2;
		samples++;

		c3_low += *samples * *coeffs;
		c3_up += *samples * *coeffs2;
		samples++;

		coeffs++;
		coeffs2++;
	}
	*res1 = (c3_low + (c3_up-c3_low) * *ipf) * 2;
	res1++;
	*res1 = (c2_low + (c2_up-c2_low) * *ipf) * 2;
	res1++;
	*res1 = (c1_low + (c1_up-c1_low) * *ipf) * 2;
}

#define src_filt_4ch_default SRC_NOT_IMPL

#endif /*SRC_IMPL_DEFAULT*/
