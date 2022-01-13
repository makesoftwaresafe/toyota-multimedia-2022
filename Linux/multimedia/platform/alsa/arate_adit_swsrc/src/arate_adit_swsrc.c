/*
 *  SW ASRC suppport for ALSA asynchronous rate plugin
 *
 *  Copyright (c) 2015 by ADIT GmbH
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <string.h>
#include <dlfcn.h>
#include <alsa/asoundlib.h>

#include "rate_core_if.h"
#include "pcm_arate.h"

/*
BACKLOG:
1) rate dependency: currently feedback+deviation is accounted in frames ->slighly different behavior on different sample rates
Solution: scale feedback/deviation to e.g. unit 'time'
2) make filter lenght configurable -> need to adapt pid defaults automatically. Ts dependency to filter length ?
3) current implementation capable to act on non-hardware devices? (e.g. loop plugin?)
4) support for capture direction
5) don't export ABSOLUTE PID values anymore ! Better solution would be a relative adaption like default + X%
6) unexpected behavior on using arate with 'high prefill' (bigger than buffer size). The generation of silence samples in arate confuses level calculation!
	possibly fix in arate to also expose the expected amount of additional frames?
*/
//#define DEBUG
//#define SWSRC_DEBUG_PID

#define ASRCERR SNDERR


#define _assert(cond) do{int _cond_res = (cond); assert(_cond_res); _cond_res = _cond_res;}while(0)

#ifdef DEBUG 
#define ASRC_TRC(i,...) snd_output_printf((i)->log, "%s %s\n", __FUNCTION__, (i)->in?"IN ":"OUT")
#define ASRCDBG SNDERR
#else
#define ASRC_TRC(...) do {} while(0)
#define ASRCDBG(...) do {} while(0)
#endif

#ifndef min
#define min(a, b) ((a)<(b)?(a):(b))
#endif

typedef enum {
	ASRC_DEFAULT = 0,
	ASRC_STOP = 1,
	ASRC_CONFIGURED = 2,
	ASRC_XRUN = 4,
	ASRC_RUNNING = 5,
} asrc_ep_state_t;

struct asrc_pair;

typedef struct asrc_end
{
	int fd;
	int in;
	snd_output_t *log;

	void *buffer;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t period_size;
	snd_pcm_format_t format;/**/
	snd_pcm_uframes_t boundary;
	int channels;
	int sample_rate;
	unsigned int frame_size; /*bytes per frame*/

	snd_pcm_uframes_t app_pos;
	snd_pcm_uframes_t app_pos_old;/*rounded down to full period*/
	snd_pcm_uframes_t hw_pos;
	asrc_ep_state_t state;
	struct asrc_pair* pair;
} asrc_end_t;

#define ASRC_EP_IN 0
#define ASRC_EP_OUT 1

typedef struct asrc_pair {
	struct asrc_pair *next;
	asrc_end_t *eps[2];
	int id;
	int clockmode;
	pthread_mutex_t arate_sw_pair_mutex;
	asrc_slave_sync_t slv_sync;
	asrc_slave_cfg_t slv_cfg;

	void *tmp_convert_buffer;
	char *tmp_convert_pos;
	snd_pcm_uframes_t tmp_avail;
	snd_pcm_uframes_t out_convert_size;
	int adjust_range;	/*max in per mille*/
	snd_pcm_uframes_t out_adjust_max;
	long user_clock;
	snd_pcm_uframes_t user_clock_frames;
	struct asrc_balancer *balancer;
	int balancer_prepared;
	void  *dlobj;
	void *src_obj;
	struct adit_swsrc_core_ops *ops;
} asrc_pair_t;

#define IN_EP(p) ((p)->eps[ASRC_EP_IN])
#define OUT_EP(p) ((p)->eps[ASRC_EP_OUT])

static int release_pair(asrc_end_t *info);

static pthread_mutex_t arate_sw_list_mutex = PTHREAD_MUTEX_INITIALIZER;/*ep list*/
static asrc_pair_t *arate_sw_pairs = NULL;

#define arate_list_lock(void)\
	_assert(!pthread_mutex_lock(&arate_sw_list_mutex))

#define arate_list_unlock(void) \
	_assert (!pthread_mutex_unlock(&arate_sw_list_mutex))

#define arate_pair_lock(p) \
	_assert(!pthread_mutex_lock(&(p)->arate_sw_pair_mutex))

#define arate_pair_unlock(p)\
	_assert (!pthread_mutex_unlock(&(p)->arate_sw_pair_mutex))


#define other_section(s) ((s)->in?OUT_EP((s)->pair):IN_EP((s)->pair))

/*-------------------------------------------------------------------*/
struct asrc_balancer;

#define BFEEDBACK_T signed int
#define BADJUST_T signed int

/*
Filter:
*/
#define Ts 	0.1 /*Abtastzeit*/
#define Kp_def	0.125
#define Ki_def	0.075
#define Kd_def	0.01

//#define NO_ANTI_WUP

#define HIST_ENTRY_T signed int

static struct asrc_balancer* balancer_create(void);
static void balancer_destroy(struct asrc_balancer *b);
static void balancer_init(struct asrc_balancer *b, BADJUST_T max, unsigned int interval_in_us, unsigned int interval_out_us);
static void balancer_push_new_feedback(struct asrc_balancer *b, BFEEDBACK_T feedback);
static BADJUST_T balancer_get_adjust(struct asrc_balancer *b);

struct balance_hist_t {
	unsigned int len;
	unsigned int idx;
	HIST_ENTRY_T *hist;
};

