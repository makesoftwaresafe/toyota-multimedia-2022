/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software. Free development licenses are
 * available for evaluation and non-commercial purposes. For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others. Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include <inttypes.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <pthread.h>

#include "intel/ipu4/ipu4_priv.h"
#include "debug.h"

#define PAGE_SIZE           4096

#define IPU4_DEV_PATH       "/dev/ipu4"

/**
 * @brief Our internal tracking of buffers
 */
typedef struct buffer_node {
    struct buffer_node*                 next;
    void*                               v_addr;
    off_t                               chunk_p_addr[MAX_CHUNKS_NUMBER_IN_BUFFER];
    size_t                              chunk_size[MAX_CHUNKS_NUMBER_IN_BUFFER];
    uint32_t                            chunks_num;
} buffer_node_t;

/**
 * @brief Our internal tracking of opened streams
 */
typedef struct stream_node {
    struct stream_node*                 next;
    ipu4_handle_t                       handle;
    uint32_t                            buffer_size;
    pthread_mutex_t                     stream_mutex;
    // linked-list of buffers that belong to user that we have seen before
    buffer_node_t*                      free_buffers;
    // linked-list of buffers that were queued to driver
    buffer_node_t*                      busy_buffers;
} stream_node_t;

/**
 * @brief State information of IPU4 driver API library
 */
typedef struct {
    int                                 fd;
    pthread_mutex_t                     stream_list_mutex;
    // linked-list of opened streams
    stream_node_t*                      streams;
} ipu4_state_t;

static ipu4_state_t                     ipu4_state;

static void add_buf_to_list(buffer_node_t** list, buffer_node_t* buf)
{
    buffer_node_t*                      last_buf;

    buf->next = NULL;
    if (*list == NULL) {
        *list = buf;
    } else {
        last_buf = *list;
        while (last_buf->next != NULL) {
            last_buf = last_buf->next;
        }
        last_buf->next = buf;
    }
}

static void remove_buf_from_list(buffer_node_t** list, buffer_node_t* buf)
{
    buffer_node_t*                      prev_buf;

    if (*list == buf) {
        *list = buf->next;
    } else {
        prev_buf = *list;
        while (prev_buf->next != NULL) {
            if (prev_buf->next == buf) {
                prev_buf->next = buf->next;
                break;
            }
            prev_buf = prev_buf->next;
        }
    }
}

static buffer_node_t* get_buf_from_list(buffer_node_t* buf, void* v_addr)
{
    while (buf) {
        if (buf->v_addr == v_addr){
            break;
        }
        buf = buf->next;
    }
    return buf;
}

static void destroy_buf_list(buffer_node_t* buf)
{
    buffer_node_t*                      next_buf;

    while (buf) {
        next_buf = buf->next;
        free(buf);
        buf = next_buf;
    }
}

static stream_node_t* get_stream_from_list(ipu4_handle_t handle)
{
    stream_node_t* stream;

    pthread_mutex_lock(&ipu4_state.stream_list_mutex);
    stream = ipu4_state.streams;
    while (stream) {
        if (stream->handle == handle){
            break;
        }
        stream = stream->next;
    }
    pthread_mutex_unlock(&ipu4_state.stream_list_mutex);
    return stream;
}

int ipu4_initialize(ipu4_csi2_config_t* config)
{
    int                                 i;
    int                                 err;
    ipu4_ipc_initialize_t               cmd;

    if (config == NULL) {
        return EINVAL;
    }

    err = pthread_mutex_init(&ipu4_state.stream_list_mutex, NULL);
    if (err != EOK) {
        qnx_error("Failed to initialize stream_list_mutex: err = %d", err);
        return err;
    }

    memset(&cmd, 0, sizeof(cmd));
    for (i = 0; i < IPU4_CSI2_NUMPORTS; i++) {
        cmd.config[i].num_lanes = config[i].num_lanes;
    }

    ipu4_state.fd = open(IPU4_DEV_PATH, O_RDONLY);
    if(ipu4_state.fd == -1) {
        qnx_error("Failed to connect to ipu4int service");
        pthread_mutex_destroy(&ipu4_state.stream_list_mutex);
        return ENODEV;
    }

    err = devctl(ipu4_state.fd, DCMD_IPU4_INITIALIZE, &cmd, sizeof(cmd), NULL);
    if (err != EOK) {
        close(ipu4_state.fd);
        ipu4_state.fd = -1;
        pthread_mutex_destroy(&ipu4_state.stream_list_mutex);
    }
    return err;
}

