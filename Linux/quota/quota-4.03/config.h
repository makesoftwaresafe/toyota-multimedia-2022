/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* File with mounted filesystems */
#define ALT_MTAB "/proc/mounts"

/* Check rights to query / set quotas before calling quotactl */
#define BSD_BEHAVIOUR 1

/* Configuration options */
#define COMPILE_OPTS " EXT2_DIRECT HOSTS_ACCESS RPC BSD_BEHAVIOUR"

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Scanning of ext? filesystems using e2fslib */
#define EXT2_DIRECT 1

/* Define to 1 if you have the Mac OS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the Mac OS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
/* #undef HAVE_DCGETTEXT */

/* Define if the GNU gettext() function is already present or preinstalled. */
/* #undef HAVE_GETTEXT */

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `ldap' library (-lldap). */
/* #undef HAVE_LIBLDAP */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Use hosts.allow and hosts.deny for access checking of rpc.rquotad */
#define HOSTS_ACCESS 1

/* Name of package */
#define PACKAGE "quota"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "jack@suse.cz"

/* Locale-specific data directory */
#define PACKAGE_LOCALE_DIR "/usr/share/locale"

/* Define to the full name of this package. */
#define PACKAGE_NAME "quota"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "quota 4.03"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "quota"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "4.03"

/* Support for RPC */
#define RPC 1

/* Allow setting of quotas over RPC */
/* #undef RPC_SETQUOTA */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Use gettext to translate messages */
/* #undef USE_GETTEXT */

/* Lookup email address using LDAP */
/* #undef USE_LDAP_MAIL_LOOKUP */

/* Version number of package */
#define VERSION "4.03"

/* Assume quota mount options for root filesystem */
/* #undef XFS_ROOTHACK */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif
