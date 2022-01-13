/**
* \file: gstxsubdecode.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* DivX XSUB subtitle decoder
* *
* \component: gst_xsubdec
*
* \author: Jens Georg ADITG/SWG jgeorg@de.adit-jv.com
*
* \copyright: (c) 2013 ADIT Corporation
*
* \history
* 0.1 Jens Georg Initial version
*
***********************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <gst/gst.h>
#include <gst/video/video.h>

#include "gstxsubdecode.h"
#include "xsub.h"

GST_DEBUG_CATEGORY_STATIC (gst_xsubdecode_debug);
#define GST_CAT_DEFAULT gst_xsubdecode_debug

/* PRQA: Lint Message 826, 160: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826 -e160*/

#define XSUB_DURATION_HEADER_LENGTH 27

/* Number of subsequent bad buffers allowed */
#define GST_XSUBDEC_MAX_BAD 3

typedef struct _SubtitleHeader
{
  guint8 duration[28];
  guint16 width;
  guint16 height;
  guint16 left;
  guint16 top;
  guint16 right;
  guint16 bottom;
  guint16 field_offset;
  guint32 colortab[4];
} SubtitleHeader;

enum
{
  PROP_SILENT = 1,
  PROP_DEBUG_DUMP_PACKET,
  PROP_DEBUG_SHOW_BACKGROUND
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("subtitle/x-xsub, has-alpha={TRUE, FALSE}, width=[1,max], height=[1,max]")
);

static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE ("src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS (GST_VIDEO_CAPS_RGBA)
    );

/* PRQA: Lint Message 19, 123, 144, 751: deactivation because macro is related to GStreamer framework */
/*lint -e19 -e123 -e144 -e751 */
GST_BOILERPLATE (GstXSUBDecode, gst_xsubdecode, GstElement,
    GST_TYPE_ELEMENT);
/*lint +e19 +e123 +e144 +e751 */

static void gst_xsubdecode_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_xsubdecode_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void gst_xsubdecode_dispose (GObject *object);

static GstFlowReturn gst_xsubdecode_chain (GstPad * pad, GstBuffer * buf);

static gboolean gst_xsubdecode_sinkpad_setcaps (GstPad *pad, GstCaps *caps);

static gboolean gst_xsubdecode_sinkpad_events (GstPad *pad, GstEvent *event);

static gboolean gst_xsubdecode_srcpad_events (GstPad *pad, GstEvent *event);

/* GObject vmethod implementations */

static void
gst_xsubdecode_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "XSUB decoder",
    "Codec/Decoder/video",
    "DivX XSUB and XSUB + decoder to raw image",
    "Jens Georg <jgeorg@adit-jv.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the xsubdecode's class */
static void
gst_xsubdecode_class_init (GstXSUBDecodeClass * klass)
{
  GObjectClass *gobject_class;

  /* debug category for filtering log messages
   */
  GST_DEBUG_CATEGORY_INIT (gst_xsubdecode_debug, "xsubdecode",
      0, "XSUB DivX subtitle decoder");

  gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_xsubdecode_set_property;
  gobject_class->get_property = gst_xsubdecode_get_property;
  gobject_class->dispose = gst_xsubdecode_dispose;

  g_object_class_install_property (gobject_class,
      PROP_SILENT,
      g_param_spec_boolean ("silent",
                            "Silent",
                            "Produce verbose output ?",
                            TRUE,
                            G_PARAM_CONSTRUCT |
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_DEBUG_DUMP_PACKET,
      g_param_spec_boolean ("debug-dump-decoded-packet",
                            "Dump decoded packet",
                            "Write the raw image data to a file",
                            FALSE,
                            G_PARAM_CONSTRUCT |
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_DEBUG_SHOW_BACKGROUND,
      g_param_spec_boolean ("debug-show-background",
                            "Paint the background",
                            "Paint the transparent XSUB background",
                            FALSE,
                            G_PARAM_CONSTRUCT |
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_xsubdecode_init (GstXSUBDecode * filter,
                     GstXSUBDecodeClass * gclass)
{
  __ADIT_UNUSED(gclass);

  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_use_fixed_caps (filter->sinkpad);
  gst_pad_set_chain_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR(gst_xsubdecode_chain));
  gst_pad_set_setcaps_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_xsubdecode_sinkpad_setcaps));
  gst_pad_set_event_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_xsubdecode_sinkpad_events));

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_event_function (filter->srcpad,
      GST_DEBUG_FUNCPTR (gst_xsubdecode_srcpad_events));

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);
  filter->silent = FALSE;

  /* Hardcode for DXSB */
  filter->plane_width = 640;
  filter->plane_height = 480;
  filter->stride = filter->plane_width * 4;
  filter->plane_size = filter->plane_height * filter->stride;
  filter->clock = gst_system_clock_obtain ();
}

