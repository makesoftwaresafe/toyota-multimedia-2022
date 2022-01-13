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

#ifndef _IPU4_INTERFACE_H_
#define _IPU4_INTERFACE_H_

/*
 * An handle for an IPU4 CSI2 stream.
 */
typedef void* ipu4_handle_t;

/*
 * Which of the CSI2 port is it desired to open; note that
 * IPU4INT_CSI2_NUMPORTS is simply used to note how many ports are supported
 * and is not a valid port number.
 */
typedef enum {
    IPU4_CSI2_PORT_1 = 0,
    IPU4_CSI2_PORT_2,
    IPU4_CSI2_PORT_3,
    IPU4_CSI2_PORT_4,
    IPU4_CSI2_PORT_5,
    IPU4_CSI2_PORT_6,
    IPU4_CSI2_NUMPORTS
} ipu4_csi2_port_t;

/*
 * Which of the CSI2 Virtual Channel (VC) is it desired to open; note that
 * IPU4INT_CSI2_NUMVCS is simply used to note how many VCs are supported
 * and is not a valid VC.
 */
typedef enum {
    IPU4_CSI2_VC_0 = 0,
    IPU4_CSI2_VC_1,
    IPU4_CSI2_VC_2,
    IPU4_CSI2_VC_3,
    IPU4_CSI2_NUMVCS
} ipu4_csi2_vc_t;

/*
 * What pixel format is the data being received over CSI2 and/or do we want
 * as output
 */
typedef enum {
    /*
     * 4:2:2 packed, Cb, Y, Cr, Y byte ordering, 16-bits per pixel
     */
    IPU4_FORMAT_CBYCRY = 0,
    /*
     * 5-bit Red, 6-bit Green, 5-bit Blue, 16 bits per pixel
     */
    IPU4_FORMAT_RGB565,
    /*
     * 8-bit Red, Green, Blue, 24-bits per pixel
     */
    IPU4_FORMAT_RGB888,
    /*
     * 8-bit Red, Green, Blue, Pad, 32-bits per pixel
     */
    IPU4_FORMAT_RGB8888,
    /*
     * 8-bit Raw Bayer
     */
    IPU4_FORMAT_BAYER8,
    /*
     * 10-bit Raw Bayer
     */
    IPU4_FORMAT_BAYER10,
    /*
     * 16-bit Raw Bayer
     */
    IPU4_FORMAT_BAYER16,
    /*
     * 4:2:2 packed, Y, Cb, Y, Cr byte ordering, 16-bits per pixel
     */
    IPU4_FORMAT_YCBYCR
} ipu4_format_t;

/*
 * The CSI2 configuration for our desired stream:
 *   num_lanes: how many CSI2 lanes are required
 *   frequency: the frequency of the CSI2 bus required in Hz
 */
typedef struct {
    uint32_t            num_lanes;
} ipu4_csi2_config_t;

/*
 * Maximum string length for decoder sync library
 */
#define IPU4_LIB_PATH_LEN (100)

/*
 * The stream configuration:
 * width: width of the frame in pixels
 * height: height of the frame in pixels
 * roi_x: x coordinate where to start copying the data (0 for full)
 * roi_y: y coordinate where to start copying the data (0 for full)
 * input_format: the format of the data being received from CSI2 bus
 * output_format: the format of the desired output
 * bytes_per_line: width * bytes per pixel plus byte alignment
 * buffersize: height * bytes_per_line plus any extra lines required
 * csi2_frequency: the frequency of the CSI2 bus in Hz
 * decoder_sync_lib: library that IPU4 dlopens and calls capture_sync_decoder() (optional)
 * decoder_sync_data: data to be used when calling capture_sync_decoder() (optional)
 */
typedef struct {
    uint16_t            width;
    uint16_t            height;
    uint16_t            roi_x;
    uint16_t            roi_y;
    ipu4_format_t       input_format;
    ipu4_format_t       output_format;
    uint16_t            bytes_per_line;
    uint32_t            buffersize;
    int64_t             csi2_frequency;
    char                decoder_sync_lib[IPU4_LIB_PATH_LEN+1];
    void                *decoder_sync_data;
} ipu4_stream_config_t;

/*
 * Information associated with buffers given/received from this library.  Note
 * that only 'data' needs to be set when feeding a buffer and the other fields
 * here are filled in by the library when filled buffers are returned.
 * data: virtual address of the buffer
 * timestamp: timestamp associated with the acquisition of the frame
 * field: 0 for odd field or interlaced, 1 for even field
 * seqNum: incrementing counter of the received frames from the stream
 */
typedef struct {
    void*               data;
    struct timespec     timestamp;
    uint32_t            field;
    uint32_t            seqNum;
} ipu4_buffer_t;

/**
 * @brief One-time library initialization
 * @details
 * This function needs to be called once before calling any other functions in
 * this library.  It does necessary one-time initialization to get the library
 * ready.
 *
 * @param config Array of IPU4_CSI2_NUMPORTS entries (one per port) that
 *               defines the CSI2 configuration for each port; set num_lanes to
 *               0 if a port is unused
 *
 * @return @c errno
 */
