/**
 * \file: gstavatpsrc.c
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

#include <stdio.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <gst/gst.h>
#include "avatp_avtp.h"
#include "gstavatpsrc.h"

/* value ranges and defaults of the avatpsrc element */
#define AVATP_INTERFACE_DEFAULT			(const gchar*)"eth0"
#define AVATP_STREAM_ID_MIN				0
#define AVATP_STREAM_ID_MAX				G_MAXUINT64
#define AVATP_STREAM_ID_DEFAULT			0
#define AVATP_RTP_STRIP_HEADER_DEFAULT	TRUE
#define AVATP_RTP_VERSION_MIN			0
#define AVATP_RTP_VERSION_MAX 			2
#define AVATP_RTP_VERSION_DEFAULT		2
#define AVATP_RTP_EXTENSION_DEFAULT		FALSE
#define AVATP_RTP_SSRC_MIN				0
#define AVATP_RTP_SSRC_MAX				G_MAXUINT32
#define AVATP_RTP_SSRC_DEFAULT			0
GST_DEBUG_CATEGORY_EXTERN(gst_avatp_debug);
#define GST_CAT_DEFAULT gst_avatp_debug

/* PRQA: Lint Message 774: deactivation because any GST_DEBUG thrown this info */
/*lint -e774*/


/* PRQA: Lint Message 826: deactivation because casting mechanism of GObject
 * throws the finding */
/*lint -e826*/

enum
{
    PROP_INTERFACE = 1,
    PROP_STREAM_ID,
    PROP_GEN_RTP_HEADER,
    PROP_RTP_VERSION,
    PROP_RTP_EXTENSION,
    PROP_RTP_SSRC
};

/* PRQA: Lint message 19, 123, 144, 751, 160, 528: deactivation because macro is related to Gstreamer framework */
/*lint -e19 -e123 -e144 -e751 -e160 -e528*/
/*lint -save -e528*/
#if GST_CHECK_VERSION(1,0,0)
G_DEFINE_TYPE(GstAVATPSrc, gst_avatpsrc, GST_TYPE_PUSH_SRC)
#else
GST_BOILERPLATE(GstAVATPSrc, gst_avatpsrc, GstPushSrc, GST_TYPE_PUSH_SRC)
#endif
;
/*lint +e19 +e123 +e144 +e751 +e160 +e528*/
/*lint -restore*/
static GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE("src",
        GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("application/x-rtp"));

/* GstAVATPSrc virtual methods */
static void gst_avatpsrc_finalize(GstAVATPSrc* src);
static void gst_avatpsrc_set_property(GObject* object, guint prop_id,
        const GValue* value, GParamSpec* pspec);
static void gst_avatpsrc_get_property(GObject* object, guint prop_id,
        GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_avatpsrc_create(GstPushSrc* base_src,
        GstBuffer** buffer);
static gboolean gst_avatpsrc_start(GstBaseSrc* base_src);
static gboolean gst_avatpsrc_stop(GstBaseSrc* base_src);
static gboolean gst_avatpsrc_unlock(GstBaseSrc*base_src);
static gboolean gst_avatpsrc_unlock_stop(GstBaseSrc* base_src);

/* GstAVATPSrc private methods */
static void gst_avatpsrc_log_listen(GstAVATPSrc* src);

#if !GST_CHECK_VERSION(1,0,0)
static void gst_avatpsrc_base_init(gpointer gclass)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS(gclass);

    gst_element_class_set_details_simple(element_class,
            "AVATP packet receiver", "Source/Network",
            "Receive data from the network via AVATP",
            "Jakob Harder <jharder@de.adit-jv.com>");

    gst_element_class_add_pad_template(element_class,
            gst_static_pad_template_get(&src_factory));

}
#endif

