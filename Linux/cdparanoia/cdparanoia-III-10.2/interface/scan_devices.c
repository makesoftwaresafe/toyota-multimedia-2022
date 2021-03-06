/******************************************************************
 * CopyPolicy: GNU Lesser General Public License 2.1 applies
 * Copyright (C) 1998-2008 Monty xiphmont@mit.edu
 * 
 * Autoscan for or verify presence of a cdrom device
 * 
 ******************************************************************/

#define _GNU_SOURCE /* get cuserid */
#define _USE_XOPEN /* get cuserid */
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <pwd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "cdda_interface.h"
#include "low_interface.h"
#include "common_interface.h"
#include "utils.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_DEV_LEN 20 /* Safe because strings only come from below */
/* must be absolute paths! */
static char *scsi_cdrom_prefixes[]={
  "/dev/scd",
  "/dev/sr",
  NULL};
static char *scsi_generic_prefixes[]={
  "/dev/sg",
  NULL};

static char *devfs_scsi_test="/dev/scsi/";
static char *devfs_scsi_cd="cd";
static char *devfs_scsi_generic="generic";

static char *cdrom_devices[]={
  "/dev/cdrom",
  "/dev/cdroms/cdrom?",
  "/dev/hd?",
  "/dev/sg?",
  "/dev/cdu31a",
  "/dev/cdu535",
  "/dev/sbpcd",
  "/dev/sbpcd?",
  "/dev/sonycd",
  "/dev/mcd",
  "/dev/sjcd",
  /* "/dev/aztcd", timeout is too long */
  "/dev/cm206cd",
  "/dev/gscd",
  "/dev/optcd",NULL};

/* Functions here look for a cdrom drive; full init of a drive type
   happens in interface.c */

cdrom_drive *cdda_find_a_cdrom(int messagedest,char **messages){
  /* Brute force... */
  
  int i=0;
  cdrom_drive *d;

  while(cdrom_devices[i]!=NULL){

    /* is it a name or a pattern? */
    char *pos;
    if((pos=strchr(cdrom_devices[i],'?'))){
      int j;
      /* try first eight of each device */
      for(j=0;j<4;j++){
	char *buffer=copystring(cdrom_devices[i]);

	/* number, then letter */
	
	buffer[pos-(cdrom_devices[i])]=j+48;
	if((d=cdda_identify(buffer,messagedest,messages)))
	  return(d);
	idmessage(messagedest,messages,"",NULL);
	buffer[pos-(cdrom_devices[i])]=j+97;
	if((d=cdda_identify(buffer,messagedest,messages)))
	  return(d);
	idmessage(messagedest,messages,"",NULL);
      }
    }else{
      /* Name.  Go for it. */
      if((d=cdda_identify(cdrom_devices[i],messagedest,messages)))
	return(d);
      
      idmessage(messagedest,messages,"",NULL);
    }
    i++;
  }
  idmessage(messagedest,messages,
	    "\n\nNo cdrom drives accessible to %s found.\n",
	    cuserid(NULL));
  return(NULL);
}

cdrom_drive *cdda_identify(const char *device, int messagedest,char **messages){
  struct stat st;
  cdrom_drive *d=NULL;  

  idmessage(messagedest,messages,"Checking %s for cdrom...",device);

  if(stat(device,&st)){
    idperror(messagedest,messages,"\tCould not stat %s",device);
    return(NULL);
  }
    
#ifndef CDDA_TEST
  if (!S_ISCHR(st.st_mode) &&
      !S_ISBLK(st.st_mode)){
    idmessage(messagedest,messages,"\t%s is not a block or character device",device);
    return(NULL);
  }
#endif

  /* an IDE device may have scsi-ide support, SG_IO support and cooked
     support.  Prefer the SCSI variants, they give the most control */
  d=cdda_identify_scsi(NULL,device,messagedest,messages);
  if(!d)d=cdda_identify_cooked(device,messagedest,messages);
  
#ifdef CDDA_TEST
  if(!d)d=cdda_identify_test(device,messagedest,messages);
#endif

  return(d);
}

