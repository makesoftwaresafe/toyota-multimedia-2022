/*
* Copyright (c) 2017 QNX Software Systems.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 * ipu4_resmgr.c
 *
 * This file creates and handles the resource manager for the IPU4 driver.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

// To override macro  OCB, forward declare the type defined below
typedef struct ipu4_resmgr_ocb ipu4_resmgr_ocb_t;

#define THREAD_POOL_PARAM_T dispatch_context_t
#define IOFUNC_OCB_T ipu4_resmgr_ocb_t
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/mman.h>

#include "ipu4_log.h"
#include "ipu4_resmgr.h"
#include <intel/ipu4/intel-ipu4-interface.h>

/**
 * @brief Our base path for our resource manager
 */
#define IPU4_DEV_PATH                   "/dev/ipu4"
#define IPU4_SHUTDOWN_WAIT_TIMEOUT_SEC  (2)

/**
 * @brief Our internal tracking of user's buffers
 */
typedef struct ipu4_resmgr_buffer_node{
    struct ipu4_resmgr_buffer_node*     next;
    off_t                               chunk_p_addr[MAX_CHUNKS_NUMBER_IN_BUFFER];
    void*                               chunk_v_addr[MAX_CHUNKS_NUMBER_IN_BUFFER];
    unsigned int                        chunk_size[MAX_CHUNKS_NUMBER_IN_BUFFER];
    unsigned int                        chunks_num;
} ipu4_resmgr_buffer_node_t;

/**
 * @brief Our internal tracking of opened streams
 */
typedef struct ipu4_resmgr_stream_node{
    struct ipu4_resmgr_stream_node*     next;
    ipu4_handle_t                       handle;
    pthread_mutex_t                     stream_mutex;
    // linked-list of buffers that belong to user that we have seen before
    ipu4_resmgr_buffer_node_t*          free_buffers;
    // linked-list of buffers that were queued to driver
    ipu4_resmgr_buffer_node_t*          busy_buffers;
} ipu4_resmgr_stream_node_t;

/**
 * @brief Our client OCB override - to store additional info we may need
 * @details
 * The 'next' entry is to make it fit easily into our linked-list.
 */
typedef struct ipu4_resmgr_ocb {
    iofunc_ocb_t                        ocb;
    ipu4_handle_t                       handle;
    struct ipu4_resmgr_ocb*             next;
} ipu4_resmgr_ocb_t;

/**
 * @brief State information of our resource manager
 */
typedef struct {
    dispatch_t*                         dispatch;
    thread_pool_t*                      thread_pool;
    iofunc_attr_t                       root_attr;
    iofunc_mount_t                      mount;
    iofunc_funcs_t                      mount_funcs;
    resmgr_connect_funcs_t              connect_funcs;
    resmgr_io_funcs_t                   io_funcs;
    int                                 resmgr_id;
    bool                                ipu4drv_initialized;
    bool                                ipu4drv_shutting_down;
    int                                 active_count;
    pthread_cond_t                      shutdown_condvar;
    pthread_mutex_t                     rm_mutex;
    ipu4_csi2_config_t                  csi2_config[IPU4_CSI2_NUMPORTS];
    // linked-list of buffer's info that belong to opened stream
    ipu4_resmgr_stream_node_t*          streams;
    ipu4_resmgr_ocb_t                   ocb[IPU4_CSI2_NUMPORTS];
    pthread_mutex_t                     ocb_mutex;
    ipu4_resmgr_ocb_t*                  ocb_free_list;
    ipu4_resmgr_ocb_t*                  ocb_in_use_list;
} ipu4_resmgr_state_t;

static ipu4_resmgr_state_t              resmgr_state;


