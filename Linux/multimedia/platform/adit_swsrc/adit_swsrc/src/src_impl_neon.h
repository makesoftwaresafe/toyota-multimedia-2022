/**
 * \file: src_impl_neon.h
 *
 * Neon optimized SRC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __SRC_IMPL_NEON_H__
#define __SRC_IMPL_NEON_H__

/*PRQA: Lint Message 123 : Lint fails parsing inline assembler... */
/*PRQA: Lint Message 715 : Lint fails parsing inline assembler... */
/*lint -save -e715 -e123*/


/*
 load 8 Left 8, 8 Right, 8 Coeff Low, 8 Coeff high
 preload some data
 clear polyphases
 calc 4 samples
 calc 4 samples
*/
#define SRC3CH_L_PL_INIT_CALC() \
         "vld1.32   {d0, d1, d2, d3}, [%1]!                \n\t"\
         "vld1.32   {d4, d5, d6, d7}, [%2]!                \n\t"\
         "vld3.32   {d8, d10, d12},  [%0]!                \n\t"\
         "vld3.32   {d9, d11, d13},  [%0]!                \n\t"\
         "vld3.32   {d14, d16, d18},  [%0]!                \n\t"\
         "vld3.32   {d15, d17, d19},  [%0]!                \n\t"\
								 \
         "pld [%1, #64]                                     \n\t"\
         "pld [%2, #64]                                     \n\t"\
         "pld [%0, #64]                                     \n\t"\
         "pld [%0, #128]                                    \n\t"\
         "pld [%0, #192]                                    \n\t"\
								 \
         "vmul.f32  q10, q4, q0                             \n\t"\
         "vmul.f32  q11, q5, q0                             \n\t"\
         "vmul.f32  q12, q6, q0                             \n\t"\
         "vmul.f32  q13, q4, q2                             \n\t"\
         "vmul.f32  q14, q5, q2                             \n\t"\
         "vmul.f32  q15, q6, q2                             \n\t"\
								 \
         "vmla.f32  q10, q7, q1                             \n\t"\
         "vmla.f32  q11, q8, q1                             \n\t"\
         "vmla.f32  q12, q9, q1                             \n\t"\
         "vmla.f32  q13, q7, q3                             \n\t"\
         "vmla.f32  q14, q8, q3                             \n\t"\
         "vmla.f32  q15, q9, q3                             \n\t"


#define SRC2CH_L_PL_INIT_CALC() \
         "vld1.32   {d0, d1, d2, d3},  [%1]!                \n\t"\
         "vld1.32   {d12,d13,d14,d15}, [%2]!                \n\t"\
         "vld2.32   {d4, d5, d6, d7},  [%0]!                \n\t"\
         "vld2.32   {d8, d9,d10,d11},  [%0]!                \n\t"\
								 \
         "pld [%1, #64]                                     \n\t"\
         "pld [%2, #64]                                     \n\t"\
         "pld [%0, #64]                                     \n\t"\
         "pld [%0, #128]                                    \n\t"\
								 \
         "vmul.f32  q12, q2, q0                             \n\t"\
         "vmul.f32  q13, q3, q0                             \n\t"\
         "vmul.f32  q14, q2, q6                             \n\t"\
         "vmul.f32  q15, q3, q6                             \n\t"\
								 \
         "vmla.f32  q12, q4, q1                             \n\t"\
         "vmla.f32  q13, q5, q1                             \n\t"\
         "vmla.f32  q14, q4, q7                             \n\t"\
         "vmla.f32  q15, q5, q7                             \n\t"

#define SRC1CH_L_PL_INIT_CALC() \
         "vld1.32   {d0, d1, d2, d3},  [%1]!                \n\t"\
         "vld1.32   {d12,d13,d14,d15}, [%2]!                \n\t"\
         "vld1.32   {d4, d5, d6, d7},  [%0]!                \n\t"\
								 \
         "pld [%1, #64]                                     \n\t"\
         "pld [%2, #64]                                     \n\t"\
         "pld [%0, #64]                                     \n\t"\
								 \
         "vmul.f32  q12, q2, q0                             \n\t"\
         "vmul.f32  q14, q2, q6                             \n\t"\
								 \
         "vmla.f32  q12, q3, q1                             \n\t"\
         "vmla.f32  q14, q3, q7                             \n\t" 