char *test_resolve_symlink(const char *file,int messagedest,char **messages){
  char resolved[PATH_MAX];
  struct stat st;
  if(lstat(file,&st)){
    idperror(messagedest,messages,"\t\tCould not stat %s",file);
    return(NULL);
  }

  if(realpath(file,resolved))
    return(strdup(resolved));

  idperror(messagedest,messages,"\t\tCould not resolve symlink %s",file);
  return(NULL);

}

cdrom_drive *cdda_identify_cooked(const char *dev, int messagedest,
				  char **messages){

  cdrom_drive *d=NULL;
  struct stat st;
  int fd=-1, i;
  int type;
  char *description=NULL;
  char *device;

  idmessage(messagedest,messages,"\tTesting %s for cooked ioctl() interface",dev);

  device=test_resolve_symlink(dev,messagedest,messages);
  if(device==NULL)return(NULL);

  if(stat(device,&st)){
    idperror(messagedest,messages,"\t\tCould not stat %s",device);
    free(device);
    return(NULL);
  }
    
  if (!S_ISCHR(st.st_mode) &&
      !S_ISBLK(st.st_mode)){
    idmessage(messagedest,messages,"\t\t%s is not a block or character device",device);
    free(device);
    return(NULL);
  }

  type=(int)(st.st_rdev>>8);
  switch (type) {
  case IDE0_MAJOR:
  case IDE1_MAJOR:
  case IDE2_MAJOR:
  case IDE3_MAJOR:
    /* Yay, ATAPI... */
    /* Ping for CDROM-ness */
    
    fd=open(device,O_RDONLY|O_NONBLOCK);
    if(fd==-1){
      idperror(messagedest,messages,"\t\tUnable to open %s",device);
      free(device);
      return(NULL);
    }
  
    if(ioctl_ping_cdrom(fd)){
      idmessage(messagedest,messages,"\t\tDevice %s is not a CDROM",device);
      close(fd);
      free(device);
      return(NULL);
    }
    {
      char *temp=atapi_drive_info(fd);
      description=catstring(NULL,"ATAPI compatible ");
      description=catstring(description,temp);
      free(temp);
    }
    
    break;
  case CDU31A_CDROM_MAJOR:
    /* major indicates this is a cdrom; no ping necessary. */
    description=copystring("Sony CDU31A or compatible");
    break;
  case CDU535_CDROM_MAJOR:
    /* major indicates this is a cdrom; no ping necessary. */
    description=copystring("Sony CDU535 or compatible");
    break;

  case MATSUSHITA_CDROM_MAJOR:
  case MATSUSHITA_CDROM2_MAJOR:
  case MATSUSHITA_CDROM3_MAJOR:
  case MATSUSHITA_CDROM4_MAJOR:
    /* major indicates this is a cdrom; no ping necessary. */
    description=copystring("non-ATAPI IDE-style Matsushita/Panasonic CR-5xx or compatible");
    break;
  case SANYO_CDROM_MAJOR:
    description=copystring("Sanyo proprietary or compatible: NOT CDDA CAPABLE");
    break;
  case MITSUMI_CDROM_MAJOR:
  case MITSUMI_X_CDROM_MAJOR:
    description=copystring("Mitsumi proprietary or compatible: NOT CDDA CAPABLE");
    break;
  case OPTICS_CDROM_MAJOR:
    description=copystring("Optics Dolphin or compatible: NOT CDDA CAPABLE");
    break;
  case AZTECH_CDROM_MAJOR:
    description=copystring("Aztech proprietary or compatible: NOT CDDA CAPABLE");
    break;
  case GOLDSTAR_CDROM_MAJOR:
    description=copystring("Goldstar proprietary: NOT CDDA CAPABLE");
    break;
  case CM206_CDROM_MAJOR:
    description=copystring("Philips/LMS CM206 proprietary: NOT CDDA CAPABLE");
    break;

  case SCSI_CDROM_MAJOR:   
  case SCSI_GENERIC_MAJOR: 
    /* Nope nope nope */
    idmessage(messagedest,messages,"\t\t%s is not a cooked ioctl CDROM.",device);
    free(device);
    return(NULL);
  default:
    /* What the hell is this? */
    idmessage(messagedest,messages,"\t\t%s is not a cooked ioctl CDROM.",device);
    free(device);
    return(NULL);
  }

  /* Minimum init */
  
  d=calloc(1,sizeof(cdrom_drive));
  d->cdda_device_name=device;
  d->ioctl_device_name=copystring(device);
  d->drive_model=description;
  d->drive_type=type;
  d->cdda_fd=fd;
  d->ioctl_fd=fd;
  d->interface=COOKED_IOCTL;
  d->bigendianp=-1; /* We don't know yet... */
  d->nsectors=-1;
  d->impl=calloc(1,sizeof(*d->impl));
  {
    /* goddamnit */
    struct timespec tv;
    d->impl->clock=(clock_gettime(CLOCK_MONOTONIC,&tv)<0?CLOCK_REALTIME:CLOCK_MONOTONIC);
  }
  idmessage(messagedest,messages,"\t\tCDROM sensed: %s\n",description);
  return(d);
}