int ipu4_set_max_streams(uint32_t num)
{
    ipu4_ipc_set_max_streams_t          cmd;
    int                                 err;

    // Sanity check
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }

    // Send command
    cmd.num_streams = num;
    err = devctl(ipu4_state.fd, DCMD_IPU4_SET_MAX_STREAMS, &cmd, sizeof(cmd), NULL);
    return err;
}

int ipu4_destroy(void)
{
    int                                 err;

    if (ipu4_state.fd == -1) {
        qnx_warning("IPU4 driver isn't initialized");
        return EOK;
    }
    close(ipu4_state.fd);
    ipu4_state.fd = -1;

    err = pthread_mutex_destroy(&ipu4_state.stream_list_mutex);
    if (err != EOK) {
        qnx_error("Failed to destroy stream_list_mutexx: err = %d", err);
    }

    return err;
}

int ipu4_reset()
{
    int                                 err;
    ipu4_ipc_reset_t                    cmd;
    stream_node_t*                      stream;

    // Sanity check
    if (ipu4_state.fd == -1) {
        qnx_warning("IPU4 driver isn't initialized");
        return EOK;
    }

    // Clean-up all local stream information
    pthread_mutex_lock(&ipu4_state.stream_list_mutex);
    while (ipu4_state.streams) {
        stream = ipu4_state.streams;
        ipu4_state.streams = stream->next;

        // destroy busy list
        destroy_buf_list(stream->busy_buffers);
        // destroy free list
        destroy_buf_list(stream->free_buffers);
        // Destroy stream mutex
        err = pthread_mutex_destroy(&stream->stream_mutex);
        if (err != EOK) {
            qnx_error("Failed to destroy stream mutex: err = %d", err);
        }
        free(stream);
    }
    pthread_mutex_unlock(&ipu4_state.stream_list_mutex);

    memset(&cmd, 0, sizeof(cmd));

    err = devctl(ipu4_state.fd, DCMD_IPU4_RESET, &cmd, sizeof(cmd), NULL);
    return err;
}

static int send_close_stream_cmd(ipu4_handle_t handle)
{
    ipu4_ipc_handle_stream_t            cmd;
    int                                 err;

    memset(&cmd, 0, sizeof(cmd));
    cmd.handle = handle;
    err = devctl(ipu4_state.fd, DCMD_IPU4_CLOSE_STREAM, &cmd, sizeof(cmd), NULL);
    return err;
}

int ipu4_open_stream(ipu4_handle_t* handle, ipu4_csi2_port_t port,
                     ipu4_csi2_vc_t chan)
{
    int                                 err;
    ipu4_ipc_open_stream_t              cmd;
    stream_node_t*                      stream;
    stream_node_t*                      prev_stream = NULL;
    stream_node_t*                      stream_node = NULL;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (port >= IPU4_CSI2_NUMPORTS) {
        qnx_error("Invalid port of %d", port);
        return EINVAL;
    }
    if (chan >= IPU4_CSI2_NUMVCS) {
        qnx_error("Invalid VC of %d", chan);
        return EINVAL;
    }
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }

    stream_node = (stream_node_t*) calloc(1, sizeof(stream_node_t));
    if (stream_node == NULL) {
        qnx_error("Failed to allocate stream information");
        return ENOMEM;
    }

    // Initialize stream mutex
    err = pthread_mutex_init(&stream_node->stream_mutex, NULL);
    if (err != EOK) {
        qnx_error("Failed to initialize stream mutex: err = %d", err);
        free(stream_node);
        return err;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.i.port = port;
    cmd.i.vc = chan;
    err = devctl(ipu4_state.fd, DCMD_IPU4_OPEN_STREAM, &cmd, sizeof(cmd), NULL);
    if (!err) {
        pthread_mutex_lock(&ipu4_state.stream_list_mutex);
        stream = ipu4_state.streams;
        while (stream) {
            if (stream->handle == cmd.o.handle) {
                qnx_error("FATAL error: got existing stream handle %p", cmd.o.handle);
                send_close_stream_cmd(cmd.o.handle);
                pthread_mutex_destroy(&stream_node->stream_mutex);
                free(stream_node);
                pthread_mutex_unlock(&ipu4_state.stream_list_mutex);
                return ENOSYS;
            }
            prev_stream = stream;
            stream = stream->next;
        }
        stream_node->handle = cmd.o.handle;
        if (prev_stream) {
            prev_stream->next = stream_node;
        } else {
            ipu4_state.streams = stream_node;
        }
        pthread_mutex_unlock(&ipu4_state.stream_list_mutex);
        *handle = cmd.o.handle;
    }

    return err;
}

