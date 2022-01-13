/**
 * \file: avatp_sender.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * AVATP sender module header.
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

#ifndef __AVATP_SENDER_H__
#define __AVATP_SENDER_H__

#include <sys/types.h>
struct mmsghdr;
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_vlan.h>

#include <gst/gst.h>

#include "avatp_avtp.h"
#include "avatp_vlan.h"

#define AVATPSENDER_SEND_ERROR				-2
#define AVATPSENDER_INVALID_ARG				-3
#define AVATPSENDER_NULL_ARG				-4
#define AVATPSENDER_SOCKET_ERROR			-5
#define AVATPSENDER_UNSUPPORTED_RTP_FORMAT	-6
#define AVATPSENDER_INVALID_RTP_PACKET		-7
#define AVATPSENDER_RTP_FRAME_TOO_LARGE		-8

#ifndef AVATPSENDER_IMPL
#define AVATPSENDER_BEGIN_PRIVATE const char _ [sizeof(struct {
#define AVATPSENDER_END_PRIVATE })];
#else
#define AVATPSENDER_BEGIN_PRIVATE
#define AVATPSENDER_END_PRIVATE
#endif

typedef struct _AVATPSender AVATPSender;

typedef enum avatp_stream_type
{
  AVATP_SENDER_NONE,
  AVATP_SENDER_RTP,
  AVATP_SENDER_H264
} avatp_stream_type;

struct _AVATPSender
{
    /* send structures */
    struct vlan_ethhdr ethernet_header;
    AVTPHeader avatp_header;
    struct sockaddr_ll socket_address;
    struct msghdr packet_header;
    struct iovec packet_data[5];

    int first_time;

    /* socket connection */
    int socket;

    /* properties */
    int stripping_enabled;
    int use_vlan_tag;
    unsigned int shape_interval;
};

/**
 * \func AVATPSender_init
 *
 * set initial values.
 *
 * \param self          object pointer
 */
void AVATPSender_init(AVATPSender* self);

/**
 * \func AVATPSender_close
 *
 * close socket connection and release memory.
 *
 * \param self          object pointer
 */
void AVATPSender_close(AVATPSender* self);

/**
 * \func AVATPSender_open
 *
 * open socket connection.
 *
 * \param self          object pointer
 * \param ifname        name of the network interface
 * \param dst_mac       MAC address of destination
 * \param stream_id     AVATP stream id
 *
 * \return error code (0 is success)
 */
int AVATPSender_open(AVATPSender* self, const char* ifname, void* dst_mac,
        u_int64_t stream_id);

/**
 * \func AVATPSender_send
 *
 * send RTP frame packed into AVATP to network.
 *
 * \param self          object pointer
 * \param payload       RTP frame data
 * \param length       RTP frame data length
 *
 * \return error code (0 is success)
 */
int AVATPSender_send(AVATPSender* self, void* header, int header_length, void* payload, int length, avatp_stream_type type);

/**
 * \func AVATPSender_set_rtp_header_stripping
 *
 * enable/disable stripping mode of RTP header.
 *
 * \param self          object pointer
 * \param enabled       1 is enabled, 0 is disabled
 *
 * \return error code (0 is success)
 */
int AVATPSender_set_rtp_header_stripping(AVATPSender* self, int enabled);

/**
 * \func AVATPSender_set_vlan_tag
 *
 * set 802.1Q VLAN tag fields.
 *
 * \param self          object pointer
 * \param enabled       1 is enabled, 0 is disabled
 * \param vlan_id       VLAN identifier field
 * \param vlan_priority VLAN priority code point field
 * \param drop          VLAN drop eligible field
 *
 * \return error code (0 is success)
 */
int AVATPSender_set_vlan_tag(AVATPSender* self, int enabled,
        unsigned int vlan_id, unsigned int vlan_priority, int drop);

/**
 * \func AVATPSender_set_shape_interval
 *
 * set minimum packet interval
 *
 * \param self           object pointer
 * \param shape_interval traffic shaping interval in nanoseconds (0-999999999).
 *
 * \return error code (0 is success)
 */
int AVATPSender_set_shape_interval(AVATPSender* self,
        unsigned int shape_interval);

#endif /* __AVATP_SENDER_H__ */
