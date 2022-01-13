/**
 * \file: dec_impl_asimd.h
 *
 * ASIMD DEC core implementation.
 *
 * author: Pavanashree/ ADIT / SW1 / pavanashree.krishnappa@in.bosch.com
 *
 * copyright (c) 2017 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *
 ***********************************************************************/

/*PRQA: Lint Message 715 818: Lint fails parsing inline assembler... */
/*lint -save -e715 -e818*/


// DECIMATION_VALUE
// FILT_LEN_DEC2 128
// FILT_LEN_DEC3 192
// FILT_LEN_DEC4 248


//4 channel
#define DEC4_CHANNEL_INITIAL()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64		\n\t"\
	"ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [%0], #64		\n\t"\
	"ld4 {v8.4s, v9.4s, v10.4s, v11.4s}, [%0], #64		\n\t"\
	"ld4 {v12.4s, v13.4s, v14.4s, v15.4s}, [%0], #64	\n\t"\
	"ld4 {v16.4s, v17.4s, v18.4s, v19.4s}, [%1], #64 	\n\t"\
	"prfm	PLDL1KEEP, [%1,#64]				\n\t"\
	"prfm	PLDL1KEEP, [%0,#64]				\n\t"\
	"prfm	PLDL1KEEP, [%0,#128]				\n\t"\
	"prfm	PLDL1KEEP, [%0,#192]				\n\t"\
	"prfm	PLDL1KEEP, [%0,#256]				\n\t"\
	"fmul v20.4s,v0.4s,v16.4s				\n\t"\
	"fmul v21.4s,v1.4s,v16.4s				\n\t"\
	"fmul v22.4s,v2.4s,v16.4s				\n\t"\
	"fmul v23.4s,v3.4s,v16.4s				\n\t"\
	"fmla v20.4s,v4.4s,v17.4s				\n\t"\
	"fmla v21.4s,v5.4s,v17.4s				\n\t"\
	"fmla v22.4s,v6.4s,v17.4s				\n\t"\
	"fmla v23.4s,v7.4s,v17.4s				\n\t"\
	"fmla v20.4s,v8.4s,v18.4s				\n\t"\
	"fmla v21.4s,v9.4s,v18.4s				\n\t"\
	"fmla v22.4s,v10.4s,v18.4s				\n\t"\
	"fmla v23.4s,v11.4s,v18.4s				\n\t"\
	"fmla v20.4s,v12.4s,v19.4s				\n\t"\
	"fmla v21.4s,v13.4s,v19.4s				\n\t"\
	"fmla v22.4s,v14.4s,v19.4s				\n\t"\
	"fmla v23.4s,v15.4s,v19.4s				\n\t"

#define DEC4_CHANNEL_CALC()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64		\n\t"\
	"ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [%0], #64		\n\t"\
	"ld4 {v8.4s, v9.4s, v10.4s, v11.4s}, [%0], #64		\n\t"\
	"ld4 {v12.4s, v13.4s, v14.4s, v15.4s}, [%0], #64	\n\t"\
	"ld4 {v16.4s, v17.4s, v18.4s, v19.4s}, [%1], #64 	\n\t"\
	"fmla v20.4s,v0.4s,v16.4s				\n\t"\
	"fmla v21.4s,v1.4s,v16.4s				\n\t"\
	"fmla v22.4s,v2.4s,v16.4s				\n\t"\
	"fmla v23.4s,v3.4s,v16.4s				\n\t"\
	"fmla v20.4s,v4.4s,v17.4s				\n\t"\
	"fmla v21.4s,v5.4s,v17.4s				\n\t"\
	"fmla v22.4s,v6.4s,v17.4s				\n\t"\
	"fmla v23.4s,v7.4s,v17.4s				\n\t"\
	"fmla v20.4s,v8.4s,v18.4s				\n\t"\
	"fmla v21.4s,v9.4s,v18.4s				\n\t"\
	"fmla v22.4s,v10.4s,v18.4s				\n\t"\
	"fmla v23.4s,v11.4s,v18.4s				\n\t"\
	"fmla v20.4s,v12.4s,v19.4s				\n\t"\
	"fmla v21.4s,v13.4s,v19.4s				\n\t"\
	"fmla v22.4s,v14.4s,v19.4s				\n\t"\
	"fmla v23.4s,v15.4s,v19.4s				\n\t"

