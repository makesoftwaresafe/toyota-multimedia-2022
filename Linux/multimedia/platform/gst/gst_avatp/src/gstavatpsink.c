/**
 * \file: gstavatpsink.c
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

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <sys/types.h>
#include <regex.h>
#include <gst/gst.h>
#include "avatp_avtp.h"
#include "gstavatpsink.h"

/* PRQA: Lint Message 774: deactivation because any GST_DEBUG thrown this info */
/*lint -e774*/


/* PRQA: Lint Message 826: deactivation because casting mechanism of GObject
 * throws the finding */
/*lint -e826*/
#define AVATP_ADDRESS_DEFAULT 			NULL
#define AVATP_INTERFACE_DEFAULT			(const gchar*)"eth0"
#define AVATP_STREAM_ID_MIN				0
#define AVATP_STREAM_ID_MAX				G_MAXUINT64
#define AVATP_STREAM_ID_DEFAULT 		0
#define AVATP_RTP_STRIP_HEADER_DEFAULT	TRUE
#define AVATP_VLAN_USE_DEFAULT			TRUE
#define AVATP_VLAN_ID_MIN				0
#define AVATP_VLAN_ID_MAX				0xfff
#define AVATP_VLAN_ID_DEFAULT			0x56
#define AVATP_VLAN_PRIORITY_MIN			0
#define AVATP_VLAN_PRIORITY_MAX			7
#define AVATP_VLAN_PRIORITY_DEFAULT		0x5
#define AVATP_VLAN_DROP_DEFAULT			FALSE
#define AVATP_SHAPE_INTERVAL_MIN        0
#define AVATP_SHAPE_INTERVAL_MAX        999999999
#define AVATP_SHAPE_INTERVAL_DEFAULT    0
#define AVATP_MAX_MTU_DEFAULT           1400

GST_DEBUG_CATEGORY_EXTERN(gst_avatp_debug);
#define GST_CAT_DEFAULT gst_avatp_debug

enum
{
    PROP_STREAM_ID = 1,
    PROP_INTERFACE,
    PROP_ADDRESS,
    PROP_STRIP_RTP_HEADER,
    PROP_USE_VLAN_TAG,
    PROP_VLAN_ID,
    PROP_VLAN_PRIORITY,
    PROP_VLAN_DROP,
    PROP_SHAPE_INTERVAL,
    PROP_MAX_MTU_SIZE
};

enum
{
  SIGNAL_AVATP_FIRST_FRAME_DATA,
  SIGNAL_AVATP_LAST_FRAME_DATA,
  SIGNAL_AVATP_FRAME_DATA_NONE
};

static guint gst_avatp_sink_signals[SIGNAL_AVATP_FRAME_DATA_NONE] = { 0 };

/* PRQA: Lint message 19, 123, 144, 751, 160, 528: deactivation because macro is related to Gstreamer framework */
/*lint -e19 -e123 -e144 -e751 -e160 -e528*/
/*lint -save -e528*/
#if GST_CHECK_VERSION(1,0,0)
G_DEFINE_TYPE(GstAVATPSink, gst_avatpsink, GST_TYPE_BASE_SINK)
#else
GST_BOILERPLATE(GstAVATPSink, gst_avatpsink, GstBaseSink, GST_TYPE_BASE_SINK)
#endif
;
/*lint +e19 +e123 +e144 +e751 +e160 +e528*/
/*lint -restore*/

static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE("sink",
		GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("application/x-rtp" ";"
				"video/x-h264" ";"
		));

static void gst_avatpsink_finalize(GstAVATPSink *avatpsink);
static void gst_avatpsink_set_property(GObject *object, guint prop_id,
        const GValue *value, GParamSpec *pspec);
static void gst_avatpsink_get_property(GObject *object, guint prop_id,
        GValue *value, GParamSpec *pspec);
static GstFlowReturn gst_avatpsink_render(GstBaseSink* sink,
        GstBuffer* buffer);
