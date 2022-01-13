#ifndef WIN32_IO_H
#define WIN32_IO_H

extern	void	InitSockets(void);
extern	void	connection_reset_fix(SOCKET fd, sockaddr_u *addr);

#endif /* WIN32_IO_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/ports/winnt/include/win32_io.h $ $Rev: 777476 $")
#endif
