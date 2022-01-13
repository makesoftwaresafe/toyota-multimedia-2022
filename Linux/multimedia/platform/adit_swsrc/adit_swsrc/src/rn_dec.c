/**
 * \file: rn_dec.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "rn_dec.h" 
#include "rn_dec2_coeffs.h" 
#include "rn_dec3_coeffs.h" 
#include "rn_dec4_coeffs.h" 
#include "rn_log.h" 


#define DEC_ERR(a...) RN_ERR(a)

#define DECFAC_MAX (24+3)	/*enough for 192kHz->8kHz*/
#define DECFAC_MIN 1		/*dec 1 = bypass */

#define DEC_FCHAIN 5		/*max amount of chained dec filters*/

#define MAX_NATIVE_CHANNELS 4
#define MAX_NATIVE_HISTORY_ROUND_UP 256		/*has to be power of 2*/

#define SAMP_BUF_LEN       (MAX_NATIVE_CHANNELS*MAX_NATIVE_HISTORY_ROUND_UP)	


struct rn_dec_ch_rt{
	float samp_buf[SAMP_BUF_LEN * 2];
	unsigned int samp_buf_pos;
};

struct rn_dec_rt{
	const decfunc_t* funcs;
	const float *coeffs;
	unsigned int use_sample;
	unsigned int num_1ch;
	unsigned int num_2ch;
	unsigned int num_3ch;
	unsigned int num_4ch;
	struct rn_dec_ch_rt *chrt;
	unsigned int dec;	/*decimation factor of filterelement*/
};

struct rn_dec{
	struct dec_funcs *dfuncs;
	unsigned int channels;			/*channel count*/
	unsigned int dec;			/*overall decimation factor*/
	struct rn_dec_rt runtime[DEC_FCHAIN];	/*list of filters with different dec factor*/
	unsigned int num;			/*amount of filters in list*/
	float *outBuf;
};


#define DEC_IMPL_MIN 2
#define DEC_IMPL_MAX 4
static const char num_decs[(DECFAC_MAX-DECFAC_MIN)+1][(DEC_IMPL_MAX-DEC_IMPL_MIN)+1] = {
/*	 2 3 4	*/
/*1*/	{0,0,0},
/*2*/	{1,0,0},
/*3*/	{0,1,0},
/*4*/	{0,0,1},
/*5*/	{1,1,0},
/*6*/	{1,1,0},
/*7*/	{1,0,1},
/*8*/	{1,0,1},
/*9*/	{0,1,1},
/*10*/	{0,1,1},
/*11*/	{0,1,1},
/*12*/	{0,1,1},
/*13*/	{0,0,2},
/*14*/	{0,0,2},
/*15*/	{0,0,2},
/*16*/	{0,0,2},
/*17*/	{1,2,0},
/*18*/	{1,2,0},
/*19*/	{1,1,1},
/*21*/	{1,1,1},
/*21*/	{1,1,1},
/*22*/	{1,1,1},
/*23*/	{1,1,1},
/*24*/	{1,1,1},
/*25*/	{0,3,0},
/*26*/	{0,3,0},
/*27*/	{0,3,0}, /*!!LAST ENTRY MUST MATCH EXACTLY (3*3*3 = 27)*/
};


static int deinit_filter(struct rn_dec_rt *rt)
{
	if (rt->chrt) {
		free(rt->chrt);
		rt->chrt = NULL;
	}
	return 0;
}

