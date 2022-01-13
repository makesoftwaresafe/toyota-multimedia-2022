/**
 * \file: avatp_rtp.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * RTP header structure.
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

#ifndef __AVATP_RTP_H__
#define __AVATP_RTP_H__


#define RTP_DYNAMIC_PAYLOAD_TYPE 96

/* PRQA: Lint Message 826: deactivation as bitfied handling is specific to gcc */
/*lint -e46*/
/* data is big endian */
struct rtp_header
{
    struct
    {
#if __BYTE_ORDER == __BIG_ENDIAN
        u_int8_t version :2;
        u_int8_t padding :1;
        u_int8_t extension :1;
        u_int8_t csrc_count :4;

        u_int8_t marker :1;
        u_int8_t payload_type :7;

        u_int16_t sequence_number;
#endif
#if __BYTE_ORDER == __LITTLE_ENDIAN
        u_int8_t csrc_count :4;
        u_int8_t extension :1;
        u_int8_t padding :1;
        u_int8_t version :2;

        u_int8_t payload_type :7;
        u_int8_t marker :1;

        u_int16_t sequence_number :16;
#endif
    }__attribute__((packed));

    u_int32_t timestamp;
    u_int32_t ssrc_identifier;
}__attribute__((packed));
/*lint +e46*/
#endif /* __AVATP_RTP_H__ */