static void add_buf_to_list(ipu4_resmgr_buffer_node_t** list, ipu4_resmgr_buffer_node_t* buf)
{
    ipu4_resmgr_buffer_node_t*      last_buf;

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

static void remove_buf_from_list(ipu4_resmgr_buffer_node_t** list, ipu4_resmgr_buffer_node_t* buf)
{
    ipu4_resmgr_buffer_node_t*      prev_buf;

    if (*list == NULL) {
        return;
    }
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

static ipu4_resmgr_buffer_node_t* get_buf_from_list(ipu4_resmgr_buffer_node_t* buf, off_t p_addr)
{
    while (buf) {
        if (buf->chunk_p_addr[0] == p_addr){
            break;
        }
        buf = buf->next;
    }
    return buf;
}

static void destroy_buf_list(ipu4_resmgr_buffer_node_t* buf)
{
    ipu4_resmgr_buffer_node_t*      next_buf;
    int                             i;

    while (buf) {
        next_buf = buf->next;
        for (i = 0; i < buf->chunks_num; i++) {
            munmap(buf->chunk_v_addr[i], buf->chunk_size[i]);
        }
        free(buf);
        buf = next_buf;
    }
}

static ipu4_resmgr_stream_node_t* get_stream_from_list(ipu4_handle_t handle)
{
    ipu4_resmgr_stream_node_t*      stream;

    pthread_mutex_lock(&resmgr_state.rm_mutex);
    stream = resmgr_state.streams;
    while (stream) {
        if (stream->handle == handle){
            break;
        }
        stream = stream->next;
    }
    pthread_mutex_unlock(&resmgr_state.rm_mutex);
    return stream;
}

static int initialize(ipu4_csi2_config_t* config)
{
    int                             i, status = EOK;

    if (resmgr_state.ipu4drv_initialized == false) {
        status = ipu4int_initialize((ipu4int_csi2_config_t*)config);
        if (status == EOK) {
            resmgr_state.ipu4drv_initialized = true;
            for (i = 0; i < IPU4_CSI2_NUMPORTS; i++) {
                resmgr_state.csi2_config[i].num_lanes = config[i].num_lanes;
            }
        }
    }
    return status;
}

static int open_stream(ipu4_ipc_open_stream_t* cmd, ipu4_resmgr_ocb_t* ocb)
{
    int                             status = EOK;
    ipu4_handle_t                   handle;
    ipu4_resmgr_stream_node_t*      stream;
    ipu4_resmgr_stream_node_t*      prev_stream = NULL;
    ipu4_resmgr_stream_node_t*      stream_node = NULL;

    stream_node = (ipu4_resmgr_stream_node_t*) calloc(1, sizeof(ipu4_resmgr_stream_node_t));
    if (stream_node == NULL) {
        LOG_ERROR("Failed to allocate stream information");
        return ENOMEM;
    }
    // Initialize stream mutex
    status = pthread_mutex_init(&stream_node->stream_mutex, NULL);
    if (status != EOK) {
        LOG_ERROR("Failed to initialize stream mutex: err = %d", status);
        free(stream_node);
        return status;
    }

    status = ipu4int_open_stream(&handle, cmd->i.port, cmd->i.vc);
    if (status == EOK) {
        pthread_mutex_lock(&resmgr_state.rm_mutex);
        stream = resmgr_state.streams;
        while (stream) {
            if (stream->handle == handle) {
                LOG_ERROR("FATAL error: got existing stream handle %p", handle);
                ipu4int_close_stream(handle);
                pthread_mutex_destroy(&stream_node->stream_mutex);
                free(stream_node);
                pthread_mutex_unlock(&resmgr_state.rm_mutex);
                return ENOSYS;
            }
            prev_stream = stream;
            stream = stream->next;
        }
        stream_node->handle = handle;
        if (prev_stream) {
            prev_stream->next = stream_node;
        } else {
            resmgr_state.streams = stream_node;
        }
        pthread_mutex_unlock(&resmgr_state.rm_mutex);
        cmd->o.handle = handle;
        ocb->handle = handle;
    }
    return status;
}

static int close_stream(ipu4_handle_t handle, ipu4_resmgr_ocb_t *ocb)
{
    int                             err, status = EOK;
    ipu4_resmgr_stream_node_t*      stream;
    ipu4_resmgr_stream_node_t*      prev_stream = NULL;

    status = ipu4int_close_stream(handle);
    pthread_mutex_lock(&resmgr_state.rm_mutex);
    stream = resmgr_state.streams;
    // unmap all buffers related to closed stream
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
            resmgr_state.streams = stream->next;
        }
        // Destroy stream mutex
        err = pthread_mutex_destroy(&stream->stream_mutex);
        if (err != EOK) {
            LOG_ERROR("Failed to destroy stream mutex: err = %d", err);
            if (status == EOK) {
                status = err;
            }
        }
        free(stream);
        ocb->handle = NULL;
    }

    pthread_mutex_unlock(&resmgr_state.rm_mutex);
    return status;
}

static int close_all_streams()
{
    int                             err, status = EOK;
    ipu4_resmgr_stream_node_t*      stream;

    pthread_mutex_lock(&resmgr_state.rm_mutex);
    stream = resmgr_state.streams;
    // unmap all buffers related to closed stream
    while (stream) {
        resmgr_state.streams = stream->next;
        pthread_mutex_unlock(&resmgr_state.rm_mutex);

        status = ipu4int_close_stream(stream->handle);
        // Note: continue closing remaining streams even if status is not EOK.

        pthread_mutex_lock(&resmgr_state.rm_mutex);
        // destroy busy list
        destroy_buf_list(stream->busy_buffers);
        // destroy free list
        destroy_buf_list(stream->free_buffers);
        // Destroy stream mutex
        err = pthread_mutex_destroy(&stream->stream_mutex);
        if (err != EOK) {
            LOG_ERROR("Failed to destroy stream mutex: err = %d", err);
            if (status == EOK) {
                status = err;
            }
        }
        free(stream);
        stream = resmgr_state.streams;
    }

    pthread_mutex_unlock(&resmgr_state.rm_mutex);
    return status;
}

