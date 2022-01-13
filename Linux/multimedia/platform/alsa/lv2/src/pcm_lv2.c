/**
 * \file: pcm_lv2.c
 *
 * LV2 plugin for ALSA.
 *
 * author: Pavanashree krishnappa / ADIT / pavanashree.krishnappa@in.bosch.com
 *
 * copyright (c) 2017 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include <lilv/lilv.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>

#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"

/*TODO:
-uri-map and uri-unmap requires be completely handled with full atom port support
 uri-mapping is just dummy implementation without full functionality
-Atom port buffer is dummy buffer which is being used only for connect_port
*/

#define LV2_URID_URI   "http://lv2plug.in/ns/ext/urid"
#define LV2_URID_PREFIX   LV2_URID_URI "#"
#define LV2_URID__map   LV2_URID_PREFIX "map"
#define LV2_URID_MAP_URI   LV2_URID__map

/* maximal size of lv2 plugin name + 1 + maximal size of lv2 port symbol name + 1 (for trailing zero) */
#define LV2_NAME_MAX 256
/* maximum channel chosen to reasonable value, can be changed if needed */
#define CHANNELS_MAX 32
/* maximum control supported */
#define CONTROLS_MAX 32
/* default control, if control exceeds maximum controls supported */
#define DEFAULT_CONTROL 30
/* default atom buffer size */
#define ATOM_BUF_SIZE 1024
/* maximum symbol length alsamixer handles (SNDRV_CTL_ELEM_ID_NAME_MAXLEN - not exported) */
#define MAX_SYMBOL_LENGTH 44
/*limiting the plugin name to reasonable value of 8 character*/
#define LV2_ALSA_CTRL_PLUGNAME_MAX 10
/*delimiter between lv2 plugin name and its control name*/
#define DELIMITER "_"

struct lv2_atom_buf_Impl {
	unsigned int       capacity;
	unsigned int       atom_Chunk;
	unsigned int       atom_Sequence;
	LV2_Atom_Sequence  atom_buf;
};
typedef struct lv2_atom_buf_Impl lv2_atom_buf;

typedef snd_pcm_uframes_t (*lv2_data_transfer)(snd_pcm_extplug_t *ext,
		const snd_pcm_channel_area_t *dst_areas, snd_pcm_uframes_t dst_offset,
		const snd_pcm_channel_area_t *src_areas, snd_pcm_uframes_t src_offset,
		snd_pcm_uframes_t size);

struct plugin_ctl {
	snd_ctl_t *ctl;
	snd_ctl_elem_value_t *elem;
};

struct lv2_ctrl_ports {
	int port_in;
	int port_out;
	float val_in;
	float val_out;
	unsigned int factor;
};

typedef struct {
    snd_pcm_channel_area_t *areas;
    lv2_atom_buf* atom_buf[CHANNELS_MAX];
    unsigned int port;
    unsigned int ctrl;
    unsigned int default_ctrl;
    unsigned int atom_p;
    unsigned int atom_cnt;
}sample;

typedef struct {
	snd_pcm_extplug_t extp;
	LilvWorld* lilvworld;
	const LilvPlugin* plugin;
	LilvInstance* instances[CHANNELS_MAX];
	sample in, out;
	struct plugin_ctl *plugctl;
	struct lv2_ctrl_ports *lv2ctl;
	lv2_data_transfer Xfer;
	LV2_URID_Map	map;
	LilvNode* atom_Chunk;
	LilvNode* atom_Sequence;
	size_t buf_size;
	unsigned int map_id;
}snd_pcm_lv2_t;

LV2_Feature map_feature = { LV2_URID__map, NULL };

const LV2_Feature* features[2] = {&map_feature,	NULL};

lv2_atom_buf* lv2_atom_buf_new(unsigned int capacity, unsigned int atom_Chunk, unsigned int atom_Sequence)
{
	lv2_atom_buf* atom_buf = (lv2_atom_buf*)malloc(sizeof(lv2_atom_buf) + sizeof(LV2_Atom_Sequence) + capacity);
	if(atom_buf != NULL){
		atom_buf->capacity      = capacity;
		atom_buf->atom_Chunk    = atom_Chunk;
		atom_buf->atom_Sequence = atom_Sequence;
		atom_buf->atom_buf.atom.size = sizeof(LV2_Atom_Sequence_Body);
		atom_buf->atom_buf.atom.type = atom_buf->atom_Sequence;
		return atom_buf;
	}
	else{
		SNDERR("Atom buffer not created");
		return NULL;
	}
}

