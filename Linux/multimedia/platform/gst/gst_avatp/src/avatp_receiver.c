/**
 * \file: avatp_receiver.c
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * AVATP receiver module implementation.
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

#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <errno.h>
#include <endian.h>

#define AVATPRECEIVER_IMPL

#include "avatp_receiver.h"
#include "avatp_avtp.h"
#include "avatp_rtp.h"
#include "avatp_vlan.h"
#include "avatp_log.h"

/* defined in net/if.h */
extern unsigned int if_nametoindex(const char *__ifname);

/* PRQA: Lint Message 774: deactivation because any GST_DEBUG thrown this info */
/*lint -e774*/


/* PRQA: Lint message 160: deactivation because endianess conversion macros
 * (e.g. be16toh) are part of standard Linux headers */
/*lint -e160*/

struct recv_buffer
{
    struct vlan_ethhdr* ethernet_header;
    AVTPHeader* avatp_header;
    char* payload;
    int payload_length;

    char* buffer;
};

/* AVATPReceiver private methods */
static int AVATPReceiver_create_rtp_frame(AVATPReceiver* self,
        struct avatp_received_frame* rtp_frame, struct recv_buffer* buffer);
static void AVATPReceiver_write_rtp_header(AVATPReceiver* self,
        AVTPHeader* avatp_header, struct avatp_received_frame* rtp_frame);
static void AVATPReceiver_update_sequence_numbers(AVATPReceiver* self,
        AVTPHeader* avatp_header);
static int AVATPReceiver_check_ethertype(AVATPReceiver* self,
        struct recv_buffer* buffer);
static int AVATPReceiver_validate_frame(AVATPReceiver* self,
        struct recv_buffer* buffer);
static int AVATPReceiver_recv(AVATPReceiver* self, struct recv_buffer* buffer,
        int max_read);

void AVATPReceiver_init(AVATPReceiver* self)
{
    if (self == NULL)
        return;

    memset(self, 0, sizeof(AVATPReceiver));

    self->first_time = 1;
}

int AVATPReceiver_open(AVATPReceiver* self, const char* ifname,
        u_int64_t stream_id)
{
    if (self == NULL || ifname == NULL)
        return AVATPRECEIVER_NULL_ARG;

    self->stream_id = stream_id;

    /* create socket */
    /* TODO: do not use ETH_P_ALL */
    self->socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (self->socket < 0)
        return AVATPRECEIVER_SOCKET_ERROR;

    /* socket recv buffer size */
    int size = 1024 * 1024 * 2; /* 2MB */
    if (0 != setsockopt(self->socket, SOL_SOCKET, SO_RCVBUF, (void*)&size, sizeof(size)))
        AVATP_DEBUG("could not set receive buffer size to %d bytes.\n", size);
    size = 0;
    unsigned int sizeofsize = sizeof(size);
    if (0 != getsockopt(self->socket, SOL_SOCKET, SO_RCVBUF, (void*)&size, &sizeofsize))
        AVATP_DEBUG("could not read current receive buffer size.\n");
    else
        AVATP_DEBUG("socket receive buffer size is %d bytes.\n", size);

    /* get device index from interface name */
    struct sockaddr_ll socket_address;
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_ALL);
    socket_address.sll_ifindex = if_nametoindex(ifname);

    /* bind socket */
    if (0 > bind(self->socket, (struct sockaddr *)&socket_address,
            sizeof(socket_address)))
    {
        return AVATPRECEIVER_SOCKET_ERROR;
    }

    /* get mac address from interface name */
    struct ifreq ifrequest;
    memset(&ifrequest, 0, sizeof(ifrequest));
    ifrequest.ifr_addr.sa_family = AF_PACKET;
    strncpy(ifrequest.ifr_name, ifname, strlen(ifname));
    ioctl(self->socket, SIOCGIFHWADDR, &ifrequest);
    memcpy(self->src_mac, ifrequest.ifr_hwaddr.sa_data, ETH_ALEN);

    return self->socket;
}

