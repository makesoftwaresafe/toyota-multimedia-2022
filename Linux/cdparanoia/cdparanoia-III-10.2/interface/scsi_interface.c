/******************************************************************
 * CopyPolicy: GNU Lesser General Public License 2.1 applies
 * Original interface.c Copyright (C) 1994-1997 
 *            Eissfeldt heiko@colossus.escape.de
 * Current incarnation Copyright (C) 1998-2008 Monty xiphmont@mit.edu
 * 
 * Generic SCSI interface specific code.
 *
 ******************************************************************/

#include "low_interface.h"
#include "common_interface.h"
#include "utils.h"
#include <time.h>
static int timed_ioctl(cdrom_drive *d, int fd, int command, void *arg){
  struct timespec tv1;
  struct timespec tv2;
  int ret1=clock_gettime(d->impl->clock,&tv1);
  int ret2=ioctl(fd, command,arg);
  int ret3=clock_gettime(d->impl->clock,&tv2);
  if(ret1<0 || ret3<0){
    d->impl->last_milliseconds=-1;
  }else{
    d->impl->last_milliseconds = (tv2.tv_sec-tv1.tv_sec)*1000. + (tv2.tv_nsec-tv1.tv_nsec)/1000000.;
  }
  return ret2;
}

/* hook */
static int Dummy (cdrom_drive *d,int s){
  return(0);
}

#include "drive_exceptions.h"
static void tweak_SG_buffer(cdrom_drive *d) {
  int table, reserved, cur, err;
  char buffer[256];

  /* SG_SET_RESERVED_SIZE doesn't actually allocate or reserve anything.
   * what it _does_ do is give you an error if you ask for a value
   * larger than q->max_sectors (the length of the device's bio request
   * queue).  So we walk it up from 1 sector until it fails, then get
   * the value we set it to last.
   */
  /* start with 2 frames, round down to our queue's sector size */
  cur = 1;
  do {
    cur <<= 1; reserved = cur * (1<<9);
    err = ioctl(d->cdda_fd, SG_SET_RESERVED_SIZE, &reserved);
  } while(err >= 0 && (cur*(1<<9) < 0x40000000));
  ioctl(d->cdda_fd, SG_GET_RESERVED_SIZE, &reserved);

  /* this doesn't currently ever work, but someday somebody might
     implement working sg lists with SG_IO devices, so who knows... */
  if (ioctl(d->cdda_fd, SG_GET_SG_TABLESIZE, &table) < 0)
    table=1;

  sprintf(buffer,"\tDMA scatter/gather table entries: %d\n\t"
	  "table entry size: %d bytes\n\t"
	  "maximum theoretical transfer: %d sectors\n",
	  table, reserved, table*(reserved/CD_FRAMESIZE_RAW));
  cdmessage(d,buffer);

  cur=reserved; /* only use one entry for now */

  /* so since we never go above q->max_sectors, we should never get -EIO.
   * we might still get -ENOMEM, but we back off for that later.  Monty
   * had an old comment: "not too much; new kernels have trouble with DMA
   * allocation, so be more conservative: 32kB max until I test more 
   * thoroughly".  We're not currently honoring that, because we should
   * always get -ENOMEM.
   *
   * Updated: but we don't always get -ENOMEM.  Sometimes USB drives 
   * still fail the wrong way.  This needs some kernel-land investigation.
   */
  /* Bumping to 64kB  transfer max --Monty */

  if (!getenv("CDDA_IGNORE_BUFSIZE_LIMIT")) {
    cur=(cur>1024*64?1024*64:cur);
  }else{
    cdmessage(d,"\tEnvironment variable CDDA_IGNORE_BUFSIZE_LIMIT set,\n"
	      "\t\tforcing maximum possible sector size.  This can break\n"
	      "\t\tspectacularly; use with caution!\n");
  }
  d->nsectors=cur/CD_FRAMESIZE_RAW;
  d->bigbuff=cur;

  sprintf(buffer,"\tSetting default read size to %d sectors (%d bytes).\n\n",
	  d->nsectors,d->nsectors*CD_FRAMESIZE_RAW);

  if(cur==0) exit(1);

  cdmessage(d,buffer);
}

static void clear_garbage(cdrom_drive *d){
  fd_set fdset;
  struct timeval tv;
  struct sg_header *sg_hd=d->impl->sg_hd;
  int flag=0;

  /* clear out any possibly preexisting garbage */
  FD_ZERO(&fdset);
  FD_SET(d->cdda_fd,&fdset);
  tv.tv_sec=0;
  tv.tv_usec=0;

  /* I like select */
  while(select(d->cdda_fd+1,&fdset,NULL,NULL,&tv)==1){
    
    sg_hd->twelve_byte = 0;
    sg_hd->result = 0;
    sg_hd->reply_len = SG_OFF;
    read(d->cdda_fd, sg_hd, 1);

    /* reset for select */
    FD_ZERO(&fdset);
    FD_SET(d->cdda_fd,&fdset);
    tv.tv_sec=0;
    tv.tv_usec=0;
    if(!flag && d->report_all)
      cdmessage(d,"Clearing previously returned data from SCSI buffer\n");
    flag=1;
  }
}

static int check_sbp_error(const unsigned char status,
			   const unsigned char *sbp) {
  char key = sbp[2] & 0xf;
  char ASC = sbp[12];
  char ASCQ = sbp[13];
  
  if(status==0)return 0;
  if(status==8)return TR_BUSY;

  if (sbp[0]) {  
    switch (key){
    case 0:
      if (errno==0)
	errno = EIO;
      return(TR_UNKNOWN);
    case 1:
      break;
    case 2:
      errno = ENOMEDIUM;
      return(TR_NOTREADY);
    case 3: 
      if ((ASC==0x0C) & (ASCQ==0x09)) {
	/* loss of streaming */
	if (errno==0)
	  errno = EIO;
	return(TR_STREAMING);
      } else {
	if (errno==0)
	  errno = EIO;
	return(TR_MEDIUM);
      }
    case 4:
      if (errno==0)
	errno = EIO;
      return(TR_FAULT);
    case 5:
      if (errno==0)
	errno = EINVAL;
      return(TR_ILLEGAL);
    default:
      if (errno==0)
	errno = EIO;
      return(TR_UNKNOWN);
    }
  }
  return 0;
}

