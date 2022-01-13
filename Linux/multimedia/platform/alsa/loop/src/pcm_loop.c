/**
 * \file: pcm_loop.c
 *
 * Loop plugin for ALSA.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2014 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stddef.h>
#include <limits.h>
#include <sched.h>
#include <mqueue.h>
#include <grp.h>
#include <ctype.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>


/*TODO: 
-need to check boundary on xfer positions through SHM ?
*/

#define LOOP_IPCBASENAME "/pcmloop"	/*basename for mq and shared memory*/

#define LOOP_IPC_PERMS 0600 /*default permissions if nothing is given*/
#define LOOP_IPC_UMASK (S_IXUSR | S_IXGRP | S_IRWXO)

//#define LOOPDEBUG
#ifdef LOOPDEBUG
#define LOOPDBG SNDERR
#else
#define LOOPDBG(...) do{}while(0)
#endif

/*chosen hw params*/
struct shm_hwparams {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;
	unsigned int interleaved;/*defines SHM layout - no influence on access itself*/
};

struct shm_ctrl {
	pthread_mutex_t mtx;
	snd_pcm_uframes_t wr;	/*write position*/
	snd_pcm_uframes_t rd;	/*read position*/
	char stop;		/*stop notified by writer until writer is in state running*/
	char reset;		/*reset notified by writer - cleared by reader*/
	char cfg_fixed;		/*config is fixed and available in SHM*/
	char in_use[2];		/*user management*/
	char cfg_master[2];	/*identify configuration master*/
};

struct shm_fwd {
	struct shm_ctrl ctrl;
	struct shm_hwparams params;
	/*sample area*/
};

#define SHM_RINGBUF(p) (((char*)((p)->shmp))+sizeof(struct shm_fwd))
#define SHM_CFG(p) (p)->shmp->params
struct loop_params_pref {/*preferred params from config*/
	snd_pcm_format_t format;
	int channels;
	int rate;
	snd_pcm_sframes_t period_size;
	snd_pcm_sframes_t buffer_size;
	int interleaved;
};

#define CFG_PARAMS_INIT \
(struct loop_params_pref){\
	.format = SND_PCM_FORMAT_UNKNOWN,\
	.channels = -1,\
	.rate = -1,\
	.period_size = -1,\
	.buffer_size = -1,\
	.interleaved = -1,\
};

#define SHMNAME_MAX 64
#define MQNAME_MAX 64
#define LOOPNAME_MAX 64

#define SUBDEVS_MAX 32	/*amount of subdevs - real limit is 256 (2digits for hex mq/shm name)*/

/*different signals only for debug purpose - receiver does not care of signal type*/
#define SIG_STOP 's'
#define SIG_GO 'g'
#define SIG_UPDATE 'u'
#define SIG_CFG 'c'

typedef struct snd_pcm_loop {
	snd_pcm_ioplug_t io; /**/
	snd_pcm_channel_area_t *shm_areas;
	snd_pcm_uframes_t avail_min; /*mirror of sw params*/
	snd_pcm_uframes_t boundary; /*mirror of sw params*/
	char loop_name[LOOPNAME_MAX];
	char shm_name[SHMNAME_MAX];
	char mqpoll_name[MQNAME_MAX];
	char mqsig_name[MQNAME_MAX];
	int shmfd;
	size_t mapped_size;
	int device;
	snd_pcm_sframes_t rw_offs;
	mqd_t mq_poll;
	mqd_t mq_sig;
	struct shm_fwd* shmp;
} snd_pcm_loop_t;

#define USE(a) ((a)=(a))

static int loop_cfg_on_open(snd_pcm_loop_t *pcm, const struct loop_params_pref *cfg_params);
static int loop_cfg_on_hw_params(snd_pcm_loop_t *pcm, snd_pcm_hw_params_t *params);
static int loop_cfg_on_hw_free(snd_pcm_loop_t *pcm, int last_user);
static int loop_cfg_on_close(snd_pcm_loop_t *pcm, int last_user);

static size_t round_up_to_page(size_t size)
{
	/* _SC_PAGE_SIZE must not be less than 1. See manpage sysconf3 */
	const size_t psz = sysconf(_SC_PAGE_SIZE);
	const size_t r = size % psz;
	return r?(size + psz - r):size;
}

static int flush_mq(mqd_t mq)
{
	char msg;
	int ret;
	do {
		ret = mq_receive(mq, &msg, sizeof(msg), NULL);
	} while (ret > 0);

	if (ret < 0) {
		if (errno == EAGAIN) {
			ret = 0;
		} else {
			SYSERR("MQ receive failed");
			ret = -errno;
		}
	}
	return ret;
}

/*signal to own side*/
static int dummy_poll(struct snd_pcm_loop *pcm, char msg)
{
	int ret = mq_send(pcm->mq_poll, &msg, sizeof(msg), 0);
	if (ret < 0) {
		/*no error if EAGAIN: no need to trigger multiple times...*/
		if (errno == EAGAIN) {
			ret = 0;
		} else {
			SYSERR("MQ SEND failed");
			ret = -errno;
		}
	}
	return ret;
}

/*signal to remote side*/
static int signal_mq(struct snd_pcm_loop *pcm, char msg)
{
	int ret = mq_send(pcm->mq_sig, &msg, sizeof(msg), 0);
	if (ret < 0) {
		/*no error if EAGAIN: no need to trigger multiple times...*/
		if (errno == EAGAIN) {
			ret = 0;
		} else {
			SYSERR("MQ SEND failed");
			ret = -errno;
		}
	}
	return ret;
}

/*mq is set to nonblocking */
static int consume_mq(struct snd_pcm_loop *pcm)
{
	return flush_mq(pcm->mq_poll);
}


/*mq is used only to implement poll descriptor -> dimension is 1 message of size 1*/
static mqd_t create_mq(char *mq_name, int instance, int uc, int suffix, int perm, int gid)
{
	mqd_t mq;
	struct mq_attr attr;
	int pos;
	mode_t umask_old;

	pos = snprintf(mq_name, MQNAME_MAX, "%s", LOOP_IPCBASENAME);
	pos+= snprintf(&mq_name[pos], MQNAME_MAX-pos, "_%02x", instance);
	pos+= snprintf(&mq_name[pos], MQNAME_MAX-pos, "_%1x", uc);
	pos+= snprintf(&mq_name[pos], MQNAME_MAX-pos, "_%1x", suffix);

	attr.mq_flags = O_NONBLOCK;
	attr.mq_maxmsg = 1;
	attr.mq_msgsize = 1;
	umask_old = umask(LOOP_IPC_UMASK);
	mq = mq_open(mq_name, O_CREAT|O_RDWR|O_NONBLOCK, perm, &attr);
	umask(umask_old);
	if (mq < 0) {
		SYSERR("MQOPEN %s failed", mq_name);
		return -errno;
	}

	if (gid > 0) {
		if (fchown(mq, (uid_t)-1, (gid_t)gid) < 0) {
			if (errno != EPERM)
				SYSERR("fchown gid %d for %s failed",gid, mq_name);
		}
	}

	LOOPDBG("MQ %s", mq_name);

	return mq;
}