/* uri-mapping is just dummy implementation without full functionality, Returning consecutive id for each uri */
static LV2_URID map_uri(LV2_URID_Map_Handle handle, const char* uri)
{
	/* FIXME: map_uri should be handled with complete functionality for full atom port support */
	snd_pcm_lv2_t* lv2 = (snd_pcm_lv2_t*)handle;
	(void)uri;
	return lv2->map_id++;
}

/* Return true if lv2 supports the given feature */
static bool feature_is_supported(const char* uri)
{
	if (uri != NULL) {
		if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#isLive"))
			return true;

		for (const LV2_Feature* const* f = features; *f; ++f) {
			if (!strcmp(uri, (*f)->URI))
				return true;
		}
	}
	return false;
}

static float get_starting_value(float dflt, float min, float max)
{
	if (!isnan(dflt)) {
		return dflt;
	}
	else {
		if (isnan(min)) {
			if (isnan(max)) {
				return 0;
			}
			else {
				return fmin(max, 0);
			}
		}
		else {
			if (isnan(max)) {
				return fmax(min, 0);
			}
			else {
				return (min+max)/2;
			}
		}
	}
}

static void update_ctrls(snd_pcm_lv2_t *lv2)
{
	unsigned int port;
	if(lv2->in.ctrl > CONTROLS_MAX) {
		lv2->in.default_ctrl = DEFAULT_CONTROL;
	} else {
		lv2->in.default_ctrl = lv2->in.ctrl;
	}
	for (port = 0; port < lv2->in.default_ctrl; port++) {
		int err = snd_ctl_elem_read(lv2->plugctl[port].ctl, lv2->plugctl[port].elem);
		if (err < 0) {
			SNDERR("ELEM READ FAILED: %d", err);
			return;
		}
		lv2->lv2ctl[port].val_in = snd_ctl_elem_value_get_integer(lv2->plugctl[port].elem, 0) / (float)lv2->lv2ctl[port].factor;
	}
}

static int get_card(snd_pcm_t *pcm)
{
	snd_pcm_info_t *info;
	snd_pcm_info_alloca(&info);
	int err;
	if ((err = snd_pcm_info(pcm, info)) < 0)
		return err;
	if ((err = snd_pcm_info_get_card(info)) < 0) {
		SNDERR("No card defined for control");
		return -EINVAL;
	}
	return err;
}

