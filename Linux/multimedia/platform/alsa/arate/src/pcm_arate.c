/*
 *  Asynchronous rate plugin for ALSA
 *
 *  Copyright (c) 2013 by ADIT GmbH
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
#include <stdint.h>
#include <semaphore.h>
#include <sys/poll.h>
#include <unistd.h>
#include <pthread.h>
#include "pcm_local.h"
#include "pcm.h"
#include "pcm_arate.h"


#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define QA_USE_VAR(v) ((v)=(v))


typedef struct _snd_pcm_arate snd_pcm_arate_t;

/*mode for silence handling of forwarding thread*/
enum arate_silence_mode {
	SILENCE_MODE_OFF = 0,		/*normal behavior: start after START_THRESHOLD_PERIODS periods*/
	SILENCE_MODE_CONTINOUS = 1,	/*immediately start filling slave with silence until asrc data available*/
	SILENCE_MODE_DEFINED_PREFILL = 2,/*use defined silence prefill and start with first available period*/
};

struct arate_slave_params {
	snd_pcm_format_t format;
	int rate;
	int period_time;
	int buffer_time;
	int prefill_ms;
	snd_pcm_sframes_t period_size;
	snd_pcm_sframes_t buffer_size;
};

struct asrc_pcm {
	snd_pcm_uframes_t hw_ptr;
	snd_pcm_uframes_t appl_ptr;
	snd_pcm_uframes_t avail_max;
	snd_pcm_state_t state;
	snd_htimestamp_t trigger_tstamp;
	snd_pcm_t *asrc_pcm;
	struct _snd_pcm_arate *arate;
	int poll_fd_err;
	asrc_io_cfg_t asrc_io_cfg;
	asrc_sync_t asrc_sync;
	asrc_slave_sync_t asrc_slave_sync;
	asrc_capabilities_t asrc_caps;
	void *asrc_obj;
	asrc_ops_t asrc_ops;
};


struct _snd_pcm_arate {
	pthread_mutex_t arate_slave_mutex;
	pthread_cond_t arate_slave_cond;
	volatile int run;
	volatile int exit;
	pthread_t forward_thread;

	int forward_timeout;
	int draining;
	int aborting;
	snd_pcm_t *slave_pcm; /*slave PCM*/

	sem_t thread_sync;
	snd_pcm_hw_params_t *s_hw_params; //needed ???
	struct arate_slave_params sparams;
	enum arate_silence_mode silence_mode;
	int silence_prolog;
	int core_locked;
	snd_pcm_uframes_t silence_size;
	snd_pcm_uframes_t silence_remain;
	struct asrc_pcm i;
	struct asrc_pcm o;

	struct asrc_pcm *asrc_user; /*linked to in/out depending on direction*/
	struct asrc_pcm *asrc_slave;/*linked to in/out depending on direction*/

	snd_output_t *log;
};

static pthread_once_t once = PTHREAD_ONCE_INIT;

static void mask2snd_mask(uint64_t mask_in, snd_mask_t *mask_out);


/*1.0.29 introduced tstamp_type*/
#if ((SND_LIB_MAJOR <= 1) && (SND_LIB_MINOR == 0) && (SND_LIB_SUBMINOR < 29))
#define get_tstamp_type(pcm) (pcm)->monotonic
#else
#define get_tstamp_type(pcm) (pcm)->tstamp_type
#endif

static int is_monotonic(snd_pcm_t *pcm)
{
#if ((SND_LIB_MAJOR <= 1) && (SND_LIB_MINOR == 0) && (SND_LIB_SUBMINOR < 29))
	return pcm->monotonic;
#else
	return pcm->tstamp_type == SND_PCM_TSTAMP_TYPE_MONOTONIC;
#endif
}

static void set_monotonic(snd_pcm_t *pcm)
{
#if ((SND_LIB_MAJOR <= 1) && (SND_LIB_MINOR == 0) && (SND_LIB_SUBMINOR < 29))
	pcm->monotonic = 1;
#else
	pcm->tstamp_type = SND_PCM_TSTAMP_TYPE_MONOTONIC;
#endif
}


#define alsa_state(a) (a->state)

#define pcm2arate(pcm) (((struct asrc_pcm *)(pcm->private_data))->arate)
#define pcm2apcm(pcm) ((struct asrc_pcm *)(pcm->private_data))

#define user_pcm(pcm) ((pcm2apcm(pcm)) == pcm2arate(pcm)->asrc_user)

//#define DEBUG
//#define ARATE_REFINE_DEBUG
#ifdef DEBUG
#define pcm2log(p)  (pcm2arate((p))->log)
#endif

#ifdef DEBUG
#define TRACE_PCM(p) snd_output_printf(pcm2log(p), "%s: %s\n",__FUNCTION__, user_pcm(p)?"PCM_USER":"PCM_SLAVE")
#define DBG_PCM(p, a...) snd_output_printf(pcm2log(p), a);
#define DBG_ARATE(ar, a...) snd_output_printf((ar)->log, a);
#else
#define TRACE_PCM(p)
#define DBG_PCM(p, a...)
#define DBG_ARATE(ar, a...)
#endif

#define DEFAULT_TIMEOUT 1000
#define START_THRESHOLD_PERIODS 2

#define wait_for(a, cond)								\
	do{										\
		int lock_err = pthread_mutex_lock(&a->arate_slave_mutex);		\
		if(lock_err)								\
			SNDERR("LOCK FAILED %d\n", lock_err);				\
		while(!(cond))								\
			pthread_cond_wait(&a->arate_slave_cond, &a->arate_slave_mutex);	\
		pthread_mutex_unlock(&arate->arate_slave_mutex);			\
	}while(0)

static inline void wakeup_waiter (snd_pcm_arate_t *a, volatile int* var, int val)
{
	int err;
	err = pthread_mutex_lock(&a->arate_slave_mutex);
	if(err)
		SNDERR("LOCK FAILED %d\n", err);

	*var = val;
	pthread_cond_signal(&a->arate_slave_cond);

	pthread_mutex_unlock(&a->arate_slave_mutex);
}

static int hw_param_interval_refine_minmax(snd_pcm_hw_params_t *params, snd_pcm_hw_param_t var,
						unsigned int imin, unsigned int imax)
{
	snd_interval_setinteger(&params->intervals[var - SND_PCM_HW_PARAM_FIRST_INTERVAL]);
	_snd_pcm_hw_param_set_minmax(params, var, imin, 0,imax, 0);
	return 0;
}

#define hw_param_mask_refine(params, var, mask) _snd_pcm_hw_param_set_mask(params, var, mask)

#define asrc_need_slaveclk(pcm) (pcm2apcm(pcm)->asrc_caps.features & ASRC_FEAT_NEED_SLAVECLK)

static int asrc_formats(snd_pcm_t *pcm, snd_mask_t *mask_out)
{
	/*merge of ASRC input/ASRC output/slave*/
	snd_pcm_format_mask_t mask_slave;
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	mask2snd_mask(arate->i.asrc_caps.formats & arate->o.asrc_caps.formats, mask_out);
	snd_pcm_hw_params_get_format_mask(arate->s_hw_params, &mask_slave);
	return snd_mask_refine(mask_out, &mask_slave);
}
#define asrc_rate_min(pcm) (pcm2apcm(pcm)->asrc_caps.rate_min)
#define asrc_rate_max(pcm) (pcm2apcm(pcm)->asrc_caps.rate_max)
static unsigned int asrc_chan_min(snd_pcm_t *pcm)
{
	unsigned int ch;
	snd_pcm_arate_t *arate =  pcm2arate(pcm);
	INTERNAL(snd_pcm_hw_params_get_channels_min)(arate->s_hw_params, &ch);
	return max((unsigned int)pcm2apcm(pcm)->asrc_caps.channels_min, ch );
}

static unsigned int asrc_chan_max(snd_pcm_t *pcm)
{
	unsigned int ch;
	snd_pcm_arate_t *arate =  pcm2arate(pcm);
	INTERNAL(snd_pcm_hw_params_get_channels_max)(arate->s_hw_params, &ch);
	return min((unsigned int)pcm2apcm(pcm)->asrc_caps.channels_max, ch );
}

#define asrc_periods_min(pcm) (pcm2apcm(pcm)->asrc_caps.periods_min)
#define asrc_periods_max(pcm) (pcm2apcm(pcm)->asrc_caps.periods_max)

#define asrc_period_bytes_min(pcm) (unsigned int)(pcm2apcm(pcm)->asrc_caps.periodbytes_min)
#define asrc_period_bytes_max(pcm) (unsigned int)(pcm2apcm(pcm)->asrc_caps.periodbytes_max)
#define asrc_period_time_min(pcm) (unsigned int)(pcm2apcm(pcm)->asrc_caps.periodtime_min)
#define asrc_period_time_max(pcm) (unsigned int)(pcm2apcm(pcm)->asrc_caps.periodtime_max)
#define asrc_buffer_time_min(pcm) (unsigned int)(pcm2apcm(pcm)->asrc_caps.buffertime_min)
#define asrc_buffer_time_max(pcm) (unsigned int)(pcm2apcm(pcm)->asrc_caps.buffertime_max)
#define asrc_buffer_bytes_min(pcm) (unsigned int)(pcm2apcm(pcm)->asrc_caps.periods_min * pcm2apcm(pcm)->asrc_caps.periodbytes_min)
#define asrc_buffer_bytes_max(pcm) (unsigned int)(pcm2apcm(pcm)->asrc_caps.bufbytes_max)


static int sync_sw(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
	int ret;
	struct asrc_pcm *apcm = pcm2apcm(pcm);
	asrc_sync_t *sync_io = &apcm->asrc_sync;

	/*static part - not changed in sync_hw*/
	sync_io->avail_min = params->avail_min;
	sync_io->boundary = params->boundary;
	sync_io->stop_threshold = params->stop_threshold;
	sync_io->start_threshold = params->start_threshold;

	/*update dynamic part*/
	sync_io->app_pos = *pcm->appl.ptr;
	sync_io->state = alsa_state(apcm);

	ret = apcm->asrc_ops.sync(apcm->asrc_obj, sync_io);
	if (ret)
		return ret;

	*pcm->hw.ptr = sync_io->hw_pos;
	params->start_threshold = sync_io->start_threshold;
	params->stop_threshold = sync_io->stop_threshold;

	DBG_PCM(pcm,"%s SYNC SW: HW %ld APP %ld -> avail %ld\n",
			user_pcm(pcm)?"USER":"SLAVE",*pcm->hw.ptr, *pcm->appl.ptr, snd_pcm_mmap_avail(pcm));
	return ret;
}


