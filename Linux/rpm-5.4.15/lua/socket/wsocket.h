#ifndef WSOCKET_H
#define WSOCKET_H
/*=========================================================================*\
* Socket compatibilization module for Win32
* LuaSocket toolkit
*
* RCS ID: $Id: wsocket.h,v 1.1 2008/10/24 22:58:40 jbj Exp $
\*=========================================================================*/

/*=========================================================================*\
* WinSock include files
\*=========================================================================*/
#include <winsock.h>

typedef int socklen_t;
typedef SOCKET t_socket;
typedef t_socket *p_socket;

#define SOCKET_INVALID (INVALID_SOCKET)

#endif /* WSOCKET_H */
