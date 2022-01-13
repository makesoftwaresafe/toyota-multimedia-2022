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

#include <stdlib.h>
#include <pthread.h>
#include <linux/qnx.h>
#include <linux/linux.h>

#include <media/v4l2-ioctl.h>
#include <media/media-device.h>
#include "intel-ipu4.h"
#include "intel-ipu4-bus.h"
#include "intel-ipu4-mmu.h"
#include "intel-ipu4-buttress.h"
#include "intel-ipu4-dma.h"
#include "intel-ipu4-isys-queue.h"
#include "intel/ipu4/intel-ipu4-interface.h"
#include "lib2600psys/libcsspsys2600.h"
#include "intel-ipu4-psys.h"
#include "libintel-ipu4.h"
#include "intel-ipu4-isys-subdev.h"
#include "intel-ipu4-isys-video.h"
#include "qnxwrapper.h"
#include "vcapture/capture.h"

// Define the following to unit test SOF/EOF detection scheme
//#define TEST_SOF_EOF_DETECTION              1

// If TEST_SOF_EOF_DETECTION is defined, it will generate such an event after
// the following number of frames have been acquired
#define SOF_EOF_NUM_FRAMES                  600

// We support as many streams has are supported by CSI2_BE_SOC
#define MAX_IPU4_STREAMS                    NR_OF_CSI2_BE_SOC_SOURCE_PADS

// Our internal tracking of buffers
typedef struct buffer_node {
    struct buffer_node                      *next;
    void*                                   data;
    struct intel_ipu4_isys_video_buffer     isys;
    enum vb2_buffer_state                   state;
    struct sg_table                         sgt;
} buffer_info_t;

// Structure associated with user handle
typedef struct {
    // Stream has been opened on this port
    bool                            in_use;
    // Stream has been started
    bool                            stream_active;
    // User is waiting to receive a buffer from the driver
    bool                            waiting_for_buffer;
    // SOF/EOF mismatch detected on the stream
    bool                            sof_eof_mismatch;
    // What stream does this represent (index into ipu4_clients array)
    uint8_t                         stream;
    // CSI2 Virtual Channel associated with this stream
    uint8_t                         vc;
    // Port associated with this stream
    uint32_t                        port;
    // Convenience pointers to IPU4 driver
    struct intel_ipu4_isys          *isys;
    struct vb2_queue                *csi2_queue;
    struct vb2_queue                *csi2be_queue;
    // linked-list of buffers that belong to user that we have seen before
    buffer_info_t                   *free_buffers;
    // linked-list of buffers that were queued to driver
    buffer_info_t                   *busy_buffers;
    // linked-list of buffers that were returned from driver but not yet to user
    buffer_info_t                   *done_buffers;
    // Count of how many different buffers have been fed
    uint32_t                        buf_index;
    // Size of each buffer in bytes
    uint32_t                        buf_size;
    // Mutex and condvar to control access to buffers
    pthread_mutex_t                 mutex;
    pthread_cond_t                  cond;
} client_state_t;

// PCI structure for the driver: ipu4_pci_dev permits us access to state of driver
static struct pci_dev               ipu4_pci_dev;

// State of each client - up to maximum number of streams
static client_state_t               ipu4_clients[MAX_IPU4_STREAMS];
static int                          active_stream_cnt;
// Mutex to control access to above active_stream_cnt and ipu4_clients structures
static pthread_mutex_t              ipu4_clients_mutex;

#ifdef TEST_SOF_EOF_DETECTION
static uint32_t                     sofEofFrameCount;
#endif

// Map a buffer that we have yet to see into the address space of iommu;
// dma is set by this function, others are input parameters
static int map_user_buffer(struct device *dev, buffer_info_t* buf, off_t paddr, size_t size,
                           dma_addr_t* dma)
{
    int                         err;
    int                         n_pages;

    // Sanity checks
    if ( (buf == NULL) || (dma == NULL) ) {
        qnx_error("map_user_buffer: NULL buffers provided");
        return EINVAL;
    }
    if ( (intel_ipu4_dma_ops.get_sgtable == NULL) || (intel_ipu4_dma_ops.map_sg == NULL) ) {
        qnx_error("map_user_buffer: NULL dma ops");
        return EIO;
    }

    // Get an sg table that describes our buffer
    err = intel_ipu4_dma_ops.get_sgtable(dev, &buf->sgt, buf->data, *dma, size, NULL);
    if (err != 0) {
        qnx_error("map_user_buffer: err %d in call to get_sgtable", err);
        return EIO;
    }

    // Map our sg to iommu
    n_pages = intel_ipu4_dma_ops.map_sg(dev, buf->sgt.sgl, buf->sgt.nents, DMA_FROM_DEVICE, NULL);
    if (n_pages != buf->sgt.nents) {
        qnx_error("map_user_buffer: call to map_sg returned %d for %d nents", n_pages, buf->sgt.nents);
        return ENOMEM;
    }

    // Set the dma addr
    *dma = sg_dma_address(buf->sgt.sgl);

    return EOK;
}

// Unmap a user buffer when it is no longer necessary
static int unmap_user_buffer(struct device *dev, buffer_info_t* buf)
{
    struct scatterlist          *sg;
    struct page                 *page;
    int                         i;

    // Sanity checks
    if (buf == NULL) {
        qnx_error("unmap_user_buffer: NULL buffer provided");
        return EINVAL;
    }
    if (intel_ipu4_dma_ops.unmap_sg == NULL) {
        qnx_error("unmap_user_buffer: NULL unmap_sg");
        return EIO;
    }

    // Unmap the buffer
    intel_ipu4_dma_ops.unmap_sg(dev, buf->sgt.sgl, buf->sgt.nents, DMA_FROM_DEVICE, NULL);

    // Cleanup sg table - pages and table itself
    for_each_sg (buf->sgt.sgl, sg, buf->sgt.nents, i) {
        page = sg_page(sg);
        free(page);
    }
    sg_free_table(&buf->sgt);

    return EOK;
}

// Utility function to free buffers from a linked list
static void free_stream_buffers(buffer_info_t* buf, struct vb2_queue* queue,
                                client_state_t* state)
{
    buffer_info_t                               *cur_buf;

    while (buf) {
        cur_buf = buf;
        buf = buf->next;
        queue->ops->buf_cleanup(&cur_buf->isys.vb);
        qnxw_free_vb2_plane_info(&cur_buf->isys.vb, 0);
        unmap_user_buffer(&state->isys->adev->dev, cur_buf);
        free(cur_buf);
    }
}

// cleanup all of our queued buffers
static void cleanup_queues(client_state_t* state)
{
    struct vb2_queue                            *queue;

    // Sanity check
    if (state == NULL) {
        return;
    }

    // Free up all buffers in our linked lists
    queue = state->csi2_queue;
    free_stream_buffers(state->free_buffers, queue, state);
    state->free_buffers = NULL;

    free_stream_buffers(state->busy_buffers, queue, state);
    state->busy_buffers = NULL;

    free_stream_buffers(state->done_buffers, queue, state);
    state->done_buffers = NULL;
}