static int sync_hw(snd_pcm_t *pcm)
{
	int ret;
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	snd_pcm_arate_t *arate =  pcm2arate(pcm);
	asrc_sync_t *sync_io = &apcm->asrc_sync;

	/*update dynamic part*/
	sync_io->app_pos = *pcm->appl.ptr;
	sync_io->state = alsa_state(apcm);
	ret = apcm->asrc_ops.sync(apcm->asrc_obj, sync_io);

	*pcm->hw.ptr = sync_io->hw_pos;

	switch(sync_io->state) {
		case SND_PCM_STATE_XRUN: 
			DBG_PCM(pcm,"%s SYNC: ENTER XRUN  HW %ld APP %ld -> avail %ld\n",
					user_pcm(pcm)?"USER":"SLAVE",*pcm->hw.ptr, *pcm->appl.ptr, snd_pcm_mmap_avail(pcm));
			alsa_state(apcm) = SND_PCM_STATE_XRUN;
			/*ASRC does no automatic stop --> do here manually*/
			apcm->asrc_ops.stop(apcm->asrc_obj); /*clear driver xrun state*/

			if (user_pcm(pcm))
				wait_for(arate, !arate->run);
			break;
		default:
			DBG_PCM(pcm,"%s SYNC: HW %ld APP %ld -> avail %ld\n",
					user_pcm(pcm)?"USER":"SLAVE",*pcm->hw.ptr, *pcm->appl.ptr, snd_pcm_mmap_avail(pcm));
			break;
	}

	return ret;
}


static void mask2snd_mask(uint64_t mask_in, snd_mask_t *mask_out)
{
	mask_out->bits[0] = (uint32_t)mask_in;
	mask_out->bits[1] = (uint32_t)(mask_in >> 32);
}


static int snd_pcm_arate_hw_refine(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	int again;
	int err;
	TRACE_PCM(pcm);

	snd_mask_t mask;

#ifdef ARATE_REFINE_DEBUG
	DBG_PCM(pcm,"---------DFORWARD REFINE ENTRY:\n");
	snd_pcm_hw_params_dump(params, pcm2log(pcm));
#endif
	/*info: init once, ~0 is default*/
	if (params->info == ~0U) {
		params->info = 0;
		params->info |= (is_monotonic(pcm) ? SND_PCM_INFO_MONOTONIC : 0);
	}

	params->fifo_size = pcm2apcm(pcm)->asrc_caps.fifo_size;

	/*MASKS*/
	snd_mask_none(&mask);
	snd_mask_set(&mask, SNDRV_PCM_ACCESS_RW_INTERLEAVED);
	snd_mask_set(&mask, SNDRV_PCM_ACCESS_MMAP_INTERLEAVED);
	err = hw_param_mask_refine(params,SND_PCM_HW_PARAM_ACCESS,&mask);

	snd_mask_none(&mask);
	asrc_formats(pcm, &mask);
	err = hw_param_mask_refine(params,SND_PCM_HW_PARAM_FORMAT,&mask);
	if (err < 0) {
		DBG_PCM(pcm,"refine format failed\n");
		return err;
	}

	/*INTERVALS*/
	err = hw_param_interval_refine_minmax(params,
			SND_PCM_HW_PARAM_CHANNELS, asrc_chan_min(pcm), asrc_chan_max(pcm));
	if (err < 0)
		return err;
	err = hw_param_interval_refine_minmax(params,
			SND_PCM_HW_PARAM_RATE, asrc_rate_min(pcm), asrc_rate_max(pcm));
	if (err < 0)
		return err;

	/*should be default - but seems forgotten in pcm_params.c::refine_intervals */
	snd_interval_setinteger(&params->intervals[SND_PCM_HW_PARAM_PERIOD_SIZE - SND_PCM_HW_PARAM_FIRST_INTERVAL]);
#ifdef ARATE_REFINE_DEBUG
	DBG_PCM(pcm,"DFORWARD REFINE MID:\n");
	snd_pcm_hw_params_dump(params, pcm2log(pcm));
#endif
 
	do {
		err = hw_param_interval_refine_minmax(params,
				SND_PCM_HW_PARAM_PERIODS, asrc_periods_min(pcm), asrc_periods_max(pcm));
		if (err < 0)
			return err;
		err = hw_param_interval_refine_minmax(params,
				SND_PCM_HW_PARAM_PERIOD_BYTES, asrc_period_bytes_min(pcm), asrc_period_bytes_max(pcm));
		if (err < 0)
			return err;
		err = _snd_pcm_hw_param_set_minmax(params, SND_PCM_HW_PARAM_PERIOD_TIME, asrc_period_time_min(pcm), 0,asrc_period_time_max(pcm), 0);
		if (err < 0)
			return err;
		err = _snd_pcm_hw_param_set_minmax(params, SND_PCM_HW_PARAM_BUFFER_TIME, asrc_buffer_time_min(pcm), 0,asrc_buffer_time_max(pcm), 0);
		if (err < 0)
			return err;
 		err = hw_param_interval_refine_minmax(params,
				SND_PCM_HW_PARAM_BUFFER_BYTES, asrc_buffer_bytes_min(pcm), asrc_buffer_bytes_max(pcm));
 		if (err < 0)
 			return err;
		err = snd_pcm_hw_refine_soft(pcm, params);

		if (err < 0) {
			DBG_PCM(pcm,"refine_soft failed\n");
			return err;
		}
		again = err;

	} while (again);

#ifdef ARATE_REFINE_DEBUG
	DBG_PCM(pcm,"---------DFORWARD REFINE EXIT: err%d\n",err);
	snd_pcm_hw_params_dump(params, pcm2log(pcm));
#endif

	return err;
}


#define arate_return_on_err(e, s) \
	do{\
		if (e != 0) {\
			SNDERR("%s failed with err %d", s, e);\
			return e;\
		}\
	}while(0)

static int snd_pcm_arate_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	int err;
	unsigned long val_l;
	unsigned int val_i;
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	struct asrc_pcm * apcm = pcm2apcm(pcm);

	TRACE_PCM(pcm);

	if (user_pcm(pcm)) {
		/*provide user params to slave side*/
		snd_pcm_hw_params_t *hw_params;
		snd_pcm_hw_params_alloca(&hw_params);
		snd_pcm_hw_params_any(arate->asrc_slave->asrc_pcm, hw_params);
		err = snd_pcm_hw_params_set_rate(arate->asrc_slave->asrc_pcm, hw_params, arate->sparams.rate, 0);
		arate_return_on_err(err, "set rate failed");

		err = snd_pcm_hw_params_set_rate(arate->slave_pcm, arate->s_hw_params, arate->sparams.rate, 0);
		arate_return_on_err(err, "set rate failed");

		err = INTERNAL(snd_pcm_hw_params_get_access)(params, (snd_pcm_access_t*)&val_i);
		arate_return_on_err(err, "get access failed");

		err = snd_pcm_hw_params_set_access(arate->asrc_slave->asrc_pcm, hw_params, (snd_pcm_access_t)val_i);
		arate_return_on_err(err, "set access failed");

		val_i = SND_PCM_ACCESS_RW_INTERLEAVED;
		err = snd_pcm_hw_params_set_access (arate->slave_pcm, arate->s_hw_params, (snd_pcm_access_t)val_i);
		arate_return_on_err(err, "set access failed");

		err = INTERNAL(snd_pcm_hw_params_get_format)(params, (snd_pcm_format_t*)&val_i);
		arate_return_on_err(err, "get format failed");

		err = snd_pcm_hw_params_set_format(arate->asrc_slave->asrc_pcm, hw_params, (snd_pcm_format_t)val_i);
		arate_return_on_err(err, "set format failed");

		err = snd_pcm_hw_params_set_format(arate->slave_pcm, arate->s_hw_params, (snd_pcm_format_t)val_i);
		arate_return_on_err(err, "set format failed");

		err = INTERNAL(snd_pcm_hw_params_get_channels)(params, &val_i);
		arate_return_on_err(err, "get channels failed");

		err = snd_pcm_hw_params_set_channels(arate->asrc_slave->asrc_pcm, hw_params, val_i);
		arate_return_on_err(err, "set channels failed");

		err = snd_pcm_hw_params_set_channels(arate->slave_pcm, arate->s_hw_params, val_i);
		arate_return_on_err(err, "set channels failed");

		/*order: 1.period size, 2.period time 3. user period time*/
		if (arate->sparams.period_size != -1) {
			val_l = arate->sparams.period_size;
			err = INTERNAL(snd_pcm_hw_params_set_period_size_near(arate->slave_pcm, arate->s_hw_params, &val_l, 0));
			arate_return_on_err(err, "set period size failed");
		} else {
			if (arate->sparams.period_time != -1) {
				val_i = arate->sparams.period_time;
			} else {
				/*fallback: configure slave side with similar buffer and period TIME*/
				err = INTERNAL(snd_pcm_hw_params_get_period_time)(params, &val_i, NULL) ;
				arate_return_on_err(err, "get period time failed");
			}
			err = INTERNAL(snd_pcm_hw_params_set_period_time_near(arate->slave_pcm, arate->s_hw_params, &val_i, 0));
			arate_return_on_err(err, "set period time failed");
		}

		/*order: 1.buffer size, 2.buffer time 3. user buffer time*/
		if (arate->sparams.buffer_size != -1) {
			val_l = arate->sparams.buffer_size;
			err = INTERNAL(snd_pcm_hw_params_set_buffer_size_near(arate->slave_pcm, arate->s_hw_params, &val_l));
			arate_return_on_err(err, "set buffer size failed");
		} else {
			if (arate->sparams.buffer_time != -1) {
				val_i = arate->sparams.buffer_time;
			} else {
				/*fallback: configure slave side with similar buffer and period TIME*/
				err = INTERNAL(snd_pcm_hw_params_get_buffer_time)(params, &val_i, NULL) ;
				arate_return_on_err(err, "get buffer time failed");
			}
			err = INTERNAL(snd_pcm_hw_params_set_buffer_time_near(arate->slave_pcm, arate->s_hw_params, &val_i, 0));
			arate_return_on_err(err, "set buffer time failed");
		}

		/*get resulting size*/
		err = INTERNAL(snd_pcm_hw_params_get_period_size)(arate->s_hw_params, &val_l, NULL);
		arate_return_on_err(err, "get period size failed");
		err = INTERNAL(snd_pcm_hw_params_set_period_size_near)(arate->asrc_slave->asrc_pcm, hw_params, &val_l, NULL);
		arate_return_on_err(err, "set period size failed");

		INTERNAL(snd_pcm_hw_params_get_buffer_size)(arate->s_hw_params, &val_l);
		arate_return_on_err(err, "get buffer size failed");
		err = INTERNAL(snd_pcm_hw_params_set_buffer_size_near)(arate->asrc_slave->asrc_pcm, hw_params, &val_l);
		arate_return_on_err(err, "set buffer size failed");

		err = snd_pcm_hw_params(arate->asrc_slave->asrc_pcm, hw_params);
		arate_return_on_err(err, "hw_params failed");
	} else {
		err = snd_pcm_hw_params(arate->slave_pcm, arate->s_hw_params);
		arate_return_on_err(err, "hw_params failed");

		if (apcm->asrc_ops.slave_config) {
			asrc_slave_cfg_t slv_cfg = {0};
			if (snd_pcm_hw_params_is_monotonic(arate->s_hw_params))
				slv_cfg.flags |= SLV_CFG_TSTAMP_MONOTONIC;
			err = INTERNAL(snd_pcm_hw_params_get_buffer_size)(arate->s_hw_params, (snd_pcm_uframes_t*)&slv_cfg.buffer_size);
			arate_return_on_err(err, "get SLAVE buffer size failed");
			err = INTERNAL(snd_pcm_hw_params_get_period_size)(arate->s_hw_params, (snd_pcm_uframes_t*)&slv_cfg.period_size, NULL);
			arate_return_on_err(err, "get SLAVE periods failed");
			err = apcm->asrc_ops.slave_config(apcm->asrc_obj, &slv_cfg);
			arate_return_on_err(err, "provide slave config failed");
		}

		/*ALSA config is fixed now, configure silence mode once..*/
		/*snd_pcm_hw_params() on slave must have been called before accessing settings via pcm->period_size !!*/
		arate->silence_mode = SILENCE_MODE_OFF;
		if ((arate->slave_pcm->stream == SND_PCM_STREAM_PLAYBACK) && asrc_need_slaveclk(pcm) ) {
			arate->silence_mode = SILENCE_MODE_CONTINOUS;/*silence until ASRC output availabe*/
			arate->silence_size = arate->slave_pcm->period_size;
		} else {
			if ((arate->sparams.prefill_ms > 0 ) && (arate->slave_pcm->stream == SND_PCM_STREAM_PLAYBACK)) {/*ignore 0*/
				arate->silence_mode = SILENCE_MODE_DEFINED_PREFILL;/*defined prefill once at startup*/
				snd_pcm_t *output = (arate->slave_pcm->stream == SND_PCM_STREAM_PLAYBACK)?arate->slave_pcm : arate->asrc_slave->asrc_pcm;
				arate->silence_size = (arate->sparams.prefill_ms * output->rate) / 1000;
			}
		}
	}
