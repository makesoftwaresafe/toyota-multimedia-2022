/*
* Copyright (c) 2017 QNX Software Systems.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _QNX_LINUX_STRING_H_
#define _QNX_LINUX_STRING_H_

#include <linux/types.h>
#ifdef __QNXNTO__
#include <stdlib.h>
#include <string.h>
#endif /* __QNXNTO__ */

long simple_strtol(const char *cp, char **endp, unsigned int base);

/**
 * sysfs_streq - return true if strings are equal, modulo trailing newline
 * @s1: one string
 * @s2: another string
 *
 * This routine returns true iff two strings are equal, treating both
 * NUL and newline-then-NUL as equivalent string terminations.  It's
 * geared for use with sysfs input strings, which generally terminate
 * with newlines but are compared against values without newlines.
 */
static inline bool
sysfs_streq(const char *s1, const char *s2)
{
	while (*s1 && *s1 == *s2) {
		s1++;
		s2++;
	}

	if (*s1 == *s2)
		return true;
	if (!*s1 && *s2 == '\n' && !s2[1])
		return true;
	if (*s1 == '\n' && !s1[1] && !*s2)
		return true;
	return false;
}

/**
 * strchrnul - Find and return a character in a string, or end of string
 * @s: The string to be searched
 * @c: The character to search for
 *
 * Returns pointer to first occurrence of 'c' in s. If c is not found, then
 * return a pointer to the null byte at the end of s.
 */
static inline
char *strchrnul(const char *s, int c)
{
	while (*s && *s != (char)c)
		s++;
	return (char *)s;
}


static inline char *
kstrdup(const char *s, gfp_t gfp){
	size_t len;
	char *buf;
	(void)gfp;
	if (!s)
		return NULL;

	len = strlen(s) + 1;
	buf = malloc(len);
	if (buf)
		memcpy(buf, s, len);
	return buf;
}

extern char* skip_spaces(const char *);
extern char *strndup_user(const char*, long);
extern void *memdup_user(const void*, size_t);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/string.h $ $Rev: 838597 $")
#endif
