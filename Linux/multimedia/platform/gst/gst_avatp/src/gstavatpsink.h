/**
 * \file: gstavatpsink.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * Gstreamer sink element avatpsink.
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

#ifndef __GST_AVATPSINK_H__
#define __GST_AVATPSINK_H__

#include <linux/if_ether.h>
#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "avatp_sender.h"

G_BEGIN_DECLS

/* iCam CR related changes */
#define START_CODE_LEN 			4
#define SPS_TYPE_ID  			7
#define PPS_TYPE_ID  			8

#define FU_HEADER_MASK_VAL		0x1f
#define START_LEFT_SHIFT_VAL	7
#define END_LEFT_SHIFT_VAL		6
#define FU_INDICATOR_CONST_VAL	28
#define FU_INDICATOR_MASK_VAL	0x60

typedef struct _GstAVATPSink GstAVATPSink;
typedef struct _GstAVATPSinkClass GstAVATPSinkClass;

struct _GstAVATPSink
{
    GstBaseSink element;

    gboolean socket_open;
    gboolean properties_locked;

    guchar src_mac[ETH_ALEN];
    guchar dst_mac[ETH_ALEN];

    GstPad *sinkpad, *srcpad;

    guint64 stream_id;
    gchar* interface;
    gchar* address;
    gboolean strip_rtp_header;
    guint max_mtu_size;

    gboolean use_vlan_tag;
    guint vlan_id;
    guint vlan_priority;
    gboolean vlan_drop;

    guint shape_interval;

    AVATPSender sender;

    /* For first and last frame buffer transmitted */
    avatp_stream_type media_type;
};

struct _GstAVATPSinkClass
{
    GstBaseSinkClass parent_class;
    void (*avatp_first_frame_data) (GstElement* element, guint length, gpointer buffer);
    void (*avatp_last_frame_data) (GstElement* element, guint length, gpointer buffer);
};

#define GST_TYPE_AVATPSINK \
    (gst_avatpsink_get_type())
#define GST_AVATPSINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AVATPSINK,GstAVATPSink))

GType gst_avatpsink_get_type(void);

G_END_DECLS

#endif /* __GST_AVATPSINK_H__ */