static int prepare_control(snd_pcm_lv2_t *lv2)
{
	unsigned int audio_in_cnt = 0;
	unsigned int audio_out_cnt = 0;
	unsigned int port;
	int ret = 0;
	lv2->in.ctrl = 0;
	lv2->out.ctrl = 0;
	LilvNode* min_size = NULL;
	lv2->in.atom_cnt = 0;
	lv2->out.atom_cnt = 0;
	lv2->lv2ctl = NULL;
	unsigned int numports = lilv_plugin_get_num_ports(lv2->plugin);
	lv2->lv2ctl = (struct lv2_ctrl_ports*)malloc(numports * sizeof(struct lv2_ctrl_ports));
	if (lv2->lv2ctl == NULL) {
		SNDERR("Allocation of memory failed for control ports");
		return -EAGAIN;
	}

	LilvNode* input_class = lilv_new_uri(lv2->lilvworld, LILV_URI_INPUT_PORT);
	LilvNode* control_class = lilv_new_uri(lv2->lilvworld, LILV_URI_CONTROL_PORT);
	LilvNode* audio_class = lilv_new_uri(lv2->lilvworld, LILV_URI_AUDIO_PORT);
	LilvNode* output_class = lilv_new_uri(lv2->lilvworld, LILV_URI_OUTPUT_PORT);
	LilvNode* atom_class = lilv_new_uri(lv2->lilvworld, LILV_URI_ATOM_PORT);
	LilvNode* rsz_minimumSize = lilv_new_uri(lv2->lilvworld, LV2_RESIZE_PORT__minimumSize);
	LilvNode* optional = lilv_new_uri(lv2->lilvworld, LILV_NS_LV2 "connectionOptional");

	for (port = 0; port < numports; port++) {
		const LilvPort* p=lilv_plugin_get_port_by_index(lv2->plugin, port);

		if (lilv_port_is_a(lv2->plugin, p, input_class) && lilv_port_is_a(lv2->plugin, p, audio_class)) {
			audio_in_cnt++;
			lv2->in.port = port;
		}
		else if (lilv_port_is_a(lv2->plugin, p, output_class) && lilv_port_is_a(lv2->plugin, p, audio_class)) {
			audio_out_cnt++;
			lv2->out.port = port;
		}
		else if (lilv_port_is_a(lv2->plugin, p, input_class) && lilv_port_is_a(lv2->plugin, p, control_class)) {
			(lv2->lv2ctl[lv2->in.ctrl].port_in) = port;
			lv2->in.ctrl++;
		}
		else if (lilv_port_is_a(lv2->plugin, p, output_class) && lilv_port_is_a(lv2->plugin, p, control_class)) {
			(lv2->lv2ctl[lv2->out.ctrl].port_out) = port;
			lv2->out.ctrl++;
		}
		else if (lilv_port_is_a(lv2->plugin, p, input_class) && lilv_port_is_a(lv2->plugin, p, atom_class)) {
			lv2->in.atom_p = port;
			lv2->in.atom_cnt++;
				}
		else if (lilv_port_is_a(lv2->plugin, p, output_class) && lilv_port_is_a(lv2->plugin, p, atom_class)) {
			lv2->out.atom_p = port;
			lv2->out.atom_cnt++;
				}
		else if (!lilv_port_has_property(lv2->plugin, p,  optional)) {
			SNDERR("Error!  Unable to handle a required port");
			ret = -EINVAL;
			break;
		}

		if (audio_in_cnt > 1 || audio_out_cnt > 1) {
			SNDERR("Multiple ports are not supported");
			ret = -EINVAL;
			break;
		}
		if(rsz_minimumSize != NULL) {
		min_size = lilv_port_get(lv2->plugin, p, rsz_minimumSize);
		if (min_size && lilv_node_is_int(min_size)) {
			lv2->buf_size = lilv_node_as_int(min_size);
		}
		lilv_node_free(min_size);
		}
	}

	lilv_node_free (audio_class);
	lilv_node_free (input_class);
	lilv_node_free (control_class);
	lilv_node_free (output_class);
	lilv_node_free (atom_class);
	lilv_node_free (rsz_minimumSize);
	lilv_node_free (optional);

	return ret;
}