#define ABORT_ON_LOCK_ERR(e)\
	if(e<0) {\
		SNDERR("err %d : MTX corrupted ? ABORT", e);\
		abort();\
	} 

/*no chance to recover locking error - abort on failure*/
#define shm_lock(shm) do {\
	int _err = pthread_mutex_lock(&(shm)->mtx);\
	if (_err == EOWNERDEAD) {\
		pthread_mutex_consistent(&(shm)->mtx);\
		_err = 0;\
	}\
	ABORT_ON_LOCK_ERR(_err);\
} while(0)

/*no chance to recover locking error - abort on failure*/
#define shm_unlock(shm) do {\
	int _err = pthread_mutex_unlock(&(shm)->mtx);\
	ABORT_ON_LOCK_ERR(_err);\
} while (0)

/*	
	reset pointers, 
	Don't get stream from pcm->io.stream, it may not yet be valid
*/
static void shm_reset(snd_pcm_loop_t *pcm, snd_pcm_stream_t stream)
{
	if (stream == SND_PCM_STREAM_PLAYBACK) {
		pcm->shmp->ctrl.wr = 0;
		pcm->shmp->ctrl.rd = 0;
		pcm->shmp->ctrl.stop = 1;
		pcm->shmp->ctrl.reset = 1;/*wronly on playback side*/
	} else {
		/*store actual read position*/
		pcm->rw_offs = -pcm->shmp->ctrl.rd;
	}
	return;
}

/*
	init shm on new attached user
*/
static void shm_attached_user(struct shm_fwd* shmp, int idx)
{
	/*only the first user needs to init*/
	if (shmp->ctrl.in_use[idx^1])
		return;

	/*init control block*/

	/*shmp->ctrl.mtx never touched after shm creation*/
	shmp->ctrl.wr = 0;
	shmp->ctrl.rd = 0;
	shmp->ctrl.stop = 0;
	shmp->ctrl.reset = 0;
	shmp->ctrl.cfg_fixed = 0;
	shmp->ctrl.cfg_master[idx^1] = 0;
	/*we are config master*/
	shmp->ctrl.cfg_master[idx] = 1;
	/*shmp->ctrl.in_use handeld by claim/unclaim*/

	/*invalidate hw params - not necessary, as cfg_fixed is cleared*/
	memset(&shmp->params, 0, sizeof(shmp->params));
}

static int claim_instance(struct shm_fwd *shmp, int device)
{
	return __sync_bool_compare_and_swap(&shmp->ctrl.in_use[device], 0, 1);
}

static int unclaim_instance(struct shm_fwd *shmp, int device)
{
	return __sync_bool_compare_and_swap(&shmp->ctrl.in_use[device], 1, 0);
}


static int delete_shm(struct snd_pcm_loop *pcm)
{
	int err;

	if (pcm->shmp != MAP_FAILED) {
		err = munmap(pcm->shmp, pcm->mapped_size);
		if (err < 0)
			SYSERR("unmap failed");
		pcm->shmp = MAP_FAILED;
	}
	if (pcm->shmfd != -1) {
		err = close(pcm->shmfd);
		if (err < 0)
			SYSERR("close failed");
		pcm->shmfd = -1;
	}

	return 0;
}


/*
If returning with success the caller has created or attached to the desired shared segment.
The segment is mapped with at least the size of the control block and mutex is available.
*/
static int create_shm(struct snd_pcm_loop *pcm, int device, int instance, int uc, int perm, long gid, int ignore_busy)
{
	int fd;
	int err = 0;
	int pos;
	int creator = 0;
	struct stat sbuf;
	ssize_t shm_size = sizeof(struct shm_fwd);
	struct shm_fwd *shmp = MAP_FAILED;
	char *shm_name = pcm->shm_name;	
	mode_t umask_old;

	pos = snprintf(shm_name, SHMNAME_MAX, "%s", LOOP_IPCBASENAME);
	pos+= snprintf(&shm_name[pos], SHMNAME_MAX-pos, "_%02x", instance);
	pos+= snprintf(&shm_name[pos], SHMNAME_MAX-pos, "_%1x", uc);

	umask_old = umask(LOOP_IPC_UMASK);
	fd = shm_open(shm_name, O_CREAT|O_RDWR, perm);
	umask(umask_old);
	if (fd < 0) {
		SYSERR("open existing failed");
		return -errno;
	}

	err = flock(fd, LOCK_EX);
	if (err < 0) {
		SYSERR("flock failed");
		close(fd);
		return -errno;
	}

	err = fstat(fd, &sbuf);
	if (err < 0) {
		SYSERR("fstat failed");
		close(fd);
		return -errno;
	}

	if (sbuf.st_size < (off_t) sizeof(struct shm_fwd)) {
		creator = 1;
		err = ftruncate(fd, sizeof(struct shm_fwd));
		if (err < 0) {
			SYSERR("truncate failed");
			err = -errno;
		}
	}

	if (!err) {
		shm_size = round_up_to_page(sizeof(struct shm_fwd));
		shmp = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (shmp == MAP_FAILED) {
			SYSERR("mmap failed");
			err = -errno;
		}
	}
	if (creator && (err == 0)) {
		/*the creator needs to initialize it... a new created shm is zeroed, no need to memset it*/
		if (gid > 0) {
			if (fchown(fd, (uid_t)-1, (gid_t)gid) < 0) {
				err = -errno;
				SYSERR("fchown gid %d for %s failed with err %d",gid, shm_name, err);
			}
		}
		if (!err) {
			pthread_mutexattr_t mutex_attr;
			err = pthread_mutexattr_init(&mutex_attr);
			if (!err)
				err = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
			if (!err)
				err = pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST);
			if (!err)
				err = pthread_mutex_init(&shmp->ctrl.mtx, &mutex_attr);
		}
	}

	/*register in atomic way without using lock to not influence active streaming of other legal user*/
	if (!err) {
		if (!claim_instance(shmp, device)) {
			/*should only be used in development phase...*/
			if (ignore_busy) {
				SNDERR("IGNORE BUSY FOR DEVICE %d", device);
			} else {
				/*instance is already in use*/
				SNDERR("instance in use");
				err = -EBUSY;
			}
		}
	}

	if (!err) { /*no need to unlock in error case - automatically released on close*/
		/*we have an instance - init area if we are the first user attaching to it
		as flock is held we can't race against a parallel creator */
		shm_lock(&shmp->ctrl);
		shm_attached_user(shmp, device);
		shm_unlock(&shmp->ctrl);

		err = flock(fd, LOCK_UN);
		if (err < 0) {
			SYSERR("file unlock failed");
			err = -errno;
		}
	}

	if (err) {
		if (shmp != MAP_FAILED) {
			if (munmap(shmp, shm_size) < 0)
				SYSERR("munmap failed");
		}
		if (close(fd) < 0)
			SYSERR("close failed");
		if (creator) {
			if (shm_unlink(shm_name) < 0)
				SYSERR("unlink failed");
		}
		return err;
	}

	/*shm initialized - locking primitives usable*/
	pcm->shmfd = fd;
	pcm->shmp = shmp;
	pcm->mapped_size = shm_size;

	return 0;
}

