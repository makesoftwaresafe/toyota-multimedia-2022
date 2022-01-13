/******************************************************************
 * CopyPolicy: GNU Lesser General Public License 2.1 applies
 * Copyright (C) Monty xiphmont@mit.edu
 *
 * CDROM code specific to the cooked ioctl interface
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

static int cooked_readtoc (cdrom_drive *d){
  int i;
  int tracks;
  struct cdrom_tochdr hdr;
  struct cdrom_tocentry entry;

  /* get TocHeader to find out how many entries there are */
  if(ioctl(d->ioctl_fd, CDROMREADTOCHDR, &hdr ))
    switch(errno){
    case EPERM:
      cderror(d,"102: Permision denied on cdrom (ioctl) device\n");
      return(-102);
    default:
      cderror(d,"004: Unable to read table of contents header\n");
      return(-4);
    }

  /* get all TocEntries */
  for(i=0;i<hdr.cdth_trk1;i++){
    entry.cdte_track= i+1;
    entry.cdte_format = CDROM_LBA;
    if(ioctl(d->ioctl_fd,CDROMREADTOCENTRY,&entry)){
      cderror(d,"005: Unable to read table of contents entry\n");
      return(-5);
    }
      
    d->disc_toc[i].bFlags = (entry.cdte_adr << 4) | (entry.cdte_ctrl & 0x0f);
    d->disc_toc[i].bTrack = i+1;
    d->disc_toc[i].dwStartSector = entry.cdte_addr.lba;
  }

  entry.cdte_track = CDROM_LEADOUT;
  entry.cdte_format = CDROM_LBA;
  if(ioctl(d->ioctl_fd, CDROMREADTOCENTRY, &entry)){
    cderror(d,"005: Unable to read table of contents entry\n");
    return(-5);
  }
  d->disc_toc[i].bFlags = (entry.cdte_adr << 4) | (entry.cdte_ctrl & 0x0f);
  d->disc_toc[i].bTrack = entry.cdte_track;
  d->disc_toc[i].dwStartSector = entry.cdte_addr.lba;

  tracks=hdr.cdth_trk1+1;
  d->cd_extra=FixupTOC(d,tracks);
  return(--tracks);  /* without lead-out */
}

/* Set operating speed */
static int cooked_setspeed(cdrom_drive *d, int speed)
{
  if(d->ioctl_fd!=-1)
    return ioctl(d->ioctl_fd, CDROM_SELECT_SPEED, speed);
  else
    return 0;
}

/* read 'SectorBurst' adjacent sectors of audio sectors 
 * to Buffer '*p' beginning at sector 'lSector'
 */

static long cooked_read (cdrom_drive *d, void *p, long begin, long sectors){
  int retry_count,err,ret=0;
  struct cdrom_read_audio arg;
  char *buffer=(char *)p;

  /* read d->nsectors at a time, max. */
  sectors=(sectors>d->nsectors?d->nsectors:sectors);
  if(p==NULL)buffer = malloc(sectors*CD_FRAMESIZE_RAW);

  arg.addr.lba = begin;
  arg.addr_format = CDROM_LBA;
  arg.nframes = sectors;
  arg.buf=buffer;
  retry_count=0;

  do {
    if((err=timed_ioctl(d,d->ioctl_fd, CDROMREADAUDIO, &arg))){
      if(!d->error_retry){
	ret=-7;
	goto done;
      }
      switch(errno){
      case ENOMEM:
	/* D'oh.  Possible kernel error. Keep limping */
	if(sectors==1){
	  /* Nope, can't continue */
	  cderror(d,"300: Kernel memory error\n");
	  ret=-300;
	  goto done;
	}
      case ENXIO:
      case EBADF:
      case ENOMEDIUM:
	errno=ENOMEDIUM;
	ret=0;
	goto done;
      default:
	if(sectors==1){
	    

	  /* *Could* be I/O or media error.  I think.  If we're at
	     30 retries, we better skip this unhappy little
	     sector. */
	  if(retry_count>MAX_RETRIES-1){
	    char b[256];
	    sprintf(b,"010: Unable to access sector %ld: skipping...\n",
		    begin);
	    cderror(d,b);
	    ret=-10;
	    goto done;
	  }
	  break;
	}
      }
      if(retry_count>4)
	if(sectors>1)
	  sectors=sectors*3/4;
      retry_count++;
      if(retry_count>MAX_RETRIES){
	cderror(d,"007: Unknown, unrecoverable error reading data\n");
	ret=-7;
	goto done;
      }
    }else
      break;
  } while (err);
  
  ret=sectors;

 done:
  if(p==NULL && buffer)free(buffer);
  return ret;
}