static void gst_avatpsrc_class_init(GstAVATPSrcClass* klass)
{
    GObjectClass* gobject_class;
    GstBaseSrcClass* gstbasesrc_class;
    GstPushSrcClass* gstpushsrc_class;

#if GST_CHECK_VERSION(1,0,0)
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
#endif
    gobject_class = (GObjectClass*)klass;
    gstbasesrc_class = (GstBaseSrcClass*)klass;
    gstpushsrc_class = (GstPushSrcClass*)klass;

#if GST_CHECK_VERSION(1,0,0)
    gst_element_class_set_details_simple(element_class,
            "AVATP packet receiver", "Source/Network",
            "Receive data from the network via AVATP",
            "Jakob Harder <jharder@de.adit-jv.com>");

    gst_element_class_add_pad_template(element_class,
            gst_static_pad_template_get(&src_factory));
#endif
    gobject_class->set_property = gst_avatpsrc_set_property;
    gobject_class->get_property = gst_avatpsrc_get_property;
    gobject_class->finalize = (GObjectFinalizeFunc)gst_avatpsrc_finalize;

    /* install element properties */
    g_object_class_install_property(gobject_class, PROP_STREAM_ID,
            g_param_spec_uint64("id", "StreamID", "The AVTP stream ID.",
                    AVATP_STREAM_ID_MIN, AVATP_STREAM_ID_MAX,
                    AVATP_STREAM_ID_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_INTERFACE,
            g_param_spec_string("if", "interface",
                    "The interface to receive the AVATP packets from",
                    AVATP_INTERFACE_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_GEN_RTP_HEADER,
            g_param_spec_boolean("strip", "Strip RTP header mode",
                    "Use strip RTP header mode and generate RTP headers ?",
                    AVATP_RTP_STRIP_HEADER_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_RTP_VERSION,
            g_param_spec_uint("rtp-version", "RTP version", "The RTP version.",
                    AVATP_RTP_VERSION_MIN, AVATP_RTP_VERSION_MAX,
                    AVATP_RTP_VERSION_DEFAULT, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_RTP_EXTENSION,
            g_param_spec_boolean("rtp-extension", "RTP extension header",
                    "RTP extension header used ?", AVATP_RTP_EXTENSION_DEFAULT,
                    G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_RTP_SSRC,
            g_param_spec_uint("rtp-ssrc", "RTP SSRC",
                    "The stream source id of packets.", AVATP_RTP_SSRC_MIN,
                    AVATP_RTP_SSRC_MAX, AVATP_RTP_SSRC_DEFAULT,
                    G_PARAM_READWRITE));

    gstbasesrc_class->start = gst_avatpsrc_start;
    gstbasesrc_class->stop = gst_avatpsrc_stop;
    gstbasesrc_class->unlock = gst_avatpsrc_unlock;
    gstbasesrc_class->unlock_stop = gst_avatpsrc_unlock_stop;

    gstpushsrc_class->create = gst_avatpsrc_create;
}

#if GST_CHECK_VERSION(1,0,0)
static void gst_avatpsrc_init(GstAVATPSrc* src)
{
#else
static void gst_avatpsrc_init(GstAVATPSrc* src, GstAVATPSrcClass* gclass)
{
    (void)gclass;

#endif
    src->properties_locked = FALSE;

    /* init default properties */
    src->stream_id = AVATP_STREAM_ID_DEFAULT;
    src->interface = g_strdup(AVATP_INTERFACE_DEFAULT);
    src->gen_rtp_header = AVATP_RTP_STRIP_HEADER_DEFAULT;
    src->rtp_version = AVATP_RTP_VERSION_DEFAULT;
    src->rtp_extension = AVATP_RTP_EXTENSION_DEFAULT;
    src->rtp_ssrc = AVATP_RTP_SSRC_DEFAULT;

    /* set base default properties */
    gst_base_src_set_live(GST_BASE_SRC(src), TRUE);
    gst_base_src_set_format(GST_BASE_SRC(src), GST_FORMAT_TIME);
    gst_base_src_set_do_timestamp(GST_BASE_SRC(src), TRUE);

    src->timeout = GST_CLOCK_TIME_NONE;
    src->sock.fd = -1;

    /* initialize receiver to default state */
    AVATPReceiver_init(&src->receiver);
}

static void gst_avatpsrc_finalize(GstAVATPSrc* src)
{
    /* calling this twice is illegal, so skip safety tests */
    g_free(src->interface);
#if GST_CHECK_VERSION(1,0,0)
    G_OBJECT_CLASS(gst_avatpsrc_parent_class)->finalize((GObject *)src);
#else
    G_OBJECT_CLASS(parent_class)->finalize((GObject*)src);
#endif
}

static gboolean gst_avatpsrc_start(GstBaseSrc* base_src)
{
    GstAVATPSrc* src = GST_AVATPSRC(base_src);

    /* prevent property change after AVATP initialization */
    src->properties_locked = TRUE;

    /* open AVATP receiver */
    int socket_handle = AVATPReceiver_open(&src->receiver, src->interface,
            src->stream_id);
    if (socket_handle < 0)
    {
        if (socket_handle == AVATPRECEIVER_SOCKET_ERROR)
            GST_ERROR("failed to open socket!\n");
        else
            GST_ERROR("failed to initialize AVATP receiver!\n");

        return FALSE;
    }
    src->sock.fd = socket_handle;

    /* log address information */
    gst_avatpsrc_log_listen(src);

    /* configure RTP header fields */
    int result = AVATPReceiver_set_rtp_header_stripping(&src->receiver,
            src->gen_rtp_header);
    result |= AVATPReceiver_set_rtp_header_values(&src->receiver,
            src->rtp_version, src->rtp_extension, src->rtp_ssrc);
    if (result != 0)
    {
        GST_ERROR("invalid value!\n");
        return FALSE;
    }

    /* check maximum Ethernet frame size */
    /* TODO: support for jumbo frames? */
    src->max_frame_size = ETH_FRAME_LEN + ETH_FCS_LEN;

    /* install receive functions */
    src->fdset = gst_poll_new(TRUE);
    if (src->fdset == NULL)
    {
        GST_ERROR("failed to register socket for polling!\n");
        return FALSE;
    }
    if (TRUE != gst_poll_add_fd(src->fdset, &src->sock))
    {
        GST_ERROR("Failed to register socket for polling!\n");
        return FALSE;
    }
    if (TRUE != gst_poll_fd_ctl_read(src->fdset, &src->sock, TRUE))
    {
        GST_ERROR("failed to register socket for polling!\n");
        return FALSE;
    }

    return TRUE;
}

static gboolean gst_avatpsrc_stop(GstBaseSrc* base_src)
{
    GstAVATPSrc* src = GST_AVATPSRC(base_src);

    /* close AVATP receiver */
    AVATPReceiver_close(&src->receiver);
    src->sock.fd = -1;

    /* uninstall receive functions */
    if (src->fdset != NULL)
    {
        gst_poll_free(src->fdset);
        src->fdset = NULL;
    }

    return TRUE;
}

static gboolean gst_avatpsrc_unlock(GstBaseSrc* base_src)
{
    GstAVATPSrc* src = GST_AVATPSRC(base_src);

    gst_poll_set_flushing(src->fdset, TRUE);

    return TRUE;
}

static gboolean gst_avatpsrc_unlock_stop(GstBaseSrc* base_src)
{
    GstAVATPSrc* src = GST_AVATPSRC(base_src);

    gst_poll_set_flushing(src->fdset, FALSE);

    return TRUE;
}

static GstFlowReturn gst_avatpsrc_create(GstPushSrc* base_src,
        GstBuffer** buffer)
{
    GstAVATPSrc* src;
    struct avatp_received_frame rtp_frame;
    int length = 0;

    src = GST_AVATPSRC(base_src);

    /* create memory buffer for RTP frame */
    rtp_frame.malloc_pointer = (void*)g_malloc(src->max_frame_size);
    rtp_frame.size = src->max_frame_size;

    int result = 0;
    do
    {
        if (result != 0 && result != AVATPRECEIVER_PACKET_DISCARD)
            GST_WARNING("invalid packet format or failed to read packet!\n");

        /* check if data is available */
        if (0 > AVATPReceiver_get_receive_length(&src->receiver, &length))
        {
            GST_WARNING("failed to read the size of available data!\n");
            return GST_FLOW_ERROR;
        }

        while (length <= 0)
        {
            /* not ready to receive yet, wait for data */
            result = 0;
            do
            {
                result = gst_poll_wait(src->fdset, src->timeout);
                if (result < 0)
                {
                    if (errno == EBUSY)
                    {
                        /* gst_avatpsrc_unlock has been called,
                         * normal expected operation */
                        GST_INFO("gst_poll_wait got unlocked\n");
#if GST_CHECK_VERSION(1,0,0)
                        return GST_FLOW_FLUSHING;
#else
                        return GST_FLOW_WRONG_STATE;
#endif
                    }
                    if (errno != EAGAIN && errno != EINTR)
                    {
                        GST_ERROR("error %d occured!\n", errno);
                        return GST_FLOW_ERROR;
                    }
                }
                else if (result == 0)
                {
                    /* should not happen as timeout is set to infinite */
                    GST_WARNING("gst_poll_wait time out!\n");
                }
            } while (result <= 0);

            /* check if data is available */
            if (0 > AVATPReceiver_get_receive_length(&src->receiver, &length))
            {
                GST_ERROR("failed to read the size of available data!\n");
                return GST_FLOW_ERROR;
            }
        }
    }
    /* if received data is not for us or invalid, try again */
    while (0 != (result = AVATPReceiver_receive(&src->receiver, &rtp_frame)));

    /* assign rtp frame to memory buffers */
#if GST_CHECK_VERSION(1,0,0)
    *buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY,
        rtp_frame.data, rtp_frame.size, 0, rtp_frame.size,
        rtp_frame.malloc_pointer, g_free);
#else
    *buffer = gst_buffer_new();

    GST_BUFFER_MALLOCDATA(*buffer) = rtp_frame.malloc_pointer;
    GST_BUFFER_DATA(*buffer) = rtp_frame.data;
    GST_BUFFER_SIZE(*buffer) = rtp_frame.size;
#endif

    return GST_FLOW_OK;
}

static void gst_avatpsrc_set_property(GObject* object, guint prop_id,
        const GValue* value, GParamSpec* pspec)
{
    GstAVATPSrc* src = GST_AVATPSRC(object);

    if (src->properties_locked == TRUE)
    {
        GST_WARNING("element properties cannot be changed after stream " \
                "initialization!\n");
        return;
    }

    /* value ranges are already checked by the framework,
     * no further checks required */
    switch (prop_id)
    {
    case PROP_STREAM_ID:
        src->stream_id = g_value_get_uint64(value);
        break;
    case PROP_INTERFACE:
        g_free(src->interface);
        src->interface = g_value_dup_string(value);
        break;
    case PROP_GEN_RTP_HEADER:
        src->gen_rtp_header = g_value_get_boolean(value);
        break;
    case PROP_RTP_VERSION:
        src->rtp_version = g_value_get_uint(value);
        break;
    case PROP_RTP_EXTENSION:
        src->rtp_extension = g_value_get_boolean(value);
        break;
    case PROP_RTP_SSRC:
        src->rtp_ssrc = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_avatpsrc_get_property(GObject* object, guint prop_id,
        GValue* value, GParamSpec* pspec)
{
    GstAVATPSrc* src = GST_AVATPSRC(object);

    switch (prop_id)
    {
    case PROP_STREAM_ID:
        g_value_set_uint64(value, src->stream_id);
        break;
    case PROP_INTERFACE:
        g_value_set_string(value, src->interface);
        break;
    case PROP_GEN_RTP_HEADER:
        g_value_set_boolean(value, src->gen_rtp_header);
        break;
    case PROP_RTP_VERSION:
        g_value_set_uint(value, src->rtp_version);
        break;
    case PROP_RTP_EXTENSION:
        g_value_set_boolean(value, src->rtp_extension);
        break;
    case PROP_RTP_SSRC:
        g_value_set_uint(value, src->rtp_ssrc);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_avatpsrc_log_listen(GstAVATPSrc* src)
{
    const unsigned char* src_mac = AVATPReceiver_get_source_mac(&src->receiver);
    if (src_mac != NULL)
    {
        char macstr[ETH_ALEN * 3 + 1];
        int i;
        for (i = 0; i < ETH_ALEN; i++)
            sprintf(macstr + i * 3, "%.2x:", src_mac[i]);
        macstr[ETH_ALEN * 3 - 1] = 0;

        GST_INFO("Listen to EtherType=0x%04x packets with stream id=%016"
        PRIx64 " on %s (%s)\n", ETH_P_AVA_AVTP, src->stream_id, src->interface,
                macstr);
    }
    else
    {
        /* Should never happen, but is not affecting normal program flow */
        GST_ERROR("internal error\n!");
    }
}

/*lint +e826*/
/*lint +e774*/
