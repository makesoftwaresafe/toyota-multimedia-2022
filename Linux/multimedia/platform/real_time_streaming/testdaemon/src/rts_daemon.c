/**
 * \file: rts_daemon.c
 *
 * Test daemon for real time streaming library
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2014 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <values.h>
#include <alsa/asoundlib.h>
#include <sched.h>
#include <sys/signal.h>

#include "rts.h"


#define ERR(st) do{fprintf(stderr, "%s\n",st );exit(1);}while(0)
static unsigned int xfers;
static unsigned int xruns;
static unsigned int do_transfer;
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b) )

#endif

#define MAX_LINE_L 100
#define MAX_ENTRY_L MAX_LINE_L

/*we use fixed format*/
#define RTSD_FORMAT_DEFAULT SND_PCM_FORMAT_S16_LE
#define RTSD_FORMAT_SIZE 2
tRTS h = RTS_HANDLE_INVAL;

struct parse_csv {
	char type[MAX_ENTRY_L];
	char str1[MAX_ENTRY_L];
	char str2[MAX_ENTRY_L];
	char str3[MAX_ENTRY_L];
	char str4[MAX_ENTRY_L];
	char str5[MAX_ENTRY_L];
};


static struct parse_csv * rtsd_parse_entry(unsigned int *num_streams, const char *path, const char *type, unsigned int fields)
{
	struct parse_csv *parsed = NULL;
	struct parse_csv *new_parsed = NULL;
	unsigned int num = 0;
	ssize_t len;
	FILE *f;
	char *line = NULL;
	size_t linelen = 0;
	int ret;

	*num_streams =0;
	fprintf(stdout, "search entries %s in config file %s...\n", type, path);
	f = fopen(path, "r");
	if (!f) {
		fprintf(stdout, "failed opening %s\n", path);
		return NULL;
	}

	while ((len = getline(&line, &linelen,f )) > 0) {
		if (len > MAX_LINE_L) {
			fprintf(stdout, "line too long %zd\n", len);
			break;
		}
		new_parsed = realloc(parsed, (num+1) * sizeof(struct parse_csv));
		if (!new_parsed) {
			fprintf(stdout, "failed allocating mem\n");
			num = 0;
			break;
		}
		parsed = new_parsed;
		ret = sscanf(line,"%[#]", parsed[num].type);
		if (ret){
			continue;
		}
		ret = sscanf(line,"%[^;];%[^;];%[^;];%[^;];%[^;];%[^;];", parsed[num].type,
				parsed[num].str1, parsed[num].str2, parsed[num].str3, parsed[num].str4,parsed[num].str5);
		if (ret < (int)fields) {
			continue;
		}

		if (strcmp(parsed[num].type,type))
			continue;
		num++;
	}
	*num_streams = num;
	free(line);

	return parsed;
}


static int rtsd_parse_streams(trts_cfgstream **streams, unsigned int *num_streams, const char *path)
{
	unsigned int i;
	struct parse_csv *parsed;
	*streams = NULL;
	*num_streams = 0;

	parsed = rtsd_parse_entry(num_streams, path, "STREAM", 3);
	*streams = calloc(*num_streams, sizeof(trts_cfgstream));

	for (i = 0; i<*num_streams; i++) {
		(*streams)[i].adevidx = atoi(parsed[i].str1);
		(*streams)[i].channel = atoi(parsed[i].str2);
	}
	if (parsed)
		free(parsed);

	return 0;
}


static int rtsd_parse_devices(trts_cfgadev **adevs, unsigned  int *num_adevs, char* path)
{
	unsigned int i;
	struct parse_csv *parsed;
	*adevs = NULL;
	*num_adevs = 0;

	parsed = rtsd_parse_entry(num_adevs, path, "ADEV", 6);
	*adevs = calloc(*num_adevs, sizeof(trts_cfgadev));

	for (i = 0; i<*num_adevs; i++) {
		if (strcmp(parsed[i].str2, "IN") && strcmp(parsed[i].str2, "OUT")) {
			fprintf(stdout, "invalid direction %s\n", parsed[i].str2);
			*num_adevs = 0;
			break;
		}
		(*adevs)[i].pcmname = strdup(parsed[i].str1);
		(*adevs)[i].dir=!strcmp(parsed[i].str2, "IN")?SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK;
		(*adevs)[i].rate = atoi(parsed[i].str3);
		(*adevs)[i].period_frames = atoi(parsed[i].str4);
		(*adevs)[i].startup_tout = atoi(parsed[i].str5);
		(*adevs)[i].format = RTSD_FORMAT_DEFAULT;
	}
	if (parsed)
		free(parsed);
	return 0;
}