/*resize: only allowed if both sides agreed about same size*/
static int resize_shm(struct snd_pcm_loop *pcm, size_t sample_bytes)
{
	const size_t shm_size = sample_bytes + sizeof(struct shm_fwd);
	size_t mapped_size;
	struct shm_fwd* shmp;
	int err;

	err = ftruncate(pcm->shmfd, shm_size);
	if (err < 0) {
		SYSERR("ftruncate failed");
		return -errno;
	}
	mapped_size = round_up_to_page(shm_size);
	shmp = mmap(NULL, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, pcm->shmfd, 0);

	if (shmp == MAP_FAILED) {
		SYSERR("mmap failed");
		return -errno;
	}

	err = munmap(pcm->shmp, pcm->mapped_size);
	if (err < 0) {
		SYSERR("unmap failed");
		return -errno;
	}
	pcm->mapped_size = mapped_size;
	pcm->shmp = shmp;

	LOOPDBG("SHM %p size %d mmapsize%d ", pcm->shmp, shm_size, pcm->mapped_size);
		
	return 0;
}


static inline snd_pcm_uframes_t snd_pcm_loop_playback_avail(snd_pcm_loop_t *pcm, snd_pcm_sframes_t hw, snd_pcm_sframes_t app)
{
	snd_pcm_sframes_t avail;
	avail = hw + pcm->io.buffer_size - app;
	/* pcm->boundary is used to wrap shared buffer. So its always smaller than INT_MAX */
	if (avail < 0)
		avail += pcm->boundary;
	else if ((snd_pcm_uframes_t) avail >= pcm->boundary)
		avail -= pcm->boundary;
	return avail;
}

static inline snd_pcm_uframes_t snd_pcm_loop_capture_avail(snd_pcm_loop_t *pcm, snd_pcm_sframes_t hw, snd_pcm_sframes_t app)
{
	snd_pcm_sframes_t avail;
	avail = hw - app;
	/* pcm->boundary is used to wrap shared buffer. So its always smaller than INT_MAX */
	if (avail < 0)
		avail += pcm->boundary;
	return avail;
}

/*in: app pointer out: hw_pointer*/
static snd_pcm_sframes_t shm_sync(snd_pcm_loop_t *pcm, snd_pcm_uframes_t app_pos)
{
	int change = 0;/*optimize: make use of avail_min ?*/
	int fwd = 0;
	int err = 0;
	snd_pcm_uframes_t hw_pos;
	snd_pcm_sframes_t avail = 0;

	shm_lock(&pcm->shmp->ctrl);

	if (pcm->io.stream == SND_PCM_STREAM_PLAYBACK) {
		/*playback side: always export current write pointer*/
		change = app_pos != pcm->shmp->ctrl.wr;
		pcm->shmp->ctrl.wr = app_pos;
		hw_pos = pcm->shmp->ctrl.rd;
		avail = snd_pcm_loop_playback_avail(pcm, hw_pos, app_pos);
	} else {
		/*capture side: if reset is requested, store actual hw pointer to use as offset in future*/
		if (pcm->shmp->ctrl.reset){
			fwd = 1; /*do forward after unlocking*/
			app_pos = pcm->io.hw_ptr;/*... but already forward local app_pos for following calculations*/
			pcm->rw_offs = pcm->io.hw_ptr;
			pcm->shmp->ctrl.reset = 0;
		}
		if (pcm->shmp->ctrl.stop) {
			hw_pos = pcm->io.hw_ptr;/*freeze hw pos until reset cleared*/
		} else {
			change = (app_pos - pcm->rw_offs) != pcm->shmp->ctrl.rd;
			pcm->shmp->ctrl.rd = app_pos - pcm->rw_offs;
			hw_pos = pcm->shmp->ctrl.wr + pcm->rw_offs;
			avail = snd_pcm_loop_capture_avail(pcm, hw_pos, app_pos);
		}
	}

	/*consume poll event only if we fall below avail_min*/
	if ((snd_pcm_uframes_t)avail < pcm->avail_min)
		(void)consume_mq(pcm);

	shm_unlock(&pcm->shmp->ctrl);

	if (fwd) {
		/*forward app pos to discard currently available frames*/
		avail = snd_pcm_forwardable(pcm->io.pcm); /*!!THIS MAY CALL SHM_SYNC in future (recursion) !!-->OK, as fwd will not be set again*/
		if (avail > 0)
			snd_pcm_forward(pcm->io.pcm, avail);
		if (avail < 0)
			hw_pos = avail;
	}

	if (hw_pos >= pcm->boundary)
		hw_pos -= pcm->boundary;

	if (change)
		err = signal_mq(pcm, SIG_UPDATE);
	return err?err:(snd_pcm_sframes_t)hw_pos;
}


static int shm_stop(snd_pcm_loop_t *pcm)
{
	int ret;
	shm_lock(&pcm->shmp->ctrl);
	shm_reset(pcm, pcm->io.stream);
	ret = signal_mq(pcm, SIG_STOP);
	shm_unlock(&pcm->shmp->ctrl);
	return ret;
}


static int shm_go(snd_pcm_loop_t *pcm)
{
	int ret;
	shm_lock(&pcm->shmp->ctrl);
	if (pcm->io.stream == SND_PCM_STREAM_PLAYBACK)
		pcm->shmp->ctrl.stop = 0;
	ret = signal_mq(pcm, SIG_GO);
	shm_unlock(&pcm->shmp->ctrl);
	return ret;
}

static int loop_start(snd_pcm_ioplug_t *io)
{
	snd_pcm_loop_t *pcm = io->private_data;
	snd_pcm_sframes_t avail;

	avail = snd_pcm_avail(pcm->io.pcm);
	if (avail < 0)
		return avail;
	if ((snd_pcm_uframes_t)avail >= pcm->avail_min) {
		/*generate dummy poll event - some applications may start with snd_pcm_wait() without syncing before.*/
		dummy_poll(pcm, 'd');
	}
	return shm_go(pcm);
}

static int loop_stop(snd_pcm_ioplug_t *io)
{
	snd_pcm_loop_t *pcm = io->private_data;
	return shm_stop(pcm);
}

static snd_pcm_sframes_t loop_pointer(snd_pcm_ioplug_t *io)
{
	snd_pcm_loop_t *pcm = io->private_data;
	
	if (io->state == SND_PCM_STATE_XRUN) {
		return -EPIPE;
	}
	return shm_sync(pcm, pcm->io.appl_ptr);
}

