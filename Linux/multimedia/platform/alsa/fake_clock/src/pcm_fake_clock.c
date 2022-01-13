/**
 * \file: pcm_fake_clock.c
 *
 * fake_clock plugin for ALSA.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <time.h>
#include <limits.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include <alsa/control.h>
//#define FCLOCK_DEBUG


#define CTRL_RATE 0
#define CTRL_JITTER 1
#define CTRL_DELAY 2

#define NUM_CTRLS 3


struct plugin_ctl_init {
	char *name;
	int range_min;
	int range_max;
	int val_default;
	int store;
};


struct plugin_ctl {
	snd_ctl_t *ctl;
	snd_ctl_elem_value_t *elem;
	int cur_val;
};

typedef struct {
        snd_pcm_extplug_t ext;
	snd_pcm_uframes_t psize;
	unsigned int ptime;

	struct timespec t_interval_start;
	snd_pcm_uframes_t interval_xfer;
	unsigned int interval_rate;
#ifdef FCLOCK_DEBUG
	struct timespec t_start;
	snd_pcm_uframes_t xfer;
	unsigned int rate;
#endif
	struct plugin_ctl ctl[NUM_CTRLS];

	unsigned int period_mul;
	unsigned int period_cnt;
	int t_valid;
	unsigned int rate_requested;
	unsigned int rate_current;
	signed int rate_add;	
	signed long jitter_sum_us;
}snd_pcm_fake_clock_t;

static struct plugin_ctl_init ctrls[NUM_CTRLS] = {
	[CTRL_RATE]={
		.name = "RATE_ADJUST",
		.range_min = -10000,
		.range_max = 10000,
		.val_default = 0
	},
	[CTRL_JITTER]={
		.name = "JITTER_ADJUST",
		.range_min = 0,
		.range_max = 10000,
		.val_default = 0
	},
	[CTRL_DELAY]={
		.name = "DELAY_ADJUST",
		.range_min = 0,
		.range_max = 10000,
		.val_default = 0
	}
};


static signed long ts_diff_us(struct timespec *before, struct timespec *now)
{
    struct timespec temp;
	if ((now->tv_nsec-before->tv_nsec)<0) {
		temp.tv_sec = now->tv_sec-before->tv_sec-1;
		temp.tv_nsec = 1000000000+now->tv_nsec-before->tv_nsec;
	} else {
		temp.tv_sec = now->tv_sec-before->tv_sec;
		temp.tv_nsec = now->tv_nsec-before->tv_nsec;
	}
	return (temp.tv_nsec/1000)+(temp.tv_sec*1000*1000);
}
#ifdef FCLOCK_DEBUG
static signed long ts_diff_ms(struct timespec *before, struct timespec *now)
{
    struct timespec temp;
	if ((now->tv_nsec-before->tv_nsec)<0) {
		temp.tv_sec = now->tv_sec-before->tv_sec-1;
		temp.tv_nsec = 1000000000+now->tv_nsec-before->tv_nsec;
	} else {
		temp.tv_sec = now->tv_sec-before->tv_sec;
		temp.tv_nsec = now->tv_nsec-before->tv_nsec;
	}
	return (temp.tv_nsec/(1000*1000))+(temp.tv_sec*1000);
}
#endif

/*range [-1 ... +1]*/
static double normrand(void)
{
	return ((double)rand()/RAND_MAX*2.0-1.0)*((double)rand()/RAND_MAX*2.0-1.0);
}


static signed long attach_delay(snd_pcm_fake_clock_t *clk, signed long delay_us)
{
	/*ctrl range 0->10000 maps to 0..1 period_time*/
	signed int delay;
	double rval = (normrand()+1.0)/2; /*-1..+1 --> 0..1*/
	if (!clk->ctl[CTRL_DELAY].cur_val)
		return delay_us;
	delay = (signed int)(rval*clk->ptime*(clk->ctl[CTRL_DELAY].cur_val)/10000);
	delay *= clk->period_mul;
	return delay_us+delay;
}