struct asrc_balancer {
	float adjust_curr;/*adjust per period*/
	float adjust_pend;/*adjust per period*/
	double Kp;
	double Ki;
	double Kd;
	unsigned int limited;
	BADJUST_T adjust_max;
	BFEEDBACK_T feedback_target;
	BFEEDBACK_T feedback_sum;
	HIST_ENTRY_T x_last;
	signed int feedback_cnt;
	signed int adj_sum;
	unsigned int recalc;
	signed int recalc_interval;
	unsigned int ready;
	signed int prefilter_len;/*signed for division*/
	signed int ioratio;/*signed for division*/
	struct balance_hist_t fbp_hist;/*feedback PRE to simple average multiple values (on quick feedbacks)*/
	struct balance_hist_t fbf_hist;/*feedback FILTERED*/
};

#ifdef SWSRC_DEBUG_PID
#define LAST_VAL(h) OLD_VAL(h, 1)
#define OLD_VAL(h, which)  (((h)->hist)[((h)->idx)>=(which)?(((h)->idx)-(which)):(((h)->len)-(which))])
#endif

static void hist_destroy(struct balance_hist_t *h)
{
	if (h->hist)
		free(h->hist);
	h->hist = NULL;
	h->len = 0;
	h->idx = 0;
}

static void hist_reset(struct balance_hist_t *h, HIST_ENTRY_T val)
{
	unsigned int idx;
	for (idx = 0; idx < h->len; idx++)
		h->hist[idx] = val;
	h->idx = 0;
}

static void hist_init(struct balance_hist_t *h, unsigned int len)
{
	if (h->hist)
		free(h->hist);
	h->hist = malloc(len * sizeof(HIST_ENTRY_T));
	h->len = len;
	hist_reset(h, 0);
}

static inline HIST_ENTRY_T hist_filter_avg(struct balance_hist_t *h)
{
	unsigned int idx;
	HIST_ENTRY_T sum = 0;

	for (idx = 0; idx < h->len; idx++)
		sum+= h->hist[idx];

	return sum/(signed int)h->len;
}

static inline HIST_ENTRY_T hist_filter_avgw(struct balance_hist_t *h, int weight)
{
	unsigned int cnt;
	unsigned int idx = h->idx;
	unsigned int num = 1;
	HIST_ENTRY_T sum = 0;

	for (cnt = 0; cnt < h->len; cnt++) {
		if (idx)
			idx--;
		else
			idx = h->len - 1;
		sum += weight * h->hist[idx];
		num += weight;
		if (weight >= 2)
			weight /= 2;
	}
	return sum/(signed int)num;
}


static void hist_add(struct balance_hist_t *h, HIST_ENTRY_T e)
{
	h->hist[h->idx] = e;
	h->idx++;
	if (h->idx >= h->len)
		h->idx = 0;
}

static struct asrc_balancer* balancer_create(void)
{
	struct asrc_balancer *b;
	b = calloc(1, sizeof(struct asrc_balancer));
	return b;
}

static void balancer_destroy(struct asrc_balancer *b)
{
	if (!b)
		return;
	hist_destroy(&b->fbp_hist);
	hist_destroy(&b->fbf_hist);
	free(b);
}

static void balancer_reset(struct asrc_balancer *b)
{
	if (!b)
		return;
	b->feedback_target = 0;
	b->feedback_sum = 0;
	b->feedback_cnt = 0;
	b->adjust_curr = 0;
	b->adjust_pend = 0;
	b->adj_sum = 0;
	b->ready = 0;
	b->recalc = 0;
	b->limited = 0;
	b->prefilter_len = 1;
	b->ioratio = 1;
	hist_reset(&b->fbp_hist, 0);
	hist_reset(&b->fbf_hist, 0);
}

static BADJUST_T balancer_get_adjust(struct asrc_balancer *b)
{
	signed int adjust_int = 0;
	float x = 0;
	float p,i,d;
	if (!b->ready)
		return 0;

	if (b->recalc) {
		x = hist_filter_avgw(&b->fbf_hist, 8);/*aktuelle Abweichung*/
		b->feedback_sum += (BFEEDBACK_T)x;
		p = (b->Kp * x);

#ifndef NO_ANTI_WUP
#define I_PART_MAX (b->adjust_max * b->ioratio/(b->Ki * Ts))
		/*anti wind up:limit integral part*/
		if (abs(b->feedback_sum) > I_PART_MAX) {
#ifdef SWSRC_DEBUG_PID
			fprintf(stderr, "limit sum %d -> %d\n", b->feedback_sum, b->feedback_sum>0? (BFEEDBACK_T)I_PART_MAX:-(BFEEDBACK_T)I_PART_MAX);
#endif
			b->feedback_sum = b->feedback_sum>0? (BFEEDBACK_T)I_PART_MAX:-(BFEEDBACK_T)I_PART_MAX;
		}
#endif
		i = (b->Ki * Ts) * (float)b->feedback_sum;
		d = (b->Kd / Ts) * (x - b->x_last);
		b->adjust_curr = -(p + i + d);
		if (abs((BADJUST_T)b->adjust_curr) > (b->adjust_max * b->ioratio)) {
			b->limited = 1;
			b->adjust_curr = (b->adjust_curr > 0)?(b->adjust_max*b->ioratio):-(b->adjust_max*b->ioratio);
		} else {
			b->limited = 0;
		}
	}
	b->adjust_pend += b->adjust_curr/b->ioratio;
	adjust_int = (BADJUST_T)b->adjust_pend;

	/*limit to max range*/
	if (abs(adjust_int) > b->adjust_max) {
		if (adjust_int > 0)
			adjust_int = b->adjust_max;
		else
			adjust_int = -b->adjust_max;
	}

	b->adjust_pend-= adjust_int;
	b->adj_sum += adjust_int;

	if (b->recalc) {
#ifdef SWSRC_DEBUG_PID
		fprintf(stderr, " %f x_last %d x_pre %d sum %d ->adjust %f/%d/%d pend %f - %s p%f, i%f d%f\n",
			x, b->x_last, LAST_VAL(&b->fbp_hist), b->feedback_sum, b->adjust_curr, adjust_int, b->adj_sum, b->adjust_pend, b->limited?"LIM":"", p,i,d);
#endif
		b->recalc = 0;
		b->adj_sum = 0;
		b->x_last = (HIST_ENTRY_T)x;
	}
	return adjust_int;
}