/* process a complete scsi command. */
static int sg2_handle_scsi_cmd(cdrom_drive *d,
			       unsigned char *cmd,
			       unsigned int cmd_len, 
			       unsigned int in_size, 
			       unsigned int out_size,       
			       unsigned char bytefill,
			       int bytecheck,
			       unsigned char *sense_buffer){
  struct timespec tv1;
  struct timespec tv2;
  int tret1,tret2;
  int status = 0;
  struct sg_header *sg_hd=d->impl->sg_hd;
  long writebytes=SG_OFF+cmd_len+in_size;

  /* generic scsi device services */

  /* clear out any possibly preexisting garbage */
  clear_garbage(d);

  memset(sg_hd,0,sizeof(sg_hd)); 
  memset(sense_buffer,0,SG_MAX_SENSE); 
  memcpy(d->impl->sg_buffer,cmd,cmd_len+in_size);
  sg_hd->twelve_byte = cmd_len == 12;
  sg_hd->result = 0;
  sg_hd->reply_len = SG_OFF + out_size;

  /* The following is one of the scariest hacks I've ever had to use.
     The idea is this: We want to know if a command fails.  The
     generic scsi driver (as of now) won't tell us; it hands back the
     uninitialized contents of the preallocated kernel buffer.  We
     force this buffer to a known value via another bug (nonzero data
     length for a command that doesn't take data) such that we can
     tell if the command failed.  Scared yet? */

  if(bytecheck && out_size>in_size){
    memset(d->impl->sg_buffer+cmd_len+in_size,bytefill,out_size-in_size); 
    /* the size does not remove cmd_len due to the way the kernel
       driver copies buffers */
    writebytes+=(out_size-in_size);
  }

  {
    /* Select on write with a 5 second timeout.  This is a hack until
       a better error reporting layer is in place; right now, always
       print a message. */

    fd_set fdset;
    struct timeval tv;

    FD_ZERO(&fdset);
    FD_SET(d->cdda_fd,&fdset);
    tv.tv_sec=60; /* Increased to 1m for plextor, as the drive will
                     try to get through rough spots on its own and
                     this can take time 19991129 */
    tv.tv_usec=0;

    while(1){
      int ret=select(d->cdda_fd+1,NULL,&fdset,NULL,&tv);
      if(ret>0)break;
      if(ret<0 && errno!=EINTR)break;
      if(ret==0){
	fprintf(stderr,"\nSCSI transport error: timeout waiting to write"
		" packet\n\n");
	return(TR_EWRITE);
      }
    }
  }

  sigprocmask (SIG_BLOCK, &(d->sigset), NULL );
  tret1=clock_gettime(d->impl->clock,&tv1);  
  errno=0;
  status = write(d->cdda_fd, sg_hd, writebytes );

  if (status<0 || status != writebytes ) {
    sigprocmask ( SIG_UNBLOCK, &(d->sigset), NULL );
    if(errno==0)errno=EIO;
    return(TR_EWRITE);
  }

  
  {
    /* Select on read (and write; this signals an error) with a 5
       second timeout.  This is a hack until a better error reporting
       layer is in place; right now, always print a
       message. */

    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(d->cdda_fd,&rset);
    tv.tv_sec=60; /* Increased to 1m for plextor, as the drive will
                     try to get through rough spots on its own and
                     this can take time 19991129 */
    tv.tv_usec=0;

    while(1){
      int ret=select(d->cdda_fd+1,&rset,NULL,NULL,&tv);
      if(ret<0 && errno!=EINTR)break;
      if(ret==0){
	sigprocmask ( SIG_UNBLOCK, &(d->sigset), NULL );
	fprintf(stderr,"\nSCSI transport error: timeout waiting to read"
		" packet\n\n");
	return(TR_EREAD);
      }
      if(ret>0){
	/* is it readable or something else? */
	if(FD_ISSET(d->cdda_fd,&rset))break;
	sigprocmask ( SIG_UNBLOCK, &(d->sigset), NULL );
	fprintf(stderr,"\nSCSI transport: error reading packet\n\n");
	return(TR_EREAD);
      }
    }
  }

  tret2=clock_gettime(d->impl->clock,&tv2);  
  errno=0;
  status = read(d->cdda_fd, sg_hd, SG_OFF + out_size);
  sigprocmask ( SIG_UNBLOCK, &(d->sigset), NULL );
  memcpy(sense_buffer,sg_hd->sense_buffer,SG_MAX_SENSE);

  if (status<0)return status;

  if(status != SG_OFF + out_size || sg_hd->result){
    if(errno==0)errno=EIO;
    return(TR_EREAD);
  }

  status = check_sbp_error(sg_hd->target_status, sense_buffer);
  if(status)return status;

  /* Failed/Partial DMA transfers occasionally get through.  Why?  No clue,
     but it's been demonstrated in practice that we occasionally get
     fewer bytes than we request with no indication anything went
     wrong. */
  
  if(bytecheck && in_size+cmd_len<out_size){
    long i,flag=0;
    for(i=in_size;i<out_size;i++)
      if(d->impl->sg_buffer[i]!=bytefill){
	flag=1;
	break;
      }
    
    if(!flag){
      errno=EINVAL;
      return(TR_ILLEGAL);
    }
  }

  errno=0;
  if(tret1<0 || tret2<0){
    d->impl->last_milliseconds=-1;
  }else{
    d->impl->last_milliseconds = (tv2.tv_sec-tv1.tv_sec)*1000 + (tv2.tv_nsec-tv1.tv_nsec)/1000000;
  }
  return(0);
}