static signed long attach_jitter(snd_pcm_fake_clock_t *clk, signed long delay_us)
{
	/*ctrl range 0->10000 maps to 0..1 period_time*/
	signed int jitter;
	if (!clk->ctl[CTRL_JITTER].cur_val)
		return delay_us;

	jitter = (signed int)(normrand()*clk->ptime*(clk->ctl[CTRL_JITTER].cur_val)/10000);

	if (clk->jitter_sum_us+jitter < -(signed long)clk->ptime*(clk->ctl[CTRL_JITTER].cur_val)/10000) {
		jitter = 0;
	} else if (clk->jitter_sum_us+jitter > (signed long)clk->ptime*(clk->ctl[CTRL_JITTER].cur_val)/10000) {
		jitter = 0;
	}
	clk->jitter_sum_us += jitter;
	jitter *= clk->period_mul;
//	fprintf(stderr,"jitter: %d - sum %ld (ctl %d) \n", jitter,clk->jitter_sum_us, clk->ctl[CTRL_JITTER].cur_val);

	return delay_us+jitter;
}


#define RATE_FAC 1000
static int update_requested_rate(snd_pcm_extplug_t *ext, snd_pcm_fake_clock_t *clk, struct plugin_ctl *ctrl)
{
	unsigned int new_rate;

	new_rate = (unsigned int)((ext->rate*RATE_FAC) + (((float)ctrl->cur_val/10000) * ((float)ext->rate * RATE_FAC/10) ));
	if (new_rate !=clk->rate_requested) {
		if(!clk->rate_requested) 
			clk->rate_current = new_rate;
		/*constant adjust of full range within 5s*/
		clk->rate_add = ((int64_t)ext->rate * clk->ptime / 50000);
#ifdef FCLOCK_DEBUG
		fprintf(stderr,"RATE CHG: OLD %d/%d NEW %d/%d CURR %d/%d->add %d\n",clk->rate_requested, clk->rate_requested/RATE_FAC, new_rate, new_rate/RATE_FAC,clk->rate_current,clk->rate_current/RATE_FAC, clk->rate_add);
#endif
		clk->rate_requested = new_rate;
	}
	if (clk->rate_current != clk->rate_requested) {
#ifdef FCLOCK_DEBUG
		fprintf(stderr,"RATE MOD: OLD %d/%d NEW %d/%d\n",clk->rate_requested, clk->rate_requested/RATE_FAC, clk->rate_current, clk->rate_current/RATE_FAC);
#endif
		if (clk->rate_requested>clk->rate_current)
			if((clk->rate_current+(clk->rate_add))>=clk->rate_requested)
				clk->rate_current = clk->rate_requested;
			else
				clk->rate_current+=clk->rate_add;
		else
			if((clk->rate_current-(clk->rate_add))<=clk->rate_requested)
				clk->rate_current = clk->rate_requested;
			else
				clk->rate_current-=clk->rate_add;
		return 1;
	}
	return 0;
}

/*
 * read current settings from kernel
 */
static void read_current(struct plugin_ctl *ctl)
{
	int err = snd_ctl_elem_read(ctl->ctl, ctl->elem);
	if (err < 0) {
		SNDERR("ELEM READ FAILED: %d", err);
		return;
	}

	const long val = snd_ctl_elem_value_get_integer(ctl->elem, 0);
	if (val > INT_MAX || val < INT_MIN) {
		SNDERR("ELEM VALUE OUT OF RANGE: %ld", val);
		return;
	}
	ctl->cur_val = val;
}

static int update_ctrls(snd_pcm_extplug_t *ext, snd_pcm_fake_clock_t *clk)
{
	int ctl_no;
	for (ctl_no = 0; ctl_no < NUM_CTRLS; ctl_no++)
		read_current(&clk->ctl[ctl_no]);
	update_requested_rate(ext, clk, &clk->ctl[CTRL_RATE]);
	return 0;
}