#define RECALC_INTERVAL_MS 500
#define RECALC_INTERVAL_MIN 1

/*smaller value gives better response time, but may introduce distortion on high input clock jitter*/
#define PREFILTER_LEN_MS 5000
#define PREFILTER_LEN_MIN 4	/*minimum len of prefilter for avg calculation*/

#define POSTFILTER_LEN_MS 30000
#define POSTFILTER_LEN (POSTFILTER_LEN_MS/PREFILTER_LEN_MS)

static void balancer_init(struct asrc_balancer *b, BADJUST_T max, unsigned int interval_in_us, unsigned int interval_out_us)
{
	balancer_reset(b);
	b->adjust_max = max;
	if (interval_in_us) {
		b->prefilter_len = (PREFILTER_LEN_MS * 1000) / interval_in_us;
		b->recalc_interval = (RECALC_INTERVAL_MS * 1000) / interval_in_us;
	}
	if (b->prefilter_len < PREFILTER_LEN_MIN)
		b->prefilter_len = PREFILTER_LEN_MIN;
	if (b->recalc_interval < RECALC_INTERVAL_MIN )
		b->recalc_interval = RECALC_INTERVAL_MIN;

	b->ioratio = (interval_in_us/interval_out_us)*b->recalc_interval;
	if(!b->ioratio)
		b->ioratio = 1;

	hist_init(&b->fbp_hist, b->prefilter_len);
	hist_init(&b->fbf_hist, POSTFILTER_LEN);
}

static void balancer_push_new_feedback(struct asrc_balancer *b, BFEEDBACK_T feedback)
{
	BFEEDBACK_T deviation;
	/*normalize relative to target level*/
	b->feedback_cnt++;

	if (!b->ready) {
		/*unless started, we collect the raw feedbacks - 
		once we have enaugh calculate the target level and switch to storing the deviation*/
		hist_add(&b->fbp_hist, feedback);
		if (b->feedback_cnt>=b->prefilter_len) {
			b->ready = 1;
			b->feedback_target = hist_filter_avg(&b->fbp_hist);
			hist_reset(&b->fbp_hist, 0);
			hist_reset(&b->fbf_hist, 0);
		}
	} else {
		deviation = feedback- b->feedback_target;
		hist_add(&b->fbp_hist, deviation);
	}
	if (b->ready && (b->feedback_cnt>=b->recalc_interval)) {
		hist_add(&b->fbf_hist, hist_filter_avg(&b->fbp_hist));
		b->feedback_cnt = 0;
		b->recalc = 1;
	}
}

/*-------------------------------------------------------------------*/

static void set_other_state(asrc_end_t *info, asrc_ep_state_t state)
{
	asrc_end_t *other = other_section(info);
	uint64_t ev = 1;
	if (!other)
		return;
	switch (state) {
	case ASRC_STOP:
		if (other->state == ASRC_RUNNING) {
			ASRCDBG("SET OTHER STATE to %d", state);
			other->state = state;
			write(other->fd, &ev, sizeof(ev));
		}
		break;
	case ASRC_XRUN:
		/*enter xrun always allowed?*/
		if (other->state == ASRC_RUNNING) {
			ASRCDBG("SET OTHER STATE to %d", state);
			other->state = state;
			write(other->fd, &ev, sizeof(ev));
		}
		break;
	default:
		break;
	}
}

/*available from users view */
static snd_pcm_uframes_t asrc_user_avail(asrc_end_t *info)
{
	snd_pcm_sframes_t avail;

	if (info->in) {
		avail = (info->buffer_size - info->app_pos) + info->hw_pos;
		if (avail < 0)
			avail += info->boundary;
		else if ((snd_pcm_uframes_t)avail >= info->boundary)
			avail -= info->boundary;
	} else {
		avail = info->hw_pos - info->app_pos;
		if (avail < 0)
			avail += info->boundary;
	}
	return avail;
}

/*available from src view */
static snd_pcm_uframes_t asrc_src_avail(asrc_end_t *info)
{
	return info->buffer_size - asrc_user_avail(info);
}

#define ADJUST_LIMIT_PM 100 /*adjust in per mille - 1 percent is enough*/
#define ADJUST_DEFAULT_PM 1 /*1pm should be enaugh*/