struct  sg_id {
  long    l1; /* target | lun << 8 | channel << 16 | low_ino << 24 */
  long    l2; /* Unique id */
} sg_id;

typedef struct scsiid{
  int bus;
  int id;
  int lun;
} scsiid;

/* Even *this* isn't as simple as it bloody well should be :-P */
/* SG has an easy interface, but SCSI overall does not */
static int get_scsi_id(int fd, scsiid *id){
  struct sg_id argid;
  int busarg;

  /* get the host/id/lun */

  if(fd==-1)return(-1);
  if(ioctl(fd,SCSI_IOCTL_GET_IDLUN,&argid))return(-1);
  id->bus=argid.l2; /* for now */
  id->id=argid.l1&0xff;
  id->lun=(argid.l1>>8)&0xff;

  if(ioctl(fd,SCSI_IOCTL_GET_BUS_NUMBER,&busarg)==0)
    id->bus=busarg;
  
  return(0);
}

/* slightly wasteful, but a clean abstraction */
static char *scsi_match(const char *device,char **prefixes,
			char *devfs_test,
			char *devfs_other,
			char *prompt,int messagedest,char **messages){
  int dev=open(device,O_RDONLY|O_NONBLOCK);
  scsiid a,b;

  int i,j;
  char buffer[200];

  /* if we're running under /devfs, build the device name from the
     device we already have */
  if(!strncmp(device,devfs_test,strlen(devfs_test))){
    char *pos;
    strcpy(buffer,device);
    pos=strrchr(buffer,'/');
    if(pos){
      int matchf;
      sprintf(pos,"/%s",devfs_other);
      matchf=open(buffer,O_RDONLY|O_NONBLOCK);
      for (i = 0; (i<10) && (matchf==-1); i++) {
        fprintf(stderr, "Error trying to open %s exclusively (%s). retrying in 1 seconds.\n", buffer, strerror(errno));
        usleep(1000000 + 100000.0 * rand()/(RAND_MAX+1.0));
        matchf = open(buffer,O_RDONLY|O_NONBLOCK);
      }
      if(matchf!=-1){
	close(matchf);
	close(dev);
	return(strdup(buffer));
      }
    }
  }	

  /* get the host/id/lun */
  if(dev==-1){
    idperror(messagedest,messages,"\t\tCould not access device %s",
	     device);
    
    goto matchfail;
  }
  if(get_scsi_id(dev,&a)){
    idperror(messagedest,messages,"\t\tDevice %s could not perform ioctl()",
	     device);

    goto matchfail;
  }

  /* go through most likely /dev nodes for a match */
  for(i=0;i<25;i++){
    for(j=0;j<2;j++){
      int pattern=0;
      int matchf, k;
      
      while(prefixes[pattern]!=NULL){
	switch(j){
	case 0:
	  /* number */
	  sprintf(buffer,"%s%d",prefixes[pattern],i);
	  break;
	case 1:
	  /* number */
	  sprintf(buffer,"%s%c",prefixes[pattern],i+'a');
	  break;
	}
	
	matchf=open(buffer,O_RDONLY|O_NONBLOCK);
	for (k = 0; (k<10) && (matchf==-1); k++) {
	  fprintf(stderr, "Error trying to open %s exclusively (%s). retrying in 1 second.\n", buffer, strerror(errno));
	  usleep(1000000 + 100000.0 * rand()/(RAND_MAX+1.0));
	  matchf=open(buffer,O_RDONLY|O_NONBLOCK);
	}

	if(matchf!=-1){
	  if(get_scsi_id(matchf,&b)==0){
	    if(a.bus==b.bus && a.id==b.id && a.lun==b.lun){
	      close(matchf);
	      close(dev);
	      return(strdup(buffer));
	    }
	  }
	  close(matchf);
	}
	pattern++;
      }
    }
  } 

  idmessage(messagedest,messages,prompt,device);

matchfail:

  if(dev!=-1)close(dev);
  return(NULL);
}

