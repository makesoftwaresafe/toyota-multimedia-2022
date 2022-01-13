/* Define to 1 to use CyaSSL as a replacement for OpenSSL. 
 * LWS_OPENSSL_SUPPORT needs to be set also for this to work. */
/* #undef USE_CYASSL */

/* The Libwebsocket version */
#define LWS_LIBRARY_VERSION "1.4"

/* The current git commit hash that we're building from */
/* #undef LWS_BUILD_HASH */

/* Build with OpenSSL support */
/* IMPORTANT !!!
 * When enabling OPENSSL_SUPPORT and using OPENSSL library in
 * multi-threaded environment make sure that OPENSSL MT requirements
 * are met, i.e. proper thread id and thread locking callbacks are
 * provided.
 * #undef LWS_OPENSSL_SUPPORT
 * we do not permanently disable openssl here because we want to
 * use it in tests
 * we rely on cmake to set or not set LWS_OPENSSL_SUPPORT
 */

/* The client should load and trust CA root certs it finds in the OS */
#define LWS_SSL_CLIENT_USE_OS_CA_CERTS

/* Sets the path where the client certs should be installed. */
#define LWS_OPENSSL_CLIENT_CERTS "/etc/pki/tls/certs/"

/* Turn off websocket extensions */
/* #undef LWS_NO_EXTENSIONS */

/* Enable libev io loop */
/* #undef LWS_USE_LIBEV */

/* Build with support for ipv6 */
/* #undef LWS_USE_IPV6 */

/* Build with support for HTTP2 */
/* #undef LWS_USE_HTTP2 */

/* Turn on latency measuring code */
/* #undef LWS_LATENCY */

/* Don't build the daemonizeation api */
#define LWS_NO_DAEMONIZE

/* Build without server support */
/* #undef LWS_NO_SERVER */

/* Build without client support */
/* #undef LWS_NO_CLIENT */

/* If we should compile with MinGW support */
/* #undef LWS_MINGW_SUPPORT */

/* Use the BSD getifaddrs that comes with libwebsocket, for uclibc support */
#define LWS_BUILTIN_GETIFADDRS 

/* Define to 1 if you have the `bzero' function. */
#undef HAVE_BZERO

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK

/* Define to 1 if you have the `getenv’ function. */
#define HAVE_GETENV

/* Define to 1 if you have the <in6addr.h> header file. */
/* #undef HAVE_IN6ADDR_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H

/* Define to 1 if you have the `ssl' library (-lssl). */
#undef HAVE_LIBSSL 

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H

/* Define to 1 if your system has a GNU libc compatible `realloc' function,
   and to 0 otherwise. */
#define HAVE_REALLOC

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR

/* Define to 1 if you have the <strings.h> header file. */
#ifndef _WIN32
#define HAVE_STRINGS_H
#endif

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H

/* Define to 1 if you have the <sys/prctl.h> header file. */
#if (defined(__APPLE__) || defined(__QNX__))
#undef HAVE_SYS_PRCTL_H
#else
#define HAVE_SYS_PRCTL_H
#endif

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H

/* Define to 1 if you have the <unistd.h> header file. */
#if !defined(_WIN32) && !defined(_WIN32_WCE)
#define HAVE_UNISTD_H
#endif

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK

/* Define to 1 if you have the <zlib.h> header file. */
#define HAVE_ZLIB_H

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#undef LT_OBJDIR // We're not using libtool

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS

/* Version number of package */
#define VERSION

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to 1 if we have getifaddrs */
//#undef HAVE_GETIFADDRS

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */

/* Define if the inline keyword doesn't exist. */
#ifdef WIN32
#define inline __inline
#endif

