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

#include <linux/qnx.h>
#include <linux/linux.h>
#include <uapi/linux/intel-ipu4-isys.h>
#include <dlfcn.h>

#ifdef LIBPCI
#include <pci/pci.h>
#include <pci/cap_msi.h>
#else
#include <hw/pci.h>
#endif

#include "intel-ipu4-dma.h"
#include "qnxwrapper.h"
#include "intel-ipu4-buttress.h"

// How many CSI2 ports we support
#define NUM_CSI2_PORTS              6

// How many CSI2 Virtual Channels are supported by CSI2 standard
#define NUM_CSI2_VC                 4

// Flags used to remember what type of entity we have encountered + masks used for bits
#define ENTITY_TYPE_CSI2            0x10000
#define ENTITY_TYPE_CSI2BE          0x20000
#define ENTITY_TYPE_TPG             0x40000
#define ENTITY_TYPE_UNKNOWN         0x80000

#define ENTITY_INST_1               0x000100
#define ENTITY_VC_1                 0x1000000

#define ENTITY_MASK_VC              0x3000000
#define ENTITY_SHIFT_VC             24
#define ENTITY_MASK_TYPE            0xFF0000
#define ENTITY_MASK_INST            0x00FF00
#define ENTITY_SHIFT_INST           8
#define ENTITY_MASK_ITER            0x0000FF

// Number of clocks are registered by IPU4 driver
#define NUM_IPU4_CLKS               6

#define MAX_MEDIA_ENTITY_LINKS      256

// Our VB2 (Video Buffer 2 used by V4L2) private structure to hold info about the buffers
typedef struct {
    void*           bufVirt;
    off_t           physAddr;
    dma_addr_t      dmaAddr;
} qnx_vb2_mem_priv_t;

typedef int (*qnx_set_ctrl_t)(struct v4l2_ctrl *ctrl);

static qnx_set_ctrl_t                          tpg_set_ctrl;
static struct intel_ipu4_isys*                 isys_inst;
static struct cache_ctrl                       cache_info;

// Information we need about each csi2 stream
typedef struct {
    uint8_t         port;
    uint8_t         vc;
    uint32_t        sofCount;
    uint32_t        eofCount;
} csi2_stream_info_t;

struct addr_node{
    void*               virtAddr;
    phys_addr_t         physAddr;
    struct addr_node *  next;
};

// Clock structure to satisfy caller; but keep in it useful information for us
// to call the necessary clock routines
struct clk {
    struct clk_hw*      hw;
    struct clk_ops*     ops;
    void*               sensor_clk;
};

struct vb2_dc_conf {
	struct device		*dev;
};

// Start stream synchronization with capture device
typedef struct {
    struct intel_ipu4_isys_video *av;

    void  *decoder_lib_handle;
    void  *decoder_sync_data;
    int   (*sync_decoder)(void*);
} qnx_sync_start_stream_t;


static struct clk                                   ipu4_registered_clks[NUM_IPU4_CLKS];
// split one big AddrTable for 2 tables:
// mainAddrTable keeps address related to opened stream. We cleanup this table when close stream
//initAddrTable keeps address related to IPU4 driver initialization. We cleanup this table only when de-init IPU4 driver
static struct addr_node *                           mainAddrTable = NULL;
static struct addr_node *                           initAddrTable = NULL;
bool                                                initAddrTableFlag = false;
pthread_mutex_t                                     addrTableMutex;
static struct intel_ipu4_isys_csi2_config           ipu4_csi2_config[NUM_CSI2_PORTS];
static int64_t                                      ipu4_csi2_link_frequency[NUM_CSI2_PORTS];
static struct v4l2_mbus_frame_desc_entry            ipu4_frame_desc[NUM_CSI2_PORTS];
static struct v4l2_subdev_format                    source_format[NUM_CSI2_PORTS][NUM_CSI2_VC];
static csi2_stream_info_t                           stream_info[NR_OF_CSI2_BE_SOC_SOURCE_PADS];
static bool                                         ipu4_reset_active = false;
static qnx_sync_start_stream_t                      ipu4_csi2_sync_start_stream[NUM_CSI2_PORTS];

// PCI handle
#ifdef LIBPCI
static pci_devhdl_t                 ipu4_pci_handle;
#else
static unsigned                     ipu4_pci_handle = -1;
static void*                        ipu4_device_handle;
#endif

// Global mutex for VM area
pthread_mutex_t                                     qnx_vm_area_mutex;

static void cleanup_addr_table(struct addr_node** addrTable)
{
    struct addr_node*   cur_node;
    struct addr_node*   next_node;

    if (*addrTable == NULL) {
        return;
    }
    pthread_mutex_lock(&addrTableMutex);
    cur_node = *addrTable;
    while (cur_node) {
        next_node = cur_node->next;
        free(cur_node);
        cur_node = next_node;
    }
    *addrTable = NULL;
    pthread_mutex_unlock(&addrTableMutex);
}

void cleanup_main_addr_table()
{
    cleanup_addr_table(&mainAddrTable);
}

static void* get_virt_addr_from_table(struct addr_node* addrTable, phys_addr_t paddr)
{
    struct addr_node*   cur_node;
    void*               vaddr = NULL;

    pthread_mutex_lock(&addrTableMutex);
    cur_node = addrTable;
    while (cur_node) {
        if (cur_node->physAddr == paddr) {
            vaddr = cur_node->virtAddr;
            break;
        }
        cur_node = cur_node->next;
    }
    pthread_mutex_unlock(&addrTableMutex);
    return vaddr;
}

static phys_addr_t get_phys_addr_from_table(struct addr_node* addrTable, void* vaddr)
{
    struct addr_node*   cur_node;
    phys_addr_t         paddr = 0;

    pthread_mutex_lock(&addrTableMutex);
    cur_node = addrTable;
    while (cur_node) {
        if (cur_node->virtAddr == vaddr) {
            paddr = cur_node->physAddr;
            break;
        }
        cur_node = cur_node->next;
    }
    pthread_mutex_unlock(&addrTableMutex);
    return paddr;
}

static void add_addr_to_table(struct addr_node** addrTable, struct addr_node* node)
{
    struct addr_node*   last_node;

    if (*addrTable == NULL) {
        *addrTable = node;
    } else {
        last_node = *addrTable;
        while (last_node->next) {
            last_node = last_node->next;
        }
        last_node->next = node;
    }
}

static bool remove_addr_from_table(struct addr_node** addrTable, void* vaddr)
{
    struct addr_node*   cur_node;
    struct addr_node*   prev_node = NULL;

    pthread_mutex_lock(&addrTableMutex);
    cur_node = *addrTable;
    while (cur_node) {
        if (cur_node->virtAddr == vaddr) {
            if (prev_node) {
                prev_node->next = cur_node->next;
            } else {
                *addrTable = cur_node->next;
            }
            free(cur_node);
            pthread_mutex_unlock(&addrTableMutex);
            return true;
        }
        prev_node = cur_node;
        cur_node = cur_node->next;
    }
    pthread_mutex_unlock(&addrTableMutex);
    return false;
}

static void sync_start_stream_init(void)
{
    int i;
    for (i = 0; i < NUM_CSI2_PORTS; i++) {
        ipu4_csi2_sync_start_stream[i].av = NULL;
        ipu4_csi2_sync_start_stream[i].decoder_lib_handle = NULL;
        ipu4_csi2_sync_start_stream[i].decoder_sync_data = NULL;
        ipu4_csi2_sync_start_stream[i].sync_decoder = NULL;
    }
}

static void sync_start_stream_destroy(void)
{
    int i;
    for (i = 0; i < NUM_CSI2_PORTS; i++) {
        qnxw_sync_start_stream_unregister(i);
    }
}

int qnxw_init(void)
{
    int err;

    err = pthread_mutex_init(&qnx_vm_area_mutex, NULL);
    if (err != EOK) {
        qnx_error("Failed to initialize qnx_vm_area_mutex: err = %d", err);
        return err;
    }

    err = pthread_mutex_init(&addrTableMutex, NULL);
    if (err != EOK) {
        qnx_error("Failed to initialize addrTableMutex: err = %d", err);
        pthread_mutex_destroy(&qnx_vm_area_mutex);
        return err;
    }

    // Initialize cache_info such that wrapper can use cache routines
    memset(&cache_info, 0, sizeof(cache_info));
    cache_info.fd = NOFD;
    err = cache_init(0, &cache_info, NULL);
    if (err != 0) {
        err = errno;
        qnx_error("Failed to initialize cache: err = %d", err);
        pthread_mutex_destroy(&qnx_vm_area_mutex);
        pthread_mutex_destroy(&addrTableMutex);
        return err;
    }
    initAddrTableFlag = true;

    sync_start_stream_init();
    return EOK;
}

int qnxw_destroy(void)
{
    int err = EOK;

    // Free up cache_info
    cache_fini(&cache_info);

    // Cleanup mainAddrTable
    cleanup_addr_table(&mainAddrTable);
    // Cleanup initAddrTable
    cleanup_addr_table(&initAddrTable);

    // Destroy qnx vm area mutex
    err = pthread_mutex_destroy(&qnx_vm_area_mutex);
    if (err != EOK) {
        qnx_error("Failed to destroy qnx_vm_area_mutex: err = %d", err);
    }

    // Destroy addrTableMutex mutex
    err = pthread_mutex_destroy(&addrTableMutex);
    if (err != EOK) {
        qnx_error("Failed to destroy addrTableMutex: err = %d", err);
    }

    sync_start_stream_destroy();

    return err;
}