static int sgio_handle_scsi_cmd(cdrom_drive *d,
				unsigned char *cmd,
				unsigned int cmd_len, 
				unsigned int in_size, 
				unsigned int out_size,       
				unsigned char bytefill,
				int bytecheck,
				unsigned char *sense){

  int status = 0;
  struct sg_io_hdr hdr;

  memset(&hdr,0,sizeof(hdr));
  memset(sense,0,sizeof(sense));
  memcpy(d->impl->sg_buffer,cmd+cmd_len,in_size);

  hdr.cmdp = cmd;
  hdr.cmd_len = cmd_len;
  hdr.sbp = sense;
  hdr.mx_sb_len = SG_MAX_SENSE;
  hdr.timeout = 50000;
  hdr.interface_id = 'S';
  hdr.dxferp =  d->impl->sg_buffer;
  hdr.flags = SG_FLAG_DIRECT_IO;  /* direct IO if we can get it */

  /* scary buffer fill hack */
  if(bytecheck && out_size>in_size)
    memset(hdr.dxferp+in_size,bytefill,out_size-in_size); 

  if (in_size) {
    hdr.dxfer_len = in_size;
    hdr.dxfer_direction = SG_DXFER_TO_DEV;
    
    errno = 0;
    status = ioctl(d->ioctl_fd, SG_IO, &hdr);
    if (status >= 0 && hdr.status){
      status = check_sbp_error(hdr.status,hdr.sbp);
      if(status) return status;
    }
    if (status < 0) return TR_EWRITE;
  }

  if (!in_size | out_size) {
    hdr.dxfer_len = out_size;

    if(bytecheck && d->interface != SGIO_SCSI_BUGGY1)
      hdr.dxfer_direction = out_size ? SG_DXFER_TO_FROM_DEV : SG_DXFER_NONE;
    else
      hdr.dxfer_direction = out_size ? SG_DXFER_FROM_DEV : SG_DXFER_NONE;

    errno = 0;
    status = timed_ioctl(d,d->ioctl_fd, SG_IO, &hdr);
    if (status >= 0 && hdr.status){
      status = check_sbp_error(hdr.status,hdr.sbp);
      if(status) return status;
    }
    if (status < 0) return status;
  }

  /* Failed/Partial DMA transfers occasionally get through.  Why?  No clue,
     but it's been demonstrated in practice that we occasionally get
     fewer bytes than we request with no indication anything went
     wrong. */
  
  if(bytecheck && in_size<out_size){
    long i,flag=0;
    for(i=in_size;i<out_size;i++)
      if(d->impl->sg_buffer[i]!=bytefill){
	flag=1;
	break;
      }
    
    if(!flag){
      errno=EINVAL;
      return(TR_ILLEGAL);
    }
  }

  /* Can't rely on .duration because we can't be certain kernel has HZ set to something useful */
  /* d->impl->last_milliseconds = hdr.duration; */

  errno = 0;
  return 0;
}

static int handle_scsi_cmd(cdrom_drive *d,
			   unsigned char *cmd,
			   unsigned int cmd_len, 
			   unsigned int in_size, 
			   unsigned int out_size,       
			   unsigned char bytefill,
			   int bytecheck,
			   unsigned char *sense){

  if(d->interface == SGIO_SCSI || d->interface == SGIO_SCSI_BUGGY1)
    return sgio_handle_scsi_cmd(d,cmd,cmd_len,in_size,out_size,bytefill,bytecheck,sense);
  return sg2_handle_scsi_cmd(d,cmd,cmd_len,in_size,out_size,bytefill,bytecheck,sense);

}

static int test_unit_ready(cdrom_drive *d){
  unsigned char sense[SG_MAX_SENSE];
  unsigned char key, ASC, ASCQ;
  unsigned char cmd[6] =  { 0x00, /* TEST_UNIT_READY */	
			    0, /* reserved */
			    0, /* reserved */
			    0, /* reserved */
			    0, /* reserved */
			    0};/* control */		

  handle_scsi_cmd(d, cmd, 6, 0, 56, 0,0, sense);

  key = d->impl->sg_buffer[2] & 0xf;
  ASC = d->impl->sg_buffer[12];
  ASCQ = d->impl->sg_buffer[13];
  
  if(key == 2 && ASC == 4 && ASCQ == 1) return 0;
  return 1;
}

static void reset_scsi(cdrom_drive *d){
  int arg,tries=0;
  d->enable_cdda(d,0);

  cdmessage(d,"sending SG SCSI reset... ");
  if(ioctl(d->cdda_fd,SG_SCSI_RESET,&arg))
    cdmessage(d,"FAILED: EBUSY\n");
  else
    cdmessage(d,"OK\n");

  while(1) {
    if(test_unit_ready(d))break;
    tries++;
    usleep(10);
  }
  
  d->enable_cdda(d,1);
}

static int mode_sense_atapi(cdrom_drive *d,int size,int page){ 
  unsigned char sense[SG_MAX_SENSE];
  unsigned char cmd[10]= {0x5A,   /* MODE_SENSE */
			  0x00, /* reserved */
			  0x00, /* page */
			  0,    /* reserved */
			  0,    /* reserved */
			  0,    /* reserved */
			  0,    /* reserved */
			  0,    /* MSB (0) */
			  0,    /* sizeof(modesense - SG_OFF) */
			  0};   /* reserved */ 

  cmd[1]=d->lun<<5;
  cmd[2]=0x3F&page;
  cmd[8]=size+4;

  if (handle_scsi_cmd (d, cmd, 10, 0, size+4,'\377',1,sense)) return(1);

  {
    unsigned char *b=d->impl->sg_buffer;
    if(b[0])return(1); /* Handles only up to 256 bytes */
    if(b[6])return(1); /* Handles only up to 256 bytes */

    b[0]=b[1]-3;
    b[1]=b[2];
    b[2]=b[3];
    b[3]=b[7];

    memmove(b+4,b+8,size-4);
  }
  return(0);
}

/* group 0 (6b) command */

static int mode_sense_scsi(cdrom_drive *d,int size,int page){  
  unsigned char sense[SG_MAX_SENSE];
  unsigned char cmd[6]={0x1A, /* MODE_SENSE */
			0x00, /* return block descriptor/lun */
			0x00, /* page */
			0,    /* reserved */
			0,    /* sizeof(modesense - SG_OFF) */
			0};   /* control */ 
  
  cmd[1]=d->lun<<5;
  cmd[2]=(0x3F&page);
  cmd[4]=size;
  
  if (handle_scsi_cmd (d, cmd, 6, 0, size, '\377',1, sense)) return(1);

  /* dump it all... */
  

  return(0);
}

static int mode_sense(cdrom_drive *d,int size,int page){
  if(d->is_atapi)
    return(mode_sense_atapi(d,size,page));
  return(mode_sense_scsi(d,size,page));
}

/* Current SG/SGIO impleenmtations specifically disallow mode set
   unless running as root (or setuid).  One can see why (could be
   disastrous on, eg, a SCSI disk), but it curtails what we can do
   with older SCSI cdroms. */