static int asrc_configure_core(asrc_pair_t *pair)
{
	asrc_end_t *in = IN_EP(pair);
	asrc_end_t *out =  OUT_EP(pair);
	

	struct rate_core_init_info info = RATE_CORE_INIT_INFO_INITIALIZER;

	if (!in || !out) {
		ASRCDBG("OTHER SIDE DIED");
		return 0;
	}

	if (in->state != ASRC_CONFIGURED || out->state != ASRC_CONFIGURED) {
		ASRCDBG("OTHER SIDE NOT CONFIGURED");
		return 0;
	}

	if (out->channels != in->channels) {
		return -EINVAL;
	}

	if (out->format != in->format) {
		return -EINVAL;
	}

	pair->out_convert_size = ((int64_t)in->period_size * out->sample_rate) / in->sample_rate;
	if (pair->adjust_range == -1)
		pair->adjust_range = ADJUST_DEFAULT_PM;
	if (pair->adjust_range)
		pair->out_adjust_max = ((pair->out_convert_size * pair->adjust_range)/1000)+1;
	if (pair->user_clock > 0)
		pair->user_clock_frames = ((int64_t)pair->user_clock*in->sample_rate)/(1000*1000);
	else
		pair->user_clock_frames = in->buffer_size/2;

	/*round up to at least 1 frame adjust range*/
	if (pair->adjust_range && (pair->out_adjust_max == 0)) {
		ASRCDBG("rounded up to range of 1 frame");
		pair->out_adjust_max = 1;
	}
	

	ASRCDBG("CONFIG: in: rate %d size %d user_clk %d -- out: rate %d  size %d +/-%d -->eff.rate %lld",
		in->sample_rate, in->period_size,pair->user_clock_frames,out->sample_rate,pair->out_convert_size,pair->out_adjust_max,
		(int64_t)in->sample_rate*pair->out_convert_size/in->period_size);

	if (pair->tmp_convert_buffer) {
		free(pair->tmp_convert_buffer);
		pair->tmp_convert_buffer = NULL;
		pair->tmp_convert_pos = NULL;
	}
	pair->tmp_convert_buffer = malloc((pair->out_convert_size + pair->out_adjust_max) * out->frame_size);
	if (pair->tmp_convert_buffer == NULL)
		return -ENOMEM;
	pair->tmp_convert_pos = pair->tmp_convert_buffer;

	info.in.freq = in->sample_rate;
	info.in.period_size = in->period_size;
	info.out.freq = out->sample_rate;
	info.out.period_size = pair->out_convert_size;
	info.out.pitch_frames_max = pair->out_adjust_max;
	info.channels = out->channels;
	return pair->ops->init(pair->src_obj, &info);
}

static inline void asrc_hw_forward(asrc_end_t *ctx, snd_pcm_uframes_t inc)
{
 	ctx->hw_pos += inc;
	ctx->hw_pos %= ctx->boundary;
}

static signed long ts_diff_us(struct timespec *before, struct timespec *now)
{
	struct timespec temp;
	if ((now->tv_nsec-before->tv_nsec)<0) {
		temp.tv_sec = (now->tv_sec-before->tv_sec)-1;
		temp.tv_nsec = 1000000000+now->tv_nsec-before->tv_nsec;
	} else {
		temp.tv_sec = now->tv_sec-before->tv_sec;
		temp.tv_nsec = now->tv_nsec-before->tv_nsec;
	}
	return (temp.tv_nsec/1000)+(temp.tv_sec*1000*1000);
}


#define ts_valid(_ts) ((_ts).tv_sec || (_ts).tv_nsec)

static int update_drift(asrc_pair_t *pair, snd_pcm_uframes_t tmp_avail, snd_pcm_uframes_t in_avail, snd_pcm_uframes_t out_avail)
{
	int ret = 0;
	struct timespec ts;
	signed long diff;
	snd_pcm_uframes_t extra = 0;
	snd_pcm_uframes_t curr_level;

	if (!ts_valid(pair->slv_sync.tstamp))
		return 0;

	ret = clock_gettime(pair->clockmode, &ts);
	if (ret)
		return -errno;

	/*current level: slave buffer + src internal*/
	curr_level = pair->slv_cfg.buffer_size - pair->slv_sync.avail;
	curr_level+= in_avail;
	curr_level+= out_avail;
	curr_level+= tmp_avail;
	
	diff = ts_diff_us(&pair->slv_sync.tstamp, &ts);
	extra = ((int64_t)diff * OUT_EP(pair)->sample_rate) / (1000*1000);/*in the meantime processed samples*/

	curr_level -= extra;
	balancer_push_new_feedback(pair->balancer, curr_level);

#ifdef SWSRC_DEBUG_FEEDBACK
	fprintf(stderr,"level: %ld (slv %ld + tmp %ld + in %ld + out %ld - played %ld) t: irq:%ld.%06ld ->now %ld.%06ld diff %ldus\n",
		curr_level, pair->slv_cfg.buffer_size - pair->slv_sync.avail, tmp_avail, in_avail, out_avail, extra,
		pair->slv_sync.tstamp.tv_sec, pair->slv_sync.tstamp.tv_nsec/(1000*1),
		ts.tv_sec, ts.tv_nsec/(1000*1), diff);
#endif
	return 0;
}

/*user 'clock' is derived from each period crossing*/
static int extract_user_clock(asrc_end_t *user, snd_pcm_uframes_t tmp_avail, snd_pcm_uframes_t in_avail, snd_pcm_uframes_t out_avail)
{
	snd_pcm_uframes_t diff;
	int err = 0;

	if (user->app_pos >= user->app_pos_old)
		diff = user->app_pos - user->app_pos_old;
	else
		diff = (user->boundary - user->app_pos_old) + user->app_pos;
	if (diff < user->pair->user_clock_frames)
		return 0;

	while (diff >= user->pair->user_clock_frames) {
		err = update_drift(user->pair, tmp_avail, in_avail, out_avail);
		diff -= user->pair->user_clock_frames;
	}
	user->app_pos_old = user->app_pos - (user->app_pos%user->pair->user_clock_frames);

	return err;
}

