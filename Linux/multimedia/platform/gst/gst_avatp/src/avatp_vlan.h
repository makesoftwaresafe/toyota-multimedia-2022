/**
 * \file: avatp_vlan.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * Ethernet 802.3 frame structure with 802.1Q VLAN tag.
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

#ifndef __AVATP_VLAN_H__
#define __AVATP_VLAN_H__

#include <sys/types.h>
#include <linux/if_ether.h>

struct vlan_ethhdr
{
    unsigned char h_dest[ETH_ALEN];
    unsigned char h_source[ETH_ALEN];
    __be16 h_proto;
    __be16 vlan_tci;
    __be16 vlan_proto;
}__attribute__((packed));

#endif /* __AVATP_VLAN_H__ */