static int mode_select(cdrom_drive *d,int density,int secsize){
  /* short circut the way Heiko does it; less flexible, but shorter */
  unsigned char sense[SG_MAX_SENSE];
  if(d->is_atapi){
    unsigned char cmd[26] = { 0x55, /* MODE_SELECT */
			      0x10, /* no save page */
			      0,    /* reserved */
			      0,    /* reserved */
			      0,    /* reserved */
			      0,    /* reserved */
			      0,    /* reserved */
			      0,    /* reserved */
			      16,   /* sizeof(mode) */
			      0,    /* reserved */
			      
			      /* mode parameter header */
			      0, 0, 0, 0,  0, 0, 0, 
			      8, /* Block Descriptor Length */
			      
			      /* descriptor block */
			      0,       /* Density Code */
			      0, 0, 0, /* # of Blocks */
			      0,       /* reserved */
			      0, 0, 0};/* Blocklen */
    unsigned char *mode = cmd + 18;
    
    /* prepare to read cds in the previous mode */
    mode [0] = density;
    mode [6] = secsize >> 8;   /* block length "msb" */
    mode [7] = secsize & 0xFF; /* block length lsb */

    /* do the scsi cmd */
    return(handle_scsi_cmd (d,cmd,10, 16, 0,0,0,sense));

  }else{
    unsigned char cmd[18] = { 0x15, /* MODE_SELECT */
			      0x10, /* no save page */
			      0, /* reserved */
			      0, /* reserved */
			      12, /* sizeof(mode) */
			      0, /* reserved */
			      /* mode section */
			      0, 
			      0, 0, 
			      8,       /* Block Descriptor Length */
			      0,       /* Density Code */
			      0, 0, 0, /* # of Blocks */
			      0,       /* reserved */
			      0, 0, 0};/* Blocklen */
    unsigned char *mode = cmd + 10;

    /* prepare to read cds in the previous mode */
    mode [0] = density;
    mode [6] =  secsize >> 8;   /* block length "msb" */
    mode [7] =  secsize & 0xFF; /* block length lsb */

    /* do the scsi cmd */
    return(handle_scsi_cmd (d,cmd,6, 12, 0,0,0,sense));
  }
}

/* get current sector size from SCSI cdrom drive */
static unsigned int get_orig_sectorsize(cdrom_drive *d){
  if(mode_sense(d,12,0x01))return(-1);

  d->orgdens = d->impl->sg_buffer[4];
  return(d->orgsize = ((int)(d->impl->sg_buffer[10])<<8)+d->impl->sg_buffer[11]);
}

/* switch CDROM scsi drives to given sector size  */
static int set_sectorsize (cdrom_drive *d,unsigned int secsize){
  return(mode_select(d,d->orgdens,secsize));
}

/* switch old Toshiba/DEC and HP drives from/to cdda density */
int scsi_enable_cdda (cdrom_drive *d, int fAudioMode){
  if (fAudioMode) {
    if(mode_select(d,d->density,CD_FRAMESIZE_RAW)){
      if(d->error_retry)
	cderror(d,"001: Unable to set CDROM to read audio mode\n");
      return(-1);
    }
  } else {
    if(mode_select(d,d->orgdens,d->orgsize)){
      if(d->error_retry)
	cderror(d,"001: Unable to set CDROM to read audio mode\n");
      return(-1);
    }
  }
  return(0);
}

typedef struct scsi_TOC {  /* structure of scsi table of contents (cdrom) */
  unsigned char reserved1;
  unsigned char bFlags;
  unsigned char bTrack;
  unsigned char reserved2;
  signed char start_MSB;
  unsigned char start_1;
  unsigned char start_2;
  unsigned char start_LSB;
} scsi_TOC;


/* read the table of contents from the cd and fill the TOC array */
/* Do it like the kernel ioctl driver; the 'all at once' approach
   fails on at least one Kodak drive. */

static int scsi_read_toc (cdrom_drive *d){
  int i,first,last;
  unsigned tracks;

  /* READTOC, MSF format flag, res, res, res, res, Start track, len msb,
     len lsb, flags */

  /* read the header first */
  unsigned char sense[SG_MAX_SENSE];
  unsigned char cmd[10] = { 0x43, 0, 0, 0, 0, 0, 1, 0, 12, 0};
  cmd[1]=d->lun<<5;

  if (handle_scsi_cmd (d,cmd,10, 0, 12,'\377',1,sense)){
    cderror(d,"004: Unable to read table of contents header\n");
    return(-4);
  }

  first=d->impl->sg_buffer[2];
  last=d->impl->sg_buffer[3];
  tracks=last-first+1;

  if (last > MAXTRK || first > MAXTRK || last<0 || first<0) {
    cderror(d,"003: CDROM reporting illegal number of tracks\n");
    return(-3);
  }

  for (i = first; i <= last; i++){
    memcpy(cmd, (char []){ 0x43, 0, 0, 0, 0, 0, 0, 0, 12, 0}, 10);
    cmd[1]=d->lun<<5;
    cmd[6]=i;
    
    if (handle_scsi_cmd (d,cmd, 10, 0, 12,'\377',1,sense)){
      cderror(d,"005: Unable to read table of contents entry\n");
      return(-5);
    }
    {
      scsi_TOC *toc=(scsi_TOC *)(d->impl->sg_buffer+4);

      d->disc_toc[i-first].bFlags=toc->bFlags;
      d->disc_toc[i-first].bTrack=i;
      d->disc_toc[i-first].dwStartSector= d->adjust_ssize * 
	(((int)(toc->start_MSB)<<24) | 
	 (toc->start_1<<16)|
	 (toc->start_2<<8)|
	 (toc->start_LSB));
    }
  }

  memcpy(cmd, (char []){ 0x43, 0, 0, 0, 0, 0, 0, 0, 12, 0}, 10);
  cmd[1]=d->lun<<5;
  cmd[6]=0xAA;
    
  if (handle_scsi_cmd (d,cmd,10, 0, 12,'\377',1,sense)){
    cderror(d,"002: Unable to read table of contents lead-out\n");
    return(-2);
  }
  {
    scsi_TOC *toc=(scsi_TOC *)(d->impl->sg_buffer+4);
    
    d->disc_toc[i-first].bFlags=toc->bFlags;
    d->disc_toc[i-first].bTrack=0xAA;
    d->disc_toc[i-first].dwStartSector= d->adjust_ssize * 
	(((int)(toc->start_MSB)<<24) | 
	 (toc->start_1<<16)|
	 (toc->start_2<<8)|
	 (toc->start_LSB));
  }

  d->cd_extra = FixupTOC(d,tracks+1); /* include lead-out */
  return(tracks);
}