static int reset_all_streams()
{
    int                             status;
    int                             err;
    ipu4_resmgr_stream_node_t*      stream;

    // Let the IPU4 driver perform the reset; this will stop, deinit and then
    // initialize the IPU4 driver.
    status = ipu4int_reset();

    // Destroy all of our streams since they are no longer used
    pthread_mutex_lock(&resmgr_state.rm_mutex);
    stream = resmgr_state.streams;
    while (stream) {
        resmgr_state.streams = stream->next;
        // destroy busy list
        destroy_buf_list(stream->busy_buffers);
        // destroy free list
        destroy_buf_list(stream->free_buffers);
        // Destroy stream mutex
        err = pthread_mutex_destroy(&stream->stream_mutex);
        if (err != EOK) {
            LOG_ERROR("Failed to destroy stream mutex: err = %d", err);
            if (status == EOK) {
                status = err;
            }
        }
        free(stream);
        stream = resmgr_state.streams;
    }
    pthread_mutex_unlock(&resmgr_state.rm_mutex);

    return status;
}

static int flush_stream(ipu4_handle_t handle)
{
    ipu4_resmgr_stream_node_t*      stream;
    int                             status;

    // Have interface first flush the stream
    status = ipu4int_flush_stream(handle);

    // Find stream associated with handle
    stream = get_stream_from_list(handle);
    if (stream == NULL) {
        LOG_ERROR("Failed to find incoming handle=%p in linked list", handle);
        return EINVAL;
    }

    // Flush our buffer lists for the stream
    pthread_mutex_lock(&stream->stream_mutex);
    destroy_buf_list(stream->busy_buffers);
    stream->busy_buffers = NULL;
    destroy_buf_list(stream->free_buffers);
    stream->free_buffers = NULL;
    pthread_mutex_unlock(&stream->stream_mutex);

    return status;
}

static int get_buffer(ipu4_ipc_get_buffer_t* cmd)
{
    int                             status = EOK;
    ipu4_resmgr_stream_node_t*      stream;
    ipu4_resmgr_buffer_node_t*      buf;
    ipu4_buffer_t                   buffer;

    stream = get_stream_from_list(cmd->i.handle);
    if (stream == NULL) {
        LOG_ERROR("Failed to find incoming handle=%p in linked list", cmd->i.handle);
        return EINVAL;
    }

    pthread_mutex_lock(&stream->stream_mutex);
    status = ipu4int_get_buffer(stream->handle, (ipu4int_buffer_t*)&buffer, cmd->i.timeout);
    if (status == EOK) {
        // look for buffer's physical address
        buf = stream->busy_buffers;
        while (buf) {
            if (buf->chunk_v_addr[0] == buffer.data) {
                break;
            }
            buf = buf->next;
        }
        if (buf == NULL) {
            LOG_ERROR("FATAL error: failed to find phys address in linked list");
            pthread_mutex_unlock(&stream->stream_mutex);
            return ENOSYS;
        }

        // remove buf from busy list
        remove_buf_from_list(&stream->busy_buffers, buf);
        // add buf to free list
        add_buf_to_list(&stream->free_buffers, buf);

        // prepare respond info
        cmd->o.p_addr = buf->chunk_p_addr[0];
        cmd->o.timestamp.tv_sec = buffer.timestamp.tv_sec;
        cmd->o.timestamp.tv_nsec = buffer.timestamp.tv_nsec;
        cmd->o.field = buffer.field;
        cmd->o.seqNum = buffer.seqNum;
    }

    pthread_mutex_unlock(&stream->stream_mutex);
    return status;
}

