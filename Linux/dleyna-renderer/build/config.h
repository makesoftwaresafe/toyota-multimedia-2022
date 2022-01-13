/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Compiling with debugging information enabled */
/* #undef DLEYNA_DEBUG_ENABLED */

/* d-Bus Name of dleyna-renderer push host interface */
#define DLEYNA_INTERFACE_PUSH_HOST "com.intel.dLeynaRenderer.PushHost"

/* Log level flag for debug messages */
#define DLEYNA_LOG_LEVEL 19

/* Log level flag for all messages */
#define DLEYNA_LOG_LEVEL_ALL 0x3F

/* Log level flag for critical messages */
#define DLEYNA_LOG_LEVEL_CRITICAL 0x02

/* Log level flag for debug messages */
#define DLEYNA_LOG_LEVEL_DEBUG 0x20

/* Log level flag to display default level messages */
#define DLEYNA_LOG_LEVEL_DEFAULT 0x13

/* Log level flag for disabled messages */
#define DLEYNA_LOG_LEVEL_DISABLED 0x00

/* Log level flag for errors */
#define DLEYNA_LOG_LEVEL_ERROR 0x01

/* Log level flag for informational messages */
#define DLEYNA_LOG_LEVEL_INFO 0x10

/* Log level flag for messages */
#define DLEYNA_LOG_LEVEL_MESSAGE 0x08

/* Log level flag for warnings */
#define DLEYNA_LOG_LEVEL_WARNING 0x04

/* d-Bus Name of dleyna-renderer main interface */
#define DLEYNA_SERVER_INTERFACE_MANAGER "com.intel.dLeynaRenderer.Manager"

/* d-Bus Name of dleyna-renderer device interface */
#define DLEYNA_SERVER_INTERFACE_RENDERER_DEVICE "com.intel.dLeynaRenderer.RendererDevice"

/* d-Bus Name of dleyna-renderer */
#define DLEYNA_SERVER_NAME "com.intel.dleyna-renderer"

/* Name of object exposed by dleyna-renderer */
#define DLEYNA_SERVER_OBJECT "/com/intel/dLeynaRenderer"

/* Path of server objects */
#define DLEYNA_SERVER_PATH "/com/intel/dLeynaRenderer/server"

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if your system has a GNU libc compatible `realloc' function,
   and to 0 otherwise. */
#define HAVE_REALLOC 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "dleyna-renderer"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://github.com/01org/dleyna-renderer/issues/new"

/* Define to the full name of this package. */
#define PACKAGE_NAME "dleyna-renderer"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "dleyna-renderer 0.6.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "dleyna-renderer"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://01.org/dleyna/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.6.0"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* User Agent prefix */
/* #undef UA_PREFIX */

/* Version number of package */
#define VERSION "0.6.0"

/* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT8_T */

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to the type of an unsigned integer type of width exactly 8 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint8_t */