#define DEC4_CHANNEL_E_CALC()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64		\n\t"\
	"ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [%0], #64		\n\t"\
	"ld2 {v8.4s, v9.4s}, [%0], #32				\n\t"\
	"fmla v20.4s,v0.4s,v16.4s				\n\t"\
	"fmla v21.4s,v1.4s,v16.4s				\n\t"\
	"fmla v22.4s,v2.4s,v16.4s				\n\t"\
	"fmla v23.4s,v3.4s,v16.4s				\n\t"\
	"fmla v20.4s,v4.4s,v17.4s				\n\t"\
	"fmla v21.4s,v5.4s,v17.4s				\n\t"\
	"fmla v22.4s,v6.4s,v17.4s				\n\t"\
	"fmla v23.4s,v7.4s,v17.4s				\n\t"

#define DEC4_CHANNEL_FINALIZE()\
	"faddp v20.4s, v20.4s, v20.4s			\n\t"\
	"faddp v20.4s, v20.4s, v20.4s			\n\t"\
	"faddp v21.4s, v21.4s, v21.4s			\n\t"\
	"faddp v21.4s, v21.4s, v21.4s			\n\t"\
	"faddp v22.4s, v22.4s, v22.4s			\n\t"\
	"faddp v22.4s, v22.4s, v22.4s			\n\t"\
	"faddp v23.4s, v23.4s, v23.4s			\n\t"\
	"faddp v23.4s, v23.4s, v23.4s			\n\t"\
	"st1 {v23.s}[0], [%2], #4			\n\t"\
	"st1 {v22.s}[0], [%2], #4			\n\t"\
	"st1 {v21.s}[0], [%2], #4			\n\t"\
	"st1 {v20.s}[0], [%2]				\n\t"\
	: "+r"(samples), "+r"(coeffs)\
	: "r"(result)\
	: "v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","v10","v11","v12","v13","v14","v15","v16","v17","v18","v19","v20","v21","v22","v23","memory"\

// 3 channel
#define DEC3_CHANNEL_INITIAL()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64		\n\t"\
	"ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [%0], #64		\n\t"\
	"ld4 {v8.4s, v9.4s, v10.4s, v11.4s}, [%0], #64		\n\t"\
	"ld4 {v12.4s, v13.4s, v14.4s, v15.4s}, [%1], #64 	\n\t"\
	"prfm	PLDL1KEEP, [%1,#64]				\n\t"\
	"prfm	PLDL1KEEP, [%0,#64]				\n\t"\
	"prfm	PLDL1KEEP, [%0,#128]				\n\t"\
	"prfm	PLDL1KEEP, [%0,#192]				\n\t"\
	"fmul v16.4s,v0.4s,v12.4s				\n\t"\
	"fmul v17.4s,v1.4s,v12.4s				\n\t"\
	"fmul v18.4s,v2.4s,v12.4s				\n\t"\
	"fmla v16.4s,v3.4s,v13.4s				\n\t"\
	"fmla v17.4s,v4.4s,v13.4s				\n\t"\
	"fmla v18.4s,v5.4s,v13.4s				\n\t"\
	"fmla v16.4s,v6.4s,v14.4s				\n\t"\
	"fmla v17.4s,v7.4s,v14.4s				\n\t"\
	"fmla v18.4s,v8.4s,v14.4s				\n\t"\
	"fmla v16.4s,v9.4s,v15.4s				\n\t"\
	"fmla v17.4s,v10.4s,v15.4s				\n\t"\
	"fmla v18.4s,v11.4s,v15.4s				\n\t"

#define DEC3_CHANNEL_CALC()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64		\n\t"\
	"ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [%0], #64		\n\t"\
	"ld4 {v8.4s, v9.4s, v10.4s, v11.4s}, [%0], #64		\n\t"\
	"ld4 {v12.4s, v13.4s, v14.4s, v15.4s}, [%1], #64 	\n\t"\
	"fmla v16.4s,v0.4s,v12.4s				\n\t"\
	"fmla v17.4s,v1.4s,v12.4s				\n\t"\
	"fmla v18.4s,v2.4s,v12.4s				\n\t"\
	"fmla v16.4s,v3.4s,v13.4s				\n\t"\
	"fmla v17.4s,v4.4s,v13.4s				\n\t"\
	"fmla v18.4s,v5.4s,v13.4s				\n\t"\
	"fmla v16.4s,v6.4s,v14.4s				\n\t"\
	"fmla v17.4s,v7.4s,v14.4s				\n\t"\
	"fmla v18.4s,v8.4s,v14.4s				\n\t"\
	"fmla v16.4s,v9.4s,v15.4s				\n\t"\
	"fmla v17.4s,v10.4s,v15.4s				\n\t"\
	"fmla v18.4s,v11.4s,v15.4s				\n\t"

