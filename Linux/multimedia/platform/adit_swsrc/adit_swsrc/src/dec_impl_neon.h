/**
 * \file: dec_impl_neon.h
 *
 * Neon optimized DEC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __DEC_IMPL_NEON_H__
#define __DEC_IMPL_NEON_H__

/*PRQA: Lint Message 123 : Lint fails parsing inline assembler... */
/*PRQA: Lint Message 715 : Lint fails parsing inline assembler... */
/*lint -save -e715 -e123*/


/*
 load samples, coeffs
 preload some data
 first calculation of samples
 calc samples
*/

#define DEC4CH_L_PL_INIT_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld4.32   {d4,d6,d8,d10}, [%0]!           \n\t"\
         "vld4.32   {d5,d7,d9,d11}, [%0]!           \n\t"\
         "vld4.32   {d12,d14,d16,d18}, [%0]!        \n\t"\
         "vld4.32   {d13,d15,d17,d19}, [%0]!        \n\t"\
         "pld [%1, #64]                             \n\t"\
         "pld [%0, #64]                             \n\t"\
         "pld [%0, #128]                            \n\t"\
         "pld [%0, #192]                            \n\t"\
         "pld [%0, #256]                            \n\t"\
         "vmul.f32  q12, q2, q0                     \n\t"\
         "vmul.f32  q13, q3, q0                     \n\t"\
         "vmul.f32  q14, q4, q0                     \n\t"\
         "vmul.f32  q15, q5, q0                     \n\t"\
         "vmla.f32  q12, q6, q1                     \n\t"\
         "vmla.f32  q13, q7, q1                     \n\t"\
         "vmla.f32  q14, q8, q1                     \n\t"\
         "vmla.f32  q15, q9, q1                     \n\t"


#define DEC3CH_L_PL_INIT_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld3.32   {d4,d6,d8}, [%0]!               \n\t"\
         "vld3.32   {d5,d7,d9}, [%0]!               \n\t"\
         "vld3.32   {d10,d12,d14}, [%0]!            \n\t"\
         "vld3.32   {d11,d13,d15}, [%0]!            \n\t"\
         "pld [%1, #64]                             \n\t"\
         "pld [%0, #64]                             \n\t"\
         "pld [%0, #128]                            \n\t"\
         "pld [%0, #192]                            \n\t"\
         "vmul.f32  q12, q2, q0                     \n\t"\
         "vmul.f32  q13, q3, q0                     \n\t"\
         "vmul.f32  q14, q4, q0                     \n\t"\
         "vmla.f32  q12, q5, q1                     \n\t"\
         "vmla.f32  q13, q6, q1                     \n\t"\
         "vmla.f32  q14, q7, q1                     \n\t"


#define DEC2CH_L_PL_INIT_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld2.32   {d4,d5,d6,d7},   [%0]!          \n\t"\
         "vld2.32   {d8,d9,d10,d11}, [%0]!          \n\t"\
         "pld [%1, #64]                             \n\t"\
         "pld [%0, #64]                             \n\t"\
         "pld [%0, #128]                            \n\t"\
         "vmul.f32  q12, q2, q0                     \n\t"\
         "vmul.f32  q13, q3, q0                     \n\t"\
         "vmla.f32  q12, q4, q1                     \n\t"\
         "vmla.f32  q13, q5, q1                     \n\t"

#define DEC1CH_L_PL_INIT_CALC() \
         "vld1.32   {d0,d1,d2,d3}, [%1]!            \n\t"\
         "vld1.32   {d4,d5,d6,d7}, [%0]!            \n\t"\
         "pld [%1, #64]                             \n\t"\
         "pld [%0, #64]                             \n\t"\
         "vmul.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q12, q3, q1                     \n\t" 

/*
 load samples, coeffs
 preload some data
 calc samples
 calc samples
*/
#define DEC4CH_L_PL_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld4.32   {d4,d6,d8,d10}, [%0]!           \n\t"\
         "vld4.32   {d5,d7,d9,d11}, [%0]!           \n\t"\
         "vld4.32   {d12,d14,d16,d18}, [%0]!        \n\t"\
         "vld4.32   {d13,d15,d17,d19}, [%0]!        \n\t"\
         "pld [%1, #64]                             \n\t"\
         "pld [%0, #64]                             \n\t"\
         "pld [%0, #128]                            \n\t"\
         "pld [%0, #192]                            \n\t"\
         "pld [%0, #256]                            \n\t"\
         "vmla.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q13, q3, q0                     \n\t"\
         "vmla.f32  q14, q4, q0                     \n\t"\
         "vmla.f32  q15, q5, q0                     \n\t"\
         "vmla.f32  q12, q6, q1                     \n\t"\
         "vmla.f32  q13, q7, q1                     \n\t"\
         "vmla.f32  q14, q8, q1                     \n\t"\
         "vmla.f32  q15, q9, q1                     \n\t"