static int queue_buffer(ipu4_ipc_queue_buffer_t* cmd)
{
    int                             status = EOK;
    ipu4_resmgr_stream_node_t*      stream;
    ipu4_resmgr_buffer_node_t*      buf;
    int                             i, j;
    void*                           virt_addr = NULL;
    ipu4_buffer_t                   buffer;
    uint32_t                        total_size;

    stream = get_stream_from_list(cmd->handle);
    if (stream == NULL) {
        LOG_ERROR("Failed to find incoming handle=%p in linked list", cmd->handle);
        return EINVAL;
    }

    pthread_mutex_lock(&stream->stream_mutex);
    // check if we already have this buffer in free list
    buf = get_buf_from_list(stream->free_buffers, cmd->chunk_p_addr[0]);
    // If buffer not found, it is either a buffer in busy list (error) or a new buffer
    if (buf == NULL) {
        // check if buffer is in busy list
        buf = get_buf_from_list(stream->busy_buffers, cmd->chunk_p_addr[0]);
        if (buf != NULL) {
            LOG_ERROR("Buffer already queued");
            pthread_mutex_unlock(&stream->stream_mutex);
            return EINVAL;
        }

        if (cmd->chunks_num > MAX_CHUNKS_NUMBER_IN_BUFFER) {
            LOG_ERROR("Provided buffer is badly fragmented ");
            pthread_mutex_unlock(&stream->stream_mutex);
            return EINVAL;
        }
        buf = (ipu4_resmgr_buffer_node_t*) calloc(1, sizeof(ipu4_resmgr_buffer_node_t));
        if (buf == NULL) {
            LOG_ERROR("Failed to allocate buffer information");
            pthread_mutex_unlock(&stream->stream_mutex);
            return ENOMEM;
        }

        total_size = 0;
        for (i = 0; i < cmd->chunks_num; i++) {
            total_size += cmd->chunk_size[i];
        }

        /* Pre-map the entire frame buffer as one contiguous memory. */
        i = 0;
        virt_addr = mmap(virt_addr, total_size, PROT_READ|PROT_WRITE, MAP_PHYS|MAP_SHARED, NOFD, cmd->chunk_p_addr[i]);
        if (virt_addr == MAP_FAILED) {
            LOG_ERROR("pre-mmap failed : %s\n", strerror(errno));
            free(buf);
            pthread_mutex_unlock(&stream->stream_mutex);
            return ENOMEM;
        }
        buf->chunk_v_addr[i] = virt_addr;
        buf->chunk_p_addr[i] = cmd->chunk_p_addr[i];
        buf->chunk_size[i] = cmd->chunk_size[i];
        virt_addr += cmd->chunk_size[i];

        /* Map the remaining chunks within the contiguous area. */
        for (i = 1; i < cmd->chunks_num; i++) {
            virt_addr = mmap(virt_addr, cmd->chunk_size[i], PROT_READ|PROT_WRITE, MAP_FIXED|MAP_PHYS|MAP_SHARED, NOFD, cmd->chunk_p_addr[i]);
            if (virt_addr == MAP_FAILED) {
                LOG_ERROR("mmap failed : %s\n", strerror(errno));
                for (j = i-1; j > -1; j--) {
                    munmap(buf->chunk_v_addr[j], buf->chunk_size[j]);
                }
                free(buf);
                pthread_mutex_unlock(&stream->stream_mutex);
                return EINVAL;
            }
            buf->chunk_v_addr[i] = virt_addr;
            buf->chunk_p_addr[i] = cmd->chunk_p_addr[i];
            buf->chunk_size[i] = cmd->chunk_size[i];
            virt_addr += cmd->chunk_size[i];
        }
        buf->chunks_num = cmd->chunks_num;
        buf->next = NULL;
    } else {
        // remove buf from free list
        remove_buf_from_list(&stream->free_buffers, buf);
    }

    // pass buffer to intel-ipu4drv library
    buffer.data = buf->chunk_v_addr[0];
    status = ipu4int_queue_buffer(cmd->handle, (ipu4int_buffer_t*)&buffer);

    buf->next = NULL;
    if (status == EOK) {
        // add buffer to busy list
        add_buf_to_list(&stream->busy_buffers, buf);
    } else {
        // add buffer to free list
        add_buf_to_list(&stream->free_buffers, buf);
    }
    pthread_mutex_unlock(&stream->stream_mutex);
    return status;
}

/**
 * Our OCB allocation implementation.
 *
 * @param [IN] ctp Pointer to the resource manager context
 * @param [IN] attr The device attribute
 * @return Pointer to the allocated OCB.
 */
static ipu4_resmgr_ocb_t* ipu4_ocb_calloc(resmgr_context_t* ctp, iofunc_attr_t* attr)
{
    ipu4_resmgr_ocb_t*         ocb;

    // Ensure atomic access to OCB alloc/free, as this is shared by all devices
    pthread_mutex_lock(&resmgr_state.ocb_mutex);

    // Get first entry in our free list if any
    if (resmgr_state.ocb_free_list) {
        ocb = resmgr_state.ocb_free_list;
        resmgr_state.ocb_free_list = ocb->next;
    } else {
        // By design, we should never run out
        LOG_ERROR("Could not find a free OCB");
        errno = ENOMEM;
        pthread_mutex_unlock(&resmgr_state.ocb_mutex);
        return NULL;
    }

    // Zero out its content for safety
    memset(ocb, 0, sizeof(ipu4_resmgr_ocb_t));

    // Add the entry to the beginning of our in-use list
    ocb->next = resmgr_state.ocb_in_use_list;
    resmgr_state.ocb_in_use_list = ocb;

    pthread_mutex_unlock(&resmgr_state.ocb_mutex);
    return ocb;
}

