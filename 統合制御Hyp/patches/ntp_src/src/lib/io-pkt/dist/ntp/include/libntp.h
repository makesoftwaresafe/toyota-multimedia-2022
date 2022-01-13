/* libntp.h */

#if defined(HAVE_SYSCONF) && defined(_SC_OPEN_MAX)
#define GETDTABLESIZE()	((int)sysconf(_SC_OPEN_MAX))
#elif defined(HAVE_GETDTABLESIZE)
#define GETDTABLESIZE	getdtablesize
#else
/*
 * if we have no idea about the max fd value set up things
 * so we will start at FOPEN_MAX
 */
#define GETDTABLESIZE()	(FOPEN_MAX + FD_CHUNK)
#endif

extern void	make_socket_nonblocking( SOCKET fd );
extern SOCKET	move_fd( SOCKET fd );

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/include/libntp.h $ $Rev: 777476 $")
#endif