/*
note: arate always assumes INTERLEAVED LAYOUT
*/
static int asrc_process(asrc_end_t *initiator)
{
	asrc_pair_t *pair = initiator->pair;
	asrc_end_t *in = IN_EP(pair);
	asrc_end_t *out =  OUT_EP(pair);
	snd_pcm_uframes_t in_avail;
	snd_pcm_uframes_t out_avail;
	snd_pcm_uframes_t chunk;
	uint64_t ev = 1;
	signed int adjust = 0;
	char in_chg = 0;
	char out_chg = 0;
	int err = 0;
	/*consume poll event if any...*/
	read(initiator->fd, &ev, sizeof(ev));
	ev = 1;

	if (!in || !out) {
		ASRCDBG("OTHER SIDE DIED");
		return 0;
	}

	if (in->state != ASRC_RUNNING || out->state != ASRC_RUNNING)
		return 0;

	in_avail = asrc_src_avail(in);
	out_avail = asrc_src_avail(out);

	if (!in_avail && !out_avail)
		return 0;

	//SNDERR("->IN: avail %d hw %d app %d", in_avail, in->hw_pos, in->app_pos);
	//SNDERR("->OUT:avail %d hw %d app %d", out_avail, out->hw_pos, out->app_pos);

	/*copy tmp frames to out -->out may have more available!!*/
	while ((chunk = min(pair->tmp_avail, out_avail)) > 0) {
		if (((out->hw_pos % out->buffer_size) + chunk) > out->buffer_size )
			chunk = out->buffer_size - (out->hw_pos % out->buffer_size);
		//SNDERR("1:CPY %p -> %p size %d", pair->tmp_convert_pos, (char*)out->buffer + ((out->hw_pos%out->buffer_size) * out->frame_size),chunk * in->frame_size);

		memcpy((char*)out->buffer + ((out->hw_pos%out->buffer_size) * out->frame_size), pair->tmp_convert_pos, chunk * in->frame_size);
		asrc_hw_forward(out, chunk);
		out_avail -= chunk;
		pair->tmp_convert_pos += chunk * in->frame_size;
		pair->tmp_avail -= chunk;
		out_chg = 1;
	}

	extract_user_clock(in, pair->tmp_avail, ((int64_t)(in_avail)*out->sample_rate)/in->sample_rate, asrc_user_avail(out));

	/*convert*/
	if ((in_avail >= in->period_size) && (pair->tmp_avail == 0)) {
		adjust = balancer_get_adjust(pair->balancer);

		chunk = pair->out_convert_size + adjust;
		pair->tmp_avail = chunk;
		pair->tmp_convert_pos = pair->tmp_convert_buffer;

		struct rate_core_convert_info info = {
			.in = {
				.buf = (int16_t *)((void*)((char*)in->buffer + ((in->hw_pos%in->buffer_size) * in->frame_size))),
				.frames = in->period_size
			},
			.out = {
				.buf = (int16_t *)pair->tmp_convert_buffer,
				.frames = pair->out_convert_size + adjust,
				.pitch_frames = adjust
			}
		};

		//SNDERR("CNVRT %p size %d -> %p size %d",info.in.buf,info.in.frames*in->frame_size, info.out.buf, info.out.frames*out->frame_size);

		err = pair->ops->convert_s16(pair->src_obj, &info);
		asrc_hw_forward(in, in->period_size);
		in_chg = 1;
	}

	/*copy tmp frames to out*/
	while ((chunk = min(pair->tmp_avail, out_avail)) > 0) {
		if (((out->hw_pos % out->buffer_size) + chunk) > out->buffer_size )
			chunk = out->buffer_size - (out->hw_pos % out->buffer_size);
		//SNDERR("2:CPY %p -> %p size %d",pair->tmp_convert_pos,(char*)out->buffer + ((out->hw_pos % out->buffer_size) * out->frame_size),chunk * in->frame_size);

		memcpy((char*)out->buffer + ((out->hw_pos % out->buffer_size) * out->frame_size), pair->tmp_convert_pos, chunk * in->frame_size);
		asrc_hw_forward(out, chunk);
		out_avail -= chunk;
		pair->tmp_convert_pos += chunk * in->frame_size;
		pair->tmp_avail -= chunk;
		out_chg = 1;
	}

	ev = 1;
	if (in_chg)/*possible optimisation? if avail > avail min */
		write(in->fd, &ev, sizeof(ev));
	ev = 1;
	if (out_chg)/*possible optimisation? if avail > avail min */
		write(out->fd, &ev, sizeof(ev));

	return err;
}

#define SWSRC_BUFFERBYTES_MAX (512*1024)
#define SWSRC_PERIODS_MIN 1 //2?
#define SWSRC_PERIODBYTES_MIN 2	//1channel, 1 S16 sample
static const asrc_capabilities_t default_caps = {
	.formats = 1<<SND_PCM_FORMAT_S16_LE,
	.rate_min = 8000,
	.rate_max = 192000,
	.channels_min = 1,
	.channels_max = 32,
	.bufbytes_max = SWSRC_BUFFERBYTES_MAX,
	.periodbytes_min = SWSRC_PERIODBYTES_MIN,
	.periodbytes_max = SWSRC_BUFFERBYTES_MAX/SWSRC_PERIODS_MIN,
	.periods_min = SWSRC_PERIODS_MIN,
	.periods_max = SWSRC_BUFFERBYTES_MAX/SWSRC_PERIODBYTES_MIN,
	.fifo_size = 0,
};

static int sw_asrc_get_caps(void *ctx, asrc_capabilities_t *caps)
{
	asrc_end_t *info = (asrc_end_t*) ctx;

	caps->formats = default_caps.formats;
	caps->rate_min = default_caps.rate_min;
	caps->rate_max = default_caps.rate_max;
	caps->channels_min = default_caps.channels_min;
	caps->channels_max = default_caps.channels_max;
	caps->bufbytes_max = default_caps.bufbytes_max;
	caps->periodbytes_min = default_caps.periodbytes_min;
	caps->periodbytes_max = default_caps.periodbytes_max;
	caps->periods_min = default_caps.periods_min;
	caps->periods_max = default_caps.periods_max;
	caps->fifo_size = default_caps.fifo_size;

	if (info == IN_EP(info->pair)) {
		if (info->pair->user_clock > 0) {
			caps->periods_min = 2;
			caps->buffertime_min = info->pair->user_clock*2;
		}
	}

	return 0;
}