/**
 * Our OCB free implementation.
 *
 * @param [IN] ocb Pointer to the OCB to free
 */
static void ipu4_ocb_free(ipu4_resmgr_ocb_t* ocb)
{
    ipu4_resmgr_ocb_t*          in_use_ocb;
    ipu4_resmgr_ocb_t*          prev_ocb;

    // Nothing to do for NULL ocb
    if (ocb == NULL) {
        return;
    }

    // Check if we have opened stream corresponding to ocb->handle
    if (get_stream_from_list(ocb->handle)) {
        close_stream(ocb->handle, ocb);
    }

    // Ensure atomic access to OCB alloc/free, as this is shared by all devices
    pthread_mutex_lock(&resmgr_state.ocb_mutex);

    // Find the ocb we want to free in our in-use list
    in_use_ocb = resmgr_state.ocb_in_use_list;
    prev_ocb = NULL;
    while (in_use_ocb) {
        if (in_use_ocb == ocb) {
            break;
        }
        prev_ocb = in_use_ocb;
        in_use_ocb = in_use_ocb->next;
    }

    // It should not happen that we cannot find a match
    if (in_use_ocb == NULL) {
        LOG_ERROR("Cannot find match in in-use list for ocb %p", ocb);
        pthread_mutex_unlock(&resmgr_state.ocb_mutex);
        return;
    }

    // Remove ocb from our in-use list
    if (prev_ocb) {
        prev_ocb->next = ocb->next;
    } else {
        resmgr_state.ocb_in_use_list = ocb->next;
    }

    // Add the ocb back to the begining of our free list
    ocb->next = resmgr_state.ocb_free_list;
    resmgr_state.ocb_free_list = ocb;

    pthread_mutex_unlock(&resmgr_state.ocb_mutex);
}

// Explicitly lock resMgr attr mutex; locked by default in all io calls
static int attr_lock()
{
    IOFUNC_ATTR_T* attr = (IOFUNC_ATTR_T*)&resmgr_state.root_attr;
    int (*attr_lock)(iofunc_attr_t *);
    iofunc_funcs_t *funcs;
    if (attr->mount == NULL || (funcs = attr->mount->funcs) == NULL ||
        funcs->nfuncs < (offsetof(iofunc_funcs_t, attr_lock) / sizeof(void *)) ||
        (attr_lock = funcs->attr_lock) == NULL) {
        attr_lock = iofunc_attr_lock;
    }
    return attr_lock(attr);
}

// Explicitly unlock resMgr attr mutex
static int attr_unlock()
{
    IOFUNC_ATTR_T* attr = (IOFUNC_ATTR_T*)&resmgr_state.root_attr;
    int (*attr_unlock)(iofunc_attr_t *);
    iofunc_funcs_t *funcs;
    if (attr->mount == NULL || (funcs = attr->mount->funcs) == NULL ||
       funcs->nfuncs < (offsetof(iofunc_funcs_t, attr_unlock) / sizeof(void *)) ||
       (attr_unlock = funcs->attr_unlock) == NULL) {
        attr_unlock = iofunc_attr_unlock;
    }
    return attr_unlock(attr);
}