static snd_pcm_sframes_t loop_xfer(snd_pcm_ioplug_t *io, const snd_pcm_channel_area_t *areas,
				snd_pcm_uframes_t offset, snd_pcm_uframes_t size)
{
	snd_pcm_loop_t *pcm = io->private_data;
	snd_pcm_uframes_t shm_offs = (pcm->io.appl_ptr-pcm->rw_offs)%pcm->io.buffer_size;
	/*buffer wrap around is not handled by snd_pcm_areas_copy, so need to do it manually on SHM access*/
	snd_pcm_uframes_t wrp = (shm_offs + size > pcm->io.buffer_size) ? pcm->io.buffer_size - shm_offs : size;

 	if (io->stream == SND_PCM_STREAM_PLAYBACK) {
 		snd_pcm_areas_copy(pcm->shm_areas, shm_offs, areas, offset,
				   io->channels, wrp, io->format);
		if (wrp < size)
			snd_pcm_areas_copy(pcm->shm_areas, 0, areas, offset+wrp,
				   io->channels, size-wrp, io->format);
 	} else {
 		snd_pcm_areas_copy(areas, offset, pcm->shm_areas, shm_offs,
				   io->channels, wrp, io->format);
		if (wrp < size)
			snd_pcm_areas_copy(areas, offset+wrp, pcm->shm_areas, 0,
				   io->channels, size-wrp, io->format);
 	}
	return size;
}

static int loop_destroy(snd_pcm_loop_t *pcm)
{
	int err;
	int last_user;
	shm_lock(&pcm->shmp->ctrl);
	last_user = pcm->shmp->ctrl.in_use[pcm->device^1] == 0;
	LOOPDBG("%s", last_user?"LAST USER":"OTHER STILL ACTIVE");

	loop_cfg_on_close(pcm, last_user);

	if (pcm->mq_sig != (mqd_t)-1) {
		(void)mq_close(pcm->mq_sig);
		pcm->mq_sig = (mqd_t)-1;
	}
	if (pcm->mq_poll != (mqd_t)-1) {
		(void)mq_close(pcm->mq_poll);
		pcm->mq_poll = (mqd_t)-1;
	}
	if (pcm->shm_areas != NULL) {
		free(pcm->shm_areas);
		pcm->shm_areas = NULL;
	}

	if (last_user) {
		LOOPDBG("trunc SHM %s", pcm->shm_name);

		err = ftruncate(pcm->shmfd, sizeof(struct shm_fwd));
		if (err < 0)
			SYSERR("ftruncate failed");
	}

	(void)unclaim_instance(pcm->shmp, pcm->device);
	shm_unlock(&pcm->shmp->ctrl);

		
	(void)delete_shm(pcm);
	free(pcm);
	return 0;
}

static int loop_close(snd_pcm_ioplug_t *io)
{
	snd_pcm_loop_t *pcm = io->private_data;
	return loop_destroy(pcm);
}

static int loop_hw_params(snd_pcm_ioplug_t *io, snd_pcm_hw_params_t *params)
{
	snd_pcm_loop_t *pcm = io->private_data;
	int err;
	/* cast to size_t to calculate with 64 bit on 64 bit system */
	const size_t phy_width = snd_pcm_format_physical_width(io->format);
	const size_t frame_size = (phy_width * io->channels) / 8;
	err = resize_shm(pcm, io->buffer_size * frame_size);
	if (!err)
		err = loop_cfg_on_hw_params(pcm, params);
	return err;
}

static int loop_hw_free(snd_pcm_ioplug_t *io)
{
	snd_pcm_loop_t *pcm = io->private_data;
	int err;
	int last_user;
	shm_lock(&pcm->shmp->ctrl);
	last_user = pcm->shmp->ctrl.in_use[pcm->device^1] == 0;
	err = loop_cfg_on_hw_free(pcm, last_user);
	shm_unlock(&pcm->shmp->ctrl);

	return err;
}

static int loop_sw_params(snd_pcm_ioplug_t *io, snd_pcm_sw_params_t *params)
{
	snd_pcm_loop_t *pcm = io->private_data;
	/*disable xrun detection - sw cannot xrun*/
	snd_pcm_sw_params_set_stop_threshold(io->pcm, params, INT_MAX);
	snd_pcm_sw_params_get_avail_min(params, &pcm->avail_min);
	/*assume same boundary on playback/capture side - fullfilled as long as same hw_params are forced.*/
	snd_pcm_sw_params_get_boundary(params, &pcm->boundary);
	return 0;
}

static int loop_poll_descriptors(snd_pcm_ioplug_t *io, struct pollfd *pfd, unsigned int space)
{
	snd_pcm_loop_t *pcm = io->private_data;

	if (space != 1)
		return -EINVAL;

	/*race if snd_pcm_abort() (from signal handler) gets called before
	entering poll(), but after checking process internal abortion flags.
	Poll may then block forever as many calls are done without timeout
	(usually when opened in blocking mode). A user has to send signal
	a second time to unblock. To minimize this race, we check for
	abort condition before giveback fd.
	Unfortunaltely ALSA does not export an API to check for
	mode flag SND_PCM_ABORT, so we need to rely on this hardcoded value
	stored by ioplug inside 'nonblock' callback.
	*/
	if (io->nonblock == 2) {
		SNDERR("Block poll");
		return -EINTR;
	}
	pfd[0].fd = pcm->mq_poll;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	return 1;
}

static const snd_pcm_ioplug_callback_t loop_callback = {
	.start = loop_start,
	.stop = loop_stop,
	.pointer = loop_pointer,
	.transfer = loop_xfer,
	.hw_params = loop_hw_params,
	.hw_free = loop_hw_free,
	.sw_params = loop_sw_params,
	.poll_descriptors = loop_poll_descriptors,
	.close = loop_close,
};


/*---------------------- config ----------------------------------*/
#define LOOP_CHN_DEFAULT_MIN 1
#define LOOP_CHN_DEFAULT_MAX 32
#define LOOP_RATE_DEFAULT_MIN 5512
#define LOOP_RATE_DEFAULT_MAX 192000
#define LOOP_PERIODBYTES_DEFAULT_MIN	16	/*reasonable limit*/
#define LOOP_PERIODBYTES_DEFAULT_MAX	65536	/*reasonable limit - no real limitation*/
#define LOOP_BUFFERBYTES_DEFAULT_MIN	LOOP_PERIODBYTES_DEFAULT_MIN
#define LOOP_BUFFERBYTES_DEFAULT_MAX	LOOP_PERIODBYTES_DEFAULT_MAX

#define LOOP_WAIT_CFG_TMOUT	5000	/*timeout in ms for slave to wait for config fixed by master*/
#define chk_cfg_master(p) ((p)->shmp->ctrl.cfg_master[(p)->device] != 0)
#define chk_cfg_fixed(p) ((p)->shmp->ctrl.cfg_fixed != 0)
#define set_cfg_fixed(p, val) ((p)->shmp->ctrl.cfg_fixed = val)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