static void rtsd_process_buffers(char **out_buffers, char **in_buffers,
					unsigned int *out_sizes, unsigned int *in_sizes,
					int num_out, int num_in)
{
	int num;

	/*forward matching buffers*/
	for (num = 0; (num<num_in) && (num < num_out); num++)
		memcpy(out_buffers[num], in_buffers[num], min(in_sizes[num],out_sizes[num]));
	/*silence remaining out buffers - assuming signed format: 0=silence*/
	for (; num < num_out; num++)
		memset(out_buffers[num], 0, out_sizes[num]);
}


static void rtsd_sighandler(int sig)
{
	switch(sig)
	{
	case SIGABRT:
	case SIGINT:
	case SIGTERM:
		do_transfer = 0;
		if (h != RTS_HANDLE_INVAL)
			rts_abort(h);
		break;
	case SIGUSR1:/*statistic*/
		fprintf(stdout, "%d xfers done - %d xruns\n", xfers, xruns);
	}
}


static void rtsd_set_prio(void)
{
	int err;
	struct sched_param sched_param;

	err = sched_getparam(0, &sched_param);
	if (err < 0) {
		if (errno == EPERM) {
			fprintf(stdout, "WARNING: sched_getparam: ignoring missing permission\n");
		} else {
			ERR("sched_getparam error ");
		}
	}
	sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	err = sched_setscheduler(0, SCHED_FIFO, &sched_param);
	if (err < 0) {
		if (errno == EPERM) {
			fprintf(stdout, "WARNING: sched_setscheduler: ignoring missing permission\n");
		} else {
			ERR("sched_setscheduler error priority SCHED_FIFO ");
		}
	}
	/*fprintf(stdout, "SET TO PRIO %d \n", sched_param.sched_priority);*/

	return;
}


static void rtsd_statistic(tRTS rts_handle)
{
	static unsigned int num_xruns = 0;
	trts_stat stat;

	xfers++;

	if (num_xruns>=20)
		stat.clear = 1;
	else
		stat.clear = 0;
	
	rts_statistic(rts_handle, &stat);
	if (stat.num_xruns && (stat.num_xruns != num_xruns)) {
		fprintf(stdout, "statistic: %d xruns\n", stat.num_xruns);
		xruns++;
	}
	num_xruns = stat.num_xruns;
}