#define INTERVAL_LEN 1
#define INTERVAL_XFER_FAC 100
static snd_pcm_uframes_t fake_clock_delay(snd_pcm_extplug_t *ext, snd_pcm_fake_clock_t *clk, snd_pcm_uframes_t size)
{
	signed long delay_us;
	struct timespec t_cur;
	signed long interval_running_us;
#ifdef FCLOCK_DEBUG
	signed long running_ms;//overall counter in ms to avoid overrun
#endif
	if (size > clk->psize)
		size = clk->psize;		

	clock_gettime(CLOCK_MONOTONIC, &t_cur);
	if (!clk->t_valid) {
		clk->t_valid = 1;
#ifdef FCLOCK_DEBUG
		clk->t_start = t_cur;
#endif
		clk->t_interval_start = t_cur;
	}

#ifdef FCLOCK_DEBUG
	clk->xfer += size;
#endif
	clk->interval_xfer += size*INTERVAL_XFER_FAC;
	clk->period_cnt++;
	if((clk->period_cnt % clk->period_mul) != 0) {
		return size;
	}
	clk->period_cnt = 0;

#ifdef FCLOCK_DEBUG
	running_ms = ts_diff_ms(&clk->t_start, &t_cur);
#endif
	interval_running_us = ts_diff_us(&clk->t_interval_start, &t_cur);

	/*interpolate time which should have elapsed*/
	delay_us = (((int64_t)clk->interval_xfer)*1000*1000*RATE_FAC/((int64_t)clk->rate_current*INTERVAL_XFER_FAC))-interval_running_us;
	//fprintf(stderr, "burst: ->delay %ld + %d (xfer %ld rate %d running %ld \n",delay_us, (clk->period_mul-1)* clk->ptime,clk->interval_xfer,clk->rate_current, interval_running_us);
	/*negative delay happens if application calls us later than expected - ignore*/
	if(delay_us <0)
		delay_us = 0;

	/*calc effective rate..(extrapolated status after usleep())*/
	interval_running_us += delay_us;
	clk->interval_rate = (((int64_t)(clk->interval_xfer/INTERVAL_XFER_FAC) * 1000 * 1000 * RATE_FAC)/interval_running_us);

#ifdef FCLOCK_DEBUG
	running_ms += delay_us/1000;
	if(running_ms)
		clk->rate = (((int64_t)(clk->xfer) * 1000 * RATE_FAC)/running_ms);
	fprintf(stderr,"ALL: rate: %d,%03d xfer: %ld/%ldms INTERVAL:rate: %d,%03d xfer: %ld/%ldus --- delay %ld req: %d,%03d\n",
		clk->rate/RATE_FAC, clk->rate%RATE_FAC, clk->xfer, running_ms, clk->interval_rate/RATE_FAC, clk->interval_rate%RATE_FAC, clk->interval_xfer, interval_running_us, delay_us,
		clk->rate_current/RATE_FAC,clk->rate_current%RATE_FAC);
#endif

	delay_us = attach_jitter(clk, delay_us);
	delay_us = attach_delay(clk, delay_us);

	if (delay_us > 0) {
//		fprintf(stderr, "sleep %ld\n", delay_us);
		usleep(delay_us);
	}else{
		fprintf(stderr, "APP TOO LATE !!\n");
	}

	/*window of INTERVAL_LENs - each second slided by 1s*/
	if ((clk->interval_xfer)>=(((int64_t)clk->interval_rate*INTERVAL_XFER_FAC)/RATE_FAC)*INTERVAL_LEN) {
		//reached every second...
//fprintf(stderr, "slide window.. xfer %ld - %lld -> %lld\n",clk->interval_xfer, (((int64_t)clk->interval_rate*INTERVAL_XFER_FAC)/RATE_FAC),clk->interval_xfer - (((int64_t)clk->interval_rate*INTERVAL_XFER_FAC)/RATE_FAC) );
		clk->interval_xfer -= (((int64_t)clk->interval_rate*INTERVAL_XFER_FAC)/RATE_FAC);
		clk->t_interval_start.tv_sec++;
	}
	update_ctrls(ext, clk);

	return size;
}