static int load_control(snd_pcm_t *pcm, int ctl_card, snd_pcm_lv2_t *lv2)
{
	snd_ctl_elem_id_t *ctl_id;
	snd_ctl_elem_info_t *cinfo;
	int err = 0;
	unsigned int port;
	int range_min = 0;
	int range_max = 0;
	const char* symbol_name;
	char tmp_name[32];
	char ctlname[LV2_NAME_MAX];
	unsigned int numports = lilv_plugin_get_num_ports(lv2->plugin);
	float minvalues[numports];
	float maxvalues[numports];
	float defaultvalues[numports];
	unsigned int port_idx = 0;
	lv2->plugctl = NULL;
	const char* const lv2name = lilv_node_as_string(lilv_plugin_get_name(lv2->plugin));
	lilv_plugin_get_port_ranges_float(lv2->plugin, minvalues, maxvalues, defaultvalues);

	/*get responsible card and open ctl*/
	if (ctl_card < 0 ) {
		ctl_card = get_card(pcm);
		if (ctl_card < 0)
			return ctl_card;
	}
	sprintf(tmp_name, "hw:%d", ctl_card);

	lv2->plugctl = (struct plugin_ctl*)calloc(lv2->in.ctrl, sizeof(struct plugin_ctl));
	if(lv2->plugctl == NULL)
	{
		SNDERR("creation of memory failed for plugin control");
		return -EAGAIN;
	}

	if(lv2->in.ctrl >= CONTROLS_MAX) {
		SNDERR("All controls are not registered");
		lv2->in.default_ctrl = DEFAULT_CONTROL;
	} else {
		lv2->in.default_ctrl = lv2->in.ctrl;
	}

	for (port = 0; port < lv2->in.default_ctrl; port++) {
		memset(ctlname, 0, sizeof(ctlname));
		range_min = 0;
		range_max = 0;
		lv2->lv2ctl[port].factor = 1;
		symbol_name = lilv_node_as_string(lilv_port_get_symbol(lv2->plugin, lilv_plugin_get_port_by_index(lv2->plugin, (lv2->lv2ctl[port].port_in))));
		strncat(ctlname, lv2name, LV2_ALSA_CTRL_PLUGNAME_MAX);
		strncat(ctlname, DELIMITER, MAX_SYMBOL_LENGTH - strlen(ctlname));
		strncat(ctlname, symbol_name, (MAX_SYMBOL_LENGTH - strlen(ctlname)) - 1);

		port_idx = lv2->lv2ctl[port].port_in;
		lv2->lv2ctl[port].val_in = get_starting_value(defaultvalues[port_idx], minvalues[port_idx], maxvalues[port_idx]);
		range_min = (int)minvalues[port_idx];
		if (maxvalues[port_idx] == 1)
			lv2->lv2ctl[port].factor = 1000;
		else if (maxvalues[port_idx] <= 10)
			lv2->lv2ctl[port].factor = 100;
		else if (maxvalues[port_idx] <= 100)
			lv2->lv2ctl[port].factor = 10;
		range_max = (int)maxvalues[port_idx] * (int)lv2->lv2ctl[port].factor;		//to match the alsa range

	reopen:
		err = snd_ctl_open(&lv2->plugctl[port].ctl, tmp_name, 0);
		if (err < 0) {
			SNDERR("Cannot open CTL %s", tmp_name);
			return err;
		}
		snd_ctl_elem_id_alloca(&ctl_id);
		snd_ctl_elem_id_set_numid(ctl_id, 0);
		snd_ctl_elem_id_set_interface(ctl_id, SND_CTL_ELEM_IFACE_MIXER);
		snd_ctl_elem_id_set_device(ctl_id, 0);
		snd_ctl_elem_id_set_subdevice(ctl_id, 0);
		snd_ctl_elem_id_set_name(ctl_id, ctlname);
		snd_ctl_elem_id_set_index(ctl_id, 0);
		snd_ctl_elem_info_alloca(&cinfo);
		snd_ctl_elem_info_set_id(cinfo, ctl_id);
		snd_ctl_elem_value_malloc(&lv2->plugctl[port].elem);
		snd_ctl_elem_value_set_id(lv2->plugctl[port].elem, ctl_id);

		if ((err = snd_ctl_elem_info(lv2->plugctl[port].ctl, cinfo)) < 0) {
			if (err != -ENOENT) {
				SNDERR("Cannot get info for CTL %s", tmp_name);
				return err;
			}
			err = snd_ctl_elem_add_integer(lv2->plugctl[port].ctl, ctl_id, 1, range_min, range_max, 1);
			if (err < 0) {
				SNDERR("ELEM ADD INTEGER FAILED");
				return err;
			}
			snd_ctl_elem_value_set_integer(lv2->plugctl[port].elem, 0, (long)(lv2->lv2ctl[port].val_in));
			err = snd_ctl_elem_write(lv2->plugctl[port].ctl, lv2->plugctl[port].elem);
			if (err < 0) {
				SNDERR("Cannot add a control");
				return err;
			}
			/*new elements are locked in kernel - reopen it to make them immediately accessible by others...*/
			snd_ctl_close(lv2->plugctl[port].ctl);
			goto reopen;
		}
		else {
			err = snd_ctl_elem_read(lv2->plugctl[port].ctl, lv2->plugctl[port].elem);
			if (err < 0) {
				SNDERR("Cannot add a control");
				return err;
			}
		}
	}
	return 0;
}