#ifdef DEBUG
	DBG_PCM(pcm,"HWPARAMS: PLUG ARATE\n");
	snd_pcm_hw_params_dump(params, arate->log);
#endif

	err = INTERNAL(snd_pcm_hw_params_get_rate)(params, (unsigned int*)&apcm->asrc_io_cfg.sample_rate, NULL);
	arate_return_on_err(err, "get rate failed");

	err = INTERNAL(snd_pcm_hw_params_get_format)(params, (snd_pcm_format_t*)&apcm->asrc_io_cfg.format);
	arate_return_on_err(err, "get format failed");

	err = INTERNAL(snd_pcm_hw_params_get_channels)(params, (unsigned int*)&apcm->asrc_io_cfg.channels);
	arate_return_on_err(err, "get channels failed");

	err = INTERNAL(snd_pcm_hw_params_get_buffer_size)(params, &val_l);
	arate_return_on_err(err, "get buffer size failed");
	apcm->asrc_io_cfg.buf_size = snd_pcm_format_size(apcm->asrc_io_cfg.format, val_l * apcm->asrc_io_cfg.channels);

	err = INTERNAL(snd_pcm_hw_params_get_periods)(params, (unsigned int*)&apcm->asrc_io_cfg.periods, NULL);
	arate_return_on_err(err, "get periods failed");

	err = apcm->asrc_ops.configure(apcm->asrc_obj, &apcm->asrc_io_cfg);
	arate_return_on_err(err, "asrc configure failed");

	alsa_state(apcm) = SND_PCM_STATE_SETUP;

	return err;
}


static int snd_pcm_arate_poll_descriptors_count(snd_pcm_t *pcm)
{
	struct asrc_pcm *apcm = pcm2apcm(pcm);
	TRACE_PCM(pcm);
	if (apcm->poll_fd_err >= 0)
		return 2;
	else
		return 1; 
}


/**
 * poll descriptors; optional
 */
static int snd_pcm_arate_poll_descriptors(snd_pcm_t *pcm, struct pollfd *pfd, unsigned int space)
{
	struct asrc_pcm *apcm = pcm2apcm(pcm);
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	TRACE_PCM(pcm);

	if (arate->aborting) {
		SNDERR("already aborted");
		return -EIO; 
	}
	pfd[0].fd = pcm->poll_fd;
	pfd[0].events = pcm->poll_events | POLLERR | POLLNVAL;
	pfd[0].revents = 0;

	if (space > 1) {
		pfd[1].fd = apcm->poll_fd_err;
		pfd[1].events = POLLIN | POLLERR | POLLNVAL;
		pfd[1].revents = 0;
	}
	return space;
}


/**
 * mangle poll events; optional
 */
static int snd_pcm_arate_poll_revents(snd_pcm_t *pcm, struct pollfd *pfd, unsigned int nfds, unsigned short *revents)
{
	TRACE_PCM(pcm);
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	int ret;
	unsigned int events;

	if (nfds >= 1) {
		if(pfd[0].fd != pcm->poll_fd)
			return -EINVAL;
	}
	events = pfd[0].revents;

	if (nfds > 1) {
		if(pfd[1].fd != apcm->poll_fd_err)
			return -EINVAL;

		if (pfd[1].revents & POLLIN)
			events |= pfd[1].revents;
	}

	ret = sync_hw(pcm);
	if (ret)
		return ret;

	switch(alsa_state(apcm)) {
		case SND_PCM_STATE_XRUN:
			if (user_pcm(pcm)) {
				gettimestamp(&apcm->trigger_tstamp, get_tstamp_type(pcm));
				DBG_PCM(pcm,"REVENT...XRUN USER\n");	
				events |= POLLERR;
			} else {
				DBG_PCM(pcm,"REVENT...XRUN SLAVE\n");
				events |= POLLERR;
			}
			break;
		case SND_PCM_STATE_RUNNING:
		case SND_PCM_STATE_PREPARED:
		case SND_PCM_STATE_PAUSED:
		case SND_PCM_STATE_DRAINING:
			break;
		default:
			events |= POLLERR;
			break;
	}

	*revents = events;
	return 0;
}


static int snd_pcm_arate_nonblock(snd_pcm_t *pcm, int nonblock)
{
	struct asrc_pcm *apcm = pcm2apcm(pcm);
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	TRACE_PCM(pcm);

	/*
	nonblock = 2 is special case 'abort' (unfortunately hardcoded)
	prevent further blocking calls if in aborting.
	*/
	if (nonblock == 2)
		arate->aborting = 1;

	if (apcm->asrc_ops.set_pollfd_blockmode)
		return apcm->asrc_ops.set_pollfd_blockmode(apcm->asrc_obj, nonblock);
	return 0;
}


static int snd_pcm_arate_hw_free(snd_pcm_t *pcm)
{
	TRACE_PCM(pcm);
#ifndef DEBUG
	pcm = pcm;
#endif
	return 0;
}


static int snd_pcm_arate_info(snd_pcm_t *pcm, snd_pcm_info_t *info ATTRIBUTE_UNUSED)
{
	TRACE_PCM(pcm);
#ifndef DEBUG
	QA_USE_VAR(pcm);
#endif
	QA_USE_VAR(info);
	return 0;
}

/*limit start threshold to buffer_size and align to period size - otherwise start threshold may never be reached*/
#define limit_start(val, _p) min((val), (((_p)->buffer_size) / ((_p)->period_size)) * ((_p)->period_size)  )

static int snd_pcm_arate_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
	TRACE_PCM(pcm);
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	struct asrc_pcm *apcm = pcm2apcm(pcm);
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_sw_params_alloca(&sw_params);

	int ret;

	if (user_pcm(pcm)) {
		snd_pcm_t *spcm = arate->asrc_slave->asrc_pcm;
		ret = snd_pcm_sw_params_current(spcm, sw_params);
		if (ret < 0) {
			SNDERR("unable to get current sw_params");
			return ret;
		}

		params->tstamp_mode = SND_PCM_TSTAMP_NONE;

		switch (arate->silence_mode) {
		case SILENCE_MODE_OFF:
			ret = snd_pcm_sw_params_set_start_threshold(spcm, sw_params, limit_start(START_THRESHOLD_PERIODS * spcm->period_size, spcm));
			break;
		case SILENCE_MODE_CONTINOUS:
			ret = snd_pcm_sw_params_set_start_threshold(spcm, sw_params, limit_start(1, spcm));
			break;
		case SILENCE_MODE_DEFINED_PREFILL: 
			ret = snd_pcm_sw_params_set_start_threshold(spcm, sw_params, limit_start(arate->silence_size + spcm->period_size, spcm));
			break;
		}
		if (ret < 0) {
			SNDERR("unable to set start threshold");
			return ret;
		}

		/*just interprete stop threshold to disable/enable underrun detection*/
		if (params->stop_threshold > pcm->buffer_size) {
			ret = snd_pcm_sw_params_set_stop_threshold(spcm, sw_params, INT_MAX);
			if (ret < 0) {
				SNDERR("unable to set stop threshold");
				return ret;
			}
		}

		/*don't use snd_pcm_sw_params_set_avail_min, as this restricts avail_min to >= spcm->period_size !*/
		sw_params->avail_min = min(arate->slave_pcm->period_size, spcm->period_size);

		ret = sync_sw(pcm, params);
		if (ret < 0) {
			SNDERR("sync sw params failed");
			return ret;
		}

		ret = snd_pcm_sw_params(spcm, sw_params);
		if (ret < 0) {
			SNDERR("unable to install sw params");
			return ret;
		}

		ret = 0;
	} else {
		snd_pcm_t *spcm = arate->slave_pcm;

		ret = snd_pcm_sw_params_current(spcm, sw_params);
		if (ret < 0) {
			SNDERR("unable to get current sw_params");
			return ret;
		}

		switch (arate->silence_mode) {
		case SILENCE_MODE_OFF:
			ret = snd_pcm_sw_params_set_start_threshold(spcm, sw_params, limit_start(START_THRESHOLD_PERIODS * spcm->period_size, spcm));
			break;
		case SILENCE_MODE_CONTINOUS:
			ret = snd_pcm_sw_params_set_start_threshold(spcm, sw_params, limit_start(1, spcm));
			break;
		case SILENCE_MODE_DEFINED_PREFILL: 
			ret = snd_pcm_sw_params_set_start_threshold(spcm, sw_params, limit_start(arate->silence_size + spcm->period_size, spcm));
			break;
		}
		if (ret < 0) {
			SNDERR("unable to set start threshold");
			return ret;
		}
		apcm->asrc_slave_sync.start_threshold = sw_params->start_threshold;

#ifdef CHAIN_XRUN_DISABLE
		/*there is no way to detect whether xrun was disabled by user or via src setting, so never switch off on slave.*/
		/*just interprete stop threshold to disable/enable underrun detection*/
		if (params->stop_threshold > pcm->buffer_size) {
			ret = snd_pcm_sw_params_set_stop_threshold(spcm, sw_params, INT_MAX);
			if (ret < 0) {
				SNDERR("unable to set stop threshold");
				return ret;
			}
		}
#endif
		/*playback: available minimum of asrc period / slave period
		can't use snd_pcm_sw_params_set_avail_min, as this restricts avail_min to >= spcm->period_size
		ret = snd_pcm_sw_params_set_avail_min(spcm, sw_params, pcm->period_size);
		if (ret < 0) {
			SNDERR("unable to set avail_min");
			return ret;
		}*/
		if (arate->silence_mode == SILENCE_MODE_CONTINOUS) {
			/*always keep 1-2 periods of silence in output buffer until asrc output available*/
			sw_params->avail_min = spcm->buffer_size - (1*spcm->period_size);
		} else
			sw_params->avail_min = min(pcm->period_size,spcm->period_size);

		if (apcm->asrc_ops.slave_sync)
			snd_pcm_sw_params_set_tstamp_mode(spcm, sw_params, SND_PCM_TSTAMP_ENABLE);

		ret = sync_sw(pcm, params);
		if (ret < 0) {
			SNDERR("sync sw params failed");
			return ret;
		}

		ret = snd_pcm_sw_params(spcm, sw_params);
		if (ret < 0) {
			SNDERR("unable to install sw params");
			return ret;
		}
	}
	return ret;
}


static int snd_pcm_arate_prepare(snd_pcm_t *pcm)
{
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	int err = 0;
	TRACE_PCM(pcm);

	arate->silence_prolog = 1;
	arate->draining = 0;
	arate->aborting = 0;
	arate->silence_remain = arate->silence_size;
	if (user_pcm(pcm)) {
		err = apcm->asrc_ops.prepare(apcm->asrc_obj);
		if (!err) {
			alsa_state(apcm) = SND_PCM_STATE_PREPARED;
			/*sync after setting state prepare!*/
			*pcm->appl.ptr = 0;
			err = sync_hw(pcm);
			if (err)
				return err;
			err = snd_pcm_prepare(arate->asrc_slave->asrc_pcm);
			if (err < 0)
				SNDERR("slave prepare failed %d",err);
		} else {
			SNDERR("asrc user prepare failed %d",err);
		}
	} else {
		err = apcm->asrc_ops.prepare(apcm->asrc_obj);
		if (!err) {
			alsa_state(apcm) = SND_PCM_STATE_PREPARED;
			/*sync after setting state prepare!*/
			*pcm->appl.ptr = 0;
			err = sync_hw(pcm);
			if (err)
				return err;
			err = snd_pcm_prepare(arate->slave_pcm);
			if( err <0)
				SNDERR("slave prepare failed %d",err);
		} else {
			SNDERR("asrc slave prepare failed %d",err);
		}
	}

	return err;
}


static int snd_pcm_arate_start(snd_pcm_t *pcm)
{
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	struct asrc_pcm *apcm = pcm2apcm(pcm);
	int err = 0;
	TRACE_PCM(pcm);

	gettimestamp(&apcm->trigger_tstamp, get_tstamp_type(pcm));

	if (arate->slave_pcm->stream == SND_PCM_STREAM_PLAYBACK) {
		if (user_pcm(pcm)) {
			err = snd_pcm_start(arate->asrc_slave->asrc_pcm);
			if (!err)
				err = apcm->asrc_ops.start(apcm->asrc_obj);
			if (!err)/*wakeup will reschedule-->asrc_ops-start must be called on both sides before starting thread*/
				wakeup_waiter(arate, &(arate->run), 1);
		} else
			err = apcm->asrc_ops.start(apcm->asrc_obj);
	} else {
		if (user_pcm(pcm)) {
			err = snd_pcm_start(arate->slave_pcm);
			if (!err)
				err = apcm->asrc_ops.start(apcm->asrc_obj);
			/*ASRC slave side is started by thread as soon as enough samples are available...*/
			if (!err)
				wakeup_waiter(arate, &(arate->run), 1);
		} else
			err = apcm->asrc_ops.start(apcm->asrc_obj);
	}

	if (err < 0)
		SNDERR("%s arate start failed %d",user_pcm(pcm)?"USER":"SLAVE",err);

	if (!err)
		alsa_state(apcm) = SND_PCM_STATE_RUNNING;

	return err;
}


static int snd_pcm_arate_drop(snd_pcm_t *pcm)
{
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	int err = 0;
	TRACE_PCM(pcm);

	gettimestamp(&apcm->trigger_tstamp, get_tstamp_type(pcm));

	if (user_pcm(pcm)) {
		err = apcm->asrc_ops.stop(apcm->asrc_obj);
		if (err < 0)
			SNDERR("asrc stop failed %d",err);

		err = snd_pcm_drop(arate->asrc_slave->asrc_pcm);

		/*wait for thread also stopped*/
		wait_for(arate, !arate->run);

	} else {
		err = apcm->asrc_ops.stop(apcm->asrc_obj);
		if (err < 0)
			SNDERR("asrc stop failed %d",err);
	}

	if(!err)
		alsa_state(apcm) = SND_PCM_STATE_SETUP;

	return 0;
}


static int arate_insert_silence(snd_pcm_t *pcm, snd_pcm_sframes_t silence_len)
{
	snd_pcm_sframes_t written=0;
	snd_pcm_sframes_t avail;
	int err;
	void *silence = (void*)alloca((size_t)snd_pcm_frames_to_bytes(pcm, silence_len));
	TRACE_PCM(pcm);

	snd_pcm_format_set_silence(pcm->format, silence, silence_len * pcm->channels);
	while (written<silence_len) {
		err = snd_pcm_wait(pcm, 1000);

		if (err<=0) {
			SNDERR("-DRAIN--WAIT %s (err %d)", err==0?"TOUT":"FAILED", err);
			return err ? err:-EIO;
		}
		avail = snd_pcm_avail(pcm);
		if (avail < 0)
			return avail;
		DBG_PCM(pcm,"drain ->avail %ld\n", avail);	
		if(avail > (silence_len-written))
			avail = silence_len-written;
		if(avail > silence_len)
			avail = silence_len;
		err = snd_pcm_mmap_writei(pcm, silence, avail);

		if (err < 0) {
			DBG_PCM(pcm,"---------------------drain MMAP FAILED %d \n",err);
			return err;
		}
		written += err;
		DBG_PCM(pcm,"drain ->written %d sum %ld\n", err, written);
	}
	return 0;
}


#define ARATE_DRAIN
static int snd_pcm_arate_drain(snd_pcm_t *pcm)
{
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	int err = 0;
	TRACE_PCM(pcm);
#ifdef ARATE_DRAIN
	pcm->stop_threshold = pcm->buffer_size;
	err = sync_hw(pcm);
	if (err)
		return err;

	if (user_pcm(pcm)) {
		/*insert silence up to next period boundary and give one extra period*/
		arate->draining = 1;
		err = arate_insert_silence(pcm, (pcm->period_size-*pcm->appl.ptr%pcm->period_size) + pcm->period_size);
		if (err)
			return err;
		/*set state drain AFTER write as write in state DRAIN is not possible*/
		alsa_state(apcm) = SND_PCM_STATE_DRAINING;
		err = snd_pcm_drain(arate->asrc_slave->asrc_pcm);
		if (!err)	
			err = snd_pcm_drop(pcm);
	} else {
		if (apcm->asrc_ops.drain) {
			/*->signal drain to ASRC and wait for thread stopped*/
			err = apcm->asrc_ops.drain(apcm->asrc_obj);
		}
		alsa_state(apcm) = SND_PCM_STATE_DRAINING;
		wait_for(arate, !arate->run);
	}
#else
	err = snd_pcm_drop(pcm);
#endif
	return err;
}


static snd_pcm_sframes_t snd_pcm_arate_avail_update(snd_pcm_t *pcm)
{
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	snd_pcm_uframes_t avail;
	TRACE_PCM(pcm);

	//err = sync_hw(pcm);
	avail = snd_pcm_mmap_avail(pcm);
	if (avail > apcm->avail_max)
		apcm->avail_max = avail;

	switch(alsa_state(pcm2apcm(pcm))) {
	case SNDRV_PCM_STATE_DRAINING:
	case SNDRV_PCM_STATE_RUNNING:
	case SNDRV_PCM_STATE_PREPARED:
	case SNDRV_PCM_STATE_SUSPENDED:
		break;
	case SNDRV_PCM_STATE_XRUN:
		return -EPIPE;
	case SNDRV_PCM_STATE_DISCONNECTED:
		return -ENODEV;
	default:
		return -EBADFD;
	}

	DBG_PCM(pcm,"%s AVAIL UPDATE: HW %ld APP %ld -> avail %ld\n", user_pcm(pcm)?"USER":"SLAVE",*pcm->hw.ptr, *pcm->appl.ptr, avail);
	return avail;
}


static int snd_pcm_arate_mmap(snd_pcm_t *pcm)
{
	TRACE_PCM(pcm);
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	unsigned int c;

	if (!apcm->asrc_io_cfg.buffer) {
		SNDERR("io buffer not mapped");
		return -EIO;
	}

	pcm->mmap_channels = calloc(pcm->channels, sizeof(pcm->mmap_channels[0]));
	if (!pcm->mmap_channels)
		return -ENOMEM;
	pcm->running_areas = calloc(pcm->channels, sizeof(pcm->running_areas[0]));
	if (!pcm->running_areas) {
		free(pcm->mmap_channels);
		pcm->mmap_channels = NULL;
		return -ENOMEM;
	}

	for (c = 0; c < pcm->channels; ++c) {
		snd_pcm_channel_info_t *i = &pcm->mmap_channels[c];
		snd_pcm_channel_area_t *a = &pcm->running_areas[c];

		i->addr = apcm->asrc_io_cfg.buffer;
		i->first = c * pcm->sample_bits;
		i->step = pcm->frame_bits;

       		a->addr = i->addr;
       		a->first = i->first;
       		a->step = i->step;
#ifdef DEBUG
		DBG_PCM(pcm,"Channel %d alloc: addr %p first %d step %d\n", c, a->addr, a->first,a->step);
#endif
	}
	return 0;
}
 
      
static int snd_pcm_arate_munmap(snd_pcm_t *pcm)
{
	TRACE_PCM(pcm);
	if (pcm->running_areas) {
		free(pcm->running_areas);
		pcm->running_areas = NULL;
	}
	if (pcm->mmap_channels) {
		free(pcm->mmap_channels);
		pcm->mmap_channels = NULL;
	}
	return 0;
}


