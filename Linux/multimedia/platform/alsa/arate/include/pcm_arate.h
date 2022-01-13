/*
 * ALSA external PCM arate-converter plugin SDK (draft version)
 *
 * Copyright (c) 2013 ADIT GmbH
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

#ifndef __ALSA_PCM_ARATE_H
#define __ALSA_PCM_ARATE_H


#include <sys/types.h>


typedef struct asrc_io_cfg
{
	snd_pcm_format_t format;/**/
	int channels;
	int sample_rate;	/*IN*/
	int buf_size;/*IN*/
	int periods;/*IN*/
	void *buffer;/*OUT*/
}asrc_io_cfg_t;

#define SLV_CFG_TSTAMP_MONOTONIC 0x01

typedef struct
{
	snd_pcm_uframes_t buffer_size;/*IN*/
	snd_pcm_uframes_t period_size;/*IN*/
	unsigned int flags;			/*IN*/
}asrc_slave_cfg_t;

#define ASRC_FEAT_NEED_SLAVECLK 0x01

typedef struct asrc_capabilities
{//formates can differ on IN/OUT side !!
 	unsigned long long formats;  /* SNDRV_PCM_FMTBIT_* */

	int rate_min;
	int rate_max;

	int channels_min;
	int channels_max;

	/*int bufbytes_min; -->periods_min * periodbytes_min*/
	int bufbytes_max;

	int periodbytes_min;
	int periodbytes_max;

	int periodtime_min;	/*us*/
	int periodtime_max;	/*us*/
	int buffertime_min;	/*us*/
	int buffertime_max;	/*us*/

	int periods_min;
	int periods_max;
	int features;
	int fifo_size; /*fifo size in frames*/
}asrc_capabilities_t;


typedef struct
{
	snd_pcm_uframes_t app_pos;	/*IN*/
	snd_pcm_uframes_t avail_min;/*IN*/
	snd_pcm_uframes_t boundary;/*IN*/
	snd_pcm_uframes_t stop_threshold;/*IN/OUT*/
	snd_pcm_uframes_t start_threshold;/*IN/OUT*/

	snd_pcm_state_t state;/*IN/ OUT*/
	snd_pcm_uframes_t hw_pos;/*OUT*/
}asrc_sync_t;


typedef struct
{
	snd_pcm_uframes_t avail;	/*IN*/
	snd_pcm_uframes_t start_threshold;/*IN*/
	snd_pcm_state_t state;		/*IN*/
	snd_htimestamp_t tstamp;	/*IN*/
}asrc_slave_sync_t;


typedef struct
{
	int (*configure)(void* ctx, asrc_io_cfg_t *cfg);/*cfg: IN/OUT (IN = default values)*/
	int (*get_capabilities)(void* ctx, asrc_capabilities_t *caps);

	int (*dump)(void *ctx, snd_output_t *out); /*OPTIONAL*/
	int (*start)(void* ctx);
	int (*prepare)(void* ctx);
	int (*stop)(void* ctx);
	int (*xrun)(void* ctx);
	int (*drain)(void* ctx); /*OPTIONAL - nonblocking, just signalling*/
	int (*get_pollfd)(void* ctx);
	int (*get_pollev)(void* ctx);
	int (*set_pollfd_blockmode)(void* ctx, int nonblock);/*OPTIONAL*/
	int (*get_pollfd_err)(void* ctx);/*OPTIONAL*/
	int (*sync)(void* ctx, asrc_sync_t *sync_io);
	int (*close)(void* ctx);
	int (*slave_config)(void* ctx, asrc_slave_cfg_t *slv_cfg);/*OPTIONAL - called once BEFORE configure*/
	int (*slave_sync)(void* ctx, asrc_slave_sync_t *slv_sync);/*OPTIONAL - called periodically*/
	void (*lock_sync)(void* ctx, int lock);/*OPTIONAL - lock/unlock syncing*/
}asrc_ops_t;


/** open function type */
#define SND_PCM_ARATE_STREAM_MODE_IN 0x01	/*direction: 0: OUT 1:IN*/
#define SND_PCM_ARATE_STREAM_MODE_FE 0x02	/*front end/backend: 0: backend 1:frontend*/
typedef int (*snd_pcm_arate_open_func_t)(unsigned int version, void **objp, asrc_ops_t *ops, unsigned int mode, int id, const snd_config_t *conf);

#define SND_PCM_ARATE_PLUGIN_VERSION 0x000201

/**
 * Define the object entry for external PCM rate-converter plugins
 */
#define SND_PCM_ARATE_PLUGIN_ENTRY(name) _snd_pcm_arate_##name##_open


#endif /*__ALSA_PCM_ARATE_H*/