#define DEC3CH_L_PL_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld3.32   {d4,d6,d8}, [%0]!               \n\t"\
         "vld3.32   {d5,d7,d9}, [%0]!               \n\t"\
         "vld3.32   {d10,d12,d14}, [%0]!            \n\t"\
         "vld3.32   {d11,d13,d15}, [%0]!            \n\t"\
         "pld [%1, #64]                             \n\t"\
         "pld [%0, #64]                             \n\t"\
         "pld [%0, #128]                            \n\t"\
         "pld [%0, #192]                            \n\t"\
         "vmla.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q13, q3, q0                     \n\t"\
         "vmla.f32  q14, q4, q0                     \n\t"\
         "vmla.f32  q12, q5, q1                     \n\t"\
         "vmla.f32  q13, q6, q1                     \n\t"\
         "vmla.f32  q14, q7, q1                     \n\t"


#define DEC2CH_L_PL_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld2.32   {d4,d5,d6,d7},   [%0]!          \n\t"\
         "vld2.32   {d8,d9,d10,d11}, [%0]!          \n\t"\
         "pld [%1, #64]                             \n\t"\
         "pld [%0, #64]                             \n\t"\
         "pld [%0, #128]                            \n\t"\
         "vmla.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q13, q3, q0                     \n\t"\
         "vmla.f32  q12, q4, q1                     \n\t"\
         "vmla.f32  q13, q5, q1                     \n\t"

#define DEC1CH_L_PL_CALC() \
         "vld1.32   {d0,d1,d2,d3},  [%1]!           \n\t"\
         "vld1.32   {d4,d5,d6,d7},  [%0]!           \n\t"\
         "pld [%1, #64]                             \n\t"\
         "pld [%0, #64]                             \n\t"\
         "vmla.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q12, q3, q1                     \n\t"

/*
 load samples, coeffs
 calc samples
 calc samples
*/
#define DEC4CH_L_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld4.32   {d4,d6,d8,d10}, [%0]!           \n\t"\
         "vld4.32   {d5,d7,d9,d11}, [%0]!           \n\t"\
         "vld4.32   {d12,d14,d16,d18}, [%0]!        \n\t"\
         "vld4.32   {d13,d15,d17,d19}, [%0]!        \n\t"\
         "vmla.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q13, q3, q0                     \n\t"\
         "vmla.f32  q14, q4, q0                     \n\t"\
         "vmla.f32  q15, q5, q0                     \n\t"\
         "vmla.f32  q12, q6, q1                     \n\t"\
         "vmla.f32  q13, q7, q1                     \n\t"\
         "vmla.f32  q14, q8, q1                     \n\t"\
         "vmla.f32  q15, q9, q1                     \n\t"

#define DEC3CH_L_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld3.32   {d4,d6,d8}, [%0]!               \n\t"\
         "vld3.32   {d5,d7,d9}, [%0]!               \n\t"\
         "vld3.32   {d10,d12,d14}, [%0]!            \n\t"\
         "vld3.32   {d11,d13,d15}, [%0]!            \n\t"\
         "vmla.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q13, q3, q0                     \n\t"\
         "vmla.f32  q14, q4, q0                     \n\t"\
         "vmla.f32  q12, q5, q1                     \n\t"\
         "vmla.f32  q13, q6, q1                     \n\t"\
         "vmla.f32  q14, q7, q1                     \n\t"

#define DEC2CH_L_CALC() \
         "vld1.32   {d0,d1,d2,d3},   [%1]!          \n\t"\
         "vld2.32   {d4,d5,d6,d7},   [%0]!          \n\t"\
         "vld2.32   {d8,d9,d10,d11}, [%0]!          \n\t"\
         "vmla.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q13, q3, q0                     \n\t"\
         "vmla.f32  q12, q4, q1                     \n\t"\
         "vmla.f32  q13, q5, q1                     \n\t"

#define DEC1CH_L_CALC() \
         "vld1.32   {d0,d1,d2,d3},  [%1]!           \n\t"\
         "vld1.32   {d4,d5,d6,d7},  [%0]!           \n\t"\
         "vmla.f32  q12, q2, q0                     \n\t"\
         "vmla.f32  q12, q3, q1                     \n\t"

/*
 accumulate to the end values
 store values
*/