// Power cycle iommu for ISYS + PSYS
static int power_cycle_iommus(struct intel_ipu4_device* isp)
{
    int rval;
    int err = EOK;

    if (isp->isys_iommu && isp->isys_iommu->ctrl) {
        // we power it off then on as this is not done when no power management is used
        rval = intel_ipu4_buttress_power(&isp->isys->dev, isp->isys_iommu->ctrl, false);
        if (!rval) {
            rval = intel_ipu4_buttress_power(&isp->isys->dev, isp->isys_iommu->ctrl, true);
        }
        if (rval) {
            qnx_error("Failed buttress power up/down %d",rval);
            err = EIO;
        }
    } else {
        qnx_error("has no isys buttress control info");
        err = EIO;
    }
    if (err != EOK) {
        return err;
    }

    if (isp->psys_iommu && isp->psys_iommu->ctrl) {
        // we power it off then on as this is not done when no power management is used
        rval = intel_ipu4_buttress_power(&isp->psys->dev, isp->psys_iommu->ctrl, false);
        if (!rval) {
            rval = intel_ipu4_buttress_power(&isp->psys->dev, isp->psys_iommu->ctrl, true);
        }
        if (rval) {
            qnx_error("Failed buttress power up/down %d",rval);
            err = EIO;
        }
    } else {
        qnx_error("has no psys buttress control info");
        err = EIO;
    }
    if (err != EOK) {
        // Power back off ISYS IOMMU on failure
        intel_ipu4_buttress_power(&isp->isys->dev, isp->isys_iommu->ctrl, false);
    }
    return err;
}

// Cleanup of ISYS
static int cleanup_isys(struct intel_ipu4_device* isp, bool cleanupIsys)
{
    int rval;
    int err = EOK;

    intel_ipu4_bus_unregister_driver(&intel_ipu4_mmu_driver);
    if ( (isp->isys_iommu->dev.bus == NULL) || (isp->isys_iommu->dev.bus->remove == NULL) ) {
        qnx_error("Unexpected NULL bus or bus->remove");
        err = EIO;
    } else {
        rval = isp->isys_iommu->dev.bus->remove(&isp->isys_iommu->dev);
        if (rval != 0) {
            qnx_error("Error in calling isys_iommu remove: err = %d", rval);
            err = EIO;
        }
    }

    // Deinitialize ISYS if required - this will cleanup its sub-devices as well
    if (cleanupIsys) {
        intel_ipu4_bus_unregister_driver(&isys_driver);
        if ( (isp->isys->dev.bus == NULL) || (isp->isys->dev.bus->remove == NULL) ) {
            qnx_error("Unexpected NULL bus or bus->remove");
            err = EIO;
        } else {
            rval = isp->isys->dev.bus->remove(&isp->isys->dev);
            if (rval != 0) {
                qnx_error("Error in calling isys remove: err = %d", rval);
                err = EIO;
            }
        }
        libipu4_library_exit();
    }
    return err;
}

// Initialization of ISYS
static int init_isys(struct intel_ipu4_device* isp)
{
    int err = EOK;
    bool cleanupIsys = false;

    // Sanity check
    if (isp->isys_iommu == NULL) {
        qnx_error("Unexpected NULL isys_iommu");
        return EIO;
    }
    if ( (isp->isys_iommu->dev.bus == NULL) || (isp->isys_iommu->dev.bus->probe == NULL) ||
         (isp->isys_iommu->dev.bus->pm == NULL) || (isp->isys_iommu->dev.bus->pm->runtime_suspend == NULL) ||
         (isp->isys_iommu->dev.bus->pm->runtime_resume == NULL) ) {
        qnx_error("Unexpected NULL for isys_iommu");
        return EIO;
    }
    if (isp->isys == NULL) {
        qnx_error("Unexpected NULL isys");
        return EIO;
    }
    if ( (isp->isys->dev.bus == NULL) || (isp->isys->dev.bus->probe == NULL) ||
         (isp->isys->dev.bus->pm == NULL) || (isp->isys->dev.bus->pm->runtime_suspend == NULL) ||
         (isp->isys->dev.bus->pm->runtime_resume == NULL) ) {
        qnx_error("Unexpected NULL for isys");
        return EIO;
    }

    // Initialize ISYS MMU
    intel_ipu4_bus_register_driver(&intel_ipu4_mmu_driver);
    isp->isys_iommu->dev.driver = &intel_ipu4_mmu_driver.drv;
    err = isp->isys_iommu->dev.bus->probe(&isp->isys_iommu->dev);
    if (err != 0) {
        qnx_error("Error in calling isys_iommu probe: err = %d", err);
        intel_ipu4_bus_unregister_driver(&intel_ipu4_mmu_driver);
        return -err;
    }

    // Initialize ISYS - it will initialize all of its sub-devices as well
    intel_ipu4_bus_register_driver(&isys_driver);
    // Init to use IPU4 ISYS library
    err = libipu4_library_init();
    if (err != EOK) {
        qnx_error("Failed to init ISYS library");
        err = EIO;
    } else {
        isp->isys->dev.driver = &isys_driver.drv;
        err = isp->isys->dev.bus->probe(&isp->isys->dev);
        if (err != 0) {
            qnx_error("Error in calling isys probe: err = %d", err);
            err = -err;
            libipu4_library_exit();
        }
    }
    if (err != EOK) {
        intel_ipu4_bus_unregister_driver(&isys_driver);
    }

    if (err == EOK) {
        cleanupIsys = true;
        // suspend HW running for ISYS
        err = pm_generic_runtime_suspend(&isp->isys->dev);
        if (err != 0) {
            qnx_error("Error in calling isys runtime_suspend: err = %d", err);
            err = -err;
        } else {
            err = isp->isys_iommu->dev.bus->pm->runtime_suspend(&isp->isys_iommu->dev);
            if (err != 0) {
                qnx_error("Error in calling iommu runtime_suspend: err = %d", err);
                err = -err;
            }
        }
    }
    // Cleanup ISYS IOMMU + ISYS on failure
    if (err != EOK) {
        cleanup_isys(isp, cleanupIsys);
    }

    return err;
}

// Cleanup of PSYS
static int cleanup_psys(struct intel_ipu4_device* isp, bool cleanupPsys)
{
    int rval;
    int err = EOK;

    // Deinitialize PSYS MMU
    if ( (isp->psys_iommu->dev.bus == NULL) || (isp->psys_iommu->dev.bus->remove == NULL) ) {
        qnx_error("Unexpected psys NULL bus or bus->remove");
        err = EIO;
    } else {
        rval = isp->psys_iommu->dev.bus->remove(&isp->psys_iommu->dev);
        if (rval != 0) {
            qnx_error("Error in calling psys_iommu remove: err = %d", rval);
            err = EIO;
        }
    }

    // Deinitialize PSYS
    if (cleanupPsys) {
        intel_ipu4_psys_exit();
        if ( (isp->psys->dev.bus == NULL) || (isp->psys->dev.bus->remove == NULL) ) {
            qnx_error("Unexpected psys NULL bus or bus->remove");
            err = EIO;
        } else {
            rval = isp->psys->dev.bus->remove(&isp->psys->dev);
            if (rval != 0) {
                qnx_error("Error in calling isys remove: err = %d", rval);
                err = EIO;
            }
        }
        libcsspsys2600_exit();
    }
    return err;
}

