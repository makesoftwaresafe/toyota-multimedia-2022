/*
 *  ADIT Rate converter plugin
 * 
 *  Copyright (c) 2013 by ADIT 
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
 */

#include <inttypes.h>
#include <dlfcn.h>
#include <alsa/asoundlib.h>
#include "pcm_rate.h"
#include "rate_core_if.h"

#define RATE_ADIT_MIN 8000
#define RATE_ADIT_MAX 192000

struct pcm_rate_adit_swsrc {
	void  *dlobj;
	void *src_obj;
	struct adit_swsrc_core_ops *ops;
};

static snd_pcm_uframes_t pcm_rate_adit_swsrc_input_frames(void *obj, snd_pcm_uframes_t frames)
{
	struct pcm_rate_adit_swsrc *rate = obj;
	return rate->ops->in_frames(rate->src_obj, frames);
}

static snd_pcm_uframes_t pcm_rate_adit_swsrc_output_frames(void *obj, snd_pcm_uframes_t frames)
{
	struct pcm_rate_adit_swsrc *rate = obj;
	return rate->ops->out_frames(rate->src_obj, frames);
}

static void pcm_rate_adit_swsrc_convert_s16(void *obj, int16_t *dst, unsigned int dst_frames, const int16_t *src, unsigned int src_frames)
{
	struct pcm_rate_adit_swsrc *rate = obj;
	struct rate_core_convert_info info = {
			.in = {.buf = (int16_t *)src, .frames = src_frames},
			.out = {.buf = dst, .frames = dst_frames} };
	rate->ops->convert_s16(rate->src_obj, &info);
}

static void pcm_rate_adit_swsrc_free(void *obj)
{
	obj = obj;
}

static int pcm_rate_adit_swsrc_init(void *obj, snd_pcm_rate_info_t *info)
{
	struct pcm_rate_adit_swsrc *rate = obj;
	struct rate_core_init_info core_info = {
			.in = {.freq = info->in.rate, .period_size = info->in.period_size},
			.out = {.freq = info->out.rate, .period_size = info->out.period_size},
			.channels = info->channels};
	return rate->ops->init(rate->src_obj, &core_info);
}

static void pcm_rate_adit_swsrc_reset(void *obj)
{
	obj = obj;
}

static void pcm_rate_adit_swsrc_close(void *obj)
{
	struct pcm_rate_adit_swsrc *rate = obj;
	rate->ops->close(rate->src_obj);
	dlclose(rate->dlobj);
	free(obj);
}

static int pcm_rate_adit_swsrc_get_supported_rates(ATTRIBUTE_UNUSED void *rate,
			       unsigned int *rate_min, unsigned int *rate_max)
{
	rate = rate;
	*rate_min = RATE_ADIT_MIN;
	*rate_max = RATE_ADIT_MAX;
	return 0;
}

static void pcm_rate_adit_swsrc_dump(void *obj, snd_output_t *out)
{
#define CORE_INFO_LEN 64
	char buf[CORE_INFO_LEN];
	struct pcm_rate_adit_swsrc *rate = obj;
	rate->ops->info(rate->src_obj, buf, CORE_INFO_LEN);
	snd_output_printf(out, "Converter: %s\n", buf);
}


static const snd_pcm_rate_ops_t pcm_rate_adit_swsrc_ops = {
	.close = pcm_rate_adit_swsrc_close,
	.init = pcm_rate_adit_swsrc_init,
	.free = pcm_rate_adit_swsrc_free,
	.reset = pcm_rate_adit_swsrc_reset,
	.convert_s16 = pcm_rate_adit_swsrc_convert_s16,
	.input_frames = pcm_rate_adit_swsrc_input_frames,
	.output_frames = pcm_rate_adit_swsrc_output_frames,
	.version = SND_PCM_RATE_PLUGIN_VERSION,
	.get_supported_rates = pcm_rate_adit_swsrc_get_supported_rates,
	.dump = pcm_rate_adit_swsrc_dump,
};


int SND_PCM_RATE_PLUGIN_ENTRY(adit_swsrc) (ATTRIBUTE_UNUSED unsigned int version,
				       void **objp, snd_pcm_rate_ops_t *ops)
{
#define SRC_CORE_LIB_NAME_STRING(name) name
#define SRC_CORE_OPEN_FUNC_STRING(name) __STRING(name)

	struct pcm_rate_adit_swsrc *rate;
	char open_name[64], lib_name[128];
	adit_swsrc_core_open_func_t open_func;
	int err;

	version = version;
	rate = calloc(1, sizeof(*rate));
	if (!rate){
		SNDERR("alloc failed");
		return -ENOMEM;
	}
	snprintf(open_name, sizeof(open_name), SRC_CORE_OPEN_FUNC_STRING(ADIT_SWSRC_CORE_ENTRY));
	snprintf(lib_name, sizeof(lib_name), "%s", SRC_CORE_LIB_NAME_STRING(ADIT_SWSRC_LIB_NAME));
	rate->dlobj = dlopen(lib_name, RTLD_NOW);
	if (!rate->dlobj) {
		SNDERR("open %s failed",lib_name);
		SNDERR("dlerr: %s ",dlerror());
		free(rate);
		return -ENOENT;
	}
	open_func = dlsym(rate->dlobj, open_name);
	if (open_func == NULL) {
		SNDERR("symbol %s is not defined inside %s", open_name, lib_name);
		dlclose(rate->dlobj);
		free(rate);
		return -ENOENT;
	}
	err = open_func(ADIT_RATE_CORE_IF_VERSION, &rate->src_obj, &rate->ops);
	if (err) {
		SNDERR("open failed with err %d", err);
		dlclose(rate->dlobj);
		free(rate);
		return -ENOENT;
	}
	rate->ops->set_log(rate->src_obj, 1, snd_lib_error);
	/*use error channel also for debug*/
	rate->ops->set_log(rate->src_obj, 0, snd_lib_error);
	
	*objp = rate;
	*ops = pcm_rate_adit_swsrc_ops;
	return 0;
}