static int ipu4_io_devctl(resmgr_context_t* ctp, io_devctl_t* msg, ipu4_resmgr_ocb_t* ocb)
{
    int nbytes, status;

    // service default devctl functions
    status = iofunc_devctl_default(ctp, msg, (iofunc_ocb_t *) ocb);
    if (status != (int)_RESMGR_DEFAULT) {
        return status;
    }
    status = iofunc_devctl_verify(ctp,
                                  msg,
                                  (iofunc_ocb_t*) ocb,
                                  _IO_DEVCTL_VERIFY_OCB_READ);
    if (status != EOK) {
        return status;
    }
    // Unlock attr mutex explicitly lock on calling to permit multiple command handling
    attr_unlock();

    status = nbytes = 0;
    nbytes = msg->i.nbytes;

    pthread_mutex_lock(&resmgr_state.rm_mutex);
    if (resmgr_state.ipu4drv_shutting_down) {
        pthread_mutex_unlock(&resmgr_state.rm_mutex);
        attr_lock();
        return EFAULT;
    }
    resmgr_state.active_count++;
    pthread_mutex_unlock(&resmgr_state.rm_mutex);

    switch (msg->i.dcmd) {
        case DCMD_IPU4_INITIALIZE:
        {
            ipu4_ipc_initialize_t*          cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(ipu4_ipc_initialize_t))) {
                LOG_ERROR("DCMD_IPU4_INITIALIZE fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            nbytes = 0;
            pthread_mutex_lock(&resmgr_state.rm_mutex);
            status = initialize(&cmd->config[0]);
            pthread_mutex_unlock(&resmgr_state.rm_mutex);
            break;
        }

        case DCMD_IPU4_OPEN_STREAM:
        {
            ipu4_ipc_open_stream_t*         cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(cmd->i)) || !ocb) {
                LOG_ERROR("DCMD_IPU4_OPEN_STREAM fail nbytes=%d, ocb=%p", nbytes, ocb);
                status = EINVAL;
                break;
            }
            nbytes = 0;
            status = open_stream(cmd, ocb);
            if (status == EOK) {
                nbytes = sizeof(cmd->o);
            }
            break;
        }

        case DCMD_IPU4_CLOSE_STREAM:
        {
            ipu4_ipc_handle_stream_t*       cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(ipu4_ipc_handle_stream_t)) || !ocb) {
                LOG_ERROR("DCMD_IPU4_CLOSE_STREAM fail nbytes=%d, ocb=%p", nbytes, ocb);
                status = EINVAL;
                break;
            }
            nbytes = 0;
            status = close_stream(cmd->handle, ocb);
            break;
        }

        case DCMD_IPU4_CONFIGURE_STREAM:
        {
            ipu4_ipc_configure_stream_t*    cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(ipu4_ipc_configure_stream_t))) {
                LOG_ERROR("DCMD_IPU4_CONFIGURE_STREAM fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            status = ipu4int_configure_stream(cmd->handle, (ipu4int_stream_config_t*)&cmd->config);
            nbytes = 0;
            break;
        }

        case DCMD_IPU4_START_STREAM:
        {
            ipu4_ipc_handle_stream_t*       cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(ipu4_ipc_handle_stream_t))) {
                LOG_ERROR("DCMD_IPU4_START_STREAM fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            status = ipu4int_start_stream(cmd->handle);
            nbytes = 0;
            break;
        }

        case DCMD_IPU4_STOP_STREAM:
        {
            ipu4_ipc_handle_stream_t*       cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(ipu4_ipc_handle_stream_t))) {
                LOG_ERROR("DCMD_IPU4_STOP_STREAM fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            status = ipu4int_stop_stream(cmd->handle);
            nbytes = 0;
            break;
        }

        case DCMD_IPU4_GET_BUFFER:
        {
            ipu4_ipc_get_buffer_t*          cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(ipu4_ipc_get_buffer_t))) {
                LOG_ERROR("DCMD_IPU4_GET_BUFFER fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            nbytes = 0;

            status = get_buffer(cmd);
            if (status == EOK) {
                nbytes = sizeof(cmd->o);
            }
            break;
        }

        case DCMD_IPU4_QUEUE_BUFFER:
        {
            ipu4_ipc_queue_buffer_t *       cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(ipu4_ipc_queue_buffer_t))) {
                LOG_ERROR("DCMD_IPU4_QUEUE_BUFFER fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            nbytes = 0;
            status = queue_buffer(cmd);
            break;
        }

        case DCMD_IPU4_FLUSH_STREAM:
        {
            ipu4_ipc_handle_stream_t*       cmd = _DEVCTL_DATA(msg->i);

            if (nbytes < (sizeof(ipu4_ipc_handle_stream_t))) {
                LOG_ERROR("DCMD_IPU4_FLUSH_STREAM fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            nbytes = 0;
            status = flush_stream(cmd->handle);
            break;
        }

        case DCMD_IPU4_SET_MAX_STREAMS:
        {
            ipu4_ipc_set_max_streams_t*     cmd = _DEVCTL_DATA(msg->i);
            if (nbytes < (sizeof(ipu4_ipc_set_max_streams_t))) {
                LOG_ERROR("DCMD_IPU4_SET_MAX_STREAMS fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            status = ipu4int_set_max_streams(cmd->num_streams);
            nbytes = 0;
            break;
        }

        case DCMD_IPU4_RESET:
        {
            if (nbytes < (sizeof(ipu4_ipc_reset_t))) {
                LOG_ERROR("DCMD_IPU4_RESET fail nbytes=%d", nbytes);
                status = EINVAL;
                break;
            }
            nbytes = 0;
            status = reset_all_streams();
            break;
        }

        default:
            status = ENOSYS;
        }

    pthread_mutex_lock(&resmgr_state.rm_mutex);
    resmgr_state.active_count--;
    if ((resmgr_state.ipu4drv_shutting_down == true) && (resmgr_state.active_count == 0)) {
        pthread_cond_signal(&resmgr_state.shutdown_condvar);
    }
    pthread_mutex_unlock(&resmgr_state.rm_mutex);

    memset(&msg->o, 0, sizeof(msg->o) );
    msg->o.ret_val = status;
    msg->o.nbytes = nbytes;
    // Must lock back attr mutex when we return
    attr_lock();
    if (status == EOK) {
        return(_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + nbytes));
    }

    return status;
}

/**
 * Creates our resource manager.
 *
 * @return EOK on success, failure code on error.
 */
int ipu4_resmgr_init(void)
{
    thread_pool_attr_t          pool_attr;
    resmgr_attr_t               resmgr_attr;
    int                         i, err = EOK;
    ipu4_resmgr_ocb_t*          prev_ocb;
    pthread_condattr_t          cattr;

    // Initialize our resource manager mutex
    err = pthread_mutex_init(&resmgr_state.rm_mutex, NULL);
    if (err != EOK) {
        LOG_ERROR("Failed to initialize our rm mutex: err = %d", err);
        return err;
    }

    // Initialize our OCB mutex
    err = pthread_mutex_init(&resmgr_state.ocb_mutex, NULL);
    if (err != EOK) {
        LOG_ERROR("Failed to initialize our OCB mutex: err = %d", err);
        goto init_cleanup_rm_mutex;
    }

    err = pthread_condattr_init(&cattr);
    if (err != EOK) {
        LOG_ERROR("Failed to initialize conditional attribute: err = %d", err);
        goto init_cleanup_ocb_mutex;
    }
    err = pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    if (err != EOK) {
        LOG_ERROR("Failed to set clock for condvar: err = %d", err);
        goto init_cleanup_cattr;
    }
    err = pthread_cond_init(&resmgr_state.shutdown_condvar, &cattr);
    if (err != EOK) {
        LOG_ERROR("Failed to initialize condvar: err = %d", err);
        goto init_cleanup_shutdown_condvar;
    }

    // Setup our resource manager directory entry
    resmgr_state.dispatch = dispatch_create();
    if (resmgr_state.dispatch) {
        // Create our thread pool
        memset(&pool_attr, 0, sizeof(pool_attr));
        pool_attr.handle = resmgr_state.dispatch;
        pool_attr.context_alloc = dispatch_context_alloc;
        pool_attr.block_func = dispatch_block;
        pool_attr.unblock_func = dispatch_unblock;
        pool_attr.handler_func = dispatch_handler;
        pool_attr.context_free = dispatch_context_free;
        pool_attr.lo_water = 1;
        pool_attr.hi_water = 4;
        pool_attr.increment = 1;
        pool_attr.maximum = IPU4_CSI2_NUMPORTS;
        pool_attr.tid_name = "Ipu4ResMgr";
        resmgr_state.thread_pool = thread_pool_create(&pool_attr, 0);
        if (resmgr_state.thread_pool) {
            // init device attributes
            memset(&resmgr_state.root_attr, 0, sizeof(resmgr_state.root_attr));
            iofunc_attr_init(&resmgr_state.root_attr, S_IFDIR | 0777, 0, 0);

            // setup functions to default and override the ones we want to implement
            iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &resmgr_state.connect_funcs,
                             _RESMGR_IO_NFUNCS, &resmgr_state.io_funcs);
            resmgr_state.io_funcs.devctl = ipu4_io_devctl;

            // create mountpoint to permit override of OCB functions
            memset(&resmgr_state.mount_funcs, 0, sizeof(resmgr_state.mount_funcs));
            resmgr_state.mount_funcs.nfuncs = _IOFUNC_NFUNCS;
            resmgr_state.mount_funcs.ocb_calloc = ipu4_ocb_calloc;
            resmgr_state.mount_funcs.ocb_free = ipu4_ocb_free;
            memset(&resmgr_state.mount, 0, sizeof(resmgr_state.mount));
            resmgr_state.mount.funcs = &resmgr_state.mount_funcs;
            resmgr_state.root_attr.mount = &resmgr_state.mount;

            // connect our resource manager
            memset(&resmgr_attr, 0, sizeof(resmgr_attr));
            resmgr_attr.msg_max_size = DCMD_IPU4_MAX_MSG_SIZE;
            resmgr_attr.nparts_max = 1;
            resmgr_state.resmgr_id = resmgr_attach(resmgr_state.dispatch,
                                                   &resmgr_attr,
                                                   IPU4_DEV_PATH,
                                                   _FTYPE_ANY,
                                                   _RESMGR_FLAG_DIR,
                                                   &resmgr_state.connect_funcs,
                                                   &resmgr_state.io_funcs,
                                                   &resmgr_state.root_attr);
            if (resmgr_state.resmgr_id == -1) {
                err = errno;
                LOG_ERROR("Failed to attach resmgr: err = %d", err);
                thread_pool_destroy(resmgr_state.thread_pool);
                resmgr_state.thread_pool = NULL;
            }
        } else {
            err = errno;
            LOG_ERROR("Failed to create our thread pool: err = %d", err);
        }
        // Cleanup on failure
        if (err != EOK) {
            dispatch_destroy(resmgr_state.dispatch);
            resmgr_state.dispatch = NULL;
        }
    } else {
        LOG_ERROR("Failed to create dispatch layer");
        err = ENOMEM;
    }

    // Early exit if failed
    if (err != EOK) {
        goto init_cleanup_shutdown_condvar;
    }

    pthread_condattr_destroy(&cattr);

    // Setup our free list of OCB - they are all free on start
    resmgr_state.ocb_free_list = resmgr_state.ocb;
    prev_ocb = NULL;
    for (i = 0; i < IPU4_CSI2_NUMPORTS; i++) {
        if (prev_ocb) {
            prev_ocb->next = &resmgr_state.ocb[i];
        }
        prev_ocb = &resmgr_state.ocb[i];
    }
    // NULL terminate the linked list
    if (prev_ocb) {
        prev_ocb->next = NULL;
    }
    resmgr_state.ocb_in_use_list = NULL;

    return EOK;

init_cleanup_shutdown_condvar:
    pthread_cond_destroy(&resmgr_state.shutdown_condvar);

init_cleanup_cattr:
    pthread_condattr_destroy(&cattr);

init_cleanup_ocb_mutex:
    pthread_mutex_destroy(&resmgr_state.ocb_mutex);

init_cleanup_rm_mutex:
    pthread_mutex_destroy(&resmgr_state.rm_mutex);

    return err;
}

/**
 * Closes our resource manager.
 *
 * @return EOK on success, failure code on error.
 */
int ipu4_resmgr_deinit(void)
{
    int err = EOK;
    int rc = EOK;

    struct timespec wait_timeout;

    // Wait for ongoing processing to end
    pthread_mutex_lock(&resmgr_state.rm_mutex);

    // Flag everyone that we are now shutting down and stop servicing new requests
    resmgr_state.ipu4drv_shutting_down = true;

    clock_gettime(CLOCK_MONOTONIC, &wait_timeout);
    wait_timeout.tv_sec += IPU4_SHUTDOWN_WAIT_TIMEOUT_SEC;

    // Wait for tasks to finish. Force deinit in case timed-out.
    while ((resmgr_state.active_count > 0) && (err != ETIMEDOUT)) {
        err = pthread_cond_timedwait(&resmgr_state.shutdown_condvar,
                                     &resmgr_state.rm_mutex,
                                     &wait_timeout);
    }
    pthread_mutex_unlock(&resmgr_state.rm_mutex);

    close_all_streams();

    // IPU4 driver de-init
    if (resmgr_state.ipu4drv_initialized) {
        rc = ipu4int_destroy();
        if (rc != EOK) {
            LOG_ERROR("Failed to de-init IPU4 driver");
        }
    }

    // Destroys our thread pool
    err = thread_pool_destroy(resmgr_state.thread_pool);
    if (err == -1) {
        rc = EIO;
        LOG_ERROR("Failed to thread_pool_destroy");
    }

    // Detach our resource manager
    err = resmgr_detach(resmgr_state.dispatch, resmgr_state.resmgr_id, _RESMGR_DETACH_ALL);
    if (err == -1) {
       rc = errno;
       LOG_ERROR("Failed to do resmgr_detach: err = %d", rc);
    }
    resmgr_state.resmgr_id = -1;

    // Destroy our dispatcher
    err = dispatch_destroy(resmgr_state.dispatch);
    if (err == -1) {
        rc = errno;
        LOG_ERROR("Failed to do dispatch_destroy: err = %d", rc);
    }

    // Destroy our OCB mutex
    err = pthread_mutex_destroy(&resmgr_state.ocb_mutex);
    if (err != EOK) {
        rc = err;
        LOG_ERROR("Failed to destroy our OCB mutex: err = %d", err);
    }

    // Destroy our rm mutex
    err = pthread_mutex_destroy(&resmgr_state.rm_mutex);
    if (err != EOK) {
        rc = err;
        LOG_ERROR("Failed to destroy our rm mutex: err = %d", err);
    }

    return rc;
}

/**
 * Starts running our resource manager.
 *
 * @return EOK on success, failure code on error.
 */
int ipu4_resmgr_start(void)
{
    int err;

    // Ensure our thread pool was created successfully
    if (resmgr_state.thread_pool == NULL) {
        LOG_ERROR("Need a valid thread pool to start");
        return ENOENT;
    }

    // Start our thread pool - it will creates its own thread to handle it
    err = thread_pool_start(resmgr_state.thread_pool);
    if (err == -1) {
        LOG_ERROR("Failed to start our thread pool");
        return EIO;
    }
    return EOK;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/ipu4/ipu4_resmgr.c $ $Rev: 871034 $")
#endif