// Initialization of PSYS
static int init_psys(struct intel_ipu4_device* isp)
{
    int err = EOK;
    bool cleanupPsys = false;

    // Sanity check
    if (isp->psys_iommu == NULL) {
        qnx_error("Unexpected NULL psys_iommu");
        return EIO;
    }
    if ( (isp->psys_iommu->dev.bus == NULL) || (isp->psys_iommu->dev.bus->probe == NULL) ||
         (isp->psys_iommu->dev.bus->pm == NULL) || (isp->psys_iommu->dev.bus->pm->runtime_suspend == NULL) ||
         (isp->psys_iommu->dev.bus->pm->runtime_resume == NULL)) {
        qnx_error("Unexpected NULL for psys_iommu");
        return EIO;
    }
    if (isp->psys == NULL) {
        qnx_error("Unexpected NULL psys");
        return EIO;
    }
    if ( (isp->psys->dev.bus == NULL) || (isp->psys->dev.bus->probe == NULL) ||
         (isp->psys->dev.bus->pm == NULL) || (isp->psys->dev.bus->pm->runtime_suspend == NULL) ||
         (isp->psys->dev.bus->pm->runtime_resume == NULL)) {
        qnx_error("Unexpected NULL for psys");
        return EIO;
    }

    // Initialize PSYS MMU
    isp->psys_iommu->dev.driver = &intel_ipu4_mmu_driver.drv;
    err = isp->psys_iommu->dev.bus->probe(&isp->psys_iommu->dev);
    if (err != 0) {
        qnx_error("Error in calling psys_iommu probe: err = %d", err);
        return -err;
    }

    // Initialize PSYS
    err = intel_ipu4_psys_init();
    if (err != 0) {
        qnx_error("Failed in call to intel_ipu4_psys_init: err = %d", err);
        err = -err;
    } else {
        isp->psys->dev.driver = &intel_ipu4_psys_driver.drv;
        err = libcsspsys2600_init();
        if (err != 0) {
            qnx_error("Error in calling psys2600_init: err = %d", err);
            err = -err;
        } else {
            err = isp->psys->dev.bus->probe(&isp->psys->dev);
            if (err != 0) {
                qnx_error("Error in calling psys probe: err = %d", err);
                err = -err;
                libcsspsys2600_exit();
            }
        }
        if (err != EOK) {
            intel_ipu4_psys_exit();
        }
    }

    if (err == EOK) {
        cleanupPsys = true;
        // suspend HW running for PSYS
        err = pm_generic_runtime_suspend(&isp->psys->dev);
        if (err != 0) {
            qnx_error("Error in calling psys runtime_suspend: err = %d", err);
            err = -err;
        } else {
            err = isp->psys_iommu->dev.bus->pm->runtime_suspend(&isp->psys_iommu->dev);
            if (err != 0) {
                qnx_error("Error in calling psys iommu runtime_suspend: err = %d", err);
                err = -err;
            }
        }
    }
    // Cleanup PSYS IOMMU and PSYS (if necessary) on failure
    if (err != EOK) {
        cleanup_psys(isp, cleanupPsys);
    }
    return err;
}

// Cleanup of overall IPU4 driver
static int cleanup_ipu4(struct intel_ipu4_device* isp)
{
    int rval;
    int err = EOK;

    // Power off buttress - must be done before IPU4 driver exits
    if (isp->isys_iommu->ctrl) {
        rval = intel_ipu4_buttress_power(&isp->isys->dev, isp->isys_iommu->ctrl, false);
        if (rval != EOK) {
            err = EIO;
        }
    }
    if (isp->psys_iommu->ctrl) {
        rval = intel_ipu4_buttress_power(&isp->psys->dev, isp->psys_iommu->ctrl, false);
        if (rval != EOK) {
            err = EIO;
        }
    }

    // Deinit IPU4 driver
    rval = intel_ipu4_exit(&ipu4_pci_dev);
    if (rval != EOK) {
        qnx_error("Failed in call to intel_ipu4_exit: err = %d", rval);
        err = EIO;
    }

    // Free memory allocated by driver using v4l2_ctrl_new_custom() or devm_kmalloc()
    // which in turn is called by devm_kzalloc() and devm_kcalloc();
    if (isp->isys) {
        if (isp->isys->dev.driver_data) {
            free(isp->isys->dev.driver_data);
        }
        if (isp->isys->pdata) {
            free(isp->isys->pdata);
        }
        free(isp->isys);
    }
    if (isp->isys_iommu) {
        if (isp->isys_iommu->dev.driver_data) {
            free(isp->isys_iommu->dev.driver_data);
        }
        if (isp->isys_iommu->pdata) {
            free(isp->isys_iommu->pdata);
        }
        if (isp->isys_iommu->ctrl) {
            free(isp->isys_iommu->ctrl);
        }
        free(isp->isys_iommu);
    }
    if (isp->psys) {
        if (isp->psys->dev.driver_data) {
            free(isp->psys->dev.driver_data);
        }
        if (isp->psys->pdata) {
            free(isp->psys->pdata);
        }
        free(isp->psys);
    }
    if (isp->psys_iommu) {
        if (isp->psys_iommu->dev.driver_data) {
            free(isp->psys_iommu->dev.driver_data);
        }
        if (isp->psys_iommu->pdata) {
            free(isp->psys_iommu->pdata);
        }
        if (isp->psys_iommu->ctrl) {
            free(isp->psys_iommu->ctrl);
        }
        free(isp->psys_iommu);
    }
    free(isp);

    // Free memory allocated for PCI bus
    free(ipu4_pci_dev.bus);
    ipu4_pci_dev.bus = NULL;

    // Destroy mutex
    pthread_mutex_destroy(&ipu4_clients_mutex);

    // Destroy wrapper now that we are done
    rval = qnxw_destroy();
    if (rval != EOK) {
        qnx_error("Failed to destroy our wrapper: err = %d", rval);
        err = rval;
    }
    return err;
}

int ipu4int_initialize(ipu4int_csi2_config_t* config)
{
    int                                 i;
    int                                 err;
    struct intel_ipu4_device            *isp;
    struct intel_ipu4_isys_csi2_config  csi2;

    // Init wrapper before first use
    err = qnxw_init();
    if (err != EOK) {
        qnx_error("Failed to initialize our wrapper: err = %d", err);
        return err;
    }

    // Apply the CSI2 config
    for (i = 0; i < IPU4INT_CSI2_NUMPORTS; i++) {
        csi2.nlanes = config[i].num_lanes;
        csi2.port = i;
        err = qnxw_set_csi2_config(i, &csi2);
        if (err != EOK) {
            qnx_error("Failed to set csi2 config for port %d: err = %d", i, err);
            break;
        }
    }
    if (err == EOK) {
        err = pthread_mutex_init(&ipu4_clients_mutex, NULL);
        if (err != EOK) {
            qnx_error("Failed to initialize ipu4_clients_mutex: err = %d", err);
        } else {
            // Availability of secure mode depends on hardware revision; will only log
            // a warning if this does not match what the hardware requires
            secure_mode_enable = false;

            // Init IPU4 driver
            ipu4_pci_dev.bus = calloc(1, sizeof(struct pci_bus));
            if (ipu4_pci_dev.bus == NULL) {
                qnx_error("Failed to allocate pci bus structure");
                err = ENOMEM;
            } else {
                err = intel_ipu4_init(&ipu4_pci_dev);
                if (err != EOK) {
                    qnx_error("Failed in call to intel_ipu4_init: err = %d", err);
                    err = -err;
                    free(ipu4_pci_dev.bus);
                    ipu4_pci_dev.bus = NULL;
                }
            }
            if (err != EOK) {
                pthread_mutex_destroy(&ipu4_clients_mutex);
            }
        }
    }
    if (err != EOK) {
        qnxw_destroy();
        return err;
    }

    // Get pointer to ipu4 structure created above
    isp = (struct intel_ipu4_device*) pci_get_drvdata(&ipu4_pci_dev);
    if (isp == NULL) {
        qnx_error("Unexpected NULL isp");
        err = EIO;
    } else {
        err = power_cycle_iommus(isp);
        if (err == EOK) {
            err = init_isys(isp);
            if (err == EOK) {
                err = init_psys(isp);
                if (err != EOK) {
                    cleanup_isys(isp, true);
                }
            }
        }
    }

    // Proper cleanup of IPU4 on failure
    if (err != EOK) {
        cleanup_ipu4(isp);
    } else {
        // All done successfully
        initAddrTableFlag = false;
        active_stream_cnt = 0;
    }
    return err;
}