static int init_filter(struct rn_dec *e, struct rn_dec_rt *rt, unsigned int decfac, unsigned int channels)
{
	unsigned int num_filters = 0;
	unsigned int i;

	/*chain for desired amount of channels depending on native DEC support*/
	rt->num_4ch = channels/4;
	channels -= rt->num_4ch*4;
	num_filters += rt->num_4ch;
	rt->num_3ch = channels/3;
	channels -= rt->num_3ch*3;
	num_filters += rt->num_3ch;
	rt->num_2ch = channels/2;
	channels -= rt->num_2ch*2;
	num_filters += rt->num_2ch;
	rt->num_1ch = channels/1;
	channels -= rt->num_1ch*1;
	num_filters += rt->num_1ch;

	rt->chrt = calloc(num_filters, sizeof(struct rn_dec_ch_rt));
	if (rt->chrt == NULL) {
		DEC_ERR("failed alloc filters");
		return -ENOMEM;
	}

	for (i=0; i<num_filters; i++) {
	   	memset(rt->chrt[i].samp_buf, 0, sizeof(rt->chrt[i].samp_buf));
		rt->chrt[i].samp_buf_pos = 0;/*pre decrement*/
	}
	switch(decfac) {
		case 2:
			rt->coeffs = dec2_coeffs;
			rt->funcs = e->dfuncs->dec2_funcs;
			break;
		case 3:
			rt->coeffs = dec3_coeffs;
			rt->funcs = e->dfuncs->dec3_funcs;
			break;
		case 4:
			rt->coeffs = dec4_coeffs;
			rt->funcs = e->dfuncs->dec4_funcs;
			break;
		default:
			return -EINVAL;
	}
	rt->use_sample = 1; /*default*/
	rt->dec = decfac;
	RN_LOG("setup dec element %d", decfac);

	return 0;
}

/**
 * \brief deinit dec
 * @see rn_dec.h
 */
int rn_dec_exit(struct rn_dec* inst)
{
	struct rn_dec *e = (struct rn_dec *) inst;
	unsigned int i;
	for(i=0;i<e->num;i++)
		deinit_filter(&e->runtime[i]);
	if (e->outBuf) {
		free(e->outBuf);
		e->outBuf = NULL;
	}
	free(e);
	return 0;
}


/**
* \brief checks availability of a desired dec-factor. It returns the requested or the next greater available decfac.
 * @see rn_dec.h
*/
int rn_dec_avail( unsigned int decfac, unsigned int channels)
{
	unsigned int i;
	unsigned int decfac_result=1;
	unsigned int num;
	channels = channels;/*no channel limitation*/

	if ((decfac > DECFAC_MAX) || (decfac < DECFAC_MIN)){
		return -EINVAL;
	}
	for (i = DEC_IMPL_MAX; i >= DEC_IMPL_MIN; i--) {
		num = num_decs[decfac-DECFAC_MIN][i-DEC_IMPL_MIN];
		while (num--)
			decfac_result*= i;
	}
	return decfac_result >= decfac?(int)decfac_result:-ERANGE;
}

/**
* \brief init decimation with requested decimation factor
 * @see rn_dec.h
*/
int rn_dec_init(struct rn_dec **inst, struct dec_funcs *dfuncs, unsigned int in_frames, unsigned int out_frames, unsigned int channels, rate_core_pitch_init_t *pitch)
{
	struct rn_dec *e;
	unsigned int decfac_result = 1;
	unsigned int i;
	unsigned int num;
	unsigned int decfac_requested = in_frames / out_frames;
	int err;

	if (pitch == NULL) {
		DEC_ERR("no pitch info given");
		return -EINVAL;
	}
	if ((in_frames % out_frames)!=0 ){
		DEC_ERR("decimation factor must be integer %d/%d)", in_frames, out_frames);
		return -EINVAL;
	}

	if ((decfac_requested > DECFAC_MAX) || (decfac_requested < DECFAC_MIN)){
		DEC_ERR("decimation factor %d not supported (max %d)", decfac_requested, DECFAC_MAX);
		return -EINVAL;
	}
	e = calloc(1,sizeof(struct rn_dec));
	if (e == NULL) {
		DEC_ERR("no mem");
		return -ENOMEM;
	}
	e->dfuncs = dfuncs;
	e->outBuf = malloc((out_frames+pitch->out_range) *channels*sizeof(float));
	if (e->outBuf == NULL) {
		RN_ERR("dec: failed alloc buf");
		free(e);
		return -ENOMEM;
	}

	e->channels = channels;
	e->dec = decfac_requested;

	/*setup dec chain - start with highest dec*/
	for (i = DEC_IMPL_MAX; i >= DEC_IMPL_MIN; i--) {
		num = num_decs[decfac_requested-DECFAC_MIN][i-DEC_IMPL_MIN];
		while (num--) {
			if (e->num >= DEC_FCHAIN) {
				DEC_ERR("exceed max filter chain len %d", DEC_FCHAIN);
				rn_dec_exit(e);
				return -EINVAL;
			}
			err = init_filter(e, &e->runtime[e->num], i, channels);
			if (err){
				DEC_ERR("failed init filter");
				rn_dec_exit(e);
				return err;	
			}
			e->num++;
			decfac_result*= i;
		}
	}
	if (decfac_result != decfac_requested) {
		DEC_ERR("failed setup dec %d", decfac_requested);
		rn_dec_exit(e);	
		return -EINVAL;
	}

	for (i=0;i<e->num;i++)
		e->runtime[i].use_sample = e->runtime[i].dec;

	RN_LOG("overall decfac %d", decfac_requested);

	*inst = e;
	return 0;
}