static snd_pcm_uframes_t interleaved_transfer(snd_pcm_extplug_t *ext,
		const snd_pcm_channel_area_t *dst_areas, snd_pcm_uframes_t dst_offset,
		const snd_pcm_channel_area_t *src_areas, snd_pcm_uframes_t src_offset,
		snd_pcm_uframes_t size)
{
	snd_pcm_lv2_t *lv2 = (snd_pcm_lv2_t*)ext;
	unsigned int ch;
	snd_pcm_areas_copy(lv2->in.areas, 0, src_areas, src_offset, ext->channels, size, ext->format);
	for (ch = 0; ch < ext->channels; ch++) {
		lilv_instance_connect_port(lv2->instances[ch], lv2->in.port, lv2->in.areas[ch].addr);
		lilv_instance_connect_port(lv2->instances[ch], lv2->out.port, lv2->out.areas[ch].addr);
		lilv_instance_run(lv2->instances[ch], size);
	}
	snd_pcm_areas_copy(dst_areas, dst_offset, lv2->out.areas, 0, ext->channels, size, ext->format);
	return size;
}

static snd_pcm_uframes_t non_interleaved_transfer(snd_pcm_extplug_t *ext,
		const snd_pcm_channel_area_t *dst_areas, snd_pcm_uframes_t dst_offset,
		const snd_pcm_channel_area_t *src_areas, snd_pcm_uframes_t src_offset,
		snd_pcm_uframes_t size)
{
	snd_pcm_lv2_t *lv2 = (snd_pcm_lv2_t*)ext;
	unsigned int ch;
	for (ch = 0; ch < ext->channels; ch++) {
		lilv_instance_connect_port(lv2->instances[ch], lv2->in.port, (char *)src_areas[ch].addr + src_offset);
		lilv_instance_connect_port(lv2->instances[ch], lv2->out.port, (char *)dst_areas[ch].addr + dst_offset);
		lilv_instance_run(lv2->instances[ch], size);
	}
	return size;
}

static snd_pcm_sframes_t lv2_transfer(snd_pcm_extplug_t *ext,
				const snd_pcm_channel_area_t *dst_areas, snd_pcm_uframes_t dst_offset,
				const snd_pcm_channel_area_t *src_areas, snd_pcm_uframes_t src_offset,
				snd_pcm_uframes_t size)
{
	snd_pcm_lv2_t *lv2 = (snd_pcm_lv2_t*)ext;
	update_ctrls(lv2);
	return (*lv2->Xfer)(ext, dst_areas, dst_offset, src_areas, src_offset, size);
}


static int lv2_scan(snd_pcm_lv2_t *lv2, const char *plugin_uri)
{
	int err;
	lv2->lilvworld = lilv_world_new();
	if (lv2->lilvworld == NULL) {
		SNDERR("lilv creation failed");
		return -EAGAIN;
	}
	lilv_world_load_all(lv2->lilvworld);
	const LilvPlugins* plugins =lilv_world_get_all_plugins(lv2->lilvworld);
	LilvNode* uri = lilv_new_uri(lv2->lilvworld, plugin_uri);
	lv2->plugin = lilv_plugins_get_by_uri(plugins, uri);

	if (lv2->plugin == NULL) {
		SNDERR("plugin listed from URI is not available. \nPlease check the plugin URI or the lv2 package once");
		lilv_node_free(uri);
		return -ENOENT;
	}
	err = prepare_control(lv2);
	if (err < 0) {
		SNDERR("control preparation failed");
		lilv_node_free(uri);
		return err;
	}
	lilv_node_free(uri);
	return 0;
}