int snd_pcm_arate_channel_info(snd_pcm_t *pcm, snd_pcm_channel_info_t * info)
{
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	unsigned int c = info->channel;
	TRACE_PCM(pcm);

	info->addr = apcm->asrc_io_cfg.buffer;
	info->first = c * pcm->sample_bits;
	info->step = pcm->frame_bits;
	info->type = SND_PCM_AREA_MMAP;
	info->u.mmap.fd = pcm->poll_fd;
	info->u.mmap.offset = 0;

	return 0;
}


static snd_pcm_sframes_t snd_pcm_arate_mmap_commit(snd_pcm_t *pcm,
						  snd_pcm_uframes_t offset ATTRIBUTE_UNUSED,
						  snd_pcm_uframes_t size)
{
	int err;
	TRACE_PCM(pcm);

	QA_USE_VAR(offset);
	if (!size)
		return 0;

	snd_pcm_mmap_appl_forward(pcm, size);
	err = sync_hw(pcm);
	if (err)
		return err;

	switch(alsa_state(pcm2apcm(pcm))) {
	case SNDRV_PCM_STATE_DRAINING:
	case SNDRV_PCM_STATE_RUNNING:
	case SNDRV_PCM_STATE_PREPARED:
	case SNDRV_PCM_STATE_SUSPENDED:
		break;
	case SNDRV_PCM_STATE_XRUN:
		return -EPIPE;
	case SNDRV_PCM_STATE_DISCONNECTED:
		return -ENODEV;
	default:
		return -EBADFD;
	}

	return size;
}


static int snd_pcm_arate_hwsync(snd_pcm_t *pcm)
{
	int err;
	TRACE_PCM(pcm);
	err = sync_hw(pcm);
	if (err)
		return err;
	return 0;
}


static int snd_pcm_arate_delay(snd_pcm_t *pcm, snd_pcm_sframes_t *delayp)
{
	int err;
	
	switch (alsa_state(pcm2apcm(pcm))) {
	case SND_PCM_STATE_DRAINING:
	case SND_PCM_STATE_RUNNING:
		err = sync_hw(pcm);
		if (err < 0)
			return err;
		/* fallthru */
	case SND_PCM_STATE_PREPARED:
	case SND_PCM_STATE_SUSPENDED:
		*delayp = snd_pcm_mmap_hw_avail(pcm);
		return 0;
	case SND_PCM_STATE_XRUN:
		return -EPIPE;
	case SND_PCM_STATE_DISCONNECTED:
		return -ENODEV;
	default:
		return -EBADFD;
	}
}


static snd_pcm_state_t snd_pcm_arate_state(snd_pcm_t *pcm)
{
	TRACE_PCM(pcm);
	return alsa_state(pcm2apcm(pcm));
}


static int snd_pcm_arate_status(snd_pcm_t *pcm, snd_pcm_status_t * status)
{
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	TRACE_PCM(pcm);

	memset(status, 0, sizeof(*status));
	status->state = alsa_state(apcm);
	status->trigger_tstamp = apcm->trigger_tstamp;
	gettimestamp(&status->tstamp, get_tstamp_type(pcm));
	status->delay = snd_pcm_mmap_hw_avail(pcm);
	status->avail = snd_pcm_mmap_avail(pcm);
	status->avail_max = status->avail > apcm->avail_max ? status->avail : apcm->avail_max;
	apcm->avail_max = 0;

	return 0;
}


static void snd_pcm_arate_dump(snd_pcm_t *pcm, snd_output_t *out)
{
	TRACE_PCM(pcm);
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	struct asrc_pcm * apcm = pcm2apcm(pcm);

	if(user_pcm(pcm)){
		if (apcm->asrc_ops.dump)
			apcm->asrc_ops.dump(apcm->asrc_obj, out);
		snd_output_printf(out, "arate PCM USER\n");
		if (pcm->setup) {
			snd_output_printf(out, "Its setup is:\n");
			snd_pcm_dump_setup(pcm, out);
			snd_output_printf(out, "  appl_ptr     : %ld\n", *pcm->appl.ptr);
			snd_output_printf(out, "  hw_ptr       : %ld\n", *pcm->hw.ptr);
			snd_output_printf(out, "  fifo_size    : %ld\n", pcm->fifo_size);
		}
		snd_pcm_dump(arate->asrc_slave->asrc_pcm, out);
	}else {
		snd_output_printf(out, "arate PCM SLAVE\n");
		if (pcm->setup) {
			snd_output_printf(out, "Its setup is:\n");
			snd_pcm_dump_setup(pcm, out);
			snd_output_printf(out, "  appl_ptr     : %ld\n", *pcm->appl.ptr);
			snd_output_printf(out, "  hw_ptr       : %ld\n", *pcm->hw.ptr);
			snd_output_printf(out, "  fifo_size    : %ld\n", pcm->fifo_size);
		}
		snd_output_printf(out, "arate SLAVE PCM \n");
		snd_pcm_dump(arate->slave_pcm, out);
		snd_output_printf(out, "  fifo_size    : %ld\n", arate->slave_pcm->fifo_size);
	}
}


static int snd_pcm_arate_async(snd_pcm_t *pcm, int sig ATTRIBUTE_UNUSED, pid_t pid ATTRIBUTE_UNUSED)
{
	TRACE_PCM(pcm);
#ifndef DEBUG
	QA_USE_VAR(pcm);
#endif
	QA_USE_VAR(sig);
	QA_USE_VAR(pid);
	return -EINVAL;
}


static snd_pcm_sframes_t snd_pcm_arate_writen(snd_pcm_t *pcm ATTRIBUTE_UNUSED, void **buffer ATTRIBUTE_UNUSED,
						snd_pcm_uframes_t size ATTRIBUTE_UNUSED)
{
	QA_USE_VAR(pcm);
	QA_USE_VAR(buffer);
	QA_USE_VAR(size);
	return -ENODEV;
}


static snd_pcm_sframes_t snd_pcm_arate_readn(snd_pcm_t *pcm ATTRIBUTE_UNUSED, void **bufs ATTRIBUTE_UNUSED,
						snd_pcm_uframes_t size ATTRIBUTE_UNUSED)
{
	QA_USE_VAR(pcm);
	QA_USE_VAR(bufs);
	QA_USE_VAR(size);
	return -ENODEV;
}


static snd_pcm_sframes_t snd_pcm_arate_rewindable(snd_pcm_t *pcm)
{
	return snd_pcm_mmap_hw_avail(pcm);
}


static snd_pcm_sframes_t snd_pcm_arate_rewind(snd_pcm_t *pcm, snd_pcm_uframes_t frames)
{
	snd_pcm_sframes_t avail;

	avail = snd_pcm_mmap_hw_avail(pcm);
	if (avail < 0)
		return 0;
	if (frames > (snd_pcm_uframes_t)avail)
		frames = avail;
	snd_pcm_mmap_appl_backward(pcm, frames);
	return frames;
}


static snd_pcm_sframes_t snd_pcm_arate_forwardable(snd_pcm_t *pcm)
{
	return snd_pcm_mmap_avail(pcm);
}


static snd_pcm_sframes_t snd_pcm_arate_forward(snd_pcm_t *pcm, snd_pcm_uframes_t frames)
{
	snd_pcm_sframes_t avail;

	avail = snd_pcm_mmap_avail(pcm);
	if (avail < 0)
		return 0;
	if (frames > (snd_pcm_uframes_t)avail)
		frames = avail;
	snd_pcm_mmap_appl_forward(pcm, frames);
	return frames;
}


static int snd_pcm_arate_pause(snd_pcm_t *pcm ATTRIBUTE_UNUSED, int enable ATTRIBUTE_UNUSED)
{
	QA_USE_VAR(pcm);
	QA_USE_VAR(enable);
	return -EIO;
}


static int snd_pcm_arate_resume(snd_pcm_t *pcm ATTRIBUTE_UNUSED)
{
	QA_USE_VAR(pcm);
	return -EIO;
}


/*TODO IMPLEMENT */
static int snd_pcm_arate_htimestamp(snd_pcm_t *pcm ATTRIBUTE_UNUSED,
				   snd_pcm_uframes_t *avail ATTRIBUTE_UNUSED,
				   snd_htimestamp_t *tstamp ATTRIBUTE_UNUSED)
{
	QA_USE_VAR(pcm);
	QA_USE_VAR(avail);
	QA_USE_VAR(tstamp);
	return -EIO;
}


static int snd_pcm_arate_reset(snd_pcm_t *pcm)
{
	*pcm->appl.ptr = *pcm->hw.ptr;
	return sync_hw(pcm);
}

/*Close must not be called from signal handler as used mutex is not async safe.
 On returning from close it is guaranteed, that asrc is closed and a new open can immediately happen.
-TODO: semaphore not needed anymore ->remove
 */
static int snd_pcm_arate_close(snd_pcm_t *pcm)
{
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	int err;
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	
	TRACE_PCM(pcm);

	if (user_pcm(pcm)) {
		err = apcm->asrc_ops.close(apcm->asrc_obj);
		if (err < 0)
			SNDERR("asrc user close failed %d",err);
		apcm->asrc_obj = NULL;
		/*IF called from signal handler mutex can deadlock with worker_thread !*/
		wakeup_waiter(arate, &(arate->exit), 1);
		sem_wait(&arate->thread_sync);
		pthread_join(arate->forward_thread, NULL);
		sem_destroy(&arate->thread_sync);
		pthread_cond_destroy(&arate->arate_slave_cond);
		pthread_mutex_destroy(&arate->arate_slave_mutex);
		if (arate->s_hw_params) {
			snd_pcm_hw_params_free(arate->s_hw_params);
			arate->s_hw_params = NULL;
		}
		if (arate->log) {
			snd_output_close(arate->log);
			arate->log = NULL;
		}
		pcm->private_data = NULL;
		free(arate);
	} else {
		err = apcm->asrc_ops.close(apcm->asrc_obj);
		if (err < 0)
			SNDERR("asrc slave close failed %d",err);
		apcm->asrc_obj = NULL;
		pcm->private_data = NULL;
		arate->exit = 0;
		sem_post(&arate->thread_sync);
	}

	return err;
}


static const snd_pcm_ops_t snd_pcm_arate_ops = {
	.close = snd_pcm_arate_close,
	.info = snd_pcm_arate_info,
	.hw_refine = snd_pcm_arate_hw_refine,
	.hw_params = snd_pcm_arate_hw_params,
	.hw_free = snd_pcm_arate_hw_free,
	.sw_params = snd_pcm_arate_sw_params,
	.channel_info = snd_pcm_arate_channel_info,
	.dump = snd_pcm_arate_dump,
	.nonblock = snd_pcm_arate_nonblock,
	.async = snd_pcm_arate_async,
	.mmap = snd_pcm_arate_mmap,
	.munmap = snd_pcm_arate_munmap,
};


