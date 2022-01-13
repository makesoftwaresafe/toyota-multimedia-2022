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

#ifndef _QNX_WRAPPER_H_
#define _QNX_WRAPPER_H_

#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <media/intel-ipu4-isys.h>
#include <linux/pci.h>

#include "intel-ipu4-isys.h"

extern pthread_mutex_t     qnx_vm_area_mutex;
extern bool                initAddrTableFlag;
extern pthread_mutex_t     addrTableMutex;

/**
 * @brief Initializes the wrapper before first use
 * @details
 * The wrapper needs to do some initialization before it is used.
 *
 * @return @c errno
 */
int qnxw_init(void);

/**
 * @brief De-initializes the wrapper after it is no longer required
 * @details
 * Call this when the wrapper is no longer needed to do proper cleanup.
 *
 * @return @c errno
 */
int qnxw_destroy(void);

/**
 * @brief Gets frame descriptor for a given data type for our camera
 * @details
 * This information is extracted from what the camera sensor supports
 * and what the user is requesting.
 *
 * @param entry On success, set to the frame descriptor entry
 * @param data_type The data type as specified by CSI2 standard
 * @param port The CSI2 port for which we want the frame descriptor
 *
 * @return @c errno
 */
int qnxw_get_frame_desc_entry_by_dt(struct v4l2_mbus_frame_desc_entry *entry,
                                    uint8_t data_type, uint32_t port);

/**
 * @brief Sets the frame descriptor for a given camera port
 * @details
 * Set by our interface code based on what is configured by the user.
 *
 * @param entry The new frame descriptor entry
 * @param port The port of the desired camera
 *
 * @return @c errno
 */
int qnxw_set_frame_desc_entry(struct v4l2_mbus_frame_desc_entry *entry,
                              uint32_t port);

/**
 * @brief Retrieves the desired CSI2 link frequency for a given port
 * @details
 * The frequency in Hz is specific to the the device we are talking to
 * and the resolution and frame rate of the data we are receiving.
 *
 * @param port CSI2 port for which we want the link frequency
 *
 * @return Frequency in Hz
 */
int64_t qnxw_get_csi2_link_frequency(uint32_t port);

/**
 * @brief Sets the desired CSI2 link frequency for a given port
 * @details
 * Set by our interface code based on what is configured by the user.
 *
 * @param port CSI2 port of interest
 * @param frequency The link frequency to apply to this port in Hz
 *
 * @return @c errno
 */
int qnxw_set_csi2_link_frequency(uint32_t port, int64_t frequency);

/**
 * @brief Retrieves the desired CSI2 configuration for a given port
 * @details
 * This configuration contains the CSI2 port number to use and the number of
 * CSI2 lanes that are required.
 *
 * @param port CSI2 port for which we want the config
 *
 * @return Pointer to CSI2 configuration
 */
struct intel_ipu4_isys_csi2_config* qnxw_get_csi2_config(uint32_t port);

/**
 * @brief Sets the desired CSI2 configuration for a given port
 * @details
 * Set by our interface code based on what is configured by the user.
 *
 * @param port CSI2 port associated with the config
 * @param config The config to apply to this port
 *
 * @return @c errno
 */
int qnxw_set_csi2_config(uint32_t port, struct intel_ipu4_isys_csi2_config* config);

/**
 * @brief Signals the occurrence of CSI2 Start of Frame event
 * @details
 * This indicates that a new CSI2 frame has started to be received on the CSI2 bus.
 *
 * @param vdev Video device handle; not used in QNX implementation and set to NULL
 * @param ev Information about the event occurrence
 *
 */
void qnxw_csi2_sof_event(struct video_device *vdev, const struct v4l2_event *ev);

/**
 * @brief Signals the occurrence of CSI2 End of Frame event
 * @details
 * This indicates that the end of a new CSI2 frame has been received on the CSI2 bus.
 *
 * @param vdev Video device handle; not used in QNX implementation and set to NULL
 * @param ev Information about the event occurrence
 *
 */
void qnxw_csi2_eof_event(struct video_device *vdev, const struct v4l2_event *ev);

/**
 * @brief Sets source format information for a given camera
 * @details
 * This information corresponds to what the user has defined.
 *
 * @param format Source format (fields width, height, field are used)
 * @param port The port of the desired camera
 * @param vc The Virtual Channel (VC) of the desired camera
 * @param code Media bus format as defined in include/uapi/linux/media-bus-format.h
 *
 * @return @c errno
 */
int qnxw_set_source_format(struct v4l2_format* format, uint32_t port, uint32_t vc,
                           uint32_t code);

