/**
 * \file: avatp_sender.c
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * AVATP sender module implementation.
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
 * 0.2 - Gautham Kantharaju - Added signal AVATP_FIRST_FRAME_DATA and
 * 							  AVATP_LAST_FRAME_DATA to notify application about
 * 							  first and the last frame data transmitted.
 * 							  Incorporated a logic to split the incoming H264
 * 							  encoded frame data buffer into NAL units and
 * 							  transmit. Changes are w.r.t change request
 *                            #CR-RB-2014-011_iCAM_gst_plugin
 *
 ***********************************************************************/

#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <inttypes.h>

#define AVATPSENDER_IMPL

#include "avatp_sender.h"
#include "avatp_rtp.h"
#include "avatp_log.h"

/* defined in net/if.h */
extern unsigned int if_nametoindex(const char *__ifname);

/* PRQA: Lint Message 774: deactivation because any GST_DEBUG thrown this info */
/*lint -e774*/


/* PRQA: Lint message 160: deactivation because endianess conversion macros
 * (e.g. be16toh) are part of standard Linux headers */
/*lint -e160*/

#define AVATP_SUBTYPE				0x3
#define AVATP_TU                    0x1
#define AVATP_STREAM_VALID			0x1
#define AVATP_FORMAT                0x2
#define AVATP_DRM                   0
#define AVATP_FORMAT_SUBTYPE        0x1

#define AVATP_MIN_PACKET_LENGTH     64 /* 64 instead of ETH_ZLEN(60) */

static char AVATPSender_padding_buffer[AVATP_MIN_PACKET_LENGTH];

/* private methods */
static int AVATPSender_read_rtp_header(AVATPSender* self, void* payload,
        int length);
static int AVATPSender_strip_rtp_header(AVATPSender* self, void** payload,
        int* length);

void AVATPSender_init(AVATPSender* self)
{
    if (self == NULL)
        return;

    memset(self, 0, sizeof(AVATPSender));

    /* prepare ethernet header */
    self->ethernet_header.h_proto = htons(ETH_P_AVA_AVTP);

    /* prepare AVATP header */
    self->avatp_header.subtype = AVATP_SUBTYPE;
    self->avatp_header.timestamp_uncertain = AVATP_TU;
    self->avatp_header.streamid_valid = AVATP_STREAM_VALID;
    self->avatp_header.format = AVATP_FORMAT;
    self->avatp_header.drm = AVATP_DRM;

    /* PRQA: Lint message 644: deactivation because endianess conversion macros
     * (e.g. be16toh) are part of standard Linux headers */
    /*lint -e644 */
    self->avatp_header.format_subtype = htobe16(AVATP_FORMAT_SUBTYPE);
    /*lint +e644 */


    /* prepare socket address */
    self->socket_address.sll_family = PF_PACKET;

    /* prepare data vector for sendmsg */
    self->packet_data[0].iov_base = (void*)&self->ethernet_header;
    self->packet_data[0].iov_len = sizeof(struct ethhdr);
    self->packet_data[1].iov_base = (void*)&self->avatp_header;
    self->packet_data[1].iov_len = sizeof(AVTPHeader);
    /* set zero padding buffer */


    memset(&AVATPSender_padding_buffer, 0, AVATP_MIN_PACKET_LENGTH);

    /* prepare msghdr header for sendmsg */
    self->packet_header.msg_name = (void*)&self->socket_address;
    self->packet_header.msg_namelen = sizeof(self->socket_address);
    self->packet_header.msg_iov = (void*)&self->packet_data;
    self->packet_header.msg_iovlen = 3;

    self->first_time = 1;
}

int AVATPSender_open(AVATPSender* self, const char* ifname, void* dst_mac,
        u_int64_t stream_id)
{
    if (self == NULL || ifname == NULL || dst_mac == NULL)
        return AVATPSENDER_NULL_ARG;

    /* open socket */
    self->socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_AVA_AVTP));
    if (self->socket == -1)
        return AVATPSENDER_SOCKET_ERROR;

    /* set index */
    self->socket_address.sll_ifindex = if_nametoindex(ifname);

    /* request source MAC address from network interface */
    struct ifreq ifrequest;
    memset(&ifrequest, 0, sizeof(ifrequest));
    ifrequest.ifr_addr.sa_family = AF_PACKET;
    strncpy(ifrequest.ifr_name, ifname, strlen(ifname));
    ioctl(self->socket, SIOCGIFHWADDR, &ifrequest);
    memcpy(self->ethernet_header.h_source, ifrequest.ifr_hwaddr.sa_data,
            ETH_ALEN);

    /* copy destination MAC address */
    memcpy((void*)self->ethernet_header.h_dest, dst_mac, ETH_ALEN);

    /* check stream id compliance */
