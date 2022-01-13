/**
 * \file: gstavatpsrc.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * Gstreamer source element avatpsink.
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

#ifndef __GST_AVTPSRC_H__
#define __GST_AVTPSRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

#include "avatp_receiver.h"

G_BEGIN_DECLS

typedef struct _GstAVATPSrc GstAVATPSrc;
typedef struct _GstAVATPSrcClass GstAVATPSrcClass;

struct _GstAVATPSrc
{
	/* internal */
    GstPushSrc parent;
    guchar src_mac[ETH_ALEN];
    guint max_frame_size;

    guint64 timeout;
    GstPad* srcpad;

    GstPollFD sock;
    GstPoll* fdset;

    /* avatp object */
    AVATPReceiver receiver;

    /* general properties */
    gboolean properties_locked;
    guint64 stream_id;
    gchar* interface;

    /* RTP related properties */
    gboolean gen_rtp_header;
    guint rtp_version;
    gboolean rtp_extension;
    gboolean rtp_marker;
    guint rtp_ssrc;
};

struct _GstAVATPSrcClass
{
    GstPushSrcClass parent_class;
};

#define GST_TYPE_AVATPSRC \
    (gst_avatpsrc_get_type())
#define GST_AVATPSRC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AVATPSRC,GstAVATPSrc))

GType gst_avatpsrc_get_type(void);

G_END_DECLS

#endif /* __GST_AVTPSRC_H__ */