/* a contribution from Boris for IMS cdd 522 */
/* check this for ACER/Creative/Foo 525,620E,622E, etc? */
static int scsi_read_toc2 (cdrom_drive *d){
  u_int32_t foo,bar;

  int i;
  unsigned tracks;

  unsigned char cmd[10] = { 0xe5, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned char sense[SG_MAX_SENSE];
  cmd[5]=1;
  cmd[8]=255;

  if (handle_scsi_cmd (d,cmd,10, 0, 256,'\377',1,sense)){
    cderror(d,"004: Unable to read table of contents header\n");
    return(-4);
  }

  /* copy to our structure and convert start sector */
  tracks = d->impl->sg_buffer[1];
  if (tracks > MAXTRK) {
    cderror(d,"003: CDROM reporting illegal number of tracks\n");
    return(-3);
  }

  for (i = 0; i < tracks; i++){
    memcpy(cmd, (char[]){ 0xe5, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 10);
    cmd[5]=i+1;
    cmd[8]=255;
    
    if (handle_scsi_cmd (d,cmd,10, 0, 256,'\377',1,sense)){
      cderror(d,"005: Unable to read table of contents entry\n");
      return(-5);
    }
    
    d->disc_toc[i].bFlags = d->impl->sg_buffer[10];
    d->disc_toc[i].bTrack = i + 1;

    d->disc_toc[i].dwStartSector= d->adjust_ssize * 
	(((signed char)(d->impl->sg_buffer[2])<<24) | 
	 (d->impl->sg_buffer[3]<<16)|
	 (d->impl->sg_buffer[4]<<8)|
	 (d->impl->sg_buffer[5]));
  }

  d->disc_toc[i].bFlags = 0;
  d->disc_toc[i].bTrack = i + 1;
  memcpy (&foo, d->impl->sg_buffer+2, 4);
  memcpy (&bar, d->impl->sg_buffer+6, 4);
  d->disc_toc[i].dwStartSector = d->adjust_ssize * (be32_to_cpu(foo) +
						    be32_to_cpu(bar));

  d->disc_toc[i].dwStartSector= d->adjust_ssize * 
    ((((signed char)(d->impl->sg_buffer[2])<<24) | 
      (d->impl->sg_buffer[3]<<16)|
      (d->impl->sg_buffer[4]<<8)|
      (d->impl->sg_buffer[5]))+
     
     ((((signed char)(d->impl->sg_buffer[6])<<24) | 
       (d->impl->sg_buffer[7]<<16)|
       (d->impl->sg_buffer[8]<<8)|
       (d->impl->sg_buffer[9]))));


  d->cd_extra = FixupTOC(d,tracks+1);
  return(tracks);
}

static int scsi_set_speed (cdrom_drive *d, int speed){
  unsigned char cmd[12]={0xBB, 0, 0, 0, 0xff, 0xff, 0, 0, 0, 0, 0, 0};
  unsigned char sense[SG_MAX_SENSE];

  if(speed>=0)
    speed=speed*44100*4/1024;
  else
    speed=-1;
  cmd[2] = (speed >> 8) & 0xFF;
  cmd[3] = (speed) & 0xFF;
  return handle_scsi_cmd(d,cmd,12,0,0,0,0,sense);
}

/* These do one 'extra' copy in the name of clean code */

static int i_read_28 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[10]={0x28, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  if(d->fua)
    cmd[1]=0x08;

  cmd[1]|=d->lun<<5;

  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,10,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_A8 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  
  if(d->fua)
    cmd[1]=0x08;
  
  cmd[1]|=d->lun<<5;

  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[9] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_D4_10 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[10]={0xd4, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  
  if(d->fua)
    cmd[1]=0x08;

  cmd[1]|=d->lun<<5;
  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,10,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_D4_12 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xd4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  if(d->fua)
    cmd[1]=0x08;

  cmd[1]|=d->lun<<5;
  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[9] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_D5 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[10]={0xd5, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  if(d->fua)
    cmd[1]=0x08;

  cmd[1]|=d->lun<<5;
  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,10,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_D8 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xd8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  if(d->fua)
    cmd[1]=0x08;

  cmd[1]|=d->lun<<5;
  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[9] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_mmc (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xbe, 0x2, 0, 0, 0, 0, 0, 0, 0, 0x10, 0, 0};

  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_mmcB (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xbe, 0x0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0, 0};

  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_mmc2 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xbe, 0x2, 0, 0, 0, 0, 0, 0, 0, 0xf8, 0, 0};

  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_mmc2B (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xbe, 0x0, 0, 0, 0, 0, 0, 0, 0, 0xf8, 0, 0};

  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_mmc3 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xbe, 0x6, 0, 0, 0, 0, 0, 0, 0, 0xf8, 0, 0};

  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_mmc3B (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xbe, 0x4, 0, 0, 0, 0, 0, 0, 0, 0xf8, 0, 0};

  cmd[3] = (begin >> 16) & 0xFF;
  cmd[4] = (begin >> 8) & 0xFF;
  cmd[5] = begin & 0xFF;
  cmd[8] = sectors;
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

/* straight from the MMC3 spec */
static inline void LBA_to_MSF(long lba,
			      unsigned char *M, 
			      unsigned char *S, 
			      unsigned char *F){
  if(lba>=-150){
    *M=(lba+150)/(60*75);
    lba-=(*M)*60*75;
    *S=(lba+150)/75;
    lba-=(*S)*75;
    *F=(lba+150);
  }else{
    *M=(lba+450150)/(60*75);
    lba-=(*M)*60*75;
    *S=(lba+450150)/75;
    lba-=(*S)*75;
    *F=(lba+450150);
  }
}


static int i_read_msf (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xb9, 0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0, 0};

  LBA_to_MSF(begin,cmd+3,cmd+4,cmd+5);
  LBA_to_MSF(begin+sectors,cmd+6,cmd+7,cmd+8);

  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_msf2 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xb9, 0, 0, 0, 0, 0, 0, 0, 0, 0xf8, 0, 0};

  LBA_to_MSF(begin,cmd+3,cmd+4,cmd+5);
  LBA_to_MSF(begin+sectors,cmd+6,cmd+7,cmd+8);

  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}

static int i_read_msf3 (cdrom_drive *d, void *p, long begin, long sectors, unsigned char *sense){
  int ret;
  unsigned char cmd[12]={0xb9, 4, 0, 0, 0, 0, 0, 0, 0, 0xf8, 0, 0};
  
  LBA_to_MSF(begin,cmd+3,cmd+4,cmd+5);
  LBA_to_MSF(begin+sectors,cmd+6,cmd+7,cmd+8);
  
  if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
    return(ret);
  if(p)memcpy(p,d->impl->sg_buffer,sectors*CD_FRAMESIZE_RAW);
  return(0);
}


static long scsi_read_map (cdrom_drive *d, void *p, long begin, long sectors,
			   int (*map)(cdrom_drive *, void *, long, long, 
				      unsigned char *)){
  unsigned char sense[SG_MAX_SENSE];
  int retry_count,err;
  char *buffer=(char *)p;

  /* read d->nsectors at a time, max. */
  sectors=(sectors>d->nsectors?d->nsectors:sectors);
  sectors=(sectors<1?1:sectors);

  retry_count=0;
  
  while(1) {

    if((err=map(d,(p?buffer:NULL),begin,sectors,sense))){
      if(d->report_all){
	char b[256];

	sprintf(b,"scsi_read error: sector=%ld length=%ld retry=%d\n",
		begin,sectors,retry_count);
	cdmessage(d,b);
	sprintf(b,"                 Sense key: %x ASC: %x ASCQ: %x\n",
		(int)(sense[2]&0xf),
		(int)(sense[12]),
		(int)(sense[13]));
	cdmessage(d,b);
	sprintf(b,"                 Transport error: %s\n",strerror_tr[err]);
	cdmessage(d,b);
	sprintf(b,"                 System error: %s\n",strerror(errno));
	cdmessage(d,b);

	fprintf(stderr,"scsi_read error: sector=%ld length=%ld retry=%d\n",
		begin,sectors,retry_count);
	fprintf(stderr,"                 Sense key: %x ASC: %x ASCQ: %x\n",
		(int)(sense[2]&0xf),
		(int)(sense[12]),
		(int)(sense[13]));
	fprintf(stderr,"                 Transport error: %s\n",strerror_tr[err]);
	fprintf(stderr,"                 System error: %s\n",strerror(errno));
      }
      
      switch(errno){
      case EINTR:
	usleep(100);
	continue;
      case ENOMEM:
	/* D'oh.  Possible kernel error. Keep limping */
	usleep(100);
	if(sectors==1){
	  /* Nope, can't continue */
	  cderror(d,"300: Kernel memory error\n");
	  return(-300);  
	}
	if(d->report_all){
	  char b[256];
	  sprintf(b,"scsi_read: kernel couldn't alloc %ld bytes.  "
		  "backing off...\n",sectors*CD_FRAMESIZE_RAW);
	    
	  cdmessage(d,b);
	}
	sectors--;
	continue;
      case ENOMEDIUM:
	cderror(d,"404: No medium present\n");
	return(-404);

      default:
	if(sectors==1){
	  if(errno==EIO)
	    if(d->fua==-1) /* testing for FUA support */
	      return(-7);
	  
	  /* *Could* be I/O or media error.  I think.  If we're at
	     30 retries, we better skip this unhappy little
	     sector. */
	  if(retry_count>MAX_RETRIES-1){
	    char b[256];
	    sprintf(b,"010: Unable to access sector %ld\n",
		    begin);
	    cderror(d,b);
	    return(-10);
	    
	  }
	  break;
	}

	/* Hmm.  OK, this is just a tad silly.  just in case this was
           a timeout and a reset happened, we need to set the drive
           back to cdda */
	reset_scsi(d);
      }
      if(!d->error_retry)return(-7);

    }else{

      /* Did we get all the bytes we think we did, or did the kernel
         suck? */
      if(buffer){
	long i;
	for(i=sectors*CD_FRAMESIZE_RAW;i>1;i-=2)
	  if(buffer[i-1]!='\177' || buffer[i-2]!='\177')
	    break;

	i/=CD_FRAMESIZE_RAW;
	if(i!=sectors){
	  if(d->report_all){
	    char b[256];
	    sprintf(b,"scsi_read underrun: pos=%ld len=%ld read=%ld retry=%d\n",
		    begin,sectors,i,retry_count);
	    
	    cdmessage(d,b);
	  }
	  reset_scsi(d);
	}
	
	if(i>0)return(i);
       
      }else
	break;
    }
    
    retry_count++;
    if(sectors==1 && retry_count>MAX_RETRIES){
      cderror(d,"007: Unknown, unrecoverable error reading data\n");
      return(-7);
    }
    if(sectors>1)sectors=sectors/2;
    d->enable_cdda(d,0);
    d->enable_cdda(d,1);

  }
  return(sectors);
}

long scsi_read_28 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_28));
}