void AVATPReceiver_close(AVATPReceiver* self)
{
    if (self->socket > 0)
    {
        if (-1 == close(self->socket))
        {
            /* Ignore EINTR and risk a possible socket leak to avoid other
             * problems. */
        }
        self->socket = 0;
    }
}

int AVATPReceiver_receive(AVATPReceiver* self,
        struct avatp_received_frame* rtp_frame)
{
    if (self == NULL || rtp_frame == NULL)
        return AVATPRECEIVER_NULL_ARG;

    if (rtp_frame->malloc_pointer == NULL || rtp_frame->size <= 0)
    {
        return AVATPRECEIVER_INVALID_ARG;
    }

    /* read directly to RTP frame buffer to avoid later memcpy */
    struct recv_buffer buffer;
    buffer.buffer = rtp_frame->malloc_pointer;

    /* read datagram buffer from socket */
    int result = AVATPReceiver_recv(self, &buffer, rtp_frame->size);
    if (result != 0)
        return result;

    /* check EtherType and modify payload pointers if necessary */
    result = AVATPReceiver_check_ethertype(self, &buffer);
    if (result != 0)
        return result;

    /* check stream id */
    /* PRQA: Lint message 572: deactivation because htobe64 is part of
     * standard Linux headers */
    /*lint -e572*/
    if (buffer.avatp_header->stream_id != htobe64(self->stream_id))
    /*lint +e572*/
    {
        /* This approach has bad performance for stream count > 1.
         * Although it works consider a centralized receiver if such
         * use case becomes necessary!
         */
        return AVATPRECEIVER_PACKET_DISCARD;
    }

    /* validate AVATP frame */
    result = AVATPReceiver_validate_frame(self, &buffer);
    if (result != 0)
        return result;

    /* PRQA: Lint message 644: deactivation because endianess conversion macros
     * (e.g. be16toh) are part of standard Linux headers */
    /*lint -e644 */
    AVATP_DEBUG("AVATP packet received with seq#=%d, timestamp=%u, "
            "length=%d, format_subtype=%d, m0=%d.\n",
            buffer.avatp_header->sequence_num,
            be32toh(buffer.avatp_header->avtp_timestamp),
            be16toh(buffer.avatp_header->stream_data_length),
            be16toh(buffer.avatp_header->format_subtype),
            buffer.avatp_header->m0);
    /*lint +e644 */

    /* check and update sequence numbers */
    AVATPReceiver_update_sequence_numbers(self, buffer.avatp_header);

    /* create rtp frame and copy payload */
    result = AVATPReceiver_create_rtp_frame(self, rtp_frame, &buffer);
    if (result != 0)
        return result;

    return 0;
}

int AVATPReceiver_set_rtp_header_stripping(AVATPReceiver* self, int enabled)
{
    if (self == NULL)
        return AVATPRECEIVER_NULL_ARG;

    /* allowed value range check */
    if (enabled < 0 && enabled > 1)
        return AVATPRECEIVER_INVALID_ARG;

    self->stripping_enabled = enabled;

    return 0;
}

int AVATPReceiver_set_rtp_header_values(AVATPReceiver* self,
        unsigned char version, unsigned char extension, unsigned int ssrc)
{
    if (self == NULL)
        return AVATPRECEIVER_NULL_ARG;

    /* allowed value range check */
    if (version > 2 || extension > 1)
        return AVATPRECEIVER_INVALID_ARG;

    self->rtp_header.version = version;
    self->rtp_header.padding = 0;
    self->rtp_header.extension = extension;
    self->rtp_header.csrc_count = 0;
    self->rtp_header.ssrc_identifier = ssrc;

    return 0;
}

int AVATPReceiver_get_receive_length(AVATPReceiver* self, int* length)
{
    if (self == NULL || length == NULL)
        return AVATPRECEIVER_NULL_ARG;

    *length = 0;
    if (ioctl(self->socket, FIONREAD, length) < 0)
        return AVATPRECEIVER_READ_ERROR;

    return 0;
}