/**
 * @brief Gets source format information for a given camera
 * @details
 * This information is extracted from what the camera sensor supports
 * and what the user is requesting.
 *
 * @param format On success, set to the source format
 * @param port The port of the desired camera
 * @param vc The Virtual Channel (VC) of the desired camera
 *
 * @return @c errno
 */
int qnxw_get_source_format(struct v4l2_subdev_format* format, uint32_t port,
                           uint32_t vc);

/**
 * @brief Give a pointer to our ISYS structure to our wrapper
 * @details
 * This state information is used to perform some of the functionality inside
 * the wrapper.
 *
 * @param isys Instance of the ISYS structure to use
 *
 */
void qnxw_set_isys_instance(struct intel_ipu4_isys* isys);

int PageHighMem(struct page *page);

/**
 * @brief Remove pair of virtual/physical addresses from address table
 * @details
 * Address table is used for physical to virtual address conversion.
 *
 * @param virtAddr Virtual address for which corresponding pair virtual/physical address will be removed from address table
 *
 */
void qnxw_remove_addr_from_table(void * virtAddr);

/**
 * @brief Get physical address for given page virtual address
 * @details
 * Calculate physical address for given page virtual address and add address's pair to address table
 *
 * @param page pointer to struct page
 *
 * @return Physical address for given page virtual address
 */
phys_addr_t page_to_phys(struct page *page);

/**
 * @brief Get virtual address for given physical address
 * @details
 * Find given physical address in address table and return corresponding virtual address
 *
 * @param address physical address
 *
 * @return Virtual address for given physical address
 */
void *phys_to_virt(phys_addr_t address);

/**
 * @brief Get physical address for given virtual address
 * @details
 * Calculate physical address for given virtual address and add address's pair to address table
 *
 * @param page pointer to struct page
 *
 * @return Physical address for given page virtual address
 */
phys_addr_t virt_to_phys(void *address);

/**
 * @brief Allocate one page
 * @details
 * Allocate contiguous memory block 4K length
 *
 * @param gfp memory allocation attributes. Not used for QNX
 *
 * @return Virtual address of allocated memory block
 */
unsigned long __get_free_page(gfp_t gfp);

/**
 * @brief Free one page
 * @details
 * Free contiguous memory block 4K length and remove pair virtual/physical adresses from address table
 *
 * @param addr virtual address of 4K memory block which should be freeded
 */
void free_page(unsigned long addr);

// Linux functions, not defined in any of our header files, so define here
int pci_set_dma_mask(struct pci_dev *dev, u64 mask);
int pci_set_consistent_dma_mask(struct pci_dev *dev, u64 mask);

/**
 * @brief Free memory allocated for pages using vmalloc_to_page
 * @details
 * Our implementations allocates memory, so we need to explicitly free it.
 *
 * @param pages array of pages to free
 * @param num_pages number of pages in the array
 */
void qnxw_free_vmalloc_pages(struct page **pages, unsigned long num_pages);

/**
 * @brief Allocate and set vb2_buffer plane info
 * @details
 * Additional information about vb2_buffer is stored in a private structure.
 * This needs to be set for these buffers once before they are used.
 *
 * @param vb Pointer to video buffer
 * @param plane_no The plane number (0 for packed formats)
 * @param buf_virt The virtual address of the buffer
 * @param buf_phys The physical address of the buffer
 * @param dma The dma address of the buffer - virtual address in hardware's memory space
 *
 * @return @c errno
 */
int qnxw_alloc_vb2_plane_info(struct vb2_buffer *vb, unsigned int plane_no,
                              void* buf_virt, off_t buf_phys, dma_addr_t dma);

/**
 * @brief Frees vb2_buffer plane info previously allocated
 * @details
 * Once a vb2_buffer is no longer required for which plane info was allocated
 * using @c qnxw_alloc_vb2_plane_info(), this function needs to be called to
 * free the associated memory.
 *
 * @param vb Pointer to video buffer
 * @param plane_no The plane number (0 for packed formats)
 *
 * @return @c errno
*/
int qnxw_free_vb2_plane_info(struct vb2_buffer *vb, unsigned int plane_no);

/**
 * @brief Retrieves the CSI2 port associated with a given media entity
 * @details
 * Some of our code needs to know the CSI2 port associated with an entity
 *
 * @param entity The entity for which we want to retrieve the csi2 port
 * @param port Filled with associated CSI2 port on success
 *
 * @return @c errno
*/
int qnxw_get_csi2_port(struct media_entity* entity, uint32_t* port);