long scsi_read_A8 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_A8));
}

long scsi_read_D4_10 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_D4_10));
}

long scsi_read_D4_12 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_D4_12));
}

long scsi_read_D5 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_D5));
}

long scsi_read_D8 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_D8));
}

long scsi_read_mmc (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_mmc));
}

long scsi_read_mmc2 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_mmc2));
}

long scsi_read_mmc3 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_mmc3));
}

long scsi_read_mmcB (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_mmcB));
}

long scsi_read_mmc2B (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_mmc2B));
}

long scsi_read_mmc3B (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_mmc3B));
}

long scsi_read_msf (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_msf));
}

long scsi_read_msf2 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_msf2));
}

long scsi_read_msf3 (cdrom_drive *d, void *p, long begin, 
			       long sectors){
  return(scsi_read_map(d,p,begin,sectors,i_read_msf3));
}


/* Some drives, given an audio read command, return only 2048 bytes
   of data as opposed to 2352 bytes.  Look for bytess at the end of the
   single sector verification read */

static int count_2352_bytes(cdrom_drive *d){
  long i;
  for(i=2351;i>=0;i--)
    if(d->impl->sg_buffer[i]!=(unsigned char)'\177')
      return(((i+3)>>2)<<2);

  return(0);
}

static int verify_nonzero(cdrom_drive *d){
  long i,flag=0;
  for(i=0;i<2352;i++)
    if(d->impl->sg_buffer[i]!=0){
      flag=1;
      break;
    }
  
  return(flag);
}

/* So many different read commands, densities, features...
   Verify that our selected 'read' command actually reads 
   nonzero data, else search through other possibilities */