int ipu4int_destroy(void)
{
    int                                 err = EOK;
    int                                 rval;
    struct intel_ipu4_device            *isp;

    pthread_mutex_destroy(&ipu4_clients_mutex);

    // Get pointer to ipu4 structure that should exist as not explicitly deleted
    isp = (struct intel_ipu4_device*) pci_get_drvdata(&ipu4_pci_dev);
    if (isp == NULL) {
        qnx_error("Unexpected NULL isp");
        return EIO;
    }

    // Cleanup ISYS MMU + ISYS
    err = cleanup_isys(isp, true);

    // Cleanup PSYS MMU + PSYS
    rval = cleanup_psys(isp, true);
    if (rval != EOK) {
        err = rval;
    }

    // Cleanup overall IPU4 driver
    rval = cleanup_ipu4(isp);
    if (rval != EOK) {
        err = rval;
    }
    return err;
}

// Assumes that caller ensures no other API's called during this reset
int ipu4int_reset(void)
{
    client_state_t                      *state;
    int                                 i;
    int                                 err;
    int                                 rc = EOK;
    struct intel_ipu4_isys_csi2_config* csi2;
    ipu4int_csi2_config_t               config[IPU4INT_CSI2_NUMPORTS];

    // Tell wrapper that the reset is in progress to speed up latency for stop
    // streaming and close.
    qnxw_set_ipu4_reset(true);

    // Close the stream for all in-use streams
    for (i = 0; i < MAX_IPU4_STREAMS; i++) {
        state = &ipu4_clients[i];
        if (state->in_use) {
            err = ipu4int_close_stream(state);
            if (err != EOK) {
                rc = err;
            }
        }
    }

    // Destroy the interface to finish cleanup
    err = ipu4int_destroy();
    if (err != EOK) {
        rc = err;
    }

    // Get CSI2 configuration required for initialization from our wrapper
    for (i = 0; i < IPU4INT_CSI2_NUMPORTS; i++) {
        csi2 = qnxw_get_csi2_config(i);
        if (csi2) {
            config[i].num_lanes = csi2->nlanes;
        } else {
            qnx_error("Unexpected NULL csi2 config for port %d", i);
            rc = EFAULT;
        }
    }

    // Initialize the driver for use
    err = ipu4int_initialize(config);
    if (err != EOK) {
        rc = err;
    }

    // Tell wrapper that reset is complete
    qnxw_set_ipu4_reset(false);

    return rc;
}

int ipu4int_set_max_streams(uint32_t num)
{
    // Sanity check
    if (num == 0) {
        qnx_error("Need to support at least one stream");
        return EINVAL;
    }
    if (num > INTEL_IPU4_ISYS_NUM_STREAMS_B0) {
        qnx_error("Only support up to %d streams", INTEL_IPU4_ISYS_NUM_STREAMS_B0);
        return EINVAL;
    }

    // Set the value that will be used by video_open()
    num_stream_support = num;
    return EOK;
}

int ipu4int_open_stream(ipu4int_handle_t* handle, ipu4int_csi2_port_t port,
                        ipu4int_csi2_vc_t chan)
{
    struct intel_ipu4_device            *isp;
    struct intel_ipu4_isys              *isys;
    struct file                         file;
    struct vb2_queue                    *csi2_queue;
    struct vb2_queue                    *csi2be_queue;
    pthread_condattr_t                  attr;
    int                                 err;
    int                                 i;
    client_state_t                      *state;
    uint8_t                             stream;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (port >= IPU4INT_CSI2_NUMPORTS) {
        qnx_error("Invalid port of %d", port);
        return EINVAL;
    }
    if (chan >= IPU4INT_CSI2_NUMVCS) {
        qnx_error("Invalid vc of %d", chan);
        return EINVAL;
    }

    pthread_mutex_lock(&ipu4_clients_mutex);

    // Ensure no other streams using same port + chan
    for (i = 0; i < MAX_IPU4_STREAMS; i++) {
        if ( (ipu4_clients[i].in_use) && (ipu4_clients[i].port == port) &&
             (ipu4_clients[i].vc == chan) ) {
            qnx_error("Port %d VC %d is already in use", port, chan);
            pthread_mutex_unlock(&ipu4_clients_mutex);
            return EBUSY;
        }
    }
    // Find available stream if any
    for (i = 0; i < MAX_IPU4_STREAMS; i++) {
        if (ipu4_clients[i].in_use == false) {
            break;
        }
    }
    if (i >= MAX_IPU4_STREAMS) {
        qnx_error("All streams are already in use - max of %d streams", MAX_IPU4_STREAMS);
        pthread_mutex_unlock(&ipu4_clients_mutex);
        return EBUSY;
    }
    state = &ipu4_clients[i];
    stream = i;
    qnxw_set_csi2_stream_info(stream, port, chan);

    // Need to call video_open() to initialize the library and load firmware
    // Note: OK to only do for CSI2 as only first open as any effect
    isp = (struct intel_ipu4_device*) pci_get_drvdata(&ipu4_pci_dev);
    if (isp == NULL) {
        qnx_error("Unexpected NULL isp");
        pthread_mutex_unlock(&ipu4_clients_mutex);
        return EIO;
    }
    if (isp->isys == NULL) {
        qnx_error("Unexpected NULL isp->isys");
        pthread_mutex_unlock(&ipu4_clients_mutex);
        return EIO;
    }

    if (!active_stream_cnt) {
        // resume HW running
        err = isp->isys_iommu->dev.bus->pm->runtime_resume(&isp->isys_iommu->dev);
        if (err == 0) {
            err = pm_generic_runtime_resume(&isp->isys->dev);
            if (err == 0) {
                err = isp->psys_iommu->dev.bus->pm->runtime_resume(&isp->psys_iommu->dev);
                if (err == 0) {
                    err = pm_generic_runtime_resume(&isp->psys->dev);
                    if (err == 0) {
                    } else {
                        qnx_error("Error in calling psys runtime_resume: err = %d", err);
                        err = EIO;
                    }
                    if (err != 0) {
                        isp->psys_iommu->dev.bus->pm->runtime_suspend(&isp->psys_iommu->dev);
                    }
                } else {
                    qnx_error("Error in calling psys iommu runtime_resume: err = %d", err);
                    err = EIO;
                }
                if (err != 0) {
                    pm_generic_runtime_suspend(&isp->isys->dev);
                }
            } else {
                qnx_error("Error in calling isys runtime_resume: err = %d", err);
                err = EIO;
            }
            if (err != 0) {
                isp->isys_iommu->dev.bus->pm->runtime_suspend(&isp->isys_iommu->dev);
            }
        } else {
            qnx_error("Error in calling isys iommu runtime_resume: err = %d", err);
            err = EIO;
        }
        if (err != 0) {
            pthread_mutex_unlock(&ipu4_clients_mutex);
            return err;
        }
    }
    active_stream_cnt++;

    isys = intel_ipu4_bus_get_drvdata(isp->isys);
    // One CSI2 instance per port, 4 AV instance each to cover each VC
    file.private_data = &isys->csi2[port].av[chan].vdev;
    if ( (isys->csi2[port].av[chan].vdev.fops == NULL) ||
         (isys->csi2[port].av[chan].vdev.fops->open == NULL) ) {
        qnx_error("Unexpected NULL fops or fops->open");
        err = EIO;
        goto out;
    }
    err = isys->csi2[port].av[chan].vdev.fops->open(&file);
    if (err != 0) {
        qnx_error("camera_open on port %d failed with err = %d", port, err);
        goto out;
    }

    // Validate queues that we need
    csi2_queue = &isys->csi2[port].av[chan].aq.vbq;
    // Use different av context for each stream (stream entry into ipu4_clients)
    csi2be_queue = &isys->csi2_be_soc.av[stream].aq.vbq;
    if ( (csi2_queue->ops == NULL) || (csi2_queue->ops->start_streaming == NULL) ||
         (csi2_queue->ops->stop_streaming == NULL) || (csi2_queue->ops->buf_init == NULL) ||
         (csi2_queue->ops->buf_prepare == NULL) || (csi2_queue->ops->buf_finish == NULL) ||
         (csi2_queue->ops->buf_cleanup == NULL) || (csi2_queue->ops->buf_queue == NULL) ) {
        qnx_error("Unexpected NULL csi2 queue ops");
        err = EIO;
        goto out;
    }
    if ( (csi2be_queue->ops == NULL) || (csi2be_queue->ops->start_streaming == NULL) ||
         (csi2be_queue->ops->stop_streaming == NULL) || (csi2be_queue->ops->buf_init == NULL) ||
         (csi2be_queue->ops->buf_prepare == NULL) || (csi2be_queue->ops->buf_finish == NULL) ||
         (csi2be_queue->ops->buf_cleanup == NULL) || (csi2be_queue->ops->buf_queue == NULL) ) {
        qnx_error("Unexpected NULL csi2 queue ops");
        err = EIO;
        goto out;
    }

    // Set VC associated with csi2_be
    isys->csi2_be_soc.av[stream].ip.vc = chan;

    // Fill in state and return handle
    memset(state, 0, sizeof(client_state_t));
    err = pthread_mutex_init(&state->mutex, NULL);
    if (err != EOK) {
        qnx_error("Failed to initialize mutex: err = %d", err);
        goto out;
    }
    err = pthread_condattr_init(&attr);
    if (err != EOK) {
        qnx_error("Failed to initialize condattr: err = %d", err);
    } else {
        err = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
        if (err != EOK) {
            qnx_error("Failed to set clock of condattr: err = %d", err);
        } else {
            err = pthread_cond_init(&state->cond, &attr);
            if (err != EOK) {
                qnx_error("Failed to initialize cond: err = %d", err);
            }
        }
        pthread_condattr_destroy(&attr);
    }
    if (err != EOK) {
        goto out;
    }
    isys->power = 1;
    state->in_use = true;
    state->port = port;
    state->vc = chan;
    state->stream = stream;
    state->isys = isys;
    state->csi2_queue = csi2_queue;
    state->csi2be_queue = csi2be_queue;
    *handle = (ipu4int_handle_t*) state;
    pthread_mutex_unlock(&ipu4_clients_mutex);

    return EOK;

out:
    active_stream_cnt--;
    if (!active_stream_cnt) {
        isp->isys->dev.bus->pm->runtime_suspend(&isp->isys->dev);
        isp->isys_iommu->dev.bus->pm->runtime_suspend(&isp->isys_iommu->dev);
    }
    pthread_mutex_unlock(&ipu4_clients_mutex);
    return err;
}