/**
 * @brief Sets the port + virtual channel (VC) associated with a given stream
 * @details
 * Wrapper needs this information to get csi2 port above.
 *
 * @param stream The stream of interest (0 to NR_OF_CSI2_BE_SOC_SOURCE_PADS - 1)
 * @param port The CSI2 port associated with this stream
 * @param vc The CSI2 VC associated with this stream
 *
 * @return @c errno
*/
int qnxw_set_csi2_stream_info(uint32_t stream, uint8_t port, uint8_t vc);

/**
 * @brief Initialize PCI access before first use
 * @details
 * Initialization is required before accessing any other wrapper PCI
 * functionality.
 *
 * @return @c errno
*/
int qnxw_pci_init(void);

/**
 * @brief Attach to a given PCI device
 * @details
 * It will try to find the PCI device specified by the given vendor ID and
 * device ID.  If found, it will enable MSI and bus mastering capability and
 * set IRQ and other information about the device in pdev.  If not found, ENODEV
 * is returned and pdev is left untouched.
 *
 * @return @c errno
*/
int qnxw_pci_attach(uint16_t vendor_id, uint16_t device_id, int pci_bar, struct pci_dev *pdev);

/**
 * @brief Read PCI device command word
 * @details
 * It will read back the config space command word and store it in 'command'.
 *
 * @return @c errno
*/
int qnxw_pci_read_cmd(struct pci_dev *dev, uint16_t* command);

/**
 * @brief Write PCI device command word
 * @details
 * It will write the desired value to the config space command word.
 *
 * @return @c errno
*/
int qnxw_pci_write_cmd(struct pci_dev *dev, uint16_t command);

/**
 * @brief De-initialize PCI once access is no longer required
 * @details
 * Does all necessary cleanup when we no longer need any PCI functionality.
 *
 * @return @c errno
*/
int qnxw_pci_deinit(void);

/**
 * @brief Returns the current clock time in nanoseconds
 * @details
 *
 * @param cur_time It gets filled with the current clock time
 *
*/
void qnxw_get_current_clk_time(uint64_t* cur_time);

/**
 * @brief Reports to the IPU4 interface module that a SOF/EOF mismatch was detected
 * @details
 * Typically they should always be an EOF long packet associated with each SOF
 * long packet detected on the CSI2 bus.  For some faulty CSI2 transmitter, it
 * may happen that we get multiple SOF long packet without any associated EOF
 * long packet causing the IPU4 firmware to go into a bad state.  The QNX
 * wrapper monitors for this condition and reports back to the interface when
 * this occurs for a given stream.
 *
 * @param stream The active stream where the mismatch was detected
 *
*/
void ipu4int_signal_sof_eof_mismatch(uint32_t stream);

/**
 * @brief Informs the wrapper if an IPU4 reset is in progress.
 * @details
 * When a reset is active, the IPU4 driver will skip some waits in order to
 * complete the reset as fast as possible.
 *
 * @param enable When set to true, an IPU4 reset is in progress
 *
*/
void qnxw_set_ipu4_reset(bool enable);

/**
 * @brief Returns if an IPU4 reset is active
 * @details
 *
 * @return @c true if an IPU4 reset is active, @c false otherwise
*/
bool qnxw_is_ipu4_reset_active(void);

/**
 * @brief Registers the port for start stream synchronization
 * @details
 *
 * @param port CSI2 port number where the capture device is connected
 * @param av isys video associated with the stream to start
 * @param decoder_sync_lib Path of decoder start stream synchronization library
 * @param decoder_sync_data Data to be passed when capture_sync_decoder() is called
 *
 * @return @c errno
 *
*/
int qnxw_sync_start_stream_register(uint32_t port, struct intel_ipu4_isys_video *av, char *decoder_sync_lib, void *decoder_sync_data);

/**
 * @brief Unregisters the port for start stream synchronization
 * @details
 *
 * @param port CSI2 port number where the capture device is connected
 *
 * @return @c errno
 *
*/
int qnxw_sync_start_stream_unregister(uint32_t port);

/**
 * @brief Performs start stream synchronization with capture device
 * @details
 * The following function from the start stream synchronization library (if any)
 * is called for synchronization:
 *     capture_sync_decoder()
 *
 * @param av isys video associated with the stream to start
 *
 * @return @c errno
 *
*/
int qnxw_sync_start_stream_with_device(struct intel_ipu4_isys_video *av);

#endif // _QNX_WRAPPER_H_

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/qnxwrapper.h $ $Rev: 876784 $")
#endif