static int AVATPReceiver_create_rtp_frame(AVATPReceiver* self,
        struct avatp_received_frame* rtp_frame, struct recv_buffer* buffer)
{
    if (self->stripping_enabled == 0)
    {
        /* RTP header included in payload */

        /* move RTP frame pointer to payload begin */
        rtp_frame->data = buffer->payload;

        /* PRQA: Lint message 644: deactivation because endianess conversion macros
         * (e.g. be16toh) are part of standard Linux headers */
        /*lint -e644 */
        rtp_frame->size = be16toh(buffer->avatp_header->stream_data_length);
        /*lint +e644 */
    }
    else
    {
        /* RTP header has to be newly created */

        /* move RTP frame pointer to payload begin minus RTP header size */
        rtp_frame->data = buffer->payload - sizeof(struct rtp_header);

        /* PRQA: Lint message 644: deactivation because endianess conversion macros
             * (e.g. be16toh) are part of standard Linux headers */
        /*lint -e644 */
        rtp_frame->size = sizeof(struct rtp_header) + be16toh(
                buffer->avatp_header->stream_data_length);
        /*lint +e644 */

        /* replace AVATP header with RTP */
        AVATPReceiver_write_rtp_header(self, buffer->avatp_header, rtp_frame);
    }

    return 0;
}

static void AVATPReceiver_write_rtp_header(AVATPReceiver* self,
        AVTPHeader* avatp_header, struct avatp_received_frame* rtp_frame)
{
    /* first read values */
    self->rtp_header.payload_type = RTP_DYNAMIC_PAYLOAD_TYPE;

    /* PRQA: Lint message 644: deactivation because endianess conversion macros
     * (e.g. be16toh) are part of standard Linux headers */
    /*lint -e644 */
    self->rtp_header.sequence_number = htobe16(avatp_header->sequence_num
            | (self->rtp_sequence_number_high << 8));
    /*lint +e644 */

    self->rtp_header.timestamp = avatp_header->avtp_timestamp;
    self->rtp_header.marker = avatp_header->m0;

    /* then copy back the RTP header */
    memcpy(rtp_frame->data, &self->rtp_header, sizeof(struct rtp_header));
}

static void AVATPReceiver_update_sequence_numbers(AVATPReceiver* self,
        AVTPHeader* avatp_header)
{
    /* check sequence number loss */
    if (self->first_time == 0 && avatp_header->sequence_num
            != (unsigned char)(self->last_sequence_number + 1))
    {
        AVATP_WARNING("AVATP packet sequence number is %d, "
                "but %d was expected! timestamp=0x%x\n", avatp_header->sequence_num,
                (unsigned char)(self->last_sequence_number + 1),
                be32toh(avatp_header->avtp_timestamp));
    }

    /* increase high byte of RTP sequence number if we had an AVATP sequence
     * number overflow */
    if (self->stripping_enabled != 0 && self->last_sequence_number
            > avatp_header->sequence_num)
    {
        self->rtp_sequence_number_high++;
    }

    /* update/init sequencing */
    self->last_sequence_number = avatp_header->sequence_num;
    if (self->first_time == 1)
        self->first_time = 0;
}

static int AVATPReceiver_check_ethertype(AVATPReceiver* self,
        struct recv_buffer* buffer)
{
    (void)self;

    if (buffer->ethernet_header->h_proto != htons(ETH_P_AVA_AVTP))
    {
        /* check for 802.1Q VLAN tag */
        if (buffer->ethernet_header->h_proto == htons(ETH_P_8021Q) &&
                buffer->ethernet_header->vlan_proto == htons(ETH_P_AVA_AVTP))
        {
            /* move pointers */
            buffer->avatp_header = (AVTPHeader*)(void*)(buffer->buffer
                    + sizeof(struct vlan_ethhdr));
            buffer->payload = buffer->buffer + sizeof(struct vlan_ethhdr)
                    + sizeof(AVTPHeader);
            buffer->payload_length -= 4;
            if (buffer->payload_length < 0)
                return AVATPRECEIVER_INVALID_PACKET;
        }
        else
        {
            /* This should never happen! */
            return AVATPRECEIVER_PACKET_DISCARD;
        }
    }

