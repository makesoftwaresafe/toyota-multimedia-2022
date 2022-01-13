/******************************************************************
 * CopyPolicy: GNU Lesser General Public License 2.1 applies
 * Copyright (C) 1998-2008 Monty xiphmont@mit.edu
 * 
 * internal include file for cdda interface kit for Linux 
 *
 ******************************************************************/

#ifndef _cdda_low_interface_
#define _cdda_low_interface_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/major.h>
#include <linux/version.h>

/* some include file locations have changed with newer kernels */

#ifdef SBPCD_H
#include <linux/sbpcd.h>
#endif

#ifdef UCDROM_H
#include <linux/ucdrom.h>
#endif

#ifndef CDROMAUDIOBUFSIZ      
#define CDROMAUDIOBUFSIZ        0x5382 /* set the audio buffer size */
#endif

#ifndef CDROM_GET_CAPABILITY
#define CDROM_GET_CAPABILITY    0x5331 /* get CDROM capabilites if CDROM */
#endif


#include <scsi/sg.h>
#include <scsi/scsi.h>

#include <linux/cdrom.h>
#include <linux/major.h>

#include "cdda_interface.h"

#ifndef SG_EMULATED_HOST
/* old kernel version; the check for the ioctl is still runtime, this
   is just to build */
#define SG_EMULATED_HOST 0x2203
#define SG_SET_TRANSFORM 0x2204
#define SG_GET_TRANSFORM 0x2205
#endif

#ifndef SG_IO
/* old kernel version; the usage is all runtime-safe, this is just to
   build */
typedef struct sg_io_hdr
{
  int interface_id;           /* [i] 'S' for SCSI generic (required) */
  int dxfer_direction;        /* [i] data transfer direction  */
  unsigned char cmd_len;      /* [i] SCSI command length ( <= 16 bytes) */
  unsigned char mx_sb_len;    /* [i] max length to write to sbp */
  unsigned short int iovec_count; /* [i] 0 implies no scatter gather */
  unsigned int dxfer_len;     /* [i] byte count of data transfer */
  void * dxferp;              /* [i], [*io] points to data transfer memory
                                 or scatter gather list */
  unsigned char * cmdp;       /* [i], [*i] points to command to perform */
  unsigned char * sbp;        /* [i], [*o] points to sense_buffer memory */
  unsigned int timeout;       /* [i] MAX_UINT->no timeout (unit: millisec) */
  unsigned int flags;         /* [i] 0 -> default, see SG_FLAG... */
  int pack_id;                /* [i->o] unused internally (normally) */
  void * usr_ptr;             /* [i->o] unused internally */
  unsigned char status;       /* [o] scsi status */
  unsigned char masked_status;/* [o] shifted, masked scsi status */
  unsigned char msg_status;   /* [o] messaging level data (optional) */
  unsigned char sb_len_wr;    /* [o] byte count actually written to sbp */
  unsigned short int host_status; /* [o] errors from host adapter */
  unsigned short int driver_status;/* [o] errors from software driver */
  int resid;                  /* [o] dxfer_len - actual_transferred */
  unsigned int duration;      /* [o] time taken by cmd (unit: millisec) */
  unsigned int info;          /* [o] auxiliary information */
} sg_io_hdr_t;


#endif

struct cdda_private_data {
  struct sg_header *sg_hd;
  unsigned char *sg_buffer; /* points into sg_hd */
  clockid_t clock;
  int last_milliseconds;
};

#define MAX_RETRIES 8
#define MAX_BIG_BUFF_SIZE 65536
#define MIN_BIG_BUFF_SIZE 4096
#define SG_OFF sizeof(struct sg_header)

extern int  cooked_init_drive (cdrom_drive *d);
extern unsigned char *scsi_inquiry (cdrom_drive *d);
extern int  scsi_init_drive (cdrom_drive *d);
#ifdef CDDA_TEST
extern int  test_init_drive (cdrom_drive *d);
#endif
#endif