/*export chosen params to SHM - slave must use these*/
static int loop_export_hw_params(snd_pcm_loop_t *pcm, snd_pcm_hw_params_t *params)
{
	snd_pcm_access_t access_type;
	snd_pcm_hw_params_get_access(params, &access_type);

	LOOPDBG("EXPORT MASTER CFG TO SHM");

	switch (access_type) {
	case SND_PCM_ACCESS_MMAP_INTERLEAVED:
	case SND_PCM_ACCESS_RW_INTERLEAVED:
		SHM_CFG(pcm).interleaved = 1;
		break;

	case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
	case SND_PCM_ACCESS_RW_NONINTERLEAVED:
		SHM_CFG(pcm).interleaved = 0;
		break;
	default:
		return -EINVAL;
	}
	snd_pcm_hw_params_get_format(params, &SHM_CFG(pcm).format);
	snd_pcm_hw_params_get_channels(params, &SHM_CFG(pcm).channels);
	snd_pcm_hw_params_get_rate(params, &SHM_CFG(pcm).rate, NULL);
	snd_pcm_hw_params_get_buffer_size(params, &SHM_CFG(pcm).buffer_size);
	snd_pcm_hw_params_get_period_size(params, &SHM_CFG(pcm).period_size, NULL);

	LOOPDBG("interl %d fmt %d ch %d rate %d buf %d period %d", SHM_CFG(pcm).interleaved, SHM_CFG(pcm).format, SHM_CFG(pcm).channels,SHM_CFG(pcm).rate,SHM_CFG(pcm).buffer_size, SHM_CFG(pcm).period_size);

	set_cfg_fixed(pcm, 1);

	/*slave may wait for fixup...*/
	signal_mq(pcm, SIG_CFG);
	return 0;
}

/*compare the chosen parameters with the original constraints given by master via SHM.
 This should never fail, as slave offered only the SHM constraints to ALSA,
 but it seems to be good to double check.
*/
static int loop_compare_hw_params(const snd_pcm_loop_t* const pcm, const snd_pcm_hw_params_t* const params)
{
	unsigned int value;
	snd_pcm_format_t format;
	snd_pcm_uframes_t size;

	snd_pcm_hw_params_get_format(params, &format);
	if (format != SHM_CFG(pcm).format)
		return -EINVAL;
	snd_pcm_hw_params_get_channels(params, &value);
	if (value != SHM_CFG(pcm).channels)
		return -EINVAL;
	snd_pcm_hw_params_get_rate(params, &value, NULL);
	if (value != SHM_CFG(pcm).rate)
		return -EINVAL;
	snd_pcm_hw_params_get_buffer_size(params, &size);
	if (size != SHM_CFG(pcm).buffer_size)
		return -EINVAL;
	snd_pcm_hw_params_get_period_size(params, &size, NULL);
	if (size != SHM_CFG(pcm).period_size)
		return -EINVAL;

	return 0;
}

#define CFG_MISMATCH(str, val_u, val_shm) do{SNDERR("%s mismatch: user:%d shm:%d", (str), (unsigned int)(val_u), (unsigned int)(val_shm) ); return -EINVAL;}while(0)
/*offer the fixed SHM parameters to ALSA*/
static int loop_offer_shm_params(snd_pcm_loop_t *pcm, const struct loop_params_pref *cfg_params)
{
	unsigned int bsize, psize;
	int err;
	snd_pcm_ioplug_t *io = &pcm->io;
	/*SHM layout depends on access interleaved/non interleaved*/
	static const snd_pcm_access_t access_list_interleaved[] = {
		SND_PCM_ACCESS_MMAP_INTERLEAVED,
		SND_PCM_ACCESS_RW_INTERLEAVED,
	};
	static const snd_pcm_access_t access_list_noninterleaved[] = {
		SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
		SND_PCM_ACCESS_RW_NONINTERLEAVED,
	};
	static const snd_pcm_access_t access_list_all[] = {
		SND_PCM_ACCESS_MMAP_INTERLEAVED,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
		SND_PCM_ACCESS_RW_NONINTERLEAVED,
	};

	LOOPDBG("OFFER SHM CFG TO USER");

	/*check preferred config against SHM*/
	if ((cfg_params->interleaved != -1) && ((unsigned int)cfg_params->interleaved != SHM_CFG(pcm).interleaved))
		CFG_MISMATCH("INTERLEAVE", cfg_params->interleaved, SHM_CFG(pcm).interleaved);

	if ((cfg_params->format != SND_PCM_FORMAT_UNKNOWN) && (cfg_params->format != SHM_CFG(pcm).format))
		CFG_MISMATCH("FORMAT", (int)cfg_params->format, (int)SHM_CFG(pcm).format);

	if ((cfg_params->channels != -1) && ((unsigned int)cfg_params->channels != SHM_CFG(pcm).channels))
		CFG_MISMATCH("CHANNELS", cfg_params->channels, SHM_CFG(pcm).channels);

	if ((cfg_params->rate != -1) && ((unsigned int)cfg_params->rate != SHM_CFG(pcm).rate))
		CFG_MISMATCH("RATE", cfg_params->rate, SHM_CFG(pcm).rate);

	if ((cfg_params->buffer_size != -1) && ((snd_pcm_uframes_t)cfg_params->buffer_size != SHM_CFG(pcm).buffer_size))
		CFG_MISMATCH("BUFFER SIZE", cfg_params->buffer_size, SHM_CFG(pcm).buffer_size);

	if ((cfg_params->period_size != -1) && ((snd_pcm_uframes_t)cfg_params->period_size != SHM_CFG(pcm).period_size))
		CFG_MISMATCH("PERIOD SIZE", cfg_params->period_size, SHM_CFG(pcm).period_size);

	LOOPDBG("interl %d fmt %d ch %d rate %d buf %d period %d", SHM_CFG(pcm).interleaved, SHM_CFG(pcm).format, SHM_CFG(pcm).channels, SHM_CFG(pcm).rate, SHM_CFG(pcm).buffer_size, SHM_CFG(pcm).period_size);


	/*access*/
	if (cfg_params->interleaved != -1) {
		if (cfg_params->interleaved)
			/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
			err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_ACCESS,
						ARRAY_SIZE(access_list_interleaved), (const unsigned int *)access_list_interleaved);
		else
			/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
			err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_ACCESS,
						ARRAY_SIZE(access_list_noninterleaved), (const unsigned int *)access_list_noninterleaved);
	} else {
		/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
		err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_ACCESS,
					ARRAY_SIZE(access_list_all), (const unsigned int *)access_list_all);
	}
	if (err < 0)
		return err;

	/*fmt*/
	/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
	err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_FORMAT,
						1, (unsigned int*) &SHM_CFG(pcm).format);
	if (err < 0)
		return err;

	/*chn*/
	err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_CHANNELS,
						SHM_CFG(pcm).channels, SHM_CFG(pcm).channels);
	if (err < 0)
		return err;

	/*rate*/
	err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_RATE,
						SHM_CFG(pcm).rate, SHM_CFG(pcm).rate);
	if (err < 0)
		return err;

	/*buffer*/
	bsize = snd_pcm_format_size(SHM_CFG(pcm).format, SHM_CFG(pcm).channels*SHM_CFG(pcm).buffer_size);
	err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_BUFFER_BYTES, bsize, bsize);
	if (err < 0)
		return err;

	/*period*/
	psize = snd_pcm_format_size(SHM_CFG(pcm).format, SHM_CFG(pcm).channels*SHM_CFG(pcm).period_size);
	err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_PERIOD_BYTES, psize, psize);
	if (err < 0)
		return err;

	err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_PERIODS, bsize/psize, bsize/psize);
	if (err < 0)
		return err;

	return 0;
}