int qnxw_get_frame_desc_entry_by_dt(struct v4l2_mbus_frame_desc_entry *entry,
                                    uint8_t data_type, uint32_t port)
{
    // Sanity check
    if (entry == NULL) {
        return EINVAL;
    }
    if (port >= NUM_CSI2_PORTS) {
        return EINVAL;
    }

    // Copy value set by user
    memcpy(entry, &ipu4_frame_desc[port], sizeof(struct v4l2_mbus_frame_desc_entry));
    entry->bus.csi2.data_type = data_type;

    return EOK;
}

int qnxw_set_frame_desc_entry(struct v4l2_mbus_frame_desc_entry *entry,
                              uint32_t port)
{
    // Sanity check
    if (entry == NULL) {
        return EINVAL;
    }
    if (port >= NUM_CSI2_PORTS) {
        return EINVAL;
    }

    // Copy value set by user
    memcpy(&ipu4_frame_desc[port], entry, sizeof(struct v4l2_mbus_frame_desc_entry));

    return EOK;
}

int64_t qnxw_get_csi2_link_frequency(uint32_t port)
{
    // Sanity check
    if (port >= NUM_CSI2_PORTS) {
        return 0;
    }

    return ipu4_csi2_link_frequency[port];
}

int qnxw_set_csi2_link_frequency(uint32_t port, int64_t frequency)
{
    // Sanity check
    if (port >= NUM_CSI2_PORTS) {
        return EINVAL;
    }

    // Apply the desired value
    ipu4_csi2_link_frequency[port] = frequency;

    return EOK;
}

struct intel_ipu4_isys_csi2_config* qnxw_get_csi2_config(uint32_t port)
{
    // Sanity check
    if (port >= NUM_CSI2_PORTS) {
        return NULL;
    }

    return &ipu4_csi2_config[port];
}

int qnxw_set_csi2_config(uint32_t port, struct intel_ipu4_isys_csi2_config* config)
{
    // Sanity check
    if ( (config == NULL) || (port >= NUM_CSI2_PORTS) ) {
        return EINVAL;
    }

    // Apply the required configuration
    ipu4_csi2_config[port].nlanes = config->nlanes;
    ipu4_csi2_config[port].port = config->port;

    return EOK;
}

void qnxw_csi2_sof_event(struct video_device *vdev, const struct v4l2_event *ev)
{
    uint32_t    stream;

    // Sanity check
    if (ev == NULL) {
        qnx_error("NULL event provided");
        return;
    }

    // Retrieve stream from event and ensure it is within range
    stream = ev->id;
    if (stream >= NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
        qnx_error("Stream %d is out of range", stream);
        return;
    }

    // Signal an error if SOF/EOF counts mismatch
    if (stream_info[stream].sofCount != stream_info[stream].eofCount) {
        stream_info[stream].sofCount = 0;
        stream_info[stream].eofCount = 0;
        ipu4int_signal_sof_eof_mismatch(stream);
    }

    // Increment our SOF event count
    stream_info[stream].sofCount++;
}

void qnxw_csi2_eof_event(struct video_device *vdev, const struct v4l2_event *ev)
{
    uint32_t    stream;

    // Sanity check
    if (ev == NULL) {
        qnx_error("NULL event provided");
        return;
    }

    // Retrieve stream from event and ensure it is within range
    stream = ev->id;
    if (stream >= NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
        qnx_error("Stream %d is out of range", stream);
        return;
    }

    // Increment our EOF event count
    stream_info[stream].eofCount++;
}

int qnxw_set_source_format(struct v4l2_format* format, uint32_t port, uint32_t vc,
                           uint32_t code)
{
    // Sanity check
    if (format == NULL) {
        return EINVAL;
    }
    if (port >= NUM_CSI2_PORTS) {
        return EINVAL;
    }
    if (vc >= NUM_CSI2_VC) {
        return EINVAL;
    }

    // Fill with values from the user
    source_format[port][vc].format.width = format->fmt.pix.width;
    source_format[port][vc].format.height = format->fmt.pix.height;
    source_format[port][vc].format.code = code;
    source_format[port][vc].format.field = format->fmt.pix.field;

    return EOK;
}

int qnxw_get_source_format(struct v4l2_subdev_format* format, uint32_t port,
                           uint32_t vc)
{
    // Sanity check
    if (format == NULL) {
        return EINVAL;
    }
    if (port >= NUM_CSI2_PORTS) {
        return EINVAL;
    }
    if (vc >= NUM_CSI2_VC) {
        return EINVAL;
    }

    format->format.width = source_format[port][vc].format.width;
    format->format.height = source_format[port][vc].format.height;
    format->format.code = source_format[port][vc].format.code;
    format->format.field = source_format[port][vc].format.field;

    return EOK;
}

void qnxw_set_isys_instance(struct intel_ipu4_isys* isys)
{
    isys_inst = isys;
}

// Defined in ./include/linux/media/v4l2-dev.h
struct video_device *video_devdata(struct file *file)
{
    // This assumes that all our calls to drivers will set file->private_data
    // to appropriate video_device before calling them.
    if (file) {
        return file->private_data;
    }
    return NULL;
}

// Defined in ./include/linux/media/v4l2-dev.h
int __video_register_device(struct video_device *vdev, int type,
                            int nr, int warn_if_nr_in_use, struct module *owner)
{
    // Remember values passed in
    vdev->vfl_type = type;
    vdev->num = nr;
    vdev->cdev = NULL;
    vdev->entity.name = vdev->name;

    // Need to set entity.parent that is being done as part of
    // media_device_register_entity() that is normally being called
    vdev->entity.parent = vdev->v4l2_dev->mdev;

    return 0;
}

// Defined in ./include/linux/media/v4l2-dev.h
void video_unregister_device(struct video_device *vdev)
{
    // Nothing to do for us
}

// Defined in ./include/linux/media/v4l2-dev.h
void video_device_release_empty(struct video_device *vdev)
{
    // This is meant to be an empty release call that does nothing
}

// Defined in ./include/linux/media/v4l2-ioctl.h
long video_ioctl2(struct file *file, unsigned int cmd, unsigned long arg)
{
    // Assume that we can simply stub this and do not need to use it
    return 0;
}

// Defined in ./include/linux/media/v4l2-subdev.h
int v4l2_subdev_link_validate(struct media_link *link)
{
    // Assume that all links are valid as we will ensure validity in our wrapper code
    return 0;
}

// Defined in ./include/linux/media/v4l2-subdev.h
int v4l2_subdev_link_validate_default(struct v4l2_subdev *sd,
                      struct media_link *link,
                      struct v4l2_subdev_format *source_fmt,
                      struct v4l2_subdev_format *sink_fmt)
{
    // Assume that all links are valid as we will ensure validity in our wrapper code
    return 0;
}

// Defined in ./include/linux/media/v4l2-subdev.h
void v4l2_subdev_init(struct v4l2_subdev *sd,
              const struct v4l2_subdev_ops *ops)
{
    // Basic initialization of structure - mainly copied from V4L2 source
    INIT_LIST_HEAD(&sd->list);
    sd->ops = ops;
    sd->v4l2_dev = NULL;
    sd->flags = 0;
    sd->name[0] = '\0';
    sd->grp_id = 0;
    sd->dev_priv = NULL;
    sd->host_priv = NULL;
    sd->entity.name = sd->name;
    sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
}

// Defined in ./include/linux/media/v4l2-device.h
int v4l2_device_register_subdev(struct v4l2_device *v4l2_dev, struct v4l2_subdev *sd)
{
    // Assume we do not need to do anything for now on registration
    return 0;
}

// Defined in ./include/linux/media/v4l2-device.h
void v4l2_device_unregister_subdev(struct v4l2_subdev *sd)
{
    // Assume for now that nothing needs to be done for our case
}

// Defined in ./include/linux/media/v4l2-device.h
int v4l2_device_register(struct device *dev, struct v4l2_device *v4l2_dev)
{
    // Mostly seems like we do not need to set much here
    v4l2_dev->dev = dev;

    return 0;
}

// Defined in ./include/linux/media/v4l2-device.h
void v4l2_device_unregister(struct v4l2_device *v4l2_dev)
{
    // No need to do anything in our use case
}

// Defined in ./include/linux/media/v4l2-device.h
int v4l2_device_register_subdev_nodes(struct v4l2_device *v4l2_dev)
{
    // Supposed to create nodes for each subdevs, but our code does not seem to
    // be using it, so seems safe to do nothing.
    return 0;
}

// Defined in ./include/linux/media/v4l2-event.h
int v4l2_event_subdev_unsubscribe(struct v4l2_subdev *sd, struct v4l2_fh *fh,
                                  struct v4l2_event_subscription *sub)
{
    // Assume nothing needs to be done here
    return 0;
}

// Defined in ./include/asm-generic/dma-mapping-broken.h
void * dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t flag)
{
    if (dma_handle == NULL) {
        return NULL;
    }
    return intel_ipu4_dma_ops.alloc(dev, size, dma_handle, flag, NULL);
}

// Defined in ./include/asm-generic/dma-mapping-broken.h
void dma_free_coherent(struct device *dev, size_t size, void *vaddr, dma_addr_t bus)
{
    return intel_ipu4_dma_ops.free(dev, size, vaddr, bus, NULL);
}

// Defined in ./include/asm-generic/dma-mapping-broken.h
void dma_sync_single_for_cpu(struct device *dev, dma_addr_t addr, size_t size,
                             enum dma_data_direction dir)
{
    // Retrieve directly what we know as being our dma ops
    const struct dma_map_ops *ops = &intel_ipu4_dma_ops;

    BUG_ON(!valid_dma_direction(dir));
    if (ops->sync_single_for_cpu) {
        // In our case, this will call intel_ipu4_dma_sync_single_for_cpu()
        ops->sync_single_for_cpu(dev, addr, size, dir);
    }
}

