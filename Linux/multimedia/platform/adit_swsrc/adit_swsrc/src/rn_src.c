/**
 * \file: rn_src.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "rn_src.h" 
#include "rn_src_coeffs.h" 

#include "rn_log.h" 


#define SAMPPP_B            13  /*linear interpolation bits (linear interpolations between polyphases)*/
#define AMOUNT_POLYPHASES_B  7
#define SAMP_BUF_LEN       256	/*has to be power of 2*/



//#define RN_VALIDATE_SIZE /*activate to check the real processed number of frames - only for debug -> this does an exit() on error*/
#ifdef TRACE_OUT
#define DBG_(c)	RN_LOG("accu d:%+08.4f, masked:%ld, int:%7ld old:%7d PPL: %3d/%3d PPU: %3d/%3d /base %3d inc %1d new %1d pos %3d ->sin %d spp %d ",\
		ctx->ipf_accu_d,(long)(ctx->ipf_accu_d) & ((1 << (AMOUNT_POLYPHASES_B + SAMPPP_B))-1),ipf_accu,ctx->ipf_accu_int_old,\
		PPLOW(ipf_accu), PPLOW(ctx->ipf_accu_int_old), PPUPP(ipf_accu), PPUPP(ctx->ipf_accu_int_old),\
		ctx->rt->base,\
		base_inc,\
		frames_inc,\
		ctx->rt->samp_buf_pos,\
		(0 - ((ctx->rt->base + (PPLOW(ipf_accu) > PPLOW(ctx->ipf_accu_int_old) ? ctx->frames_inc_A:ctx->frames_inc_A+1)) & (SAMP_BUF_LEN-1))) & (SAMP_BUF_LEN-1),\
		ipf_accu & SPPMASK\
		);
#else
#define DBG_(c)
#endif

/*****************************************************************************
 * sample rate conversion
 *****************************************************************************/
#define IPF_ACCU_UPTHRESHOLD ((1<<(AMOUNT_POLYPHASES_B + SAMPPP_B + 1))-1)
#define IPF_ACCU_DEC ((1<<(AMOUNT_POLYPHASES_B + SAMPPP_B + 1))-1)

#define IPF_ACCU_LOWTHRESHOLD (-(1<<(AMOUNT_POLYPHASES_B + SAMPPP_B + 1)))
#define IPF_ACCU_INC ((1<<(AMOUNT_POLYPHASES_B + SAMPPP_B + 1)))

#define PPMASK ((1 << AMOUNT_POLYPHASES_B)-1)   /* mask for polyphase calcualtion for audio_synchronisation */
#define SPPMASK ((1 << SAMPPP_B)-1)

#define EINS_DURCH_IPFLEN (1.0/(1<<SAMPPP_B))

#define PPLOW(acc) ((acc) >> SAMPPP_B) 			/* polyphase, which is out if ipf is at maximum */
#define PPUPP(acc) ((((acc) >> SAMPPP_B) + 1) & PPMASK)	/* polyphase, which is out if ipf is zero */

#define SPPM(c) ((c)->syncppm[(c)->sppmi])

#define GET_SAMPLE(r, in) do {r.samp_buf_pos = (r.samp_buf_pos - 1) & (SAMP_BUF_LEN-1);\
r.samp_buf[r.samp_buf_pos] = r.samp_buf[r.samp_buf_pos + SAMP_BUF_LEN] = *(in++); }while(0)


struct rn_src_runtime{
	float samp_buf[SAMP_BUF_LEN * 2];
	int   base;
	unsigned int  samp_buf_pos;       /* pointer where FDX writes into sample_buffer */
};

#define SPPMI_DEFAULT 0
#define SPPMI_ALT 1

struct rn_src{
	double syncppm[2];
	unsigned int adjustable;
	unsigned int sppmi;
	unsigned int up;
	struct rn_src_runtime *rt;
	unsigned int frames_inc_A;
	unsigned int frames_inc_B;
	double   ipf_accu_d;
	unsigned int   ipf_accu_int_old;
	float *out_buf;
	unsigned int num_4ch;
	unsigned int num_3ch;
	unsigned int num_2ch;
	unsigned int num_1ch;
	struct src_funcs *sfuncs;
};

