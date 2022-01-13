/**
 * \file: rn_dec.h
 *
 * Decimation filter.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __RN_DEC_H__
#define __RN_DEC_H__

#include "rn_main.h"

struct rn_dec;

typedef void (*decfunc_t)(float *result1, const float *samples, const float *coeffs);

/*table needs to be 0-padded only in case an implementation processes 2/4/8/16... samples in parallel
*/
#define FILT_ALIGN_MAX 8

#define FILT_LEN_DEC2_REAL 123
#define FILT_LEN_DEC3_REAL 186
#define FILT_LEN_DEC4_REAL 246

#define FILT_LEN_DEC2_MAX FILT_LEN_DEC(2, FILT_ALIGN_MAX) // ->128 for align 8
#define FILT_LEN_DEC3_MAX FILT_LEN_DEC(3, FILT_ALIGN_MAX) // ->192 for align 8
#define FILT_LEN_DEC4_MAX FILT_LEN_DEC(4, FILT_ALIGN_MAX) // ->248 for align 8

#define FILT_LEN_DEC2 FILT_LEN_DEC2_MAX
#define FILT_LEN_DEC3 FILT_LEN_DEC3_MAX
#define FILT_LEN_DEC4 FILT_LEN_DEC4_MAX

#define FILT_LEN_DEC(order, align) ((((FILT_LEN_DEC##order##_REAL-1)/(align))+1)*(align))


#define DEC_CH_MAX 4
struct dec_funcs{
	decfunc_t dec2_funcs[DEC_CH_MAX];
	decfunc_t dec3_funcs[DEC_CH_MAX];
	decfunc_t dec4_funcs[DEC_CH_MAX];
};


/**
* \brief checks availability of a desired dec-factor. It returns the requested or the next greater available decfac.
* This call is optional before rn_dec_init().
* \param decfac requested dec-factor
* \param channels amount of channels to use
* \return return: <0 error, >= decfac: available decfac
*/
int rn_dec_avail(unsigned int decfac, unsigned int channels);

/**
* \brief init decimation with requested decimation factor
* \param inst pointer returning context
* \param in_frames requested in_frames to be converted to out_frames ->decfactor = in/out
* \param out_frames requested in_frames to be converted to out_frames ->decfactor = in/out
* \param channels amount of channels to use
* \param pitch info
* \return <0 error, 0: requested decfac was accepted
*/
int rn_dec_init(struct rn_dec **inst, struct dec_funcs *dfuncs, unsigned int in_frames, unsigned int out_frames, unsigned int channels, rate_core_pitch_init_t *pitch);

/**
 * \brief deinit dec
 * \param inst pointer to instance 
 * \return 0 on success otherwise negative error code
 */
int rn_dec_exit(struct rn_dec *inst);

/**
 * \brief convert samples
 * It is assumed that all input samples get consumed and an output matching the desired decimation is produced.
 * Number of input and output frames must have an alignment equal to decfac.
 *
 * \param inst pointer to instance 
 * \param inBuf pointer to input buffer 
 * \param num_in number of input frames 
 * \param p_outBuf pointer to store output buffer pointer
 * \param num_out number of output frames 
 */
void rn_dec(struct rn_dec *inst,  float *inBuf, unsigned int num_in, float **p_outBuf, unsigned int num_out);

#endif /* __RN_DEC_H__*/