static const snd_pcm_fast_ops_t snd_pcm_arate_fast_ops = {
	.status = snd_pcm_arate_status,
	.state = snd_pcm_arate_state,
	.hwsync = snd_pcm_arate_hwsync,
	.delay = snd_pcm_arate_delay,
	.prepare = snd_pcm_arate_prepare,
	.reset = snd_pcm_arate_reset,
	.start = snd_pcm_arate_start,
	.drop = snd_pcm_arate_drop,
	.drain = snd_pcm_arate_drain,
	.pause = snd_pcm_arate_pause,
	.rewindable = snd_pcm_arate_rewindable,
	.rewind = snd_pcm_arate_rewind,
	.forwardable = snd_pcm_arate_forwardable,
	.forward = snd_pcm_arate_forward,
	.resume = snd_pcm_arate_resume,
	.link = NULL,
	.link_slaves = NULL,
	.unlink = NULL,
	.writei = snd_pcm_mmap_writei,
	.writen = snd_pcm_arate_writen,
	.readi = snd_pcm_mmap_readi,
	.readn = snd_pcm_arate_readn,
	.avail_update = snd_pcm_arate_avail_update,
	.mmap_commit = snd_pcm_arate_mmap_commit,
	.htimestamp = snd_pcm_arate_htimestamp,
	.poll_descriptors_count = snd_pcm_arate_poll_descriptors_count,
	.poll_descriptors = snd_pcm_arate_poll_descriptors,
	.poll_revents = snd_pcm_arate_poll_revents,
};

/*clear unnecessary events to prevent unnecessary returning from poll*/
static int clear_event(struct pollfd *pfd, int num, short events)
{
	int i;
	if(events == 0)
		return 0;
	for(i=0;i<num;i++)
		pfd[i].events &= ~events;
	return 0;
}

/*similar to snd_pcm_wait_nocheck(), but waiting for 2 pcm's*/
static int snd_pcm_arate_wait_pcms(snd_pcm_t *pcmA, short ign_eventsA, snd_pcm_t *pcmB, short ign_eventsB, int timeout)
{
	struct pollfd *pfd;
	unsigned short reventsA = 0;
	unsigned short reventsB = 0;
	int npfdsA,npfdsB, err, err_poll;
	
	npfdsA = snd_pcm_poll_descriptors_count(pcmA);
	if (npfdsA <= 0 || npfdsA >= 16) {
		SNDERR("Invalid poll_fds %d\n", npfdsA);
		return -EIO;
	}
	npfdsB = snd_pcm_poll_descriptors_count(pcmB);
	if (npfdsB <= 0 || npfdsB >= 16) {
		SNDERR("Invalid poll_fds %d\n", npfdsB);
		return -EIO;
	}
	pfd = (struct pollfd *)alloca(sizeof(*pfd) * (npfdsA+npfdsB));

	err = snd_pcm_poll_descriptors(pcmA, &pfd[0], npfdsA);
	if (err < 0)
		return err;
	if (err != npfdsA) {
		SNDMSG("invalid poll descriptors %d\n", err);
		return -EIO;
	}
	clear_event(&pfd[0], npfdsA, ign_eventsA);

	err = snd_pcm_poll_descriptors(pcmB, &pfd[npfdsA], npfdsB);
	if (err < 0)
		return err;
	if (err != npfdsB) {
		SNDMSG("invalid poll descriptors %d\n", err);
		return -EIO;
	}
	clear_event(&pfd[npfdsA], npfdsB, ign_eventsB);

	do {
		err_poll = poll(pfd, npfdsA+npfdsB, timeout);
		if (err_poll < 0) {
		        if (errno == EINTR)
		                continue;
			return -errno;
                }
		if (!err_poll)
			break;

		err = snd_pcm_poll_descriptors_revents(pcmA, &pfd[0], npfdsA, &reventsA);
		if (err < 0)
			return err;
		if (reventsA & (POLLERR | POLLNVAL)) {
			/* check more precisely */
			switch (snd_pcm_state(pcmA)) {
			case SND_PCM_STATE_XRUN:
				return -EPIPE;
			case SND_PCM_STATE_SUSPENDED:
				return -ESTRPIPE;
			case SND_PCM_STATE_DISCONNECTED:
				return -ENODEV;
			default:
				return -EIO;
			}
		}

		err = snd_pcm_poll_descriptors_revents(pcmB, &pfd[npfdsA], npfdsB, &reventsB);
		if (err < 0)
			return err;
		if (reventsB & (POLLERR | POLLNVAL)) {
			/* check more precisely */
			switch (snd_pcm_state(pcmB)) {
			case SND_PCM_STATE_XRUN:
				return -EPIPE;
			case SND_PCM_STATE_SUSPENDED:
				return -ESTRPIPE;
			case SND_PCM_STATE_DISCONNECTED:
				return -ENODEV;
			default:
				return -EIO;
			}
		}
	} while ((!(reventsA & ((POLLIN | POLLOUT)&~ign_eventsA)))&& (!(reventsB & ((POLLIN | POLLOUT) &~ign_eventsB))));

	return err_poll > 0 ? 1 : 0;
}

static inline void forward_atomic_begin(snd_pcm_arate_t *arate, struct asrc_pcm * apcm)
{
	if (apcm->asrc_ops.lock_sync) {
		apcm->asrc_ops.lock_sync(apcm->asrc_obj, 1);
		arate->core_locked = 1;
	}
}

static inline void forward_atomic_end(snd_pcm_arate_t *arate, struct asrc_pcm * apcm)
{
	if (apcm->asrc_ops.lock_sync && arate->core_locked) {
		apcm->asrc_ops.lock_sync(apcm->asrc_obj, 0);
		arate->core_locked = 0;
	}
}

static inline void forward_err(snd_pcm_arate_t *arate, struct asrc_pcm * apcm)
{
	forward_atomic_end(arate, apcm);
	snd_pcm_drop((arate)->slave_pcm);
	if(alsa_state(apcm) != SND_PCM_STATE_XRUN) {
		apcm->asrc_ops.xrun((apcm)->asrc_obj);
		apcm->asrc_ops.stop((apcm)->asrc_obj);
	}
	wakeup_waiter(arate, &((arate)->run), 0);
}


static int exit_silence_prolog(snd_pcm_t *source_pcm, snd_pcm_t *sink_pcm, snd_pcm_arate_t *arate)
{
	int err;
	snd_pcm_sw_params_t *sw_params;
	DBG_ARATE(arate,"exit silence prolog\n");
	/*switch to normal behavior*/
	arate->silence_prolog = 0;
	switch (arate->silence_mode) {
	case SILENCE_MODE_OFF:
		return 0;
	case SILENCE_MODE_CONTINOUS:
		/*exit continuous mode: restore settings*/
		snd_pcm_sw_params_alloca(&sw_params);
		err = snd_pcm_sw_params_current(sink_pcm, sw_params);
		if (err < 0)
			return err;
		sw_params->avail_min = min(source_pcm->period_size, sink_pcm->period_size);
		return snd_pcm_sw_params(sink_pcm, sw_params);
	case SILENCE_MODE_DEFINED_PREFILL:
		return 0;
	}
	return 0;
}

static int do_silence_prolog(snd_pcm_t *source_pcm, snd_pcm_t *sink_pcm, snd_pcm_arate_t *arate, snd_pcm_uframes_t source_avail, snd_pcm_uframes_t *sink_avail)
{
	int err;
	char *silence;
	snd_pcm_uframes_t silence_size = 0;

	switch (arate->silence_mode) {
	case SILENCE_MODE_OFF:
		return exit_silence_prolog(source_pcm, sink_pcm, arate);
	case SILENCE_MODE_CONTINOUS:
		if (source_avail > 0)
			return exit_silence_prolog(source_pcm, sink_pcm, arate);
		/*still no output available - wait for data on asrc or slave running low on silence output*/
		arate->silence_remain  = arate->silence_size;
		silence_size = min(arate->silence_remain, sink_pcm->period_size);
		break;
	case SILENCE_MODE_DEFINED_PREFILL:
		silence_size = min(arate->silence_remain, sink_pcm->period_size);
		break;
	}

	DBG_ARATE(arate,"do silence prolog\n");

	silence = (char*)alloca((size_t)snd_pcm_frames_to_bytes(sink_pcm, sink_pcm->period_size));
	snd_pcm_format_set_silence(sink_pcm->format, silence, sink_pcm->period_size * sink_pcm->channels);

	while (silence_size > 0) {
		err = snd_pcm_writei(sink_pcm, silence, silence_size);
		if (err<0)
			return err;
		*sink_avail -= err;
		silence_size -= err;
		arate->silence_remain -= err;
	}

	/*plugins may return always 0 on snd_pcm_mmap_playback_hw_avail() such as plugin null
	-detect such behavior and avoid endless loop*/
	if (snd_pcm_mmap_playback_hw_avail(sink_pcm) == 0) {
 		DBG_ARATE(arate,"inconsistent hw position ?? (e.g known on type null)\n");
		return exit_silence_prolog(source_pcm,sink_pcm, arate);
 	}

	if ((arate->silence_mode == SILENCE_MODE_DEFINED_PREFILL) && (arate->silence_remain == 0) )
		return exit_silence_prolog(source_pcm, sink_pcm, arate);

 	return 0;
}


static int ts_monotonic(struct timespec *before, struct timespec *now)
{
	struct timespec temp;
	if ((now->tv_nsec-before->tv_nsec)<0) {
		temp.tv_sec = (now->tv_sec-before->tv_sec)-1;
		temp.tv_nsec = 1000000000+now->tv_nsec-before->tv_nsec;
	} else {
		temp.tv_sec = now->tv_sec-before->tv_sec;
		temp.tv_nsec = now->tv_nsec-before->tv_nsec;
	}
	return ((temp.tv_nsec > 0) || (temp.tv_sec > 0)) ? 1 : 0;
}


#define ts_valid(_ts) ((_ts).tv_sec || (_ts).tv_nsec)
#define ts_after(ts_before, ts_after) (ts_monotonic(&(ts_before), &(ts_after))>0)