// Defined in ./include/asm-generic/dma-mapping-broken.h
int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
               enum dma_data_direction direction)
{
    // attrs seem unused, so seems OK to pass NULL
    return intel_ipu4_dma_ops.map_sg(dev, sg, nents, direction, NULL);
}

// Defined in ./include/asm-generic/dma-mapping-broken.h
void dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nhwentries,
                  enum dma_data_direction direction)
{
    // No need for attrs
    return intel_ipu4_dma_ops.unmap_sg(dev, sg, nhwentries, direction, NULL);
}

// Defined in ./include/asm-generic/dma-mapping-broken.h
void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg, int nelems,
                         enum dma_data_direction direction)
{
    struct page*    page;

    // Sanity check
    if (sg == NULL) {
        return;
    }

    // This assumes our use case where only 1 element in our sg list
    page = sg_page(sg);
    if (page == NULL) {
        return;
    }
    CACHE_FLUSH(&cache_info, page->virtual, (off_t) sg->dma_address, sg->length);
}

int qnxw_alloc_vb2_plane_info(struct vb2_buffer *vb, unsigned int plane_no,
                              void* buf_virt, off_t buf_phys, dma_addr_t dma)
{
    qnx_vb2_mem_priv_t*         info;

    // Sanity check
    if (vb == NULL) {
        return EINVAL;
    }
    if (plane_no >= vb->num_planes) {
        return EINVAL;
    }

    // Allocate plane information
    info = (qnx_vb2_mem_priv_t*) malloc(sizeof(qnx_vb2_mem_priv_t));
    if (info == NULL) {
        return ENOMEM;
    }
    info->bufVirt = buf_virt;
    info->physAddr = buf_phys;
    info->dmaAddr = dma;
    vb->planes[plane_no].mem_priv = info;

    return EOK;
}

int qnxw_free_vb2_plane_info(struct vb2_buffer *vb, unsigned int plane_no)
{
    // Sanity check
    if (vb == NULL) {
        return EINVAL;
    }
    if (plane_no >= vb->num_planes) {
        return EINVAL;
    }
    if (vb->planes[plane_no].mem_priv) {
        free(vb->planes[plane_no].mem_priv);
        vb->planes[plane_no].mem_priv = NULL;
    }
    return EOK;
}

// Defined in ./include/media/videobuf2-core.h
void *vb2_plane_vaddr(struct vb2_buffer *vb, unsigned int plane_no)
{
    qnx_vb2_mem_priv_t*         info;

    // Sanity check
    if (vb == NULL) {
        return NULL;
    }
    if (plane_no >= vb->num_planes) {
        return NULL;
    }

    // Get info from our private structure
    info = (qnx_vb2_mem_priv_t*) vb->planes[plane_no].mem_priv;
    if (info == NULL) {
        return NULL;
    }
    return info->bufVirt;
}

// Defined in ./include/media/videobuf2-core.h
// Cookie is the DMA address associated with the buffer by the hardware
void *vb2_plane_cookie(struct vb2_buffer *vb, unsigned int plane_no)
{
    qnx_vb2_mem_priv_t*         info;

    // Sanity check
    if (vb == NULL) {
        return NULL;
    }
    if (plane_no >= vb->num_planes) {
        return NULL;
    }

    // Get info from our private structure
    info = (qnx_vb2_mem_priv_t*) vb->planes[plane_no].mem_priv;
    if (info == NULL) {
        return NULL;
    }
    return (void*) &info->dmaAddr;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_reqbufs(struct file *file, void *priv,
                      struct v4l2_requestbuffers *p)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_create_bufs(struct file *file, void *priv,
                          struct v4l2_create_buffers *p)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_prepare_buf(struct file *file, void *priv,
                          struct v4l2_buffer *p)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_ioctl_expbuf(struct file *file, void *priv, struct v4l2_exportbuffer *p)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
unsigned int vb2_fop_poll(struct file *file, poll_table *wait)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_fop_mmap(struct file *file, struct vm_area_struct *vma)
{
    // Never called with our wrapper - do nothing
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
int vb2_queue_init(struct vb2_queue *q)
{
    // Does not seem like anything needs to be initialized for our own use
    return 0;
}

// Defined in ./include/media/videobuf2-core.h
void vb2_queue_release(struct vb2_queue *q)
{
    // Nothing seems to need cleaning up for our own use
}

// Defined in ./include/media/videobuf2-dma-contig.h
void *vb2_dma_contig_init_ctx(struct device *dev)
{
    struct vb2_dc_conf *ctx;

    // No real need for this context, but allocate something to satisfy the code
    ctx = (struct vb2_dc_conf*) malloc(sizeof(struct vb2_dc_conf*));
    if (ctx == NULL) {
        return ERR_PTR(-ENOMEM);
    }
    ctx->dev = dev;
    return ctx;
}

// Defined in ./include/media/videobuf2-dma-contig.h
void vb2_dma_contig_cleanup_ctx(void *alloc_ctx)
{
    // Free what we allocated
    if (!IS_ERR_OR_NULL(alloc_ctx)) {
        free(alloc_ctx);
    }
}

// Defined in ./include/asm-generic/dma-mapping-broken.h
void dma_sync_single_range_for_cpu(struct device *dev, dma_addr_t addr,
                                   unsigned long offset, size_t size, enum dma_data_direction dir)
{
    dma_sync_single_for_cpu(dev, addr + offset, size, dir);
}

// Defined in ./include/media/v4l2-ctrls.h
struct v4l2_ctrl *v4l2_ctrl_new_custom(struct v4l2_ctrl_handler *hdl,
                                       const struct v4l2_ctrl_config *cfg, void *priv)
{
    // Sanity check
    if (cfg == NULL) {
        return NULL;
    }

    // Allocate structure
    struct v4l2_ctrl* ctrl = (struct v4l2_ctrl*) malloc(sizeof(struct v4l2_ctrl));
    if (ctrl == NULL) {
        return NULL;
    }

    // Handle intel-ipu4-isys-isa.c use case
    if (cfg->id == V4L2_CID_INTEL_IPU4_ISA_EN) {
        // Do not enable any options for ISA for now
        ctrl->val = 0;
    } else if (cfg->id == V4L2_CID_TEST_PATTERN) {
        // These values are set by calling their s_ctrl function, so take note of it
        tpg_set_ctrl = cfg->ops->s_ctrl;
    }

    return ctrl;
}

// Defined in ./include/media/v4l2-ctrls.h
struct v4l2_ctrl *v4l2_ctrl_new_std(struct v4l2_ctrl_handler *hdl,
                                    const struct v4l2_ctrl_ops *ops,
                                    u32 id, s64 min, s64 max, u64 step, s64 def)
{
    // Allocate structure
    struct v4l2_ctrl* ctrl = (struct v4l2_ctrl*) malloc(sizeof(struct v4l2_ctrl));
    if (ctrl == NULL) {
        return NULL;
    }

    // Only used by intel-ipu4-isys-tpg.c - nothing needs to be filled in for
    // its use case
    return ctrl;
}

// Defined in ./include/media/v4l2-event.h
int v4l2_event_subscribe(struct v4l2_fh *fh,
                         const struct v4l2_event_subscription *sub,
                         unsigned elems,
                         const struct v4l2_subscribed_event_ops *ops)
{
    // Assume for now that nothing needs to be done for event subscription
    return 0;
}

// Defined in ./include/media/v4l2-ctrls.h
int v4l2_ctrl_handler_setup(struct v4l2_ctrl_handler *hdl)
{
    // Our only user of this function is tpg, so simply call tpg_set_ctrl for what it supports
    struct v4l2_ctrl        ctrl;

    // Set handler which is used by tpg_set_ctrl
    ctrl.handler = hdl;

    // Call for horizontal blanking - TODO: set to desired value
    ctrl.id = V4L2_CID_HBLANK;
    ctrl.val = 0;
    tpg_set_ctrl(&ctrl);

    // Call for vertical blanking - TODO - set to desired value
    ctrl.id = V4L2_CID_VBLANK;
    ctrl.val = 0;
    tpg_set_ctrl(&ctrl);

    // Call for desired test pattern
    ctrl.id = V4L2_CID_TEST_PATTERN;
    ctrl.val = 0;
    tpg_set_ctrl(&ctrl);

    return 0;
}

// Defined in ./include/media/v4l2-ctrls.h
int v4l2_ctrl_handler_init_class(struct v4l2_ctrl_handler *hdl,
                 unsigned nr_of_controls_hint,
                 struct lock_class_key *key, const char *name)
{
    // Init structure - mostly taken from v4l2-ctrls.c
    hdl->lock = &hdl->_lock;
    mutex_init(hdl->lock);
    INIT_LIST_HEAD(&hdl->ctrls);
    INIT_LIST_HEAD(&hdl->ctrl_refs);
    hdl->nr_of_buckets = 1 + nr_of_controls_hint / 8;
    hdl->buckets = calloc(hdl->nr_of_buckets, sizeof(hdl->buckets[0]));
    hdl->error = hdl->buckets ? 0 : -ENOMEM;
    return hdl->error;
}

// Defined in ./include/media/v4l2-ctrls.h
void v4l2_ctrl_handler_free(struct v4l2_ctrl_handler *hdl)
{
    free(hdl->buckets);
    hdl->buckets = NULL;
    if (hdl->lock) {
        mutex_destroy(hdl->lock);
    }
}

// Defined in ./include/media/v4l2-ctrls.h
int __v4l2_ctrl_s_ctrl_int64(struct v4l2_ctrl *ctrl, s64 val)
{
    // Done in intel-ipu4-isys-tpg to set read-only field of pixel rate
    // We do not need it, so simply ignore it
    return 0;
}

// Defined in ./include/linux/device.h
void *devm_kmalloc(struct device *dev, size_t size, gfp_t gfp)
{
    void*       mem;

    // Driver does not explicitly free this; freed manually in ipu4int_destroy()
    if (gfp & __GFP_ZERO) {
        mem = calloc(1, size);
    } else {
        mem = malloc(size);
    }
    return mem;
}

// Defined in ./include/media/media-entity.h
int media_entity_init(struct media_entity *entity, u16 num_pads,
        struct media_pad *pads, u16 extra_links)
{
    // Init basic structure - code taken from media-entity.c
    struct media_link *links;
    unsigned int max_links = num_pads + extra_links;
    unsigned int i;

    // Do not want to re-alloc, so always create enough links
    if (max_links < MAX_MEDIA_ENTITY_LINKS) {
        max_links = MAX_MEDIA_ENTITY_LINKS;
    }
    links = calloc(max_links, sizeof(links[0]));
    if (links == NULL) {
        return -ENOMEM;
    }
    entity->group_id = 0;
    entity->max_links = max_links;
    entity->num_links = 0;
    entity->num_backlinks = 0;
    entity->num_pads = num_pads;
    entity->pads = pads;
    entity->links = links;

    for (i = 0; i < num_pads; i++) {
        pads[i].entity = entity;
        pads[i].index = i;
    }

    return 0;
}

// Defined in ./include/media/media-entity.h
void media_entity_cleanup(struct media_entity *entity)
{
    if (entity && entity->links) {
        free(entity->links);
        entity->links = NULL;
    }
}

// Helper function to call validate link operations on links of an entity
static int validateEntityLinks(struct media_entity *entity)
{
    int i;

    // Nothing to do if no link validation function to call
    if ( (entity->ops == NULL) || (entity->ops->link_validate == NULL) ) {
        return 0;
    }
    for (i = 0; i < entity->num_links; i++) {
        struct media_link *link = &entity->links[i];

        // Only call validation function if entity is sink end of link
        if (link->sink->entity == entity) {
            entity->ops->link_validate(link);
        }
    }
    return 0;
}

int qnxw_get_csi2_port(struct media_entity* entity, uint32_t* port)
{
    int i;

    // Sanity check
    if ( (port == NULL) || (entity == NULL) ) {
        return EINVAL;
    }
    for (i = 0; i < NR_OF_CSI2_BE_SOC_SOURCE_PADS; i++) {
        if (&isys_inst->csi2_be_soc.av[i].vdev.entity == entity) {
            break;
        }
    }
    if (i < NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
        *port = stream_info[i].port;
        return EOK;
    }
    // Not found - expect it to be CSI2 BE entity
    return ENODEV;
}

int qnxw_set_csi2_stream_info(uint32_t stream, uint8_t port, uint8_t vc)
{
    if (stream >= NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
        return EINVAL;
    }
    stream_info[stream].port = port;
    stream_info[stream].vc = vc;
    stream_info[stream].sofCount = 0;
    stream_info[stream].eofCount = 0;

    return EOK;
}

// Defined in ./include/media/media-entity.h
int media_entity_pipeline_start(struct media_entity *entity,
                                struct media_pipeline *pipe)
{
    int i;
    uint8_t port;

    // Find entity and related entities: set pipe and validate links
    // We expect that CSI2 BE SOC will be what is started
    for (i = 0; i < NR_OF_CSI2_BE_SOC_SOURCE_PADS; i++) {
        if (&isys_inst->csi2_be_soc.av[i].vdev.entity == entity) {
            break;
        }
    }
    if (i < NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
        // Set CSI2 BE pipeline
        isys_inst->csi2_be_soc.asd.sd.entity.pipe = pipe;
        validateEntityLinks(&isys_inst->csi2_be_soc.asd.sd.entity);
    } else {
        // Unexpected that we will start something other then CSI2 BE
        qnx_error("Unexpected pipeline start not on CSI2 BE SOC");
        return -EINVAL;
    }

    // Also set pipe on CSI2 port which we can retrieve from stream_info
    port = stream_info[i].port;
    stream_info[i].sofCount = 0;
    stream_info[i].eofCount = 0;
    isys_inst->csi2[port].asd.sd.entity.pipe = pipe;
    entity->pipe = pipe;
    validateEntityLinks(entity);

    // Need to perform additional initialization that is typically done in
    // csi2_link_validate that we do not call in our case.
    intel_ipu_isys_csi2_init_pipe(&isys_inst->csi2[port], to_intel_ipu4_isys_pipeline(pipe));

    return 0;
}

// Defined in ./include/media/media-entity.h
void media_entity_pipeline_stop(struct media_entity *entity)
{
    int i;

    // Find entity and only clear pipe for that link only to supported multiple instances
    // We expect that CSI2 BE SOC will be what is stopped
    for (i = 0; i < NR_OF_CSI2_BE_SOC_SOURCE_PADS; i++) {
        if (&isys_inst->csi2_be_soc.av[i].vdev.entity == entity) {
            break;
        }
    }
    if (i < NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
        // Clear pipe associated with CSI2 BE
        entity->pipe = NULL;
        isys_inst->csi2_be_soc.asd.sd.entity.pipe = NULL;
    } else {
        // Unexpected that we are stopping something other than CSI2 BE
        qnx_error("Unexpected pipeline stop on non-source entity");
        return;
    }

    // Clear CSI2 port if no streams left - port is retrieved from stream_info
    if (isys_inst->csi2[stream_info[i].port].stream_count == 0) {
        isys_inst->csi2[stream_info[i].port].asd.sd.entity.pipe = NULL;
    }
}

// Defined in ./include/media/media-entity.h
void media_entity_graph_walk_start(struct media_entity_graph *graph,
                                   struct media_pad *pad)
{
    int i, j;
    struct media_entity* entity;

    if ( (pad == NULL) || (graph == NULL) ) {
        return;
    }
    entity = pad->entity;

    // Initialize to track from the top
    graph->top = 0;
    graph->stack[0].entity = entity;
    graph->stack[1].entity = NULL;

    // Find which entity user is interested in so we know what to give him
    for (i = 0; i < isys_inst->pdata->ipdata->csi2.nports; i++) {
        for (j = 0; j < NR_OF_CSI2_SOURCE_PADS; j++) {
            if (&isys_inst->csi2[i].av[j].vdev.entity == entity) {
                graph->stack[1].entity = &isys_inst->csi2[i].asd.sd.entity;
                break;
            }
        }
        if (j < NR_OF_CSI2_SOURCE_PADS) {
            break;
        }
        if (&isys_inst->csi2[i].asd.sd.entity == entity) {
            // TODO - assuming VC 0 here since no way to know; unused though
            graph->stack[1].entity = &isys_inst->csi2[i].av[0].vdev.entity;
            qnx_error("Assuming VC 0 for port %d", i);
            j = 0;
            break;
        }
    }
    if (i < isys_inst->pdata->ipdata->csi2.nports) {
        // CSI2 device
        graph->top |= ENTITY_TYPE_CSI2;
        graph->top |= (ENTITY_INST_1 * i);
        graph->top |= (ENTITY_VC_1 * j);
        return;
    }

    for (i = 0; i < isys_inst->pdata->ipdata->tpg.ntpgs; i++) {
        if (&isys_inst->tpg[i].av.vdev.entity == entity) {
            graph->stack[1].entity = &isys_inst->tpg[i].asd.sd.entity;
            break;
        }
        if (&isys_inst->tpg[i].asd.sd.entity == entity) {
            graph->stack[1].entity = &isys_inst->tpg[i].av.vdev.entity;
            break;
        }
    }
    if (i < isys_inst->pdata->ipdata->tpg.ntpgs) {
        // TPG device
        graph->top |= ENTITY_TYPE_TPG;
        graph->top |= (ENTITY_INST_1 * i);
        return;
    }

    for (i = 0; i < NR_OF_CSI2_BE_SOC_SOURCE_PADS; i++) {
        if (&isys_inst->csi2_be_soc.av[i].vdev.entity == entity) {
            graph->stack[1].entity = &isys_inst->csi2_be_soc.asd.sd.entity;
            break;
        }
        if (&isys_inst->csi2_be_soc.asd.sd.entity == entity) {
            graph->stack[1].entity = &isys_inst->csi2_be_soc.av[i].vdev.entity;
            break;
        }
    }
    if (i < NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
        // CSI2 BE device
        graph->top |= ENTITY_TYPE_CSI2BE;
        graph->top |= (ENTITY_INST_1 * i);
        return;
    }

    // Do not support ISA for now
    qnx_error("Unexpected cannot find entity for graph walk start");
    graph->top |= ENTITY_TYPE_UNKNOWN;
}

// Defined in ./include/media/media-entity.h
struct media_entity * media_entity_graph_walk_next(struct media_entity_graph *graph)
{
    struct media_entity* cur_entity = NULL;
    uint32_t inst;
    uint8_t port;
    uint8_t vc;
    uint32_t i;

    // Simpler not to follow the links since we know the entities
    if ((graph->top & ENTITY_MASK_ITER) == 0) {
        // First iteration - simply give back entity that was given
        cur_entity = graph->stack[0].entity;
    } else if ((graph->top & ENTITY_MASK_ITER) == 1) {
        // Second iteration - return paired device
        cur_entity = graph->stack[1].entity;
    } else {
        switch (graph->top & ENTITY_MASK_TYPE) {
        case ENTITY_TYPE_CSI2:
        case ENTITY_TYPE_TPG:
            // Return instances of CSI2-BE - assume only SOC instance is OK
            switch (graph->top & ENTITY_MASK_ITER) {
            case 2:
                inst = (graph->top & ENTITY_MASK_INST) >> ENTITY_SHIFT_INST;
                port = (uint8_t) inst;
                vc = (graph->top & ENTITY_MASK_VC) >> ENTITY_SHIFT_VC;
                // Find stream that matches this port + vc
                for (i = 0; i < NR_OF_CSI2_BE_SOC_SOURCE_PADS; i++) {
                    if ( (stream_info[i].port == port) &&
                         (stream_info[i].vc == vc) ) {
                        break;
                    }
                }
                if (i >= NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
                    qnx_error("Could not find stream for port %d VC %d", port, vc);
                    break;
                }
                cur_entity = &isys_inst->csi2_be_soc.av[i].vdev.entity;
                break;
            case 3:
                cur_entity = &isys_inst->csi2_be_soc.asd.sd.entity;
                break;
            default:
                // Nothing left to return
                break;
            }
            break;
        case ENTITY_TYPE_CSI2BE:
            // Return active CSI2 port based on CSI2_BE instance
            inst = (graph->top & ENTITY_MASK_INST) >> ENTITY_SHIFT_INST;
            if (inst >= NR_OF_CSI2_BE_SOC_SOURCE_PADS) {
                qnx_error("Unexpected CSI2_BE_SOC instance %d", inst);
                break;
            }
            port = stream_info[inst].port;
            vc = stream_info[inst].vc;
            switch (graph->top & ENTITY_MASK_ITER) {
            case 2:
                cur_entity = &isys_inst->csi2[port].av[vc].vdev.entity;
                break;
            case 3:
                cur_entity = &isys_inst->csi2[port].asd.sd.entity;
                break;
            default:
                // Nothing left to return
                break;
            }
            break;
        default:
            // Nothing will be sent
            break;
        }
    }
    graph->top += 1;
    return cur_entity;
}

// Defined in ./include/media/media-entity.h
int media_entity_create_link(struct media_entity *source, u16 source_pad,
                             struct media_entity *sink, u16 sink_pad, u32 flags)
{
    // Links are used directly by some of the code, so need to create them; our
    // wrapper code will not use them though
    struct media_link *link;

    // Sanity check
    if ( (source == NULL) || (sink == NULL) ) {
        qnx_error("NULL source or sink");
        return -EINVAL;
    }

    // Create link for source
    if (source->num_links >= source->max_links) {
        qnx_error("source num_links %d equals or exceeds max %d", source->num_links, source->max_links);
        return -EINVAL;
    }
    link = &source->links[source->num_links++];
    link->source = &source->pads[source_pad];
    link->sink = &sink->pads[sink_pad];
    link->flags = flags;

    // Create link for destination
    if (sink->num_links >= sink->max_links) {
        qnx_error("sink num_links %d equals or exceeds max %d", sink->num_links, sink->max_links);
        return -EINVAL;
    }
    link = &sink->links[sink->num_links++];
    link->source = &source->pads[source_pad];
    link->sink = &sink->pads[sink_pad];
    link->flags = flags;

    return 0;
}

// Defined in ./include/media/media-entity.h
// Code comes from /drivers/media/media-entity.c in Linux
bool media_entity_has_route(struct media_entity *entity, unsigned int pad0,
                unsigned int pad1)
{
    if (pad0 >= entity->num_pads || pad1 >= entity->num_pads)
        return false;

    if (pad0 == pad1)
        return true;

    if (!entity->ops || !entity->ops->has_route)
        return true;

    return entity->ops->has_route(entity, pad0, pad1);
}

// Defined in ./include/media/media-device.h
int __must_check __media_device_register(struct media_device *mdev,
                                         struct module *owner)
{
    // Only graph mutex + possibly parent are used by the driver code
    mutex_init(&mdev->graph_mutex);
    mdev->devnode.parent = mdev->dev;

    return 0;
}

// Defined in ./include/media/media-device.h
void media_device_unregister(struct media_device *mdev)
{
    // Only need to destroy the graph_mutex
    mutex_destroy(&mdev->graph_mutex);
}

// Defined in ./include/linux/mutex.h
int mutex_lock(struct mutex * m)
{
    int rc = pthread_mutex_lock(&m->mux);
    return rc;
}

// Defined in ./include/linux/mutex.h
void mutex_unlock(struct mutex * m)
{
    pthread_mutex_unlock(&m->mux);
}

// Defined in ./include/linux/mutex.h
int  mutex_trylock(struct mutex * m)
{
    int rc = pthread_mutex_trylock(&m->mux);
    return rc;
}

// Defined in ./include/linux/mutex.h
int mutex_is_locked(struct mutex * m)
{
    int rc = pthread_mutex_trylock(&m->mux);
    if (rc == EOK) {
        // Locked, so was not locked; unlock and return not locked
        pthread_mutex_unlock(&m->mux);
        return 0;
    } else if (rc == EBUSY) {
        // Already locked
        return 1;
    } else {
        // Error occurred, so we do not really know, but be conversative
        return 1;
    }
}

// Defined in ./include/media/v4l2-fh.h
int v4l2_fh_open(struct file *filp)
{
    // Assume nothing to do for us
    return 0;
}

// Defined in ./include/media/v4l2-fh.h
int v4l2_fh_release(struct file *filp)
{
    // Assume nothing to do for us
    return 0;
}

// Defined in ./include/media/v4l2-common.h
void v4l2_get_timestamp(struct timeval *tv)
{
    struct timespec cur_time;

    // Needs to set the time to the current time as it will be used for a buffer
    // we have just acquired; needs to be monotonic time
    clock_gettime(CLOCK_MONOTONIC, &cur_time);

    if (tv) {
        tv->tv_sec = cur_time.tv_sec;
        tv->tv_usec = cur_time.tv_nsec / NSEC_PER_USEC;
    }
}

// Defined in ./include/media/videobuf2-core.h
int vb2_fop_release(struct file *file)
{
    // Assume nothing to do for us
    return 0;
}

// Defined in ./include/linux/completion.h
unsigned long wait_for_completion_timeout(struct completion *x,
                                          unsigned long timeout)
{
    // Assume timeout is in milliseconds
    int err;
    unsigned long done;
    struct timespec to;

    // Lock the mutex
    err = pthread_mutex_lock(&x->mutex);
    if (err != EOK) {
        return 0;
    }

    // If completion already met, we are done
    if (x->done == 1) {
        pthread_mutex_unlock(&x->mutex);
        return 1;
    }

    // Or else, need to wait on condvar until condition is met with timeout
    clock_gettime(CLOCK_MONOTONIC, &to);
    // add usecs to the timeout
    to.tv_sec += (timeout / 1000L);
    to.tv_nsec += (timeout % 1000L) * 1000000L;
    // catch nsecs overflow
    if (to.tv_nsec >= 1000000000L) {
        to.tv_nsec -= 1000000000L;
        to.tv_sec++;
    }
    err = pthread_cond_timedwait(&x->cond, &x->mutex, &to);

    // Need to return non-zero if completion was successful, 0 if not
    done = x->done;
    pthread_mutex_unlock(&x->mutex);
    if ( (err == EOK) && (done == 1) ) {
        return 1;
    }
    return 0;
}

// Defined in ./include/linux/completion.h
void complete(struct completion *x)
{
    int err;

    // Lock the mutex
    err = pthread_mutex_lock(&x->mutex);
    if (err != EOK) {
        return;
    }

    // Signal completion
    x->done = 1;
    pthread_cond_signal(&x->cond);

    // Unlock the mutex
    pthread_mutex_unlock(&x->mutex);
}

// Borrow code from drivers/base/power/power.h
static inline void device_pm_init_common(struct device *dev)
{
	if (!dev->power.early_init) {
		spin_lock_init(&dev->power.lock);
		dev->power.qos = NULL;
		dev->power.early_init = true;
	}
}

// Borrow code from drivers/base/power/power.h
static inline void device_pm_init(struct device *dev)
{
	device_pm_init_common(dev);
	//TODO: check if we actually need to use device_pm_sleep_init() from drivers\base\power\main.c
	//TODO: check if we actually need to use pm_runtime_init() from drivers\base\power\runtime.c
}

// Defined in ./include/linux/device.h
void device_initialize(struct device *dev)
{
	INIT_LIST_HEAD(&dev->dma_pools);
	mutex_init(&dev->mutex);
	lockdep_set_novalidate_class(&dev->mutex);
	spin_lock_init(&dev->devres_lock);
	INIT_LIST_HEAD(&dev->devres_head);
	device_pm_init(dev);
	set_dev_node(dev, -1);
}

// Defined in ./include/linux/device.h
void put_device(struct device *dev)
{
	mutex_destroy(&dev->mutex);
}

// Defined in ./include/linux/device.h
int device_register(struct device *dev)
{
	device_initialize(dev);
	// Remove device_add() call.
	// I guess we don't need to add device to linux device hierarcy, but need to double check if miss any struct device fields initialization there
	return 0;
}

// Defined in ./include/linux/device.h
void device_unregister(struct device *dev)
{
    pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
    // Remove device_del() call since we don't add device at device_register()
    put_device(dev);
    // delete device name
    if (dev->kobj.name) {
        free((void*)dev->kobj.name);
        dev->kobj.name = NULL;
    }
    spin_destroy(&dev->power.lock);
    spin_destroy(&dev->devres_lock);
}

// Defined in ./include/linux/device.h
int driver_register(struct device_driver *drv)
{
	return 0;
}

// Defined in ./include/linux/device.h
void driver_unregister(struct device_driver *drv)
{
}

// Defined in ./include/linux/device.h
int bus_register(struct bus_type *bus)
{
	return 0;
}

// Defined in ./include/linux/device.h
void bus_unregister(struct bus_type *bus)
{
}

// Defined in ./include/linux/device.h
int dev_set_name(struct device *dev, const char *fmt, ...)
{
    int len;
    va_list arglist;
    const char *old_name = dev->kobj.name;
    char *s;

    if (dev->kobj.name && !fmt) {
        return 0;
    }

    va_start(arglist, fmt);
    len = vsnprintf(NULL, 0, fmt, arglist) + 1;

    dev->kobj.name = malloc(len);
    if (!dev->kobj.name) {
        va_end( arglist );
        dev->kobj.name = old_name;
        qnx_error("Failed to allocate kobj.name");
        return -ENOMEM;
    }
    vsnprintf((char*)dev->kobj.name, len, fmt, arglist);
    va_end( arglist );

    /* ewww... some of these buggers have '/' in the name ... */
    while ((s = strchr(dev->kobj.name, '/'))) {
        s[0] = '!';
    }

    if (old_name) {
        free((void*)old_name);
    }

    return 0;
}

// Defined in ./include/x86/asm/cacheflush.h
void clflush_cache_range(void *vaddr, unsigned int size)
{
    // The following code is the same as used in DRM
    off_t  paddr = 0;
    if (vaddr == NULL) {
        return;
    }
    if  (mem_offset(vaddr, NOFD, size, &paddr, 0) != -1) {
        CACHE_FLUSH(&cache_info, vaddr, paddr, size);
    }
}

// Defined in ./include/linux/firmware.h
int request_firmware(const struct firmware **fw, const char *name,
                     struct device *device)
{
    struct stat         file_info;
    struct firmware     *firmware_info;
    uint8_t             *firmware_buffer;
    int                 err = 0;
    size_t              read_bytes;
    size_t              bytes_to_read;
    FILE                *fd;

    // Sanity check
    if ( (fw == NULL) || (name == NULL) ) {
        return -EINVAL;
    }
    *fw = NULL;

    // Get size of firmware file to know how much memory to allocate
    if (stat(name, &file_info) == -1) {
        qnx_error("Failed to stat firmware file: errno = %d", errno);
        return -EIO;
    }

    // Allocate firmware structure
    firmware_info = (struct firmware*) malloc(sizeof(struct firmware));
    if (firmware_info == NULL) {
        qnx_error("Failed to allocate firmware_info");
        return -ENOMEM;
    }
    firmware_info->size = file_info.st_size;

    // Allocate buffer to hold the firmware - make it physically contiguous to
    // simplify our code as we need to generate sg list for it
    firmware_buffer = (uint8_t*) mmap(NULL, firmware_info->size,
                                      PROT_READ | PROT_WRITE | PROT_NOCACHE,
                                      MAP_PRIVATE | MAP_PHYS | MAP_ANON, NOFD, 0);
    if (firmware_buffer != MAP_FAILED) {
        firmware_info->data = firmware_buffer;
        // Open firmware file
        fd = fopen(name, "r");
        if (fd != NULL) {
            // Read in firmware file
            bytes_to_read = firmware_info->size;
            while (bytes_to_read) {
                read_bytes = fread(firmware_buffer, sizeof(uint8_t), bytes_to_read, fd);
                if (ferror(fd)) {
                    qnx_error("Error reading firmware file");
                    err = -EIO;
                    break;
                }
                if (read_bytes < bytes_to_read) {
                    bytes_to_read -= read_bytes;
                    firmware_buffer += read_bytes;
                    if (feof(fd)) {
                        qnx_error("Unexpected end of file reached when %zu bytes left to read", bytes_to_read);
                        err = -EIO;
                        break;
                    }
                } else if (read_bytes == bytes_to_read) {
                    bytes_to_read = 0;
                } else {
                    qnx_error("Unexpectedly read %zu bytes when %zu bytes left to read", read_bytes, bytes_to_read);
                    err = -EIO;
                    break;
                }
            }
            fclose(fd);
            if (bytes_to_read == 0) {
                // All went good - return info to user
                *fw = firmware_info;
                return 0;
            }
        } else {
            qnx_error("Failed to open firmware file: errno = %d", errno);
            err = -EIO;
        }
        munmap(firmware_buffer, firmware_info->size);
    } else {
        qnx_error("Failed to allocate firmware_buffer");
        err = -ENOMEM;
    }
    free(firmware_info);
    return err;
}

// Defined in ./include/linux/firmware.h
void release_firmware(const struct firmware *fw)
{
    // Free up memory associated with firmware
    if (fw) {
        munmap((void*) fw->data, fw->size);
        free((void*) fw);
    }
}

int PageHighMem(struct page *page)
{
    return 0;
}

void qnxw_remove_addr_from_table(void * virtAddr)
{
    if (!remove_addr_from_table(&mainAddrTable, virtAddr)) {
        remove_addr_from_table(&initAddrTable, virtAddr);
    }
}

phys_addr_t page_to_phys(struct page *page)
{
    return(virt_to_phys(page->virtual));
}

void *phys_to_virt(phys_addr_t address)
{
    void*   vaddr = NULL;

    // check initAddrTable
    vaddr = get_virt_addr_from_table(initAddrTable, address);
    if (vaddr == NULL) {
        // check mainAddrTable
        vaddr = get_virt_addr_from_table(mainAddrTable, address);
    }
    if (vaddr == NULL) {
#ifdef QNX_ADDRESSING_64BITS
        qnx_error("phys_to_virt failed paddr=0x%lx", address);
#else
        qnx_error("phys_to_virt failed paddr=0x%x", address);
#endif
    }
    return vaddr;
}

phys_addr_t virt_to_phys(void *address)
{
    phys_addr_t         offset = 0;
    struct addr_node *  cur_node;

    // try to find virtAddr in initAddrTable
    offset = get_phys_addr_from_table(initAddrTable, address);
    if (offset == 0) {
        // try to find virtAddr in mainAddrTable
        offset = get_phys_addr_from_table(mainAddrTable, address);
    }

    if (offset == 0) {
        // There is no provided address in addr tables
        if ( mem_offset(address, NOFD, PAGE_SIZE, (off_t *)&offset, 0) == -1 ) {
            qnx_error("%s: error getting physical address for buf %p [errno %d]", __FUNCTION__, address, errno);
            return (phys_addr_t) 0;
        }
        // add virtual and physical addresses to addr table
        cur_node = (struct addr_node *) malloc(sizeof(struct addr_node));
        if (cur_node == NULL) {
            qnx_error("Failed to allocate addrTable node");
            return -ENOMEM;
        }
        cur_node->virtAddr = address;
        cur_node->physAddr = offset;
        cur_node->next = NULL;
        pthread_mutex_lock(&addrTableMutex);
        if (initAddrTableFlag) {
            add_addr_to_table(&initAddrTable, cur_node);
        } else {
            add_addr_to_table(&mainAddrTable, cur_node);
        }
        pthread_mutex_unlock(&addrTableMutex);
    }

    return offset;
}

unsigned long __get_free_page(gfp_t gfp)
{
    void *page = NULL;

    page = mmap( 0, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_NOCACHE,
                 MAP_PRIVATE | MAP_PHYS | MAP_ANON, NOFD, 0);
    return((unsigned long)page);
}

void free_page(unsigned long addr)
{
    void *ptr = (void*) addr;
    if (ptr == NULL) {
        return;
    }
    qnxw_remove_addr_from_table(ptr);
    munmap(ptr, PAGE_SIZE);
}

// Defined in ./include/linux/fs.h
/**
 * simple_read_from_buffer - copy data from the buffer to user space
 * @to: the user space buffer to read to
 * @count: the maximum number of bytes to read
 * @ppos: the current position in the buffer
 * @from: the buffer to read from
 * @available: the size of the buffer
 *
 * The simple_read_from_buffer() function reads up to @count bytes from the
 * buffer @from at offset @ppos into the user space address starting at @to.
 *
 * On success, the number of bytes read is returned and the offset @ppos is
 * advanced by this number, or negative value is returned on error.
 **/
ssize_t simple_read_from_buffer(void __user *to, size_t count, loff_t *ppos,
                                const void *from, size_t available)
{
    loff_t pos = *ppos;

    if (pos < 0)
        return -EINVAL;
    if (pos >= available || !count)
        return 0;
    if (count > available - pos)
        count = available - pos;
    memcpy(to, from + pos, count);
    *ppos = pos + count;
    return count;
}

// Defined in ./include/linux/fs.h
ssize_t simple_write_to_buffer(void *to, size_t available, loff_t *ppos,
                               const void __user *from, size_t count)
{
    loff_t pos = *ppos;

    if (pos < 0)
        return -EINVAL;
    if (pos >= available || !count)
        return 0;
    if (count > available - pos)
        count = available - pos;
    memcpy(to + pos, from, count);
    *ppos = pos + count;
    return count;
}

// Defined in ./include/x86/asm/uaccess.h
unsigned long copy_from_user(void *to, const void __user *from, unsigned long n)
{
    if (likely(access_ok(VERIFY_READ, from, n)))
        memcpy(to, from, n);
    else
        memset(to, 0, n);
    return n;
}

// Defined in ./include/x86/asm/uaccess.h
unsigned long copy_to_user(void *to, const void *from, unsigned long n)
{
    if (likely(access_ok(VERIFY_READ, from, n)))
        memcpy(to, from, n);
    else
        memset(to, 0, n);
    return n;
}

// Not defined in any headers we have, so define it in our wrapper
int pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
    // Assume we have nothing to do for this, but take note of it
    if (dev) {
        dev->dma_mask = mask;
    }
    return 0;
}

// Not defined in any headers we have, so define it in our wrapper
int pci_set_consistent_dma_mask(struct pci_dev *dev, u64 mask)
{
    // Assume we have nothing to do for this, as no obvious QNX equivalent
    return 0;
}

// Defined in ./include/linux/vmalloc.h
struct page *vmalloc_to_page(const void *addr)
{
    // Memory allocated here must be free by calling qnxw_free_vmalloc_pages
    struct page * page;

    page = (struct page *) malloc(sizeof(struct page));
    if (page) {
        page->virtual = (void*) addr;
    }
    return page;
}

void qnxw_free_vmalloc_pages(struct page **pages, unsigned long num_pages)
{
    unsigned long i;

    // Sanity check
    if (pages == NULL) {
        return;
    }

    // Free what was allocated in vmalloc_to_page
    for (i = 0; i < num_pages; i++) {
        if (pages[i]) {
            free(pages[i]);
         }
    }
}

// Defined in ./include/linux/clk-provider.h
struct clk *clk_register(struct device *dev, struct clk_hw *hw)
{
    int             i;
    struct clk      *clk = NULL;

    // Sanity check
    if (hw == NULL) {
        return ERR_PTR(-EINVAL);
    }

    // Find next empty entry
    for (i = 0; i < NUM_IPU4_CLKS; i++) {
        if (ipu4_registered_clks[i].hw == NULL) {
            clk = &ipu4_registered_clks[i];
            break;
        }
    }

    // Should find an empty one unless something goes unexpected
    if (clk == NULL) {
        qnx_error("%s: failed to find empty clk entry", __FUNCTION__);
        return ERR_PTR(-EIO);
    } else {
        // Fill in entries for easy use
        clk->hw = hw;
        clk->ops = (struct clk_ops*) hw->init->ops;
        clk->sensor_clk = (void*)to_clk_intel_ipu4_sensor(hw);
    }

    return clk;
}

// Defined in ./include/linux/clk-provider.h
void clk_unregister(struct clk *clk)
{
    // Sanity check
    if (clk == NULL) {
        return;
    }

    // NULL entry which will NULL it in our array
    clk->hw = NULL;
    clk->ops = NULL;
    if (clk->sensor_clk) {
        free(clk->sensor_clk);
        clk->sensor_clk = NULL;
    }
}

// Defined in ./include/linux/clkdev.h
void clkdev_add(struct clk_lookup *cl)
{
    // Do not need to to anything for this
}

// Defined in ./include/linux/fs.h
loff_t no_llseek(struct file *file, loff_t offset, int whence)
{
    // Stub routine to indicate that seek is not supported
    return -ESPIPE;
}

int qnxw_pci_init(void)
{
#ifdef LIBPCI
    // No init required when using PCI library
#else
    ipu4_pci_handle = (unsigned) pci_attach(0);
    if (ipu4_pci_handle == -1) {
        qnx_error("Failed to attach to PCI server");
        return -EIO;
    }
#endif
    return EOK;
}

#ifdef LIBPCI
// Utility function to enable/disable MSI functionality
static int set_pci_msi(pci_bdf_t pci_info, pci_devhdl_t pci_hdl, bool enable)
{
    int                 pci_msi_idx;
    pci_cap_t           pci_msi_cap = NULL;
    pci_err_t           pci_err;

    pci_msi_idx = pci_device_find_capid(pci_info, CAPID_MSI);
    if (pci_msi_idx != -1) {
        pci_err = pci_device_read_cap(pci_info, &pci_msi_cap, pci_msi_idx);
        if (pci_err == PCI_ERR_OK) {
            if (enable) {
                pci_err = pci_device_cfg_cap_enable(pci_hdl, pci_reqType_e_ADVISORY, pci_msi_cap);
            } else {
                pci_err = pci_device_cfg_cap_disable(pci_hdl, pci_reqType_e_ADVISORY, pci_msi_cap);
            }
            free(pci_msi_cap);
            if (pci_err == PCI_ERR_OK) {
                // All went good
                return EOK;
            } else {
                qnx_error("Failed to enable/disable MSI capability: enable = %d err = %s",
                          enable, pci_strerror(pci_err));
            }
        } else {
            qnx_error("Failed to read MSI capability: err = %s", pci_strerror(pci_err));
        }
    } else {
        qnx_error("Failed to find MSI capability");
    }
    return -EIO;
}

// Utility function to fill in pdev info from the device
static int fill_pci_pdev(pci_bdf_t pci_info, pci_devhdl_t pci_hdl, int pci_bar, struct pci_dev *pdev)
{
    pci_err_t           pci_err;
    pci_revid_t         pci_revid;
    pci_irq_t           pci_irq;
    pci_cs_t            pci_slot;
    pci_ba_t            pci_bar_info;

    // Copy info required to Linux structure
    pci_err = pci_device_read_revid(pci_info, &pci_revid);
    if (pci_err != PCI_ERR_OK) {
        qnx_error("Failed to read revision Id: err %s", pci_strerror(pci_err));
        return -EIO;
    }
    pdev->revision      = pci_revid;
    pci_err = pci_device_read_irq(pci_hdl, NULL, &pci_irq);
    if (pci_err != PCI_ERR_OK) {
        qnx_error("Failed to read Irq: err %s", pci_strerror(pci_err));
        return -EIO;
    }
    pdev->irq           = pci_irq;
    pci_slot            = pci_device_chassis_slot(pci_info);
    pdev->devfn         = PCI_DEVFN(PCI_SLOT(pci_slot), PCI_FUNC(pci_info));
    pdev->bus->number   = PCI_BUS(pci_info);

    // Retrieve info for our desired BAR
    pci_bar_info.bar_num = pci_bar;
    pci_err = pci_device_read_ba(pci_hdl, NULL, &pci_bar_info, pci_reqType_e_MANDATORY);
    if (pci_err != PCI_ERR_OK) {
        qnx_error("Failed to read PCI base address: err %s", pci_strerror(pci_err));
        return -EIO;
    }
    pdev->resource[pci_bar].start = (resource_size_t) pci_bar_info.addr;
    pdev->resource[pci_bar].end   = pdev->resource[pci_bar].start;
    pdev->resource[pci_bar].end   += pci_bar_info.size - 1;

    return EOK;
}

int qnxw_pci_attach(uint16_t vendor_id, uint16_t device_id, int pci_bar, struct pci_dev *pdev)
{
    pci_bdf_t               pci_info = PCI_BDF_NONE;
    pci_err_t               pci_err;
    int                     rval;

    // Find the device
    pci_info = pci_device_find(0, vendor_id, device_id, PCI_CCODE_ANY);
    if (pci_info == PCI_BDF_NONE) {
        // No such device found
        return ENODEV;
    }

    // Attach to it
    ipu4_pci_handle = pci_device_attach(pci_info, pci_attachFlags_EXCLUSIVE_OWNER, &pci_err);
    if (ipu4_pci_handle != NULL) {
        // Enable MSI-X and bus-mastering - must be done prior to reading IRQ
        rval = set_pci_msi(pci_info, ipu4_pci_handle, true);
        if (rval == EOK) {
            // Fill in pdev info with IRQ and other info
            pdev->device = device_id;
            pdev->vendor = vendor_id;
            rval = fill_pci_pdev(pci_info, ipu4_pci_handle, pci_bar, pdev);
            if (rval == EOK) {
                // All good
                return EOK;
            }
            set_pci_msi(pci_info, ipu4_pci_handle, false);
        }
        pci_device_detach(ipu4_pci_handle);
        ipu4_pci_handle = NULL;
    } else {
        qnx_error("Failed to attach to PCI device");
        rval = EIO;
    }

    return rval;
}

#else
int qnxw_pci_attach(uint16_t vendor_id, uint16_t device_id, int pci_bar, struct pci_dev *pdev)
{
    struct pci_dev_info     info;

    info.VendorId = vendor_id;
    info.DeviceId = device_id;
    ipu4_device_handle = pci_attach_device(NULL, PCI_INIT_ALL | PCI_MASTER_ENABLE | PCI_USE_MSI, 0, &info);
    if (ipu4_device_handle == NULL) {
        // No such device found
        return ENODEV;
    }

    // Copy info required to Linux structure
    pdev->device        = info.DeviceId;
    pdev->vendor        = info.VendorId;
    pdev->revision      = info.Revision;
    pdev->irq           = info.Irq;
    pdev->devfn         = info.DevFunc;
    pdev->bus->number   = info.BusNumber;

    // PCI enumeration is giving us the wrong value without libpci and needs to be corrected
    pdev->resource[pci_bar].start = (resource_size_t) info.CpuBaseAddress[pci_bar] - 4;
    pdev->resource[pci_bar].end   = pdev->resource[pci_bar].start;
    pdev->resource[pci_bar].end   += info.BaseAddressSize[pci_bar] - 1;

    return EOK;
}
#endif

int qnxw_pci_read_cmd(struct pci_dev *dev, uint16_t* command)
{
    if ( (dev == NULL) || (command == NULL) ) {
        qnx_error("NULL parameters provided");
        return EINVAL;
    }
#ifdef LIBPCI
    pci_bdf_t       pci_info;
    pci_err_t       pci_err;

    pci_info = pci_bdf(ipu4_pci_handle);
    if (pci_info == PCI_BDF_NONE) {
        qnx_error("Failed to get pci_bdf");
        return EINVAL;
    }

    pci_err = pci_device_read_cmd(pci_info, command);
    if (pci_err != PCI_ERR_OK) {
        qnx_error("Failed to read PCI_COMMAND: err = %s", pci_strerror(pci_err));
        return EIO;
    }
#else
    int err = pci_read_config16(dev->bus->number, dev->devfn, PCI_COMMAND, 1, command);
    if (err != PCI_SUCCESS) {
        qnx_error("Failed to read PCI_COMMAND: err = %d", err);
        return EIO;
    }
#endif
    return EOK;
}

int qnxw_pci_write_cmd(struct pci_dev *dev, uint16_t command)
{
    if (dev == NULL) {
        qnx_error("NULL dev parameter provided");
        return EINVAL;
    }
#ifdef LIBPCI
    pci_bdf_t       pci_info;
    pci_err_t       pci_err;

    pci_info = pci_bdf(ipu4_pci_handle);
    if (pci_info == PCI_BDF_NONE) {
        qnx_error("Failed to get pci_bdf");
        return EINVAL;
    }

    pci_err = pci_device_write_cmd(ipu4_pci_handle, command, NULL);
    if (pci_err != PCI_ERR_OK) {
        qnx_error("Failed to write PCI_COMMAND: err = %s", pci_strerror(pci_err));
        return EIO;
    }
#else
    int err = pci_write_config16(dev->bus->number, dev->devfn, PCI_COMMAND, 1, &command);
    if (err != PCI_SUCCESS) {
        qnx_error("Failed to write PCI_COMMAND: err = %d", err);
        return EIO;
    }
#endif
    return EOK;
}

int qnxw_pci_deinit(void)
{
#ifdef LIBPCI
    pci_bdf_t               pci_info;

    if (ipu4_pci_handle) {
        pci_info = pci_bdf(ipu4_pci_handle);
        if (pci_info != PCI_BDF_NONE) {
            set_pci_msi(pci_info, ipu4_pci_handle, false);
        }
        pci_device_detach(ipu4_pci_handle);
        ipu4_pci_handle = NULL;
    }
#else
    if (ipu4_device_handle) {
        pci_detach_device(ipu4_device_handle);
        ipu4_device_handle = NULL;
    }
    if (ipu4_pci_handle != -1) {
        pci_detach(ipu4_pci_handle);
        ipu4_pci_handle = -1;
    }
#endif
    return EOK;
}

// Defined in ./include/linux/pm_runtime.h
int pm_generic_runtime_suspend(struct device *dev)
{
    const struct dev_pm_ops *pm = dev->driver ? dev->driver->pm : NULL;
    int ret;

    ret = pm && pm->runtime_suspend ? pm->runtime_suspend(dev) : 0;

    return ret;
}

// Defined in ./include/linux/pm_runtime.h
int pm_generic_runtime_resume(struct device *dev)
{
    const struct dev_pm_ops *pm = dev->driver ? dev->driver->pm : NULL;
    int ret;

    ret = pm && pm->runtime_resume ? pm->runtime_resume(dev) : 0;

    return ret;
}

// Defined in ./include/linux/bitmap.h
// Code copied from /lib/bitmap.c from Linux
void bitmap_set(unsigned long *map, unsigned int start, int len)
{
    unsigned long *p = map + BIT_WORD(start);
    const unsigned int size = start + len;
    int bits_to_set = BITS_PER_LONG - (start % BITS_PER_LONG);
    unsigned long mask_to_set = BITMAP_FIRST_WORD_MASK(start);

    while (len - bits_to_set >= 0) {
        *p |= mask_to_set;
        len -= bits_to_set;
        bits_to_set = BITS_PER_LONG;
        mask_to_set = ~0UL;
        p++;
    }
    if (len) {
        mask_to_set &= BITMAP_LAST_WORD_MASK(size);
        *p |= mask_to_set;
    }
}

// Defined in ./include/linux/bitmap.h
// Code copied from /lib/bitmap.c from Linux
void bitmap_clear(unsigned long *map, unsigned int start, int len)
{
    unsigned long *p = map + BIT_WORD(start);
    const unsigned int size = start + len;
    int bits_to_clear = BITS_PER_LONG - (start % BITS_PER_LONG);
    unsigned long mask_to_clear = BITMAP_FIRST_WORD_MASK(start);

    while (len - bits_to_clear >= 0) {
        *p &= ~mask_to_clear;
        len -= bits_to_clear;
        bits_to_clear = BITS_PER_LONG;
        mask_to_clear = ~0UL;
        p++;
    }
    if (len) {
        mask_to_clear &= BITMAP_LAST_WORD_MASK(size);
        *p &= ~mask_to_clear;
    }
}

// Defined in ./include/linux/bitmap.h
// Code copied from /lib/bitmap.c from Linux
/**
 * bitmap_find_next_zero_area_off - find a contiguous aligned zero area
 * @map: The address to base the search on
 * @size: The bitmap size in bits
 * @start: The bitnumber to start searching at
 * @nr: The number of zeroed bits we're looking for
 * @align_mask: Alignment mask for zero area
 * @align_offset: Alignment offset for zero area.
 *
 * The @align_mask should be one less than a power of 2; the effect is that
 * the bit offset of all zero areas this function finds plus @align_offset
 * is multiple of that power of 2.
 */
unsigned long bitmap_find_next_zero_area_off(unsigned long *map,
                         unsigned long size,
                         unsigned long start,
                         unsigned int nr,
                         unsigned long align_mask,
                         unsigned long align_offset)
{
    unsigned long index, end, i;
again:
    index = find_next_zero_bit(map, size, start);

    /* Align allocation */
    index = __ALIGN_MASK(index + align_offset, align_mask) - align_offset;

    end = index + nr;
    if (end > size)
        return end;
    i = find_next_bit(map, end, index);
    if (i < end) {
        start = i + 1;
        goto again;
    }
    return index;
}

// Defined in ./include/linux/wait.h
void __init_waitqueue_head(wait_queue_head_t *q, const char *name, struct lock_class_key *key)
{
    // We are not using wait queue structure, so nothing to init
}

void qnxw_get_current_clk_time(uint64_t* cur_time)
{
    if (cur_time == NULL) {
        return;
    }
    if (ClockTime(CLOCK_MONOTONIC, NULL, cur_time) == -1) {
        // Log failure, but set time to 0
        qnx_error("Failed to get ClockTime: err = %d", errno);
        *cur_time = 0;
    }
}

// Define in /linux/timer.h
void* timer_wait_thread(void* arg)
{
    struct timespec timeout;
    bool wait_for_timeout = true;
    struct timer_list *timer = (struct timer_list*) arg;

    if (timer == NULL) {
        return NULL;
    }
    while (wait_for_timeout) {
        pthread_mutex_lock(&timer->mutex);
        if ( (timer->thread_active == false) || (timer->expires == 0) ) {
            wait_for_timeout = false;
        } else {
            timeout = ns_to_timespec(timer->expires);
            pthread_cond_timedwait(&timer->cond, &timer->mutex, &timeout);
            if (timer_pending(timer) == 0) {
                // Timeout expired - call function then exit thread
                timer->function(timer->data);
                timer->thread_active = false;
                wait_for_timeout = false;
            }
        }
        pthread_mutex_unlock(&timer->mutex);
    }
    return NULL;
}

void qnxw_set_ipu4_reset(bool enable)
{
    ipu4_reset_active = enable;
}

bool qnxw_is_ipu4_reset_active(void)
{
    return ipu4_reset_active;
}

int qnxw_sync_start_stream_register(uint32_t port, struct intel_ipu4_isys_video *av, char *decoder_sync_lib, void *decoder_sync_data)
{
    void *handle;
    qnx_sync_start_stream_t *sync_entry;

    // Sanity check
    if (port >= NUM_CSI2_PORTS) {
        return EINVAL;
    }
    sync_entry      = &ipu4_csi2_sync_start_stream[port];
    sync_entry->av  = av;

    handle = dlopen(decoder_sync_lib, RTLD_LOCAL);
    if (handle == NULL) {
        qnx_error("dlopen of library %s failed with err %s", decoder_sync_lib, dlerror());
        return EIO;
    }
    sync_entry->sync_decoder = dlsym(handle, "capture_sync_decoder");
    if (sync_entry->sync_decoder == NULL) {
        qnx_error("dlsym of capture_sync_decoder failed with err %s", dlerror());
        dlclose(handle);
        return EIO;
    }
    sync_entry->decoder_lib_handle = handle;
    sync_entry->decoder_sync_data = decoder_sync_data;

    return EOK;
}

int qnxw_sync_start_stream_unregister(uint32_t port)
{
    qnx_sync_start_stream_t *sync_entry;

    // Sanity check
    if (port >= NUM_CSI2_PORTS) {
        return EINVAL;
    }
    sync_entry = &ipu4_csi2_sync_start_stream[port];

    if (sync_entry->decoder_lib_handle != NULL) {
        dlclose(sync_entry->decoder_lib_handle);
    }

    sync_entry->av = NULL;
    sync_entry->decoder_lib_handle = NULL;
    sync_entry->decoder_sync_data = NULL;
    sync_entry->sync_decoder = NULL;

    return EOK;
}

int qnxw_sync_start_stream_with_device(struct intel_ipu4_isys_video *av)
{
    int i;
    int ret;

    for (i = 0; i < NUM_CSI2_PORTS; i++) {
        if (ipu4_csi2_sync_start_stream[i].decoder_lib_handle != NULL &&
            ipu4_csi2_sync_start_stream[i].sync_decoder != NULL &&
            ipu4_csi2_sync_start_stream[i].av == av) {
            break;
        }
    }
    if (i == NUM_CSI2_PORTS) {
        // Sync not requested. Nothing to do.
        return EOK;
    }

    ret = ipu4_csi2_sync_start_stream[i].sync_decoder(ipu4_csi2_sync_start_stream[i].decoder_sync_data);
    if (ret != EOK) {
        qnx_error("capture_sync_decoder failed! ret=%d(%s)", ret, strerror(ret));
    }

    return ret;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/qnxwrapper.c $ $Rev: 876784 $")
#endif