static void gst_avatpsink_log_sending(GstAVATPSink* sink);

static int gst_avatpsink_open_sender(GstAVATPSink* sink);

#if !GST_CHECK_VERSION(1,0,0)
gboolean gst_avatpsink_setcaps(GstPad *pad, GstCaps *caps);
#endif

#if GST_CHECK_VERSION(1,0,0)
static gboolean gst_avatpsink_event (GstBaseSink *bsink, GstEvent *event)
{
    GstAVATPSink *sink = GST_AVATPSINK(bsink);
    if (sink != NULL && GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
        GstCaps *caps = NULL;
        GstStructure *structure = NULL;
        const char *type = NULL;

        gst_event_parse_caps(event, &caps);
        structure = gst_caps_get_structure(caps, 0);
        type = gst_structure_get_name(structure);

        GST_DEBUG("got type: %s", type);
        if (0 == strcasecmp(type, "video/x-h264"))
        {
            sink->media_type = AVATP_SENDER_H264;
            return TRUE;
        }
        else if (0 == strcasecmp(type, "application/x-rtp"))
        {
            sink->media_type = AVATP_SENDER_RTP;
            return TRUE;
        }
        else
        {
            return FALSE;
        }

    }

    return GST_BASE_SINK_CLASS(gst_avatpsink_parent_class)->event(bsink, event);
}
#endif

static gboolean get_address_from_string(gchar* mac_string, guchar* mac_bytes)
{
    if (strlen(mac_string) != (ETH_ALEN * 3 - 1))
        return FALSE;

    int i;
    for (i = 0; i < ETH_ALEN; i++)
    {
        int result = sscanf(mac_string, "%2hhx", &mac_bytes[i]);
        if (result != 1)
            return FALSE;
        if (i < ETH_ALEN - 1 && mac_string[2] != ':')
            return FALSE;
        mac_string += 3;
    }

    return TRUE;
}

#if !GST_CHECK_VERSION(1,0,0)
static void gst_avatpsink_base_init(gpointer gclass)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS(gclass);

    gst_element_class_set_details_simple(element_class, "AVATP packet sender",
            "Sink/Network", "Send data over the network via AVATP",
            "Jakob Harder <jharder@de.adit-jv.com>");

    gst_element_class_add_pad_template(element_class,
            gst_static_pad_template_get(&sink_factory));
}
#endif

