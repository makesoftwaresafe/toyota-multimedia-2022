/*
 *  IMX ASRC suppport for ALSA asynchronous rate plugin
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
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

#include <alsa/asoundlib.h>
#include "pcm_arate.h"

#include <linux/mxc_asrc.h>

//#define DEBUG

#define ASRC_MXC_PATH "/dev/mxc_asrc"
#define ASRCERR SNDERR


#ifdef DEBUG 
#define ASRC_TRC(i) snd_output_printf((i)->log,"%s %s\n",__FUNCTION__,(i)->in?"IN ":"OUT")
#else
#define ASRC_TRC(i) (void)(i)
#endif

static size_t round_up_to_page(size_t size)
{
	long psz = sysconf(_SC_PAGE_SIZE); /* page size is a positive number */
	size_t r = size % (unsigned long)psz;
	return r?(size + psz - r):size;
}

#define PAIR_MASK_DEFAULT (0xff)

#define FILTER_INVALID -1 /*enum range*/
#define FIFO_WM_DEFAULT 32 /*uchar range*/

typedef struct asrc_end
{
	int fd;
	unsigned char pair_mask;
	char *clk;
	int filter;
	unsigned char fifo_wm;
	void *buffer; /*mmapped kernel buffer*/
	int buf_size; /*mmapped kernel buffer size*/
	int in;
	char need_slaveclk;
	snd_output_t *log;
} asrc_end_t;


static int parse_cfg( asrc_end_t *info, const snd_config_t *converter_cfg)
{
	snd_config_iterator_t i, next;
	int err;
	const char* entry;

	if (!converter_cfg)
		return 0;

	if (snd_config_get_type(converter_cfg) != SND_CONFIG_TYPE_COMPOUND)
		return 0;

	snd_config_for_each(i, next, converter_cfg) {
		snd_config_t *n = snd_config_iterator_entry(i);

		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (strcmp(id, "in_clk") == 0) {
			if(info->in) {
				snd_config_get_string(n, &entry);
				if (info->clk)
					free(info->clk);
				info->clk = strdup(entry);
			}
			continue;
		}
		if (strcmp(id, "out_clk") == 0) {
			if (!info->in) {
				snd_config_get_string(n, &entry);
				if (info->clk)
					free(info->clk);
				info->clk = strdup(entry);
			}
			continue;
		}
		if (strcmp(id, "filter") == 0) {
			if (info->in) {
				long val;
				err = snd_config_get_integer(n, &val);
				if (err < 0)
					return err;
				/* alsa-lib guaranteed val <= INT_MAX. See conf.c:int parse_value(). */
				info->filter = val;
			}
			continue;
		}
		if (strcmp(id, "in_fifo_wm") == 0) {
			if(info->in) {
				long val;
				err = snd_config_get_integer(n, &val);
				if (err < 0)
					return err;
				if (val > UCHAR_MAX)
					return -EINVAL;
				info->fifo_wm = val;
			}
			continue;
		}
		if (strcmp(id, "out_fifo_wm") == 0) {
			if(!info->in) {
				long val;
				err = snd_config_get_integer(n, &val);
				if (err < 0)
					return err;
				if (val > UCHAR_MAX)
					return -EINVAL;
				info->fifo_wm = val;
			}
			continue;
		}
		if (strcmp(id, "pair_mask") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0)
				return err;
			if (val > UCHAR_MAX)
				return -EINVAL;
			info->pair_mask = val;
			continue;
		}
		if (strcmp(id, "need_slaveclk") == 0) {
			err = snd_config_get_bool(n);
			if (err < 0)
				return err;
			info->need_slaveclk = err;
			continue;
		}

		ASRCERR("Unknown field %s", id);
		return -EINVAL;
	}
	return 0;
}


static int destroy_route_info(asrc_end_t *info)
{
	if (info->clk){
		free(info->clk);
		info->clk = NULL;
	}
	return 0;
}


