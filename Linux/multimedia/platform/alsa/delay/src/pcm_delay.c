/**
 * \file: pcm_delay.c
 *
 * Delay plugin for ALSA.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>

#define CHANNELS_MAX 32
#define DELAY_MS_MAX 5000 /*limit to reasonable value*/
typedef struct {
        snd_pcm_uframes_t size;
	snd_pcm_uframes_t offset;
	snd_pcm_channel_area_t area;
}dly_buffer_t;

typedef struct {
        snd_pcm_extplug_t ext;
        unsigned int cfg_delay_ms[CHANNELS_MAX];
	dly_buffer_t *ch;
}snd_pcm_delay_t;


static void do_shift(snd_pcm_uframes_t size, dly_buffer_t *buffer, snd_pcm_format_t format, const snd_pcm_channel_area_t *dly_area,
			const snd_pcm_channel_area_t *dst_area, snd_pcm_uframes_t dst_offset, 
			const snd_pcm_channel_area_t *src_area, snd_pcm_uframes_t src_offset)
{
	snd_pcm_uframes_t cpy = 0;

	if (size > buffer->size) { /*bypass delay buffer*/
		cpy = size - buffer->size;
		snd_pcm_area_copy(dst_area, dst_offset + buffer->size, src_area, src_offset, cpy, format);
		size -= cpy;
	}
	if (!size)
		return;
	src_offset += cpy;
	cpy = buffer->size - buffer->offset;
	if (size < cpy)
		cpy = size;
	snd_pcm_area_copy(dst_area, dst_offset, dly_area, buffer->offset, cpy, format);
	snd_pcm_area_copy(dly_area, buffer->offset, src_area, src_offset, cpy, format);
	buffer->offset += cpy;
	if (buffer->offset == buffer->size)
		buffer->offset = 0;
	size -= cpy;
	if (!size)
		return;
	dst_offset += cpy;
	src_offset += cpy;
	cpy = buffer->size - cpy;
	if (size < cpy)
		cpy = size;
	snd_pcm_area_copy(dst_area, dst_offset, dly_area, buffer->offset, cpy, format);
	snd_pcm_area_copy(dly_area, buffer->offset, src_area, src_offset, cpy, format);
	buffer->offset += cpy;
}

static snd_pcm_sframes_t delay_transfer( snd_pcm_extplug_t *ext,
				const snd_pcm_channel_area_t *dst_areas, snd_pcm_uframes_t dst_offset,
				const snd_pcm_channel_area_t *src_areas, snd_pcm_uframes_t src_offset,
				snd_pcm_uframes_t size)
{
	snd_pcm_delay_t *dly = (snd_pcm_delay_t *)ext;
	unsigned int ch;

	for (ch = 0; ch < ext->channels; ch++)
		do_shift(size, &dly->ch[ch], ext->format, &dly->ch[ch].area, &dst_areas[ch], dst_offset, &src_areas[ch], src_offset);
	return size;
}

static int delay_init(snd_pcm_extplug_t *ext)
{
    snd_pcm_delay_t *dly = (snd_pcm_delay_t *)ext;
	unsigned int ch;

	for (ch = 0; ch < ext->channels && (ch < CHANNELS_MAX); ch++) {
		if (dly->ch[ch].size != 0)
			snd_pcm_format_set_silence(ext->format, dly->ch[ch].area.addr, dly->ch[ch].size);
	}	
	return 0;
}

static int delay_close(snd_pcm_extplug_t *ext)
{
	snd_pcm_delay_t *dly = (snd_pcm_delay_t *)ext;
	free(dly);
	return 0;
}


static int delay_hw_params(snd_pcm_extplug_t *ext, snd_pcm_hw_params_t *params)
{
    snd_pcm_delay_t *dly = (snd_pcm_delay_t *)ext;
	unsigned int ch;
    (void)params;

	if (ext->channels != ext->slave_channels) {
		SNDERR("channels conversion not supported: %d != %d", ext->channels, ext->slave_channels);
		return -EINVAL;
	}
	if (ext->format != ext->slave_format) {
		SNDERR("format conversion not supported: %d != %d", ext->format, ext->slave_format);
		return -EINVAL;
	}
	dly->ch = calloc(ext->channels, sizeof(dly_buffer_t));
	if (dly->ch == NULL) {
		SNDERR("alloc channels failed");
		return -ENOMEM;
	}

	for (ch = 0; ch < ext->channels && (ch < CHANNELS_MAX); ch++) {
		if ((dly->ch[ch].size = (long)dly->cfg_delay_ms[ch] * ext->rate / 1000) != 0){
			dly->ch[ch].area.step = snd_pcm_format_physical_width(ext->format);
			dly->ch[ch].area.addr = malloc((dly->ch[ch].area.step/8) * dly->ch[ch].size);
			dly->ch[ch].area.first = 0;
			if (dly->ch[ch].area.addr == NULL)
				return -ENOMEM;
		}
	}	
	return 0;

}

static int delay_hw_free(snd_pcm_extplug_t *ext)
{
	snd_pcm_delay_t *dly = (snd_pcm_delay_t *)ext;
	unsigned int ch;

	if (dly->ch) {
		for (ch = 0; ch < ext->channels; ch++) {
			if (dly->ch[ch].area.addr) {
				free(dly->ch[ch].area.addr);
				dly->ch[ch].area.addr = NULL;
			}
		}
		free(dly->ch);
		dly->ch = NULL;
	}
    return 0;
}