#if ETH_ALEN > 6
    int len = 6;
#else
    int len = ETH_ALEN;
#endif
    /* PRQA: Lint message 572: deactivation because htobe64 is part of
     * standard Linux headers */
    /*lint -e572*/
    u_int64_t stream_id_mac = stream_id & 0xffffffffffff0000;
    u_int64_t stream_id_mac_be = htobe64(stream_id_mac);
    /*lint +e572*/
    u_int64_t source_mac_be = 0;
    /* PRQA: Lint message 419: deactivation because Lint claims u_int64_t
     * is only 4 bytes while it is actually 8 bytes */
    /*lint -e419*/
    memcpy((void*)&source_mac_be, self->ethernet_header.h_source, len);
    /*lint +e419*/
    if (stream_id_mac_be != source_mac_be)
    {
        AVATP_WARNING("%016" PRIx64 " is not a valid stream id! First %d "
                "bytes should be equal to sender MAC.\n", stream_id, len);
    }

    /* set stream id */
    /* PRQA: Lint message 572: deactivation because htobe64 is part of
     * standard Linux headers */
    /*lint -e572*/
    self->avatp_header.stream_id = htobe64(stream_id);
    /*lint +e572*/

    return 0;
}

int AVATPSender_send(AVATPSender* self, void* header, int header_length, void* payload, int length, avatp_stream_type type)
{
    int result = 0;

    if (self == NULL || payload == NULL)
        return AVATPSENDER_NULL_ARG;

    if (type == AVATP_SENDER_RTP)
      {
        /* read and strip RTP header */
        if (0 != (result = AVATPSender_read_rtp_header(self, payload, length)))
          return result;
        if (0 != (result = AVATPSender_strip_rtp_header(self, &payload, &length)))
          return result;
      }

    if (length > AVTP_MAXIMUM_PAYLOAD)
        return AVATPSENDER_RTP_FRAME_TOO_LARGE;

    /* PRQA: Lint message 644: deactivation because endianess conversion macros
     * (e.g. be16toh) are part of standard Linux headers */
    /*lint -e644 */
    self->avatp_header.stream_data_length = htobe16(length+header_length);
    /*lint +e644 */


    /* update payload pointer and length */
    int packet_data_cnt = 2;

    if(header && header_length)
      {
        self->packet_data[packet_data_cnt].iov_base = header;
        self->packet_data[packet_data_cnt].iov_len = header_length;
        packet_data_cnt++;
      }

    self->packet_data[packet_data_cnt].iov_base = payload;
    self->packet_data[packet_data_cnt].iov_len = length;
    packet_data_cnt++;

    int data_length = self->packet_data[0].iov_len
            + self->packet_data[1].iov_len + header_length + length;

    if (data_length < AVATP_MIN_PACKET_LENGTH)
    {
        /* Ethernet frame is too small: minimum is AVATP_MIN_PACKET_LENGTH bytes
         * including 802.3 header */
        self->packet_data[packet_data_cnt].iov_base = AVATPSender_padding_buffer;
        self->packet_data[packet_data_cnt].iov_len = AVATP_MIN_PACKET_LENGTH - data_length;
        packet_data_cnt++;
    }

    self->packet_header.msg_iovlen = packet_data_cnt;

    /* send */
    while (-1 == (result = sendmsg(self->socket, &self->packet_header, 0)))
    {
        if (errno != EINTR)
            return AVATPSENDER_SEND_ERROR;
    }

    /* PRQA: Lint message 644: deactivation because endianess conversion macros
     * (e.g. be16toh) are part of standard Linux headers */
    /*lint -e644 */
    AVATP_DEBUG("AVATP packet sent with seq#=%d, timestamp=%u length=%d, format_subtype=%d, m0=%d.\n",
            self->avatp_header.sequence_num,
            be32toh(self->avatp_header.avtp_timestamp),
            be16toh(self->avatp_header.stream_data_length),
            be16toh(self->avatp_header.format_subtype),
            self->avatp_header.m0);
    /*lint +e644 */


    /* increase sequence number */
    self->avatp_header.sequence_num++;

    /* for debugging: shape traffic */
    if (self->shape_interval > 0)
    {
        struct timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = self->shape_interval;

        while (-1 == nanosleep(&sleep_time, &sleep_time))
        {
            if (errno != EINTR)
                break;
        }
    }

    return 0;
}