void strscat(char *a,char *b,int n){
  int i;

  for(i=n;i>0;i--)
    if(b[i-1]>' ')break;

  strncat(a,b,i);
  strcat(a," ");
}

/* At this point, we're going to punt compatability before SG2, and
   allow only SG2 and SG3 */
static int verify_SG_version(cdrom_drive *d,int messagedest,
			     char **messages){
  /* are we using the new SG driver by Doug Gilbert? If not, punt */
  int version,major,minor;
  char buffer[256];
  idmessage(messagedest,messages,
	    "\nFound an accessible SCSI CDROM drive."
	    "\nLooking at revision of the SG interface in use...","");

  if(ioctl(d->cdda_fd,SG_GET_VERSION_NUM,&version)){
    /* Up, guess not. */
    idmessage(messagedest,messages,
	      "\tOOPS!  Old 2.0/early 2.1/early 2.2.x (non-ac patch) style "
	      "SG.\n\tCdparanoia no longer supports the old interface.\n","");
    return(0);
  }
  major=version/10000;
  version-=major*10000;
  minor=version/100;
  version-=minor*100;
  
  sprintf(buffer,"\tSG interface version %d.%d.%d; OK.",
	  major,minor,version);

  idmessage(messagedest,messages,buffer,"");
  return(major);
}

int check_sgio(const char *device, int messagedest, char **messages){
  int fd;
  struct sg_io_hdr hdr;

  if (!device) return 0;

  /* we don't really care what type of device it is -- if it can do
   * SG_IO, then we'll put it through the normal mmc/atapi/etc tests
   * later, but it's good enough for now. */
  fd = open(device, O_RDWR|O_NONBLOCK);
  if (fd < 0){
    idperror(messagedest,messages,
	     "\t\tCould not access device %s to test for SG_IO support",device);    
    return 0;
  }

  memset(&hdr, 0, sizeof (struct sg_io_hdr));
  /* First try with interface_id = 'A'; for all but the sg case,
   * that'll get us a -EINVAL if it supports SG_IO, and some other
   * error for all other cases. */
  hdr.interface_id = 'A';
  if (ioctl(fd, SG_IO, &hdr)) {
    switch (errno) {
    case EINVAL: /* sr and ata give us EINVAL when SG_IO is
		  * supported but interface_id is bad. */
      
    case ENOSYS: /* sg gives us ENOSYS when SG_IO is supported but
		  * interface_id is bad.  IMHO, this is wrong and
		  * needs fixing in the kernel. */
      
      close(fd);
      return 1;
      
    default: /* everything else gives ENOTTY, I think.  I'm just
	      * going to be paranoid and reject everything else. */
      
      close(fd);
      return 0;
      
    }
  }
  /* if we get here, something is dreadfuly wrong. ioctl(fd,SG_IO,&hdr)
   * handled SG_IO, but took hdr.interface_id = 'A' as valid, and an empty
   * command as good.  Don't trust it. */
  close(fd);
  return 0;
}

