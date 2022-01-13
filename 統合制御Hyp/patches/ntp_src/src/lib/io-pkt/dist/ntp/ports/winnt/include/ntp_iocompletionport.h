#ifndef NTP_IOCPMPLETIONPORT_H
#define NTP_IOCPMPLETIONPORT_H

#include "ntp_fp.h"
#include "ntp.h"
#include "clockstuff.h"
#include "ntp_worker.h"

#if defined(HAVE_IO_COMPLETION_PORT)

struct refclockio;	/* in ntp_refclock.h but inclusion here triggers problems */
struct interface;	/* likewise */


extern	void	init_io_completion_port(void);
extern	void	uninit_io_completion_port(void);

extern	BOOL	io_completion_port_add_interface(struct interface*);
extern	void	io_completion_port_remove_interface(struct interface*);

extern	BOOL	io_completion_port_add_socket(SOCKET fd, struct interface *, BOOL bcast);
extern	void	io_completion_port_remove_socket(SOCKET fd, struct interface*);

extern	int	io_completion_port_sendto(struct interface*, SOCKET, void *, size_t, sockaddr_u *);

extern	BOOL	io_completion_port_add_clock_io(struct refclockio *rio);
extern	void	io_completion_port_remove_clock_io(struct refclockio *rio);

extern	int	GetReceivedBuffers(void);

extern	HANDLE	WaitableExitEventHandle;

#endif /*!defined(HAVE_IO_COMPLETION_PORT)*/
#endif /*!defined(NTP_IOCPMPLETIONPORT_H)*/

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/ports/winnt/include/ntp_iocompletionport.h $ $Rev: 802718 $")
#endif
