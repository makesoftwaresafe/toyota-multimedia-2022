/**
 * \file: rts_main.c
 *
 * Real time streaming library.
 * Supports ALSA devices.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2014 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <values.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "rts.h"

/*TODO:
-don't call snd_pcm_drop in state 'open' -> assert in plugin null
-support for draining outputs
-silence prefill: CG: 'if silence is a multiple of ideal period ms use a differnt calc to avoid rounding issues'
-support for multichannel in/output (interleaved)
-support for buffer = 1 period w/o silence prefill (e.g. blocking sw loop device)
-silence prefill configurable per device ?
-logging
-better approch to decide for recover delay: check whether stream was active instead of retry_cnt 
    (then sleep will also correctly be applied on FIRST xrun - but not on standard streaming xrun)
-configure buffer size period based. 
	rate plugin shows issues in buffer size calculation which makes the hw refusing many settings.
	Approach to work around this via setting periods instead of buffer size does not lead to expected result
-make logging earlier available to avoid usage of RTS_ERR_EARLY
*/

#define RTS_LOGL_FORCE		0 /*internally used to skip check*/
#define RTS_LOGL_SYS_ERR	1 /*unexpected system errors (which lead to termination of streaming)*/
#define RTS_LOGL_STREAM_ERR	2 /*streaming errors (even if recoverable)*/
#define RTS_LOGL_TRACE_INSTANCE	3 /*API trace for create/destroy/first read*/
#define RTS_LOGL_DUMP_XRUN	4 /*dump status on underrun (for debugging purpose)*/
#define RTS_LOGL_CONFIG		5 /*configuration on init (for debugging purpose)*/


