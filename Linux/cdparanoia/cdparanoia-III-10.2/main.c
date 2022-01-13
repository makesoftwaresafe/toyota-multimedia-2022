/*
 * Copyright: GNU Public License 2 applies
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * cdparanoia (C) 2008 Monty <monty@xiph.org>
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "interface/cdda_interface.h"
#include "paranoia/cdda_paranoia.h"
#include "utils.h"
#include "report.h"
#include "version.h"
#include "header.h"

extern int analyze_cache(cdrom_drive *d, FILE *progress, FILE *log, int speed);

static long parse_offset(cdrom_drive *d, char *offset, int begin){
  long track=-1;
  long hours=-1;
  long minutes=-1;
  long seconds=-1;
  long sectors=-1;
  char *time=NULL,*temp=NULL;
  long ret;

  if(offset==NULL)return(-1);

  /* seperate track from time offset */
  temp=strchr(offset,']');
  if(temp){
    *temp='\0';
    temp=strchr(offset,'[');
    if(temp==NULL){
      report("Error parsing span argument");
      exit(1);
    }
    *temp='\0';
    time=temp+1;
  }

  /* parse track */
  {
    int chars=strspn(offset,"0123456789");
    if(chars>0){
      offset[chars]='\0';
      track=atoi(offset);
      if(track<0 || track>d->tracks){ /*take track 0 as pre-gap of 1st track*/
	report("Track #%ld does not exist.",track);
	exit(1);
      }
    }
  }

  while(time){
    long val,chars;
    char *sec=strrchr(time,'.');
    if(!sec)sec=strrchr(time,':');
    if(!sec)sec=time-1;

    chars=strspn(sec+1,"0123456789");
    if(chars)
      val=atoi(sec+1);
    else
      val=0;
    
    switch(*sec){
    case '.':
      if(sectors!=-1){
	report("Error parsing span argument");
	exit(1);
      }
      sectors=val;
      break;
    default:
      if(seconds==-1)
	seconds=val;
      else
	if(minutes==-1)
	  minutes=val;
	else
	  if(hours==-1)
	    hours=val;
	  else{
	    report("Error parsing span argument");
	    exit(1);
	  }
      break;
    }
	 
    if(sec<=time)break;
    *sec='\0';
  }

  if(track==-1){
    if(seconds==-1 && sectors==-1)return(-1);
    if(begin==-1)
      ret=cdda_disc_firstsector(d);
    else
      ret=begin;
  }else{
    if(seconds==-1 && sectors==-1){
      if(begin==-1){ /* first half of a span */
	return(cdda_track_firstsector(d,track));
      }else{
	return(cdda_track_lastsector(d,track));
      }
    }else{
      /* relative offset into a track */
      ret=cdda_track_firstsector(d,track);
    }
  }
   
  /* OK, we had some sort of offset into a track */

  if(sectors!=-1)ret+=sectors;
  if(seconds!=-1)ret+=seconds*75;
  if(minutes!=-1)ret+=minutes*60*75;
  if(hours!=-1)ret+=hours*60*60*75;

  /* We don't want to outside of the track; if it's relative, that's OK... */
  if(track!=-1){
    if(cdda_sector_gettrack(d,ret)!=track){
      report("Time/sector offset goes beyond end of specified track.");
      exit(1);
    }
  }

  /* Don't pass up end of session */

  if(ret>cdda_disc_lastsector(d)){
    report("Time/sector offset goes beyond end of disc.");
    exit(1);
  }

  return(ret);


}

static void display_toc(cdrom_drive *d){
  long audiolen=0;
  int i;
  report("\nTable of contents (audio tracks only):\n"
	 "track        length               begin        copy pre ch\n"
	 "===========================================================");
  
  for(i=1;i<=d->tracks;i++)
    if(cdda_track_audiop(d,i)>0){

      long sec=cdda_track_firstsector(d,i);
      long off=cdda_track_lastsector(d,i)-sec+1;
      
      report("%3d.  %7ld [%02d:%02d.%02d]  %7ld [%02d:%02d.%02d]  %s %s %s",
	     i,
	     off,(int)(off/(60*75)),(int)((off/75)%60),(int)(off%75),
	     sec,(int)(sec/(60*75)),(int)((sec/75)%60),(int)(sec%75),
	     cdda_track_copyp(d,i)?"  OK":"  no",
	     cdda_track_preemp(d,i)?" yes":"  no",
	     cdda_track_channels(d,i)==2?" 2":" 4");
      audiolen+=off;
    }
  report("TOTAL %7ld [%02d:%02d.%02d]    (audio only)",
	 audiolen,(int)(audiolen/(60*75)),(int)((audiolen/75)%60),
	 (int)(audiolen%75));
  report(" ");
}