/*called with LOCK held */
static int loop_cfg_on_open_m(snd_pcm_loop_t *pcm, const struct loop_params_pref *cfg_params)
{
	int err;
	snd_pcm_ioplug_t *io = &pcm->io;

	static const snd_pcm_access_t access_list_interleaved[] = {
		SND_PCM_ACCESS_MMAP_INTERLEAVED,
		SND_PCM_ACCESS_RW_INTERLEAVED,
	};
	static const snd_pcm_access_t access_list_noninterleaved[] = {
		SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
		SND_PCM_ACCESS_RW_NONINTERLEAVED,
	};
	static const snd_pcm_access_t access_list_all[] = {
		SND_PCM_ACCESS_MMAP_INTERLEAVED,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
		SND_PCM_ACCESS_RW_NONINTERLEAVED,
	};

	static const snd_pcm_format_t formats[] = {
		SND_PCM_FORMAT_S8,
		SND_PCM_FORMAT_U8,
		SND_PCM_FORMAT_S16_LE,
		SND_PCM_FORMAT_S16_BE,
		SND_PCM_FORMAT_U16_LE,
		SND_PCM_FORMAT_U16_BE,
		/* _3LE/_3BE variants needed ?
		SND_PCM_FORMAT_S24_3LE,
		SND_PCM_FORMAT_S24_3BE,
		SND_PCM_FORMAT_U24_3LE,
		SND_PCM_FORMAT_U24_3BE,
		*/
		SND_PCM_FORMAT_S24_LE,
		SND_PCM_FORMAT_S24_BE,
		SND_PCM_FORMAT_U24_LE,
		SND_PCM_FORMAT_U24_BE,
		SND_PCM_FORMAT_S32_LE,
		SND_PCM_FORMAT_S32_BE,
		SND_PCM_FORMAT_U32_LE,
		SND_PCM_FORMAT_U32_BE,
	};

	LOOPDBG("OFFER MASTER CFG TO USER");

	/*access - mmap/rw independent on both sides - interleaved is dependent*/
	if (cfg_params->interleaved != -1) {
		if (cfg_params->interleaved)
			/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
			err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_ACCESS,
						ARRAY_SIZE(access_list_interleaved), (const unsigned int *)access_list_interleaved);
		else
			/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
			err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_ACCESS,
						ARRAY_SIZE(access_list_noninterleaved), (const unsigned int *)access_list_noninterleaved);
	} else {
		/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
		err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_ACCESS,
					ARRAY_SIZE(access_list_all), (const unsigned int *)access_list_all);
	}
	if (err < 0)
		return err;

	/*fmt*/
	if (cfg_params->format == SND_PCM_FORMAT_UNKNOWN)
		/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
		err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_FORMAT,
						ARRAY_SIZE(formats), (const unsigned int *)formats);
	else
		/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
		err = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_FORMAT,
						1,(unsigned int*) &cfg_params->format);
	if (err < 0)
		return err;

	/*chn*/
	if (cfg_params->channels == -1)
		err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_CHANNELS,
						LOOP_CHN_DEFAULT_MIN, LOOP_CHN_DEFAULT_MAX);
	else
		err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_CHANNELS,
						cfg_params->channels, cfg_params->channels);
	if (err < 0)
		return err;

	/*rate*/
	if (cfg_params->rate == -1)
		err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_RATE,
						LOOP_RATE_DEFAULT_MIN, LOOP_RATE_DEFAULT_MAX);
	else
		err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_RATE,
						cfg_params->rate, cfg_params->rate);
	if (err < 0)
		return err;

	/*buffer*/
	if (cfg_params->buffer_size == -1)
		err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_BUFFER_BYTES,
						LOOP_BUFFERBYTES_DEFAULT_MIN, LOOP_BUFFERBYTES_DEFAULT_MAX);
	else {
		/*size depends on format and channels*/
		if ((cfg_params->format == SND_PCM_FORMAT_UNKNOWN) || (cfg_params->channels == -1)) {
			SNDERR("format and channels must be defined to set a buffer size");
			return -EINVAL;
		}
		const ssize_t bsize = snd_pcm_format_size(cfg_params->format, cfg_params->channels*cfg_params->buffer_size);
		if (bsize < 0)
			return -EINVAL;
		if ((size_t)bsize > UINT_MAX) {
			SNDERR("buffer size exceeds UINT_MAX for choosen formate and channels");
			return -EINVAL;
		}
		/* cast is safe, becasue of previous checks */
		err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_BUFFER_BYTES,
						(unsigned int)bsize, (unsigned int)bsize);
	}
	if (err < 0)
		return err;

	/*period*/
	if (cfg_params->period_size == -1)
		err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_PERIOD_BYTES,
						LOOP_PERIODBYTES_DEFAULT_MIN, LOOP_PERIODBYTES_DEFAULT_MAX);
	else {
		/*size depends on format and channels*/
		if ((cfg_params->format == SND_PCM_FORMAT_UNKNOWN) || (cfg_params->channels == -1)) {
			SNDERR("format and channels must be defined to set a period size");
			return -EINVAL;
		}
		const ssize_t psize = snd_pcm_format_size(cfg_params->format, cfg_params->channels*cfg_params->period_size);
		if (psize < 0)
			return -EINVAL;
		if ((size_t)psize > UINT_MAX) {
			SNDERR("period size exceeds UINT_MAX for choosen formate and channels");
			return -EINVAL;
		}
		err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_PERIOD_BYTES,
							(unsigned int)psize, (unsigned int)psize);
	}
	if (err < 0)
		return err;

	err = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_PERIODS, 1, LOOP_BUFFERBYTES_DEFAULT_MAX/LOOP_PERIODBYTES_DEFAULT_MIN);
	if (err < 0)
		return err;

	return 0;
}