static int create_cfg( asrc_end_t *info, const snd_config_t *converter_cfg)
{
	/*mandatory: set default*/	
	info->pair_mask = PAIR_MASK_DEFAULT;
	info->fifo_wm = FIFO_WM_DEFAULT;

	/*optional: set invalid*/	
	info->filter = FILTER_INVALID;

	parse_cfg(info, converter_cfg);

	if (!info->clk)
		info->clk = strdup("NONE");
		
	return 0;
}

static int asrc_imx_get_caps(void* ctx, asrc_capabilities_t *caps)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err;
	struct asrc_section_capabilities  comm_caps;
	struct buffer_capabilities buf_caps;

	ASRC_TRC(info);

	if ((err = ioctl(info->fd, ASRC_GET_CAPABILITY, &comm_caps)) < 0) {
		ASRCERR("Failed to get capabilities err %d", errno);
		return -errno;
	}

	if ((err = ioctl(info->fd, ASRC_GET_BUF_CAPABILITY, &buf_caps)) < 0) {
		ASRCERR("Failed to get capabilities err %d", errno);
		return -errno;
	}
	if(info->need_slaveclk)
		caps->features |= ASRC_FEAT_NEED_SLAVECLK;

	caps->formats = comm_caps.formats;
	caps->channels_min = comm_caps.channels_min;
	caps->channels_max = comm_caps.channels_max;

	caps->rate_min = comm_caps.rate_min;
	caps->rate_max = comm_caps.rate_max;

	/*caps->bufbytes_min = buf_caps->bufbytes_min;*/
	caps->bufbytes_max = buf_caps.bufbytes_max;

	caps->periods_min = buf_caps.periods_min;
	caps->periods_max = buf_caps.periods_max;

	caps->periodbytes_min = buf_caps.periodbytes_min;
	caps->periodbytes_max = buf_caps.periodbytes_max;
	caps->fifo_size = info->fifo_wm;
	return 0;
}


static int asrc_imx_configure(void *ctx, struct asrc_io_cfg *asrc_io_cfg)
{
	int err = 0;
	asrc_end_t *info = (asrc_end_t*) ctx;
	struct section_config sect_cfg;
	struct section_buffer_config buf_cfg;

	ASRC_TRC(info);

	buf_cfg.buffer_bytes = asrc_io_cfg->buf_size;
	buf_cfg.periods = asrc_io_cfg->periods;

	if (info->buffer) {
		if ((err = ioctl(info->fd, ASRC_FREE_BUFFER)) < 0) {
			ASRCERR("Failed to free buffer err %d", errno);
			return -errno;
		}
		info->buffer = NULL;
		info->buf_size = 0;
	}

	if ((err = ioctl(info->fd, ASRC_CONFIG_BUFFER, &buf_cfg)) < 0) {
		ASRCERR("Failed to configure buffer err %d", errno);
		return -errno;
	}

	if ((err = ioctl(info->fd, ASRC_SET_CLK_REFERENCE, info->clk)) < 0) {
		ASRCERR("Failed to set clk %s - err %d", info->clk, errno);
		return -errno;
	}
	
	asrc_io_cfg->buffer = mmap(NULL, round_up_to_page(asrc_io_cfg->buf_size), info->in?PROT_WRITE:PROT_READ,
				MAP_SHARED, info->fd, 0);

	if (asrc_io_cfg->buffer == MAP_FAILED) {
		ASRCERR(" mmap error %d", errno);
		return -errno;
	}

	info->buffer = asrc_io_cfg->buffer;
	info->buf_size = asrc_io_cfg->buf_size;

	sect_cfg.rate = asrc_io_cfg->sample_rate;
	sect_cfg.channels = asrc_io_cfg->channels;
	sect_cfg.format = asrc_io_cfg->format;

	if ((err = ioctl(info->fd, ASRC_SETUP_SECTION, &sect_cfg)) < 0) {
		ASRCERR("Failed to configure section err %d", errno);
		return -errno;
	}

	return 0;
}



