/**
 * \file: avatp_receiver.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * AVATP receiver module header.
 *
 * \component: gst-plugins-avatp
 *
 * \author: Jakob Harder / ADIT / SW1 / jharder@de.adit-jv.com
 *
 * \copyright (c) 2012 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \history
 * 0.1 - Jakob Harder - initial version
 *
 ***********************************************************************/

#ifndef __AVATP_RECEIVER_H__
#define __AVATP_RECEIVER_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include "avatp_rtp.h"

#define AVATPRECEIVER_PACKET_DISCARD	1

#define AVATPRECEIVER_READ_ERROR		-2
#define AVATPRECEIVER_INVALID_ARG		-3
#define AVATPRECEIVER_NULL_ARG			-4
#define AVATPRECEIVER_SOCKET_ERROR		-5
#define AVATPRECEIVER_INVALID_PACKET	-6

#ifndef AVATPRECEIVER_IMPL
#define AVATPRECEIVER_BEGIN_PRIVATE const char _ [sizeof(struct {
#define AVATPRECEIVER_END_PRIVATE })];
#else
#define AVATPRECEIVER_BEGIN_PRIVATE
#define AVATPRECEIVER_END_PRIVATE
#endif

struct avatp_receiver
{
AVATPRECEIVER_BEGIN_PRIVATE
    /* stream id */
    u_int64_t    stream_id;

    /* sequencing */
    int first_time;
    u_int32_t last_sequence_number;
    unsigned char rtp_sequence_number_high;

    /* socket connection */
    int socket;
    unsigned char src_mac[ETH_ALEN];

    /* rtp header settings */
    int stripping_enabled;
    struct rtp_header rtp_header;
AVATPRECEIVER_END_PRIVATE
};

typedef struct avatp_receiver AVATPReceiver;

struct avatp_received_frame
{
    void* malloc_pointer;
    void* data;
    unsigned int size;
};

/**
 * \func AVATPReceiver_init
 *
 * set initial values.
 *
 * \param self          object pointer
 */
void AVATPReceiver_init(AVATPReceiver* self);

/**
 * \func AVATPReceiver_open
 *
 * open socket connection.
 *
 * \param self          object pointer
 * \param ifname        name of the network interface
 * \param stream_id     AVATP stream id
 *
 * \return error code (0 is success)
 */
int AVATPReceiver_open(AVATPReceiver* self, const char* ifname,
        u_int64_t stream_id);

/**
 * \func AVATPReceiver_close
 *
 * close socket connection and release memory.
 *
 * \param self          object pointer
 */
void AVATPReceiver_close(AVATPReceiver* self);

/**
 * \func AVATPReceiver_receive
 *
 * receive RTP frame packed into AVATP from network.
 *
 * \param self          object pointer
 * \param rtp_frame     [out] pointer to RTP frame
 *
 * \return error code (0 is success)
 */
int AVATPReceiver_receive(AVATPReceiver* self,
        struct avatp_received_frame* rtp_frame);

/**
 * \func AVATPReceiver_set_rtp_header_stripping
 *
 * enable/disable stripping mode of RTP header.
 *
 * \param self          object pointer
 * \param enabled       1 is enabled, 0 is disabled
 *
 * \return error code (0 is success)
 */
int AVATPReceiver_set_rtp_header_stripping(AVATPReceiver* self, int enabled);

/**
 * \func AVATPReceiver_set_rtp_header_values
 *
 * set values for RTP header fields (valid when RTP stripping is on).
 *
 * \param self          object pointer
 * \param version       RTP version
 * \param extension     RTP extension header bit
 * \param ssrc          stream source identifier
 *
 * \return error code (0 is success)
 */
int AVATPReceiver_set_rtp_header_values(AVATPReceiver* self,
        unsigned char version, unsigned char extension, unsigned int ssrc);

/**
 * \func AVATPReceiver_get_receive_length
 *
 * get number of bytes that are available to read.
 *
 * \param self          object pointer
 * \param read_size     [out] number of bytes
 *
 * \return error code (0 is success)
 */
int AVATPReceiver_get_receive_length(AVATPReceiver* self, int* read_size);

/**
 * \func AVATPReceiver_get_source_mac
 *
 * return MAC address of the stream source.
 *
 * \param self          object pointer
 *
 * \return MAC address
 */
const unsigned char* AVATPReceiver_get_source_mac(AVATPReceiver* self);

#endif /* __AVATP_RECEIVER_H__ */