int ipu4_close_stream(ipu4_handle_t handle)
{
    int                                 err;
    stream_node_t*                      stream;
    stream_node_t*                      prev_stream = NULL;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }

    err = send_close_stream_cmd(handle);

    // cleanup all linked lists related to closed stream
    pthread_mutex_lock(&ipu4_state.stream_list_mutex);
    stream = ipu4_state.streams;
    while (stream) {
        if (stream->handle == handle) {
            break;
        }
        prev_stream = stream;
        stream = stream->next;
    }
    if (stream) {
        // destroy busy list
        destroy_buf_list(stream->busy_buffers);
        // destroy free list
        destroy_buf_list(stream->free_buffers);
        // remove stream from list and destroy it
        if (prev_stream) {
            prev_stream->next = stream->next;
        } else {
            ipu4_state.streams = stream->next;
        }
        // Destroy stream mutex
        err = pthread_mutex_destroy(&stream->stream_mutex);
        if (err != EOK) {
            qnx_error("Failed to destroy stream mutex: err = %d", err);
        }
        free(stream);
    }
    pthread_mutex_unlock(&ipu4_state.stream_list_mutex);

    return err;
}

int ipu4_configure_stream(ipu4_handle_t handle, ipu4_stream_config_t* config)
{
    int                                 err;
    ipu4_ipc_configure_stream_t         cmd;
    stream_node_t*                      stream;

    // Sanity check
    if (config == NULL) {
        qnx_error("NULL config provided");
        return EINVAL;
    }
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }
    stream = get_stream_from_list(handle);
    if (stream == NULL) {
        qnx_error("Failed to find incoming handle=%p in linked list", handle);
        return EINVAL;
    }
    stream->buffer_size = config->buffersize;

    memset(&cmd, 0, sizeof(cmd));
    cmd.handle = handle;
    cmd.config.width = config->width;
    cmd.config.height = config->height;
    cmd.config.roi_x = config->roi_x;
    cmd.config.roi_y = config->roi_y;
    cmd.config.input_format = config->input_format;
    cmd.config.output_format = config->output_format;
    cmd.config.bytes_per_line = config->bytes_per_line;
    cmd.config.buffersize = config->buffersize;
    cmd.config.csi2_frequency = config->csi2_frequency;
    cmd.config.width = config->width;
    strlcpy(cmd.config.decoder_sync_lib, config->decoder_sync_lib, sizeof(cmd.config.decoder_sync_lib));
    cmd.config.decoder_sync_data = config->decoder_sync_data;

    err = devctl(ipu4_state.fd, DCMD_IPU4_CONFIGURE_STREAM, &cmd, sizeof(cmd), NULL);

    return err;
}

int ipu4_start_stream(ipu4_handle_t handle)
{
    int                                 err;
    ipu4_ipc_handle_stream_t            cmd;
    stream_node_t*                      stream;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }
    stream = get_stream_from_list(handle);
    if (stream == NULL) {
        qnx_error("Failed to find incoming handle=%p in linked list", handle);
        return EINVAL;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.handle = handle;
    err = devctl(ipu4_state.fd, DCMD_IPU4_START_STREAM, &cmd, sizeof(cmd), NULL);
    return err;
}

