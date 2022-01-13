/**
 * \file: src_impl_asimd.h
 *
 * ASIMD SRC core implementation.
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


// INTERPOLATION VALUES
// FILT_LEN_SRC	64

// 1 channel calculation

#define SRC1CH_INITIAL()\
	"ld1 {v0.4s}, [%0], #16		\n\t"\
	"ld1 {v2.4s}, [%1], #16		\n\t"\
	"ld1 {v3.4s}, [%2], #16		\n\t"\
	"prfm	PLDL1KEEP, [%1,#64]	\n\t"\
	"prfm	PLDL1KEEP, [%2,#64]	\n\t"\
	"prfm	PLDL1KEEP, [%0,#64]	\n\t"\
	"fmul v4.4s, v0.4s, v2.4s	\n\t"\
	"fmul v5.4s, v0.4s, v3.4s	\n\t"\
	"ld1 {v0.4s}, [%0], #16		\n\t"\
	"ld1 {v2.4s}, [%1], #16		\n\t"\
	"ld1 {v3.4s}, [%2], #16		\n\t"\
	"fmla v4.4s, v0.4s, v2.4s	\n\t"\
	"fmla v5.4s, v0.4s, v3.4s	\n\t"

#define SRC1CH_CALC()\
	"ld1 {v0.4s}, [%0], #16		\n\t"\
	"ld1 {v2.4s}, [%1], #16		\n\t"\
	"ld1 {v3.4s}, [%2], #16		\n\t"\
	"fmla v4.4s, v0.4s, v2.4s	\n\t"\
	"fmla v5.4s, v0.4s, v3.4s	\n\t"\
	"ld1 {v0.4s}, [%0], #16		\n\t"\
	"ld1 {v2.4s}, [%1], #16		\n\t"\
	"ld1 {v3.4s}, [%2], #16		\n\t"\
	"fmla v4.4s, v0.4s, v2.4s	\n\t"\
	"fmla v5.4s, v0.4s, v3.4s	\n\t"

#define SRC1CH_FINALIZE()\
	"faddp v4.4s, v4.4s, v4.4s	\n\t"\
	"faddp v4.4s, v4.4s, v4.4s	\n\t"\
	"faddp v5.4s, v5.4s, v5.4s	\n\t"\
	"faddp v5.4s, v5.4s, v5.4s	\n\t"\
	"ldr s0, [%3]			\n\t"\
	"fsub s5, s5, s4		\n\t"\
	"fmadd s1, s5, s0, s4		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4]			\n\t"\
	: "+r"(samples), "+r"(coeffs), "+r"(coeffs2), "+r"(ipf)\
	: "r"(res1)\
	: "v0","v1","v2","v3","v4","v5","s0","s1","s4","s5","memory"\

// 2 channel calculation

#define SRC2CH_INITIAL()\
	"ld2 {v0.s, v1.s}[0], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[1], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[2], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[3], [%0], #8	\n\t"\
	"ld1 {v2.4s}, [%1], #16		\n\t"\
	"ld1 {v3.4s}, [%2], #16		\n\t"\
	"prfm	PLDL1KEEP, [%0,#128]	\n\t"\
	"fmul v4.4s, v0.4s, v2.4s	\n\t"\
	"prfm	PLDL1KEEP, [%1,#64]	\n\t"\
	"fmul v5.4s, v0.4s, v3.4s	\n\t"\
	"prfm	PLDL1KEEP, [%2,#64]	\n\t"\
	"fmul v6.4s, v1.4s, v2.4s	\n\t"\
	"prfm	PLDL1KEEP, [%0,#64]	\n\t"\
	"fmul v7.4s, v1.4s, v3.4s	\n\t"\
	"ld2 {v0.s, v1.s}[0], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[1], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[2], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[3], [%0], #8	\n\t"\
	"ld1 {v2.4s}, [%1], #16		\n\t"\
	"ld1 {v3.4s}, [%2], #16		\n\t"\
	"fmla v4.4s, v0.4s, v2.4s	\n\t"\
	"fmla v5.4s, v0.4s, v3.4s	\n\t"\
	"fmla v6.4s, v1.4s, v2.4s	\n\t"\
	"fmla v7.4s, v1.4s, v3.4s	\n\t"

#define SRC2CH_CALC()\
	"ld2 {v0.s, v1.s}[0], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[1], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[2], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[3], [%0], #8	\n\t"\
	"ld1 {v2.4s}, [%1], #16		\n\t"\
	"ld1 {v3.4s}, [%2], #16		\n\t"\
	"fmla v4.4s, v0.4s, v2.4s	\n\t"\
	"fmla v5.4s, v0.4s, v3.4s	\n\t"\
	"fmla v6.4s, v1.4s, v2.4s	\n\t"\
	"fmla v7.4s, v1.4s, v3.4s	\n\t"\
	"ld2 {v0.s, v1.s}[0], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[1], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[2], [%0], #8	\n\t"\
	"ld2 {v0.s, v1.s}[3], [%0], #8	\n\t"\
	"ld1 {v2.4s}, [%1], #16		\n\t"\
	"ld1 {v3.4s}, [%2], #16		\n\t"\
	"fmla v4.4s, v0.4s, v2.4s	\n\t"\
	"fmla v5.4s, v0.4s, v3.4s	\n\t"\
	"fmla v6.4s, v1.4s, v2.4s	\n\t"\
	"fmla v7.4s, v1.4s, v3.4s	\n\t"

#define SRC2CH_FINALIZE()\
	"faddp v6.4s, v6.4s, v6.4s	\n\t"\
	"faddp v6.4s, v6.4s, v6.4s	\n\t"\
	"faddp v7.4s, v7.4s, v7.4s	\n\t"\
	"ldr s0, [%3]			\n\t"\
	"faddp v7.4s, v7.4s, v7.4s	\n\t"\
	"fsub s7, s7, s6		\n\t"\
	"faddp v4.4s, v4.4s, v4.4s	\n\t"\
	"fmadd s1, s7, s0, s6		\n\t"\
	"faddp v5.4s, v5.4s, v5.4s	\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"faddp v5.4s, v5.4s, v5.4s	\n\t"\
	"str s1, [%4], #4		\n\t"\
	"faddp v4.4s, v4.4s, v4.4s	\n\t"\
	"fsub s5, s5, s4		\n\t"\
	"fmadd s1, s5, s0, s4		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4]			\n\t"\
	: "+r"(samples), "+r"(coeffs), "+r"(coeffs2), "+r"(ipf)\
	: "r"(res1)\
	: "v0","v1","v2","v3","v4","v5","v6","v7","s0","s1","s4","s5","s6","s7","memory"\

// 3 channel calculation

#define SRC3CH_INITIAL()\
	"ld3 {v0.s, v1.s, v2.s}[0], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[1], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[2], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[3], [%0], #12	\n\t"\
	"ld1 {v3.4s}, [%1], #16			\n\t"\
	"ld1 {v4.4s}, [%2], #16			\n\t"\
	"prfm	PLDL1KEEP, [%1,#64]		\n\t"\
	"prfm	PLDL1KEEP, [%2,#64]		\n\t"\
	"prfm	PLDL1KEEP, [%0,#64]		\n\t"\
	"prfm	PLDL1KEEP, [%0,#128]		\n\t"\
	"prfm	PLDL1KEEP, [%0,#192]		\n\t"\
	"fmul v5.4s, v0.4s, v3.4s		\n\t"\
	"fmul v6.4s, v0.4s, v4.4s		\n\t"\
	"fmul v7.4s, v1.4s, v3.4s		\n\t"\
	"fmul v8.4s, v1.4s, v4.4s		\n\t"\
	"fmul v9.4s, v2.4s, v3.4s		\n\t"\
	"fmul v10.4s, v2.4s, v4.4s		\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[0], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[1], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[2], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[3], [%0], #12	\n\t"\
	"ld1 {v3.4s}, [%1], #16			\n\t"\
	"ld1 {v4.4s}, [%2], #16			\n\t"\
	"fmla v5.4s, v0.4s, v3.4s		\n\t"\
	"fmla v6.4s, v0.4s, v4.4s		\n\t"\
	"fmla v7.4s, v1.4s, v3.4s		\n\t"\
	"fmla v8.4s, v1.4s, v4.4s		\n\t"\
	"fmla v9.4s, v2.4s, v3.4s		\n\t"\
	"fmla v10.4s, v2.4s, v4.4s		\n\t"

#define SRC3CH_CALC()\
	"ld3 {v0.s, v1.s, v2.s}[0], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[1], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[2], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[3], [%0], #12	\n\t"\
	"ld1 {v3.4s}, [%1], #16		\n\t"\
	"ld1 {v4.4s}, [%2], #16		\n\t"\
	"fmla v5.4s, v0.4s, v3.4s	\n\t"\
	"fmla v6.4s, v0.4s, v4.4s	\n\t"\
	"fmla v7.4s, v1.4s, v3.4s	\n\t"\
	"fmla v8.4s, v1.4s, v4.4s	\n\t"\
	"fmla v9.4s, v2.4s, v3.4s	\n\t"\
	"fmla v10.4s, v2.4s, v4.4s	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[0], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[1], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[2], [%0], #12	\n\t"\
	"ld3 {v0.s, v1.s, v2.s}[3], [%0], #12	\n\t"\
	"ld1 {v3.4s}, [%1], #16		\n\t"\
	"ld1 {v4.4s}, [%2], #16		\n\t"\
	"fmla v5.4s, v0.4s, v3.4s	\n\t"\
	"fmla v6.4s, v0.4s, v4.4s	\n\t"\
	"fmla v7.4s, v1.4s, v3.4s	\n\t"\
	"fmla v8.4s, v1.4s, v4.4s	\n\t"\
	"fmla v9.4s, v2.4s, v3.4s	\n\t"\
	"fmla v10.4s, v2.4s, v4.4s	\n\t"


#define SRC3CH_FINALIZE()\
	"faddp v8.4s, v8.4s, v8.4s	\n\t"\
	"faddp v8.4s, v8.4s, v8.4s	\n\t"\
	"faddp v5.4s, v5.4s, v5.4s	\n\t"\
	"faddp v5.4s, v5.4s, v5.4s	\n\t"\
	"faddp v6.4s, v6.4s, v6.4s	\n\t"\
	"faddp v6.4s, v6.4s, v6.4s	\n\t"\
	"faddp v7.4s, v7.4s, v7.4s	\n\t"\
	"faddp v7.4s, v7.4s, v7.4s	\n\t"\
	"faddp v9.4s, v9.4s, v9.4s	\n\t"\
	"faddp v9.4s, v9.4s, v9.4s	\n\t"\
	"faddp v10.4s, v10.4s, v10.4s	\n\t"\
	"faddp v10.4s, v10.4s, v10.4s	\n\t"\
	"ldr s0, [%3]			\n\t"\
	"fsub s10, s10, s9		\n\t"\
	"fmadd s1, s10, s0, s9		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4], #4		\n\t"\
	"fsub s8, s8, s7		\n\t"\
	"fmadd s1, s8, s0, s7		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4], #4		\n\t"\
	"fsub s6, s6, s5		\n\t"\
	"fmadd s1, s6, s0, s5		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4]			\n\t"\
	: "+r"(samples), "+r"(coeffs), "+r"(coeffs2), "+r"(ipf)\
	: "r"(res1)\
	: "v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","v10","s0","s1","s5","s6","s7","s8","s9","s10","memory"\

// 4 channel calculation

#define SRC4CH_INITIAL()\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[0], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[1], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[2], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[3], [%0], #16	\n\t"\
	"ld1 {v4.4s}, [%1], #16				\n\t"\
	"ld1 {v5.4s}, [%2], #16				\n\t"\
	"prfm	PLDL1KEEP, [%1,#64]			\n\t"\
	"prfm	PLDL1KEEP, [%2,#64]			\n\t"\
	"prfm	PLDL1KEEP, [%0,#64]			\n\t"\
	"prfm	PLDL1KEEP, [%0,#128]			\n\t"\
	"prfm	PLDL1KEEP, [%0,#192]			\n\t"\
	"prfm	PLDL1KEEP, [%0,#256]			\n\t"\
	"fmul v6.4s, v0.4s, v4.4s			\n\t"\
	"fmul v7.4s, v0.4s, v5.4s			\n\t"\
	"fmul v8.4s, v1.4s, v4.4s			\n\t"\
	"fmul v9.4s, v1.4s, v5.4s			\n\t"\
	"fmul v10.4s, v2.4s, v4.4s			\n\t"\
	"fmul v11.4s, v2.4s, v5.4s			\n\t"\
	"fmul v12.4s, v3.4s, v4.4s			\n\t"\
	"fmul v13.4s, v3.4s, v5.4s			\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[0], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[1], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[2], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[3], [%0], #16	\n\t"\
	"ld1 {v4.4s}, [%1], #16				\n\t"\
	"ld1 {v5.4s}, [%2], #16				\n\t"\
	"fmla v6.4s, v0.4s, v4.4s			\n\t"\
	"fmla v7.4s, v0.4s, v5.4s			\n\t"\
	"fmla v8.4s, v1.4s, v4.4s			\n\t"\
	"fmla v9.4s, v1.4s, v5.4s			\n\t"\
	"fmla v10.4s, v2.4s, v4.4s			\n\t"\
	"fmla v11.4s, v2.4s, v5.4s			\n\t"\
	"fmla v12.4s, v3.4s, v4.4s			\n\t"\
	"fmla v13.4s, v3.4s, v5.4s			\n\t"

#define SRC4CH_CALC()\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[0], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[1], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[2], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[3], [%0], #16	\n\t"\
	"ld1 {v4.4s}, [%1], #16		\n\t"\
	"ld1 {v5.4s}, [%2], #16		\n\t"\
	"fmla v6.4s, v0.4s, v4.4s	\n\t"\
	"fmla v7.4s, v0.4s, v5.4s	\n\t"\
	"fmla v8.4s, v1.4s, v4.4s	\n\t"\
	"fmla v9.4s, v1.4s, v5.4s	\n\t"\
	"fmla v10.4s, v2.4s, v4.4s	\n\t"\
	"fmla v11.4s, v2.4s, v5.4s	\n\t"\
	"fmla v12.4s, v3.4s, v4.4s	\n\t"\
	"fmla v13.4s, v3.4s, v5.4s	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[0], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[1], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[2], [%0], #16	\n\t"\
	"ld4 {v0.s, v1.s, v2.s, v3.s}[3], [%0], #16	\n\t"\
	"ld1 {v4.4s}, [%1], #16		\n\t"\
	"ld1 {v5.4s}, [%2], #16		\n\t"\
	"fmla v6.4s, v0.4s, v4.4s	\n\t"\
	"fmla v7.4s, v0.4s, v5.4s	\n\t"\
	"fmla v8.4s, v1.4s, v4.4s	\n\t"\
	"fmla v9.4s, v1.4s, v5.4s	\n\t"\
	"fmla v10.4s, v2.4s, v4.4s	\n\t"\
	"fmla v11.4s, v2.4s, v5.4s	\n\t"\
	"fmla v12.4s, v3.4s, v4.4s	\n\t"\
	"fmla v13.4s, v3.4s, v5.4s	\n\t"

#define SRC4CH_FINALIZE()\
	"faddp v6.4s, v6.4s, v6.4s	\n\t"\
	"faddp v6.4s, v6.4s, v6.4s	\n\t"\
	"faddp v7.4s, v7.4s, v7.4s	\n\t"\
	"faddp v7.4s, v7.4s, v7.4s	\n\t"\
	"faddp v8.4s, v8.4s, v8.4s	\n\t"\
	"faddp v8.4s, v8.4s, v8.4s	\n\t"\
	"faddp v9.4s, v9.4s, v9.4s	\n\t"\
	"faddp v9.4s, v9.4s, v9.4s	\n\t"\
	"faddp v10.4s, v10.4s, v10.4s	\n\t"\
	"faddp v10.4s, v10.4s, v10.4s	\n\t"\
	"faddp v11.4s, v11.4s, v11.4s	\n\t"\
	"faddp v11.4s, v11.4s, v11.4s	\n\t"\
	"faddp v12.4s, v12.4s, v12.4s	\n\t"\
	"faddp v12.4s, v12.4s, v12.4s	\n\t"\
	"faddp v13.4s, v13.4s, v13.4s	\n\t"\
	"faddp v13.4s, v13.4s, v13.4s	\n\t"\
	"ldr s0, [%3]			\n\t"\
	"fsub s13, s13, s12		\n\t"\
	"fmadd s1, s13, s0, s12		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4], #4		\n\t"\
	"fsub s11, s11, s10		\n\t"\
	"fmadd s1, s11, s0, s10		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4], #4		\n\t"\
	"fsub s9, s9, s8		\n\t"\
	"fmadd s1, s9, s0, s8		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4], #4		\n\t"\
	"fsub s7, s7, s6		\n\t"\
	"fmadd s1, s7, s0, s6		\n\t"\
	"fadd s1, s1, s1		\n\t"\
	"str s1, [%4]			\n\t"\
	: "+r"(samples), "+r"(coeffs), "+r"(coeffs2), "+r"(ipf)\
	: "r"(res1)\
	: "v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","v10","v11","v12","v13","s0","s1","s6","s7","s8","s9","s10","s11","s12","s13","memory"\



static inline void src_filt_1ch_asimd(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{
	asm volatile(
		SRC1CH_INITIAL()
		SRC1CH_CALC()
		SRC1CH_CALC()
		SRC1CH_CALC()
		SRC1CH_CALC()
		SRC1CH_CALC()
		SRC1CH_CALC()
		SRC1CH_CALC()
		SRC1CH_FINALIZE()
	);		
}

static inline void src_filt_2ch_asimd(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{
	asm volatile(
		SRC2CH_INITIAL()
		SRC2CH_CALC()
		SRC2CH_CALC()
		SRC2CH_CALC()
		SRC2CH_CALC()
		SRC2CH_CALC()
		SRC2CH_CALC()
		SRC2CH_CALC()
		SRC2CH_FINALIZE()
	);
}

static inline void src_filt_3ch_asimd(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{
	asm volatile(
		SRC3CH_INITIAL()
		SRC3CH_CALC()
		SRC3CH_CALC()
		SRC3CH_CALC()
		SRC3CH_CALC()
		SRC3CH_CALC()
		SRC3CH_CALC()
		SRC3CH_CALC()
		SRC3CH_FINALIZE()
	);
}

static inline void src_filt_4ch_asimd(float *res1, const float *samples, const float *coeffs, const float *coeffs2, const float * ipf)
{
	asm volatile(
		SRC4CH_INITIAL()
		SRC4CH_CALC()
		SRC4CH_CALC()
		SRC4CH_CALC()
		SRC4CH_CALC()
		SRC4CH_CALC()
		SRC4CH_CALC()
		SRC4CH_CALC()
		SRC4CH_FINALIZE()
	);
}

/*lint -restore */

