/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems. All Rights Reserved.
 *
 * This software is QNX Confidential Information subject to
 * confidentiality restrictions. DISCLOSURE OF THIS SOFTWARE
 * IS PROHIBITED UNLESS AUTHORIZED BY QNX SOFTWARE SYSTEMS IN
 * WRITING.
 *
 * You must obtain a written license from and pay applicable license
 * fees to QNX Software Systems before you may reproduce, modify or
 * distribute this software, or any work that includes all or part
 * of this software. For more information visit
 * http://licensing.qnx.com or email licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

/*
 * ipu4_priv.h
 *
 * IPU4 driver IPC private parts
 * Shared between service and API lib
 */

#ifndef _IPU4_PRIV_H_INCLUDED
#define _IPU4_PRIV_H_INCLUDED

#include <devctl.h>

#include "intel/ipu4/ipu4-interface.h"

/* Use case: 800x480 x RGB32 / 4096 + alpha => 400
 * This should be customized for optimum memory usage.
 */
#define MAX_CHUNKS_NUMBER_IN_BUFFER 400

typedef struct {
    ipu4_csi2_config_t      config[IPU4_CSI2_NUMPORTS];
} ipu4_ipc_initialize_t;

typedef union {
    struct {
        ipu4_csi2_port_t    port;
        ipu4_csi2_vc_t      vc;
    } i;
    struct {
        ipu4_handle_t       handle;
    } o;
} ipu4_ipc_open_stream_t;

typedef struct {
    ipu4_handle_t           handle;
} ipu4_ipc_handle_stream_t;

typedef struct {
    ipu4_handle_t           handle;
    ipu4_stream_config_t    config;
} ipu4_ipc_configure_stream_t;

typedef union {
    struct {
        ipu4_handle_t       handle;
        uint64_t            timeout;
    } i;
    struct {
        off_t               p_addr;
        struct timespec     timestamp;
        uint32_t            field;
        uint32_t            seqNum;
    } o;
} ipu4_ipc_get_buffer_t;

typedef struct {
    ipu4_handle_t           handle;
    uint32_t                chunks_num;
    off_t                   chunk_p_addr[MAX_CHUNKS_NUMBER_IN_BUFFER];
    uint32_t                chunk_size[MAX_CHUNKS_NUMBER_IN_BUFFER];
} ipu4_ipc_queue_buffer_t;

typedef struct {
    uint32_t                num_streams;
} ipu4_ipc_set_max_streams_t;

typedef struct {
    uint32_t                reserved;
} ipu4_ipc_reset_t;

#define IPU4_CMD_CODE      1

/* IPU4 library initialization */
#define DCMD_IPU4_INITIALIZE __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 1, ipu4_ipc_initialize_t)

/* Open a stream on a given CSI2 port */
#define DCMD_IPU4_OPEN_STREAM __DIOTF(_DCMD_MISC, IPU4_CMD_CODE + 2, ipu4_ipc_open_stream_t)

/* Close a stream */
#define DCMD_IPU4_CLOSE_STREAM __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 3, ipu4_ipc_handle_stream_t)

/* Configure a stream */
#define DCMD_IPU4_CONFIGURE_STREAM __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 4, ipu4_ipc_configure_stream_t)

/* Start a stream */
#define DCMD_IPU4_START_STREAM __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 5, ipu4_ipc_handle_stream_t)

/* Stop a stream */
#define DCMD_IPU4_STOP_STREAM __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 6, ipu4_ipc_handle_stream_t)

/* Get the next filled buffer from the active stream */
#define DCMD_IPU4_GET_BUFFER __DIOTF(_DCMD_MISC, IPU4_CMD_CODE + 7, ipu4_ipc_get_buffer_t)

/* Queue an empty buffer to be filled with an acquired frame */
#define DCMD_IPU4_QUEUE_BUFFER __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 8, ipu4_ipc_queue_buffer_t)

/* Flush a stream */
#define DCMD_IPU4_FLUSH_STREAM __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 9, ipu4_ipc_handle_stream_t)

/* Set maximum number of streams to support */
#define DCMD_IPU4_SET_MAX_STREAMS __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 10, ipu4_ipc_set_max_streams_t)

/* Perform a full re-initialization of the IPU4 driver; all streams will be closed in the process. */
#define DCMD_IPU4_RESET __DIOT(_DCMD_MISC, IPU4_CMD_CODE + 11, ipu4_ipc_reset_t)

#define DCMD_IPU4_MAX_MSG_SIZE max(sizeof(ipu4_ipc_initialize_t),       \
                               max(sizeof(ipu4_ipc_open_stream_t),      \
                               max(sizeof(ipu4_ipc_handle_stream_t),    \
                               max(sizeof(ipu4_ipc_configure_stream_t), \
                               max(sizeof(ipu4_ipc_get_buffer_t),       \
                                   sizeof(ipu4_ipc_queue_buffer_t))))))


#endif   /* _IPU4_PRIV_H_INCLUDED */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/intel-ipu4/private/intel/ipu4/ipu4_priv.h $ $Rev: 871034 $")
#endif