static int verify_read_command(cdrom_drive *d){
  int i,j,k;
  int audioflag=0;

  int  (*enablecommand)  (struct cdrom_drive *d, int speed);
  long (*readcommand)   (struct cdrom_drive *d, void *p, long begin, 
		       long sectors);
  unsigned char density;
  
  int16_t *buff=malloc(CD_FRAMESIZE_RAW);

  cdmessage(d,"Verifying CDDA command set...\n");

  /* try the expected command set; grab the center of each track, look
     for data */

  if(d->enable_cdda(d,1)==0){
    
    for(i=1;i<=d->tracks;i++){
      if(cdda_track_audiop(d,i)==1){
	long firstsector=cdda_track_firstsector(d,i);
	long lastsector=cdda_track_lastsector(d,i);
	long sector=(firstsector+lastsector)>>1;
	audioflag=1;

	if(d->read_audio(d,buff,sector,1)>0){
	  if(count_2352_bytes(d)==2352){
	    cdmessage(d,"\tExpected command set reads OK.\n");
	    d->enable_cdda(d,0);
	    free(buff);
	    return(0);
	  }
	}
      }
    }
    
    d->enable_cdda(d,0);
  }
  if(!audioflag){
    cdmessage(d,"\tCould not find any audio tracks on this disk.\n");
    return(-403);
  }

  {
    char *es="",*rs="";
    d->bigendianp=-1;
    density=d->density;
    readcommand=d->read_audio;
    enablecommand=d->enable_cdda;


    /* No nonzeroes?  D'oh.  Exhaustive search */
    cdmessage(d,"\tExpected command set FAILED!\n"
	      "\tPerforming full probe for CDDA command set...\n");
    
    /* loops:  
       density/enable no,  0x0/org,  0x04/org, 0x82/org
       read command read_10 read_12 read_nec read_sony read_mmc read_mmc2 */

    /* NEC test must come before sony; the nec drive expects d8 to be
       10 bytes, and a 12 byte verson (Sony) crashes the drive */

    for(j=0;j<15;j++){
      int densitypossible=1;

      switch(j){
      case 0:
	d->read_audio=scsi_read_28;
	rs="28 0x,00";
	break;
      case 1:
	d->read_audio=scsi_read_A8;
	rs="a8 0x,00";
	break;

      case 2:
	d->read_audio=scsi_read_mmcB;
	rs="be 02,10";
	densitypossible=0;
	break;
      case 3:
	d->read_audio=scsi_read_mmc2B;
	rs="be 02,f8";
	densitypossible=0;
	break;
      case 4:
	d->read_audio=scsi_read_mmc3B;
	rs="be 06,f8";
	densitypossible=0;
	break;

      case 5:
	d->read_audio=scsi_read_mmc;
	rs="be 00,10";
	densitypossible=0;
	break;
      case 6:
	d->read_audio=scsi_read_mmc2;
	rs="be 00,f8";
	densitypossible=0;
	break;
      case 7:
	d->read_audio=scsi_read_mmc3;
	rs="be 04,f8";
	densitypossible=0;
	break;

      case 8:
	d->read_audio=scsi_read_msf;
	rs="b9 00,10";
	densitypossible=0;
	break;
      case 9:
	d->read_audio=scsi_read_msf2;
	rs="b9 00,f8";
	densitypossible=0;
	break;
      case 10:
	d->read_audio=scsi_read_msf3;
	rs="b9 04,f8";
	densitypossible=0;
	break;

      case 11:
	d->read_audio=scsi_read_D4_10;
	rs="d4(10)0x";
	break;
      case 12:
	d->read_audio=scsi_read_D4_12;
	rs="d4(12)0x";
	break;
      case 13:
	d->read_audio=scsi_read_D5;
	rs="d5 0x,00";
	break;
      case 14:
	d->read_audio=scsi_read_D8;
	rs="d8 0x,00";
	break;
      }
      
      for(i=0;i<5;i++){
	switch(i){
	case 0:
	  d->density=0;
	  d->enable_cdda=Dummy;
	  es="none    ";
	  if(!densitypossible)i=5; /* short circuit MMC style commands */
	  break;
	case 1:
	  d->density=0;
	  d->enable_cdda=scsi_enable_cdda;
	  es="yes/0x00";
	  break;
	case 2:
	  d->density=0x04;
	  d->enable_cdda=scsi_enable_cdda;
	  es="yes/0x04";
	  break;
	case 3:
	  d->density=0x82;
	  d->enable_cdda=scsi_enable_cdda;
	  es="yes/0x82";
	  break;
	case 4:
	  d->density=0x81;
	  d->enable_cdda=scsi_enable_cdda;
	  es="yes/0x81";
	  break;
	}

	cdmessage(d,"\ttest -> density: [");
	cdmessage(d,es);
	cdmessage(d,"]  command: [");
	cdmessage(d,rs);
	cdmessage(d,"]\n");

	{
	  int densityflag=0;
	  int rejectflag=0;
	  int zeroflag=0;
	  int lengthflag=0;

	  if(d->enable_cdda(d,1)==0){
	    for(k=1;k<=d->tracks;k++){
	      if(cdda_track_audiop(d,k)==1){
		long firstsector=cdda_track_firstsector(d,k);
		long lastsector=cdda_track_lastsector(d,k);
		long sector=(firstsector+lastsector)>>1;
		
		if(d->read_audio(d,buff,sector,1)>0){
		  if((lengthflag=count_2352_bytes(d))==2352){
		    if(verify_nonzero(d)){
		      cdmessage(d,"\t\tCommand set FOUND!\n");
		      
		      free(buff);
		      d->enable_cdda(d,0);
		      return(0);
		    }else{
		      zeroflag++;
		    }
		  }
		}else{
		  rejectflag++;
		  break;
		}
	      }
	    }
	    d->enable_cdda(d,0);
	  }else{
	    densityflag++;
	  }
	  
	  if(densityflag)
	    cdmessage(d,"\t\tDrive rejected density set\n");
	  if(rejectflag){
	    char buffer[256];
	    sprintf(buffer,"\t\tDrive rejected read command packet(s)\n");
	    cdmessage(d,buffer);
	  }
	  if(lengthflag>0 && lengthflag<2352){
	    char buffer[256];
	    sprintf(buffer,"\t\tDrive returned at least one packet, but with\n"
		        "\t\tincorrect size (%d)\n",lengthflag);
	    cdmessage(d,buffer);
	  }
	  if(zeroflag){
	    char buffer[256];
	    sprintf(buffer,"\t\tDrive returned %d packet(s), but contents\n"
		        "\t\twere entirely zero\n",zeroflag);
	    cdmessage(d,buffer);
	  }
	}
      }
    }

    /* D'oh. */
    d->density=density;
    d->read_audio=readcommand;
    d->enable_cdda=enablecommand;

    cdmessage(d,"\tUnable to find any suitable command set from probe;\n"
	      "\tdrive probably not CDDA capable.\n");

    cderror(d,"006: Could not read any data from drive\n");

  }
  free(buff);
  return(-6);
}

