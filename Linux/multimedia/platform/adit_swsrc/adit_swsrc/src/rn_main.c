/**
 * \file: rn_main.c
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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "rate_core_if.h"
#include "arch_selector.h"
#include "rn_dec.h" 
#include "rn_src.h" 
#include "rn_log.h" 


#ifndef THRES_DOWNSAMP_WO_DEC
#define THRES_DOWNSAMP_WO_DEC 0	/*downsampling in percent without dec filter (max ~10%)*/
#endif
#ifndef THRES_DOWNSAMP_W_DEC
#define THRES_DOWNSAMP_W_DEC 0	/*downsampling in percent with dec filter (max ~40%)*/
#endif
#define DEC_MIN 2

static void rate_error_default(const char *file, int line, const char *function, int err, const char *fmt, ...);
void (*rate_error)(const char *file, int line, const char *function, int err, const char *fmt, ...) = rate_error_default;
static void rate_log_default(const char *file, int line, const char *function, int err, const char *fmt, ...);
void (*rate_log)(const char *file, int line, const char *function, int err, const char *fmt, ...) = rate_log_default;


struct rate_core{
#ifdef SRC_FILE_OUTPUT
	int fd_out;
#endif
#ifdef SRC_FILE_INPUT
	int fd_in;
#endif
	unsigned int adjust_in_max;
	unsigned int adjust_out_max;
	unsigned int adjustable;
	unsigned int channels;
	unsigned int decfac;	/*decimation factor*/
	float *in_buf;		/*in buffer*/
	float div;
	const char* selected_optimization;
	struct src_funcs sfuncs;
	struct dec_funcs dfuncs;
	struct rn_dec *dec_inst;
	struct rn_src *src_inst;
};

/*calculate resulting in_frames from given (theoretical) amount of out_frames*/
static unsigned int rate_core_in_frames(void *ctx, unsigned int frames)
{
	struct rate_core *inst = (struct rate_core *)ctx;
	if (!frames)
		return 0;
	return (unsigned int)(frames*inst->div);
}

/*calculate resulting out_frames from given (theoretical) amount of in_frames*/
static unsigned int rate_core_out_frames(void *ctx, unsigned int frames)
{
	struct rate_core *inst = (struct rate_core *)ctx;
	if (!frames)
		return 0;
	return (unsigned int)(frames/inst->div);
}

static inline void S16_TO_FLOAT(unsigned int cnt, float *dst, int16_t *src)
{	
	unsigned int i;
	for (i=0; i<cnt; i++)
		dst[i] = src[i];
}


static inline void FLOAT_TO_S16(unsigned int cnt, int16_t *dst, float *src)
{
	unsigned int i;
	for(i=0; i<cnt; i++) {
		if(src[i]>32767 ){
			dst[i] = 32767;
		} else if (src[i]<-32768) {
			dst[i] = -32768;
		} else
			dst[i]= (int16_t)src[i];
	}
}


static int rate_core_convert(void *ctx, struct rate_core_convert_info *info)
{
	struct rate_core * inst = (struct rate_core *)ctx;
	rate_core_pitch_t pitch = {.in_pitch = info->in.pitch_frames, .out_pitch = info->out.pitch_frames};
	float *src_out_buf;
	float *dec_out_buf;

	if (abs(pitch.in_pitch) > (int)inst->adjust_in_max) {
		RN_ERR("in pitch overflow ( %d > %d )", pitch.in_pitch, inst->adjust_in_max);
		return -ERANGE;
	}
	if (abs(pitch.out_pitch) > (int)inst->adjust_out_max) {
		RN_ERR("out pitch overflow ( %d > %d )", pitch.out_pitch, inst->adjust_out_max);
		return -ERANGE;
	}

#ifdef SRC_FILE_INPUT
	if(inst->fd_in>=0)
		write(inst->fd_in,info->in.buf,info->in.frames*inst->channels*sizeof(int16_t) );
#endif
	if((info->in.frames==info->out.frames)&&(!inst->adjustable)) {
		memcpy(info->out.buf, info->in.buf, info->in.frames*inst->channels*sizeof(int16_t));
#ifdef SRC_FILE_OUTPUT
		if(inst->fd_out>=0)
			write(inst->fd_out,info->out.buf,info->out.frames*inst->channels*sizeof(int16_t) );
#endif
		return 0;	
	}
	S16_TO_FLOAT(info->in.frames*inst->channels, inst->in_buf, info->in.buf);
	rn_src(inst->src_inst, inst->in_buf, info->in.frames, &src_out_buf, info->out.frames* inst->decfac, &pitch);
	rn_dec(inst->dec_inst, src_out_buf, info->out.frames * inst->decfac, &dec_out_buf, info->out.frames);
	FLOAT_TO_S16(info->out.frames*inst->channels, info->out.buf, dec_out_buf);
#ifdef SRC_FILE_OUTPUT
	if(inst->fd_out>=0)
		write(inst->fd_out,info->out.buf,info->out.frames*inst->channels*sizeof(int16_t) );
#endif
	return 0;
}

