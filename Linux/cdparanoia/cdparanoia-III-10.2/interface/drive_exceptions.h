extern int scsi_enable_cdda(cdrom_drive *,int);
extern long scsi_read_mmc(cdrom_drive *,void *,long,long);
extern long scsi_read_mmc2(cdrom_drive *,void *,long,long);
extern long scsi_read_D4_10(cdrom_drive *,void *,long,long);
extern long scsi_read_D4_12(cdrom_drive *,void *,long,long);
extern long scsi_read_D8(cdrom_drive *,void *,long,long);
extern long scsi_read_28(cdrom_drive *,void *,long,long);
extern long scsi_read_A8(cdrom_drive *,void *,long,long);

typedef struct exception {
  char *model;
  int atapi; /* If the ioctl doesn't work */
  unsigned char density;
  int  (*enable)(struct cdrom_drive *,int);
  long (*read)(struct cdrom_drive *,void *,long,long);
  int  bigendianp;
} exception;

/* specific to general */

/* list of drives that affect autosensing in ATAPI specific portions of code 
   (force drives to detect as ATAPI or SCSI, force ATAPI read command */

static exception atapi_list[]={
  {"SAMSUNG SCR-830 REV 2.09 2.09 ", 1,   0,         Dummy,scsi_read_mmc2,0},
  {"Memorex CR-622",                 1,   0,         Dummy,          NULL,0},
  {"SONY CD-ROM CDU-561",            0,   0,         Dummy,          NULL,0},
  {"Chinon CD-ROM CDS-525",          0,   0,         Dummy,          NULL,0},
  {NULL,0,0,NULL,NULL,0}};

/* list of drives that affect MMC default settings */

static exception mmc_list[]={
  {"SAMSUNG SCR-830 REV 2.09 2.09 ", 1,   0,         Dummy,scsi_read_mmc2,0},
  {"Memorex CR-622",                 1,   0,         Dummy,          NULL,0},
  {"SONY CD-ROM CDU-561",            0,   0,         Dummy,          NULL,0},
  {"Chinon CD-ROM CDS-525",          0,   0,         Dummy,          NULL,0},
  {"KENWOOD CD-ROM UCR",          -1,   0,            NULL,scsi_read_D8,  0},
  {NULL,0,0,NULL,NULL,0}};

/* list of drives that affect SCSI default settings */

static exception scsi_list[]={
  {"TOSHIBA",                     -1,0x82,scsi_enable_cdda,scsi_read_28,  0},
  {"IBM",                         -1,0x82,scsi_enable_cdda,scsi_read_28,  0},
  {"DEC",                         -1,0x82,scsi_enable_cdda,scsi_read_28,  0},
  
  {"IMS",                         -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"KODAK",                       -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"RICOH",                       -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"HP",                          -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"PHILIPS",                     -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"PLASMON",                     -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"GRUNDIG CDR100IPW",           -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"MITSUMI CD-R ",               -1,   0,scsi_enable_cdda,scsi_read_28,  1},
  {"KENWOOD CD-ROM UCR",          -1,   0,            NULL,scsi_read_D8,  0},

  {"YAMAHA",                      -1,   0,scsi_enable_cdda,        NULL,  0},

  {"PLEXTOR",                     -1,   0,            NULL,        NULL,  0},
  {"SONY",                        -1,   0,            NULL,        NULL,  0},

  {"NEC",                         -1,   0,           NULL,scsi_read_D4_10,0},

  /* the 7501 locks up if hit with the 10 byte version from the
     autoprobe first */
  {"MATSHITA CD-R   CW-7501",     -1,   0,           NULL,scsi_read_D4_12,-1},

  {NULL,0,0,NULL,NULL,0}};

