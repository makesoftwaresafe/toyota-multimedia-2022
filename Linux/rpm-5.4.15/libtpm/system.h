/**
 * \file system.h
 */

#ifndef	H_SYSTEM
#define	H_SYSTEM

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <stdio.h>

#if defined(STDC_HEADERS)
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
# include <string.h>
#else
# include <strings.h>
char *memchr ();
#endif

#ifdef TPM_POSIX
#include <netinet/in.h>
#endif

#ifdef TPM_WINDOWS
#include <winsock2.h>
#endif

#define MIN(x,y) (x) < (y) ? (x) : (y)

#define N_(_s)  _s

#endif	/* H_SYSTEM */
