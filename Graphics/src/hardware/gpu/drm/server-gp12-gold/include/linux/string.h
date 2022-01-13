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

char* kstrdup(const char *s, gfp_t gfp);
extern char* skip_spaces(const char *);
extern char *strndup_user(const char*, long);
extern void *memdup_user(const void*, size_t);
extern void *memdup_user_nul(const void __user *, size_t);
void *memchr_inv(const void *s, int c, size_t n);
char *strreplace(char *s, char old, char new);
int match_string(const char * const *array, size_t n, const char *string);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/string.h $ $Rev: 836322 $")
#endif