static void check_cache(cdrom_drive *d){
  long i;

  if(!(d->read_audio==scsi_read_mmc ||
     d->read_audio==scsi_read_mmc2 ||
     d->read_audio==scsi_read_mmc3 ||
     d->read_audio==scsi_read_mmcB ||
     d->read_audio==scsi_read_mmc2B ||
       d->read_audio==scsi_read_mmc3B)){

    cdmessage(d,"This command set may use a Force Unit Access bit.");
    cdmessage(d,"\nChecking drive for FUA bit support...\n");
    
    d->enable_cdda(d,1);
    d->fua=1;
    
    for(i=1;i<=d->tracks;i++){
      if(cdda_track_audiop(d,i)==1){
	long firstsector=cdda_track_firstsector(d,i);
	long lastsector=cdda_track_lastsector(d,i);
	long sector=(firstsector+lastsector)>>1;
	
	if(d->read_audio(d,NULL,sector,1)>0){
	  cdmessage(d,"\tDrive accepted FUA bit.\n");
	  d->enable_cdda(d,0);
	  return;
	}
      }
    }
    
    d->fua=0;
    cdmessage(d,"\tDrive rejected FUA bit.\n");

    /* we only use the FUA bit as a possible extra layer of
       redundancy; too many drives accept it, but still don't force
       unit access. Still use the old cachebusting algo. */
    return;
  }
}

static int check_atapi(cdrom_drive *d){
  int atapiret=-1;
  int fd = d->cdda_fd; /* check the device we'll actually be using to read */
			  
  cdmessage(d,"\nChecking for SCSI emulation...\n");

  if (ioctl(fd,SG_EMULATED_HOST,&atapiret)){
    cderror(d,"\tSG_EMULATED_HOST ioctl() failed!\n");
    return(-1);
  } else {
    if(atapiret==1){
      if(d->interface == SGIO_SCSI){
	cdmessage(d,"\tDrive is ATAPI (using SG_IO host adaptor emulation)\n");
      }else if(d->interface == SGIO_SCSI_BUGGY1){
	cdmessage(d,"\tDrive is ATAPI (using SG_IO host adaptor emulation with workarounds)\n");
      }else{
	cdmessage(d,"\tDrive is ATAPI (using SCSI host adaptor emulation)\n");
	/* Disable kernel SCSI command translation layer for access through sg */
	if (ioctl(fd,SG_SET_TRANSFORM,0))
	  cderror(d,"\tCouldn't disable kernel command translation layer\n");
      }
      d->is_atapi=1;
    }else{
      cdmessage(d,"\tDrive is SCSI\n");
      d->is_atapi=0;
    }

    return(d->is_atapi);
  }
}  

static int check_mmc(cdrom_drive *d){
  unsigned char *b;
  cdmessage(d,"\nChecking for MMC style command set...\n");

  d->is_mmc=0;
  if(mode_sense(d,22,0x2A)==0){
  
    b=d->impl->sg_buffer;
    b+=b[3]+4;
    
    if((b[0]&0x3F)==0x2A){
      /* MMC style drive! */
      d->is_mmc=1;
      
      if(b[1]>=4){
	if(b[5]&0x1){
	  cdmessage(d,"\tDrive is MMC style\n");
	  return(1);
	}else{
	  cdmessage(d,"\tDrive is MMC, but reports CDDA incapable.\n");
	  cdmessage(d,"\tIt will likely not be able to read audio data.\n");
	  return(1);
	}
      }
    }
  }
  
  cdmessage(d,"\tDrive does not have MMC CDDA support\n");
  return(0);
}

static void check_exceptions(cdrom_drive *d,exception *list){

  int i=0;
  while(list[i].model){
    if(!strncmp(list[i].model,d->drive_model,strlen(list[i].model))){
      if(list[i].density)d->density=list[i].density;
      if(list[i].enable)d->enable_cdda=list[i].enable;
      if(list[i].read)d->read_audio=list[i].read;
      if(list[i].bigendianp!=-1)d->bigendianp=list[i].bigendianp;
      return;
    }
    i++;
  }
}

/* request vendor brand and model */
unsigned char *scsi_inquiry(cdrom_drive *d){
  unsigned char sense[SG_MAX_SENSE];
  unsigned char cmd[6]={ 0x12,0,0,0,56,0 };
  
  if(handle_scsi_cmd(d,cmd,6, 0, 56,'\377',1,sense)) {
    cderror(d,"008: Unable to identify CDROM model\n");
    return(NULL);
  }
  return (d->impl->sg_buffer);
}

int scsi_init_drive(cdrom_drive *d){
  int ret;

  check_atapi(d);
  check_mmc(d);

  /* generic Sony type defaults; specialize from here */
  d->density = 0x0;
  d->enable_cdda = Dummy;
  d->read_audio = scsi_read_D8;
  d->fua=0x0;
  if(d->is_atapi)d->lun=0; /* it should already be; just to make sure */
      
  if(d->is_mmc){

    d->read_audio = scsi_read_mmc2B;
    d->bigendianp=0;

    check_exceptions(d,mmc_list);

  }else{
    
    if(d->is_atapi){
      /* Not MMC maybe still uses 0xbe */

      d->read_audio = scsi_read_mmc2B;
      d->bigendianp=0;

      check_exceptions(d,atapi_list);

    }else{

      check_exceptions(d,scsi_list);

    }
  }

  if(!d->is_atapi)set_sectorsize(d,2048); /* we really do want the
					     sector size at 2048 to begin.*/
  d->enable_cdda(d,0);

  d->read_toc = (!memcmp(d->drive_model, "IMS", 3) && !d->is_atapi) ? scsi_read_toc2 : 
    scsi_read_toc;
  d->set_speed = scsi_set_speed;

  if(!d->is_atapi){
    unsigned sector_size= get_orig_sectorsize(d);
    
    if(sector_size<2048 && set_sectorsize(d,2048))
      d->adjust_ssize = 2048 / sector_size;
    else
      d->adjust_ssize = 1;
  }else
    d->adjust_ssize = 1;
  
  d->tracks=d->read_toc(d);
  if(d->tracks<1)
    return(d->tracks);

  tweak_SG_buffer(d);
  d->opened=1;

  if((ret=verify_read_command(d)))return(ret);
  check_cache(d);

  d->error_retry=1;
  d->impl->sg_hd=realloc(d->impl->sg_hd,d->nsectors*CD_FRAMESIZE_RAW + SG_OFF + 128);
  d->impl->sg_buffer=((unsigned char *)d->impl->sg_hd)+SG_OFF;
  d->report_all=1;
  return(0);
}

