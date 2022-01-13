/*
 *  Internal functions
 *  Copyright (c) 2000 by Abramo Bagnara <abramo@alsa-project.org>
 * 			Jaroslav Kysela <perex@perex.cz>
 *
 *   This file contains copies of alsalib internal functions needed to build plugin arate.
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

#include <time.h>
#include "pcm_local.h"

int snd_pcm_conf_generic_id(const char *id)
{
	static const char ids[3][8] = { "comment", "type", "hint" };
	unsigned int k;
	for (k = 0; k < sizeof(ids) / sizeof(ids[0]); ++k) {
		if (strcmp(id, ids[k]) == 0)
			return 1;
	}
	return 0;
}


static void snd_pcm_set_ptr(snd_pcm_t *pcm, snd_pcm_rbptr_t *rbptr,
			    volatile snd_pcm_uframes_t *hw_ptr, int fd, off_t offset)
{
	rbptr->master = NULL;	/* I'm master */
	rbptr->ptr = hw_ptr;
	rbptr->fd = fd;
	rbptr->offset = offset;
	if (rbptr->changed)
		rbptr->changed(pcm, NULL);
}

void snd_pcm_set_hw_ptr(snd_pcm_t *pcm, volatile snd_pcm_uframes_t *hw_ptr, int fd, off_t offset)
{
	assert(pcm);
	assert(hw_ptr);
	snd_pcm_set_ptr(pcm, &pcm->hw, hw_ptr, fd, offset);
}

void snd_pcm_set_appl_ptr(snd_pcm_t *pcm, volatile snd_pcm_uframes_t *appl_ptr, int fd, off_t offset)
{
	assert(pcm);
	assert(appl_ptr);
	snd_pcm_set_ptr(pcm, &pcm->appl, appl_ptr, fd, offset);
}

int snd_pcm_new(snd_pcm_t **pcmp, snd_pcm_type_t type, const char *name,
		snd_pcm_stream_t stream, int mode)
{
	snd_pcm_t *pcm;
	pcm = calloc(1, sizeof(*pcm));
	if (!pcm)
		return -ENOMEM;
	pcm->type = type;
	if (name)
		pcm->name = strdup(name);
	pcm->stream = stream;
	pcm->mode = mode;
	pcm->poll_fd_count = 1;
	pcm->poll_fd = -1;
	pcm->op_arg = pcm;
	pcm->fast_op_arg = pcm;
	INIT_LIST_HEAD(&pcm->async_handlers);
	*pcmp = pcm;
	return 0;
}

int snd_pcm_free(snd_pcm_t *pcm)
{
	assert(pcm);
	free(pcm->name);
	free(pcm->hw.link_dst);
	free(pcm->appl.link_dst);
	/*snd_dlobj_cache_put(pcm->open_func);*/
	free(pcm);
	return 0;
}

void snd_pcm_mmap_appl_backward(snd_pcm_t *pcm, snd_pcm_uframes_t frames)
{
	snd_pcm_sframes_t appl_ptr = *pcm->appl.ptr;
	appl_ptr -= frames;
	if (appl_ptr < 0)
		appl_ptr += pcm->boundary;
	*pcm->appl.ptr = appl_ptr;
}

void snd_pcm_mmap_appl_forward(snd_pcm_t *pcm, snd_pcm_uframes_t frames)
{
	snd_pcm_uframes_t appl_ptr = *pcm->appl.ptr;
	appl_ptr += frames;
	if (appl_ptr >= pcm->boundary)
		appl_ptr -= pcm->boundary;
	*pcm->appl.ptr = appl_ptr;
}