/* hook */
static int Dummy (cdrom_drive *d,int Switch){
  return(0);
}

static int verify_read_command(cdrom_drive *d){
  int i;
  int16_t *buff=malloc(CD_FRAMESIZE_RAW);
  int audioflag=0;

  cdmessage(d,"Verifying drive can read CDDA...\n");

  d->enable_cdda(d,1);

  for(i=1;i<=d->tracks;i++){
    if(cdda_track_audiop(d,i)==1){
      long firstsector=cdda_track_firstsector(d,i);
      long lastsector=cdda_track_lastsector(d,i);
      long sector=(firstsector+lastsector)>>1;
      audioflag=1;

      if(d->read_audio(d,buff,sector,1)>0){
	cdmessage(d,"\tExpected command set reads OK.\n");
	d->enable_cdda(d,0);
	free(buff);
	return(0);
      }
    }
  }
 
  d->enable_cdda(d,0);

  if(!audioflag){
    cdmessage(d,"\tCould not find any audio tracks on this disk.\n");
    return(-403);
  }

  cdmessage(d,"\n\tUnable to read any data; "
	    "drive probably not CDDA capable.\n");
  
  cderror(d,"006: Could not read any data from drive\n");

  free(buff);
  return(-6);
}

#include "drive_exceptions.h"

static void check_exceptions(cdrom_drive *d,exception *list){

  int i=0;
  while(list[i].model){
    if(!strncmp(list[i].model,d->drive_model,strlen(list[i].model))){
      if(list[i].bigendianp!=-1)d->bigendianp=list[i].bigendianp;
      return;
    }
    i++;
  }
}

/* set function pointers to use the ioctl routines */
int cooked_init_drive (cdrom_drive *d){
  int ret;

  switch(d->drive_type){
  case MATSUSHITA_CDROM_MAJOR:	/* sbpcd 1 */
  case MATSUSHITA_CDROM2_MAJOR:	/* sbpcd 2 */
  case MATSUSHITA_CDROM3_MAJOR:	/* sbpcd 3 */
  case MATSUSHITA_CDROM4_MAJOR:	/* sbpcd 4 */
    /* don't make the buffer too big; this sucker don't preempt */

    cdmessage(d,"Attempting to set sbpcd buffer size...\n");

    d->nsectors=8;
    while(1){

      /* this ioctl returns zero on error; exactly wrong, but that's
         what it does. */

      if(ioctl(d->ioctl_fd, CDROMAUDIOBUFSIZ, d->nsectors)==0){
	d->nsectors>>=1;
	if(d->nsectors==0){
	  char buffer[256];
	  d->nsectors=8;
	  sprintf(buffer,"\tTrouble setting buffer size.  Defaulting to %d sectors.\n",
		  d->nsectors);
	  cdmessage(d,buffer);
	  break; /* Oh, well.  Try to read anyway.*/
	}
      }else{
	char buffer[256];
	sprintf(buffer,"\tSetting read block size at %d sectors (%ld bytes).\n",
		d->nsectors,(long)d->nsectors*CD_FRAMESIZE_RAW);
	cdmessage(d,buffer);
	break;
      }
    }

    break;
  case IDE0_MAJOR:
  case IDE1_MAJOR:
  case IDE2_MAJOR:
  case IDE3_MAJOR:
    d->nsectors=8; /* it's a define in the linux kernel; we have no
		      way of determining other than this guess tho */
    d->bigendianp=0;
    d->is_atapi=1;

    check_exceptions(d,atapi_list);

    break;
  default:
    d->nsectors=40; 
  }
  d->enable_cdda = Dummy;
  d->read_audio = cooked_read;
  d->read_toc = cooked_readtoc;
  d->set_speed = cooked_setspeed;
  ret=d->tracks=d->read_toc(d);
  if(d->tracks<1)
    return(ret);

  d->opened=1;
  if((ret=verify_read_command(d)))return(ret);
  d->error_retry=1;
  return(0);
}

