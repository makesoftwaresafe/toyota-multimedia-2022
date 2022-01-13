/*
 * This file was originally distributed as part of
 * shadow-utils-4.0.17-12.fc6.src.rpm and has been modified
 * in WRLinux for inclusion in rpm.
 */

/* $Id: chkname.h,v 1.1 2008/08/17 18:31:35 jbj Exp $ */
#ifndef _CHKNAME_H_
#define _CHKNAME_H_

/*
 * check_user_name(), check_group_name() - check the new user/group
 * name for validity; return value: 1 - OK, 0 - bad name
 */

#include "defines.h"

extern int check_user_name (const char *);
extern int check_group_name (const char *name);

#endif