int ipu4int_close_stream(ipu4int_handle_t handle)
{
    client_state_t                      *state;
    struct intel_ipu4_isys              *isys;
    struct file                         file;
    uint32_t                            port;
    uint8_t                             vc;
    int                                 err;
    int                                 rc = EOK;
    struct intel_ipu4_device            *isp;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    state = (client_state_t*) handle;

    // Ensure it is in use
    if (state->in_use == false) {
        qnx_error("Handle is not valid");
        return ENODEV;
    }

    // Stop stream if it is active - or else still cleanup queues in case any
    // buffers were queued before starting
    if (state->stream_active) {
        ipu4int_stop_stream(handle);
    } else {
        intel_ipu4_isys_queue_flush(state->csi2be_queue);
        cleanup_queues(state);
    }

    // Need to call video_release() to cleanup the library
    if (state->isys == NULL) {
        qnx_error("Unexpected NULL state->isys");
        return EIO;
    }
    isys = state->isys;
    port = state->port;
    vc = state->vc;
    file.private_data = &isys->csi2[port].av[vc].vdev;
    if ( (isys->csi2[port].av[vc].vdev.fops == NULL) ||
         (isys->csi2[port].av[vc].vdev.fops->release == NULL) ) {
        qnx_error("Unexpected NULL fops or fops->release");
        return EIO;
    }
    err = isys->csi2[port].av[vc].vdev.fops->release(&file);
    if (err != 0) {
        qnx_error("camera_release on port %d vc %d failed with err = %d", port, vc, err);
        rc = err;
    }

    // Unregister with the sync start library if any
    qnxw_sync_start_stream_unregister(port);

    // Cleanup mutex/cond
    pthread_cond_destroy(&state->cond);
    pthread_mutex_destroy(&state->mutex);

    pthread_mutex_lock(&ipu4_clients_mutex);

    // Mark the handle as no longer in use
    state->in_use = false;

    if (active_stream_cnt) {
        active_stream_cnt--;
        if (!active_stream_cnt) {
            // suspend HW running since the last active stream has been closed
            isp = isys->adev->isp;
            err = pm_generic_runtime_suspend(&isp->isys->dev);
            if (err != 0) {
                qnx_error("Error in calling isys runtime_suspend: err = %d", err);
                rc = EIO;
            }
            err = isp->isys_iommu->dev.bus->pm->runtime_suspend(&isp->isys_iommu->dev);
            if (err != 0) {
                qnx_error("Error in calling isys iommu runtime_suspend: err = %d", err);
                rc = EIO;
            }
            err = pm_generic_runtime_suspend(&isp->psys->dev);
            if (err != 0) {
                qnx_error("Error in calling psys runtime_suspend: err = %d", err);
                rc = EIO;
            }
            err = isp->psys_iommu->dev.bus->pm->runtime_suspend(&isp->psys_iommu->dev);
            if (err != 0) {
                qnx_error("Error in calling psys iommu runtime_suspend: err = %d", err);
                rc = EIO;
            }
        }
    }
    pthread_mutex_unlock(&ipu4_clients_mutex);

    return rc;
}