/*early log function to use before RT is established or cannot be accessed */
#define RTS_ERR_EARLY(fmt, ...) \
do { \
		fprintf(stderr,"ERROR: %s: "fmt, __FUNCTION__, ##__VA_ARGS__ );\
		fflush(stderr);\
} while (0)

#define RTS_ERR(rt_, ...) RTS_LOG(rt_, RTS_LOGL_FORCE, __VA_ARGS__)

#define RTS_LOG(rt_, level, ...) \
do {\
	if (rt_->loglevel >= level) {\
		if (RTS_FEATURE_ENABLED(rt_, RTS_FEAT_LOG_TSTAMP)) {\
               struct timespec _time;\
               clock_gettime(CLOCK_MONOTONIC, &_time);\
               snd_output_printf(rt_->log, "[%ld.%06ld] ", _time.tv_sec, _time.tv_nsec/1000);\
		}\
		snd_output_printf(rt_->log, __VA_ARGS__);\
		snd_output_flush(rt_->log);\
	}\
} while (0)

#define RTS_MAX_STREAMS 8
#define RTS_MAX_CHANNELS 32
#define RTS_MAGIC_OK 0x79616b6f
#define RTS_MAGIC_INVAL 0x64616564

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define RTS_LOGL_DEFAULT RTS_LOGL_SYS_ERR

#define RTS_RECOVER_DELAY_DEFAULT 20	/*delay in ms*/
#define RTS_RECOVER_DELAY_EXIT (-1)	/*terminate streaming on xrun*/
#define RTS_PREFILL_WARN_THRES 1
#define RTS_TOUT_ENDLESS (-1)

typedef snd_pcm_sframes_t (*XferFunc)(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size);

/*representation of an alsa device*/
struct rts_adev {
	/*static data - fixed after configuration of device*/
	snd_pcm_t *pcm;			/*alsa pcm handle*/
	snd_pcm_stream_t dir;		/*stream direction playback/capture */
	char *name;			/*alsa device name*/
	unsigned int n_streams;		/*number of user streams attached to alsa device*/
	unsigned int rate;		/*samplerate for device*/
	snd_pcm_uframes_t ideal_period_size;/*preferred period size*/
	unsigned int ideal_period_time;/*preferred period time in ms*/
	snd_pcm_uframes_t silence_prefill;/*<=2*period_size !!*/
	snd_pcm_uframes_t buffer_size;  /*buffer size in frames*/
	unsigned long bf_channels;	/*bitfield of used channels*/
	unsigned int channels;		/*configured channels max_channel+1..n */
	unsigned int order;		/*setup order*/
	snd_pcm_format_t format;	/*sample format*/
	snd_pcm_access_t access;	/*configured access method*/
	snd_pcm_channel_area_t *areas;	/*intermediate areas for interleaved access*/
	snd_pcm_channel_area_t silence_area;/*prepared silence area*/
	struct rts_stream *s[RTS_MAX_STREAMS];/*attached user streams*/
	XferFunc xfer;			/*transfer function - depends on access method*/
	int init_wait;			/*tout in ms for initial wait*/
	int stream_wait;		/*tout in ms for stream wait*/

	/*runtime data - to be initialised on every start/recovery*/
	char wait_done;			/*initial wait done*/
	snd_pcm_uframes_t startup_xfer; /*counter on startup to detect remote side active*/
};

/*representation of a user stream*/
struct rts_stream {
	struct rts_adev *adev;		/*attached alsa device*/
	unsigned int channel;		/*attached channel 0..n*/
	snd_pcm_channel_area_t area;	/*first=0, step=format_width, adr=update on access*/
};

/*representation of input/output side*/
struct rts_side {
	unsigned int started;		/*0:stream stopped, !0:running*/
	snd_pcm_stream_t dir;		/*direction playback/capture*/
	unsigned int n_s;		/*no of streams*/
	unsigned int n_a;		/*no of devices*/
	struct rts_stream s[RTS_MAX_STREAMS];/*streams*/
	struct rts_adev a[RTS_MAX_STREAMS];/*devices*/
	struct rts_runtime* rt;		/*reference to runtime*/
};

/*streaming instance*/
struct rts_runtime {
	unsigned int magic;		/*magic to validate handle*/
	pthread_t tid;			/*tid to validate context*/
	struct rts_side in;		/*input side (capture)*/
	struct rts_side out;		/*output side(playback)*/
	snd_output_t *log;		/*logging*/
	int broken;			/*last returned error on read/write*/
	int aborted;			/*flag indication streaming is aborted*/
	/*options*/
	int features;			/*featurebits @see RTS_FEAT_...*/
	int prefill_ms;			/*prefill in ms*/
	int extra_tout_ms;		/*additional timout in ms*/
	int recover_delay_ms;		/*delay after recover in ms*/
	int loglevel;			/*@see RTS_LOGL_...*/
	/*statistics*/
	unsigned int num_xruns;		/*number of xruns*/
	snd_pcm_status_t *status;	/*pcm status*/
};

struct rts_feature {
	const char *name;		/*feature name*/
	unsigned long bitmask;		/*corresponding bit*/
};


static struct rts_feature rts_features[] = {
	{"RTS_FEAT_DISABLE_XRUN_DETECT", RTS_FEAT_DISABLE_XRUN_DETECT},
	{"RTS_FEAT_FORCE_CFG_RELOAD", RTS_FEAT_FORCE_CFG_RELOAD},
	{"RTS_FEAT_MANUAL_START", RTS_FEAT_MANUAL_START},
	{"RTS_FEAT_INJECT_XRUN", RTS_FEAT_INJECT_XRUN},
	{"RTS_FEAT_LOG_TSTAMP", RTS_FEAT_LOG_TSTAMP},
	{"RTS_FEAT_LAZY_SETTINGS", RTS_FEAT_LAZY_SETTINGS},
	{NULL, 0},
};

#define RTS_FEATURE_ENABLED(r, feat) (((r)->features & (feat)) != 0)
#define RTS_CHK_HNDL(h) if ((h == NULL) || (h->magic != RTS_MAGIC_OK)) { RTS_ERR_EARLY("invalid handle\n");return -EINVAL;}
#ifdef RTS_CHK_CONTEXT_ENABLED
#define RTS_CHK_CTX(h) do {pthread_t _tid = pthread_self(); if (_tid != h->tid) { RTS_ERR_EARLY("Enter %s from invalid context -caller %d != creator %d\n",__FUNCTION__, (unsigned int)_tid, (unsigned int)h->tid);return -EACCES;}} while (0)
#else
#define RTS_CHK_CTX(h) do {} while (0)
#endif
#define RTS_ENTRY(h) do {RTS_CHK_HNDL(h); RTS_CHK_CTX(h);} while (0)

#define RTS_WAIT_ADDON_MS 20

#define RTS_AD_DIR_STRING(ad) (((ad)->dir==SND_PCM_STREAM_PLAYBACK)?" P":" C")
/**
 * \brief Start all devices of given side
 * \param side side handle
 * \return 0: OK, or <0 error
 *
 * Starts all device of a given side. Stop on first failing device
 * 
 */
static int rts_start_devs(struct rts_side *side)
{
	unsigned int i;
	int err = 0;
	for (i = 0; i < side->n_a && (err == 0); i++) {
		err = snd_pcm_start(side->a[i].pcm);
		if (err)
			RTS_LOG(side->rt, RTS_LOGL_STREAM_ERR,"snd_pcm_start failed with %d (%s) on dev %s%s\n", err, snd_strerror(err),side->a[i].name, RTS_AD_DIR_STRING(&side->a[i]) );
	}
	return err;
}


/**
 * \brief Stop all devices of both sides of given runtime
 * \param rt runtime handle
 * \return 0: OK, <0 error
 *
 */
static int rts_stop_all_adevs(struct rts_runtime *rt)
{
	unsigned int i;
	struct rts_side *side;
	int err = 0;
	int tmp_err = 0;

	side = &rt->in;
	side->started = 0;
	for (i = 0; i < side->n_a; i++) {
		side->a[i].wait_done = 0;
		side->a[i].startup_xfer = 0;
		tmp_err = snd_pcm_drop(side->a[i].pcm);
		if (err == 0)
			err = tmp_err;
	}
	side = &rt->out;
	side->started = 0;
	for (i = 0; i < side->n_a; i++) {
		side->a[i].wait_done = 0;
		side->a[i].startup_xfer = 0;
		tmp_err = snd_pcm_drop(side->a[i].pcm);
		if (err == 0)
			err = tmp_err;
	}
	return err;
}

/**
 * \brief Abort all devices of both sides of given runtime
 * \param rt runtime handle
 * \return 0: OK, <0 error
 *
 */
static int rts_abort_all_adevs(struct rts_runtime *rt)
{
	unsigned int i;
	struct rts_side *side;
	int err = 0;
	int tmp_err = 0;

	side = &rt->in;
	for (i = 0; i < side->n_a; i++) {
		tmp_err = snd_pcm_abort(side->a[i].pcm);
		if (err == 0)
			err = tmp_err;
	}
	side = &rt->out;
	for (i = 0; i < side->n_a; i++) {
		tmp_err = snd_pcm_abort(side->a[i].pcm);
		if (err == 0)
			err = tmp_err;
	}
	return err;
}


/**
 * \brief Prepare all devices of both sides of given runtime
 * \param rt runtime handle
 * \return 0: OK, <0 error
 *
 */
static int rts_prepare_all_adevs(struct rts_runtime *rt)
{
	unsigned int i;
	struct rts_side *side;
	int err = 0;
	side = &rt->in;
	for (i = 0; i < side->n_a && (err == 0); i++)
		err = snd_pcm_prepare(side->a[i].pcm);

	side = &rt->out;
	for (i = 0; i < side->n_a && (err == 0); i++)
		err = snd_pcm_prepare(side->a[i].pcm);
	return err;
}

/**
 * \brief Close all devices of both sides of given runtime
 * \param rt runtime handle
 * \return 0: OK, <0 error
 *
 */
static int rts_close_all_adevs(struct rts_runtime *rt)
{
	unsigned int i;
	struct rts_side *side;

	side = &rt->in;
	for (i = 0; i < side->n_a; i++) {
		if (side->a[i].pcm != NULL)
			(void)snd_pcm_close(side->a[i].pcm);
		side->a[i].pcm = NULL;
	}
	side = &rt->out;
	for (i = 0; i < side->n_a; i++) {
		if (side->a[i].pcm != NULL)
			(void)snd_pcm_close(side->a[i].pcm);
		side->a[i].pcm = NULL;
	}
	return 0;
}

#define MAX_BLOCK_MS 100 /*maximum time to block*/
/**
 * \brief Wait for availability of given device
 * \param rt runtime
 * \param ad audio device
 * \return 0: OK, <0 error
 *
 */
static int rts_wait_adev(struct rts_runtime *rt, struct rts_adev *ad)
{
	snd_pcm_sframes_t avail;
	int err;
	int wait_time_ms = ad->wait_done ? ad->stream_wait : ad->init_wait;
	int wait_tries;
	if (wait_time_ms != RTS_TOUT_ENDLESS) {
	    wait_tries = wait_time_ms/MAX_BLOCK_MS;
	    if (wait_time_ms % MAX_BLOCK_MS)
		    wait_tries++;
	    wait_time_ms /= wait_tries;
    } else {
        /*endless timeout*/
        wait_time_ms = MAX_BLOCK_MS;
        wait_tries = -1;
    }

	 while (wait_tries != 0) {
        if (wait_tries != -1)
            wait_tries--;
		avail = snd_pcm_avail_update(ad->pcm);
		if (avail < 0) {
			RTS_LOG(rt, RTS_LOGL_STREAM_ERR, "WAIT AVAIL ERR %ld (%s) on dev %s%s\n", avail, snd_strerror(avail), ad->name, RTS_AD_DIR_STRING(ad));
			return avail;
		}
		err = snd_pcm_wait(ad->pcm, wait_time_ms);
		if (rt->aborted) {
			RTS_LOG(rt, RTS_LOGL_TRACE_INSTANCE,"WAIT ABORTED on dev %s%s\n", ad->name, RTS_AD_DIR_STRING(ad));
			return (err == -EINTR)?err:-ECANCELED;
        }
		if (err <= 0) {
			if (err < 0) {
				RTS_LOG(rt, RTS_LOGL_STREAM_ERR, "WAIT ERR %d (%s) on dev %s%s\n", err, snd_strerror(err), ad->name, RTS_AD_DIR_STRING(ad));
				return err;
			}
			if (!wait_tries) {
				RTS_LOG(rt, RTS_LOGL_STREAM_ERR,"WAIT %s TOUT dev %s%s->END\n",ad->wait_done ?"stream":"initial", ad->name, RTS_AD_DIR_STRING(ad));
				return -ENODATA;
			}
			RTS_LOG(rt, RTS_LOGL_STREAM_ERR,"WAIT %s TOUT dev %s%s-->RETRY %d\n",ad->wait_done ?"stream":"initial", ad->name, RTS_AD_DIR_STRING(ad), wait_tries );
		} else
            break;
	};

	/*bogus return from poll possible: clear on first successful read/write*/
	/*ad->wait_done = 1; */
	return 0;
}

/**
 * \brief Wait for availability of all devices of a given side
 * \param side side handle
 * \return 0: OK, <0 error
 *
 */
static int rts_wait_adevs(struct rts_side *side)
{
	unsigned int i;
	int err;
	for (i = 0; i < side->n_a; i++) {
		err = rts_wait_adev(side->rt, &side->a[i]);
		if (err != 0)
			return err;
	}
	return 0;
}


#define rts_return_on_err(rt, h, e, s) \
	do {\
		if (e < 0) {\
			RTS_ERR(rt, "%s failed with err %d (%s) for stream %s%s\n", s, e, snd_strerror(e), h->name, RTS_AD_DIR_STRING(h));\
			if (rt->loglevel >= RTS_LOGL_CONFIG) {\
				snd_pcm_hw_params_dump(hw_params, rt->log);\
				snd_output_flush(rt->log);\
			}\
			return e;\
		}\
	} while (0)


/**
 * \brief Configure ALSA device
 * \param rt runtime handle
 * \param ad device handle
 * \return 0: OK, <0 error
 *
 */
static int rts_configure_adev(struct rts_runtime *rt, struct rts_adev *ad)
{
#define RTS_ACCESS_TYPES 4
	snd_pcm_t *pcm;
	int err;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	unsigned int i;
	snd_pcm_uframes_t buffer_size_requested;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t period_size;
	unsigned char dividers[] = {2,3,5,6,7,10,0}; /*zero terminated list of dividers*/
	unsigned int curr_div;
	unsigned int open_min_max = 1;	/*default on first round: allow open interval*/
	unsigned int periods_integer = 0; /*default on first round: no integer constraint on periods*/

	/*setup acces types in preferred order*/
	snd_pcm_access_t pcm_access[RTS_ACCESS_TYPES] = {
		SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
		SND_PCM_ACCESS_RW_NONINTERLEAVED,
		SND_PCM_ACCESS_MMAP_INTERLEAVED,
		SND_PCM_ACCESS_RW_INTERLEAVED
	};

	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_sw_params_alloca(&sw_params);
	RTS_LOG(rt, RTS_LOGL_CONFIG,"RTS CONFIG %s rate %d channels %d psize %ld\n", ad->name, ad->rate, ad->channels, ad->ideal_period_size);

	err = snd_pcm_open(&pcm, ad->name, ad->dir, SND_PCM_NONBLOCK);
	rts_return_on_err(rt, ad, err, "snd_pcm_open");

	ad->pcm = pcm;

retry:
	err = snd_pcm_hw_params_any(pcm, hw_params);/*undocumented: returns change mask >=0 or error <0 */
	rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_any");

	err = snd_pcm_hw_params_set_format(pcm, hw_params, ad->format);
	rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_format");

	err = snd_pcm_hw_params_set_channels_min(pcm, hw_params, &ad->channels);
	rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_channels");

	err = snd_pcm_hw_params_set_channels_first(pcm, hw_params, &ad->channels);
	rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_channels_first");

	err = snd_pcm_hw_params_set_rate(pcm, hw_params, ad->rate, 0);
	rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_rate");

	for (i = 0; i < RTS_ACCESS_TYPES; i++) {
		err = snd_pcm_hw_params_set_access(pcm, hw_params, pcm_access[i]);
		if (err == 0) {
			ad->access = pcm_access[i];
			break;
		}
	}
	rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_access");

	err = -EINVAL;
	for (i = 0; (dividers[i] != 0) && err; i++) {
		curr_div = dividers[i];
		period_size = ad->ideal_period_size;
		do {
			snd_pcm_uframes_t psize_min = period_size;
			snd_pcm_uframes_t psize_max = period_size;
			int mindir = -1;
			int maxdir = +1;
			if (!open_min_max) {
				mindir = 0;
				maxdir = 0;
			}

			err = snd_pcm_hw_params_set_period_size_minmax(pcm, hw_params, &psize_min, &mindir, &psize_max, &maxdir);
			if (!err) /*don't change period_size on success, as it might be used later*/
				break;
			period_size = ad->ideal_period_size/curr_div;
			curr_div *= dividers[i];
		} while ((period_size >= 16) && err);/*reasonable limit*/
	}

	if ((err < 0) && RTS_FEATURE_ENABLED(rt, RTS_FEAT_LAZY_SETTINGS)) {
		period_size = ad->ideal_period_size;
		err = snd_pcm_hw_params_set_period_size_near(pcm, hw_params, &period_size, NULL);
		RTS_LOG(rt, RTS_LOGL_CONFIG,"retry lazy period size %ld -> %ld, ret %d",ad->ideal_period_size, period_size, err);
		/*no buffer size set. This uses maximum buffer...*/
		err = snd_pcm_hw_params(pcm, hw_params);
		rts_return_on_err(rt, ad, err, "snd_pcm_hw_params");

		err = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
		rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_get_buffer_size");
		ad->buffer_size = buffer_size;
		ad->silence_prefill = 0; /*no prefill, we start at (nearly) full buffer*/

		err = snd_pcm_sw_params_current(pcm, sw_params);
		rts_return_on_err(rt, ad, err, "snd_pcm_sw_params_current");

		if (ad->dir == SND_PCM_STREAM_CAPTURE)
			err = snd_pcm_sw_params_set_start_threshold(pcm, sw_params, 1);
		else
			err = snd_pcm_sw_params_set_start_threshold(pcm, sw_params, min(buffer_size-(buffer_size%ad->ideal_period_size),buffer_size-(buffer_size%period_size)));
		rts_return_on_err(rt, ad, err, "snd_pcm_sw_params_set_start_threshold");
	} else {
		rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_period_size");

		buffer_size = ad->ideal_period_size;
		/*extra space for silence prefill*/
		buffer_size += (ad->silence_prefill/ad->ideal_period_size)*ad->ideal_period_size;
		if (ad->silence_prefill % ad->ideal_period_size)
			buffer_size += ad->ideal_period_size;

		buffer_size_requested = buffer_size;
		err = snd_pcm_hw_params_set_buffer_size_min(pcm, hw_params, &buffer_size);
		rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_buffer_size_min");

		if (periods_integer) {
			err = snd_pcm_hw_params_set_periods_integer(pcm, hw_params);
			rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_periods_integer");
		}
		err = snd_pcm_hw_params_set_buffer_size_near(pcm, hw_params, &buffer_size);
		rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_set_buffer_size_near");

		ad->buffer_size = buffer_size;

		err = snd_pcm_hw_params(pcm, hw_params);
		rts_return_on_err(rt, ad, err, "snd_pcm_hw_params");

		/* rate plugin shows strange rounding issues which results in 
		a) rounded periods (requesting 64 results in 63) 
			or/and
		b) rounding buffer size (requesting buffer of 3*64 results in 191)

		To overcome this, we retry configuration with different approaches:
		-disabling 'direction' (leading to open intervals on calculation) on parameter 'period_size' should avoid rounding period size.
		-setting integer constraint on parameter 'periods' should avoid rounding buffer size
		-we can't use this settings by default as this may break configuration on other plugins
			(e.g. enabling 'direction' is currently necessary for rate plugin on top of dshare plugin)
		*/
		{
		snd_pcm_uframes_t size;		
		int dir;		
		err = snd_pcm_hw_params_get_period_size(hw_params, &size, &dir);
		rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_get_period_size");
		/*resulting period_size must match the requested size, otherwise this might result in a huge rate deviation on a rate plugin:
		easy example: requesting to convert 64frames@8000Hz to 64frames@8000Hz would result in effective rate of 8000Hz*64/63 = ~8127Hz if period 63 is accepted.
		*/
		if ((size != period_size) || (dir !=0)) {
			if (open_min_max) {/*retry only once*/
				RTS_LOG(rt, RTS_LOGL_CONFIG,"period size %ld not available (got %ld) retry...\n",period_size, size);
				snd_pcm_hw_free(pcm);
				open_min_max = 0; /*retry with rounding not allowed..*/
				goto retry; /*ugly, I know... */
			} else {
				RTS_ERR(rt, "period size %ld not available (got %ld) NO retry...\n",period_size, size);
				err = -EINVAL;
				rts_return_on_err(rt, ad, err, "size == period_size");
			}
		}

		err = snd_pcm_hw_params_get_buffer_size(hw_params, &size);
		rts_return_on_err(rt, ad, err, "snd_pcm_hw_params_get_buffer_size");
		/*overall buffer size must be period aligned - 
		rate plugin can produce 100% CPU load due to deadlock situation:
		snd_pcm_wait() returns success as slave might have enaugh space while a following
		snd_pcm_writei() can't write anything as rate is waiting for a full period provided by user which is not (yet) fitting*/
		if (size % ad->ideal_period_size) {
			if (!periods_integer) {/*retry only once*/
				RTS_LOG(rt, RTS_LOGL_CONFIG,"buffer_size %ld not available (got %ld) retry...\n",buffer_size_requested, size);
				snd_pcm_hw_free(pcm);
				periods_integer = 1; /*retry with integer periods..*/
				goto retry; /*ugly, I know... */
			} else {
				RTS_ERR(rt, "buffer_size %ld not available (got %ld) NO retry...\n",buffer_size_requested, size);
				err = -EINVAL;
				rts_return_on_err(rt, ad, err, "size == buffer_size");
			}
		}
		}

		err = snd_pcm_sw_params_current(pcm, sw_params);
		rts_return_on_err(rt, ad, err, "snd_pcm_sw_params_current");

		if (ad->dir == SND_PCM_STREAM_CAPTURE)
			err = snd_pcm_sw_params_set_start_threshold(pcm, sw_params, 1);
		else
			err = snd_pcm_sw_params_set_start_threshold(pcm, sw_params, ad->ideal_period_size+ad->silence_prefill);
		rts_return_on_err(rt, ad, err, "snd_pcm_sw_params_set_start_threshold");
	}

	if (RTS_FEATURE_ENABLED(rt, RTS_FEAT_DISABLE_XRUN_DETECT)) {
		err = snd_pcm_sw_params_set_stop_threshold(pcm, sw_params, MAXINT);
		rts_return_on_err(rt, ad, err, "snd_pcm_sw_params_set_stop_threshold");
	}
	err = snd_pcm_sw_params_set_avail_min(pcm, sw_params, ad->ideal_period_size);
	rts_return_on_err(rt, ad, err, "snd_pcm_sw_params_set_avail_min");

	err = snd_pcm_sw_params(pcm, sw_params);
	rts_return_on_err(rt, ad, err, "snd_pcm_sw_params");

	if (rt->loglevel >= RTS_LOGL_CONFIG) {
		snd_pcm_dump(pcm, rt->log);
		snd_output_flush(rt->log);
	}
	return err;
}


/**
 * \brief Allocate area for interleaved access
 * \param ad device handle
 * \return 0: OK, <0 error
 *
 */
static int rts_alloc_adev_area(struct rts_runtime *rt, struct rts_adev *ad)
{
	unsigned int i = 0;

	ad->areas = calloc(ad->channels, sizeof(snd_pcm_channel_area_t));
	if (ad->areas == NULL) {
		RTS_ERR(rt, "alloc channel area failed\n");
		return -ENOMEM;
	}

	ad->areas[0].addr = malloc(ad->channels * snd_pcm_format_size(ad->format, 1) * ad->ideal_period_size);
	if (ad->areas[0].addr == NULL) {
		free(ad->areas);
		ad->areas = NULL;
		RTS_ERR(rt, "alloc channel area addr failed\n");
		return -ENOMEM;
	}
	for (i = 0; i < ad->channels; i++) {
		ad->areas[i].addr = ad->areas[0].addr;
		ad->areas[i].first = i * snd_pcm_format_width(ad->format);
		ad->areas[i].step = snd_pcm_format_width(ad->format) * ad->channels;
		snd_pcm_area_silence(&ad->areas[i], 0, ad->ideal_period_size, ad->format);
	}
	return 0;
}


/**
 * \brief Setup streaming buffer and access to ALSA device
 * \param rt runtime
 * \param ad device handle
 * \return 0: OK, <0 error
 *
 */
static int rts_configure_adev_area(struct rts_runtime *rt, struct rts_adev *ad)
{
	int ret = 0;
	(void) rt;
	/*create silence area for later usage*/
	ad->silence_area.addr = malloc(snd_pcm_format_size(ad->format, 1) * ad->ideal_period_size);
	if (ad->silence_area.addr == NULL) {
		RTS_ERR(rt, "no memory for silence area\n");
		return -ENOMEM;
	}
	ad->silence_area.first = 0;
	ad->silence_area.step = snd_pcm_format_width(ad->format);
	snd_pcm_area_silence(&ad->silence_area, 0, ad->ideal_period_size, ad->format);

	switch (ad->access) {

	case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
		/*on non interleaved acces we can directly pass user buffers*/
		if (ad->dir == SND_PCM_STREAM_PLAYBACK)
			ad->xfer = (XferFunc) snd_pcm_mmap_writen;
		else
			ad->xfer = (XferFunc) snd_pcm_mmap_readn;
		break;

	case SND_PCM_ACCESS_RW_NONINTERLEAVED:
		/*on non interleaved acces we can directly pass user buffers*/
		if (ad->dir == SND_PCM_STREAM_PLAYBACK)
			ad->xfer = (XferFunc) snd_pcm_writen;
		else
			ad->xfer = (XferFunc) snd_pcm_readn;
		break;

	case SND_PCM_ACCESS_MMAP_INTERLEAVED:
		/*on interleaved acces we first have to copy user buffers to an intermediate buffer*/
		if (ad->dir == SND_PCM_STREAM_PLAYBACK)
			ad->xfer = (XferFunc) snd_pcm_mmap_writei;
		else
			ad->xfer = (XferFunc) snd_pcm_mmap_readi;
		ret = rts_alloc_adev_area(rt, ad);
		break;

	case SND_PCM_ACCESS_RW_INTERLEAVED:
		/*on interleaved acces we first have to copy user buffers to an intermediate buffer*/
		if (ad->dir == SND_PCM_STREAM_PLAYBACK)
			ad->xfer = (XferFunc) snd_pcm_writei;
		else
			ad->xfer = (XferFunc) snd_pcm_readi;
		ret = rts_alloc_adev_area(rt, ad);
		break;
	default:
		RTS_ERR(rt, "invalid access %d\n", ad->access);
		return -EINVAL;
	}
	return ret;
}


/**
 * \brief Execute read/write on ALSA device
 * \param rt runtime
 * \param ad device handle
 * \param do_silence 0: normal read write !0: silence prefill frames
 * \return 0: OK, <0 error
 *
 */
static inline int rts_process_pcm_rw(struct rts_runtime *rt, struct rts_adev *ad, snd_pcm_uframes_t do_silence)
{
	unsigned int i;
	unsigned int retry_done = 0;
	int err = 0;
	void *bufs_i[ad->channels];
	snd_pcm_uframes_t xfer_size;
	snd_pcm_uframes_t offset = 0;
	snd_pcm_channel_area_t *src_area;

	if (do_silence != 0)
		xfer_size = do_silence;
	else
		xfer_size = ad->ideal_period_size;

	while ((xfer_size > 0) && (err == 0) ) {
		switch (ad->access) {
		case SND_PCM_ACCESS_MMAP_INTERLEAVED:
		case SND_PCM_ACCESS_RW_INTERLEAVED:
			/*copy used areas - unused are already prefilled with silence*/
			if (ad->dir == SND_PCM_STREAM_PLAYBACK) {
				for (i = 0; i < ad->n_streams; i++) {
					if (do_silence != 0)
						src_area = &ad->silence_area;
					else
						src_area = &ad->s[i]->area;
					snd_pcm_area_copy(&ad->areas[ad->s[i]->channel], 0,
					      src_area, offset, xfer_size, ad->format);
				}
			}
			do {/*EINTR should not occur, as we operate in nonblocking mode*/
				err = ad->xfer(ad->pcm, ad->areas[0].addr, xfer_size);
			} while (err == -EINTR);

			if (err >= 0) {
				if (ad->dir == SND_PCM_STREAM_CAPTURE) {
					for (i = 0; i < ad->n_streams; i++) {
						snd_pcm_area_copy(&ad->s[i]->area, offset,
					      &ad->areas[ad->s[i]->channel], 0, err, ad->format);
					}
				}
			}
			break;

		case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
		case SND_PCM_ACCESS_RW_NONINTERLEAVED:
			/*map available user buffers, unused channels mapped to silence*/
			memset(bufs_i, 0, sizeof(void*) * ad->channels);
			if (do_silence == 0) {
				for (i = 0; i < ad->n_streams; i++) {
					bufs_i[ad->s[i]->channel] = ((char*)(ad->s[i]->area.addr) + snd_pcm_samples_to_bytes(ad->pcm, offset));
				}
			}
			for (i = 0; i < ad->channels; i++) {
				if (bufs_i[i] == NULL)
					bufs_i[i] = (char*)(ad->silence_area.addr) + snd_pcm_samples_to_bytes(ad->pcm, offset);
			}
			do {/*EINTR should not occur, as we operate in nonblocking mode*/
				err = ad->xfer(ad->pcm, bufs_i, xfer_size);
			} while (err == -EINTR);
			break;

		default:
			RTS_LOG(rt, RTS_LOGL_STREAM_ERR,"rw: invalid access %d\n", ad->access);
			err = -EINVAL;
			break;
		}

		if (err >= 0) {
			if ((err > 0) && (!ad->wait_done)){
				ad->startup_xfer += err;
				if (ad->dir == SND_PCM_STREAM_CAPTURE) {
					ad->wait_done = 1;
				} else {
					if (ad->startup_xfer > ad->buffer_size)
						ad->wait_done = 1;
				}
			}
			if (err > (int)xfer_size) {
				RTS_LOG(rt, RTS_LOGL_STREAM_ERR,"invalid size %d\n", err);
				err = -EINVAL;
			} else {
				offset += err;
				xfer_size -= err;
				err = 0;
			}
		}

		if (err == -EAGAIN) {
			/*
			Non period aligned write is buggy in libasound < 1.0.27:
			Writing smaller buffers than period size can result in 100% CPU load, as snd_pcm_wait()
			succeeds, while following write access returs -EAGAIN.
			Fixed with commit 'PCM: Avoid busy loop in snd_pcm_write_areas() with rate plugin'.
			As a workaround we do a short sleep if we get repeated EAGAIN's
			*/
			if (retry_done != 0)
				usleep(1000);
			retry_done = 1;
			err = rts_wait_adev(rt, ad);
		}
	}

	return err;
}


/**
 * \brief Execute read/write on all ALSA devices of given side
 * \param side side handle
 * \return 0: OK, <0 error
 *
 */
static int rts_rw_buffers(struct rts_side *side)
{
	unsigned int i;
	int err = 0;
	for (i = 0; i < side->n_a; i++) {
		err = rts_process_pcm_rw(side->rt, &side->a[i], 0);
		if (err < 0) {
			RTS_LOG(side->rt, RTS_LOGL_STREAM_ERR,"RW ERROR %d (%s) on dev %s%s\n", err, snd_strerror(err), side->a[i].name, RTS_AD_DIR_STRING(&side->a[i]));
			break;
		}
	}
	return err;
}


/**
 * \brief Map user buffers to ALSA device areas
 * \param side side handle
 * \param buffers user buffers
 *
 */
static void rts_setup_areas(struct rts_side *side, void *buffers[])
{
	unsigned int i;
	for (i = 0; i < side->n_s; i++)
		side->s[i].area.addr = buffers[i];
}


/**
 * \brief Dump status of all ALSA devices
 * \param rt runtime handle
 *
 */
static void rts_dump_all(struct rts_runtime *rt)
{
	unsigned int i;
	struct rts_side *side;

	side = &rt->in;
	for (i = 0; i < side->n_a; i++) {
		if (side->a[i].pcm != NULL) {
			snd_output_printf(rt->log, "STATUS of device %s%s :%s\n",
				side->a[i].name, RTS_AD_DIR_STRING(&side->a[i]), snd_pcm_state_name((snd_pcm_state_t)snd_pcm_state(side->a[i].pcm)));
			snd_pcm_hwsync(side->a[i].pcm);
			snd_pcm_status(side->a[i].pcm, rt->status);
			snd_pcm_status_dump(rt->status, side->rt->log);
		}
	}

	side = &rt->out;
	for (i = 0; i < side->n_a; i++) {
		if (side->a[i].pcm != NULL) {
			snd_output_printf(rt->log, "STATUS of device %s%s :%s\n",
				side->a[i].name, RTS_AD_DIR_STRING(&side->a[i]), snd_pcm_state_name((snd_pcm_state_t)snd_pcm_state(side->a[i].pcm)));
			snd_pcm_hwsync(side->a[i].pcm);
			snd_pcm_status(side->a[i].pcm, rt->status);
			snd_pcm_status_dump(rt->status, side->rt->log);
		}
	}
	snd_output_flush(rt->log);
}


/**
 * \brief Prefill outputs with silence
 * \param side side handle
 * \return 0: OK, <0 error
 *
 */
static int rts_prefill_silence(struct rts_side *side)
{
	unsigned int i;
	int err = 0;
	snd_pcm_uframes_t silence_remain;
	for (i = 0; i < side->n_a; i++) {
		silence_remain = side->a[i].silence_prefill;
		while ((silence_remain > 0) && (err == 0)) {
			snd_pcm_uframes_t chunk = min(silence_remain, side->a[i].ideal_period_size);
			err = rts_process_pcm_rw(side->rt, &side->a[i], chunk);
			silence_remain -= chunk;
		}
		if (err < 0) {
			RTS_LOG(side->rt, RTS_LOGL_STREAM_ERR,"RW SILENCE ERROR %d (%s) on dev %s%s\n", err, snd_strerror(err), side->a[i].name, RTS_AD_DIR_STRING(&side->a[i]));
			break;
		}
	}
	return err;
}


/**
 * \brief Recover devices after XRUN
 * \param rt runtime handle
 * \return 0: OK, <0 error
 *
 */
static int rts_recover_all_adevs(struct rts_runtime *rt)
{
	int err = rts_stop_all_adevs(rt);
	if (err != 0)
		return err;
	err =  rts_prepare_all_adevs(rt);
	return err;
}


/**
 * \brief Process read /write on given side
 * \param side side handle
 * \param buffers user buffers
 * \return 0: OK, <0 error
 *
 * Processing includes:
 * -waiting for availability of all devices of the given side
 * -executing real read/write
 * -do recovery on detected xrun
 */
static int rts_rw(struct rts_side *side, void *buffers[])
{
	int err = 0;
	int tmp_err;
	static int cnt = 0;
	int retry_cnt = 0;
#define NUM_TRY_RECOVER 10

	if (side->rt->aborted != 0)
		return -ECANCELED;

	if (side->rt->broken != 0)
		return -EINVAL;

	rts_setup_areas(side, buffers);

	/*force xrun if option set*/
	if (RTS_FEATURE_ENABLED(side->rt, RTS_FEAT_INJECT_XRUN)) {
		cnt++;
		if ((cnt % 200) == 0)
			usleep(30000);
	}

	do {
		err = 0;
		if ((side->started == 0) && (side->dir == SND_PCM_STREAM_CAPTURE)) {
			side->started = 1;
			RTS_LOG(side->rt, RTS_LOGL_TRACE_INSTANCE,"START CAPTURE\n");
			err = rts_start_devs(side);
		}
		if (err != 0)
			break;/*no recover */

		if ((side->started == 0) && (side->dir == SND_PCM_STREAM_PLAYBACK)) {
			/*side->started=1;*/
			err = rts_prefill_silence(side);
		}
		if (err && (err != -EPIPE))
			break;/*recover EPIPE on prefill (can happen on DMIX/DSHARE)*/

		if (!err) {
			err = rts_wait_adevs(side);
			if (err == 0)
				err = rts_rw_buffers(side);
		}

		if (err == -EPIPE) {
			if (side->rt->loglevel >= RTS_LOGL_DUMP_XRUN)
				rts_dump_all(side->rt);

			side->rt->num_xruns++;
			tmp_err = rts_recover_all_adevs(side->rt);
			if (side->rt->recover_delay_ms != 0) {
				if ((side->rt->recover_delay_ms > 0) && (retry_cnt > 0))
					usleep(side->rt->recover_delay_ms * 1000);
				if (side->rt->recover_delay_ms == RTS_RECOVER_DELAY_EXIT)
					break;
			}
			if (tmp_err != 0) {
				RTS_LOG(side->rt, RTS_LOGL_STREAM_ERR,"RECOVER ERR %d (%s)\n", tmp_err, snd_strerror(tmp_err));
				err = tmp_err;
			} else {
				if ((side->dir == SND_PCM_STREAM_PLAYBACK) && (side->rt->in.n_s != 0)) {
					/*playback prefill/start on next write call if input devices exist*/
					err = 0;
					break;
				}
			}
		}

		if (err == 0) {
			if ((side->started == 0) && (side->dir == SND_PCM_STREAM_PLAYBACK)) {
				side->started = 1;
				if (RTS_FEATURE_ENABLED(side->rt, RTS_FEAT_MANUAL_START))
					rts_start_devs(side);
			}
		}
		retry_cnt++;
	} while ((err == -EPIPE) && (retry_cnt <= NUM_TRY_RECOVER));

	if (err != 0) {
		RTS_LOG(side->rt, RTS_LOGL_STREAM_ERR,"STREAM ABORTED WITH ERR %d (%s) retry %d/%d\n", err, snd_strerror(err), retry_cnt,NUM_TRY_RECOVER );
		if (side->rt->loglevel >= RTS_LOGL_DUMP_XRUN)
			rts_dump_all(side->rt);
		(void)rts_stop_all_adevs(side->rt);
		side->rt->broken = err;
	}
	return err;
}


/**
 * \brief Get alsa device with given name if already existing
 * \param ad list of devices
 * \param name name to look up
 * \return NULL: not found, <0 pointer to found entry
 *
 */
static struct rts_adev* rts_get_adev(struct rts_adev *ad, const char *name)
{
	unsigned int i;

	for (i = 0; i < RTS_MAX_STREAMS; i++) {
		if (ad[i].name != NULL) {
			if (strcmp(ad[i].name, name) == 0)
				return &ad[i];
		}
	}
	return NULL;
}


/**
 * \brief Parse features given via environment
 * \param rt runtime handle
 * \return 0: OK
 *
 */
static int rts_set_env_features(struct rts_runtime *rt)
{
	char *env;
	struct rts_feature *feat = rts_features;

	if (RTS_FEATURE_ENABLED(rt, RTS_FEAT_DISABLE_ENV)) {
		RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES: Disabled feature setting via environment\n");
		return 0;
	}

	/*make loglevel setting affect immediately*/
	env = getenv("RTS_LOGLEVEL");
	if ((env != NULL) && ((*env) != '\0') && (atoi(env) > 0)) {/*allow >0*/
		rt->loglevel = atoi(env);
		RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES ENV: RTS_LOGLEVEL = %d\n", rt->loglevel);
	}

	/*feature switches...*/
	while (feat->name != NULL) {
		env = getenv(feat->name);
		if ((env != NULL) && ((*env) != '\0')) {
			if (atoi(env) > 0) {
				rt->features |= feat->bitmask;
				RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES ENV: Enable feature %s\n", feat->name);
			} else {
				rt->features &= ~feat->bitmask;
				RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES ENV: Disable feature %s ->0x%x\n", feat->name, rt->features);
			}
		}
		feat++;
	}

	/*other settings...*/
	env = getenv("RTS_PLAYBACK_PREFILL");
	if ((env != NULL) && ((*env) != '\0') && (atoi(env) >= 0)) {/*allow 0 !*/
		rt->prefill_ms = atoi(env);
		RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES ENV: RTS_PLAYBACK_PREFILL = %d\n", rt->prefill_ms);
	}

	env = getenv("RTS_EXTRA_TIMEOUT");
	if ((env != NULL) && ((*env) != '\0') ) {
        if (atoi(env) >= 0) {/*allow 0 !*/
		    rt->extra_tout_ms = atoi(env);
		    RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES ENV: RTS_EXTRA_TIMEOUT = %d\n", rt->extra_tout_ms);
        } else {
		    rt->extra_tout_ms = RTS_TOUT_ENDLESS;
		    RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES ENV: RTS_EXTRA_TIMEOUT = ENDLESS\n");
        }
	}

	env = getenv("RTS_RECOVER_DELAY");
	if ((env != NULL) && ((*env) != '\0') ) {
		if (atoi(env) >= 0) {/*allow 0*/
			rt->recover_delay_ms = atoi(env);
			RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES ENV: RTS_RECOVER_DELAY = %d\n", rt->recover_delay_ms);
		} else {
			rt->recover_delay_ms = RTS_RECOVER_DELAY_EXIT;
			RTS_LOG(rt, RTS_LOGL_CONFIG, "FEATURES ENV: RTS_RECOVER_DELAY = NO RECOVER\n");
		}
	}
	return 0;
}


/**
 * \brief Parse user configuration
 * \param cfg user configuration
 * \param rt handle to (empty) runtime
 * \return 0: OK
 *
 */
static int rts_parse_config(trts_cfg *cfg, struct rts_runtime *rt)
{
	struct rts_stream *s;
	struct rts_adev *ad;
	struct rts_side *side;
	trts_cfgadev *adev_cfg;
	trts_cfgstream *stream_cfg;
	unsigned int i;
	int err;

	rt->prefill_ms = cfg->prefill_ms;
	rt->extra_tout_ms = 0;
	rt->features = cfg->features;
	rt->recover_delay_ms = RTS_RECOVER_DELAY_DEFAULT;

	rts_set_env_features(rt);

	rt->in.rt = rt;
	rt->in.dir = SND_PCM_STREAM_CAPTURE;

	rt->out.rt = rt;
	rt->out.dir = SND_PCM_STREAM_PLAYBACK;

	err = snd_pcm_status_malloc(&rt->status);
	if (err < 0) {
		RTS_ERR(rt, "alloc status failed with err %d (%s)\n", err, snd_strerror(err));
		return err;
	}

	if (rt->prefill_ms < RTS_PREFILL_WARN_THRES)
		RTS_LOG(rt, RTS_LOGL_STREAM_ERR,"---- WARNING --- PREFILL OF %dms will probably cause xrun!!\n", rt->prefill_ms);

	/*iterate streams, check for valid entry, create adev entry if not exist, create stream entry.*/
	for (i = 0; i < cfg->num_streams; i++) {
		stream_cfg = &(cfg->streams[i]);

		if (stream_cfg->channel >= RTS_MAX_CHANNELS) {
			RTS_ERR(rt, "channel %d out of range (max %d - 1)\n", stream_cfg->channel, RTS_MAX_CHANNELS);
			return -EINVAL;
		}

		/*adev in range ?*/
		if (stream_cfg->adevidx >= cfg->num_adevs) {
			RTS_ERR(rt, "invalid adevidx %d (>=num_adevs %d)\n", stream_cfg->adevidx, cfg->num_adevs);
			return -EINVAL;
		}
		adev_cfg = &cfg->adevs[stream_cfg->adevidx];

		if (adev_cfg->dir == SND_PCM_STREAM_CAPTURE)
			side = &rt->in;
		else
			side = &rt->out;

		if (adev_cfg->pcmname == NULL) {
			RTS_ERR(rt, "adev %d has no name\n", stream_cfg->adevidx);
			return -EINVAL;
		}
		RTS_LOG(rt, RTS_LOGL_TRACE_INSTANCE,"-----RTS CREATE STREAM %d on dev \"%s\" channel %d\n", i, adev_cfg->pcmname, stream_cfg->channel);

		/*create adev or get existing*/	
		ad = rts_get_adev(side->a, adev_cfg->pcmname);
		if (ad == NULL) {
			/*ALSA device */
			ad = &side->a[side->n_a];
			ad->name = strdup(adev_cfg->pcmname);
			ad->rate = adev_cfg->rate;
			ad->ideal_period_size = adev_cfg->period_frames;
			ad->ideal_period_time = (ad->ideal_period_size * 1000) / ad->rate;
			ad->dir = adev_cfg->dir;
			ad->order = rt->in.n_a + rt->out.n_a;
			ad->format = adev_cfg->format;
            if (rt->extra_tout_ms == RTS_TOUT_ENDLESS) {
                ad->init_wait = RTS_TOUT_ENDLESS;
                ad->stream_wait = RTS_TOUT_ENDLESS;
            } else {
			    ad->stream_wait = ad->ideal_period_time + RTS_WAIT_ADDON_MS + rt->extra_tout_ms;
			    if (adev_cfg->startup_tout == RTS_TOUT_ENDLESS)
				    ad->init_wait = RTS_TOUT_ENDLESS;
			    else if (adev_cfg->startup_tout == 0)
				    ad->init_wait = ad->stream_wait;
			    else
				    ad->init_wait = adev_cfg->startup_tout + rt->extra_tout_ms;
            }
			ad->areas = NULL;
			ad->silence_prefill = min((rt->prefill_ms * ad->rate) / 1000, ad->ideal_period_size * 8);
			RTS_LOG(rt, RTS_LOGL_TRACE_INSTANCE,"RTS NEW ADEV \"%s\" prefill %ld init tout %d stream_tout %d\n", ad->name, ad->silence_prefill, ad->init_wait, ad->stream_wait);
			side->n_a++;
		}

		/*rts stream -> always new one*/
		s = &side->s[side->n_s++];
		s->channel = stream_cfg->channel;
		s->area.addr = NULL; 	/*set on access*/
		s->area.first = 0; 	/*1channel-> no offset*/
		s->area.step = snd_pcm_format_width(ad->format); /*used format width*/

		/*link forward/backward*/
		s->adev = ad;
		ad->s[ad->n_streams++] = s;
		if (ad->bf_channels & (1lu << s->channel)) {
			RTS_ERR(rt, "channel %d on adev %s already in use\n", s->channel, ad->name);
			return -EINVAL;
		}
		ad->bf_channels |= (1lu << s->channel);

		if (s->channel >= ad->channels)
			ad->channels = s->channel+1;
	}

	return 0;
}


/**
 * \brief Destroy audio device instance
 * \param ad device to destroy
 * \return 0: OK
 *
 */
static int destroy_adev(struct rts_adev *ad)
{
	if (ad->silence_area.addr != NULL) {
		free(ad->silence_area.addr);
		ad->silence_area.addr = NULL;
	}
	if (ad->name != NULL) {
		free(ad->name);
		ad->name = NULL;
	}
	if (ad->areas != NULL) {
		if (ad->areas[0].addr != NULL) {
			free(ad->areas[0].addr);
			ad->areas[0].addr = NULL;
		}
		free(ad->areas);
		ad->areas = NULL;
	}
	return 0;
}


/**
 * \brief Destroy runtime
 * \param rt handle to runtime
 * \return 0: OK
 *
 */
static int rts_detroy_rt(struct rts_runtime *rt)
{
	unsigned int i;
	struct rts_side *side;

	RTS_LOG(rt, RTS_LOGL_TRACE_INSTANCE,"RTS DESTROY\n");
	(void)rts_close_all_adevs(rt);

	side = &rt->in;
	for (i = 0; i < side->n_s; i++) {
		/*nothing to do yet*/
	}

	for (i = 0; i < side->n_a; i++) {
		destroy_adev(&side->a[i]);
	}

	side = &rt->out;
	for (i = 0; i < side->n_s; i++) {
		/*nothing to do yet*/
	}

	for (i = 0; i < side->n_a; i++) {
		destroy_adev(&side->a[i]);
	}

	if (rt->log != NULL) {
		snd_output_close(rt->log);
		rt->log = NULL;
	}
	if (rt->status != NULL) {
		snd_pcm_status_free(rt->status);
		rt->status = NULL;
	}
	rt->magic = RTS_MAGIC_INVAL;
	rt->tid = 0;

	free(rt);
	return 0;
}	


/**
 * \brief Configure all ALSA devices
 * \param rt handle to runtime
 * \return 0: OK
 *
 */
static int rts_configure_all_adevs(struct rts_runtime *rt)
{
	unsigned int in = 0;
	unsigned int out = 0;
	unsigned int i; 
	int err = 0;
	struct rts_side *side_in = &rt->in;
	struct rts_side *side_out = &rt->out;
	struct rts_adev *ad;

	/*force reload of alsa configuration*/
	if (RTS_FEATURE_ENABLED(rt, RTS_FEAT_FORCE_CFG_RELOAD))
		snd_config_update_free_global();

	for (i = 0; i < side_in->n_a+side_out->n_a; i++) {
		if ((in < side_in->n_a ) && (side_in->a[in].order == i))
			ad = &side_in->a[in++];
		else
			ad = &side_out->a[out++];
		err = rts_configure_adev(rt, ad);
		if (err != 0)
			break;
		err = rts_configure_adev_area(rt, ad);
		if (err != 0)
			break;
	}
	return err;
}


/*----------------------------------------------------------------------------------*/
/**
 * \brief abort RTS activities. This shall unblock devices
 *  and make a pending call returning as fast as possible
 * \param h handle to RTS
 * \return 0: OK
 *
 */
int rts_abort(tRTS h)
{
	struct rts_runtime *rt = h;
	RTS_CHK_HNDL(h);
	rt->aborted = 1;
	return rts_abort_all_adevs(rt);
}

/**
 * \brief destroy RTS instance
 * \param h handle to RTS
 * \return 0: OK
 *
 */
int rts_destroy(tRTS h)
{
	struct rts_runtime *rt = h;
	RTS_ENTRY(h);

	/*rt must not be accessed after destroy*/
	(void)rts_detroy_rt(rt);
	return 0;
}


/**
 * \brief Create RTS instance
 * \param h pointer to handle of RTS
 * \param cfg user provided configuration
 * \return 0: OK, otherwise negative error code
 *
 */
int rts_create_versioned(tRTS *h, trts_cfg *cfg, unsigned int version)
{
	struct rts_runtime *rt;
	int err;

	if (h == NULL) {
		RTS_ERR_EARLY("no handle given\n");
		return -EINVAL;
	}
	*h = RTS_HANDLE_INVAL;

	if (cfg == NULL) {
		RTS_ERR_EARLY("no config given\n");
		return -EINVAL;
	}

	/*check for SAME version - in future we could also extend to backward compatibility*/
	if (version != RTS_API_VERSION) {
		RTS_ERR_EARLY("version mismatch: Lib V%d.%d.%d - given V%d.%d.%d!\n",
			RTS_API_VERSION_MAJOR, RTS_API_VERSION_MINOR, RTS_API_VERSION_TINY,
			(version>>16)&0xff, (version>>8)&0xff, (version)&0xff);
		return -EINVAL;
	}

	if (cfg->num_streams > RTS_MAX_STREAMS) {
		RTS_ERR_EARLY("too many streams (%d exceed max %d)\n", cfg->num_streams, RTS_MAX_STREAMS);
		return -EINVAL;
	}

	if (cfg->num_streams == 0) {
		RTS_ERR_EARLY("No streams given\n");
		return -EINVAL;
	}

	rt = calloc(1, sizeof(struct rts_runtime));
	if (rt == NULL) {
		RTS_ERR_EARLY("no memory\n");
		return -ENOMEM;
	}
	rt->magic = RTS_MAGIC_OK;
	rt->tid = pthread_self();

	rt->loglevel = RTS_LOGL_DEFAULT;
	err = snd_output_stdio_attach(&rt->log, stdout, 0);
	if (err < 0) {
		RTS_ERR_EARLY("attach log output failed with err %d (%s)\n", err, snd_strerror(err));
		rts_detroy_rt(rt);
		return err;
	}

	err = rts_parse_config(cfg, rt);
	if (err < 0) {
		RTS_ERR(rt, "create failed with err %d (%s)\n", err, snd_strerror(err));
		rts_detroy_rt(rt);
		return err;
	}

	err = rts_configure_all_adevs(rt);
	if (err < 0) {
		RTS_ERR(rt, "configure failed with err %d (%s)\n", err, snd_strerror(err));
		rts_detroy_rt(rt);
		return err;
	}
	*h = rt;
	RTS_LOG(rt, RTS_LOGL_TRACE_INSTANCE,"RTS CREATED\n");

	return 0;
}


/**
 * \brief Read xrun statistic
 * \param h handle of RTS
 * \param stat statistic to fill
 * \return 0: OK
 *
 */
int rts_statistic(tRTS h, trts_stat *stat)
{
	struct rts_runtime *rt = h;
	RTS_ENTRY(h);
	if (stat == NULL)
		return -EINVAL;

	stat->num_xruns = rt->num_xruns;
	if (stat->clear != 0)
		rt->num_xruns = 0;	

	return 0;
}


/**
 * \brief Write data to all output streams
 * \param h handle of RTS
 * \param buffers buffers to write 
 * \return 0: OK
 *
 */
int rts_write(tRTS h, void *buffers[])
{
	struct rts_runtime *rt = h;
	int err;
	RTS_ENTRY(h);
	err = rts_rw(&rt->out, buffers);
	return err;
}


/**
 * \brief Read data from all input streams
 * \param h handle of RTS
 * \param buffers buffers to put read data 
 * \return 0: OK
 *
 */
int rts_read(tRTS h, void *buffers[])
{
	struct rts_runtime *rt = h;
	int err;
	RTS_ENTRY(h);
	err = rts_rw(&rt->in, buffers);
	return err;
}

/**
 * \brief Try recover from any error given by rts_read/rts_write.
 * \param h handle of RTS
 * \return 0: OK
 *
 */
int rts_recover(tRTS h)
{
	struct rts_runtime *rt = h;
	RTS_ENTRY(h);

	if (rt->aborted != 0)
		return -ECANCELED;

	if (!rt->broken) {
		RTS_LOG(rt, RTS_LOGL_SYS_ERR,"ILLEGAL RECOVERY TRY\n");
		return -EINVAL;
    }

	if ((rt->broken != -EPIPE) && (rt->broken != -ENODATA)) {
        RTS_LOG(rt, RTS_LOGL_STREAM_ERR,"NOT POSSIBLE TO RECOVER FROM ERR %d (%s)\n", rt->broken, snd_strerror(rt->broken));
		return rt->broken;
    }

    RTS_LOG(rt, RTS_LOGL_STREAM_ERR,"RECOVER FROM ERR %d (%s)\n", rt->broken, snd_strerror(rt->broken));
	rt->broken = 0;
	/*stop was already executed on entering error state*/
	return rts_prepare_all_adevs(rt);
}