int ipu4_initialize(ipu4_csi2_config_t* config);

/**
 * @brief One-time library de-initialization
 * @details
 * This function needs to be called when the library is no longer required to
 * permit proper cleanup.  After calling this function, you cannot use any
 * other functions in this library.
 *
 * @return @c errno
 */
int ipu4_destroy(void);

/**
 * @brief Sets the maximum number of streams to support by the IPU4 driver
 * @details
 * Optional configuration of the maximum streams supported by the IPU4 driver
 * that must be set prior to starting any streams.  If not set, the maximum
 * number of streams will be the default value supported by the driver.
 *
 * @param num The desired maximum number of streams
 *
 * @return @c errno
 */
int ipu4_set_max_streams(uint32_t num);

/**
 * @brief Reset the IPU4 driver
 * @details
 * This function assumes that ipu4_initialize was already performed.  This
 * function performs a reset in order to permit the IPU4 driver to recover from
 * an non-recoverable error (i.e. malformed CSI2 packets received).  The reset
 * will stop and close all active streams and then destroy the driver instance.
 * The reset will then initialize the driver and bring it to a state where
 * streams can be started again.  It is up to the user to start any of the
 * streams that it desires to restart after this reset completes.
 *
 * @return @c errno
 */
int ipu4_reset(void);

/**
 * @brief Open a stream on a given CSI2 port
 * @details
 * The handle returned by this function must be used in all other functions
 * related to this stream.  You can only open a port with a valid CSI2 port
 * configuration (num_lanes is non-zero) and that is not already open.
 *
 * @param handle On success, set to the handle to use for this stream
 * @param port Which port to open
 * @param chan Which CSI2 Virtual Channel (VC) to use
 *
 * @return @c errno
 */
int ipu4_open_stream(ipu4_handle_t* handle, ipu4_csi2_port_t port,
                     ipu4_csi2_vc_t chan);

/**
 * @brief Close a stream
 * @details
 * Call this function to close an opened stream that is no longer required.
 *
 * @param handle The handle of the stream to close
 *
 * @return @c errno
 */
int ipu4_close_stream(ipu4_handle_t handle);

/**
 * @brief Configure a stream
 * @details
 * Call this function to configure the details about the stream.  You need to
 * configure the stream before starting it.
 *
 * @param handle The handle of the stream to configure
 * @param config The configuration to apply to this stream
 *
 * @return @c errno
 */
int ipu4_configure_stream(ipu4_handle_t handle, ipu4_stream_config_t* config);

/**
 * @brief Start a stream
 * @details
 * Call this function to start streaming on a configured stream.  Before starting
 * streaming, you need to have configured the stream and have fed sufficient
 * buffers to the stream (TO CONFIRM - seems like you may need to feed all buffers).
 *
 * @param handle The handle of the stream to start
 *
 * @return @c errno
 */
int ipu4_start_stream(ipu4_handle_t handle);

/**
 * @brief Stop a stream
 * @details
 * Call this function to stop streaming on a stream that has previously been
 * started.  All buffers that have been fed to the driver can be safely
 * re-used once this function returns.
 *
 * @param handle The handle of the stream to stop
 *
 * @return @c errno
 */
int ipu4_stop_stream(ipu4_handle_t handle);

/**
 * @brief Flushes a stream
 * @details
 * Call this function to flush the buffers that have been fed so far.  This
 * permits use of a new set of buffers instead of the ones that were used
 * previously.
 *
 * @param handle The handle of the stream to flush
 *
 * @return @c errno
 */
int ipu4_flush_stream(ipu4_handle_t handle);

/**
 * @brief Get the next filled buffer from the active stream
 * @details
 * Call this function to get the next filled buffer from this stream.  The
 * stream must have been previously started and sufficient buffers must have
 * been queued to the stream using @c ipu4_queue_buffer().  This function
 * will block until the next filled frame is received or a timeout occurs.
 *
 * @param handle The handle of the stream
 * @param buffer On success, it gets filled with information about the filled
 *               buffer
 * @param timeout The number of nanoseconds to wait for buffer availability.
 *
 * @return @c errno
 */
int ipu4_get_buffer(ipu4_handle_t handle, ipu4_buffer_t* buffer, uint64_t timeout);

/**
 * @brief Queue an empty buffer to be filled with an acquired frame
 * @details
 * Call this function to queue a buffer that can be used by the library to
 * store an acquired frame from the camera.  Once a frame is queued, it cannot
 * be used by the caller until the buffer is returned to caller through a call
 * to @c ipu4_get_buffer() or the stream is stopped.
 *
 * @param handle The handle of the stream
 * @param buffer Information about the buffer; only 'data' field needs to be set
 *
 * @return @c errno
 */
int ipu4_queue_buffer(ipu4_handle_t handle, ipu4_buffer_t* buffer);


#endif // _IPU4_INTERFACE_H_

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/intel-ipu4/public/intel/ipu4/ipu4-interface.h $ $Rev: 876784 $")
#endif