#define DEC3_CHANNEL_E_CALC()\
	"ld3 {v0.4s, v1.4s, v2.4s}, [%0], #48			\n\t"\
	"ld3 {v3.4s, v4.4s, v5.4s}, [%0], #48			\n\t"\
	"ld2 {v6.4s, v7.4s}, [%1], #32		 		\n\t"\
	"fmla v16.4s,v0.4s,v6.4s				\n\t"\
	"fmla v17.4s,v1.4s,v6.4s				\n\t"\
	"fmla v18.4s,v2.4s,v6.4s				\n\t"\
	"fmla v16.4s,v3.4s,v7.4s				\n\t"\
	"fmla v17.4s,v4.4s,v7.4s				\n\t"\
	"fmla v18.4s,v5.4s,v7.4s				\n\t"

#define DEC3_CHANNEL_FINALIZE()\
	"faddp v16.4s, v16.4s, v16.4s			\n\t"\
	"faddp v16.4s, v16.4s, v16.4s			\n\t"\
	"faddp v17.4s, v17.4s, v17.4s			\n\t"\
	"faddp v17.4s, v17.4s, v17.4s			\n\t"\
	"faddp v18.4s, v18.4s, v18.4s			\n\t"\
	"faddp v18.4s, v18.4s, v18.4s			\n\t"\
	"st1 {v18.s}[0], [%2], #4			\n\t"\
	"st1 {v17.s}[0], [%2], #4			\n\t"\
	"st1 {v16.s}[0], [%2]				\n\t"\
	: "+r"(samples), "+r"(coeffs)\
	: "r"(result)\
	: "v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","v10","v11","v12","v13","v14","v15","v16","v17","v18","memory"\


// 2 channel
#define DEC2_CHANNEL_INITIAL()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64	\n\t"\
	"ld2 {v4.4s, v5.4s}, [%1], #32			\n\t"\
	"fmul v6.4s,v0.4s,v4.4s				\n\t"\
	"prfm	PLDL1KEEP, [%1,#64]			\n\t"\
	"fmul v7.4s,v1.4s,v4.4s				\n\t"\
	"prfm	PLDL1KEEP, [%0,#64]			\n\t"\
	"fmla v6.4s,v2.4s,v5.4s				\n\t"\
	"prfm	PLDL1KEEP, [%0,#128]			\n\t"\
	"fmla v7.4s,v3.4s,v5.4s				\n\t"

#define DEC2_CHANNEL_CALC()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64	\n\t"\
	"ld2 {v4.4s, v5.4s}, [%1], #32			\n\t"\
	"fmla v6.4s,v0.4s,v4.4s				\n\t"\
	"fmla v7.4s,v1.4s,v4.4s				\n\t"\
	"fmla v6.4s,v2.4s,v5.4s				\n\t"\
	"fmla v7.4s,v3.4s,v5.4s				\n\t"\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64	\n\t"\
	"ld2 {v4.4s, v5.4s}, [%1], #32			\n\t"\
	"fmla v6.4s,v0.4s,v4.4s				\n\t"\
	"fmla v7.4s,v1.4s,v4.4s				\n\t"\
	"fmla v6.4s,v2.4s,v5.4s				\n\t"\
	"fmla v7.4s,v3.4s,v5.4s				\n\t"

#define DEC2_CHANNEL_E_CALC()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64	\n\t"\
	"ld2 {v4.4s, v5.4s}, [%1], #32			\n\t"\
	"fmla v6.4s,v0.4s,v4.4s				\n\t"\
	"fmla v7.4s,v1.4s,v4.4s				\n\t"\
	"fmla v6.4s,v2.4s,v5.4s				\n\t"\
	"fmla v7.4s,v3.4s,v5.4s				\n\t"