/*
 load 8 Left 8, 8 Right, 8 Coeff Low, 8 Coeff high
 preload some data
 calc 4 samples
 calc 4 samples
*/
#define SRC3CH_L_PL_CALC() \
         "vld1.32   {d0, d1, d2, d3}, [%1]!                \n\t"\
         "vld1.32   {d4, d5, d6, d7}, [%2]!                \n\t"\
         "vld3.32   {d8, d10, d12},  [%0]!                \n\t"\
         "vld3.32   {d9, d11, d13},  [%0]!                \n\t"\
         "vld3.32   {d14, d16, d18},  [%0]!                \n\t"\
         "vld3.32   {d15, d17, d19},  [%0]!                \n\t"\
								 \
         "pld [%1, #64]                                     \n\t"\
         "pld [%2, #64]                                     \n\t"\
         "pld [%0, #64]                                     \n\t"\
         "pld [%0, #128]                                    \n\t"\
         "pld [%0, #192]                                    \n\t"\
								 \
         "vmla.f32  q10, q4, q0                             \n\t"\
         "vmla.f32  q11, q5, q0                             \n\t"\
         "vmla.f32  q12, q6, q0                             \n\t"\
         "vmla.f32  q13, q4, q2                             \n\t"\
         "vmla.f32  q14, q5, q2                             \n\t"\
         "vmla.f32  q15, q6, q2                             \n\t"\
								 \
         "vmla.f32  q10, q7, q1                             \n\t"\
         "vmla.f32  q11, q8, q1                             \n\t"\
         "vmla.f32  q12, q9, q1                             \n\t"\
         "vmla.f32  q13, q7, q3                             \n\t"\
         "vmla.f32  q14, q8, q3                             \n\t"\
         "vmla.f32  q15, q9, q3                             \n\t"


#define SRC2CH_L_PL_CALC() \
         "vld1.32   {d0, d1, d2, d3},  [%1]!                \n\t"\
         "vld1.32   {d12,d13,d14,d15}, [%2]!                \n\t"\
         "vld2.32   {d4, d5, d6, d7},  [%0]!                \n\t"\
         "vld2.32   {d8, d9, d10,d11}, [%0]!                \n\t"\
								 \
         "pld [%1, #64]                                     \n\t"\
         "pld [%2, #64]                                     \n\t"\
         "pld [%0, #64]                                     \n\t"\
         "pld [%0, #128]                                    \n\t"\
								 \
         "vmla.f32  q12, q2, q0                             \n\t"\
         "vmla.f32  q13, q3, q0                             \n\t"\
         "vmla.f32  q14, q2, q6                             \n\t"\
         "vmla.f32  q15, q3, q6                             \n\t"\
								 \
         "vmla.f32  q12, q4, q1                             \n\t"\
         "vmla.f32  q13, q5, q1                             \n\t"\
         "vmla.f32  q14, q4, q7                             \n\t"\
         "vmla.f32  q15, q5, q7                             \n\t"


#define SRC1CH_L_PL_CALC() \
         "vld1.32   {d0, d1, d2, d3},  [%1]!                \n\t"\
         "vld1.32   {d12,d13,d14,d15}, [%2]!                \n\t"\
         "vld1.32   {d4, d5, d6, d7},  [%0]!                \n\t"\
								 \
         "pld [%1, #64]                                     \n\t"\
         "pld [%2, #64]                                     \n\t"\
         "pld [%0, #64]                                     \n\t"\
								 \
         "vmla.f32  q12, q2, q0                             \n\t"\
         "vmla.f32  q14, q2, q6                             \n\t"\
								 \
         "vmla.f32  q12, q3, q1                             \n\t"\
         "vmla.f32  q14, q3, q7                             \n\t"