static void rn_src_sppm_chg(struct rn_src *e, int is_initial);


/**
 * \brief convert samples
 * @see rn_src.h
 * \param num_in number of input frames 
 * \param num_out number of output frames 
 */
void rn_src(struct rn_src *ctx, const float *inBuf, unsigned int num_in, float **p_outBuf, unsigned int num_out, rate_core_pitch_t *pitch)
{
	unsigned int base_inc;
	unsigned int frames_inc;
	unsigned int ipf_accu;
	unsigned int i;
#ifdef RN_VALIDATE_SIZE
	unsigned int _num_in = 0;
	unsigned int _num_out = num_out;
#endif
	unsigned int rt;
	float        ipf;	/* interpolationfactor (0..255)/256  */
	float*       pSA;	/* base invert*/
	const float* pCoef1;
	const float* pCoef2;
	float*       outBuf = ctx->out_buf;

	*p_outBuf = ctx->out_buf;
	if((num_in == num_out)&&(!ctx->adjustable)) {
		*p_outBuf = (float*)inBuf;
		return;
	}

	if (pitch->in_pitch || pitch->out_pitch) {
		ctx->sppmi = SPPMI_ALT;
		ctx->syncppm[SPPMI_ALT] = (1<<(SAMPPP_B + AMOUNT_POLYPHASES_B))*((double)num_in/num_out-1);
		rn_src_sppm_chg(ctx, 0);
	} else {
		ctx->sppmi = SPPMI_DEFAULT;
	}
	ctx->ipf_accu_d = 0;
	while (num_out--) {
		if (ctx->up) {
			ipf_accu = (int)(ctx->ipf_accu_d+0.5) & ((1 << (AMOUNT_POLYPHASES_B + SAMPPP_B))-1);
			base_inc = PPLOW(ipf_accu) > PPLOW(ctx->ipf_accu_int_old) ? ctx->frames_inc_A:ctx->frames_inc_B;
			frames_inc = PPUPP(ipf_accu) > PPUPP(ctx->ipf_accu_int_old) ? ctx->frames_inc_A:ctx->frames_inc_B;
		} else {
			ipf_accu = (int)(ctx->ipf_accu_d-0.5) & ((1 << (AMOUNT_POLYPHASES_B + SAMPPP_B))-1);
			base_inc = PPLOW(ipf_accu) >= PPLOW(ctx->ipf_accu_int_old) ? ctx->frames_inc_A:ctx->frames_inc_B;
			frames_inc = PPUPP(ipf_accu) >= PPUPP(ctx->ipf_accu_int_old) ? ctx->frames_inc_A:ctx->frames_inc_B;
		}

		DBG_(ctx);

		while (base_inc--) {
			rt =0;
			for (i=0; i<ctx->num_4ch; i++,rt++)
				ctx->rt[rt].base = (ctx->rt[rt].base + 4) & (SAMP_BUF_LEN-1);
			for (i=0; i<ctx->num_3ch; i++,rt++)
				ctx->rt[rt].base = (ctx->rt[rt].base + 3) & (SAMP_BUF_LEN-1);
			for (i=0; i<ctx->num_2ch; i++,rt++)
				ctx->rt[rt].base = (ctx->rt[rt].base + 2) & (SAMP_BUF_LEN-1);
			for (i=0; i<ctx->num_1ch; i++,rt++)
				ctx->rt[rt].base = (ctx->rt[rt].base + 1) & (SAMP_BUF_LEN-1);
		}

#ifdef RN_VALIDATE_SIZE
		_num_in += frames_inc;
#endif
		while (frames_inc--) {
			rt =0;
			for (i=0; i<ctx->num_4ch; i++,rt++) {
				GET_SAMPLE(ctx->rt[rt], inBuf);
				GET_SAMPLE(ctx->rt[rt], inBuf);
				GET_SAMPLE(ctx->rt[rt], inBuf);
				GET_SAMPLE(ctx->rt[rt], inBuf);
			}
			for (i=0; i<ctx->num_3ch; i++,rt++) {
				GET_SAMPLE(ctx->rt[rt], inBuf);
				GET_SAMPLE(ctx->rt[rt], inBuf);
				GET_SAMPLE(ctx->rt[rt], inBuf);
			}
			for (i=0; i<ctx->num_2ch; i++,rt++) {
				GET_SAMPLE(ctx->rt[rt], inBuf);
				GET_SAMPLE(ctx->rt[rt], inBuf);
			}
			for (i=0; i<ctx->num_1ch; i++,rt++)
				GET_SAMPLE(ctx->rt[rt], inBuf);
		}

		ipf = (float)((ipf_accu & SPPMASK) * EINS_DURCH_IPFLEN);
		pCoef2 = &src_coeffs[PPLOW(ipf_accu)+1][0];	/* dont change */
		pCoef1 = &src_coeffs[PPLOW(ipf_accu)][0];	/*  order      */
		rt =0;
		for (i=0; i<ctx->num_4ch; i++,rt++) {
			pSA = &ctx->rt[rt].samp_buf[(0 - ctx->rt[rt].base) & (SAMP_BUF_LEN-1)];
			ctx->sfuncs->src_funcs[3](outBuf, pSA, pCoef1, pCoef2, (const float *)&ipf);
			outBuf+=4;
		}
		for (i=0; i<ctx->num_3ch; i++,rt++) {
			pSA = &ctx->rt[rt].samp_buf[(0 - ctx->rt[rt].base) & (SAMP_BUF_LEN-1)];
			ctx->sfuncs->src_funcs[2](outBuf, pSA, pCoef1, pCoef2, (const float *)&ipf);
			outBuf+=3;
		}
		for (i=0; i<ctx->num_2ch; i++,rt++) {
			pSA = &ctx->rt[rt].samp_buf[(0 - ctx->rt[rt].base) & (SAMP_BUF_LEN-1)];
			ctx->sfuncs->src_funcs[1](outBuf, pSA, pCoef1, pCoef2, (const float *)&ipf);
			outBuf+=2;
		}
		for (i=0; i<ctx->num_1ch; i++,rt++) {
			pSA = &ctx->rt[rt].samp_buf[(0 - ctx->rt[rt].base) & (SAMP_BUF_LEN-1)];
			ctx->sfuncs->src_funcs[0](outBuf, pSA, pCoef1, pCoef2, (const float *)&ipf);
			outBuf+=1;
		}
		ctx->ipf_accu_int_old = ipf_accu;
		ctx->ipf_accu_d += SPPM(ctx);
		if (ctx->ipf_accu_d > IPF_ACCU_UPTHRESHOLD)
			ctx->ipf_accu_d -= IPF_ACCU_DEC;
		else if (ctx->ipf_accu_d < IPF_ACCU_LOWTHRESHOLD)
			ctx->ipf_accu_d += IPF_ACCU_INC;
	}
#ifdef RN_VALIDATE_SIZE
	if (_num_in != num_in) {
		RN_ERR("MISMATCH num_in %d num_out %d ->processed in %d ",num_in, _num_out, _num_in);
		exit(1);
	}
#endif
}