static int lv2_hw_params(snd_pcm_extplug_t *ext, snd_pcm_hw_params_t *params)
{
	snd_pcm_lv2_t *lv2 = (snd_pcm_lv2_t*)ext;
	snd_pcm_access_t access_type;
	unsigned int i, ch;
	int dir;
	snd_pcm_uframes_t frames;
	lv2->in.areas = NULL;
	lv2->out.areas = NULL;
	memset(lv2->in.atom_buf, 0, CHANNELS_MAX * sizeof(lv2->in.atom_buf[0]));
	memset(lv2->out.atom_buf, 0, CHANNELS_MAX * sizeof(lv2->out.atom_buf[0]));

	lv2->map.handle  = &lv2;
	lv2->map.map     = map_uri;
	map_feature.data = &lv2->map;

	/* Check that any required features are supported */
	LilvNodes* req_feats = lilv_plugin_get_required_features(lv2->plugin);
	if(req_feats != NULL) {
		LILV_FOREACH(nodes, f, req_feats) {
			const char* uri = lilv_node_as_uri(lilv_nodes_get(req_feats, f));
			if (!feature_is_supported(uri)) {
				SNDERR("Feature %s is not supported", uri);
				lilv_nodes_free(req_feats);
				return -EINVAL;
			}
		}
		lilv_nodes_free(req_feats);
	}

	snd_pcm_hw_params_get_access(params, &access_type);
	switch (access_type) {
		case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
		case SND_PCM_ACCESS_RW_NONINTERLEAVED:
			lv2->Xfer = non_interleaved_transfer;
			break;

		case SND_PCM_ACCESS_MMAP_INTERLEAVED:
		case SND_PCM_ACCESS_RW_INTERLEAVED:
			lv2->Xfer = interleaved_transfer;
			break;
		default:
			return -EINVAL;
	}
	for (ch = 0; ch < ext->channels; ch++) {
		lv2->instances[ch]=lilv_plugin_instantiate(lv2->plugin, ext->rate, features);
		lilv_instance_activate(lv2->instances[ch]);
	}
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	size_t sample_bits = snd_pcm_format_physical_width(ext->format);

	lv2->in.areas = (snd_pcm_channel_area_t*)malloc(sizeof(snd_pcm_channel_area_t) * ext->channels);
	lv2->out.areas = (snd_pcm_channel_area_t*)malloc(sizeof(snd_pcm_channel_area_t) * ext->channels);

	if (lv2->in.areas == NULL || lv2->out.areas == NULL) {
		SNDERR("PCM area memory allocation failed");
		ext->callback->hw_free(ext);
		return -ENOMEM;
	} else {
		lv2->in.areas[0].addr = malloc(frames * (sample_bits / 8) * ext->channels);
		lv2->out.areas[0].addr = malloc(frames * (sample_bits / 8) * ext->channels);
	}

	if (lv2->in.areas[0].addr == NULL || lv2->out.areas[0].addr == NULL) {
		SNDERR("memory allocation failed in hw_params");
		ext->callback->hw_free(ext);
		return -ENOMEM;
	}
	for (ch = 0; ch < ext->channels; ch++) {
		lv2->in.areas[ch].first = 0;
		lv2->in.areas[ch].step = sample_bits;
		lv2->in.areas[ch].addr = (char *)lv2->in.areas[0].addr+(ch * frames * (sample_bits / 8));

		lv2->out.areas[ch].first = 0;
		lv2->out.areas[ch].step = sample_bits;
		lv2->out.areas[ch].addr = (char *)lv2->out.areas[0].addr+(ch * frames * (sample_bits / 8));
	}
	/* Atom port buffer is dummy buffer, only used for connect_port */
	const size_t buf_size = (lv2->buf_size > 0) ? lv2->buf_size : ATOM_BUF_SIZE;
	if(lv2->in.atom_cnt > 0) {
		for (ch = 0; ch < ext->channels; ch++) {
			lv2->in.atom_buf[ch] = lv2_atom_buf_new(buf_size,
					lv2->map.map(lv2->map.handle, lilv_node_as_string(lv2->atom_Chunk)),
					lv2->map.map(lv2->map.handle, lilv_node_as_string(lv2->atom_Sequence)));
			if(lv2->in.atom_buf[ch] == NULL){
				ext->callback->hw_free(ext);
				return -ENOMEM;
			}
			lilv_instance_connect_port(lv2->instances[ch],lv2->in.atom_p, &lv2->in.atom_buf[ch]->atom_buf);
		}
	}
	if(lv2->out.atom_cnt > 0) {
		for (ch = 0; ch < ext->channels; ch++) {
			lv2->out.atom_buf[ch] = lv2_atom_buf_new(buf_size,
					lv2->map.map(lv2->map.handle, lilv_node_as_string(lv2->atom_Chunk)),
					lv2->map.map(lv2->map.handle, lilv_node_as_string(lv2->atom_Sequence)));
			if(lv2->out.atom_buf[ch] == NULL){
				ext->callback->hw_free(ext);
				return -ENOMEM;
			}
			lilv_instance_connect_port(lv2->instances[ch],lv2->out.atom_p, &lv2->out.atom_buf[ch]->atom_buf);
		}
	}

	for (ch = 0; ch < ext->channels; ch++) {
		for (i = 0; i < lv2->out.ctrl; i++) {
			lilv_instance_connect_port(lv2->instances[ch], (lv2->lv2ctl[i].port_out), &(lv2->lv2ctl[i].val_out));
		}
		for (i = 0; i < lv2->in.ctrl; i++) {
			lilv_instance_connect_port(lv2->instances[ch], (lv2->lv2ctl[i].port_in), &(lv2->lv2ctl[i].val_in));
		}
	}
	return 0;
}