static void usage(FILE *f){
  fprintf( f,
VERSION"\n"


"(C) 2008 Monty <monty@xiph.org> and Xiph.Org\n\n"	\
		"Report bugs to paranoia@xiph.org\n"\
		"http://www.xiph.org/paranoia/\n"

"USAGE:\n"
"  cdparanoia [options] <span> [outfile]\n\n"

"OPTIONS:\n"
"  -A --analyze-drive              : run and log a complete analysis of drive\n"
"                                    caching, timing and reading behavior;\n"
"                                    verifies that cdparanoia is correctly\n"
"                                    modelling a sprcific drive's cache and\n"
"                                    read behavior. Implies -vQL\n\n"
"  -v --verbose                    : extra verbose operation\n"
"  -q --quiet                      : quiet operation\n"
"  -e --stderr-progress            : force output of progress information to\n"
"                                    stderr (for wrapper scripts)\n"
"  -l --log-summary [<file>]       : save result summary to file, default\n"
"                                    filename cdparanoia.log\n"
"  -L --log-debug   [<file>]       : save detailed device autosense and\n"
"                                    debugging output to file, default\n"
"                                    filename cdparanoia.log\n"
"  -V --version                    : print version info and quit\n"
"  -Q --query                      : autosense drive, query disc and quit\n"
"  -B --batch                      : 'batch' mode (saves each track to a\n"
"                                    seperate file.\n"
"  -s --search-for-drive           : do an exhaustive search for drive\n"
"  -h --help                       : print help\n\n"

"  -p --output-raw                 : output raw 16 bit PCM in host byte \n"
"                                    order\n"
"  -r --output-raw-little-endian   : output raw 16 bit little-endian PCM\n"
"  -R --output-raw-big-endian      : output raw 16 bit big-endian PCM\n"
"  -w --output-wav                 : output as WAV file (default)\n"
"  -f --output-aiff                : output as AIFF file\n"
"  -a --output-aifc                : output as AIFF-C file\n\n"

"  -c --force-cdrom-little-endian  : force treating drive as little endian\n"
"  -C --force-cdrom-big-endian     : force treating drive as big endian\n"
"  -n --force-default-sectors <n>  : force default number of sectors in read\n"
"                                    to n sectors\n"
"  -o --force-search-overlap  <n>  : force minimum overlap search during\n"
"                                    verification to n sectors\n"
"  -d --force-cdrom-device   <dev> : use specified device; disallow \n"
"                                    autosense\n"
"  -k --force-cooked-device  <dev> : use specified cdrom device and force\n"
"                                    use of the old 'cooked ioctl' kernel\n"
"                                    interface. -k cannot be used with -d\n"
"                                    or -g.\n"
"  -g --force-generic-device <dev> : use specified generic scsi device and\n"
"                                    force use of the old SG kernel\n"
"                                    interface. -g cannot be used with -k.\n"
"  -S --force-read-speed <n>       : read from device at specified speed; by\n"
"                                    default, cdparanoia sets drive to full\n"
"                                    speed.\n"
"  -t --toc-offset <n>             : Add <n> sectors to the values reported\n"
"                                    when addressing tracks. May be negative\n"
"  -T --toc-bias                   : Assume that the beginning offset of \n"
"                                    track 1 as reported in the TOC will be\n"
"                                    addressed as LBA 0.  Necessary for some\n"
"                                    Toshiba drives to get track boundaries\n"
"                                    correct\n"
"  -O --sample-offset <n>          : Add <n> samples to the offset when\n"
"                                    reading data.  May be negative.\n"
"  -z --never-skip[=n]             : never accept any less than perfect\n"
"                                    data reconstruction (don't allow 'V's)\n"
"                                    but if [n] is given, skip after [n]\n"
"                                    retries without progress.\n"
"  -Z --disable-paranoia           : disable all paranoia checking\n"
"  -Y --disable-extra-paranoia     : only do cdda2wav-style overlap checking\n"
"  -X --abort-on-skip              : abort on imperfect reads/skips\n\n"

"OUTPUT SMILIES:\n"
"  :-)   Normal operation, low/no jitter\n"
"  :-|   Normal operation, considerable jitter\n"
"  :-/   Read drift\n"
"  :-P   Unreported loss of streaming in atomic read operation\n"
"  8-|   Finding read problems at same point during reread; hard to correct\n"
"  :-0   SCSI/ATAPI transport error\n"
"  :-(   Scratch detected\n"
"  ;-(   Gave up trying to perform a correction\n"
"  8-X   Aborted (as per -X) due to a scratch/skip\n"
"  :^D   Finished extracting\n\n"

"PROGRESS BAR SYMBOLS:\n"
"<space> No corrections needed\n"
"   -    Jitter correction required\n"
"   +    Unreported loss of streaming/other error in read\n"
"   !    Errors are getting through stage 1 but corrected in stage2\n"
"   e    SCSI/ATAPI transport error (corrected)\n"
"   V    Uncorrected error/skip\n\n"

"SPAN ARGUMENT:\n"
"The span argument may be a simple track number or a offset/span\n"
"specification.  The syntax of an offset/span takes the rough form:\n\n"
  
"                       1[ww:xx:yy.zz]-2[aa:bb:cc.dd] \n\n"

"Here, 1 and 2 are track numbers; the numbers in brackets provide a\n"
"finer grained offset within a particular track. [aa:bb:cc.dd] is in\n"
"hours/minutes/seconds/sectors format. Zero fields need not be\n"
"specified: [::20], [:20], [20], [20.], etc, would be interpreted as\n"
"twenty seconds, [10:] would be ten minutes, [.30] would be thirty\n"
"sectors (75 sectors per second).\n\n"

"When only a single offset is supplied, it is interpreted as a starting\n"
"offset and ripping will continue to the end of he track.  If a single\n"
"offset is preceeded or followed by a hyphen, the implicit missing\n"
"offset is taken to be the start or end of the disc, respectively. Thus:\n\n"

"    1:[20.35]    Specifies ripping from track 1, second 20, sector 35 to \n"
"                 the end of track 1.\n\n"

"    1:[20.35]-   Specifies ripping from 1[20.35] to the end of the disc\n\n"

"    -2           Specifies ripping from the beginning of the disc up to\n"
"                 (and including) track 2\n\n"

"    -2:[30.35]   Specifies ripping from the beginning of the disc up to\n"
"                 2:[30.35]\n\n"

"    2-4          Specifies ripping from the beginning of track two to the\n"
"                 end of track 4.\n\n"

"Don't forget to protect square brackets and preceeding hyphens from\n"
"the shell...\n\n"
"A few examples, protected from the shell:\n"
"  A) query only with exhaustive search for a drive and full reporting\n"
"     of autosense:\n"
"       cdparanoia -vsQ\n\n"
"  B) extract up to and including track 3, putting each track in a seperate\n"
"     file:\n"
"       cdparanoia -B -- \"-3\"\n\n"
"  C) extract from track 1, time 0:30.12 to 1:10.00:\n"
"       cdparanoia \"1[:30.12]-1[1:10]\"\n\n"

"Submit bug reports to paranoia@xiph.org\n\n");
}