int ipu4_stop_stream(ipu4_handle_t handle)
{
    int                                 err;
    ipu4_ipc_handle_stream_t            cmd;
    stream_node_t*                      stream;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }
    stream = get_stream_from_list(handle);
    if (stream == NULL) {
        qnx_error("Failed to find incoming handle=%p in linked list", handle);
        return EINVAL;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.handle = handle;
    err = devctl(ipu4_state.fd, DCMD_IPU4_STOP_STREAM, &cmd, sizeof(cmd), NULL);
    return err;
}

int ipu4_flush_stream(ipu4_handle_t handle)
{
    stream_node_t*                      stream;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }
    stream = get_stream_from_list(handle);
    if (stream == NULL) {
        qnx_error("Failed to find incoming handle=%p in linked list", handle);
        return EINVAL;
    }

    // Empty out our buffer lists such that start with new ones
    pthread_mutex_lock(&stream->stream_mutex);
    destroy_buf_list(stream->busy_buffers);
    destroy_buf_list(stream->free_buffers);
    stream->busy_buffers = NULL;
    stream->free_buffers = NULL;
    pthread_mutex_unlock(&stream->stream_mutex);

    // TODO: should be sending DCMD_IPU4_FLUSH_STREAM command to flush buffers
    // on ResMgr/driver side as well, but this is currently not working
    return EOK;
}

int ipu4_get_buffer(ipu4_handle_t handle, ipu4_buffer_t* buffer, uint64_t timeout)
{
    int                                 err;
    ipu4_ipc_get_buffer_t               cmd;
    stream_node_t*                      stream;
    buffer_node_t*                      buf;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (buffer == NULL) {
        qnx_error("NULL buffer provided");
        return EINVAL;
    }
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }
    stream = get_stream_from_list(handle);
    if (stream == NULL) {
        qnx_error("Failed to find incoming handle=%p in linked list", handle);
        return EINVAL;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.i.handle = handle;
    cmd.i.timeout = timeout;
    err = devctl(ipu4_state.fd, DCMD_IPU4_GET_BUFFER, &cmd, sizeof(cmd), NULL);
    if (!err) {
        pthread_mutex_lock(&stream->stream_mutex);
        buf = stream->busy_buffers;
        while (buf) {
            if (buf->chunk_p_addr[0] == cmd.o.p_addr){
                break;
            }
            buf = buf->next;
        }
        if (buf == NULL) {
            qnx_error("Failed to find phys buffer addr in linked list");
            pthread_mutex_unlock(&stream->stream_mutex);
            return EINVAL;
        }

        // remove buf from busy list
        remove_buf_from_list(&stream->busy_buffers, buf);
        // add buf to free list
        add_buf_to_list(&stream->free_buffers,buf);

        buffer->data                = buf->v_addr;
        buffer->timestamp.tv_sec    = cmd.o.timestamp.tv_sec;
        buffer->timestamp.tv_nsec   = cmd.o.timestamp.tv_nsec;
        buffer->field               = cmd.o.field;
        buffer->seqNum              = cmd.o.seqNum;
        pthread_mutex_unlock(&stream->stream_mutex);
    }

    return err;
}