int ipu4int_configure_stream(ipu4int_handle_t handle, ipu4int_stream_config_t* config)
{
    int                                         err;
    struct v4l2_mbus_frame_desc_entry           entry;
    client_state_t                              *state;
    struct intel_ipu4_isys                      *isys;
    struct file                                 file;
    struct v4l2_format                          format;
    uint32_t                                    csi2_be_pixelformat;
    uint32_t                                    port;
    uint8_t                                     vc;
    uint8_t                                     stream;

    // Sanity check
    if (config == NULL) {
        qnx_error("NULL config provided");
        return EINVAL;
    }
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    state = (client_state_t*) handle;
    if (state->in_use == false) {
        qnx_error("Inactive handle provided");
        return EINVAL;
    }
    if (state->isys == NULL) {
        qnx_error("Unexpected NULL state->isys");
        return EIO;
    }

    // Set CSI2 frequency
    port = state->port;
    vc = state->vc;
    stream = state->stream;
    err = qnxw_set_csi2_link_frequency(port, config->csi2_frequency);
    if (err != EOK) {
        qnx_error("Failed to set csi2 link frequency for port %d: err = %d", port, err);
        return err;
    }

    // Register the start sync library if any - this will dlopen the library
    if (config->decoder_sync_lib[0] != '\0') {
        struct intel_ipu4_isys_video *av = intel_ipu4_isys_queue_to_video(vb2_queue_to_intel_ipu4_isys_queue(state->csi2be_queue));
        err = qnxw_sync_start_stream_register(state->port, av, config->decoder_sync_lib, config->decoder_sync_data);
        if (err != EOK) {
            qnx_error("Failed to register sync start stream for port %d: err = %d", state->port, err);
            return err;
        }
    }

    // Fill in entry with necessary information - inform our wrapper
    switch (config->input_format) {
    case IPU4INT_FORMAT_CBYCRY:
        entry.pixelcode = MEDIA_BUS_FMT_UYVY8_1X16;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
        break;
    case IPU4INT_FORMAT_YCBYCR:
        entry.pixelcode = MEDIA_BUS_FMT_YUYV8_1X16;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        break;
    case IPU4INT_FORMAT_RGB888:
        entry.pixelcode = MEDIA_BUS_FMT_RGB888_1X24;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
        break;
    default:
        qnx_error("Unsupported input_format %d", config->input_format);
        return EINVAL;
    }
    switch(config->output_format) {
    case IPU4INT_FORMAT_CBYCRY:
        if (config->input_format != IPU4INT_FORMAT_CBYCRY) {
            qnx_error("Invalid output_format %d for input_format %d",
                      config->input_format, config->output_format);
            return EINVAL;
        }
        csi2_be_pixelformat = V4L2_PIX_FMT_UYVY;
        entry.bpp = 16;
        break;
    case IPU4INT_FORMAT_YCBYCR:
        if (config->input_format != IPU4INT_FORMAT_YCBYCR) {
            qnx_error("Invalid output_format %d for input_format %d",
                      config->input_format, config->output_format);
            return EINVAL;
        }
        csi2_be_pixelformat = V4L2_PIX_FMT_YUYV;
        entry.bpp = 16;
        break;
    case IPU4INT_FORMAT_RGB8888:
        if (config->input_format != IPU4INT_FORMAT_RGB888) {
            qnx_error("Invalid output_format %d for input_format %d",
                      config->input_format, config->output_format);
            return EINVAL;
        }
        csi2_be_pixelformat = V4L2_PIX_FMT_XBGR32;
        entry.bpp = 32;
        break;
    default:
        qnx_error("Unsupported output_format %d", config->output_format);
        return EINVAL;
    }
    entry.flags = 0;
    entry.size.two_dim.width = config->width;
    entry.size.two_dim.height = config->height;
    entry.size.two_dim.start_pixel = config->roi_x;
    entry.size.two_dim.start_line = config->roi_y;
    entry.bus.csi2.channel = port;
    // Ignored - it is set in qnxw_get_frame_desc_entry() based on param
    entry.bus.csi2.data_type = 0;

    err = qnxw_set_frame_desc_entry(&entry, port);
    if (err != EOK) {
        qnx_error("Failed to set desc entry: err = %d", err);
        return err;
    }

    // Set V4L2 format using IOCTL for CSI2 + CSI2_BE
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    format.fmt.pix.width = config->width;
    format.fmt.pix.height = config->height;
    format.fmt.pix.field = V4L2_FIELD_NONE;
    format.fmt.pix.bytesperline = config->bytes_per_line;
    format.fmt.pix.sizeimage = config->buffersize;
    state->buf_size = format.fmt.pix.sizeimage;
    format.fmt.pix.flags = 0;
    isys = state->isys;
    file.private_data = &isys->csi2[port].av[vc].vdev;

    err = qnxw_set_source_format(&format, port, state->vc, entry.pixelcode);
    if (err != EOK) {
        qnx_error("Failed to set format: err = %d", err);
        return err;
    }

    // Sanity check on pointers
    if (isys->csi2[port].av[vc].vdev.ioctl_ops == NULL) {
        qnx_error("Unexpected NULL ioctl_ops");
        return EIO;
    }
    if ( (isys->csi2[port].av[vc].aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) &&
         (isys->csi2[port].av[vc].vdev.ioctl_ops->vidioc_s_fmt_vid_cap == NULL) ) {
        qnx_error("Unexpected NULL vidioc_s_fmt_vid_cap");
        return EIO;
    }
    if ( (isys->csi2[port].av[vc].aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) &&
         (isys->csi2[port].av[vc].vdev.ioctl_ops->vidioc_s_fmt_vid_cap_mplane == NULL) ) {
        qnx_error("Unexpected NULL vidioc_s_fmt_vid_cap_mplane");
        return EIO;
    }

    // Set format based on buffer type
    if (isys->csi2[port].av[vc].aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        err = isys->csi2[port].av[vc].vdev.ioctl_ops->vidioc_s_fmt_vid_cap(&file, NULL, &format);
    } else if (isys->csi2[port].av[vc].aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        err = isys->csi2[port].av[vc].vdev.ioctl_ops->vidioc_s_fmt_vid_cap_mplane(&file, NULL, &format);
    } else {
        qnx_error("Unsupported buffer type = %d", isys->csi2[port].av[vc].aq.vbq.type);
        err = EINVAL;
    }
    if (err != EOK) {
        qnx_error("vidioc_s_fmt_vid_cap on port %d failed with err = %d", port, err);
        return err;
    }
    file.private_data = &isys->csi2_be_soc.av[stream].vdev;
    if ( (isys->csi2_be_soc.av[stream].vdev.ioctl_ops == NULL) ||
         (isys->csi2_be_soc.av[stream].vdev.ioctl_ops->vidioc_s_fmt_vid_cap == NULL) ) {
        qnx_error("Unexpected NULL ioctl_ops or ioctl_ops->vidioc_s_fmt_vid_cap");
        return EIO;
    }
    format.fmt.pix.pixelformat = csi2_be_pixelformat;
    err = isys->csi2_be_soc.av[stream].vdev.ioctl_ops->vidioc_s_fmt_vid_cap(&file, NULL, &format);
    if (err != 0) {
        qnx_error("vidioc_s_fmt_vid_cap failed with err = %d", err);
        return err;
    }

    return EOK;
}

int ipu4int_start_stream(ipu4int_handle_t handle)
{
    client_state_t                              *state;
    struct vb2_queue                            *queue;
    int                                         err;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    state = (client_state_t*) handle;
    if (state->in_use == false) {
        qnx_error("Inactive handle provided");
        return EINVAL;
    }
    if (state->stream_active) {
        qnx_error("Stream already started");
        return EBUSY;
    }
    if ( (state->csi2_queue == NULL) || (state->csi2be_queue == NULL) ) {
        qnx_error("Unexpected NULL queues");
        return EIO;
    }

    // Tell CSI2 BE queue to start streaming
    // Note - start_streaming assumes that queue is locked
    queue = state->csi2be_queue;
    intel_ipu4_isys_queue_lock(queue);
    err = queue->ops->start_streaming(queue, 0);
    intel_ipu4_isys_queue_unlock(queue);
    if (err != 0) {
        qnx_error("Failed to start_streaming CSI2 on port %d vc %d, err = %d",
                  state->port, state->vc, err);
        if (state->isys && state->isys->reset_needed)  {
            qnx_error("Reset needed indicated");
        }
        return EIO;
    }
    queue->streaming = 1;

    // Update state and return
    state->stream_active = true;
    return EOK;
}