#define DEC4CH_FINALIZE()\
         "vpadd.f32 d3, d24, d25                    \n\t"\
         "vpadd.f32 d2, d26, d27                    \n\t"\
         "vpadd.f32 d1, d28, d29                    \n\t"\
         "vpadd.f32 d0, d30, d31                    \n\t"\
         "vadd.f32  s0, s0, s1                      \n\t"\
         "vadd.f32  s1, s2, s3                      \n\t"\
         "vadd.f32  s2, s4, s5                      \n\t"\
         "vadd.f32  s3, s6, s7                      \n\t"\
	 "vstmia %2, {s0,s1,s2,s3} 	            \n\t"\
         : "+r"(samples),"+r"(coeffs)\
         : "r"(result)\
         : "q0","q1","q2","q3","q4","q5","q6","q7","q8","q9","q12","q13","q14","q15","memory"\

#define DEC3CH_FINALIZE()\
         "vpadd.f32 d2, d24, d25                    \n\t"\
         "vpadd.f32 d1, d26, d27                    \n\t"\
         "vpadd.f32 d0, d28, d29                    \n\t"\
         "vadd.f32  s0, s0, s1                      \n\t"\
         "vadd.f32  s1, s2, s3                      \n\t"\
         "vadd.f32  s2, s4, s5                      \n\t"\
	 "vstmia %2, {s0,s1,s2} 	            \n\t"\
         : "+r"(samples),"+r"(coeffs)\
         : "r"(result)\
         : "q0","q1","q2","q3","q4","q5","q6","q7","q8","q12","q13","q14","memory"\


#define DEC2CH_FINALIZE()\
         "vpadd.f32 d1, d24, d25                    \n\t"\
         "vpadd.f32 d0, d26, d27                    \n\t"\
         "vadd.f32  s0, s0, s1                      \n\t"\
         "vadd.f32  s1, s2, s3                      \n\t"\
	 "vstmia %2, {s0,s1} 	                    \n\t"\
         : "+r"(samples),"+r"(coeffs)\
         : "r"(result)\
         : "q0","q1","q2","q3","q4","q5","q12","q13","memory"\


#define DEC1CH_FINALIZE()\
         "vpadd.f32 d0, d24, d25                    \n\t"\
         "vadd.f32  s0, s0, s1                      \n\t"\
         "vstmia %2, {s0} 	                    \n\t"\
         : "+r"(samples), "+r"(coeffs)\
         : "r"(result)\
         : "q0","q1","q2","q4","q12","memory"\


static inline void dec2_filt_1ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
	DEC1CH_L_PL_INIT_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_CALC()
	DEC1CH_L_CALC()
	DEC1CH_FINALIZE()
	);
}

static inline void dec2_filt_2ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
 	DEC2CH_L_PL_INIT_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_CALC()
	DEC2CH_L_CALC()                            
	DEC2CH_FINALIZE()
	);
}

static inline void dec2_filt_3ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
 	DEC3CH_L_PL_INIT_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_CALC()
	DEC3CH_L_CALC()                            
	DEC3CH_FINALIZE()
	);
}

static inline void dec2_filt_4ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
 	DEC4CH_L_PL_INIT_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_CALC()
	DEC4CH_L_CALC()                            
	DEC4CH_FINALIZE()
	);
}

static inline void dec3_filt_1ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
 	DEC1CH_L_PL_INIT_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_CALC()
	DEC1CH_L_CALC()                            
	DEC1CH_FINALIZE()
	);
}

static inline void dec3_filt_2ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
 	DEC2CH_L_PL_INIT_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_CALC()
	DEC2CH_L_CALC()                            
	DEC2CH_FINALIZE()
	);
}

static inline void dec3_filt_3ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
 	DEC3CH_L_PL_INIT_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_CALC()
	DEC3CH_L_CALC()                            
	DEC3CH_FINALIZE()
	);
}

static inline void dec3_filt_4ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
 	DEC4CH_L_PL_INIT_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_CALC()
	DEC4CH_L_CALC()                            
	DEC4CH_FINALIZE()
	);
}

static inline void dec4_filt_1ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
	DEC1CH_L_PL_INIT_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_PL_CALC()
	DEC1CH_L_CALC()
	DEC1CH_L_CALC()                            
	DEC1CH_FINALIZE()
	);
}

static inline void dec4_filt_2ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
	DEC2CH_L_PL_INIT_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_PL_CALC()
	DEC2CH_L_CALC()
	DEC2CH_L_CALC()                            
	DEC2CH_FINALIZE()
	);
}

static inline void dec4_filt_3ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
	DEC3CH_L_PL_INIT_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_PL_CALC()
	DEC3CH_L_CALC()
	DEC3CH_L_CALC()                            
	DEC3CH_FINALIZE()
	);
}

static inline void dec4_filt_4ch_neon(float *result, const float *samples, const float *coeffs)
{
    asm volatile (
	DEC4CH_L_PL_INIT_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_PL_CALC()
	DEC4CH_L_CALC()
	DEC4CH_L_CALC()                            
	DEC4CH_FINALIZE()
	);
}

/*lint -restore */
#endif /*__DEC_IMPL_NEON_H__*/
