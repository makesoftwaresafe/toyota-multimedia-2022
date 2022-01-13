/**
* Support for Intel Camera Imaging ISP subsystem.
* Copyright (c) 2010 - 2016, Intel Corporation.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public License,
* version 2, as published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*/

#ifndef __RECV_PORT_H
#define __RECV_PORT_H


struct recv_port;
struct sys_queue;
struct port_env;

void
recv_port_open(struct recv_port *p, const struct sys_queue *q,
	       const struct port_env *env);

unsigned int
recv_port_available(const struct recv_port *p);

unsigned int
recv_port_transfer(const struct recv_port *p, void *data);


#endif /* __RECV_PORT_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600/port/interface/recv_port.h $ $Rev: 838597 $")
#endif