#define GET_SAMPLE(r, in) do{(r)->samp_buf_pos = ((r)->samp_buf_pos - 1) & (SAMP_BUF_LEN-1);\
			(r)->samp_buf[(r)->samp_buf_pos] = (r)->samp_buf[(r)->samp_buf_pos + SAMP_BUF_LEN] = *((in)++);}while(0)

static void rn_decX(struct rn_dec_rt *rt, float *inBuf,float *outBuf, unsigned int num_in)
{
	unsigned int i;
	unsigned int chrt;
	unsigned int cnt = 0;
	while(num_in--) {
		cnt++;
		chrt = 0;
		for (i=0; i<rt->num_4ch; i++,chrt++) {
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			if(cnt==rt->use_sample){
				rt->funcs[3](outBuf, &rt->chrt[chrt].samp_buf[rt->chrt[chrt].samp_buf_pos], rt->coeffs);
				outBuf+=4;
			}
		}
		for (i=0; i<rt->num_3ch; i++,chrt++) {
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			if(cnt==rt->use_sample){
				rt->funcs[2](outBuf, &rt->chrt[chrt].samp_buf[rt->chrt[chrt].samp_buf_pos], rt->coeffs);
				outBuf+=3;
			}
		}
		for (i=0; i<rt->num_2ch; i++,chrt++) {
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			if(cnt==rt->use_sample){
				rt->funcs[1](outBuf, &rt->chrt[chrt].samp_buf[rt->chrt[chrt].samp_buf_pos], rt->coeffs);
				outBuf+=2;
			}
		}
		for (i=0; i<rt->num_1ch; i++,chrt++) {
			GET_SAMPLE(&rt->chrt[chrt], inBuf);
			if(cnt==rt->use_sample){
				rt->funcs[0](outBuf, &rt->chrt[chrt].samp_buf[rt->chrt[chrt].samp_buf_pos], rt->coeffs);
				outBuf+=1;
			}
		}
		if(cnt==rt->use_sample)
			cnt=0;
	}
}

/**
 * \brief convert samples
 * @see rn_dec.h
 */
void rn_dec(struct rn_dec *inst, float *inBuf, unsigned int num_in, float **p_outBuf, unsigned int num_out)
{
	unsigned int i;
	if (!inst->num) {
		*p_outBuf = inBuf;
		return;
	}
	*p_outBuf = inst->outBuf;
	num_out = num_out;
	for (i = 0; i < inst->num; i++) {
		/*intermediate dec write to inBuf, last one to real output buffer*/
		rn_decX(&inst->runtime[i],inBuf, i<(inst->num-1)?inBuf:inst->outBuf, num_in);
		num_in/=inst->runtime[i].use_sample;
	}
}