long callbegin;
long callend;
long callscript=0;

static char *callback_strings[16]={"wrote",
                                   "finished",
				   "read",
				   "verify",
				   "jitter",
				   "correction",
				   "scratch",
				   "scratch repair",
				   "skip",
				   "drift",
				   "backoff",
				   "overlap",
				   "dropped",
				   "duped",
				   "transport error",
                                   "cache error"};

static int skipped_flag=0;
static int abort_on_skip=0;
FILE *logfile = NULL;
static void callback(long inpos, int function){
  /*

 (== PROGRESS == [--+!---x-------------->           | 007218 01 ] == :-) . ==) 

 */

  int graph=30;
  char buffer[256];
  static long c_sector=0,v_sector=0;
  static char dispcache[]="                              ";
  static int last=0;
  static long lasttime=0;
  long sector,osector=0;
  struct timeval thistime;
  static char heartbeat=' ';
  int position=0,aheadposition=0;
  static int overlap=0;
  static int printit=-1;

  static int slevel=0;
  static int slast=0;
  static int stimeout=0;
  static int cacheerr=0;
  char *smilie="= :-)";
  
  if(callscript)
    fprintf(stderr,"##: %d [%s] @ %ld\n",
	    function,(function>=-2&&function<=13?callback_strings[function+2]:
		      ""),inpos);
  else{
    if(function==PARANOIA_CB_CACHEERR){
      if(!cacheerr){
	fprintf(stderr,
		"\rWARNING: The CDROM drive appears to be seeking impossibly quickly.\n"
		"This could be due to timer bugs, a drive that really is improbably fast,\n"
		"or, most likely, a bug in cdparanoia's cache modelling.\n\n"
		"Please consider using the -A option to perform an analysis run, then mail\n"
		"the cdparanoia.log file produced by the analysis to paranoia-dev@xiph.org\n"
		"to assist developers in correcting the problem.\n\n");
      }
      cacheerr++;
    }
  }

  if(!quiet){
    long test;
    osector=inpos;
    sector=inpos/CD_FRAMEWORDS;
    
    if(printit==-1){
      if(isatty(STDERR_FILENO)){
	printit=1;
      }else{
	printit=0;
      }
    }

    if(printit==1){  /* else don't bother; it's probably being 
			redirected */
      position=((float)(sector-callbegin)/
		(callend-callbegin))*graph;
      
      aheadposition=((float)(c_sector-callbegin)/
		     (callend-callbegin))*graph;
      
      if(function==-2){
	v_sector=sector;
	return;
      }
      if(function==-1){
	last=8;
	heartbeat='*';
	slevel=0;
	v_sector=sector;
      }else
	if(position<graph && position>=0)
	  switch(function){
	  case PARANOIA_CB_VERIFY:
	    if(stimeout>=30){
	      if(overlap>CD_FRAMEWORDS)
		slevel=2;
	      else
		slevel=1;
	    }
	    break;
	  case PARANOIA_CB_READ:
	    if(sector>c_sector)c_sector=sector;
	    break;
	    
	  case PARANOIA_CB_FIXUP_EDGE:
	    if(stimeout>=5){
	      if(overlap>CD_FRAMEWORDS)
		slevel=2;
	      else
		slevel=1;
	    }
	    if(dispcache[position]==' ') 
	      dispcache[position]='-';
	    break;
	  case PARANOIA_CB_FIXUP_ATOM:
	    if(slevel<3 || stimeout>5)slevel=3;
	    if(dispcache[position]==' ' ||
	       dispcache[position]=='-')
	      dispcache[position]='+';
	    break;
	  case PARANOIA_CB_READERR:
	    slevel=6;
	    if(dispcache[position]!='V' && dispcache[position]!='C')
	      dispcache[position]='e';
	    break;
	  case PARANOIA_CB_CACHEERR:
	    slevel=8;
	    dispcache[position]='C';
	    break;
	  case PARANOIA_CB_SKIP:
	    slevel=8;
	    if(dispcache[position]!='C')
	      dispcache[position]='V';
	    break;
	  case PARANOIA_CB_OVERLAP:
	    overlap=osector;
	    break;
	  case PARANOIA_CB_SCRATCH:
	    slevel=7;
	    break;
	  case PARANOIA_CB_DRIFT:
	    if(slevel<4 || stimeout>5)slevel=4;
	    break;
	  case PARANOIA_CB_FIXUP_DROPPED:
	  case PARANOIA_CB_FIXUP_DUPED:
	    slevel=5;
	    if(dispcache[position]==' ' ||
	       dispcache[position]=='-' ||
	       dispcache[position]=='+')
	      dispcache[position]='!';
	    break;
	  }
    
      switch(slevel){
      case 0:  /* finished, or no jitter */
	if(skipped_flag)
	  smilie=" 8-X";
	else
	  smilie=" :^D";
	break;
      case 1:  /* normal.  no atom, low jitter */
	smilie=" :-)";
	break;
      case 2:  /* normal, overlap > 1 */
	smilie=" :-|";
	break; 
      case 4:  /* drift */
	smilie=" :-/";
	break;
      case 3:  /* unreported loss of streaming */
	smilie=" :-P";
	break;
      case 5:  /* dropped/duped bytes */
	smilie=" 8-|";
	break;
      case 6:  /* scsi error */
	smilie=" :-0";
	break;
      case 7:  /* scratch */
	smilie=" :-(";
	break;
      case 8:  /* skip */
	smilie=" ;-(";
	skipped_flag=1;
	break;
	
      }
      
      gettimeofday(&thistime,NULL);
      test=thistime.tv_sec*10+thistime.tv_usec/100000;

      if(lasttime!=test || function==-1 || slast!=slevel){
	if(lasttime!=test || function==-1){
	  last++;
	  lasttime=test;
	  if(last>7)last=0;
	  stimeout++;
	  switch(last){
	  case 0:
	    heartbeat=' ';
	    break;
	  case 1:case 7:
	    heartbeat='.';
	    break;
	  case 2:case 6:
	    heartbeat='o';
	    break;
	  case 3:case 5:  
	    heartbeat='0';
	    break;
	  case 4:
	    heartbeat='O';
	    break;
	  }
	  if(function==-1)
	    heartbeat='*';

	}
	if(slast!=slevel){
	  stimeout=0;
	}
	slast=slevel;
	
	if(abort_on_skip && skipped_flag && function !=-1){
	  sprintf(buffer,
		  "\r (== PROGRESS == [%s| %06ld %02d ] ==%s %c ==)   ",
		  "  ...aborting; please wait... ",
		  v_sector,overlap/CD_FRAMEWORDS,smilie,heartbeat);
	}else{
	  if(v_sector==0)
	    sprintf(buffer,
		    "\r (== PROGRESS == [%s| ...... %02d ] ==%s %c ==)   ",
		    dispcache,overlap/CD_FRAMEWORDS,smilie,heartbeat);
	  
	  else
	    sprintf(buffer,
		    "\r (== PROGRESS == [%s| %06ld %02d ] ==%s %c ==)   ",
		    dispcache,v_sector,overlap/CD_FRAMEWORDS,smilie,heartbeat);
	  
	  if(aheadposition>=0 && aheadposition<graph && !(function==-1))
	    buffer[aheadposition+19]='>';
	}
   
	fprintf(stderr,buffer);
       
	if (logfile != NULL && function==-1) {
	  fprintf(logfile,buffer+1);
	  fprintf(logfile,"\n\n");
	  fflush(logfile);
	}
      }
    }
  }
  
  /* clear the indicator for next batch */
  if(function==-1)
    memset(dispcache,' ',graph);
}

