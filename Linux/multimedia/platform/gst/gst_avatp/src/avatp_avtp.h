/**
 * \file: avatp_avtp.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * IEEE 1722 AVTP header structure used for AVATP.
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

#ifndef __AVATP_AVTP_H__
#define __AVATP_AVTP_H__

#define ETH_P_AVA_AVTP          0x22f0

#define AVTP_HEADER_LENGTH      24
#define AVTP_MAXIMUM_PAYLOAD	1476

#define BITS_BIG_ENDIAN 1


/* PRQA: Lint Message 826: deactivation as bitfied handling is specific to gcc */
/*lint -e46*/
/* header is defined in big endian */
struct _AVTPHeader
{
#if __BYTE_ORDER == __BIG_ENDIAN
    /* 4 bytes */
    u_int8_t control_data :1;
    u_int8_t subtype :7;
    u_int8_t streamid_valid :1;
    u_int8_t version :3;
    u_int8_t media_clock_restart :1;
    u_int8_t r :1;
    u_int8_t gateway_info_valid :1;
    u_int8_t avtp_timestamp_valid :1;
    u_int8_t sequence_num;
    u_int8_t reserved :7;
    u_int8_t timestamp_uncertain :1;

    /* 8 bytes */
    u_int64_t stream_id;

    /* 4 bytes */
    u_int32_t avtp_timestamp;

    /* 4 bytes */
    u_int8_t format;
    u_int8_t drm :4;
    u_int8_t reserved1 :4;
    u_int16_t format_subtype;

    /* 4 bytes */
    u_int16_t stream_data_length;
    u_int16_t reserved2 :2;
    u_int16_t m3 :1;
    u_int16_t m2 :1;
    u_int16_t m1 :1;
    u_int16_t m0 :1;
    u_int16_t reserved3 :10;
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
    /* 4 bytes */
    u_int8_t subtype :7;
    u_int8_t control_data :1;
    u_int8_t avtp_timestamp_valid :1;
    u_int8_t gateway_info_valid :1;
    u_int8_t r :1;
    u_int8_t media_clock_restart :1;
    u_int8_t version :3;
    u_int8_t streamid_valid :1;
    u_int8_t sequence_num;
    u_int8_t timestamp_uncertain :1;
    u_int8_t reserved :7;

    /* 8 bytes */
    u_int64_t stream_id;

    /* 4 bytes */
    u_int32_t avtp_timestamp;

    /* 4 bytes */
    u_int8_t format;
    u_int8_t reserved1 :4;
    u_int8_t drm :4;
    u_int16_t format_subtype;

    /* 4 bytes */
    u_int16_t stream_data_length;
    u_int16_t reserved3 :2;
    u_int16_t m0 :1;
    u_int16_t m1 :1;
    u_int16_t m2 :1;
    u_int16_t m3 :1;
    u_int16_t reserved2 :10;
#endif
}__attribute__((packed));
/*lint +e46*/

typedef struct _AVTPHeader AVTPHeader;

#endif /* __AVATP_AVTP_H__ */