int ipu4int_stop_stream(ipu4int_handle_t handle)
{
    client_state_t                              *state;
    struct vb2_queue                            *queue;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    state = (client_state_t*) handle;
    if (state->in_use == false) {
        qnx_error("Inactive handle provided");
        return EINVAL;
    }
    if (state->stream_active == false) {
        qnx_error("Stream not started");
        return ENODEV;
    }
    if ( (state->csi2_queue == NULL) || (state->csi2be_queue == NULL) ) {
        qnx_error("Unexpected NULL queues");
        return EIO;
    }

    // Tell CSI2 queue to stop streaming
    // Note - stop_streaming assumes that queue is locked
    queue = state->csi2be_queue;
    intel_ipu4_isys_queue_lock(queue);
    queue->ops->stop_streaming(queue);
    intel_ipu4_isys_queue_unlock(queue);
    queue->streaming = 0;

    // Free up all buffers in our linked lists
    cleanup_queues(state);

    // Update state and return
    state->stream_active = false;
    state->sof_eof_mismatch = false;
    return EOK;
}

int ipu4int_flush_stream(ipu4int_handle_t handle)
{
    client_state_t                              *state;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }

    // Cleanup queues to flush the buffers
    state = (client_state_t*) handle;
    pthread_mutex_lock(&state->mutex);
    cleanup_queues(state);
    state->buf_index = 0;
    pthread_mutex_unlock(&state->mutex);

    return EOK;
}

int ipu4int_get_buffer(ipu4int_handle_t handle, ipu4int_buffer_t* buffer, uint64_t request_timeout)
{
    client_state_t                              *state;
    struct timespec                             timeout;
    uint64_t                                    timeout_nsec;
    buffer_info_t                               *buf;
    buffer_info_t                               *free_buf;
    int                                         err;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (buffer == NULL) {
        qnx_error("NULL buffer provided");
        return EINVAL;
    }
    state = (client_state_t*) handle;
    if (state->in_use == false) {
        qnx_error("Inactive handle provided");
        return EINVAL;
    }
    if (state->stream_active == false) {
        qnx_error("Stream not started");
        return ENODEV;
    }

    // Wait on condvar if no done buffers available yet
    pthread_mutex_lock(&state->mutex);
    if ( (state->done_buffers == NULL) && (state->sof_eof_mismatch == false) ) {
        state->waiting_for_buffer = true;
        if (request_timeout == CAPTURE_TIMEOUT_INFINITE) {
            err = pthread_cond_wait(&state->cond, &state->mutex);
        } else {
            clock_gettime(CLOCK_MONOTONIC, &timeout);
            timeout_nsec = timespec2nsec(&timeout) + request_timeout;
            nsec2timespec(&timeout, timeout_nsec);
            err = pthread_cond_timedwait(&state->cond, &state->mutex, &timeout);
        }
        state->waiting_for_buffer = false;
        if (err != EOK) {
            qnx_error("Failed in waiting on condvar: err =%d", err);
            pthread_mutex_unlock(&state->mutex);
            return err;
        }
        if ( (state->done_buffers == NULL) && (state->sof_eof_mismatch == false) ) {
            qnx_error("Signaled, but no waiting buffers");
            pthread_mutex_unlock(&state->mutex);
            return EIO;
        }
    }

#ifdef TEST_SOF_EOF_DETECTION
    // For unit test, fake the occurrence of SOF/EOF mismatch once reach a given
    // frame count.
    sofEofFrameCount++;
    if (sofEofFrameCount > SOF_EOF_NUM_FRAMES) {
        sofEofFrameCount = 0;
        state->sof_eof_mismatch = true;
    }
#endif

    // Return special failure code if SOF/EOF mismatch was detected
    if (state->sof_eof_mismatch) {
        state->sof_eof_mismatch = false;
        pthread_mutex_unlock(&state->mutex);
        return ENOMSG;
    }

    // Will return to user the first entry from our done_buffers list
    buf                         = state->done_buffers;
    state->done_buffers         = buf->next;
    buf->next                   = NULL;
    buffer->data                = buf->data;
    buffer->timestamp.tv_sec    = buf->isys.vb.v4l2_buf.timestamp.tv_sec;
    buffer->timestamp.tv_nsec   = buf->isys.vb.v4l2_buf.timestamp.tv_usec * 1000;
    buffer->field               = buf->isys.vb.v4l2_buf.field;
    buffer->seqNum              = buf->isys.vb.v4l2_buf.sequence;

    // Call buf_finish, even though it does nothing in our case
    state->csi2be_queue->ops->buf_finish(&buf->isys.vb);

    // Log in case the state of the buffer was not OK
    // TODO: share this info with the user through an additional parameter
    if (buf->state != VB2_BUF_STATE_DONE) {
        qnx_error("State of returned buffer is %d", buf->state);
    }

    // Add to end of free_buffers list
    free_buf = state->free_buffers;
    if (free_buf) {
        while (free_buf->next) {
            free_buf = free_buf->next;
        }
        free_buf->next = buf;
    } else {
        state->free_buffers = buf;
    }

    pthread_mutex_unlock(&state->mutex);
    return EOK;
}