static snd_pcm_sframes_t fake_clock_transfer( snd_pcm_extplug_t *ext,
				const snd_pcm_channel_area_t *dst_areas, snd_pcm_uframes_t dst_offset,
				const snd_pcm_channel_area_t *src_areas, snd_pcm_uframes_t src_offset,
				snd_pcm_uframes_t size)
{
	snd_pcm_fake_clock_t *clk = (snd_pcm_fake_clock_t *)ext;
	unsigned int ch;

	size = fake_clock_delay(ext, clk, size);
	for (ch = 0; ch < ext->channels; ch++)
		snd_pcm_area_copy(&dst_areas[ch], dst_offset, &src_areas[ch], src_offset, size, ext->slave_format);

	return size;
}

static int fake_clock_init(snd_pcm_extplug_t *ext)
{
	snd_pcm_fake_clock_t *clk = (snd_pcm_fake_clock_t *)ext;

	if (ext->channels != ext->slave_channels) {
		SNDERR("channels conversion not supported: %d != %d", ext->channels, ext->slave_channels);
		return -EINVAL;
	}
	if (ext->format != ext->slave_format) {
		SNDERR("format conversion not supported: %d != %d", ext->format, ext->slave_format);
		return -EINVAL;
	}
	clk->rate_requested = ext->rate*RATE_FAC;
	clk->rate_current = clk->rate_requested;
	update_ctrls(ext, clk);

	clk->jitter_sum_us = 0;
	clk->t_valid = 0;
#ifdef FCLOCK_DEBUG
	clk->xfer = 0;
#endif
	clk->interval_xfer = 0;

	clk->period_cnt = 0;
	return 0;
}

static int fake_clock_hw_params(snd_pcm_extplug_t *ext, snd_pcm_hw_params_t *params)
{
	snd_pcm_fake_clock_t *clk = (snd_pcm_fake_clock_t *)ext;
	snd_pcm_hw_params_get_period_size(params, &clk->psize, NULL);
	snd_pcm_hw_params_get_period_time(params, &clk->ptime, NULL);
	return 0;
}

static int fake_clock_close(snd_pcm_extplug_t *ext)
{
	snd_pcm_fake_clock_t *clk = (snd_pcm_fake_clock_t *)ext;
	free(clk);
	return 0;
}

static const snd_pcm_extplug_callback_t fake_clock_callback = {
        .transfer = fake_clock_transfer,
        .init = fake_clock_init,
        .hw_params = fake_clock_hw_params,
        .close = fake_clock_close,
};


static int get_card(snd_pcm_t *pcm)
{
	snd_pcm_info_t *info;//to get hw device
	snd_pcm_info_alloca(&info);
	int err;
	err = snd_pcm_info(pcm, info);
	if (err < 0)
		return err;
	err = snd_pcm_info_get_card(info);
	if (err < 0) {
		SNDERR("No card defined for control");
		return -EINVAL;
	}
	return err;
}
/*
 * load and set up user-control
 * returns 0 if the user-control is found or created,
 * returns 1 if the control is a hw control,
 * or a negative error code
 */