static int sw_asrc_configure(void *ctx, struct asrc_io_cfg *asrc_io_cfg)
{
	int err = 0;
	asrc_end_t *info = (asrc_end_t*) ctx;

	ASRC_TRC(info);

	if (info->pair->clockmode == -1)
		return -EINVAL;

	asrc_io_cfg->buffer = malloc(asrc_io_cfg->buf_size);
	if (!asrc_io_cfg->buffer)
		return -ENOMEM;
	if (info->buffer) {
		free(info->buffer);
		info->buffer = NULL;
	}
	info->buffer = asrc_io_cfg->buffer;
	if (info->buffer == NULL)
		return -ENOMEM;
	info->frame_size = snd_pcm_format_size(asrc_io_cfg->format, asrc_io_cfg->channels);
	info->buffer_size = (unsigned int)asrc_io_cfg->buf_size / info->frame_size;
	info->period_size = info->buffer_size / (unsigned int)asrc_io_cfg->periods;
	info->sample_rate = asrc_io_cfg->sample_rate;
	info->channels = asrc_io_cfg->channels;
	info->format = asrc_io_cfg->format;

	arate_pair_lock(info->pair);
	info->state = ASRC_CONFIGURED;
	/*configure SRC, if both sides have set config.*/
	err = asrc_configure_core(info->pair);
	arate_pair_unlock(info->pair);

	ASRCDBG("CONFIG: size %d, psize %d rate%d channels %d fmt %d pitch %d",
		info->buffer_size, info->period_size, info->sample_rate, info->channels, info->format, info->pair->adjust_range);
	return err;
}

static int sw_asrc_start(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err = 0;
	ASRC_TRC(info);

	arate_pair_lock(info->pair);
	info->state = ASRC_RUNNING;
	err = asrc_process(info);
	arate_pair_unlock(info->pair);

	return err;
}

static int sw_asrc_prepare(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	ASRC_TRC(info);
	info->hw_pos = 0;
	info->app_pos = 0;
	info->app_pos_old = 0;

	arate_pair_lock(info->pair);
	info->pair->tmp_avail = 0;
	info->pair->tmp_convert_pos = info->pair->tmp_convert_buffer;
	info->state = ASRC_CONFIGURED;
	memset(&info->pair->slv_sync, 0, sizeof(info->pair->slv_sync));
	if (!info->pair->balancer_prepared) {
		if (IN_EP(info->pair)->state == ASRC_CONFIGURED && OUT_EP(info->pair)->state == ASRC_CONFIGURED) {
			info->pair->balancer_prepared = 1;
			balancer_init(info->pair->balancer,info->pair->out_adjust_max,
					((int64_t)(info->pair->user_clock_frames)*(1000*1000))/IN_EP(info->pair)->sample_rate,
					((int64_t)(IN_EP(info->pair)->period_size)*(1000*1000))/IN_EP(info->pair)->sample_rate);
		}
	}
	arate_pair_unlock(info->pair);
	return 0;
}

static int sw_asrc_stop(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err = 0;
	ASRC_TRC(info);

	arate_pair_lock(info->pair);
	info->state = ASRC_STOP;
	set_other_state(info, ASRC_STOP);
	info->pair->balancer_prepared = 0;
	arate_pair_unlock(info->pair);

	return err;
}

static int sw_asrc_xrun(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err = 0;

	ASRC_TRC(info);

	arate_pair_lock(info->pair);
	info->state = ASRC_XRUN;
	set_other_state(info, ASRC_XRUN);
	arate_pair_unlock(info->pair);

	return err;
}

static int sw_asrc_sync(void *ctx, asrc_sync_t *sync_io)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err = 0;

	arate_pair_lock(info->pair);
	info->boundary = sync_io->boundary;
	info->app_pos = sync_io->app_pos;
	err = asrc_process(info);
	sync_io->hw_pos = info->hw_pos;
	sync_io->start_threshold = info->in?info->pair->user_clock_frames:OUT_EP(info->pair)->period_size;
	sync_io->stop_threshold = info->boundary;
	if (info->state == ASRC_XRUN)
		sync_io->state = SND_PCM_STATE_XRUN;
	arate_pair_unlock(info->pair);
	return err;
}

static int sw_asrc_getfd(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	ASRC_TRC(info);
	return info->fd;
}

static int sw_asrc_getfdev(void *ctx)
{
	ctx = ctx;
	return POLLIN;
}

static int sw_asrc_free(asrc_end_t *info)
{
	if (!info)
		return 0;

	ASRC_TRC(info);

	/*detatch from pair*/
	(void)release_pair(info);

	if (info->buffer) {
		free(info->buffer);
		info->buffer = NULL;
	}

	if (info->fd >= 0) {
		close(info->fd);
		info->fd = -1;
	}

	if (info->log) {
		snd_output_close(info->log);
		info->log = NULL;
	}

	free(info);

	return 0;	
}

static int sw_asrc_close(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	ASRC_TRC(info);
	sw_asrc_free(info);
	return 0;
}


static int sw_asrc_dump(void *ctx, snd_output_t *out)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	asrc_pair_t *pair = info->pair;
	ASRC_TRC(info);
#define CORE_INFO_LEN 64
	char buf[CORE_INFO_LEN];
	pair->ops->info(pair->src_obj, buf, CORE_INFO_LEN);
	snd_output_printf(out, "Converter: %s\n", buf);
	return 0;
}

static int asrc_slave_config(void *ctx, asrc_slave_cfg_t *slv_cfg)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	info->pair->clockmode = (slv_cfg->flags&SLV_CFG_TSTAMP_MONOTONIC)?CLOCK_MONOTONIC:CLOCK_REALTIME;
	info->pair->slv_cfg = *slv_cfg;
	ASRCDBG("SLAVE CFG: BUFFER %d PERIODSIZE %d clock %s", slv_cfg->buffer_size, slv_cfg->period_size, info->pair->clockmode == CLOCK_MONOTONIC?"MONOTONIC":"REALTIME");
	return 0;
}