static void usage(void)
{
	fprintf(stdout, "\n");
	fprintf(stdout, "Usage: rtsd FILE [FILE2]\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "If only one file is given: device and streams are searched in that file\n");
	fprintf(stdout, "If 2 files are given: devices are taken from 1st file, streams from 2nd file\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "Layout of configuration file:\n");
	fprintf(stdout, "ADEV;<pcm_name>;<direction>;<rate>;<period_size>;<init_timeout>\n");
	fprintf(stdout, "STREAM;<adev_idx>;<channel>\n");
	fprintf(stdout, "-no spaces in between\n");
	fprintf(stdout, "-direction must be IN or OUT\n");
	fprintf(stdout, "-no ticks ' \" \n");
	fprintf(stdout, "example:\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "ADEV;AdevMicro1In;IN;16000;256;1000\n");
	fprintf(stdout, "ADEV;AdevTxOut;OUT;16000;256;0\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "STREAM;0;0;\n");
	fprintf(stdout, "STREAM;0;1;\n");
	fprintf(stdout, "STREAM;1;0;\n");
	fprintf(stdout, "STREAM;1;1;\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "This defines following:\n");
	fprintf(stdout, "--Input device AdevMicro1In, rate 16000Hz, period size of 256frames, init timeout 1000ms\n");
	fprintf(stdout, "--Output device AdevTxOut, rate 16000Hz, period size of 256frames, default init timeout\n");
	fprintf(stdout, "--Input stream on device 0, channel 0\n");
	fprintf(stdout, "--Input stream on device 0, channel 1\n");
	fprintf(stdout, "--Output stream on device 1, channel 0\n");
	fprintf(stdout, "--Output stream on device 1, channel 1\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\n");
}


#define MAX_STREAMS_PER_DIR 10
int main(int argc, char** argv)
{
	
	trts_cfg cfg;
	unsigned int i;
	unsigned int in =0;
	unsigned int out =0;
	int err;
	char* in_buffers[MAX_STREAMS_PER_DIR];
	char* out_buffers[MAX_STREAMS_PER_DIR];
	unsigned int in_size[MAX_STREAMS_PER_DIR];
	unsigned int out_size[MAX_STREAMS_PER_DIR];
	static trts_cfgadev *adevs; 
	static trts_cfgstream *streams;
	unsigned int num_adevs;
	unsigned int num_streams;
	char *path_dev;
	char *path_stream;
	unsigned int ptime=0;
	struct sigaction new_action;
 	sigemptyset (&new_action.sa_mask);
  	new_action.sa_flags = 0;
	new_action.sa_handler = rtsd_sighandler;

	/*minimum arg: path*/
	if (argc<2) {
		fprintf(stdout,"ERROR: no path given\n");
		usage();
		ERR("...exiting");
	}

	if (argc>3) {
		fprintf(stdout,"ERROR: additional args given\n");
		usage();
		ERR("...exiting");
	}

	path_dev = argv[1];
	path_stream = argv[1];
	if (argc > 2)
		path_stream = argv[2];

	if (rtsd_parse_devices(&adevs, &num_adevs, path_dev)<0)
		ERR("failed parsing devices");

	if (rtsd_parse_streams(&streams, &num_streams, path_stream)<0)
		ERR("failed parsing streams");

	/*
	fprintf(stdout, "num streams %d num_devices %d\n", num_streams, num_adevs);

	if (num_streams){
		for (i = 0; i<num_streams; i++) {
			fprintf(stdout, "streams %d devices %d idx %d\n", i, streams[i].adevidx, streams[i].channel);
		}		
	}
	*/
	cfg.features = 0;
	//cfg.features |= RTS_FEAT_INJECT_XRUN;
	//cfg.features |= RTS_FEAT_DISABLE_XRUN_DETECT;
	cfg.prefill_ms = 5;
	cfg.adevs = adevs;
	cfg.num_adevs = num_adevs;
	cfg.streams = streams;
	cfg.num_streams = num_streams;

	for (i = 0; i<cfg.num_streams; i++) {
		unsigned int adevidx = cfg.streams[i].adevidx;
		unsigned int ptime_tmp;

		if (adevidx>=num_adevs)
			ERR("INVALID DEVICE INDEX");

		ptime_tmp = (cfg.adevs[adevidx].period_frames *1000)/
					cfg.adevs[adevidx].rate;
		if (!ptime)
			ptime = ptime_tmp;
		else if (ptime != ptime_tmp)
			ERR("DIFFERENT PERIOD TIMES NOT SUPPORTED");
		
		if (cfg.adevs[adevidx].dir == SND_PCM_STREAM_PLAYBACK) {
			out_size[out] = cfg.adevs[adevidx].period_frames*RTSD_FORMAT_SIZE;
			out_buffers[out] = malloc(out_size[out]);
			if(!out_buffers[out])
				ERR("MALLOC FAILED");	
			memset(out_buffers[out], 0, out_size[out]);
			out++;
		} else {
			in_size[in] = cfg.adevs[adevidx].period_frames*RTSD_FORMAT_SIZE;
			in_buffers[in] = malloc(in_size[in]);
			if(!in_buffers[in])
				ERR("MALLOC FAILED");	
			memset(in_buffers[in], 0, in_size[in]);
			in++;
		}
	}

	rtsd_set_prio();

	do_transfer = 1;
	sigaction(SIGUSR1, &new_action, NULL);
	sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGTERM, &new_action, NULL);

	err = rts_create( &h, &cfg);
	if (err)
		ERR("CREATE FAILED");

	while ((err == 0)&& (do_transfer)) {
		err = rts_read(h, (void**)in_buffers);
		if (err) {
			fprintf(stdout,"READ FAILED\n");
			break;
		}
		rtsd_process_buffers(out_buffers, in_buffers, out_size, in_size, out, in);
		err = rts_write(h, (void**)out_buffers);
		if (err) {
			fprintf(stdout,"WRITE FAILED\n");
			break;
		}
		rtsd_statistic(h);
	}

	err = rts_destroy(h);
	h = RTS_HANDLE_INVAL;

	sigdelset (&new_action.sa_mask, SIGUSR1);
	sigdelset (&new_action.sa_mask, SIGINT);
	sigdelset (&new_action.sa_mask, SIGTERM);

	return err;
}