static void *arate_async_thread(void *arg)
{
	int err;
	char *buffer;
	snd_pcm_t *pcm = (snd_pcm_t*)arg;
	struct asrc_pcm * apcm = pcm2apcm(pcm);
	snd_pcm_arate_t *arate = pcm2arate(pcm);
	snd_pcm_uframes_t avail = 0;
	snd_pcm_uframes_t savail = 0;
	snd_pcm_t *spcm = arate->slave_pcm;
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t offset = 0;
	snd_pcm_uframes_t *source_avail;
	snd_pcm_uframes_t *sink_avail;
	snd_pcm_t *source_pcm;
	snd_pcm_t *sink_pcm;

	snd_pcm_status_t *status;
	snd_pcm_uframes_t frag;

	snd_pcm_status_alloca(&status);
	if (spcm->stream == SND_PCM_STREAM_PLAYBACK) {
		source_avail = &avail;
		sink_avail = &savail;
		source_pcm = pcm;
		sink_pcm = spcm;
	} else {
		source_avail = &savail;
		sink_avail = &avail;
		source_pcm = spcm;
		sink_pcm = pcm;
	}

	TRACE_PCM(pcm);
	
	while (1) {
		wait_for(arate, arate->run || arate->exit);
		if(arate->exit) {
			DBG_ARATE(arate,"THREAD EXIT\n");
			snd_pcm_close(spcm);
			arate->slave_pcm = NULL;
			snd_pcm_close(pcm);/*no access to arate beyond this point !!*/
			break;
		}

		DBG_ARATE(arate,"-->WAIT avail %ld (min %ld) savail %ld (min %ld)\n",avail,pcm->avail_min, savail,spcm->avail_min );

		/*wait for source AND sink availability*/
		if ((!*source_avail) || (!*sink_avail)) {
			/*always poll on ERRORS, but on IN/OUT only if needed*/
			err = snd_pcm_arate_wait_pcms(source_pcm, *source_avail?(POLLIN|POLLOUT):0,
					sink_pcm, (*source_avail|arate->silence_prolog)?0:(POLLIN|POLLOUT), arate->forward_timeout);
			if (err<=0) {
				if (arate->draining &&(err == -EPIPE)) {
					DBG_ARATE(arate,"  DRAIN SLAVE --------\n");

					snd_pcm_nonblock(spcm, 0);
					err = snd_pcm_drain(spcm);
				}
				avail = 0;
				savail = 0;
				DBG_ARATE(arate, "-----------------------WAIT ERR %d\n",err);
				forward_err(arate, apcm);
				continue;
			}
			avail = snd_pcm_avail_update(pcm);
			savail = snd_pcm_avail_update(spcm);
		}

		if (arate->silence_prolog) {
			DBG_ARATE(arate,"<--SILENCE: WAIT avail %ld (min %ld) savail %ld (min %ld)\n",avail,pcm->avail_min, savail,spcm->avail_min );
			err = do_silence_prolog(source_pcm, sink_pcm, arate, *source_avail, sink_avail);
			if ((err<0) && (err != -EAGAIN)) {
				avail = 0;
				savail = 0;
				DBG_ARATE(arate, "silence prolog err %d\n", err);
				forward_err(arate, apcm);
				continue;
			}
			continue;
		}

		/*source and sink available*/
		frag = min(avail, savail);

		DBG_ARATE(arate,"<--WAIT avail %ld (min %ld) savail %ld (min %ld)\n",avail,pcm->avail_min, savail,spcm->avail_min );

		if (!avail || !savail)
			continue;

		err = snd_pcm_mmap_begin(pcm, &areas, &offset, &frag);
		if (err<0) {
			avail = 0;
			savail = 0;
			DBG_ARATE(arate, "mmap begin = %ld err %d\n", frag, err);
			forward_err(arate, apcm);
			continue;
		}
		buffer = snd_pcm_channel_area_addr(areas, offset);

		if (spcm->stream == SND_PCM_STREAM_PLAYBACK)
			/*do write in any case to reach start threshold */
			err = snd_pcm_writei(spcm, buffer, frag);
		else
			err = snd_pcm_readi(spcm, buffer, frag);
		if (err < 0) {
			if (err != -EAGAIN) {
				avail = 0;
				savail = 0;
				DBG_ARATE(arate, "----------WRITE (len %ld) FAILED %d\n", frag, err);
				forward_err(arate, apcm);
			}
			continue;
		}
		/*mmap_commit and slave_sync needs to be atomic*/
		forward_atomic_begin(arate, apcm);
		err = snd_pcm_mmap_commit(pcm, offset, err);
		if (err<= 0) { /*treat err=0 as error, as otherwise could result in endless loop*/
			avail = 0;
			savail = 0;
			DBG_ARATE(arate, "mmap commit err %d %ld frames\n", err, frag);
			forward_err(arate, apcm);
			continue;
		}
		if ((pcm->stream == SND_PCM_STREAM_PLAYBACK) && (alsa_state(apcm) == SND_PCM_STATE_PREPARED)) {
			if ((snd_pcm_uframes_t)snd_pcm_mmap_playback_hw_avail(pcm) >= pcm->start_threshold)
				snd_pcm_start(pcm);
		}
		if (apcm->asrc_ops.slave_sync) {
			/*need some workarounds for proper timestamping:
			normally we would use just snd_pcm_status() to obtain audio timestamp, but this is broken on
			at least plugin dshare,dmix,dsnoop. 
			htimestamp can be obtained separately, but the tstamp is not necessarily an audio tstamp:
			-it may be tstamp of last sync call.
			*/
			err = snd_pcm_status(spcm, status);
			if (err) {
				SNDERR("Getting status failed with err %d", err);
				forward_err(arate, apcm);
				continue;
			}
			err = snd_pcm_htimestamp(spcm, &apcm->asrc_slave_sync.avail, &apcm->asrc_slave_sync.tstamp);
			if (err) {
				SNDERR("Getting tstamp failed with err %d", err);
				forward_err(arate, apcm);
				continue;
			}
			apcm->asrc_slave_sync.state = snd_pcm_state(spcm);
			/*fixup timestamps - set trigger tstamp as first valid timestamp*/
			if (apcm->asrc_slave_sync.state == SND_PCM_STATE_RUNNING) {
				if (ts_valid(apcm->asrc_slave_sync.tstamp)) {
					if (!ts_after(status->trigger_tstamp, apcm->asrc_slave_sync.tstamp))
						apcm->asrc_slave_sync.tstamp = status->trigger_tstamp;
				} else {
					apcm->asrc_slave_sync.tstamp = status->trigger_tstamp;
				}
			} else {
				apcm->asrc_slave_sync.tstamp.tv_sec = 0;
				apcm->asrc_slave_sync.tstamp.tv_nsec = 0;
			}

			err = apcm->asrc_ops.slave_sync(apcm->asrc_obj, &apcm->asrc_slave_sync);
			if (err) {
				SNDERR("slave sync failed with err %d", err);
				forward_err(arate, apcm);
				continue;
			}
		}
		forward_atomic_end(arate, apcm);
		avail-= frag;
		savail-= frag;
	}
	return NULL;
}


static int find_converter(const snd_config_t *converter, snd_pcm_arate_open_func_t *open_func)
{
	int err;
	const char *type = NULL;
	char open_name[64], lib_name[128], *lib = NULL;
	void  *dlobj = NULL;

	err = -ENOENT;
	if (!converter) {
		SNDERR("No converter defined");
		return err;
	}

	if (snd_config_get_string(converter, &type)) {
		SNDERR("Invalid type for rate converter");
		return -EINVAL;
	}

	snprintf(open_name, sizeof(open_name), "_snd_pcm_arate_%s_open", type);
	snprintf(lib_name, sizeof(lib_name), "%s/libasound_module_arate_%s.so", ALSA_PLUGIN_DIR, type);
	lib = lib_name;
	dlobj = snd_dlopen(lib, RTLD_LAZY);
	if (!dlobj) {
		SNDERR("open %s failed",lib);
		return -ENOENT;
	}
	*open_func = snd_dlsym(dlobj, open_name, NULL);
	if (*open_func == NULL) {
		SNDERR("symbol %s is not defined inside %s", open_name, lib);
		return -ENOENT;
	}
	return 0;
}

static void init_random(void)
{
	srand(getpid());
}

static void default_caps(asrc_capabilities_t *caps)
{
	caps->formats = 1<<SND_PCM_FORMAT_S16_LE;
	caps->rate_min = 8000;
	caps->rate_max = 96000;
	caps->channels_min = 1;
	caps->channels_max = 256;
	caps->bufbytes_max = 0x7fffffff;
	caps->periodbytes_min = 1;
	caps->periodbytes_max = 0x7fffffff;
	caps->periodtime_min = 1;
	caps->periodtime_max = 0x7fffffff;
	caps->buffertime_min = 1;
	caps->buffertime_max = 0x7fffffff;
	caps->periods_min = 2;
	caps->periods_max = 1024;
	caps->features = 0;
	caps->fifo_size = 0;
}