static int load_control(snd_pcm_t *pcm, struct plugin_ctl *ctl, struct plugin_ctl_init *ctlinit, int ctl_card)
{
	char tmp_name[32];
	int err;

	snd_ctl_elem_id_t *ctl_id;
	snd_ctl_elem_info_t *cinfo;

	/*get responsible card and open ctl*/
	if (ctl_card <0 ) {
		ctl_card = get_card(pcm);
		if (ctl_card < 0)
			return ctl_card;
	}
	sprintf(tmp_name, "hw:%d", ctl_card);

reopen:
	err = snd_ctl_open(&ctl->ctl, tmp_name, 0);
	if (err < 0) {
		SNDERR("Cannot open CTL %s", tmp_name);
		return err;
	}
	/*only set name??*/
	snd_ctl_elem_id_alloca(&ctl_id);
	snd_ctl_elem_id_set_numid(ctl_id, 0);
	snd_ctl_elem_id_set_interface(ctl_id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_device(ctl_id, 0);
	snd_ctl_elem_id_set_subdevice(ctl_id, 0);
	snd_ctl_elem_id_set_name(ctl_id, ctlinit->name);
	snd_ctl_elem_id_set_index(ctl_id, 0);

	snd_ctl_elem_info_alloca(&cinfo);
	snd_ctl_elem_info_set_id(cinfo, ctl_id);

	snd_ctl_elem_value_malloc(&ctl->elem);
	snd_ctl_elem_value_set_id(ctl->elem, ctl_id);

	/*check if existing*/
	if ((err = snd_ctl_elem_info(ctl->ctl, cinfo)) < 0) {
		if (err != -ENOENT) {
			SNDERR("Cannot get info for CTL %s", tmp_name);
			return err;
		}
		err = snd_ctl_elem_add_integer(ctl->ctl, ctl_id, 1, ctlinit->range_min, ctlinit->range_max, 1);
		if (err < 0) {
			SNDERR("ELEM ADD INTEGER FAILED");
			return err;
		}
		snd_ctl_elem_value_set_integer(ctl->elem, 0, ctlinit->val_default);
		err = snd_ctl_elem_write(ctl->ctl, ctl->elem);
		if (err < 0) {
			SNDERR("Cannot add a control");
			return err;
		}
		/*new elements are locked in kernel - reopen it to make them immediately accessible by others...*/
		snd_ctl_close(ctl->ctl);
		goto reopen;

	}else{
#ifdef FCLOCK_DEBUG
		fprintf(stderr,"ELEM INFO ALREADY EXISTING\n");
#endif

		if (ctlinit->store != 0) {
			snd_ctl_elem_value_set_integer(ctl->elem, 0, ctlinit->val_default);
			err = snd_ctl_elem_write(ctl->ctl, ctl->elem);
			if (err < 0) {
				SNDERR("Cannot add a control");
				return err;
			}
		}


	}
	return 0;
}

/*!  pcm_plugins

\section Plugin: fake_clock

General:

\code
pcm.name {
	type fake_clock         # fake_clock PCM
        slave STR               # Slave name
        # or
        slave {                 # Slave definition
                pcm STR         # Slave PCM name
                # or
                pcm { }         # Slave PCM definition
        }
	jitter_def		#default jitter in range 0..10000 = 0..1 period time
	delay_def		#default delay  in range 0..10000 = 0..1 period time
	rate_def		#default rate adjustment in range +/-10000 = +/-10%
}

\endcode

*/

/**
 * \brief Creates a new fake_clock PCM
 * \param pcmp Returns created PCM handle
 * \param name Name of PCM
 * \param root Root configuration node
 * \param conf Configuration node with fake_clock PCM description
 * \param stream Stream type
 * \param mode Stream mode
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
SND_PCM_PLUGIN_DEFINE_FUNC(fake_clock)
{
        snd_config_iterator_t i, next;
        snd_pcm_fake_clock_t *clk;
        snd_config_t *sconf = NULL;
	snd_config_t *control = NULL;
        int err;
	int ctl_no;
	int card = -1;
	snd_ctl_elem_id_t *ctl_id;

	const snd_pcm_format_t formats[] = {
		SND_PCM_FORMAT_S8,SND_PCM_FORMAT_U8,
		SND_PCM_FORMAT_S16_LE, SND_PCM_FORMAT_S16_BE, SND_PCM_FORMAT_U16_LE, SND_PCM_FORMAT_U16_BE,
		SND_PCM_FORMAT_S32_LE, SND_PCM_FORMAT_S32_BE, SND_PCM_FORMAT_U32_LE, SND_PCM_FORMAT_U32_BE
 	};

        clk = calloc(1, sizeof(snd_pcm_fake_clock_t));
        if (!clk)
                return -ENOMEM;

	clk->period_mul = 1;

        snd_config_for_each(i, next, conf) {
                snd_config_t *n = snd_config_iterator_entry(i);
                const char *id;
                if (snd_config_get_id(n, &id) < 0)
                        continue;
		if ((strcmp(id, "comment") == 0) || (strcmp(id, "type") == 0) || (strcmp(id, "hint") == 0))
			continue;
		if (strcmp(id, "control") == 0) {
			control = n;
			continue;
		}
                if (strcmp(id, "slave") == 0) {
                        sconf = n;
                        continue;
                }
                if (strcmp(id, "jitter_def") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0) {
				free(clk);
				return err;
			}
			/* alsa-lib guaranteed val <= INT_MAX. See conf.c:int parse_value().
			   Negative values will not be checked in alsa-lib,
			   but this should be fixed in alsa-lib
			 */
			ctrls[CTRL_JITTER].val_default = val;
			ctrls[CTRL_JITTER].store = 1;
			continue;
                }
                if (strcmp(id, "rate_def") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0) {
				free(clk);
				return err;
			}
			/* alsa-lib guaranteed val <= INT_MAX. See conf.c:int parse_value().
			   Negative values will not be checked in alsa-lib,
			   but this should be fixed in alsa-lib
			 */
			ctrls[CTRL_RATE].val_default = val;
			ctrls[CTRL_RATE].store = 1;
			continue;
                }

                if (strcmp(id, "delay_def") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0) {
				free(clk);
				return err;
			}
			/* alsa-lib guaranteed val <= INT_MAX. See conf.c:int parse_value().
			   Negative values will not be checked in alsa-lib,
			   but this should be fixed in alsa-lib
			 */
			ctrls[CTRL_DELAY].val_default = val;
			ctrls[CTRL_DELAY].store = 1;
			continue;
                }

                if (strcmp(id, "period_mul") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0) {
				free(clk);
				return err;
			}
			if (val<= 0) {
				free(clk);
				return -EINVAL;
			}
			clk->period_mul = val;
			continue;
                }

                SNDERR("Unknown field %s", id);
		free(clk);
                return -EINVAL;
        }

        if (!sconf) {
                SNDERR("No slave configuration for fake_clock pcm");
		free(clk);
                return -EINVAL;
        }

	snd_ctl_elem_id_alloca(&ctl_id);
	if (control != NULL) {
		if ((err = snd_pcm_parse_control_id(control, ctl_id, &card, NULL, NULL)) < 0) {
			free(clk);
			return err;
		}
	}
	clk->ext.name = "fake_clock Plugin";
	clk->ext.version = SND_PCM_EXTPLUG_VERSION;
	clk->ext.callback = &fake_clock_callback;
	clk->ext.private_data = clk;

	err = snd_pcm_extplug_create(&clk->ext, name, root, sconf, stream, mode);
	if (err < 0) {
		SNDERR("Create extplug failed with %d", err);
		free(clk);
		return err;
	}
	snd_pcm_extplug_set_param_list(&clk->ext, SND_PCM_EXTPLUG_HW_FORMAT, sizeof(formats)/sizeof(snd_pcm_format_t), (const unsigned int*)formats);
#ifdef PCM_DELAY_WITHOUT_HF_027_ALSA_MISC
	snd_pcm_extplug_set_slave_param_list(&clk->ext, SND_PCM_EXTPLUG_HW_FORMAT, sizeof(formats)/sizeof(snd_pcm_format_t), (const unsigned int*)formats);
#endif

	for (ctl_no = 0; ctl_no < NUM_CTRLS; ctl_no++)
		load_control(clk->ext.pcm, &clk->ctl[ctl_no], &ctrls[ctl_no], card);

	*pcmp = clk->ext.pcm;
	return 0;
}
/*PRQA: Lint Message 19 : This is mandatory for ALSA lib plugins */
/*lint -save -e19 */
SND_PCM_PLUGIN_SYMBOL(fake_clock);
/*lint -restore */