static int calc_decfactor(struct rate_core *inst, unsigned int in_frames, unsigned int out_frames)
{
	unsigned int decfac_requested = 0;
	unsigned int decfac_result = 0;
	unsigned int in_frames_min;
	int err;
	inst->decfac = 1;

	if (out_frames >= in_frames)
		return 1;

	/*no decimation needed?*/
	if (out_frames >= (in_frames - ((in_frames * THRES_DOWNSAMP_WO_DEC) / 100)))
		return 1;

	/*1)try ideal dec*/
	if ((in_frames % out_frames) == 0) {
		decfac_requested = in_frames / out_frames;
		err = rn_dec_avail(decfac_requested, inst->channels);
		if ((err > 0) && ((unsigned int)err == decfac_requested))
			decfac_result = (unsigned int)err;
	}

	/*2) try smallest possible dec (at least dec 2)*/
	if (decfac_result == 0) {
		in_frames_min = in_frames -((in_frames * THRES_DOWNSAMP_W_DEC) / 100);
		decfac_requested = in_frames_min / out_frames;
		if ((in_frames_min % out_frames)!= 0)
			decfac_requested++;
		if (decfac_requested < DEC_MIN)
			decfac_requested = DEC_MIN;
		err = rn_dec_avail(decfac_requested, inst->channels);
		if ((err < 0) || ((unsigned int)err < decfac_requested)) {
			RN_ERR("dec_avail %d failed with err %d",decfac_requested, err);
			return -EINVAL;
		}
		decfac_result = (unsigned int)err;
	}
	return decfac_result;
}



static int rate_core_set_log(void *ctx __attribute__ ((unused)), int is_err, void(*log_fkt)(const char *file, int line, const char *function, int err, const char *fmt, ...))
{
	ctx = ctx;
	if (is_err)
		rate_error = log_fkt;
	else
		rate_log = log_fkt;
	return 0;
}



static int rate_core_init(void *ctx, struct rate_core_init_info *info)
{
	struct rate_core *inst = (struct rate_core *)ctx;
	int err;
	rate_core_pitch_init_t pitch = PITCH_INIT_NO_PITCH;

	inst->channels = info->channels;
	inst->div = (float)(info->in.period_size)/info->out.period_size;		
	inst->adjustable = info->out.pitch_frames_max||info->in.pitch_frames_max;
	inst->adjust_in_max = abs(info->in.pitch_frames_max);
	inst->adjust_out_max = abs(info->out.pitch_frames_max);

#ifdef SRC_DOWNSAMPLING_BROKEN
	err = calc_decfactor(inst, info->in.period_size, info->out.period_size - info->out.pitch_frames_max);
#else
	err = calc_decfactor(inst, info->in.period_size, info->out.period_size);
#endif
	if (err < 0)
		return err;
	inst->decfac = (unsigned int)err;

	inst->in_buf = malloc(info->in.period_size *inst->channels*sizeof(float));
	if (!inst->in_buf) {
		RN_ERR("failed alloc memory for sample buffer");
		return -ENOMEM;
	}

	pitch.out_range = info->out.pitch_frames_max;
	pitch.in_range = info->in.pitch_frames_max;
	err = rn_dec_init(&inst->dec_inst, &inst->dfuncs , info->out.period_size*inst->decfac, info->out.period_size, inst->channels, &pitch);
	if (err < 0) {
		RN_ERR("dec_init %d failed with err %d",inst->decfac, err);
		return err;
	}

	pitch.out_range = info->out.pitch_frames_max*inst->decfac;
	pitch.in_range = info->in.pitch_frames_max;
	err = rn_src_init(&inst->src_inst, &inst->sfuncs, info->in.period_size, info->out.period_size*inst->decfac, inst->channels, &pitch);
	if (err < 0) {
		RN_ERR("failed init src (err %d)",err);
		return err;
	}

	RN_LOG("in:%d out:%d srcout %d decfac %d div %f",
		info->in.period_size, info->out.period_size, info->out.period_size * inst->decfac , inst->decfac, inst->div);

	return 0;
}