static int asrc_imx_start(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err;

	ASRC_TRC(info);

	if ((err = ioctl(info->fd, ASRC_START_CONV, 0)) < 0) {
		err = -errno;
		if (err != -EAGAIN) {
			ASRCERR("error starting conversion %d", err);
			return err;
		}
		err = 0;
	}
	return err;
}


static int asrc_imx_prepare(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err = 0;

	ASRC_TRC(info);

	if ((err = ioctl(info->fd, ASRC_SECTION_PREPARE, 0)) < 0) {
		err = -errno;
		if (err != -EBUSY) {
			ASRCERR("error prepare conversion %d", err);
			return err;
		} 
		err = 0;
	}
	return err;
}


static int asrc_imx_stop(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err;

	ASRC_TRC(info);

	if ((err = ioctl(info->fd, ASRC_STOP_CONV, 0)) < 0) {
		err = -errno;
		ASRCERR("error stoping conversion %d", err);
		return err;
	}
	return err;
}

static int asrc_imx_drain(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err = 0;

	ASRC_TRC(info);

#ifdef ASRC_DRAIN_SUPPORT
	if ((err = ioctl(info->fd, ASRC_DRAIN, &info->pair_index)) < 0)
	{
		err = -errno;
		ASRCERR("error drain %d", err);
	}
#endif
	return err;
}


static int asrc_imx_xrun(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	int err = 0;

	ASRC_TRC(info);

	if ((err = ioctl(info->fd, ASRC_SECTION_XRUN, 0)) < 0) {
		err = -errno;
		ASRCERR("error xrun %d", err);
	}
	return err;
}


static int asrc_imx_sync(void* ctx, asrc_sync_t *sync_io)
{
	struct section_sync imx_sync;
	int err;
	asrc_end_t *info = (asrc_end_t*) ctx;

	ASRC_TRC(info);

	imx_sync.appl_pos = sync_io->app_pos; /*IN*/
	imx_sync.avail_min = sync_io->avail_min;/*IN*/
	imx_sync.stop_threshold = sync_io->stop_threshold; /*IN*/
	imx_sync.boundary = sync_io->boundary;/*IN*/
	imx_sync.hw_pos = 0;/*OUT*/
	if ((err = ioctl(info->fd, ASRC_SYNC, &imx_sync)) < 0) {
		err = -errno;
		ASRCERR("ASRC_SYNC error %d", err);
		return err;

	}
	sync_io->hw_pos = imx_sync.hw_pos;
	switch(imx_sync.state) {
		case SECTION_SOFT_XRUN:
		case SECTION_HARD_XRUN:
		case SECTION_IRQ_ERR: //TODO: recoverable ?
		sync_io->state = SND_PCM_STATE_XRUN;
		break;
	}
	return 0;
}


static int asrc_imx_getfd(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	ASRC_TRC(info);
	return info->fd;
}

static int asrc_imx_getfdev(void *ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	ASRC_TRC(info);
	return info->in?POLLOUT : POLLIN;
}

static int asrc_imx_setfdmode(void *ctx, int nonblock)
{
	long flags;
	asrc_end_t *info = (asrc_end_t*) ctx;
	int fd = info->fd;
	int err;
	ASRC_TRC(info);

	if ((flags = fcntl(fd, F_GETFL)) < 0) {
		err = -errno;
		ASRCERR("F_GETFL failed (%i)", err);
		return err;
	}

	if (nonblock)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) {
		err = -errno;
		ASRCERR("F_SETFL for O_NONBLOCK failed (%i)", err);
		return err;
	}
	return 0;
}

static int asrc_imx_geterrfd(void *ctx)
{
	ASRC_TRC((asrc_end_t*) ctx);
	return -1;
}


static int asrc_imx_free(asrc_end_t *info)
{
	if (!info)
		return 0;

	ASRC_TRC(info);

	if (info->buffer) {
		munmap(info->buffer,info->buf_size);
		info->buffer = NULL;
	}

	if (info->fd >= 0) {
		close(info->fd);
		info->fd = -1;
	}
	destroy_route_info(info);

	if (info->log) {
		snd_output_close(info->log);
		info->log = NULL;
	}
	free(info);
	return 0;	
}