/* scanning is always done by specifying a device name in
   specialized_device; generic_device is only filled when the generic device
   force option is used, and in that case, use of SG (not SGIO) should indeed be
   forced */
cdrom_drive *cdda_identify_scsi(const char *generic_device, 
				const char *specialized_device, int messagedest,
				char **messages){

  cdrom_drive *d=NULL;
  struct stat i_st;
  struct stat g_st;
  int use_sgio=1;
  int i_fd=-1, i;
  int g_fd=-1;
  int version;
  int type;
  char *p;

  if(generic_device)
    idmessage(messagedest,messages,"\tTesting %s for SCSI/MMC interface",
	      generic_device);
  else
    if(specialized_device)
      idmessage(messagedest,messages,"\tTesting %s for SCSI/MMC interface",
		specialized_device);
  

  if(generic_device){
    use_sgio = 0;
    idmessage(messagedest,messages,"\t\tgeneric device forced; not testing for SG_IO interface",
	      generic_device);
    
    if(stat(generic_device,&g_st)){
      idperror(messagedest,messages,"\t\tCould not access device %s",
	       generic_device);
      return(NULL);
    }

    if((int)(g_st.st_rdev>>8)!=SCSI_GENERIC_MAJOR){
      idmessage(messagedest,messages,"\t\t%s is not a generic SCSI device",
		generic_device);
      return(NULL);
    }
  }
  
  if(specialized_device){ 
    if(stat(specialized_device,&i_st)){
      idperror(messagedest,messages,"\t\tCould not access device %s",
	       specialized_device);
      return(NULL);
    }
  }

  /* we need to resolve any symlinks for the lookup code to work */

  if(generic_device){
    generic_device=test_resolve_symlink(generic_device,messagedest,messages);
    if(generic_device==NULL)goto cdda_identify_scsi_fail;
  }
  if(specialized_device){
    specialized_device=test_resolve_symlink(specialized_device,messagedest,messages);
    if(specialized_device==NULL)goto cdda_identify_scsi_fail;
  }

  /* sgio is always preferred if it's there, unless user has forced the generic scsi device name */
  if(use_sgio){
    if(check_sgio(specialized_device,messagedest,messages)){
      idmessage(messagedest,messages,"\t\tSG_IO device: %s",specialized_device);
    }else{
      idmessage(messagedest,messages,"\t\tno SG_IO support for device: %s",specialized_device);
      use_sgio=0;
    }
  }
  
  if(!use_sgio){

    /* was a generic device passed in as the specialized device name? */
    if(specialized_device){ 
      if((int)(i_st.st_rdev>>8)==SCSI_GENERIC_MAJOR){
	char *temp=(char *)generic_device;
	generic_device=specialized_device;
	specialized_device=temp;
      }
      
      if(!generic_device || !specialized_device){
	if(generic_device){
	  specialized_device=
	    scsi_match(generic_device,scsi_cdrom_prefixes,
		       devfs_scsi_test,devfs_scsi_cd,
		       "\t\tNo cdrom device found to match generic device %s",
		       messagedest,messages);
	}else{
	  generic_device=
	    scsi_match(specialized_device,scsi_generic_prefixes,
		       devfs_scsi_test,devfs_scsi_generic,
		       "\t\tNo generic SCSI device found to match CDROM device %s",
		       messagedest,messages);
	  if(!generic_device)	
	    goto cdda_identify_scsi_fail;
	}
      }
    }
    
    idmessage(messagedest,messages,"\t\tgeneric device: %s",generic_device);
    idmessage(messagedest,messages,"\t\tioctl device: %s",(specialized_device?
							   specialized_device:
							   "not found"));
    if(specialized_device){
      if(stat(specialized_device,&i_st)){
	idperror(messagedest,messages,"\t\tCould not access cdrom device "
		 "%s",specialized_device);
	goto cdda_identify_scsi_fail;
      }
    }

    if(stat(generic_device,&g_st)){
      idperror(messagedest,messages,"\t\tCould not access generic SCSI device "
	       "%s",generic_device);
      
      goto cdda_identify_scsi_fail;
    }
  }

  if(specialized_device) {
    if(use_sgio)
      i_fd=open(specialized_device,O_RDWR|O_NONBLOCK);
    else
      i_fd=open(specialized_device,O_RDONLY|O_NONBLOCK);
  }

  if(generic_device)
    g_fd=open(generic_device,O_RDWR);
  
  
  if(specialized_device && i_fd==-1){
    idperror(messagedest,messages,"\t\tCould not open cdrom device "
	     "%s (continuing)",specialized_device);
    goto cdda_identify_scsi_fail;
  }
  
  if(generic_device && g_fd==-1){
    idperror(messagedest,messages,"\t\tCould not open generic SCSI device "
	     "%s",generic_device);
    goto cdda_identify_scsi_fail;
  }
  
  if(i_fd!=-1){
    type=(int)(i_st.st_rdev>>8);

    if(!use_sgio){
      if(type==SCSI_CDROM_MAJOR){
	if (!S_ISBLK(i_st.st_mode)) {
	  idmessage(messagedest,messages,"\t\tSCSI CDROM device %s not a "
		    "block device",specialized_device);
	  goto cdda_identify_scsi_fail;
	}
      }else{
	idmessage(messagedest,messages,"\t\tSCSI CDROM device %s has wrong "
		  "major number",specialized_device);
	goto cdda_identify_scsi_fail;
      }
    }
  }
  
  if(g_fd != -1){
    if((int)(g_st.st_rdev>>8)==SCSI_GENERIC_MAJOR){
      if (!S_ISCHR(g_st.st_mode)) {
	idmessage(messagedest,messages,"\t\tGeneric SCSI device %s not a "
		  "char device",generic_device);
	goto cdda_identify_scsi_fail;
      }
    }else{
      idmessage(messagedest,messages,"\t\tGeneric SCSI device %s has wrong "
		"major number",generic_device);
      goto cdda_identify_scsi_fail;
    }
  }

  d=calloc(1,sizeof(cdrom_drive));
  d->drive_type=type;
  d->cdda_fd=g_fd;
  d->ioctl_fd=i_fd;
  d->bigendianp=-1; /* We don't know yet... */
  d->nsectors=-1;
  d->messagedest = messagedest;
  d->impl=calloc(1,sizeof(*d->impl));
  {
    /* goddamnit */
    struct timespec tv;
    d->impl->clock=(clock_gettime(CLOCK_MONOTONIC,&tv)<0?CLOCK_REALTIME:CLOCK_MONOTONIC);
  }
  if(use_sgio){
    d->interface=SGIO_SCSI;
    d->impl->sg_buffer=(unsigned char *)(d->impl->sg_hd=malloc(MAX_BIG_BUFF_SIZE));
    g_fd=d->cdda_fd=dup(d->ioctl_fd);
  }else{
    version=verify_SG_version(d,messagedest,messages);
    switch(version){
    case -1:case 0:case 1:
      d->interface=GENERIC_SCSI;
      goto cdda_identify_scsi_fail;
    case 2:case 3:
      d->interface=GENERIC_SCSI;
      break;
    }

    /* malloc our big buffer for scsi commands */
    d->impl->sg_hd=malloc(MAX_BIG_BUFF_SIZE);
    d->impl->sg_buffer=((unsigned char *)d->impl->sg_hd)+SG_OFF;
  }

  {
    /* get the lun */
    scsiid lun;
    if(get_scsi_id(i_fd,&lun))
      d->lun=0; /* a reasonable guess on a failed ioctl */
    else
      d->lun=lun.lun;
  }

  p = (char *)scsi_inquiry(d);

  if (!p){

    /* 2.6 kernels have a bug where the block layer implements
       SG_DXFER_TO_FROM_DEVICE as a write operation instead of read.
       I've submitted a kernel patch, but it will take a while to
       propogate.  Work around it for now. --Monty */

    if(d->interface == SGIO_SCSI){
      /* test the inquiry with the workaround */
      d->interface = SGIO_SCSI_BUGGY1;
      p = (char *)scsi_inquiry(d);

      if(p){
	idmessage(messagedest,messages,
		  "\t\tThis kernel's block layer has a buggy SG_DXFER_TO_FROM_DEVICE;\n\t\t   activating workaround.\n",NULL);
      }else{
	/* Failed for some reason other than the buggy ioctl(); set the interface type back */
	d->interface = SGIO_SCSI;
      }
    }
  }

  if (!p){
    idmessage(messagedest,messages,
	      "\t\tInquiry command failed; unable to probe drive\n",NULL);
    goto cdda_identify_scsi_fail;
  }
  
  /* It would seem some TOSHIBA CDROMs gets things wrong */
  if (p &&
      !strncmp (p + 8, "TOSHIBA", 7) &&
      !strncmp (p + 16, "CD-ROM", 6) &&
      p[0] == TYPE_DISK) {
    p[0] = TYPE_ROM;
    p[1] |= 0x80;     /* removable */
  }

  if (*p != TYPE_ROM && *p != TYPE_WORM) {
    idmessage(messagedest,messages,
	      "\t\tDrive is neither a CDROM nor a WORM device\n",NULL);
    goto cdda_identify_scsi_fail;
  }

  d->drive_model=calloc(36,1);
  memcpy(d->inqbytes,p,4);
  d->cdda_device_name=copystring(generic_device);
  d->ioctl_device_name=copystring(specialized_device);
  d->drive_model=calloc(36,1);
  strscat(d->drive_model,p+8,8);
  strscat(d->drive_model,p+16,16);
  strscat(d->drive_model,p+32,4);

  idmessage(messagedest,messages,"\nCDROM model sensed sensed: %s",d->drive_model);
  return(d);
  
cdda_identify_scsi_fail:
  if(generic_device)free((char *)generic_device);
  if(specialized_device)free((char *)specialized_device);
  if(i_fd!=-1)close(i_fd);
  if(g_fd!=-1)close(g_fd);
  if(d){
    if(d->impl){
      if(d->impl->sg_hd)free(d->impl->sg_hd);
      free(d->impl);
    }
    free(d);
  }
  return(NULL);
}