static int lv2_init(snd_pcm_extplug_t *ext)
{
	(void)ext;
	//init call may be called more than once, hence no initialization done.
	return 0;
}

static int lv2_hw_free(snd_pcm_extplug_t *ext)
{
	snd_pcm_lv2_t *lv2 = (snd_pcm_lv2_t*)ext;
	unsigned int ch;
	if (lv2 != NULL) {
		if (lv2->in.areas != NULL) {
			free(lv2->in.areas[0].addr);
			free(lv2->in.areas);
			lv2->in.areas = NULL;
		}
		if (lv2->out.areas != NULL) {
			free(lv2->out.areas[0].addr);
			free(lv2->out.areas);
			lv2->out.areas = NULL;
		}
		for(ch = 0; ch < ext->channels; ch++){
			if (lv2->instances[ch] != NULL) {
				if(lv2->lv2ctl != NULL){
					free(lv2->lv2ctl);
					lv2->lv2ctl = NULL;
				}
				if(lv2->in.atom_buf[ch] != NULL){
					free(lv2->in.atom_buf[ch]);
					lv2->in.atom_buf[ch] = NULL;
				}
				if(lv2->out.atom_buf[ch] != NULL){
					free(lv2->out.atom_buf[ch]);
					lv2->out.atom_buf[ch] = NULL;
				}
				lilv_instance_deactivate(lv2->instances[ch]);
				lilv_instance_free(lv2->instances[ch]);
				lv2->instances[ch] = NULL;
			}
		}
	}
	return 0;
}

static void lv2_dump(snd_pcm_extplug_t *ext, snd_output_t *out)
{
	snd_pcm_lv2_t *lv2 = (snd_pcm_lv2_t*)ext;
	LilvNode *lv2_plugname = lilv_plugin_get_name(lv2->plugin);
	snd_output_printf(out, "URI  		: %s\n", lilv_node_as_uri(lilv_plugin_get_uri(lv2->plugin)));
	snd_output_printf(out, "Name of the plugin  	: %s\n", lilv_node_as_string(lv2_plugname));
	snd_output_printf(out, "Channels  		: %d\n", ext->channels);
	snd_output_printf(out, "Rate  			: %d\n", ext->rate);
	snd_output_printf(out, "Number of audio ports  	: %d\n", lv2->in.port);
	snd_output_printf(out, "Number of control ports : %d\n", lv2->in.ctrl);
	lilv_node_free(lv2_plugname);
}

static int lv2_close(snd_pcm_extplug_t *ext)
{
	snd_pcm_lv2_t *lv2 = (snd_pcm_lv2_t*)ext;
	unsigned int i;
	if (lv2 != NULL) {
		if (lv2->plugctl != NULL) {
			for(i = 0; i < lv2->in.ctrl ; i++) {
				if(lv2->plugctl[i].ctl != NULL)
					snd_ctl_close(lv2->plugctl[i].ctl);
				if(lv2->plugctl[i].elem != NULL)
					snd_ctl_elem_value_free(lv2->plugctl[i].elem);
			}
				free(lv2->plugctl);
		}
		if(lv2->lv2ctl != NULL)
			free(lv2->lv2ctl);
		lilv_world_free(lv2->lilvworld);
		free(lv2);
	}
	return 0;
}

static const snd_pcm_extplug_callback_t lv2_callback = {
	.init = lv2_init,
	.hw_params = lv2_hw_params,
	.transfer = lv2_transfer,
	.hw_free = lv2_hw_free,
	.dump = lv2_dump,
	.close = lv2_close,
};

