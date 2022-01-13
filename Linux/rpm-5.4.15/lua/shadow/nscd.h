/*
 * This file was originally distributed as part of
 * shadow-utils-4.0.17-12.fc6.src.rpm and has been modified
 * in WRLinux for inclusion in rpm.
 */

/* Copyright (c) 1999 SuSE GmbH Nuerenberg, Germany
   Author: Thorsten Kukuk <kukuk@suse.de> */

#ifndef _FAILURE_H_
#define _FAILURE_H_

/*
 * nscd_flush_cache - flush specyfied service bufor in nscd cache
 */
extern int nscd_flush_cache (char *service);

#endif