static int asrc_imx_close(void* ctx)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	ASRC_TRC(info);
	asrc_imx_free(info);
	return 0;
}

static int asrc_imx_dump(void *ctx, snd_output_t *out)
{
	asrc_end_t *info = (asrc_end_t*) ctx;
	ASRC_TRC(info);
	snd_output_printf(out, "Converter: ASRC IMX\n");
	return 0;
}

static const asrc_ops_t asrc_ops = {
	.get_capabilities = asrc_imx_get_caps,
	.configure = asrc_imx_configure,
	.dump = asrc_imx_dump,
	.start = asrc_imx_start,
	.prepare = asrc_imx_prepare,
	.stop = asrc_imx_stop,
	.drain = asrc_imx_drain,
	.xrun = asrc_imx_xrun,
	.get_pollfd = asrc_imx_getfd,
	.get_pollev = asrc_imx_getfdev,
	.set_pollfd_blockmode = asrc_imx_setfdmode,
	.get_pollfd_err = asrc_imx_geterrfd,
	.sync = asrc_imx_sync,
	.close = asrc_imx_close,
};


/**
 * \brief Creates a new iMX6 async sample rate conversion instance.
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
int SND_PCM_ARATE_PLUGIN_ENTRY(asrc_imx) (unsigned int version, void **objp, asrc_ops_t *ops, unsigned int mode, int id, const snd_config_t *conf)
{
	asrc_end_t *info = calloc(1, sizeof(asrc_end_t));
	int err;
	struct asrc_req req;

	if (info == NULL){
		ASRCERR("ALLOC ARATE INSTANCE FAILED");
		return -ENOMEM;
	}
	snd_output_stdio_attach(&info->log, stdout, 0);

	info->fd = -1;
	info->in = mode & SND_PCM_ARATE_STREAM_MODE_IN;

	if (version != SND_PCM_ARATE_PLUGIN_VERSION) {
		ASRCERR("version mismatch :V%x - expected V%x", version, SND_PCM_ARATE_PLUGIN_VERSION);
		asrc_imx_free(info);
		return -EINVAL;
	}

	if (create_cfg(info, conf) < 0) {
		ASRCERR("CREATE ROUTE FAILED");
		asrc_imx_free(info);
		return -EINVAL;
	}

	info->fd = open(ASRC_MXC_PATH, info->in?O_RDWR:O_RDONLY);
	if (info->fd < 0) {
		ASRCERR("open asrc %s failed with err %d",ASRC_MXC_PATH, errno);
		asrc_imx_free(info);
		return -errno;
	}

	req.pair_mask = info->pair_mask;
	req.uid = id;
	if ((err = ioctl(info->fd, ASRC_REQ_PAIR, &req)) < 0) {
		ASRCERR("Request pair(s) failed with err %d", errno);
		asrc_imx_free(info);
		return -errno;
	}

	if ((err = ioctl(info->fd, ASRC_INIT_SECTION, 0)) < 0) {
		ASRCERR("Failed to init section err %d", errno);
		asrc_imx_free(info);
		return -errno;
	}

	if ((int)info->filter != FILTER_INVALID) {
		enum filter_settings filter_conf;
		filter_conf = (enum filter_settings) info->filter;
		if ((err = ioctl(info->fd, ASRC_SET_FILTER, &filter_conf)) < 0) {
			ASRCERR("Failed to set filter: err %d", errno);
			asrc_imx_free(info);
			return -errno;
		}
	}

	if ((err = ioctl(info->fd, ASRC_SET_PERIOD, &info->fifo_wm)) < 0) {
		ASRCERR("Failed to set wm: err %d", errno);
		asrc_imx_free(info);
		return -errno;
	}

	ASRC_TRC(info);

	*ops = asrc_ops;
	*objp = info;

	return 0;
}