/*!  pcm_plugins

\section Plugin: LV2

General:
This plugin will provides provision for accesing the LV2 plugins

Internal:
The plugin is implemented by invoking the plugin by uri.
This will allow to invoke the LV2 plugins. Which allows to control its ports.

\code
pcm.name {
        type lv2              		# LV2 PCM
        slave STR               # Slave name
        # or
        slave {                 # Slave definition
                pcm STR         # Slave PCM name
                # or
                pcm { }         # Slave PCM definition
        }
        uri "http://plugin URI" # uri of the plugin, eg: uri "http://lv2plug.in/plugins/eg-amp"
}

\endcode

*/

/**
 * \brief Creates a new lv2 PCM
 * \ below params are mandatory to use. Macro for SND_PCM_PLUGIN_ENTRY
 * \param pcmp Returns created PCM handle
 * \param name Name of PCM
 * \param root Root configuration node
 * \param conf Configuration node with lv2 PCM description
 * \param stream Stream type
 * \param mode Stream mode
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
SND_PCM_PLUGIN_DEFINE_FUNC(lv2)
{
	int card = -1;
	const char *plugin_uri = NULL;
	snd_config_iterator_t i, next;
	snd_config_t *sconf = NULL;
	int err;
	const snd_pcm_format_t formats[] = {
		SND_PCM_FORMAT_FLOAT
	};

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if ((strcmp(id, "type") == 0) || (strcmp(id, "hint") == 0) || (strcmp(id, "comment") == 0))
			continue;
		if (strcmp(id, "slave") == 0) {
			sconf = n;
			continue;
		}
		if (strcmp(id, "uri") == 0) {
			snd_config_get_string(n, &plugin_uri);
			continue;
		}
		SNDERR("Lv2: Unknown field %s", id);
		return -EINVAL;
	}

	if (!sconf) {
		SNDERR("No slave configuration for lv2 pcm");
		return -EINVAL;
	}
	if (plugin_uri == NULL) {
		SNDERR("Please fill the URI. URI should not be empty");
		return -EAGAIN;
	}
	snd_pcm_lv2_t *lv2;
	lv2 = calloc(1, sizeof(snd_pcm_lv2_t));
	if (!lv2)
		return -ENOMEM;
	lv2->extp.name = "LV2 Plugin";
	lv2->extp.version = SND_PCM_EXTPLUG_VERSION;
	lv2->extp.callback = &lv2_callback;
	lv2->extp.private_data = lv2;

	err = snd_pcm_extplug_create(&lv2->extp, name, root, sconf, stream, mode);
	if (err < 0) {
		SNDERR("LV2: Create extplug failed with %d", err);
		free(lv2);
		return err;
	}

	err = snd_pcm_extplug_set_param_minmax(&lv2->extp,
					 SND_PCM_EXTPLUG_HW_CHANNELS,
					 1, CHANNELS_MAX);
	if (err < 0) {
		SNDERR("Set param failed for channels %d", err);
		free(lv2);
		return err;
	}

	err = lv2_scan(lv2, plugin_uri);
	if (err < 0) {
		SNDERR("LV2: scanning Lv2 failed with the error %d", err);
		snd_pcm_extplug_delete(&lv2->extp);
		return err;
	}

	err = load_control(lv2->extp.pcm, card, lv2);
	if (err < 0) {
		SNDERR("Loading the controls failed");
		snd_pcm_extplug_delete(&lv2->extp);
		return err;
	}
	snd_pcm_extplug_set_param_list(&lv2->extp, SND_PCM_EXTPLUG_HW_FORMAT, sizeof(formats) / sizeof(snd_pcm_format_t), (const unsigned int*)formats);
	snd_pcm_extplug_set_slave_param_list(&lv2->extp, SND_PCM_EXTPLUG_HW_FORMAT, sizeof(formats) / sizeof(snd_pcm_format_t), (const unsigned int*)formats);

	*pcmp = lv2->extp.pcm;
	return 0;
}
/*PRQA: Lint Message 19 : This is mandatory for ALSA lib plugins */
/*lint -save -e19 */
SND_PCM_PLUGIN_SYMBOL(lv2);
/*lint -restore */