/*
set master/slave 
no need to restore role on failure - cfg_on_close will get called.
master:
	offer not fixed seetings to user
slave:
	if fixed:
		offer settings to user
	else:
		wait with timeout on fixup
*/
static int loop_cfg_on_open(snd_pcm_loop_t *pcm, const struct loop_params_pref *cfg_params)
{
	int err;

	shm_lock(&pcm->shmp->ctrl);
	if (chk_cfg_master(pcm))
		err = loop_cfg_on_open_m(pcm, cfg_params);
	else {
		int err_mq;
		struct pollfd pfd;
		pfd.fd = pcm->mq_poll;
		pfd.events = POLLIN;
		LOOPDBG("CFG SLAVE");
		while (!chk_cfg_fixed(pcm)) {
			shm_unlock(&pcm->shmp->ctrl);
			LOOPDBG("CFG SLAVE - wait 4 master");
			/*wait until config is fixed or ownership changes.*/
			err = poll(&pfd, 1, LOOP_WAIT_CFG_TMOUT);
			err_mq = consume_mq(pcm);
			if (err == 0)
				return -EINVAL;
			if (err < 0) {
				SYSERR("poll failed");
				return err;
			}
			if (err_mq < 0)
				return err_mq;
			/*May have changed to master*/
			shm_lock(&pcm->shmp->ctrl);
			if (chk_cfg_master(pcm)) {
				LOOPDBG("CFG SLAVE - changed ownership");
				err = loop_cfg_on_open_m(pcm, cfg_params);
				shm_unlock(&pcm->shmp->ctrl);
				return err;
			}
		};
		err = loop_offer_shm_params(pcm, cfg_params);
	}
	shm_unlock(&pcm->shmp->ctrl);

	return err;
}

static int setup_shm_area(snd_pcm_loop_t *pcm)
{
	unsigned int c;
	size_t sample_bits = snd_pcm_format_physical_width(SHM_CFG(pcm).format);
	size_t frame_bits = sample_bits * SHM_CFG(pcm).channels;

	pcm->shm_areas = calloc(SHM_CFG(pcm).channels, sizeof(pcm->shm_areas[0]));
	if (pcm->shm_areas == NULL)
		return -ENOMEM;
	for (c = 0; c < SHM_CFG(pcm).channels; ++c) {
		snd_pcm_channel_area_t *a = &pcm->shm_areas[c];
		if (SHM_CFG(pcm).interleaved) {
			a->addr = SHM_RINGBUF(pcm);
			a->first = c * sample_bits;
			a->step = frame_bits;
		} else {
			a->addr = SHM_RINGBUF(pcm) + (c * SHM_CFG(pcm).buffer_size * (sample_bits/8) );
			a->first = 0;
			a->step = sample_bits;
		}
		LOOPDBG("[%d] addr=%x first %d step %d", c, a->addr, a->first, a->step);
	}
	return 0;
}

/*master:
	export to SHM, notify other side
 slave:
	compare with SHM values
*/
static int loop_cfg_on_hw_params(snd_pcm_loop_t *pcm, snd_pcm_hw_params_t *params)
{
	int err;

	shm_lock(&pcm->shmp->ctrl);
	if (chk_cfg_master(pcm))
		err = loop_export_hw_params(pcm, params);
	else
		err = loop_compare_hw_params(pcm, params);
	shm_unlock(&pcm->shmp->ctrl);
	if (!err)
		err = setup_shm_area(pcm);

	return err;
}

/*called with lock held*/
static int loop_cfg_on_hw_free(snd_pcm_loop_t *pcm, int last_user)
{
	int err = 0;
	if (chk_cfg_master(pcm)) {
		if (last_user)/*recall config if no slave attached*/
			set_cfg_fixed(pcm, 0);
	}

	return err;
}


/*
called with lock held
master:
	move responsibility to other side
slave:
	nothing
*/
static int loop_cfg_on_close(snd_pcm_loop_t *pcm, int last_user)
{
	int err = 0;
	if (chk_cfg_master(pcm)) {
		pcm->shmp->ctrl.cfg_master[pcm->device] = 0;
		if (!last_user) {
			/*move responsibility to other side - config remains fixed if already chosen by master*/
			pcm->shmp->ctrl.cfg_master[pcm->device^1] = 1;
			/*slave may wait for fixup...*/
			err = signal_mq(pcm, SIG_CFG);
		} else {
			set_cfg_fixed(pcm, 0);
		}
	}
	return err;
}

/*read hw params from user config*/
static int loop_config_get_hwparam(const char *id, snd_config_t *n, struct loop_params_pref *params)
{
	int err;
	const char *str;
	if (strcmp(id, "format") == 0) {
		err = snd_config_get_string(n, &str);
		if (err < 0) {
			SNDERR("invalid type for %s", id);
			return err;
		}
		params->format = snd_pcm_format_value(str);
		return 0;
	}
	if (strcmp(id, "channels") == 0) {
		long val;
		err = snd_config_get_integer(n, &val);
		if (err < 0) {
			SNDERR("Invalid type for %s", id);
			return err;
		}
		/* alsa-lib guaranteed val <= INT_MAX. See conf.c:int parse_value().
		   Negative values will not be checked in alsa-lib,
		   but this should be fixed in alsa-lib
		 */
		params->channels = val;
		return 0;
	}
	if (strcmp(id, "rate") == 0) {
		long val;
		err = snd_config_get_integer(n, &val);
		if (err < 0) {
			SNDERR("Invalid type for %s", id);
			return err;
		}
		/* see above block */
		params->rate = val;
		return 0;
	}
	if (strcmp(id, "period_size") == 0) {
		long val;
		err = snd_config_get_integer(n, &val);
		if (err < 0) {
			SNDERR("Invalid type for %s", id);
			return err;
		}
		params->period_size = val;
		return 0;
	}
	if (strcmp(id, "buffer_size") == 0) {
		long val;
		err = snd_config_get_integer(n, &val);
		if (err < 0) {
			SNDERR("Invalid type for %s", id);
			return err;
		}
		params->buffer_size = val;
		return 0;
	}
	if (strcmp(id, "interleaved") == 0) {
		err = snd_config_get_bool(n);
		if (err < 0) {
			SNDERR("Invalid type for %s", id);
			return err;
		}
		params->interleaved = err;
		return 0;
	}

	return 1;
}


/*! pcm plugin loop

 This plugin loopback samples from a playback to a capture stream.
 Samples written to device 0 can be read out on device 1 and vice versa.
 Multiple instances are supported by defining different subdevices.
 The streaming is clockless - reader/writer blocks if no samples are available.
 In general reader and writer are equal - once the first has set a configuration the other is forced to also use that.

\code
pcm.name {
	type loop               # Loop PCM
	device INT		# Device number (default 0 on playback, 1 on capture)
	subdevice INT		# Subdevice number (default 0)
	ipc_perm INT		# IPC permissions on creating IPC obj (octal, default 0600)
	ipc_gid INT/STR		# IPC group on creating IPC obj (id or group name)
	format STR		# format definition
	rate INT		# rate definition
	channels INT		# channels definition
	period_size INT		# in frames
	buffer_size INT 	# in frames
	interleaved BOOL	# force interleaved/noninterleaved access
}
\endcode

*/