/**
 * \brief deinit src
 * @see rn_src.h
 * \return 0 on success otherwise negative error code
 */
int rn_src_exit(struct rn_src *inst)
{
	if (inst->rt) {
		free(inst->rt);
		inst->rt = NULL;
	}
	if (inst->out_buf) {
		free(inst->out_buf);
		inst->out_buf = NULL;
	}
	free(inst);
	return 0;
}

/**
 * \brief activities on new/initial sppm change
 *
*/
static void rn_src_sppm_chg(struct rn_src *e, int is_initial)
{
	if (is_initial) {
		e->ipf_accu_int_old = 0; /*ORIG: PPMASK << SAMPPP_B*/
		e->ipf_accu_d = 0; /*ORIG: (1 << ( (int)AMOUNT_POLYPHASES_B + SAMPPP_B))  - (1<<SAMPPP_B);*/
	}

	if (SPPM(e) <= 0) {
		e->up = 1;
		e->frames_inc_A = 0;
		e->frames_inc_B = 1;
	} else {
		e->up = 0;
		e->frames_inc_A = 1;
		e->frames_inc_B = 2;
	}
}

/**
 * \brief init src 
 * @see rn_src.h
 * \return 0 on success otherwise negative error code
 */
int rn_src_init(struct rn_src **inst, struct src_funcs *sfuncs, unsigned int in_frames, unsigned int out_frames, unsigned int channels, rate_core_pitch_init_t *pitch)
{
	struct rn_src *e;
	unsigned int i;
	unsigned int sum_filters=0;

	if (pitch == NULL) {
		RN_ERR("no pitch info given");
		return -EINVAL;
	}
	e = calloc(1,sizeof(struct rn_src));
	if (e == NULL) {
		RN_ERR("src: failed alloc mem");
		return -ENOMEM;
	}
	e->sfuncs = sfuncs;
	e->adjustable = pitch->out_range||pitch->in_range;
	e->out_buf = malloc((out_frames+pitch->out_range)*channels*sizeof(float));
	if (e->out_buf == NULL) {
		RN_ERR("src: failed alloc buf");
		free(e);
		return -ENOMEM;
	}
	e->sppmi = SPPMI_DEFAULT;
	e->syncppm[SPPMI_DEFAULT] = (1<<(SAMPPP_B + AMOUNT_POLYPHASES_B))*((double)in_frames/out_frames-1);

	/*chain for desired amount of channels depending on native SRC support*/
	/*TODO: combine from LUT ? e.g. 4 channels currently result in 3+1 instead of 2+2 ->fails if e.g no 1channel support*/
	if(e->sfuncs->src_funcs[3]!= SRC_NOT_IMPL) {
		e->num_4ch = channels/4;
		channels-=e->num_4ch*4;
		sum_filters+=e->num_4ch;
	}
	if(e->sfuncs->src_funcs[2]!= SRC_NOT_IMPL) {
		e->num_3ch = channels/3;
		channels-=e->num_3ch*3;
		sum_filters+=e->num_3ch;
	}
	if(e->sfuncs->src_funcs[1]!= SRC_NOT_IMPL) {
		e->num_2ch = channels/2;
		channels-=e->num_2ch*2;
		sum_filters+=e->num_2ch;
	}
	if(e->sfuncs->src_funcs[0]!= SRC_NOT_IMPL) {
		e->num_1ch = channels/1;
		channels-=e->num_1ch*1;
		sum_filters+=e->num_1ch;
	}
	if (channels != 0) {
		RN_ERR("src: failed combine all channels - %d channels left", channels);
		free(e->out_buf);
		free(e);
		return -ENOMEM;
	}

	e->rt = calloc(sum_filters, sizeof(struct rn_src_runtime));
	if (e->rt == NULL) {
		RN_ERR("src: failed alloc filters");
		free(e->out_buf);
		free(e);
		return -ENOMEM;
	}

	for (i=0; i<sum_filters; i++) {
	   	memset(e->rt[i].samp_buf, 0, sizeof(e->rt[i].samp_buf));
/*TODO check initial values !!!*/
		e->rt[i].samp_buf_pos = 0; /*pre decrement*/ /*ORIG: 1+2;*/
		e->rt[i].base = 0; /*ORIG: SAMP_BUF_LEN - 1;*/
	}

	rn_src_sppm_chg(e, 1);
	RN_LOG("in:%d out:%d sppm:%f %d-4CH %d-3CH  %d-2CH %d-1CH",in_frames, out_frames,
			SPPM(e), e->num_4ch, e->num_3ch, e->num_2ch, e->num_1ch);
	*inst = e;
	return 0;
}