static int rate_core_info(void *ctx, char* buf, int len)
{
	struct rate_core *inst = (struct rate_core *)ctx;
	return snprintf(buf, len, "%s %s","ADIT SWSRC",inst->selected_optimization);
}

static int rate_core_close(void*ctx)
{
	struct rate_core * inst = (struct rate_core *)ctx;

	if(inst->dec_inst) {
		rn_dec_exit(inst->dec_inst);
		inst->dec_inst = NULL;
	}
	if(inst->src_inst) {
		rn_src_exit(inst->src_inst);
		inst->src_inst = NULL;
	}
	if(inst->in_buf) {
		free(inst->in_buf);
		inst->in_buf = NULL;
	}
#ifdef SRC_FILE_OUTPUT
	if(inst->fd_out>=0) {
		close(inst->fd_out);
		inst->fd_out = -1;
	}
#endif
#ifdef SRC_FILE_INPUT
	if(inst->fd_in>=0) {
		close(inst->fd_in);
		inst->fd_in = -1;
	}
#endif
	free(inst);
	return 0;
}

static void rate_error_default(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
/* PRQA: Lint Message 530: va_list args gets initialized via va_start */
/*lint -save -e530*/
	va_list arg;
	va_start(arg, fmt);
	fprintf(stderr, "RATE CORE lib %s:%i:(%s) ", file, line, function);
	vfprintf(stderr, fmt, arg);
	if (err)
		fprintf(stderr, ": %s", strerror(err));
	putc('\n', stderr);
	va_end(arg);
/*lint -restore*/
}

static void rate_log_default(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
/* PRQA: Lint Message 530: va_list args gets initialized via va_start */
/*lint -save -e530*/
	va_list arg;
	va_start(arg, fmt);
	fprintf(stdout, "RATE CORE lib %s:%i:(%s) ", file, line, function);
	vfprintf(stdout, fmt, arg);
	if (err)
		fprintf(stdout, ": %s", strerror(err));
	putc('\n', stdout);
	va_end(arg);
/*lint -restore*/
}

/**
 * \struct adit_swsrc_core_ops
 * \brief library operations
 */
static struct adit_swsrc_core_ops rn_ops =
{
	.set_log = rate_core_set_log,
	.init = rate_core_init,
	.info = rate_core_info,
	.close = rate_core_close,
	.convert_s16 = rate_core_convert,
	.in_frames = rate_core_in_frames,
	.out_frames = rate_core_out_frames,
};


/**
 * \brief entry for synchronous operation
 * \param version plugin API version
 * \param ops Returned operations
 * \param objp Returned object
 * \return 0 on success, otherwise negative error code
 */
int ADIT_SWSRC_CORE_ENTRY(unsigned int version, void **objp, struct adit_swsrc_core_ops **ops)
{
	struct rate_core *ctx;
	if (!ops )
		return -EINVAL;

	if (!objp )
		return -EINVAL;

	if (version != ADIT_RATE_CORE_IF_VERSION) {
		RN_ERR("version mismatch :V%x - expected V%x", version, ADIT_RATE_CORE_IF_VERSION);
		return -EINVAL;
	}

	ctx = calloc(1,sizeof(*ctx));
	if (ctx == NULL) {
		return -ENOMEM;
	}
#ifdef SRC_FILE_OUTPUT
	ctx->fd_out = open(SRC_FILE_OUTPUT, O_RDWR|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
#endif
#ifdef SRC_FILE_INPUT
	ctx->fd_in = open(SRC_FILE_INPUT, O_RDWR|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
#endif
	select_arch(&ctx->sfuncs, &ctx->dfuncs, &ctx->selected_optimization);

	*objp = ctx;
	*ops = &rn_ops;
	return 0;
}