    return 0;
}

static int AVATPReceiver_validate_frame(AVATPReceiver* self,
        struct recv_buffer* buffer)
{
    /* first time only checks */
    if (self->first_time == 1)
    {
        /* check if RTP header is removed in stripping mode
         * note: this can result in false positives, so warn only once
         *  */
        if (self->stripping_enabled == 1)
        {
            struct rtp_header* rtp = (struct rtp_header*)(void*)buffer->payload;
            if (rtp->version == 2 && rtp->payload_type == RTP_DYNAMIC_PAYLOAD_TYPE)
            {
                /* inconsistent, but might still work */
                AVATP_WARNING("RTP header stripping is enabled, but payload "
                        "seems to still include the RTP header!\n");
            }
        }
    }

    /* check if RTP header is available in non-stripping mode */
    if (self->stripping_enabled == 0)
    {
        struct rtp_header* rtp = (struct rtp_header*)(void*)buffer->payload;
        if (rtp->version != 2 || rtp->payload_type != RTP_DYNAMIC_PAYLOAD_TYPE)
        {
            /* inconsistent, but might still work */
            AVATP_WARNING("RTP header stripping is disabled, but payload does "
                    "not include a valid RTP header!\n");
        }
    }

    /* PRQA: Lint message 644: deactivation because endianess conversion macros
     * (e.g. be16toh) are part of standard Linux headers */
    /*lint -e644 */
    if (be16toh(buffer->avatp_header->stream_data_length)
            > (unsigned int)buffer->payload_length)
    {
        /* corrupt data, return error */
        AVATP_WARNING("AVATP stream_data_length=%d is longer than actual "
                "payload=%d!\n",
                be16toh(buffer->avatp_header->stream_data_length),
                buffer->payload_length);
        return AVATPRECEIVER_INVALID_PACKET;
    }
    /*lint +e644 */

    return 0;
}

static int AVATPReceiver_recv(AVATPReceiver* self, struct recv_buffer* buffer,
        int max_read)
{
    /* check whether data is available to read */
    int bytes_read = 0;
    int result = ioctl(self->socket, FIONREAD, &bytes_read);
    if (result < 0)
        return AVATPRECEIVER_READ_ERROR;
    if (bytes_read <= 0)
        return AVATPRECEIVER_PACKET_DISCARD;

    /* read */
    do
    {
        struct sockaddr sa;
        socklen_t slen = sizeof(sa);

        buffer->payload_length = recvfrom(self->socket, buffer->buffer,
                max_read, 0, &sa, &slen);
        if (buffer->payload_length < 0 && errno != EAGAIN && errno != EINTR)
            return AVATPRECEIVER_READ_ERROR;
    } while (buffer->payload_length < 0);

    /* cast headers structures */
    buffer->ethernet_header = (struct vlan_ethhdr*)(void*)buffer->buffer;
    buffer->avatp_header = (AVTPHeader*)(void*)(buffer->buffer
            + sizeof(struct ethhdr));
    buffer->payload = buffer->buffer + sizeof(struct ethhdr)
            + sizeof(AVTPHeader);

    /* calculate payload length */
    buffer->payload_length -= sizeof(struct ethhdr) + sizeof(AVTPHeader);

    if (buffer->payload_length < 0)
        return AVATPRECEIVER_INVALID_PACKET;

    return 0;
}

const unsigned char* AVATPReceiver_get_source_mac(AVATPReceiver* self)
{
    if (self == NULL)
        return NULL;

    return self->src_mac;
}

/*lint +e160*/
/*lint +e774*/
