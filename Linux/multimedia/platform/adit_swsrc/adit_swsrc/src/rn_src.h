/**
 * \file: rn_src.h
 *
 * ADIT SRC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __RN_SRC_H__
#define __RN_SRC_H__

#include "rn_main.h"

struct rn_src;


/*table needs to be 0-padded only in case an implementation processes 2/4/8/16... samples in parallel
*/
#define FILT_SRC_ALIGN_MAX 8

#define FILT_LEN_SRC_REAL 63

#define FILT_LEN_SRC_MAX FILT_LEN_SRC_(FILT_SRC_ALIGN_MAX)

#define FILT_LEN_SRC FILT_LEN_SRC_MAX

#define FILT_LEN_SRC_(align) ((((FILT_LEN_SRC_REAL-1)/(align))+1)*(align))


typedef void (*srcfunc_t) (float *res1, const float *samp, const float *coef1, const float *coef2, const float *ipf);

#define SRC_CH_MAX 4
struct src_funcs{
	srcfunc_t src_funcs[SRC_CH_MAX];
};

#define SRC_NOT_IMPL (NULL)
/**
 * \brief convert samples
 * \param inst pointer to instance 
 * \param inBuf pointer to input buffer 
 * \param num_in number of input frames 
 * \param p_outBuf pointer to store output buffer pointer
 * \param num_out number of output frames 
 * \param pitch info
 */
void rn_src(struct rn_src *ctx, const float *inBuf, unsigned int num_in, float **p_outBuf, unsigned int num_out, rate_core_pitch_t *pitch);

/**
 * \brief init src
 * \param inst pointer to store pointer to created instance 
 * \param in_frames amount of input frames
 * \param out_frames  amount of input frames
 * \param channels amount of channels
 * \param pitch info
 * \return 0 on success otherwise negative error code
 */
int rn_src_init(struct rn_src **ctx, struct src_funcs *sfuncs, unsigned int in_frames, unsigned int out_frames, unsigned int channels, rate_core_pitch_init_t *pitch);

/**
 * \brief deinit src
 * \param inst pointer to instance 
 * \return 0 on success otherwise negative error code
 */
int rn_src_exit(struct rn_src *ctx);

#endif /* __RN_SRC_H__*/