const char *optstring = "escCn:o:O:d:g:k:S:prRwafvqVQhZz::YXWBi:Tt:l::L::A";

struct option options [] = {
	{"stderr-progress",no_argument,NULL,'e'},
	{"search-for-drive",no_argument,NULL,'s'},
	{"force-cdrom-little-endian",no_argument,NULL,'c'},
	{"force-cdrom-big-endian",no_argument,NULL,'C'},
	{"force-default-sectors",required_argument,NULL,'n'},
	{"force-search-overlap",required_argument,NULL,'o'},
	{"force-cdrom-device",required_argument,NULL,'d'},
	{"force-cooked-device",required_argument,NULL,'k'},
	{"force-generic-device",required_argument,NULL,'g'},
	{"force-read-speed",required_argument,NULL,'S'},
	{"sample-offset",required_argument,NULL,'O'},
	{"toc-offset",required_argument,NULL,'t'},
	{"toc-bias",no_argument,NULL,'T'},
	{"output-raw",no_argument,NULL,'p'},
	{"output-raw-little-endian",no_argument,NULL,'r'},
	{"output-raw-big-endian",no_argument,NULL,'R'},
	{"output-wav",no_argument,NULL,'w'},
	{"output-aiff",no_argument,NULL,'f'},
	{"output-aifc",no_argument,NULL,'a'},
	{"batch",no_argument,NULL,'B'},
	{"verbose",no_argument,NULL,'v'},
	{"quiet",no_argument,NULL,'q'},
	{"version",no_argument,NULL,'V'},
	{"query",no_argument,NULL,'Q'},
	{"help",no_argument,NULL,'h'},
	{"analyze-drive",no_argument,NULL,'A'},
	{"disable-paranoia",no_argument,NULL,'Z'},
	{"disable-extra-paranoia",no_argument,NULL,'Y'},
	{"abort-on-skip",no_argument,NULL,'X'},
	{"disable-fragmentation",no_argument,NULL,'F'},
	{"output-info",required_argument,NULL,'i'},
	{"never-skip",optional_argument,NULL,'z'},
	{"log-summary",optional_argument,NULL,'l'},
	{"log-debug",optional_argument,NULL,'L'},

	{NULL,0,NULL,0}
};

long blocking_write(int outf, char *buffer, long num){
  long words=0,temp;

  while(words<num){
    temp=write(outf,buffer+words,num-words);
    if(temp==-1){
      if(errno!=EINTR && errno!=EAGAIN)
	return(-1);
      temp=0;
    }
    words+=temp;
  }
  return(0);
}

static cdrom_drive *d=NULL;
static cdrom_paranoia *p=NULL;

static void cleanup(void){
  if(p)paranoia_free(p);
  if(d)cdda_close(d);
}