/*
 load 8 Left 8, 8 Right, 8 Coeff Low, 8 Coeff high
 calc 4 samples
 calc 4 samples
*/
#define SRC3CH_L_CALC() \
         "vld1.32   {d0, d1, d2, d3}, [%1]!                \n\t"\
         "vld1.32   {d4, d5, d6, d7}, [%2]!                \n\t"\
         "vld3.32   {d8, d10, d12},  [%0]!                \n\t"\
         "vld3.32   {d9, d11, d13},  [%0]!                \n\t"\
         "vld3.32   {d14, d16, d18},  [%0]!                \n\t"\
         "vld3.32   {d15, d17, d19},  [%0]!                \n\t"\
								 \
         "vmla.f32  q10, q4, q0                             \n\t"\
         "vmla.f32  q11, q5, q0                             \n\t"\
         "vmla.f32  q12, q6, q0                             \n\t"\
         "vmla.f32  q13, q4, q2                             \n\t"\
         "vmla.f32  q14, q5, q2                             \n\t"\
         "vmla.f32  q15, q6, q2                             \n\t"\
								 \
         "vmla.f32  q10, q7, q1                             \n\t"\
         "vmla.f32  q11, q8, q1                             \n\t"\
         "vmla.f32  q12, q9, q1                             \n\t"\
         "vmla.f32  q13, q7, q3                             \n\t"\
         "vmla.f32  q14, q8, q3                             \n\t"\
         "vmla.f32  q15, q9, q3                             \n\t"


#define SRC2CH_L_CALC() \
         "vld1.32   {d0, d1, d2, d3},  [%1]!                \n\t"\
         "vld1.32   {d12,d13,d14,d15}, [%2]!                \n\t"\
         "vld2.32   {d4, d5, d6, d7},  [%0]!                \n\t"\
         "vld2.32   {d8, d9, d10,d11}, [%0]!                \n\t"\
								 \
         "vmla.f32  q12, q2, q0                             \n\t"\
         "vmla.f32  q13, q3, q0                             \n\t"\
         "vmla.f32  q14, q2, q6                             \n\t"\
         "vmla.f32  q15, q3, q6                             \n\t"\
								 \
         "vmla.f32  q12, q4, q1                             \n\t"\
         "vmla.f32  q13, q5, q1                             \n\t"\
         "vmla.f32  q14, q4, q7                             \n\t"\
         "vmla.f32  q15, q5, q7                             \n\t"

#define SRC1CH_L_CALC() \
         "vld1.32   {d0, d1, d2, d3},  [%1]!                \n\t"\
         "vld1.32   {d12,d13,d14,d15}, [%2]!                \n\t"\
         "vld1.32   {d4, d5, d6, d7},  [%0]!                \n\t"\
								 \
         "vmla.f32  q12, q2, q0                             \n\t"\
         "vmla.f32  q14, q2, q6                             \n\t"\
								 \
         "vmla.f32  q12, q3, q1                             \n\t"\
         "vmla.f32  q14, q3, q7                             \n\t"


/*
 accumulate to the end values
 load ipf variable
 accumulate to the end values
 execute ratio calculation 
 store values
*/
#define SRC3CH_FINALIZE()\
         "vpadd.f32 d0, d20, d21                            \n\t"\
         "vpadd.f32 d1, d22, d23                            \n\t"\
         "vpadd.f32 d2, d24, d25                            \n\t"\
         "vpadd.f32 d3, d26, d27                            \n\t"\
         "vpadd.f32 d4, d28, d29                            \n\t"\
         "vpadd.f32 d5, d30, d31                            \n\t"\
								 \
         "vldr s12,  [%3]                                    \n\t"\
								\
         "vadd.f32  s0, s0, s1                              \n\t"\
         "vadd.f32  s2, s2, s3                              \n\t"\
         "vadd.f32  s4, s4, s5                              \n\t"\
         "vadd.f32  s6, s6, s7                              \n\t"\
         "vadd.f32  s8, s8, s9                              \n\t"\
         "vadd.f32  s10, s10, s11                            \n\t"\
								\
         "vsub.f32  s6, s6, s0                              \n\t"\
         "vsub.f32  s8, s8, s2                              \n\t"\
         "vsub.f32  s10, s10, s4                              \n\t"\
								\
         "vmla.f32  s0, s12, s6                              \n\t"\
         "vmla.f32  s2, s12, s8                              \n\t"\
         "vmla.f32  s4, s12, s10                              \n\t"\
								 \
         "vmov.f32  s12, #2.0                               \n\t"\
         "vmul.f32  s1, s2, s12                              \n\t"\
         "vmul.f32  s2, s0, s12                              \n\t"\
         "vmul.f32  s0, s4, s12                              \n\t"\
								\
	 "vstmia %4, {s0,s1,s2} 	                    	\n\t"\
								\
         : "+r"(samples), "+r"(coeffs), "+r"(coeffs2), "+r"(ipf)\
         : "r"(res1)\
         : "q0", "q1", "q2", "q3","q4","q5", "q6", "q7", "q8", "q9", "q10","q11","q12","q13", "q14", "q15","memory"