static int asrc_slave_sync(void *ctx, asrc_slave_sync_t *slv_sync)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	arate_pair_lock(info->pair);
	if (slv_sync->state == SND_PCM_STATE_RUNNING) {
		if (ts_valid(slv_sync->tstamp))
			info->pair->slv_sync = *slv_sync;
	}
	arate_pair_unlock(info->pair);
	return 0;
}

/*PRQA: Lint Message 454,455,456: Lint can't check external locking API - CR-SWGIII-523*/
/*lint -save -e454 -e455 -e456 */
static void asrc_lock(void* ctx, int lock)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	if (lock)
		arate_pair_lock(info->pair);
	else
		arate_pair_unlock(info->pair);
	return;
}
/*lint -restore */

static const asrc_ops_t asrc_ops = {
	.get_capabilities = sw_asrc_get_caps,
	.configure = sw_asrc_configure,
	.dump = sw_asrc_dump,
	.start = sw_asrc_start,
	.prepare = sw_asrc_prepare,
	.stop = sw_asrc_stop,
	.xrun = sw_asrc_xrun,
	.get_pollfd = sw_asrc_getfd,
	.get_pollev = sw_asrc_getfdev,
	.sync = sw_asrc_sync,
	.close = sw_asrc_close,
	.slave_sync = asrc_slave_sync,
	.slave_config = asrc_slave_config,
	.lock_sync = asrc_lock,
};

static int open_src_core(asrc_pair_t *pair)
{
#define SRC_CORE_LIB_NAME_STRING(name) name
#define SRC_CORE_OPEN_FUNC_STRING(name) __STRING(name)

	char open_name[64], lib_name[128];
	adit_swsrc_core_open_func_t open_func;
	int err;
	snprintf(open_name, sizeof(open_name), SRC_CORE_OPEN_FUNC_STRING(ADIT_SWSRC_CORE_ENTRY));
	snprintf(lib_name, sizeof(lib_name), SRC_CORE_LIB_NAME_STRING(ADIT_SWSRC_LIB_NAME));

	if (pair->dlobj) {
		SNDERR("LIB %s already loaded...", lib_name);
		return 0;
	}

	pair->dlobj = dlopen(lib_name, RTLD_NOW);
	if (!pair->dlobj) {
		SNDERR("open %s failed with err %s", lib_name, dlerror());
		return -ENOENT;
	}

	open_func = dlsym(pair->dlobj, open_name);
	if (open_func == NULL) {
		SNDERR("symbol %s is not defined inside %s", open_name, lib_name);
		dlclose(pair->dlobj);
		pair->dlobj = NULL;
		return -ENOENT;
	}

	err = open_func(ADIT_RATE_CORE_IF_VERSION, &pair->src_obj, &pair->ops);
	if (err) {
		SNDERR("open failed with err %d", err);
		dlclose(pair->dlobj);
		pair->dlobj = NULL;
		return -ENOENT;
	}

	pair->ops->set_log(pair->src_obj, 1, snd_lib_error);
	/*use error channel also for debug*/
	pair->ops->set_log(pair->src_obj, 0, snd_lib_error);
	return 0;
}

static int parse_cfg( asrc_pair_t *info, const snd_config_t *converter_cfg)
{
	snd_config_iterator_t i, next;

	info->adjust_range = -1;
	info->user_clock = -1;
	info->balancer->Kp = Kp_def;
	info->balancer->Ki = Ki_def;
	info->balancer->Kd = Kd_def;

	/*config is optional*/
	if (!converter_cfg)
		return 0;

	if (snd_config_get_type(converter_cfg) != SND_CONFIG_TYPE_COMPOUND) {
		ASRCERR("WRONG CFG ...");
		return 0;
	}
	snd_config_for_each(i, next, converter_cfg) {
		snd_config_t *n = snd_config_iterator_entry(i);

		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (strcmp(id, "pitch_range") == 0) {
			long val;
			snd_config_get_integer(n, &val);
			/* alsa-lib guaranteed val <= INT_MAX. See conf.c:int parse_value(). */
			info->adjust_range = val;
			if (info->adjust_range > ADJUST_LIMIT_PM) {
				SNDERR("Limit range to max %dpm", ADJUST_LIMIT_PM);
				info->adjust_range = ADJUST_LIMIT_PM;
			}
			continue;
		}
		if (strcmp(id, "kp") == 0) {
			double val;
			if (snd_config_get_real(n, &val) < 0)
				return -EINVAL;
			info->balancer->Kp = val;
			continue;
		}
		if (strcmp(id, "ki") == 0) {
			double val;
			if (snd_config_get_real(n, &val) < 0)
				return -EINVAL;
			info->balancer->Ki = val;
			continue;
		}
		if (strcmp(id, "kd") == 0) {
			double val;
			if (snd_config_get_real(n, &val) < 0)
				return -EINVAL;
			info->balancer->Kd = val;
			continue;
		}
		if (strcmp(id, "clock") == 0) {
			long val;
			snd_config_get_integer(n, &val);
			info->user_clock = val;
			continue;
		}
		ASRCERR("Unknown field %s", id);
		return -EINVAL;
	}

	return 0;
}