int main(int argc,char *argv[]){
  int toc_bias=0;
  int toc_offset=0;
  int sample_offset=0;
  int force_cdrom_endian=-1;
  int force_cdrom_sectors=-1;
  int force_cdrom_overlap=-1;
  char *force_cdrom_device=NULL;
  char *force_generic_device=NULL;
  char *force_cooked_device=NULL;
  int force_cdrom_speed=0;
  int max_retries=20;
  char *span=NULL;
  int output_type=1; /* 0=raw, 1=wav, 2=aifc */
  int output_endian=0; /* -1=host, 0=little, 1=big */
  int query_only=0;
  int batch=0,i;
  int run_cache_test=0;

  char *logfile_name=NULL;
  char *reportfile_name=NULL;
  int logfile_open=0;
  int reportfile_open=0;

  /* full paranoia, but allow skipping */
  int paranoia_mode=PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP; 

  char *info_file=NULL;
  int out;

  int search=0;
  int c,long_option_index;

  atexit(cleanup);

  while((c=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
    case 'B':
      batch=1;
      break;
    case 'c':
      force_cdrom_endian=0;
      break;
    case 'C':
      force_cdrom_endian=1;
      break;
    case 'n':
      force_cdrom_sectors=atoi(optarg);
      break;
    case 'o':
      force_cdrom_overlap=atoi(optarg);
      break;
    case 'd':
      if(force_cdrom_device)free(force_cdrom_device);
      force_cdrom_device=copystring(optarg);
      break;
    case 'g':
      if(force_cooked_device){
	report("-g option incompatable with -k\n");
	exit(1);
      }
      force_cooked_device=NULL;
      if(force_generic_device)free(force_generic_device);
      force_generic_device=copystring(optarg);
      break;
    case 'k':
      if(force_generic_device || force_cdrom_device){
	report("-k option incompatable with -d and -g\n");
	exit(1);
      }
      if(force_cooked_device)free(force_cooked_device);
      force_cooked_device=copystring(optarg);
      break;
    case 'S':
      force_cdrom_speed=atoi(optarg);
      break;
    case 'p':
      output_type=0;
      output_endian=-1;
      break;
    case 'r':
      output_type=0;
      output_endian=0;
      break;
    case 'R':
      output_type=0;
      output_endian=1;
      break;
    case 'w':
      output_type=1;
      output_endian=0;
      break;
    case 'a':
      output_type=2;
      output_endian=1;
      break;
    case 'f':
      output_type=3;
      output_endian=1;
      break;
    case 'v':
      verbose=CDDA_MESSAGE_PRINTIT;
      quiet=0;
      break;
    case 's':
      search=1;
      break;
    case 'q':
      verbose=CDDA_MESSAGE_FORGETIT;
      quiet=1;
      break;
    case 'e':
      callscript=1;
      fprintf(stderr,"Sending all callbacks to stderr for wrapper script\n");
      break;
    case 'V':
      fprintf(stderr,VERSION);
      fprintf(stderr,"\n");
      exit(0);
      break;
    case 'Q':
      query_only=1;
      break;
    case 'h':
      usage(stdout);
      exit(0);
    case 'Z':
      paranoia_mode=PARANOIA_MODE_DISABLE; 
      break;
    case 'A':
      run_cache_test=1;
      query_only=1;
      reportfile_open=1;
      verbose=CDDA_MESSAGE_PRINTIT;
      break;
    case 'z':
      if (optarg) {
        max_retries = atoi (optarg);
        paranoia_mode&=~PARANOIA_MODE_NEVERSKIP;
      } else {
        paranoia_mode|=PARANOIA_MODE_NEVERSKIP;
      }
      break;
    case 'Y':
      paranoia_mode|=PARANOIA_MODE_OVERLAP; /* cdda2wav style overlap 
						check only */
      paranoia_mode&=~PARANOIA_MODE_VERIFY;
      break;
    case 'X':
      /*paranoia_mode&=~(PARANOIA_MODE_SCRATCH|PARANOIA_MODE_REPAIR);*/
      abort_on_skip=1;
      break;
    case 'W':
      paranoia_mode&=~PARANOIA_MODE_REPAIR;
      break;
    case 'F':
      paranoia_mode&=~(PARANOIA_MODE_FRAGMENT);
      break;
    case 'i':
      if(info_file)free(info_file);
      info_file=copystring(info_file);
      break;
    case 'T':
      toc_bias=-1;
      break;
    case 't':
      toc_offset=atoi(optarg);
      break;
    case 'l':
      if(logfile_name)free(logfile_name);
      logfile_name=NULL;
      if(optarg)
	logfile_name=strdup(optarg);
      logfile_open=1;
      break;
    case 'L':
      if(reportfile_name)free(reportfile_name);
      reportfile_name=NULL;
      if(optarg)
	reportfile_name=strdup(optarg);
      reportfile_open=1;
      break;
    case 'O':
      sample_offset=atoi(optarg);
      break;
    default:
      usage(stderr);
      exit(1);
    }
  }

  if(logfile_open){
    if(logfile_name==NULL)
      logfile_name=strdup("cdparanoia.log");
    if(!strcmp(logfile_name,"-")){
      logfile=stdout;
      logfile_open=0;
    }else{
      logfile=fopen(logfile_name,"w");
      if(logfile==NULL){
	report("Cannot open log summary file %s: %s",logfile_name,
	       strerror(errno));
	exit(1);
      }
    }
  }
  if(reportfile_open){
    if(reportfile_name==NULL)
      reportfile_name=strdup("cdparanoia.log");
    if(!strcmp(reportfile_name,"-")){
      reportfile=stdout;
      reportfile_open=0;
    }else{
      if(logfile_name && !strcmp(reportfile_name,logfile_name)){
	reportfile=logfile;
	reportfile_open=0;
      }else{
	reportfile=fopen(reportfile_name,"w");
	if(reportfile==NULL){
	  report("Cannot open debug log file %s: %s",reportfile_name,
		 strerror(errno));
	  exit(1);
	}
      }
    }
  }
    
  if(logfile){
    /* log command line and version */
    int i;
    for (i = 0; i < argc; i++) 
      fprintf(logfile,"%s ",argv[i]);
    fprintf(logfile,"\n");

    if(reportfile!=logfile){
      fprintf(logfile,VERSION);
      fprintf(logfile,"\n");
      fprintf(logfile,"Using cdda library version: %s\n",cdda_version());
      fprintf(logfile,"Using paranoia library version: %s\n",paranoia_version());
    }
    fflush(logfile);
  }

  if(reportfile && reportfile!=logfile){
    /* log command line */
    int i;
    for (i = 0; i < argc; i++) 
      fprintf(reportfile,"%s ",argv[i]);
    fprintf(reportfile,"\n");
    fflush(reportfile);
  }

  if(optind>=argc && !query_only){
    if(batch)
      span=NULL;
    else{
      /* D'oh.  No span. Fetch me a brain, Igor. */
      usage(stderr);
      exit(1);
    }
  }else
    span=copystring(argv[optind]);

  report(VERSION);
  if(verbose){
    report("Using cdda library version: %s",cdda_version());
    report("Using paranoia library version: %s",paranoia_version());
  }

  /* Query the cdrom/disc; we may need to override some settings */

  if(force_cooked_device){
    d=cdda_identify_cooked(force_cooked_device,verbose,NULL);
  }else if(force_generic_device)
    d=cdda_identify_scsi(force_generic_device,force_cdrom_device,verbose,NULL);
  else
    if(force_cdrom_device)
      d=cdda_identify(force_cdrom_device,verbose,NULL);
    else
      if(search)
	d=cdda_find_a_cdrom(verbose,NULL);
      else{
	/* does the /dev/cdrom link exist? */
	struct stat s;
	if(lstat("/dev/cdrom",&s)){
	  /* no link.  Search anyway */
	  d=cdda_find_a_cdrom(verbose,NULL);
	}else{
	  d=cdda_identify("/dev/cdrom",verbose,NULL);
	  if(d==NULL  && !verbose){
	    verbose=1;
	    report("\n/dev/cdrom exists but isn't accessible.  By default,\n"
		   "cdparanoia stops searching for an accessible drive here.\n"
		   "Consider using -sv to force a more complete autosense\n"
		   "of the machine.\n\nMore information about /dev/cdrom:");

	    d=cdda_identify("/dev/cdrom",CDDA_MESSAGE_PRINTIT,NULL);
	    report("\n");
	    exit(1);
	  }else
	    report(" ");
	}
      }

  if(!d){
    if(!verbose)
      report("\nUnable to open cdrom drive; -v will give more information.");
    exit(1);
  }
  
  if(verbose)
    cdda_verbose_set(d,CDDA_MESSAGE_PRINTIT,CDDA_MESSAGE_PRINTIT);
  else
    cdda_verbose_set(d,CDDA_MESSAGE_PRINTIT,CDDA_MESSAGE_FORGETIT);

  /* possibly force hand on endianness of drive, sector request size */
  if(force_cdrom_endian!=-1){
    d->bigendianp=force_cdrom_endian;
    switch(force_cdrom_endian){
    case 0:
      report("Forcing CDROM sense to little-endian; ignoring preset and autosense");
      break;
    case 1:
      report("Forcing CDROM sense to big-endian; ignoring preset and autosense");
      break;
    }
  }
  if(force_cdrom_sectors!=-1){
    if(force_cdrom_sectors<0 || force_cdrom_sectors>100){
      report("Default sector read size must be 1<= n <= 100\n");
      cdda_close(d);
      d=NULL;
      exit(1);
    }
    report("Forcing default to read %d sectors; "
	   "ignoring preset and autosense",force_cdrom_sectors);
    d->nsectors=force_cdrom_sectors;
    d->bigbuff=force_cdrom_sectors*CD_FRAMESIZE_RAW;
  }
  if(force_cdrom_overlap!=-1){
    if(force_cdrom_overlap<0 || force_cdrom_overlap>75){
      report("Search overlap sectors must be 0<= n <=75\n");
      cdda_close(d);
      d=NULL;
      exit(1);
    }
    report("Forcing search overlap to %d sectors; "
	   "ignoring autosense",force_cdrom_overlap);
  }

  switch(cdda_open(d)){
  case -2:case -3:case -4:case -5:
    report("\nUnable to open disc.  Is there an audio CD in the drive?");
    exit(1);
  case -6:
    report("\ncdparanoia could not find a way to read audio from this drive.");
    exit(1);
  case 0:
    break;
  default:
    report("\nUnable to open disc.");
    exit(1);
  }

  if(force_cdrom_speed==0)force_cdrom_speed=-1;
  if(force_cdrom_speed!=-1){
    report("\nAttempting to set speed to %dx... ",force_cdrom_speed);
  }else{
    if(verbose)
      report("\nAttempting to set cdrom to full speed... ");
  }

  if(cdda_speed_set(d,force_cdrom_speed)){
    if(verbose || force_cdrom_speed!=-1)
      report("\tCDROM speed set FAILED. Continuing anyway...");
  }else{
    if(verbose)
      report("\tdrive returned OK.");
  }
  
  if(run_cache_test){
    int warn=analyze_cache(d, stderr, reportfile, force_cdrom_speed);
    
    if(warn==0){
      reportC("\nDrive tests OK with Paranoia.\n\n");
      return 0;
    }

    if(warn==1)
      reportC("\nWARNING! PARANOIA MAY NOT BE TRUSTWORTHY WITH THIS DRIVE!\n"
	      "\nThe Paranoia library may not model this CDROM drive's cache"
	      "\ncorrectly according to this analysis run. Analysis is not"
	      "\nalways accurate (it can be fooled by machine load or random"
	      "\nkernel latencies), but if a failed result happens more often"
	      "\nthan one time in twenty on an unloaded machine, please mail"
	      "\nthe %s file produced by this failed analysis to"
	      "\nparanoia-dev@xiph.org to assist developers in extending"
	      "\nParanoia to handle this CDROM properly.\n\n",reportfile_name);
    return 1;
  }


  /* Dump the TOC */
  if(query_only || verbose)display_toc(d);
  if(query_only)exit(0);

  /* bias the disc.  A hack.  Of course. */
  /* we may need to read before or past user area; this is never
     default, and we do it because the [allegedly informed] user told
     us to */
  if(sample_offset){
    toc_offset+=sample_offset/588;
    sample_offset%=588;
    if(sample_offset<0){
      sample_offset+=588;
      toc_offset--;
    }
  }

  if(toc_bias){
    toc_offset=-cdda_track_firstsector(d,1);
  }
  for(i=0;i<d->tracks+1;i++)
    d->disc_toc[i].dwStartSector+=toc_offset;


  if(d->nsectors==1){
    report("WARNING: The autosensed/selected sectors per read value is\n"
	   "         one sector, making it very unlikely Paranoia can \n"
	   "         work.\n\n"
	   "         Attempting to continue...\n\n");
  }

  /* parse the span, set up begin and end sectors */

  {
    long first_sector;
    long last_sector;
    long batch_first;
    long batch_last;
    int batch_track;

    if(span){
      /* look for the hyphen */ 
      char *span2=strchr(span,'-');
      if(strrchr(span,'-')!=span2){
	report("Error parsing span argument");
	cdda_close(d);
	d=NULL;
	exit(1);
      }
      
      if(span2!=NULL){
	*span2='\0';
	span2++;
      }
      
      first_sector=parse_offset(d,span,-1);
      if(first_sector==-1)
	last_sector=parse_offset(d,span2,cdda_disc_firstsector(d));
      else
	last_sector=parse_offset(d,span2,first_sector);
      
      if(first_sector==-1){
	if(last_sector==-1){
	  report("Error parsing span argument");
	  cdda_close(d);
	  d=NULL;
	  exit(1);
	}else{
	  first_sector=cdda_disc_firstsector(d);
	}
      }else{
	if(last_sector==-1){
	  if(span2){ /* There was a hyphen */
	    last_sector=cdda_disc_lastsector(d);
	  }else{
	    last_sector=
	      cdda_track_lastsector(d,cdda_sector_gettrack(d,first_sector));
	  }
	}
      }
    }else{
      first_sector=cdda_disc_firstsector(d);
      last_sector=cdda_disc_lastsector(d);
    }

    {
      int track1=cdda_sector_gettrack(d,first_sector);
      int track2=cdda_sector_gettrack(d,last_sector);
      long off1=first_sector-cdda_track_firstsector(d,track1);
      long off2=last_sector-cdda_track_firstsector(d,track2);
      int i;

      for(i=track1;i<=track2;i++)
	if(!cdda_track_audiop(d,i)){
	  report("Selected span contains non audio tracks.  Aborting.\n\n");
	  exit(1);
	}

      report("Ripping from sector %7ld (track %2d [%d:%02d.%02d])\n"
	     "\t  to sector %7ld (track %2d [%d:%02d.%02d])\n",first_sector,
	     track1,(int)(off1/(60*75)),(int)((off1/75)%60),(int)(off1%75),
	     last_sector,
	     track2,(int)(off2/(60*75)),(int)((off2/75)%60),(int)(off2%75));
      
    }

    {
      long cursor;
      int16_t offset_buffer[1176];
      int offset_buffer_used=0;
      int offset_skip=sample_offset*4;

      p=paranoia_init(d);
      paranoia_modeset(p,paranoia_mode);
      if(force_cdrom_overlap!=-1)paranoia_overlapset(p,force_cdrom_overlap);

      if(verbose)
        cdda_verbose_set(d,CDDA_MESSAGE_LOGIT,CDDA_MESSAGE_LOGIT);
      else
        cdda_verbose_set(d,CDDA_MESSAGE_FORGETIT,CDDA_MESSAGE_FORGETIT);

      paranoia_seek(p,cursor=first_sector,SEEK_SET);      

      /* this is probably a good idea in general */
      seteuid(getuid());
      setegid(getgid());

      /* we'll need to be able to read one sector past user data if we
	 have a sample offset in order to pick up the last bytes.  We
	 need to set the disc length forward here so that the libs are
	 willing to read past, assuming that works on the hardware, of
	 course */
      if(sample_offset)
	d->disc_toc[d->tracks].dwStartSector++;

      while(cursor<=last_sector){
	char outfile_name[256];
	if(batch){
	  batch_first=cursor;
	  batch_last=
	    cdda_track_lastsector(d,batch_track=
				  cdda_sector_gettrack(d,cursor));
	  if(batch_last>last_sector)batch_last=last_sector;
	}else{
	  batch_first=first_sector;
	  batch_last=last_sector;
	  batch_track=-1;
	}
	
	callbegin=batch_first;
	callend=batch_last;
	
	/* argv[optind] is the span, argv[optind+1] (if exists) is outfile */
	
	if(optind+1<argc){
	  if(!strcmp(argv[optind+1],"-")){
	    out=dup(fileno(stdout));
	    if(batch)report("Are you sure you wanted 'batch' "
			    "(-B) output with stdout?");
	    report("outputting to stdout\n");
	    if(logfile){
	      fprintf(logfile,"outputting to stdout\n");
	      fflush(logfile);
	    }
	    outfile_name[0]='\0';
	  }else{
	    char path[256];

	    char *post=strrchr(argv[optind+1],'/');
	    int pos=(post?post-argv[optind+1]+1:0);
	    char *file=argv[optind+1]+pos;
	    
	    path[0]='\0';

	    if(pos)
	      strncat(path,argv[optind+1],pos>256?256:pos);

	    if(batch)
	      snprintf(outfile_name,246,"%strack%02d.%s",path,batch_track,file);
	    else
	      snprintf(outfile_name,246,"%s%s",path,file);

	    if(file[0]=='\0'){
	      switch(output_type){
	      case 0: /* raw */
		strcat(outfile_name,"cdda.raw");
		break;
	      case 1:
		strcat(outfile_name,"cdda.wav");
		break;
	      case 2:
		strcat(outfile_name,"cdda.aifc");
		break;
	      case 3:
		strcat(outfile_name,"cdda.aiff");
		break;
	      }
	    }
	    
	    out=open(outfile_name,O_RDWR|O_CREAT|O_TRUNC,0666);
	    if(out==-1){
	      report("Cannot open specified output file %s: %s",outfile_name,
		      strerror(errno));
	      cdda_close(d);
	      d=NULL;
	      exit(1);
	    }
	    report("outputting to %s\n",outfile_name);
	    if(logfile){
	      fprintf(logfile,"outputting to %s\n",outfile_name);
	      fflush(logfile);
	    }
	  }
	}else{
	  /* default */
	  if(batch)
	    sprintf(outfile_name,"track%02d.",batch_track);
	  else
	    outfile_name[0]='\0';
	  
	  switch(output_type){
	  case 0: /* raw */
	    strcat(outfile_name,"cdda.raw");
	    break;
	  case 1:
	    strcat(outfile_name,"cdda.wav");
	    break;
	  case 2:
	    strcat(outfile_name,"cdda.aifc");
	    break;
	  case 3:
	    strcat(outfile_name,"cdda.aiff");
	    break;
	  }
	  
	  out=open(outfile_name,O_RDWR|O_CREAT|O_TRUNC,0666);
	  if(out==-1){
	    report("Cannot open default output file %s: %s",outfile_name,
		    strerror(errno));
	    cdda_close(d);
	    d=NULL;
	    exit(1);
	  }
	  report("outputting to %s\n",outfile_name);
	  if(logfile){
	    fprintf(logfile,"outputting to %s\n",outfile_name);
	    fflush(logfile);
	  }
	}
	
	switch(output_type){
	case 0: /* raw */
	  break;
	case 1: /* wav */
	  WriteWav(out,(batch_last-batch_first+1)*CD_FRAMESIZE_RAW);
	  break;
	case 2: /* aifc */
	  WriteAifc(out,(batch_last-batch_first+1)*CD_FRAMESIZE_RAW);
	  break;
	case 3: /* aiff */
	  WriteAiff(out,(batch_last-batch_first+1)*CD_FRAMESIZE_RAW);
	  break;
	}
	
	/* Off we go! */

	if(offset_buffer_used){
	  /* partial sector from previous batch read */
	  cursor++;
	  if(buffering_write(out,
			     ((char *)offset_buffer)+offset_buffer_used,
			     CD_FRAMESIZE_RAW-offset_buffer_used)){
	    report("Error writing output: %s",strerror(errno));
	    exit(1);
	  }
	}
	
	skipped_flag=0;
	while(cursor<=batch_last){
	  /* read a sector */
	  int16_t *readbuf=paranoia_read_limited(p,callback,max_retries);
	  char *err=cdda_errors(d);
	  char *mes=cdda_messages(d);

	  if(mes || err)
	    fprintf(stderr,"\r                               "
		    "                                           \r%s%s\n",
		    mes?mes:"",err?err:"");
	  
	  if(err)free(err);
	  if(mes)free(mes);
	  if(readbuf==NULL){
	    if(errno==EBADF || errno==ENOMEDIUM){
	      report("\nparanoia_read: CDROM drive unavailable, bailing.\n");
	      exit(1);
	    }
	    skipped_flag=1;
	    report("\nparanoia_read: Unrecoverable error, bailing.\n");
	    break;
	  }
	  if(skipped_flag && abort_on_skip){
	    cursor=batch_last+1;
	    break;
	  }

	  skipped_flag=0;
	  cursor++;
	  
	  if(output_endian!=bigendianp()){
	    int i;
	    for(i=0;i<CD_FRAMESIZE_RAW/2;i++)readbuf[i]=swap16(readbuf[i]);
	  }
	  
	  callback(cursor*(CD_FRAMEWORDS)-1,-2);

	  if(buffering_write(out,((char *)readbuf)+offset_skip,
			     CD_FRAMESIZE_RAW-offset_skip)){
	    report("Error writing output: %s",strerror(errno));
	    exit(1);
	  }
	  offset_skip=0;
	  
	  if(output_endian!=bigendianp()){
	    int i;
	    for(i=0;i<CD_FRAMESIZE_RAW/2;i++)readbuf[i]=swap16(readbuf[i]);
	  }

	  /* One last bit of silliness to deal with sample offsets */
	  if(sample_offset && cursor>batch_last){
	    int i;
	    /* read a sector and output the partial offset.  Save the
               rest for the next batch iteration */
	    readbuf=paranoia_read_limited(p,callback,max_retries);
	    err=cdda_errors(d);mes=cdda_messages(d);

	    if(mes || err)
	      fprintf(stderr,"\r                               "
		      "                                           \r%s%s\n",
		      mes?mes:"",err?err:"");
	  
	    if(err)free(err);if(mes)free(mes);
	    if(readbuf==NULL){
	      skipped_flag=1;
	      report("\nparanoia_read: Unrecoverable error reading through "
		     "sample_offset shift\n\tat end of track, bailing.\n");
	      break;
	    }
	    if(skipped_flag && abort_on_skip)break;
	    skipped_flag=0;
	    /* do not move the cursor */
	  
	    if(output_endian!=bigendianp())
	      for(i=0;i<CD_FRAMESIZE_RAW/2;i++)
		offset_buffer[i]=swap16(readbuf[i]);
	    else
	      memcpy(offset_buffer,readbuf,CD_FRAMESIZE_RAW);
	    offset_buffer_used=sample_offset*4;
	  
	    callback(cursor*(CD_FRAMEWORDS),-2);

	    if(buffering_write(out,(char *)offset_buffer,
			       offset_buffer_used)){
	      report("Error writing output: %s",strerror(errno));
	      exit(1);
	    }
	  }
	}
	callback(cursor*(CD_FRAMESIZE_RAW/2)-1,-1);
	buffering_close(out);
	if(skipped_flag){
	  /* remove the file */
	  report("\nRemoving aborted file: %s",outfile_name);
	  unlink(outfile_name);
	  /* make the cursor correct if we have another track */
	  if(batch_track!=-1){
	    batch_track++;
	    cursor=cdda_track_firstsector(d,batch_track);
	    paranoia_seek(p,cursor,SEEK_SET);      
	    offset_skip=sample_offset*4;
	    offset_buffer_used=0;
	  }
	}
	report("\n");
      }

      paranoia_free(p);
      p=NULL;
    }
  }

  report("Done.\n\n");
  
  cdda_close(d);
  d=NULL;
  if(logfile_open)
    fclose(logfile);
  if(reportfile_open)
    fclose(reportfile);
  return 0;
}
