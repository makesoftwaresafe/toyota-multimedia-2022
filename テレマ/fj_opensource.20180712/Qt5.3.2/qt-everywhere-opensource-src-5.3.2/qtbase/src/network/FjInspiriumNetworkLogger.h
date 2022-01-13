#ifndef FJINSPIRIUMNETWORKLOGGER_H
#define FJINSPIRIUMNETWORKLOGGER_H

//#include <qdebug.h>
//#include <qstring.h>
//#include <ctype.h>
#include "FjInspiriumLogger.h"

//#define FJ_INSP_NETWORKLOG

#define DEBUG_SOCKET_QDEBUG(...)	if(getenv("INSPIRIUM_DEBUG_SOCKET") || getenv("INSPIRIUM_DEBUG_SOCKET2") || getenv("INSPIRIUM_DEBUG_SOCKET3")){	\
						qInspiriumNetworkLogger(__VA_ARGS__);		\
					}
#define DEBUG_SOCKET_QDEBUG2(...)	if(getenv("INSPIRIUM_DEBUG_SOCKET2") || getenv("INSPIRIUM_DEBUG_SOCKET3")){	\
						qInspiriumNetworkLogger(__VA_ARGS__);		\
					}
#define DEBUG_SOCKET_QDEBUG3(...)	if(getenv("INSPIRIUM_DEBUG_SOCKET3")){	\
						qInspiriumNetworkLogger(__VA_ARGS__);		\
					}

#define DEBUG_SOCKET_PERROR(a)		if(getenv("INSPIRIUM_DEBUG_SOCKET")){	\
						perror(a);			\
					}

#endif // FjInspiriumLogger.h
