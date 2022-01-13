/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef UAPI_LINUX_CRLMODULE_H
#define UAPI_LINUX_CRLMODULE_H

#define V4L2_CID_CRLMODULE_BASE		(V4L2_CID_USER_BASE + 0x2050)

#define V4L2_CID_FRAME_LENGTH_LINES (V4L2_CID_CRLMODULE_BASE + 1)
#define V4L2_CID_LINE_LENGTH_PIXELS (V4L2_CID_CRLMODULE_BASE + 2)
#define CRL_CID_SENSOR_THERMAL_DATA (V4L2_CID_CRLMODULE_BASE + 3)

/*
 * Select sensor mode directly, driver programs media pad
 * formats as in configuration file
 */
#define CRL_CID_SENSOR_MODE (V4L2_CID_CRLMODULE_BASE + 4)

/* IMX230 HDR specific controls */
#define CRL_CID_IMX230_HDR_MODE		(V4L2_CID_CRLMODULE_BASE + 5)
#define CRL_CID_IMX230_HDR_ET_RATIO	(V4L2_CID_CRLMODULE_BASE + 6)
#define CRL_CID_IMX230_HDR_ZIGZAG	(V4L2_CID_CRLMODULE_BASE + 7)

/* Switch sensor WDR mode on-the-fly */
#define CRL_CID_SENSOR_WDR_SWITCH	(V4L2_CID_CRLMODULE_BASE + 8)

#endif /* UAPI_LINUX_CRLMODULE_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/uapi/linux/crlmodule.h $ $Rev: 836043 $")
#endif