#define DEC2_CHANNEL_FINALIZE()\
	"faddp v6.4s, v6.4s, v6.4s			\n\t"\
	"faddp v6.4s, v6.4s, v6.4s			\n\t"\
	"faddp v7.4s, v7.4s, v7.4s			\n\t"\
	"faddp v7.4s, v7.4s, v7.4s			\n\t"\
	"st1 {v7.s}[0], [%2], #4			\n\t"\
	"st1 {v6.s}[0], [%2]				\n\t"\
	: "+r"(samples), "+r"(coeffs)\
	: "r"(result)\
	: "v0","v1","v2","v3","v4","v5","v6","v7","memory"\

//1 channel
#define DEC1_CHANNEL_INITIAL()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64	\n\t"\
	"ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [%1], #64  	\n\t"\
	"prfm	PLDL1KEEP, [%1,#64]			\n\t"\
	"prfm	PLDL1KEEP, [%0,#64]			\n\t"\
	"fmul v8.4s, v0.4s, v4.4s                 	\n\t"\
	"fmla v8.4s, v1.4s, v5.4s                 	\n\t"\
	"fmla v8.4s, v2.4s, v6.4s                 	\n\t"\
	"fmla v8.4s, v3.4s, v7.4s                 	\n\t"

#define DEC1_CHANNEL_CALC()\
	"ld4 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64	\n\t"\
	"ld4 {v4.4s, v5.4s, v6.4s, v7.4s}, [%1], #64  	\n\t"\
	"fmla v8.4s, v0.4s, v4.4s                 	\n\t"\
	"fmla v8.4s, v1.4s, v5.4s                 	\n\t"\
	"fmla v8.4s, v2.4s, v6.4s                 	\n\t"\
	"fmla v8.4s, v3.4s, v7.4s                 	\n\t"

#define DEC1_CHANNEL_E_CALC()\
	"ld2 {v0.4s, v1.4s}, [%0], #32			\n\t"\
	"ld2 {v2.4s, v3.4s}, [%1], #32		  	\n\t"\
	"fmla v8.4s, v0.4s, v2.4s                 	\n\t"\
	"fmla v8.4s, v1.4s, v3.4s                 	\n\t"

#define DEC1_CHANNEL_FINALIZE()\
	"faddp v8.4s, v8.4s, v8.4s             		\n\t"\
	"faddp v8.4s, v8.4s, v8.4s             		\n\t"\
	"st1 {v8.s}[0], [%2]                          	\n\t"\
	: "+r"(samples), "+r"(coeffs)\
	: "r"(result)\
	: "v0","v1","v2","v3","v4","v5","v6","v7","v8","memory"\

// 1 channel
static inline void dec2_filt_1ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC1_CHANNEL_INITIAL()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_FINALIZE()
	);
}

static inline void dec3_filt_1ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC1_CHANNEL_INITIAL()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_FINALIZE()
	);	
}

static inline void dec4_filt_1ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC1_CHANNEL_INITIAL()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_CALC()
		DEC1_CHANNEL_E_CALC()
		DEC1_CHANNEL_FINALIZE()
	);
}

//2 channel

static inline void dec2_filt_2ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC2_CHANNEL_INITIAL()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_E_CALC()
		DEC2_CHANNEL_FINALIZE()
	);	
}

static inline void dec3_filt_2ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC2_CHANNEL_INITIAL()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_E_CALC()
		DEC2_CHANNEL_FINALIZE()
	);	
}

static inline void dec4_filt_2ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC2_CHANNEL_INITIAL()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_CALC()
		DEC2_CHANNEL_FINALIZE()
	);	
}

//3channel

static inline void dec2_filt_3ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC3_CHANNEL_INITIAL()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_FINALIZE()
	);	
}

static inline void dec3_filt_3ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC3_CHANNEL_INITIAL()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_FINALIZE()
	);	
}

static inline void dec4_filt_3ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC3_CHANNEL_INITIAL()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_CALC()
		DEC3_CHANNEL_E_CALC()
		DEC3_CHANNEL_FINALIZE()
	);	
}

//4 channel

static inline void dec2_filt_4ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC4_CHANNEL_INITIAL()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_FINALIZE()
	);	
}

static inline void dec3_filt_4ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC4_CHANNEL_INITIAL()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_FINALIZE()
	);	
}

static inline void dec4_filt_4ch_asimd(float *result, const float *samples, const float *coeffs)
{
	asm volatile(
		DEC4_CHANNEL_INITIAL()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_CALC()
		DEC4_CHANNEL_E_CALC()
		DEC4_CHANNEL_FINALIZE()
	);	
}

/*lint -restore */