static void delay_dump(snd_pcm_extplug_t *ext, snd_output_t *out)
{
    snd_pcm_delay_t *dly = (snd_pcm_delay_t *)ext;
    unsigned int ch;
    snd_output_printf(out, "Delay Plugin with settings:\n");

	for (ch = 0; ch < ext->channels; ch++)
        snd_output_printf(out, "CH %2d -> delay %4d ms\n",ch, dly->cfg_delay_ms[ch]);

}

static const snd_pcm_extplug_callback_t delay_callback = {
        .transfer = delay_transfer,
        .hw_params = delay_hw_params,
        .hw_free = delay_hw_free,
        .dump = delay_dump,
        .init = delay_init,
        .close = delay_close,
};

/*!  pcm_plugins

\section Plugin: Delay

General:
This plugin delays a stream.
Each channel can be separately delayed by a configurable amount of ms.

Internal:
The delay is implemented by inserting silence at the beginning of the stream, and delaying
the real samples by routing them through an internal FIFO with size equal to the configured delay.
The effective start of the slave is not delayed.
As neither the plugin user, nor the slave PCM has knowledge about that buffering,
this plugin has following influence on the stream end: 
On drain or drop, samples with size equal to the configured delay are cut off at the end of the stream.

\code
pcm.name {
	type delay              # Delay PCM
        slave STR               # Slave name
        # or
        slave {                 # Slave definition
                pcm STR         # Slave PCM name
                # or
                pcm { }         # Slave PCM definition
        }
	delay {
		0 10		# delay in ms for channel 0
		1 15		# delay in ms for channel 1
				# not configured channels evaluate to delay of 0 ms 
		3 5		# delay in ms for channel 3
	}
}

\endcode

*/

/**
 * \brief Creates a new delay PCM
 * \param pcmp Returns created PCM handle
 * \param name Name of PCM
 * \param root Root configuration node
 * \param conf Configuration node with delay PCM description
 * \param stream Stream type
 * \param mode Stream mode
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
SND_PCM_PLUGIN_DEFINE_FUNC(delay)
{
        snd_config_iterator_t i, next;
        snd_pcm_delay_t *dly;
        snd_config_t *sconf = NULL;
        int err;
	const snd_pcm_format_t formats[] = {
		SND_PCM_FORMAT_S8,SND_PCM_FORMAT_U8,
		SND_PCM_FORMAT_S16_LE, SND_PCM_FORMAT_S16_BE, SND_PCM_FORMAT_U16_LE, SND_PCM_FORMAT_U16_BE,
		SND_PCM_FORMAT_S32_LE, SND_PCM_FORMAT_S32_BE, SND_PCM_FORMAT_U32_LE, SND_PCM_FORMAT_U32_BE
 	};

        dly = calloc(1, sizeof(snd_pcm_delay_t));
        if (!dly)
                return -ENOMEM;

        snd_config_for_each(i, next, conf) {
                snd_config_t *n = snd_config_iterator_entry(i);
                const char *id;
                if (snd_config_get_id(n, &id) < 0)
                        continue;
		if ((strcmp(id, "comment") == 0) || (strcmp(id, "type") == 0) || (strcmp(id, "hint") == 0))
			continue;
                if (strcmp(id, "delay") == 0) {
			snd_config_iterator_t j, nextj;
			snd_config_for_each(j, nextj, n) {
				snd_config_t *e = snd_config_iterator_entry(j);
				int ch;
				long val;
				if (snd_config_get_id(e, &id) < 0)
					continue;
				ch = atoi(id);
				if (ch < 0 || ch >= CHANNELS_MAX) {
					SNDERR("Invalid channel number %s", id);
					free(dly);
					return -EINVAL;
				}
				if (snd_config_get_integer(e, &val) < 0) {
					SNDERR("Delay must be positive number");
					free(dly);
					return -EINVAL;
				}
				if (val < 0) {
					SNDERR("Negative delay");
					free(dly);
					return -EINVAL;
				}
				if (val > DELAY_MS_MAX) {
					SNDERR("delay exceeds limit of %dms", DELAY_MS_MAX);
					free(dly);
					return -EINVAL;
				}
				dly->cfg_delay_ms[ch] = val;
			}
                        continue;
                }
                if (strcmp(id, "slave") == 0) {
                        sconf = n;
                        continue;
                }
                SNDERR("Unknown field %s", id);
		free(dly);
                return -EINVAL;
        }

        if (!sconf) {
                SNDERR("No slave configuration for delay pcm");
		free(dly);
                return -EINVAL;
        }

	dly->ext.name = "Delay Plugin";
	dly->ext.version = SND_PCM_EXTPLUG_VERSION;
	dly->ext.callback = &delay_callback;
	dly->ext.private_data = dly;

	err = snd_pcm_extplug_create(&dly->ext, name, root, sconf, stream, mode);
	if (err < 0) {
		SNDERR("Create extplug failed with %d", err);
		free(dly);
		return err;
	}

	snd_pcm_extplug_set_param_list(&dly->ext, SND_PCM_EXTPLUG_HW_FORMAT, sizeof(formats)/sizeof(snd_pcm_format_t), (const unsigned int*)formats);
#ifdef PCM_DELAY_WITHOUT_HF_027_ALSA_MISC
	snd_pcm_extplug_set_slave_param_list(&dly->ext, SND_PCM_EXTPLUG_HW_FORMAT, sizeof(formats)/sizeof(snd_pcm_format_t), (const unsigned int*)formats);
#endif

	*pcmp = dly->ext.pcm;
	return 0;
}
/*PRQA: Lint Message 19 : This is mandatory for ALSA lib plugins */
/*lint -save -e19 */
SND_PCM_PLUGIN_SYMBOL(delay);
/*lint -restore */