SND_PCM_PLUGIN_DEFINE_FUNC(loop)
{
	snd_config_iterator_t i, next;
	int err;
	int ignore_busy = 0;
	snd_pcm_loop_t *pcm;
	long device, subdevice = -1;
	mode_t ipc_perm = LOOP_IPC_PERMS;
	long ipc_gid = -1;
	int uc; /*usecase: playback on device 0 vs capture on device 0*/

	struct loop_params_pref cfg_params = CFG_PARAMS_INIT;
	USE(root);

	device = (stream == SND_PCM_STREAM_PLAYBACK)?0:1;

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0
		    || strcmp(id, "hint") == 0)
			continue;

		if (strcmp(id, "device") == 0) {
			err = snd_config_get_integer(n, &device);
			if (err < 0) {
				SNDERR("Invalid type for %s", id);
				return err;
			}
			continue;
		}

		if (strcmp(id, "subdevice") == 0) {
			err = snd_config_get_integer(n, &subdevice);
			if (err < 0) {
				SNDERR("Invalid type for %s", id);
				return err;
			}
			continue;
		}

		if (strcmp(id, "ipc_perm") == 0) {
			long perm;
			err = snd_config_get_integer(n, &perm);
			if (err < 0) {
				SNDERR("Invalid type for %s", id);
				return err;
			}
			if ((perm & ~0777) != 0) {
				SNDERR("The field ipc_perm must be a valid file permission");
				return -EINVAL;
			}
			/* alsa-lib guaranteed val <= INT_MAX. See conf.c:int parse_value().
			   Negative values will not be checked in alsa-lib,
			   but this should be fixed in alsa-lib
			 */
			ipc_perm = (mode_t)perm;
			continue;
		}

		if (strcmp(id, "ipc_gid") == 0) {
			char *entry;
			err = snd_config_get_ascii(n, &entry);
			if (err < 0) {
				SNDERR("Invalid type for %s", id);
				return err;
			}
			if (isdigit(*entry)) {
				ipc_gid = atol(entry);
			} else {
				struct group *g;
				errno = 0;
				g = getgrnam(entry);
				if (!g) {
					SNDERR("Invalid group %s err %d", entry, errno);
					return errno ? -errno:-EINVAL;
				}
				ipc_gid = g->gr_gid;
			}
			continue;
		}

		if (strcmp(id, "force_reset") == 0) {
			err = snd_config_get_bool(n);
			if (err < 0) {
				SNDERR("Invalid type for %s", id);
				return err;
			}
			ignore_busy = err;
			continue;
		}

		err = loop_config_get_hwparam(id, n, &cfg_params);
		if (err < 0)
			return err;
		if (err == 0)
			continue;

		SNDERR("Unknown field %s", id);
		return -EINVAL;
	}

	if ((device != 0) && (device != 1)) {
		SNDERR("Invalid device %d", device);
		return -EINVAL;
	}
	/*usecase - to differentiate stream on full duplex*/
	/* cast is safe on 64 bit systems as well, because sizeof(enum) == sizeof(unsigned) == 4 byte */
	uc = device ^ (unsigned int)stream;

	/*no support for 'first free' (need to attach to different SHM)*/
	if (subdevice == -1)
		subdevice = 0;

	if (subdevice >= SUBDEVS_MAX) {
		SNDERR("Invalid subdevice %d", subdevice);
		return -EINVAL;
	}

	pcm = calloc(1, sizeof(*pcm));
	if (!pcm) {
		SNDERR("Allocate memory failed");
		return -ENOMEM;
	}
	pcm->mq_sig = (mqd_t)-1;
	pcm->mq_poll = (mqd_t)-1;
	pcm->shmfd = -1;
	/* device can only be between 0 and 1, because of the above assignment.
	   So it is safe to assign it to an integer.
	 */
	pcm->device = device;
	pcm->shmp = MAP_FAILED;

	/*initial creation - only ctrl block available, resize done later*/
	err = create_shm(pcm, device, subdevice, uc, ipc_perm, ipc_gid, ignore_busy);
	if (err < 0) {
		SNDERR("Setup shared memory failed with err %d", err);
		free(pcm);
		return err;
	}

	shm_lock(&pcm->shmp->ctrl);
	shm_reset(pcm, stream);
	shm_unlock(&pcm->shmp->ctrl);

	/*cross connect sig and poll message queue*/
	err = create_mq(pcm->mqsig_name, subdevice, uc, (stream == SND_PCM_STREAM_PLAYBACK) ? 1:0, ipc_perm, ipc_gid); 
	if (err < 0) {
		SNDERR("Create message queue 1 failed with err %d", err);
		goto error;
	}
	pcm->mq_sig = err;
	err = flush_mq(pcm->mq_sig);
	if (err < 0) {
		SNDERR("Flush message queue 1 failed with err %d", err);
		goto error;
	}

	err = create_mq(pcm->mqpoll_name, subdevice, uc, (stream == SND_PCM_STREAM_PLAYBACK) ? 0:1, ipc_perm, ipc_gid);
	if (err < 0) {
		SNDERR("Create message queue 2 failed with err %d", err);
		goto error;
	}
	pcm->mq_poll = err;
	err = flush_mq(pcm->mq_poll);
	if (err < 0) {
		SNDERR("Flush message queue 2 failed with err %d", err);
		goto error;
	}

	snprintf(pcm->loop_name, LOOPNAME_MAX, "ALSA loop Plugin: subdev %d dev %d",
				(unsigned int)subdevice, (unsigned int)device);

	pcm->io.version = SND_PCM_IOPLUG_VERSION;
	pcm->io.name = pcm->loop_name;
	pcm->io.poll_fd = pcm->mq_poll;
	pcm->io.poll_events = POLLIN;
	pcm->io.mmap_rw = 0;
	pcm->io.callback = &loop_callback;
	pcm->io.private_data = pcm;
	pcm->io.flags = SND_PCM_IOPLUG_FLAG_MONOTONIC;

	err = snd_pcm_ioplug_create(&pcm->io, name, stream, mode);
	if (err < 0) {
		SNDERR("Ioplug create failed with err %d", err);
		goto error;
	}
	err = loop_cfg_on_open(pcm, &cfg_params);
	if (err < 0) {
		SNDERR("Init configuration failed with err %d", err);
		/*this calls close !!*/
		(void)snd_pcm_ioplug_delete(&pcm->io);
		return err;
	}

	*pcmp = pcm->io.pcm;
	return 0;

error:
	(void)loop_destroy(pcm);
	return err;
}

/*PRQA: Lint Message 19 : This is mandatory for ALSA lib plugins */
/*lint -save -e19 */
SND_PCM_PLUGIN_SYMBOL(loop);
/*lint -restore */