/**
 * \brief Creates a new arate PCM
 * \param pcmp Returns created PCM handle
 * \param name Name of PCM
 * \param sformat Slave format
 * \param srate Slave rate
 * \param converter SRC type string node
 * \param setup_identifier for converter
 * \param slave Slave PCM handle
 * \param close_slave When set, the slave PCM handle is closed with copy PCM
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
int snd_pcm_arate_open(snd_pcm_t **pcmp, const char *name,
		      struct arate_slave_params *sparams,
		      const snd_config_t *converter,
		      const snd_config_t *converter_cfg,
		      snd_pcm_t *slave, int close_slave ATTRIBUTE_UNUSED)
{
	snd_pcm_arate_t *arate;
	struct asrc_pcm *apcm;
	int err;
	int id;
	unsigned int mode = 0;
	snd_pcm_arate_open_func_t open_func = NULL;
	QA_USE_VAR(close_slave);

	arate = calloc(1, sizeof(*arate));
	if (!arate) {
		SNDERR("no memory");
		return -ENOMEM;
	}
	pthread_once(&once, init_random);
	id = rand();

	snd_output_stdio_attach(&arate->log, stdout, 0);

	err = sem_init(&arate->thread_sync, 0, 0);
	if (err < 0)
		goto err_sem;

	err = find_converter(converter, &open_func);
	if (err < 0)
		goto err_converter;

	arate->slave_pcm = slave;
	arate->sparams = *sparams;
	arate->forward_timeout = DEFAULT_TIMEOUT; //TODO: dynamically calculate from avail_min

	/*link In/OUT to USER/SLAVE side*/
	if (slave->stream == SND_PCM_STREAM_PLAYBACK) {
		arate->asrc_user = &arate->i;
		arate->asrc_slave = &arate->o;
	} else {
		arate->asrc_user = &arate->o;
		arate->asrc_slave = &arate->i;
	}

	/*IN*/
	err = snd_pcm_new(&arate->i.asrc_pcm, SND_PCM_TYPE_RATE, name, SND_PCM_STREAM_PLAYBACK, slave->mode);
	if (err < 0) {
		SNDERR("create IN pcm failed");
		goto err_pcm;
	}

	/*OUT*/
	err = snd_pcm_new(&arate->o.asrc_pcm, SND_PCM_TYPE_RATE, name, SND_PCM_STREAM_CAPTURE, slave->mode);
	if (err < 0) {
		SNDERR("create OUT pcm failed");
		goto err_pcm;
	}

	/*user side PCM*/
	apcm = arate->asrc_user;
	mode = SND_PCM_ARATE_STREAM_MODE_FE;
	if (apcm->asrc_pcm->stream == SND_PCM_STREAM_PLAYBACK)
		mode |= SND_PCM_ARATE_STREAM_MODE_IN;
	err = open_func(SND_PCM_ARATE_PLUGIN_VERSION, &apcm->asrc_obj, &apcm->asrc_ops,
				mode, id, converter_cfg);
	if (err < 0) {
		SNDERR("open arate user side failed");
		goto err_pcm;
	}

	apcm->arate = arate;
	apcm->asrc_pcm->ops = &snd_pcm_arate_ops;
	apcm->asrc_pcm->fast_ops = &snd_pcm_arate_fast_ops;

	apcm->asrc_pcm->private_data = arate->asrc_user;
	apcm->asrc_pcm->mmap_rw = 1;/*this forces snd_pcm_mmap() call regardless of mode*/
	apcm->asrc_pcm->mmap_shadow = 1;/*real mmap done internally*/

	apcm->asrc_pcm->poll_fd = apcm->asrc_ops.get_pollfd(apcm->asrc_obj);
	apcm->asrc_pcm->poll_events = apcm->asrc_ops.get_pollev(apcm->asrc_obj);
	default_caps(&apcm->asrc_caps);
	apcm->asrc_ops.get_capabilities(apcm->asrc_obj, &apcm->asrc_caps);

	/*If available we use an additional fd for error notifications from slave side...*/
	if (apcm->asrc_ops.get_pollfd_err)
		apcm->poll_fd_err = apcm->asrc_ops.get_pollfd_err(apcm->asrc_obj);
	else
		apcm->poll_fd_err = -1;

	snd_pcm_set_hw_ptr(apcm->asrc_pcm, &apcm->hw_ptr, -1, 0);
	snd_pcm_set_appl_ptr(apcm->asrc_pcm, &apcm->appl_ptr, -1, 0);
	alsa_state(apcm) = SND_PCM_STATE_OPEN;


	/*slave side PCM*/
	apcm = arate->asrc_slave;
	mode = 0;
	if (apcm->asrc_pcm->stream == SND_PCM_STREAM_PLAYBACK)
		mode |= SND_PCM_ARATE_STREAM_MODE_IN;
	err = open_func(SND_PCM_ARATE_PLUGIN_VERSION, &apcm->asrc_obj, &apcm->asrc_ops,
				mode, id, converter_cfg);

	if (err < 0) {
		SNDERR("open arate slave side failed");
		goto err_pcm;
	}

	apcm->arate = arate;
	apcm->asrc_pcm->ops = &snd_pcm_arate_ops;
	apcm->asrc_pcm->fast_ops = &snd_pcm_arate_fast_ops;

	apcm->asrc_pcm->private_data = arate->asrc_slave;
	apcm->asrc_pcm->mmap_rw = 1;/*this forces snd_pcm_mmap() call regardless of mode*/
	apcm->asrc_pcm->mmap_shadow = 1;/*real mmap done internally*/

	apcm->asrc_pcm->poll_fd = apcm->asrc_ops.get_pollfd(apcm->asrc_obj);
	apcm->asrc_pcm->poll_events = apcm->asrc_ops.get_pollev(apcm->asrc_obj);
	default_caps(&apcm->asrc_caps);
	apcm->asrc_ops.get_capabilities(apcm->asrc_obj, &apcm->asrc_caps);

	if (apcm->asrc_ops.get_pollfd_err)
		apcm->poll_fd_err = apcm->asrc_ops.get_pollfd_err(apcm->asrc_obj);
	else
		apcm->poll_fd_err = -1;

	snd_pcm_set_hw_ptr(apcm->asrc_pcm, &apcm->hw_ptr, -1, 0);
	snd_pcm_set_appl_ptr(apcm->asrc_pcm, &apcm->appl_ptr, -1, 0);
	alsa_state(apcm) = SND_PCM_STATE_OPEN;

	/*forwarding always uses nonblocking mode*/
	err = snd_pcm_nonblock(arate->slave_pcm, 1);
	if (err < 0) {
		SNDERR("unable to set nonblock mode for slave");
		goto err_nonblock;
	}
	err = snd_pcm_nonblock(arate->asrc_slave->asrc_pcm, 1);
	if (err < 0) {
		SNDERR("unable to set nonblock mode for arate slave side");
		goto err_nonblock;
	}

	/*global init*/
	snd_pcm_hw_params_malloc(&arate->s_hw_params);
	err = snd_pcm_hw_params_any(arate->slave_pcm, arate->s_hw_params);
	if (err<0) {
		SNDERR("SLAVE HW PARAMS ANY FAILED");
		goto err_hwparams;
	}
	/*use same timestamp type as slave*/
	if (snd_pcm_hw_params_is_monotonic(arate->s_hw_params)) {
		set_monotonic(arate->asrc_user->asrc_pcm);
		set_monotonic(arate->asrc_slave->asrc_pcm);
	}
	/* create forwarding thread*/
	err = pthread_mutex_init(&arate->arate_slave_mutex, NULL);
	if (err < 0) {
		SNDERR("pthread_cond_mutex failed");
		goto err_mtx;
	}
	err = pthread_cond_init(&arate->arate_slave_cond, NULL);
	if (err < 0) {
		SNDERR("pthread_cond_init failed");
		goto err_cond;
	}

	err = pthread_create( &arate->forward_thread, NULL, arate_async_thread, (void *)arate->asrc_slave->asrc_pcm );
	if (err < 0) {
		SNDERR("unable to create thread for %s",name);
		goto err_thread;
	}

	*pcmp = arate->asrc_user->asrc_pcm;

	return 0;

err_thread:
	pthread_cond_destroy(&arate->arate_slave_cond);
err_cond:
	pthread_mutex_destroy(&arate->arate_slave_mutex);
err_mtx:
	snd_pcm_hw_params_free(arate->s_hw_params);
err_hwparams:
err_nonblock:
err_pcm:
	if (arate->i.asrc_obj) 
		arate->i.asrc_ops.close(arate->i.asrc_obj);
	if (arate->i.asrc_pcm)
		snd_pcm_free(arate->i.asrc_pcm);
	if (arate->o.asrc_obj) 
		arate->o.asrc_ops.close(arate->o.asrc_obj);
	if (arate->o.asrc_pcm)
		snd_pcm_free(arate->o.asrc_pcm);
err_converter:
	sem_destroy(&arate->thread_sync);
err_sem:
	if (arate->log)
		snd_output_close(arate->log);
	free(arate);
	return err;
}


static int snd_pcm_open_arate_slave(snd_pcm_t **pcmp,
			     snd_config_t *root ATTRIBUTE_UNUSED,
			     snd_config_t *conf, snd_pcm_stream_t stream,
			     int mode,
			     snd_config_t *parent_conf ATTRIBUTE_UNUSED)
{
	const char *name;
	const char *id;
	int err;
	QA_USE_VAR(root);
	QA_USE_VAR(parent_conf);
	if ( snd_config_get_id(conf, &id) <0) {
		SNDERR("failed getting entry PCM");
		return -EINVAL;
	}
	if (snd_config_get_string(conf, &name) < 0) {
		SNDERR("failed getting PCM name");
		return -EINVAL;
	}
	err = snd_pcm_open(pcmp, name, stream, mode);
	if (err < 0) {
		SNDERR("unable to open slave %s",name);
		return err;
	}
	/*preserve slave mode until snd_pcm_new() is done...*/
	return 0;
}


/**
 * \brief Creates a new arate PCM
 * \param pcmp Returns created PCM handle
 * \param name Name of PCM
 * \param root Root configuration node
 * \param conf Configuration node with rate PCM description
 * \param stream Stream type
 * \param mode Stream mode
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
int _snd_pcm_arate_open(snd_pcm_t **pcmp, const char *name,
		       snd_config_t *root, snd_config_t *conf, 
		       snd_pcm_stream_t stream, int mode)
{
	snd_config_iterator_t i, next;
	int err;
	snd_pcm_t *spcm;
	snd_config_t *slave = NULL, *sconf;
	const snd_config_t *converter = NULL;
	const snd_config_t *converter_cfg = NULL;
	struct arate_slave_params sparams;
	const char* curr_version = snd_asoundlib_version();
	
	if (strcmp(curr_version, SND_LIB_VERSION_STR) != 0) {
		SNDERR("Version mismatch (build:%s current:%s)",SND_LIB_VERSION_STR, curr_version);
		return -EPERM;
	}

	sparams.format = SND_PCM_FORMAT_UNKNOWN;
	sparams.rate = -1;
	sparams.period_time = -1;
	sparams.buffer_time = -1;
	sparams.period_size = -1;
	sparams.buffer_size = -1;
	sparams.prefill_ms = -1;

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (snd_pcm_conf_generic_id(id))
			continue;
		if (strcmp(id, "slave") == 0) {
			slave = n;
			continue;
		}
		if (strcmp(id, "converter") == 0) {
			converter = n;
			continue;
		}
		if (strcmp(id, "converter_cfg") == 0) {
			converter_cfg = n;
			continue;
		}
		if (strcmp(id, "prefill_ms") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0)
				return err;
			sparams.prefill_ms = val;
			continue;
		}
		SNDERR("Unknown field %s", id);
		return -EINVAL;
	}
	if (!slave) {
		SNDERR("slave is not defined");
		return -EINVAL;
	}

	err = snd_pcm_slave_conf(root, slave, &sconf, 6,
				 SND_PCM_HW_PARAM_FORMAT, 0, &sparams.format,
				 SND_PCM_HW_PARAM_RATE, SCONF_MANDATORY, &sparams.rate,
				 SND_PCM_HW_PARAM_PERIOD_TIME, 0, &sparams.period_time,
				 SND_PCM_HW_PARAM_BUFFER_TIME, 0, &sparams.buffer_time,
				 SND_PCM_HW_PARAM_PERIOD_SIZE, 0, &sparams.period_size,
				 SND_PCM_HW_PARAM_BUFFER_SIZE, 0, &sparams.buffer_size);
	if (err < 0)
		return err;
	if (sparams.format != SND_PCM_FORMAT_UNKNOWN &&
	    snd_pcm_format_linear(sparams.format) != 1) {
	    	snd_config_delete(sconf);
		SNDERR("slave format is not linear");
		return -EINVAL;
	}

	/*snd_pcm_open_slave() is not exportet and not easy to reimplement in same way,
	so we use a stripped version
	err = snd_pcm_open_slave(&spcm, root, sconf, stream, mode, conf);*/
	err = snd_pcm_open_arate_slave(&spcm, root, sconf, stream, mode, conf);

	snd_config_delete(sconf);
	if (err < 0)
		return err;

	err = snd_pcm_arate_open(pcmp, name, &sparams,
				converter, converter_cfg, spcm, 1);
	if (err < 0)
		snd_pcm_close(spcm);
	return err;
}

/*PRQA: Lint Message 19 : This is mandatory for ALSA lib plugins */
/*lint -save -e19 */
SND_DLSYM_BUILD_VERSION(_snd_pcm_arate_open, SND_PCM_DLSYM_VERSION);
/*lint -restore */