#define SRC2CH_FINALIZE()\
         "vpadd.f32 d0, d24, d25                            \n\t"\
         "vpadd.f32 d1, d26, d27                            \n\t"\
         "vpadd.f32 d2, d28, d29                            \n\t"\
         "vpadd.f32 d3, d30, d31                            \n\t"\
								 \
         "vldr s8,  [%3]                                    \n\t"\
								 \
         "vadd.f32  s0, s0, s1                              \n\t"\
         "vadd.f32  s2, s2, s3                              \n\t"\
         "vadd.f32  s4, s4, s5                              \n\t"\
         "vadd.f32  s6, s6, s7                              \n\t"\
								 \
         "vsub.f32  s4, s4, s0                              \n\t"\
         "vsub.f32  s6, s6, s2                              \n\t"\
         "vmla.f32  s0, s8, s4                              \n\t"\
         "vmla.f32  s2, s8, s6                              \n\t"\
								\
         "vmov.f32  s12,  #2.0                               \n\t"\
         "vmul.f32  s1, s0, s12                              \n\t"\
         "vmul.f32  s0, s2, s12                              \n\t"\
								 \
	 "vstmia %4, {s0,s1} 	                    	\n\t"\
								 \
         : "+r"(samples), "+r"(coeffs), "+r"(coeffs2), "+r"(ipf)\
         : "r"(res1)\
         : "q0", "q1", "q2", "q3","q4","q5", "q6", "q7", "q8", "q9", "q10","q11","q12","q13", "q14", "q15","memory"

#define SRC1CH_FINALIZE()\
         "vpadd.f32 d0, d24, d25                            \n\t"\
         "vpadd.f32 d2, d28, d29                            \n\t"\
								 \
         "vldr s8,  [%3]                                    \n\t"\
								 \
         "vadd.f32  s0, s0, s1                              \n\t"\
         "vadd.f32  s4, s4, s5                              \n\t"\
								 \
         "vsub.f32  s4, s4, s0                              \n\t"\
         "vmla.f32  s0, s8, s4                              \n\t"\
								\
         "vmov.f32  s12,  #2.0                               \n\t"\
         "vmul.f32  s0, s0, s12                              \n\t"\
								\
	 "vstmia %4, {s0} 	                    	\n\t"\
								 \
         : "+r"(samples), "+r"(coeffs), "+r"(coeffs2), "+r"(ipf)\
         : "r"(res1)\
         : "q0","q1","q2","q3","q6","q7","q12","q14","memory"

static inline void src_filt_1ch_neon(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{
    asm volatile (
	SRC1CH_L_PL_INIT_CALC()
	SRC1CH_L_PL_CALC()
        SRC1CH_L_PL_CALC()                         
        SRC1CH_L_PL_CALC()                         
        SRC1CH_L_PL_CALC()                         
        SRC1CH_L_PL_CALC()                         
        SRC1CH_L_CALC()                       
	SRC1CH_L_CALC()
	SRC1CH_FINALIZE()
	);
}

static inline void src_filt_2ch_neon(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{
    asm volatile (
	SRC2CH_L_PL_INIT_CALC()
	SRC2CH_L_PL_CALC()
	SRC2CH_L_PL_CALC()
	SRC2CH_L_PL_CALC()
	SRC2CH_L_PL_CALC()
	SRC2CH_L_PL_CALC()
	SRC2CH_L_CALC()
        SRC2CH_L_CALC()                     
	SRC2CH_FINALIZE()
	);
}

static inline void src_filt_3ch_neon(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{
    asm volatile (
	SRC3CH_L_PL_INIT_CALC()
	SRC3CH_L_PL_CALC()
	SRC3CH_L_PL_CALC()
	SRC3CH_L_PL_CALC()
	SRC3CH_L_PL_CALC()
	SRC3CH_L_PL_CALC()
	SRC3CH_L_CALC()
        SRC3CH_L_CALC()                     
	SRC3CH_FINALIZE()
	);
}


#define src_filt_4ch_neon SRC_NOT_IMPL

/*lint -restore */
#endif /*__SRC_IMPL_NEON_H__*/