int ipu4_queue_buffer(ipu4_handle_t handle, ipu4_buffer_t* buffer)
{
    buffer_node_t*                      buf;
    int                                 err;
    ipu4_ipc_queue_buffer_t*            cmd;
    stream_node_t*                      stream;
    uint32_t                            buffer_size;
    void*                               buffer_addr;
    int                                 i, idx = 0;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (buffer == NULL) {
        qnx_error("NULL buffer provided");
        return EINVAL;
    }
    if (ipu4_state.fd == -1) {
        qnx_error("IPU4 driver isn't initialized");
        return ENODEV;
    }
    stream = get_stream_from_list(handle);
    if (stream == NULL) {
        qnx_error("Failed to find incoming handle=%p in linked list", handle);
        return EINVAL;
    }

    pthread_mutex_lock(&stream->stream_mutex);

    /* Allocate temporary cmd buffer.
     * This may be large, given the change in max number of chunks.
     */
    cmd = calloc(sizeof(ipu4_ipc_queue_buffer_t), 1);
    if (cmd == NULL) {
        qnx_error("No memory for cmd");
        return ENOMEM;
    }

    // check if we already have this buffer in free list
    buf = get_buf_from_list(stream->free_buffers, buffer->data);
    // If buffer not found, it is either a buffer in busy list (error) or a new buffer
    if (buf == NULL) {
        // check if buffer is in busy list
        buf = get_buf_from_list(stream->busy_buffers, buffer->data);
        if (buf != NULL) {
            qnx_error("Buffer already queued");
            free(cmd);
            pthread_mutex_unlock(&stream->stream_mutex);
            return EINVAL;
        }

        buf = (buffer_node_t*) calloc(1, sizeof(buffer_node_t));
        if (buf == NULL) {
            qnx_error("Failed to allocate buffer information");
            free(cmd);
            pthread_mutex_unlock(&stream->stream_mutex);
            return ENOMEM;
        }
        buf->v_addr = buffer->data;
        buffer_size = stream->buffer_size;
        buffer_addr = buf->v_addr;
        // calculate address of physically contigious chunks for provided buffer
        while (buffer_size > 0) {
            if ( mem_offset(buffer_addr, NOFD, buffer_size, &buf->chunk_p_addr[idx], &buf->chunk_size[idx]) == -1 )
            {
                qnx_error("Failed to get physical address for buf %p", buffer_addr);
                free(buf);
                free(cmd);
                pthread_mutex_unlock(&stream->stream_mutex);
                return ENOMEM;
            }
            // check that not last chunk is aligned to 4K
            if (buffer_size - buf->chunk_size[idx] > 0) {
                if (buf->chunk_size[idx] % PAGE_SIZE) {
                    qnx_error("Physically contigious chunk is not aligned to 4K");
                    free(buf);
                    free(cmd);
                    pthread_mutex_unlock(&stream->stream_mutex);
                    return EINVAL;
                }
                // check that provided buffer is not badly fragmented
                if (idx + 1 == MAX_CHUNKS_NUMBER_IN_BUFFER) {
                    qnx_error("Provided buffer %p is badly fragmented", buf->v_addr);
                    free(buf);
                    free(cmd);
                    pthread_mutex_unlock(&stream->stream_mutex);
                    return EINVAL;
                }
            }
            buffer_size -= buf->chunk_size[idx];
            buffer_addr += buf->chunk_size[idx];
            idx++;
        }
        buf->chunks_num = idx;
        buf->next = NULL;
    }else {
        // remove buf from free list
        remove_buf_from_list(&stream->free_buffers, buf);
    }
    pthread_mutex_unlock(&stream->stream_mutex);

    // pass buffer to ipu4 service
    memset(cmd, 0, sizeof(ipu4_ipc_queue_buffer_t));
    cmd->handle = handle;
    cmd->chunks_num = buf->chunks_num;
    for (i = 0; i < buf->chunks_num; i++) {
        cmd->chunk_p_addr[i] = buf->chunk_p_addr[i];
        cmd->chunk_size[i] = buf->chunk_size[i];
    }
    err = devctl(ipu4_state.fd, DCMD_IPU4_QUEUE_BUFFER, cmd, sizeof(ipu4_ipc_queue_buffer_t), NULL);

    pthread_mutex_lock(&stream->stream_mutex);
    if (err == EOK) {
        // add buffer to busy list
        add_buf_to_list(&stream->busy_buffers, buf);
    } else {
        // add buffer to free list
        add_buf_to_list(&stream->free_buffers, buf);
    }
    free(cmd);
    pthread_mutex_unlock(&stream->stream_mutex);

    return err;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/intel-ipu4/ipu4-interface.c $ $Rev: 876784 $")
#endif