static int release_pair(asrc_end_t *info)
{
	asrc_pair_t *pair = info->pair;
	asrc_pair_t *elem;
	if (!pair)
		return 0;

	arate_list_lock();
	arate_pair_lock(pair);

	if (other_section(info) == NULL) {
		/*last user*/
		ASRCDBG("LAST ONE");
		if (pair->tmp_convert_buffer) {
			ASRCDBG("free tmp buffer..");
			free(pair->tmp_convert_buffer);
			pair->tmp_convert_buffer = NULL;
			pair->tmp_convert_pos = NULL;
		}

		/*delete from list*/
		if (arate_sw_pairs == pair) {
			ASRCDBG("delete from list..1st entry.");

			arate_sw_pairs = pair->next;
			pair->next = NULL;
		} else {
			elem = arate_sw_pairs;
			while (elem) {
				if (elem->next == pair) {
					ASRCDBG("delete from list mid");
					elem->next = pair->next;
					pair->next = NULL;
				}
				elem = elem->next;
			}
		}
		ASRCDBG("destroy balancer...");
		if (pair->balancer) {
			balancer_destroy(pair->balancer);
			pair->balancer = NULL;
		}
		if (pair->ops && pair->src_obj) {
			pair->ops->close(pair->src_obj);
			pair->ops = NULL;
			pair->src_obj = NULL;
		}
		info->pair = NULL;
		arate_pair_unlock(pair);
		free(pair);
	} else {
		pair->eps[info->in?ASRC_EP_IN:ASRC_EP_OUT] = NULL;
		info->pair = NULL;
		arate_pair_unlock(pair);
	}
	arate_list_unlock();

	return 0;
}

static asrc_pair_t *create_pair(int id)
{
	pthread_mutexattr_t mutex_attr;
	asrc_pair_t *elem = calloc(1, sizeof(*elem));
	ASRCDBG("CREATE PAIR\n");
	if (!elem)
		return NULL;

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&elem->arate_sw_pair_mutex, &mutex_attr);
	elem->id = id;
	/*easiest way: insert at begin*/
	elem->next = arate_sw_pairs;
	arate_sw_pairs = elem;

	return elem;
}

static int find_pair(asrc_end_t *info, int id, asrc_pair_t **found)
{
	asrc_pair_t *elem = arate_sw_pairs;

	while (elem) {
		ASRCDBG("FIND PAIR: compare %d with existing id %d\n", id, elem->id);
		if (elem->id == id) {
			if (elem->eps[info->in?ASRC_EP_IN:ASRC_EP_OUT] != NULL)
				return -EBUSY;
			break;
		}
		elem = elem->next;
	}

	if (!elem)
		elem = create_pair(id);
	if (!elem)
		return -ENOMEM;

	*found = elem;
	return 0;
}

/*find existing pair or setup new one*/
static int setup_pair(asrc_end_t *info, int id, const snd_config_t *conf)
{	
	int err;
	asrc_pair_t *pair;
	arate_list_lock();

	err = find_pair(info, id, &pair);
	if (err) {
		arate_list_unlock();
		return err;
	}

	/*have pair */
	arate_pair_lock(pair);

	pair->eps[info->in?ASRC_EP_IN:ASRC_EP_OUT] = info;
	info->pair = pair;
	pair->clockmode = -1;
	if (other_section(info) == NULL) {
		/*first one...*/
		ASRCDBG("FIRST ONE: %s", info->in?"IN":"OUT");
		pair->balancer = balancer_create();

		err = parse_cfg(pair, conf);
		if (err)
			ASRCERR("parse cfg failed");
		if (!err) {
			err = open_src_core(pair);
			if (err)
				ASRCERR("open src core failed");
		}
	}
	arate_pair_unlock(info->pair);
	arate_list_unlock();

	return err;
}

/*setup pair independent stuff*/
static int create_section(asrc_end_t **sect, unsigned int mode)
{
	asrc_end_t *info;
	int err;

	if ((mode & SND_PCM_ARATE_STREAM_MODE_FE) && !(mode & SND_PCM_ARATE_STREAM_MODE_IN)) {
		ASRCERR("CAPTURE NOT SUPPORTED");
		return -EINVAL;
	}

	info = calloc(1, sizeof(asrc_end_t));
	if (info == NULL) {
		ASRCERR("ALLOC ARATE INSTANCE FAILED");
		return -ENOMEM;
	}

	info->fd = -1;
	info->in = mode & SND_PCM_ARATE_STREAM_MODE_IN;;
	info->state = ASRC_DEFAULT;

	err = snd_output_stdio_attach(&info->log, stdout, 0);
	if (err < 0) {
		ASRCERR("attach to stdio failed");
		sw_asrc_free(info);
		return err;
	}

	info->fd = eventfd(0, EFD_NONBLOCK);
	if (info->fd < 0) {
		ASRCERR("open eventfd failed with err %d", errno);
		sw_asrc_free(info);
		return -errno;
	}

	*sect = info;
	return 0;
}

/**
 * \brief Creates a new async sw sample rate conversion instance.
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
int SND_PCM_ARATE_PLUGIN_ENTRY(adit_swsrc) (unsigned int version, void **objp, asrc_ops_t *ops, unsigned int mode, int id, const snd_config_t *conf)
{
	asrc_end_t *info = NULL;
	int err;

	if (!objp) {
		ASRCERR("no objp given");
		return -EINVAL;
	}
	if (!ops) {
		ASRCERR("no ops given");
		return -EINVAL;
	}
	if (version != SND_PCM_ARATE_PLUGIN_VERSION) {
		ASRCERR("version mismatch :V%x - expected V%x", version, SND_PCM_ARATE_PLUGIN_VERSION);
		return -EINVAL;
	}

	/*create section*/
	err = create_section(&info, mode);
	if (err < 0) {
		ASRCERR("create section failed");
		return err;
	}

	/*new or existing pair*/
	err = setup_pair(info, id, conf);
	if (err < 0) {
		ASRCERR("request pair failed");
		sw_asrc_free(info);
		return err;
	}
	*ops = asrc_ops;
	*objp = info;

	return err;
}