#ifdef CDDA_TEST

cdrom_drive *cdda_identify_test(const char *filename, int messagedest,
				char **messages){
  
  cdrom_drive *d=NULL;
  struct stat st;
  int fd=-1,i;

  idmessage(messagedest,messages,"\tTesting %s for file/test interface",
	    filename);

  if(stat(filename,&st)){
    idperror(messagedest,messages,"\t\tCould not access file %s",
	     filename);
    return(NULL);
  }

  if(!S_ISREG(st.st_mode)){
    idmessage(messagedest,messages,"\t\t%s is not a regular file",
		  filename);
    return(NULL);
  }

  fd=open(filename,O_RDONLY);
  if(fd==-1){
    idperror(messagedest,messages,"\t\tCould not open file %s",filename);
    return(NULL);
  }
  
  d=calloc(1,sizeof(cdrom_drive));

  d->cdda_device_name=copystring(filename);
  d->ioctl_device_name=copystring(filename);
  d->drive_type=-1;
  d->cdda_fd=fd;
  d->ioctl_fd=fd;
  d->interface=TEST_INTERFACE;
  d->bigendianp=-1; /* We don't know yet... */
  d->nsectors=-1;
  d->impl=calloc(1,sizeof(*d->impl));
  d->drive_model=copystring("File based test interface");
  idmessage(messagedest,messages,"\t\tCDROM sensed: %s\n",d->drive_model);
  
  return(d);
}

#endif