static void
gst_xsubdecode_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstXSUBDecode *filter = GST_XSUBDECODE (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_DEBUG_DUMP_PACKET:
      filter->debug_dump_decoded_package = g_value_get_boolean (value);
      break;
    case PROP_DEBUG_SHOW_BACKGROUND:
      filter->debug_show_background = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_xsubdecode_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstXSUBDecode *filter = GST_XSUBDECODE (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_DEBUG_DUMP_PACKET:
      g_value_set_boolean (value, filter->debug_dump_decoded_package);
      break;
    case PROP_DEBUG_SHOW_BACKGROUND:
      g_value_set_boolean (value, filter->debug_show_background);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_xsubdecode_dispose (GObject *object)
{
  GstXSUBDecode *filter = GST_XSUBDECODE(object);

  if (filter->clock)
  {
    gst_object_unref (filter->clock);
    filter->clock = NULL;
  }
}

/* GstElement vmethod implementations */

/* Dump the XSUB header */
static void
dump_header (const SubtitleHeader *header)
{
  GST_DEBUG ("Duration: %s", header->duration);
  GST_DEBUG ("Resolution:");
  GST_DEBUG ("  width: %d", header->width);
  GST_DEBUG ("  height: %d", header->height);
  GST_DEBUG ("  left: %d", header->left);
  GST_DEBUG ("  top: %d", header->top);
  GST_DEBUG ("  bottom: %d", header->top);
  GST_DEBUG ("  field-offset: %d", header->field_offset);

  GST_DEBUG ("Colors:");
  GST_DEBUG ("  background: (0x%08x)", header->colortab[0]);
  GST_DEBUG ("  palette1  : (0x%08x)", header->colortab[1]);
  GST_DEBUG ("  palette2  : (0x%08x)", header->colortab[2]);
  GST_DEBUG ("  palette3  : (0x%08x)", header->colortab[3]);
}

/* Read a nibble (4 bits) from a buffer */
static guint8
read_nibble (const guint8 const *buffer, goffset offset)
{
    guint8 nibble = buffer[offset / 2];

    /* mask lower nibble for odd offsets */
    if (offset & 0x01) {
        nibble &= 0x0f;
    } else {
        /* get upper nibble */
        nibble >>= 4;
    }

    return nibble;
}

/* Read a RLE value. Keeps on reading nibbles until either the value is
 * valid or 16 bits are read. The encoding format is described in section 4.7 of
 * fs_divx_cesdk_bmp_subtitles.pdf, part of the DivX format specification.
 *
 * For a single nibble to be a valid RLE its value has to be at least 0x04
 * (1px, color 0)
 *
 * For a byte the value has to be at least 0x10 (4 px with color 0; 4 is the
 * smallest possible number in a byte RLE, otherwise it would have been 4 bits.
 *
 * For 12 bit RLE value the magic is 0x40 (16 px with color 0). The largest
 * possible RLE is 16 bits.
 *
 * Modifies nibble_offset accordingly to be the next nibble to read.
 */
static guint16
rle_value (const guint8 const *buffer, goffset *nibble_offset)
{
  goffset offset = *nibble_offset;
  guint16 value = 0;
  guint8 nibble;
  guint16 nibble_min = 0x01;

  while (value < nibble_min && offset < *nibble_offset + 4)
  {
    value <<= 4;
    nibble = read_nibble (buffer, offset);
    offset++;
    value |= nibble;
    nibble_min <<= 2;
  }

  *nibble_offset = offset;

  return value;
}

static gboolean
decode_xsub_data (const GstXSUBDecode *filter,
                  const SubtitleHeader *header,
                  const guint8 const *rle_data,
                  guint8 *output)
{
  goffset nibble_offset = 0;
  guint x = 0;
  guint y = 0;
  guint top_offset = 0;
  guint nfill = 0;
  guint left_offset = 0;
  goffset output_offset = 0;

  /* Actual subtitle decoding */
  top_offset = header->top * filter->stride;
  left_offset = header->left * 4;
  nfill = ((filter->plane_width - header->width) - header->left) * 4;
  output_offset = top_offset;

  for (y = 0 ; y < header->height; y++)
  {
    x = 0;

    /* switch to bottom fields of interlaced image */
    if (y == header->height / 2)
    {
      if (nibble_offset / 2 != header->field_offset)
      {
        GST_WARNING ("Field offset in header is incorrect");
      }
      output_offset = filter->stride + top_offset;
    }

    /* shift line to the right */
    output_offset += left_offset;

    while (x < header->width)
    {
      guint16 value;
      int color;
      guint rle;
      guint i;

      value = rle_value (rle_data, &nibble_offset);

      /* Color is coded in the last two bits */
      color = value & 0x0003;

      /* Pixel run length is coded in the other bits */
      rle = value >> 2;

      /* Make sure we don't overshoot as a precaution - happened during
       * development while accidently not skipping the padding. */
      rle = MIN (rle, header->width - x);

      /* Run length of zero means to fill remainder of line */
      if (rle == 0)
      {
        rle = header->width - x;
      }

      x += rle;

      if (G_UNLIKELY(output_offset + (rle * 4) > filter->plane_size))
      {
        GST_WARNING_OBJECT(filter, "RLE data larger than plane size, discarding frame. Demuxer bug?");

        return FALSE;
      }

      /* Write RGBA image data */
      for (i = 0; i < rle; i++)
      {
        GST_WRITE_UINT32_BE (&(output[output_offset]),
                             header->colortab[color]);
        output_offset += 4;
      }
    }

    /* if subtitle is cropped, skip rest of line */
    output_offset += nfill;

    /* image data is interlaced, so skip a line */
    output_offset += filter->stride;

    /* skip bitstream padding, odd nibble offset means we only "read" the upper
     * half of the byte */
    if (nibble_offset & 0x01)
    {
      nibble_offset++;
    }
  }

  return TRUE;
}

static GstFlowReturn
alloc_output_buffer (GstXSUBDecode *filter, GstBuffer **output)
{
  GstFlowReturn ret;

#ifndef HOST_COMPILE
  ret = gst_pad_alloc_buffer_and_set_caps (filter->srcpad,
                                           0,
                                           filter->plane_size,
                                           GST_PAD_CAPS (filter->srcpad),
                                           output);
#else
  *output = gst_buffer_new_and_alloc (filter->plane_size);

  if (*output != NULL)
  {
    gst_buffer_set_caps (*output, GST_PAD_CAPS (filter->srcpad));
    ret = GST_FLOW_OK;
  }
  else
  {
    ret = GST_FLOW_ERROR;
  }
#endif

  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_xsubdecode_chain (GstPad * pad, GstBuffer * buf)
{
  GstXSUBDecode *filter = NULL;
  SubtitleHeader header;
  guint8 *data = GST_BUFFER_DATA (buf);
  goffset header_offset = 0;
  GstBuffer *output = NULL;
  GstClockTime start = 0;
  GstClockTime duration = 0;
  gboolean retval;
  GstFlowReturn ret;

  memset ((void *) &header, 0, sizeof (SubtitleHeader));
  memcpy (header.duration, data, 27);

  filter = GST_XSUBDECODE (GST_OBJECT_PARENT (pad));

  if (G_UNLIKELY(filter->bad_buffer_count > GST_XSUBDEC_MAX_BAD))
  {
    return GST_FLOW_OK;
  }

  if (!GST_PAD_CAPS (filter->srcpad))
  {
    GstCaps *srccaps;
    GValue fraction = G_VALUE_INIT;

    srccaps = gst_caps_from_string (GST_VIDEO_CAPS_RGBA);
    gst_caps_set_simple (srccaps,
                         "width", G_TYPE_INT, filter->plane_width,
                         "height", G_TYPE_INT, filter->plane_height,
                         NULL);

    g_value_init(&fraction, GST_TYPE_FRACTION);
    gst_value_set_fraction(&fraction, 0, 1);

    gst_caps_set_value (srccaps, "framerate", &fraction);
    g_value_reset(&fraction);

    retval = gst_pad_set_caps (filter->srcpad, srccaps);
    gst_caps_unref (srccaps);
    if (!retval)
    {
      GST_WARNING ("Failed to set src caps");

      return GST_FLOW_ERROR;
    }
  }

  if (!GST_PAD_CAPS (filter->sinkpad))
  {
    GstCaps *sinkcaps;

    sinkcaps = gst_caps_from_string ("video/x-avi-unknown");
    if (filter->has_alpha)
    {
      gst_caps_set_simple (sinkcaps,
                           "fourcc", GST_TYPE_FOURCC,
                             GST_MAKE_FOURCC('D', 'X', 'S', 'A'),
                           NULL);
    }
    else
    {
      gst_caps_set_simple (sinkcaps,
                           "fourcc", GST_TYPE_FOURCC,
                             GST_MAKE_FOURCC('D', 'X', 'S', 'B'),
                           NULL);
    }

    retval = gst_pad_set_caps (filter->sinkpad, sinkcaps);
    gst_caps_unref (sinkcaps);

    if (!retval)
    {
      GST_WARNING ("Failed to set sinkcaps");

      return GST_FLOW_ERROR;
    }
  }

  if (filter->language != NULL)
  {
    gst_element_found_tags_for_pad (GST_ELEMENT (filter), filter->srcpad,
        filter->language);
    filter->language = NULL; /* found_tags_for_pad takes ownership of taglist */
  }

  if (gst_xsub_parse_duration (header.duration, &start, &duration) == FALSE)
  {
      GST_WARNING_OBJECT (filter, "Buffer had invalid timestamp; discarding\n");

      return GST_FLOW_OK;
  }

  header_offset += XSUB_DURATION_HEADER_LENGTH;

  header.width = GST_READ_UINT16_LE (&(data[header_offset]));
  header_offset += 2;

  header.height = GST_READ_UINT16_LE (&(data[header_offset]));
  header_offset += 2;

  header.left = GST_READ_UINT16_LE (&(data[header_offset]));
  header_offset += 2;

  header.top = GST_READ_UINT16_LE (&(data[header_offset]));
  header_offset += 2;

  header.right = GST_READ_UINT16_LE (&(data[header_offset]));
  header_offset += 2;

  header.bottom = GST_READ_UINT16_LE (&(data[header_offset]));
  header_offset += 2;

  header.field_offset = GST_READ_UINT16_LE (&(data[header_offset]));
  header_offset += 2;

  header.colortab[0] = GST_READ_UINT24_BE (&(data[header_offset])) << 8;
  header_offset += 3;

  header.colortab[1] = GST_READ_UINT24_BE (&(data[header_offset])) << 8;
  header_offset += 3;

  header.colortab[2] = GST_READ_UINT24_BE (&(data[header_offset])) << 8;
  header_offset += 3;

  header.colortab[3] = GST_READ_UINT24_BE (&(data[header_offset])) << 8;
  header_offset += 3;

  if (filter->has_alpha)
  {
	  /* DXSA - transparency values are the last four bytes of the header */
	  header.colortab[0] |= data[header_offset];
	  header_offset++;
	  header.colortab[1] |= data[header_offset];
	  header_offset++;
	  header.colortab[2] |= data[header_offset];
	  header_offset++;
	  header.colortab[3] |= data[header_offset];
	  header_offset++;
  }
  else
  {
	  /* DXSB - all colors are opaque, background is always transparent,
	   * unless requested by plugin option */
	  if (filter->debug_show_background)
	  {
		  header.colortab[0] |= 0xff;
	  }

	  /* make opaque */
	  header.colortab[1] |= 0xff;
	  header.colortab[2] |= 0xff;
	  header.colortab[3] |= 0xff;
  }

  if (filter->silent == FALSE)
  {
    dump_header (&header);
  }

  ret = alloc_output_buffer(filter, &output);
  if (ret != GST_FLOW_OK)
  {
    GST_WARNING ("Failed to allocate buffer of size %d", filter->plane_size);

    return ret;
  }

  memset(GST_BUFFER_DATA(output), 0, GST_BUFFER_SIZE(output));
  output->timestamp = start;
  output->duration = duration - 1;

  if (!decode_xsub_data (filter, &header, &(data[header_offset]),
      GST_BUFFER_DATA (output)))
  {
    gst_buffer_unref (output);
    filter->bad_buffer_count++;

    if (filter->bad_buffer_count > GST_XSUBDEC_MAX_BAD)
    {
      GST_ERROR_OBJECT(filter, "Too many bad buffers. Will not continue decoding");
    }

    /* Ok, that didn't work; but let's continue anyway with the next packet */
    return GST_FLOW_OK;
  }

  /* a successfully decoded buffer, reset bad buffer count */
  filter->bad_buffer_count = 0;

  if (filter->debug_dump_decoded_package)
  {
    char *fname;
    GError *error = NULL;

    fname = g_strdup_printf ("%s.raw", header.duration);
    g_file_set_contents (fname,
                         (const char *) GST_BUFFER_DATA (output),
                         GST_BUFFER_SIZE (output),
                         &error);
    g_free (fname);

    if (error != NULL)
    {
      GST_WARNING ("Failed to dump contents: %s", error->message);
      g_error_free (error);
    }
  }

  gst_buffer_unref (buf);


  GST_TRACE_OBJECT (filter, "Pushing subtitle %s", header.duration);

  ret = gst_pad_push (filter->srcpad, output);
  if (ret != GST_FLOW_OK)
  {
    return ret;
  }

  /* Push an empty buffer directly after the subtitle so we clear it off the
   * screen
   *
   * FIXME: Should avoid flashing somehow when subtitles are back-to-back.
   */
  ret = alloc_output_buffer(filter, &output);
  if (ret != GST_FLOW_OK)
  {
    /* Well, no clearing then; */
    GST_WARNING ("Failed to create clear buffer");

    return GST_FLOW_OK;
  }

  memset(GST_BUFFER_DATA(output), 0, GST_BUFFER_SIZE(output));

  output->timestamp = start + duration - 1;
  output->duration = 1;
  return gst_pad_push (filter->srcpad, output);
}

static gboolean
gst_xsubdecode_sinkpad_setcaps (GstPad *pad, GstCaps *caps)
{
  GstElement *element = GST_PAD_PARENT (pad);
  GstXSUBDecode *filter = GST_XSUBDECODE (element);
  int w = 0;
  int h = 0;
  GstVideoFormat format;
  GstStructure *s;
  gboolean ret;
  guint32 fourcc;

  gst_video_format_parse_caps (caps, &format, &w, &h);
  s = gst_caps_get_structure (caps, 0);

  if (w != 0 && h != 0)
  {
    filter->plane_width = w;
    filter->plane_height = h;
    filter->stride = filter->plane_width * 4;
    filter->plane_size = h * filter->stride;
  }
  else
  {
    GST_INFO_OBJECT (filter, "Did not get size information from demuxer, using SD fall-back");
  }

  if (g_strcmp0 (gst_structure_get_name(s), "subtitle/x-xsub") == 0)
  {
    gst_structure_get_boolean (s, "has-alpha", &filter->has_alpha);
    return TRUE;
  }

  ret = gst_structure_get_fourcc (s, "fourcc", &fourcc);
  if (!ret)
  {
    /* The fourcc didn't exist; we need it to distinguish between other unknown avi
     * streams and subtitle streams. */
    return FALSE;
  }

  if (fourcc != GST_MAKE_FOURCC ('D', 'X', 'S', 'A') &&
      fourcc != GST_MAKE_FOURCC ('D', 'X', 'S', 'B'))
  {
    /* We only entertain DivX subtitles. */
    return FALSE;
  }

  if (fourcc == GST_MAKE_FOURCC('D', 'X', 'S', 'A'))
  {
    /* ideally, this should be cross-checked with the header-length from the
     * RIFF chunk but we don't have that currently.
     */
    filter->has_alpha = TRUE;
  }

  return TRUE;
}

static gboolean
gst_xsubdecode_sinkpad_events (GstPad *pad, GstEvent *event)
{
  gboolean ret = FALSE;
  GstXSUBDecode *filter = GST_XSUBDECODE(gst_pad_get_parent (pad));

  /* Try to pass on language-code tags, the tag pass-through is too early or
   * something for playbin2. This is not necessary when the pipeline is
   * using xsubparse, though.
   */
  if (GST_EVENT_TYPE(event) == GST_EVENT_TAG)
  {
    GstTagList *taglist = NULL;
    char *lang = NULL;

    gst_event_parse_tag (event, &taglist);
    if (gst_tag_list_get_string (taglist, GST_TAG_LANGUAGE_CODE, &lang))
    {
      if (filter->language == NULL)
      {
        filter->language = gst_tag_list_new_full (GST_TAG_LANGUAGE_CODE, lang, NULL);
      }
      else
      {
        /* Last language code wins */
        gst_tag_list_add (filter->language, GST_TAG_MERGE_REPLACE,
            GST_TAG_LANGUAGE_CODE, lang, NULL);
      }
      g_free(lang);
    }
  }
  else
  {
    /* Do nothing */
  }

  ret = gst_pad_push_event (filter->srcpad, event);

  gst_object_unref (filter);
  return ret;
}

static gboolean
gst_xsubdecode_srcpad_events (GstPad *pad, GstEvent *event)
{
  gboolean ret = FALSE;
  GstXSUBDecode *filter = GST_XSUBDECODE(gst_pad_get_parent (pad));

  ret = gst_pad_push_event (filter->sinkpad, event);

  gst_object_unref (filter);
  return ret;
}


/*lint -e826 -e160*/