static void gst_avatpsink_class_init(GstAVATPSinkClass *klass)
{
    GObjectClass *gobject_class;
    GstBaseSinkClass *gstbasesink_class;

    gobject_class = (GObjectClass *)klass;
    gstbasesink_class = (GstBaseSinkClass *)klass;

#if GST_CHECK_VERSION(1,0,0)
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_details_simple(element_class, "AVATP packet sender",
            "Sink/Network", "Send data over the network via AVATP",
            "Jakob Harder <jharder@de.adit-jv.com>");

    gst_element_class_add_pad_template(element_class,
            gst_static_pad_template_get(&sink_factory));

    gstbasesink_class->event = gst_avatpsink_event;
#endif

    gobject_class->set_property = gst_avatpsink_set_property;
    gobject_class->get_property = gst_avatpsink_get_property;

    gobject_class->finalize = (GObjectFinalizeFunc)gst_avatpsink_finalize;

    g_object_class_install_property(gobject_class, PROP_STREAM_ID,
            g_param_spec_uint64("id", "StreamID", "The AVATP stream ID.",
                    AVATP_STREAM_ID_MIN, AVATP_STREAM_ID_MAX,
                    AVATP_STREAM_ID_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_INTERFACE,
            g_param_spec_string("if", "interface",
                    "The interface to send the AVATP packets with",
                    AVATP_INTERFACE_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_ADDRESS,
            g_param_spec_string("address", "address",
                    "The MAC device to send the AVATP packets to",
                    AVATP_ADDRESS_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_STRIP_RTP_HEADER,
            g_param_spec_boolean("strip", "Strip RTP header mode",
                    "Use strip RTP header mode and strip RTP header from payloads ?",
                    AVATP_RTP_STRIP_HEADER_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_USE_VLAN_TAG,
            g_param_spec_boolean("use-vlan-tag", "Use VLAN tag",
                    "Use VLAN tags in packets ?", AVATP_VLAN_USE_DEFAULT,
                    G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_VLAN_ID,
            g_param_spec_uint("vlan-id", "VLAN Identifier",
                    "Field specifying the VLAN to which the frames belong.",
                    AVATP_VLAN_ID_MIN, AVATP_VLAN_ID_MAX,
                    AVATP_VLAN_ID_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_VLAN_PRIORITY,
            g_param_spec_uint("vlan-priority", "VLAN Prority Code Point",
                    "Field specifying the 802.1p priority of the frames.",
                    AVATP_VLAN_PRIORITY_MIN, AVATP_VLAN_PRIORITY_MAX,
                    AVATP_VLAN_PRIORITY_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_VLAN_DROP,
            g_param_spec_boolean("vlan-drop", "VLAN Drop Eligible",
                    "Field indicating frames may be dropped in case of congestion.",
                    AVATP_VLAN_DROP_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_SHAPE_INTERVAL,
            g_param_spec_uint("interval", "Minimum packet interval",
                    "For debugging only: shape the packet traffic with the specified interval in nanoseconds per packet. 0 means deactivated.",
                    AVATP_SHAPE_INTERVAL_MIN, AVATP_SHAPE_INTERVAL_MAX,
                    AVATP_SHAPE_INTERVAL_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_MAX_MTU_SIZE,
            g_param_spec_uint("max_mtu_size", "Maximum MTU size", "Maximum MTU size (for backward compatiblity MTU=MTU-sizeof(RTPHeader)+sizeof(AVATPHeader))",
                    0, G_MAXUINT32,
                    AVATP_MAX_MTU_DEFAULT, G_PARAM_READWRITE));

     /*
      * Add signal. To notify the application about the
      * first frame data to be send out, so that application
      * will validate and is data is corrupt memset data buffer
      * to '0'. Either will send validate or '0' data out.
      */
     gst_avatp_sink_signals[SIGNAL_AVATP_FIRST_FRAME_DATA] =
         g_signal_new ("avatp-first-frame-data",
             G_TYPE_FROM_CLASS (klass),
             G_SIGNAL_RUN_FIRST,
             G_STRUCT_OFFSET (GstAVATPSinkClass, avatp_first_frame_data),
             NULL, NULL,
             g_cclosure_marshal_VOID__UINT_POINTER,
             G_TYPE_NONE, 2,
             G_TYPE_UINT,
             G_TYPE_POINTER);

     /*
      * Add signal. To notify the application about the
      * last frame data has been sent out, so that application
      * will memset data buffer to '0'. By doing this application
      * confirms that cyclic repeating frames are not been transmitted
      * during vpu encoder error case.
      */
     gst_avatp_sink_signals[SIGNAL_AVATP_LAST_FRAME_DATA] =
         g_signal_new ("avatp-last-frame-data",
             G_TYPE_FROM_CLASS (klass),
             G_SIGNAL_RUN_FIRST,
             G_STRUCT_OFFSET (GstAVATPSinkClass, avatp_last_frame_data),
             NULL, NULL,
             g_cclosure_marshal_VOID__OBJECT,
             G_TYPE_NONE, 1,
             GST_TYPE_BUFFER);

    gstbasesink_class->render = gst_avatpsink_render;
}

#if GST_CHECK_VERSION(1,0,0)
static void gst_avatpsink_init(GstAVATPSink* sink)
{
#else
static void gst_avatpsink_init(GstAVATPSink* sink, GstAVATPSinkClass* gclass)
{
    (void)gclass;
#endif

    sink->properties_locked = FALSE;

    /* init default properties */
    sink->stream_id = AVATP_STREAM_ID_DEFAULT;
    sink->interface = g_strdup(AVATP_INTERFACE_DEFAULT);
    sink->address = g_strdup(AVATP_ADDRESS_DEFAULT);
    sink->strip_rtp_header = AVATP_RTP_STRIP_HEADER_DEFAULT;
    sink->use_vlan_tag = AVATP_VLAN_USE_DEFAULT;
    sink->vlan_id = AVATP_VLAN_ID_DEFAULT;
    sink->vlan_priority = AVATP_VLAN_PRIORITY_DEFAULT;
    sink->vlan_drop = AVATP_VLAN_DROP_DEFAULT;
    sink->shape_interval = AVATP_SHAPE_INTERVAL_DEFAULT;
    sink->max_mtu_size = AVATP_MAX_MTU_DEFAULT;

    /* push out data as fast as possible -> disable clock synchronization */
    gst_base_sink_set_sync(&sink->element, FALSE);

    sink->socket_open = FALSE;

#if !GST_CHECK_VERSION(1,0,0)
    gst_pad_set_setcaps_function(GST_BASE_SINK_PAD(sink), GST_DEBUG_FUNCPTR(
        gst_avatpsink_setcaps));
#endif

    /* initialize sender to default state */
    AVATPSender_init(&sink->sender);



}

static void gst_avatpsink_finalize(GstAVATPSink * avbtpsink)
{
    g_free(avbtpsink->address);
    g_free(avbtpsink->interface);

#if GST_CHECK_VERSION(1,0,0)
    G_OBJECT_CLASS(gst_avatpsink_parent_class)->finalize((GObject *)avbtpsink);
#else
    G_OBJECT_CLASS(parent_class)->finalize((GObject *)avbtpsink);
#endif
}


#if !GST_CHECK_VERSION(1,0,0)
gboolean gst_avatpsink_setcaps(GstPad *pad, GstCaps *caps)
{
    GstAVATPSink *sink = GST_AVATPSINK(GST_PAD_PARENT(pad));
    GstStructure * structure = gst_caps_get_structure(caps, 0);
    const gchar * type = gst_structure_get_name(structure);

    GST_DEBUG("got type: %s", type);
    if (0 == strcasecmp(type, "video/x-h264"))
      {
        sink->media_type = AVATP_SENDER_H264;
      }
    else if (0 == strcasecmp(type, "application/x-rtp"))
      {
        sink->media_type = AVATP_SENDER_RTP;
      }
    else
      {
        return FALSE;
      }

    return TRUE;
}
#endif

static GstFlowReturn
gst_avatpsink_transmit_data(GstAVATPSink *sink, guint8 *data, guint size,
    GstClockTime timestamp)
{
  GstFlowReturn result = GST_FLOW_OK;
  guint8 nal_header = *data;
  guint transmit_size = 0;
  guint payload_len = gst_rtp_buffer_calc_payload_len(sink->max_mtu_size, 0, 0);

  gint start = 1;
  gint end = 0;
  gint position = 0;
  gboolean is_frame = FALSE;

  guint8 header[2];
  guint header_size = 0;
  if ((data[0] & FU_HEADER_MASK_VAL) != SPS_TYPE_ID
      && (data[0] & FU_HEADER_MASK_VAL) != PPS_TYPE_ID)
    {
      header[0] = (nal_header & FU_INDICATOR_MASK_VAL) | FU_INDICATOR_CONST_VAL;
      header[1] = (start << START_LEFT_SHIFT_VAL) | (end << END_LEFT_SHIFT_VAL)
          | (nal_header & FU_HEADER_MASK_VAL);
      header_size = 2;
      data++;
      size--;
      is_frame = TRUE;
    }

  /* Now packetize and transmit the encoded frame data */
  while (end != 1)
    {
      transmit_size =
          size < (payload_len - header_size) ?
              size : (payload_len - header_size);
      if (transmit_size == size)
        {
          /*
           * Transmit_size equals to actual data size
           * so this must be the last frame transmitted
           * so emit last frame signal after transmission
           */
          if (is_frame)
            {
              sink->sender.avatp_header.m0 = 1;
            }
          else
            {
              sink->sender.avatp_header.m0 = 0;
            }
          end = 1;
        }
      else
        {
          sink->sender.avatp_header.m0 = 0;
        }

      if (start && is_frame)
        {
          GST_DEBUG("First frame data =%p \n", data);
          /*
           * Transmitting the first encoded frame data
           * Emit the first frame signal so that application
           * will validate the data been transmitted
           */
          g_signal_emit(sink,
              gst_avatp_sink_signals[SIGNAL_AVATP_FIRST_FRAME_DATA], 0,
              transmit_size, data + position);
        }

      /*
       * Transmit encoded frame data
       * using avatp sender function
       */
      sink->sender.avatp_header.avtp_timestamp = htobe32(timestamp);
      if(header_size)
      {
          header[1] = (start << START_LEFT_SHIFT_VAL) | (end << END_LEFT_SHIFT_VAL)
          | (nal_header & FU_HEADER_MASK_VAL);
      }

     int rv = AVATPSender_send(&sink->sender, &header, header_size,
          (data + position), transmit_size, sink->media_type);

      if (rv != 0)
        {
          if (result == AVATPSENDER_RTP_FRAME_TOO_LARGE)
            GST_ERROR("RTP frame is too large to fit into one AVATP packet!\n");
          else
            GST_ERROR("failed to send AVATP packet!\n");

          return GST_FLOW_ERROR;
        }

      /*
       * Need to set marker bit
       * only for the first packet of the
       * encoded frame data
       */
      sink->sender.avatp_header.m0 = 0;
      size -= transmit_size;
      position += transmit_size;
      start = 0;
    }

  return result;
}


static guint
gst_avatpsink_get_nal_len(const guint8 *data, const guint size)
{
  guint offset = 3;
  // check length to next nal unit (startcode 0x00 0x00 0x00 0x01)
  // otherwise if end of buffer is reached assume that "data" contains
  // complete nal unit

  while (offset < size)
    {
      if (data[offset] == 1)
        {
          if (data[offset - 1] == 0 &&
              data[offset - 2] == 0 &&
              data[offset - 3] == 0)
            {
              return offset - 3;
            }
          else
            {
              offset += 4;
            }
        }
      else
        {
          if (data[offset] != 0)
            {
              offset += 4;
            }
          else
            {
              offset++;
            }
        }
    }

  return size;
}


static GstFlowReturn
gst_avatp_h264_parse_packetize(GstAVATPSink *sink, GstBuffer *buffer)
{
  gint size;
  GstClockTime timestamp;
  guint8 *data;
  GstFlowReturn result = GST_FLOW_OK;
  guint len = 0;
#if GST_CHECK_VERSION(1,0,0)
  GstMapInfo map_info;

  gst_buffer_map(buffer, &map_info, GST_MAP_READ);
  size = map_info.size;
  data = map_info.data;
#else
  size = GST_BUFFER_SIZE (buffer);
  data = GST_BUFFER_DATA (buffer);
#endif
  timestamp = GST_BUFFER_TIMESTAMP (buffer);

  // get offset to first startcode (usually 0) and skip it
  len = gst_avatpsink_get_nal_len(data, size);
  data += START_CODE_LEN + len;
  size -= START_CODE_LEN + len;

  while (size > 0)
    {
      if ((data[0] & FU_HEADER_MASK_VAL) != SPS_TYPE_ID
          && (data[0] & FU_HEADER_MASK_VAL) != PPS_TYPE_ID)
        {
          // we receive only one frame per buffer -> if we detect a
          // frame, we can assume that the rest of the buffer is only one
          // nal unit
          len = size;
        }
      else
        {
          len = gst_avatpsink_get_nal_len(data, size);
        }

      if (len)
        {
          result = gst_avatpsink_transmit_data(sink, data, len, timestamp);
        }
      if (result != GST_FLOW_OK)
        {
          break;
        }
      data += len + START_CODE_LEN;
      size -= len + START_CODE_LEN;
    }

#if GST_CHECK_VERSION(1,0,0)
  gst_buffer_unmap(buffer, &map_info);
#endif

  return result;
}


/* GstElement vmethod implementations */

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_avatpsink_render(GstBaseSink *bsink, GstBuffer *buf)
{
    GstAVATPSink* sink;
    gint result = 0;
    GstFlowReturn ret = GST_FLOW_OK;

    sink = GST_AVATPSINK(bsink);

    /* socket has not been created yet, so create it */
    if (G_UNLIKELY(sink->socket_open != TRUE))
    {
        /* throw warnings in case mandatory properties were not set */
        if (sink->address == NULL)
        {
            GST_ERROR("Stream destination address not set!\n");
            return GST_FLOW_ERROR;
        }

        result = gst_avatpsink_open_sender(sink);
        if (result != 0)
        {
            if (result == AVATPSENDER_SOCKET_ERROR)
                GST_ERROR("failed to open socket!\n");
            else
                GST_ERROR("failed to initialize AVATP sender!\n");

            return GST_FLOW_ERROR;
        }

        sink->socket_open = TRUE;
    }

    if (sink->media_type == AVATP_SENDER_H264)
    {
		result = gst_avatp_h264_parse_packetize(sink, buf);
		if (result != 0)
			ret = GST_FLOW_ERROR;
    }
    else if (sink->media_type == AVATP_SENDER_RTP)
    {
        guint8 *data;
        guint size = 0;
#if GST_CHECK_VERSION(1,0,0)
        GstMapInfo map_info;

        gst_buffer_map(buf, &map_info, GST_MAP_READ);

        size = map_info.size;
        data = map_info.data;
#else
        size = GST_BUFFER_SIZE(buf);
        data = GST_BUFFER_DATA(buf);
#endif

        /* send packet */
        result = AVATPSender_send(&sink->sender, NULL, 0, data, size, sink->media_type);
        if (result != 0)
        {
            if (result == AVATPSENDER_RTP_FRAME_TOO_LARGE)
              GST_ERROR("RTP frame is too large to fit into one AVATP packet!\n");
            else
              GST_ERROR("failed to send AVATP packet!\n");

            ret = GST_FLOW_ERROR;
        }
#if GST_CHECK_VERSION(1,0,0)
        gst_buffer_unmap(buf, &map_info);
#endif
    }
    else
    {
        GST_WARNING("unsupported streamtype");
    	/* Nothing to be done */
    }


   /*
   * Need to emit last frame signal to application
   * so that application will memset entire frame data
   * buffer to zero '0' to avoid cyclic repeating frames
   * in error case of the vpu
   */
  g_signal_emit(sink, gst_avatp_sink_signals[SIGNAL_AVATP_LAST_FRAME_DATA], 0,
      buf);


    return ret;
}

static void gst_avatpsink_set_property(GObject* object, guint prop_id,
        const GValue* value, GParamSpec* pspec)
{
    GstAVATPSink* sink = GST_AVATPSINK(object);

    if (sink->properties_locked == TRUE)
    {
        GST_WARNING(
                "element properties cannot be changed after stream initialization!\n");
        return;
    }

    switch (prop_id)
    {
    case PROP_ADDRESS:
        g_free(sink->address);
        sink->address = g_value_dup_string(value);
        if (TRUE != get_address_from_string(sink->address, sink->dst_mac))
        {
            GST_ERROR(
                    "address format is invalid! Please use xx:xx:xx:xx:xx:xx.\n");
            g_free(sink->address);
            sink->address = NULL;
        }
        break;
    case PROP_STREAM_ID:
        sink->stream_id = g_value_get_uint64(value);
        break;
    case PROP_INTERFACE:
        g_free(sink->interface);
        sink->interface = g_value_dup_string(value);
        break;

    case PROP_STRIP_RTP_HEADER:
        sink->strip_rtp_header = g_value_get_boolean(value);
        break;

    case PROP_USE_VLAN_TAG:
        sink->use_vlan_tag = g_value_get_boolean(value);
        break;
    case PROP_VLAN_ID:
        sink->vlan_id = g_value_get_uint(value);
        break;
    case PROP_VLAN_PRIORITY:
        sink->vlan_priority = g_value_get_uint(value);
        break;
    case PROP_VLAN_DROP:
        sink->vlan_drop = g_value_get_boolean(value);
        break;
    case PROP_SHAPE_INTERVAL:
        sink->shape_interval = g_value_get_uint(value);
        break;
    case PROP_MAX_MTU_SIZE:
        sink->max_mtu_size = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_avatpsink_get_property(GObject* object, guint prop_id,
        GValue* value, GParamSpec* pspec)
{
    GstAVATPSink* sink = GST_AVATPSINK(object);

    switch (prop_id)
    {
    case PROP_ADDRESS:
        g_value_set_string(value, sink->address);
        break;
    case PROP_STREAM_ID:
        g_value_set_uint64(value, sink->stream_id);
        break;
    case PROP_INTERFACE:
        g_value_set_string(value, sink->interface);
        break;

    case PROP_STRIP_RTP_HEADER:
        g_value_set_boolean(value, sink->strip_rtp_header);
        break;

    case PROP_USE_VLAN_TAG:
        g_value_set_boolean(value, sink->use_vlan_tag);
        break;
    case PROP_VLAN_ID:
        g_value_set_uint(value, sink->vlan_id);
        break;
    case PROP_VLAN_PRIORITY:
        g_value_set_uint(value, sink->vlan_priority);
        break;
    case PROP_VLAN_DROP:
        g_value_set_boolean(value, sink->vlan_drop);
        break;
    case PROP_SHAPE_INTERVAL:
        g_value_set_uint(value, sink->shape_interval);
        break;
    case PROP_MAX_MTU_SIZE:
        g_value_set_uint(value, sink->max_mtu_size);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_avatpsink_log_sending(GstAVATPSink* sink)
{
    if (sink == NULL)
        return;

    char macstr[ETH_ALEN * 3 + 1];
    int i;
    for (i = 0; i < ETH_ALEN; i++)
        sprintf(macstr + i * 3, "%.2x:", sink->dst_mac[i]);
    macstr[ETH_ALEN * 3 - 1] = 0;

    GST_INFO("Sending EtherType=0x%04x packets with stream id=%016"
    PRIx64 " on %s to %s\n", ETH_P_AVA_AVTP, sink->stream_id, sink->interface,
            macstr);
}

static int gst_avatpsink_open_sender(GstAVATPSink* sink)
{
    sink->properties_locked = TRUE;
    int result = 0;

    if (sink->media_type == AVATP_SENDER_RTP)
      {
        result = AVATPSender_set_rtp_header_stripping(&sink->sender,
            sink->strip_rtp_header);
        if (result != 0)
          return result;
    }
    else if (!sink->strip_rtp_header && sink->media_type == AVATP_SENDER_H264)
      {
        GST_WARNING("RTP header transmission is not supported with H.264 streams");
      }

    result = AVATPSender_set_vlan_tag(&sink->sender, sink->use_vlan_tag,
            sink->vlan_id, sink->vlan_priority, sink->vlan_drop);
    if (result != 0)
        return result;

    result = AVATPSender_set_shape_interval(&sink->sender,
            sink->shape_interval);
    if (result != 0)
        return result;

    result = AVATPSender_open(&sink->sender, sink->interface,
            (void*)sink->dst_mac, sink->stream_id);
    if (result != 0)
        return result;

    /* log address information */
    gst_avatpsink_log_sending(sink);

    return 0;
}

/*lint +e826*/
/*lint +e774*/