int AVATPSender_set_rtp_header_stripping(AVATPSender* self, int enabled)
{
    if (self == NULL)
        return AVATPSENDER_NULL_ARG;

    /* allowed value range check */
    if (enabled < 0 && enabled > 1)
        return AVATPSENDER_INVALID_ARG;

    self->stripping_enabled = enabled;

    return 0;
}

int AVATPSender_set_vlan_tag(AVATPSender* self, int enabled,
        unsigned int vlan_id, unsigned int vlan_priority, int drop)
{
    if (self == NULL)
        return AVATPSENDER_NULL_ARG;

    /* allowed value range check */
    if ((enabled < 0 && enabled > 1) || (vlan_id != (vlan_id & 0xfff)) || (drop
            != (drop & 0x1)) || (vlan_priority != (vlan_priority & 0x7)))
    {
        return AVATPSENDER_INVALID_ARG;
    }

    self->use_vlan_tag = enabled;

    if (enabled != 0)
    {
        self->ethernet_header.h_proto = htons(ETH_P_8021Q);

        /* PRQA: Lint message 644: deactivation because endianess conversion macros
         * (e.g. be16toh) are part of standard Linux headers */
        /*lint -e644 */
        self->ethernet_header.vlan_tci = htobe16(((vlan_id & 0xfff) | (drop & 0x1)
                << 12 | (vlan_priority & 0x7) << 13));
        /*lint +e644 */

        self->ethernet_header.vlan_proto = htons(ETH_P_AVA_AVTP);

        self->packet_data[0].iov_len = sizeof(struct vlan_ethhdr);
    }
    else
    {
        self->ethernet_header.h_proto = htons(ETH_P_AVA_AVTP);
        self->ethernet_header.vlan_tci = 0;
        self->ethernet_header.vlan_proto = 0;

        self->packet_data[0].iov_len = sizeof(struct ethhdr);
    }

    return 0;
}

int AVATPSender_set_shape_interval(AVATPSender* self,
        unsigned int shape_interval)
{
    if (self == NULL)
        return AVATPSENDER_NULL_ARG;

    if (shape_interval > 999999999)
        return AVATPSENDER_INVALID_ARG;

    self->shape_interval = shape_interval;

    return 0;
}

static int AVATPSender_read_rtp_header(AVATPSender* self, void* payload,
        int length)
{
    (void)length;

    if (self == NULL || payload == NULL)
        return AVATPSENDER_NULL_ARG;

    struct rtp_header* rtp = (struct rtp_header*)payload;

    if (rtp->version != 2)
        return AVATPSENDER_UNSUPPORTED_RTP_FORMAT;

    /* the following fields will be ignored
     * - extension
     * - ssrc
     * - sequence number
     */

    self->avatp_header.avtp_timestamp = rtp->timestamp;
    self->avatp_header.m0 = rtp->marker;

    /* debug output of RTP header values */
    if (self->first_time == 1)
    {
        self->first_time = 0;
        if (self->stripping_enabled != 0)
        {
            AVATP_INFO("first stripped RTP header: version=%d, "
                    "extension=%d, csrc_count=%d, marker=%d, ssrc=0x%x\n",
                    rtp->version, rtp->extension, rtp->csrc_count, rtp->marker,
                    be32toh(rtp->ssrc_identifier));
        }
    }
    else if (self->stripping_enabled != 0)
    {
        AVATP_DEBUG("stripped RTP header: version=%d, "
                "extension=%d, csrc_count=%d, marker=%d, ssrc=0x%x\n",
                rtp->version, rtp->extension, rtp->csrc_count, rtp->marker,
                be32toh(rtp->ssrc_identifier));
    }

    return 0;
}

static int AVATPSender_strip_rtp_header(AVATPSender* self, void** payload,
        int* length)
{
    if (self == NULL || payload == NULL || length == NULL)
        return AVATPSENDER_NULL_ARG;

    int strip_size = 0;
    int padding = 0;
    struct rtp_header* rtp = (struct rtp_header*)*payload;

    if ((unsigned int)*length < sizeof(struct rtp_header))
        return AVATPSENDER_INVALID_RTP_PACKET;

    if (self->stripping_enabled != 0)
    {
        /* remove csrcs */
        strip_size = rtp->csrc_count * 4;

        /* remove padding */
        if (rtp->padding == 1)
        {
            /* last bytes contains padding length */
            unsigned char* last = ((*(unsigned char**)payload) + *length - 1);
            padding = *last;
        }

        /* remove rtp header */
        strip_size += sizeof(struct rtp_header);

        if (strip_size > *length)
            return AVATPSENDER_INVALID_RTP_PACKET;

        *(unsigned char**)payload += strip_size;
        *length -= strip_size + padding;
    }

    return 0;
}

/*lint +e160*/
/*lint +e774*/