int ipu4int_queue_buffer(ipu4int_handle_t handle, ipu4int_buffer_t* buffer)
{
    client_state_t                              *state;
    int                                         err = EOK;
    buffer_info_t                               *buf;
    buffer_info_t                               *prev_buf;
    buffer_info_t                               *busy_buf;
    buffer_info_t                               *free_buf;
    off_t                                       paddr;
    dma_addr_t                                  dma;

    // Sanity check
    if (handle == NULL) {
        qnx_error("NULL handle provided");
        return EINVAL;
    }
    if (buffer == NULL) {
        qnx_error("NULL buffer provided");
        return EINVAL;
    }
    state = (client_state_t*) handle;
    if (state->in_use == false) {
        qnx_error("Inactive handle provided");
        return EINVAL;
    }
    if ( (state->csi2_queue == NULL) || (state->csi2be_queue == NULL) ) {
        qnx_error("Unexpected NULL queues");
        return EIO;
    }

    // Find buffer in free list
    pthread_mutex_lock(&state->mutex);
    buf = state->free_buffers;
    prev_buf = NULL;
    while (buf) {
        if (buf->data == buffer->data) {
            // Found a match - means this buffer has been used previously
            break;
        }
        prev_buf = buf;
        buf = buf->next;
    }

    // If buffer not found, it is either a buffer in busy list (error) or a new buffer
    if (buf == NULL) {
        buf = state->busy_buffers;
        while (buf) {
            if (buf->data == buffer->data) {
                // User is trying to queue a buffer that is already queued
                break;
            }
            buf = buf->next;
        }
        pthread_mutex_unlock(&state->mutex);
        if (buf != NULL) {
            qnx_error("Buffer already queued");
            return EINVAL;
        }
        // Allocate new buffer
        buf = (buffer_info_t*) calloc(1, sizeof(buffer_info_t));
        if (buf == NULL) {
            qnx_error("Failed to allocate buffer information");
            return ENOMEM;
        }
        buf->data                               = buffer->data;
        buf->next                               = NULL;
        buf->state                              = VB2_BUF_STATE_QUEUED;
        buf->isys.ib.type                       = INTEL_IPU4_ISYS_VIDEO_BUFFER;
        buf->isys.vb.vb2_queue                  = state->csi2be_queue;
        buf->isys.vb.v4l2_buf.index             = state->buf_index++;
        buf->isys.vb.v4l2_buf.type              = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf->isys.vb.v4l2_buf.length            = state->buf_size;
        buf->isys.vb.v4l2_buf.field             = V4L2_FIELD_NONE;
        buf->isys.vb.v4l2_buf.memory            = V4L2_MEMORY_USERPTR;
        buf->isys.vb.v4l2_buf.m.userptr         = (unsigned long) buffer->data;
        buf->isys.vb.num_planes                 = 1;
        buf->isys.vb.v4l2_planes[0].length      = state->buf_size;
        buf->isys.vb.v4l2_planes[0].m.userptr   = (unsigned long) buffer->data;
        // Set plane info information
        if  (mem_offset(buffer->data, NOFD, state->buf_size, &paddr, 0) == -1) {
            err = errno;
            qnx_error("Failed to get physical address: err = %d", err);
        } else {
            // Map buffer to ISYS MMU to get necessary dma addr
            err = map_user_buffer(&state->isys->adev->dev, buf, paddr,
                                  state->buf_size, &dma);
            if (err != EOK) {
                qnx_error("Failed to map user buffer: err = %d", err);
            } else {
                // Allocate info about this buffer
                err = qnxw_alloc_vb2_plane_info(&buf->isys.vb, 0, buffer->data, paddr, dma);
                if (err != EOK) {
                    qnx_error("Failed to set vb2 plane info: err = %d", err);
                } else {
                    // Call buf_init for this new buffer
                    err = state->csi2be_queue->ops->buf_init(&buf->isys.vb);
                    if (err != 0) {
                        qnx_error("buf_init failed with err = %d", err);
                        err = EIO;
                    }
                }
            }
        }
    } else {
        // Remove found buffer from our free list
        if (prev_buf) {
            prev_buf->next      = buf->next;
        } else {
            state->free_buffers = buf->next;
        }
        pthread_mutex_unlock(&state->mutex);
        buf->next = NULL;
    }

    if (err == EOK) {
        // Call buf_prepare before we queue the buffer
        err = state->csi2be_queue->ops->buf_prepare(&buf->isys.vb);
        if (err != 0) {
            qnx_error("buf_prepare failed with err = %d", err);
            err = EIO;
        } else {
            // Add buffer to end of our busy list before it is queued
            pthread_mutex_lock(&state->mutex);
            busy_buf = state->busy_buffers;
            if (busy_buf) {
                while (busy_buf->next) {
                    busy_buf = busy_buf->next;
                }
                busy_buf->next = buf;
            } else {
                state->busy_buffers = buf;
            }
            pthread_mutex_unlock(&state->mutex);

            // Queue up buffer by calling buf_queue
            // Note: this assumes that it is only necessary to queue up buffers to CSI2
            //       and that I do not need to then queue then up as well to CSI2 BE
            state->csi2be_queue->ops->buf_queue(&buf->isys.vb);
        }
    }

    if (err != EOK) {
        // Add buffer to end of free buffer list on error as it has not been queued
        pthread_mutex_lock(&state->mutex);
        free_buf = state->free_buffers;
        if (free_buf) {
            while (free_buf->next) {
                free_buf = free_buf->next;
            }
            free_buf->next = buf;
        } else {
            state->free_buffers = buf;
        }
        pthread_mutex_unlock(&state->mutex);
    }
    return err;
}

// Defined in ./include/media/videobuf2-core.h
/*
 * A buffer is done and ready to be sent back to user; state can be one of the following:
 *  VB2_BUF_STATE_QUEUED: error occurred when starting streaming - buffers are ours
 *  VB2_BUF_STATE_DONE: all went well - buffer was filled successfully
 *  VB2_BUF_STATE_ERROR: error when acquiring data on this buffer
 */
void vb2_buffer_done(struct vb2_buffer *vb, enum vb2_buffer_state buf_state)
{
    uint32_t                                    i;
    client_state_t                              *state;
    buffer_info_t                               *buf;
    buffer_info_t                               *done_buf;
    buffer_info_t                               *prev_buf;

    // Sanity check
    if (vb == NULL) {
        qnx_error("NULL vb received");
        return;
    }

    pthread_mutex_lock(&ipu4_clients_mutex);
    // Find stream to which this buffer belongs by matching queue with active streams
    for (i = 0; i < MAX_IPU4_STREAMS; i++) {
        state = &ipu4_clients[i];
        if (state->stream_active) {
            if ( (vb->vb2_queue == state->csi2_queue) ||
                 (vb->vb2_queue == state->csi2be_queue) ) {
                break;
            }
        }
    }
    pthread_mutex_unlock(&ipu4_clients_mutex);

    // Unexpected that we would get a buffer that would not belong to any of our queues
    if (i >= MAX_IPU4_STREAMS) {
        qnx_error("Failed to find matching stream for buffer");
        return;
    }

    // Match the buffer with a buffer in our busy list
    pthread_mutex_lock(&state->mutex);
    buf = state->busy_buffers;
    prev_buf = NULL;
    while (buf) {
        if (&buf->isys.vb == vb) {
            break;
        }
        prev_buf = buf;
        buf = buf->next;
    }
    if (buf == NULL) {
        qnx_error("Failed to find matching busy buffer");
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    // Update state of the buffer
    buf->state = buf_state;

    // Remove from busy list and add buffer to end of our done list
    if (prev_buf) {
        prev_buf->next = buf->next;
    } else {
        state->busy_buffers = buf->next;
    }
    buf->next = NULL;
    done_buf = state->done_buffers;
    if (done_buf) {
        while (done_buf->next) {
            done_buf = done_buf->next;
        }
        done_buf->next = buf;
    } else {
        state->done_buffers = buf;
    }

    // Signal condvar if user is waiting for a buffer
    if (state->waiting_for_buffer) {
        pthread_cond_signal(&state->cond);
    }
    pthread_mutex_unlock(&state->mutex);
}

// Defined and called by qnxwrapper.h; signals when a mismatch is detected
void ipu4int_signal_sof_eof_mismatch(uint32_t stream)
{
    client_state_t                              *state;

    // Sanity check
    if (stream >= MAX_IPU4_STREAMS) {
        qnx_error("Invalid stream specified");
        return;
    }

    // Retrieve state associated with this stream
    pthread_mutex_lock(&ipu4_clients_mutex);
    state = &ipu4_clients[stream];
    pthread_mutex_unlock(&ipu4_clients_mutex);

    // Ensure stream is active
    pthread_mutex_lock(&state->mutex);
    if (state->stream_active == true) {
        // Note occurrence and wakeup client if waiting for a buffer
        qnx_warning("SOF EOF mismatch detected on stream %d", stream);
        state->sof_eof_mismatch = true;
        if (state->waiting_for_buffer) {
            pthread_cond_signal(&state->cond);
        }
    } else {
        qnx_error("SOF EOF mismatch for stream %d that is not active", stream);
    }
    pthread_mutex_unlock(&state->mutex);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/intel-ipu4-interface.c $ $Rev: 876784 $")
#endif
