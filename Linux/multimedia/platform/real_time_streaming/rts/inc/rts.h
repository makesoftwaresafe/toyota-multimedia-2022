/**
 * \file: rts.h
 *
 * Real time streaming library interface.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2014 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __RTS_H__
#define __RTS_H__

#ifdef __cplusplus
extern "C" {
#endif

struct rts_runtime;

typedef struct rts_runtime* tRTS; 
#define RTS_HANDLE_INVAL NULL

#define RTS_API_VERSION_MAJOR	0	/**< Protocol major version */
#define RTS_API_VERSION_MINOR	9	/**< Protocol minor version */
#define RTS_API_VERSION_TINY	2	/**< Protocol tiny version */
#define RTS_API_VERSION		((RTS_API_VERSION_MAJOR<<16) |\
				 (RTS_API_VERSION_MINOR<<8) |\
				 (RTS_API_VERSION_TINY))

/*API history 
	0.9.1 initial version
	0.9.2 introduced 16bytes additional config space for ALSA device.
		introduced startup_timeout
*/

/*per stream cfg*/
typedef struct {
	const char* pcmname;	/*zero terminated string*/
	snd_pcm_stream_t dir;	/*SND_PCM_STREAM_PLAYBACK*/
	unsigned int rate;	/*any, e.g. 8000-192000*/
	unsigned int period_frames;	/*e.g. 64/128/256/512*/
	snd_pcm_format_t format;/*format*/
	signed short startup_tout;/*tout ms for initial read/write 0=default chosen by RTS, -1=endless */
	char _future_use_[14];	/*must be initialized to ZERO */
}trts_cfgadev;/*ALSA device*/

typedef struct {
	unsigned int adevidx;	/*audio device index in trts_cfg.adevs  (0..num_adevs -1)*/
	unsigned int channel;	/*used channel on given audio device (0..n)*/
}trts_cfgstream;/*user stream*/

typedef struct {
	unsigned int features;	/*e.g. @see RTS_FEAT_...*/
	unsigned int prefill_ms;/*output prefill in ms*/
	unsigned int num_adevs;	/*number of audio devices*/
	trts_cfgadev *adevs;	/*pointer to audio device array. 'num_adevs' entries*/

	unsigned int num_streams;/*number of streams*/
	trts_cfgstream *streams; /*pointer to stream array. 'num_streams' entries*/
}trts_cfg;

#define RTS_FEAT_DISABLE_XRUN_DETECT	0x0001 /*disables XRUN detection*/
#define RTS_FEAT_FORCE_CFG_RELOAD	0x0002 /*useful to change asound.conf at runtime*/
#define RTS_FEAT_MANUAL_START		0x0004 /*do manual start of ALSA devices: DEBUG*/
#define RTS_FEAT_INJECT_XRUN		0x0008 /*alternating xrun on in/output side*/
#define RTS_FEAT_LOG_TSTAMP		0x0010 /*add timestamp to log messages*/
#define RTS_FEAT_LAZY_SETTINGS		0x0020 /*lazy stream settings to suppot streaming on e.g virtualized non RT capable devices*/
#define RTS_FEAT_DISABLE_ENV		0x8000 /*disable feature setting via getenv*/

typedef struct {
	unsigned int clear; /*input: clear after read*/
	unsigned int num_xruns;
}trts_stat;


/*EACH RTS-API except rts_abort() IS NON-REENTRANT WITH RESPECT TO ANY OTHER RTS-API*/
#define rts_create(h, cfg) rts_create_versioned(h, cfg, RTS_API_VERSION)
int rts_create_versioned(tRTS* h, trts_cfg* cfg, unsigned int version);
int rts_destroy(tRTS h);

int rts_read(tRTS h, void* buffer[]);/*all read buffers in order given in config*/
int rts_write(tRTS h, void* buffer[]);/*all write buffers in order given in config*/
int rts_recover(tRTS h);/*Try recover from any error given by rts_read/rts_write*/

int rts_statistic(tRTS h, trts_stat *stat);

/*only allowed asyncronous interface - user must ensure that it does not race with rts_destroy()*/
int rts_abort(tRTS h);

#ifdef __cplusplus
}
#endif

#endif /* __RTS_H__ */
