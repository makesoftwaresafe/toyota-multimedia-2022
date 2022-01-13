/**
* \file: gst_v4l_csc.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* V4L mem2mem based colorspaceconversion
*
* \component: multimedia/gst
*
* \author: Michael Methner ADITG/SW1 mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
*
***********************************************************************/
#include "gst_v4l_csc.h"
#include "gst_v4l_m2m_buffer.h"

#include <gst/video/video.h>

#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <media/imx6.h>

#include <gst/fsl/gstbufmeta.h>

/* PRQA: Lint Message 826: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826*/

#ifndef PACKAGE
#define PACKAGE "gst_v4l_csc"
#endif


/* We are setting number of buffers (DEFAULT_NUM_OUTPUT_FRAMES) to 3
 * 1) To increase the efficiency compare to 2 buffers
 * 2) When we set number of buffers to 2 and update meta data info in gst_buffer for an hardware
 * copy at gst_apx_sink. The layermanager releases first buffer two times because there is two
 * commit of first buffer from gst_apx_sink in gst_apx_render_buffer.One commit during
 * PAUSED state (PREROLLED) and another commit during PLAYING state.
 */
#define DEFAULT_NUM_OUTPUT_FRAMES 3
#define DEFAULT_USE_PAD_ALLOC FALSE
#define DEFAULT_PASSTHROUGH FALSE
#define DEFAULT_DEVICENAME "/dev/video1"
#define DEFAULT_HFLIP FALSE
#define DEFAULT_VFLIP FALSE

#define DEFAULT_HUE                 0
#define DEFAULT_SATURATION          100
#define DEFAULT_BRIGHTNESS          100
#define DEFAULT_CONTRAST            100
#define DEFAULT_HUE_OFFSET          0
#define DEFAULT_SATURATION_OFFSET   0
#define DEFAULT_BRIGHTNESS_OFFSET   0

#define IDENTITY_MAT4     {{1.0, 0.0, 0.0, 0.0},\
                           {0.0, 1.0, 0.0, 0.0},\
                           {0.0, 0.0, 1.0, 0.0},\
                           {0.0, 0.0, 0.0, 1.0}}

#define NUM_SCALE_FACTORS_IMX6   3
#define IMX6_CSC_IPUIC_NORM_VALUE 256
#define IMX6_CSC_IPUIC_MAX_VALUE 255


#define MAX_PIX_VAL           255
#define MIN_PIX_OFFSET	      -255
#define MAX_BRIGHT_SAT_VAL    100
#define SECTOR_LENGHT_IN_DEG  60
#define MAX_ANGLE             360


/*Insert enums carefully between PROP_HUE and PROP_BRIGHTNESS_OFFSET 
 * as it is used as an array index*/
enum
{
  PROP_NUM_OUTPUT_FRAMES = 1,
  PROP_USE_PAD_ALLOC,
  PROP_DEVICENAME,
  PROP_HFLIP,
  PROP_VFLIP,
  PROP_HUE,
  PROP_SATURATION,
  PROP_BRIGHTNESS,
  PROP_CONTRAST,
  PROP_HUE_OFFSET,
  PROP_SATURATION_OFFSET,
  PROP_BRIGHTNESS_OFFSET,
  PROP_PASSTHROUGH
};

typedef struct CpropCaps_t
{
  gint min;
  gint max;
}CpropCaps;

/*Insert enums carefully between PROP_HUE and PROP_BRIGHTNESS_OFFSET
 * as it is used as an array index*/
static const CpropCaps color_prop_caps[(PROP_BRIGHTNESS_OFFSET - PROP_HUE) + 1] =
{
  {-180, 180},
  {0, 200},
  {0, 200},
  {0, 200},
  {-180, 180},
  {0, 100},
  {-100, 100}
};

static const unsigned int valid_scale_factors[NUM_SCALE_FACTORS_IMX6] = {1, 2, 4};

static float yuv2rgb[4][4] = {    {1.164,    1.164,     1.164, 0.0},
                                        {0.0  ,   -0.392,     2.017, 0.0},
                                        {1.596,   -0.813,     0.0  , 0.0},
                                     {-222.912,  135.616,  -276.800, 1.0}   };

static float rgb2yuv[4][4] = {  {0.299, -0.169,  0.500,   0.0},
                                      {0.587, -0.332, -0.419,   0.0},
                                      {0.114,  0.500, -0.081,   0.0},
                                      {0.0  ,  128.0,  128.0,   1.0}    };

static float rgb2yuvinverse[4][4] = {   {1.0,    1.0,     1.002,  0.0},
                                        {-0.001,   -0.344,     1.772,  0.0},
                                        {1.402,   -0.714,     0.0  ,  0.0},
                                        {-179.297,  135.360,  -226.723,  1.0} };

static float yuv2rgbinverse[4][4] = {  { 0.257  , -0.148   ,  0.439  ,0.0},
                                       { 0.504  , -0.291   , -0.368  ,0.0},
                                       { 0.099  ,  0.439   , -0.072  ,0.0},
                                       {16.0    ,128.0     ,128.0    ,1.0}   };

static GstStaticPadTemplate sink_factory =
  GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        GST_VIDEO_CAPS_YUV ("I420") ";"
        GST_VIDEO_CAPS_YUV ("UYVY") ";"
        GST_VIDEO_CAPS_YUV ("NV12") ";"
        GST_VIDEO_CAPS_YUV ("YUY2") ";"
        GST_VIDEO_CAPS_YUV ("YUYV") ";"
        GST_VIDEO_CAPS_YUV ("YV12") ";"
        GST_VIDEO_CAPS_RGBA ";"
        GST_VIDEO_CAPS_BGRA ";"
        GST_VIDEO_CAPS_RGB_16 ";"
        )
  );


static GstStaticPadTemplate src_factory =
  GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        GST_VIDEO_CAPS_YUV ("I420") ";"
        GST_VIDEO_CAPS_YUV ("UYVY") ";"
        GST_VIDEO_CAPS_YUV ("NV12") ";"
        GST_VIDEO_CAPS_YUV ("YUY2") ";"
        GST_VIDEO_CAPS_YUV ("YUYV") ";"
        GST_VIDEO_CAPS_YUV ("YV12") ";"
        GST_VIDEO_CAPS_RGBA ";"
        GST_VIDEO_CAPS_BGRA ";"
        GST_VIDEO_CAPS_RGB_16 ";"
        )
  );


GST_DEBUG_CATEGORY_STATIC (gst_v4l_csc_debug);
#define GST_CAT_DEFAULT gst_v4l_csc_debug


/* PRQA: Lint Message 19,123, 144, 751, 160: deactivation because macro is related to GStreamer framework */
/*lint -e19 -e123 -e144 -e751 -e160 */
GST_BOILERPLATE (GstV4lCsc,gst_v4l_csc, GstElement,
                GST_TYPE_ELEMENT);
/*lint +e19 +e123 +e144 +e751 +e160 */


static GstStateChangeReturn
gst_v4l_csc_change_state (GstElement *element, GstStateChange transition);

static gboolean
gst_v4l_csc_setcaps(GstPad *pad, GstCaps *caps);


static GstFlowReturn gst_v4l_csc_chain(GstPad * pad, GstBuffer * buf);

static float
gst_v4l_csc_get_bpp(int format);

static void
gst_v4l_csc_finalize(GObject * object);

static void
gst_v4l_csc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void
gst_v4l_csc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);



static short int
gst_v4l_csc_cap_to_valid_offset(float color_offset)
{
  int valid_color_val = (int)round(color_offset);
  short int ret_val;
  if (valid_color_val < MIN_PIX_OFFSET)
  {
    ret_val = MIN_PIX_OFFSET;
  }
  else if (valid_color_val > MAX_PIX_VAL)
  {
    ret_val = MAX_PIX_VAL;
  }
  else
  {
      ret_val = (short int)valid_color_val;
  }
  return ret_val;
}


static void
gst_v4l_csc_hsv_to_rgb_offset(gint Hue_off, guint8 Sat_off,
                                 gint Bright_off, short int *pRed_off,
                                 short int *pGreen_off, short int *pBlue_off)
{
  int actual_hue;
  short int hue_sector;
  float hue_fract;
  float color_circle[6];
  float bright;
  float sat;
  bright = (float)Bright_off / (float)MAX_BRIGHT_SAT_VAL;
  sat = (float)Sat_off / (float)MAX_BRIGHT_SAT_VAL;

  if (0 == Sat_off)
  {
    *pRed_off = gst_v4l_csc_cap_to_valid_offset(bright * MAX_PIX_VAL);
    *pGreen_off = *pRed_off;
    *pBlue_off = *pRed_off;
  }
  else
  {
    actual_hue = Hue_off;
    if ((Hue_off >= MAX_ANGLE) || (Hue_off <= -MAX_ANGLE))
    {
      actual_hue = Hue_off - ((Hue_off / MAX_ANGLE) * MAX_ANGLE);
    }
    if (actual_hue < 0)
    {
      actual_hue += MAX_ANGLE;
    }
    hue_sector = actual_hue / SECTOR_LENGHT_IN_DEG;
    hue_fract = (float)(actual_hue - (hue_sector * (SECTOR_LENGHT_IN_DEG))) /
                (float)SECTOR_LENGHT_IN_DEG;

    /* Each 60 deg sector has a color which will hold same value as brightness.
     * we need to decrease other colors as per saturation
     * Also the variation of colors is made completely linear.
     * E.g. in the first sector, the offset of green should vary linearly
     * from -1.0 to bright with respect to hue variation from 0 to 60 and
     * saturation of 1.0*/

    color_circle[0] = bright;
    color_circle[1] = bright - ((1.0 + bright) * hue_fract * sat);
    color_circle[2] = ((1.0 - sat) - 1.0);
    color_circle[3] = color_circle[2];
    color_circle[4] = (((1.0 + bright) * hue_fract) - 1.0) * sat;
    color_circle[5] = color_circle[0];
    *pRed_off = gst_v4l_csc_cap_to_valid_offset((color_circle[hue_sector]) * (float)MAX_PIX_VAL);
    *pGreen_off = gst_v4l_csc_cap_to_valid_offset(color_circle[(hue_sector + 4) % 6] * (float)MAX_PIX_VAL);
    *pBlue_off = gst_v4l_csc_cap_to_valid_offset(color_circle[(hue_sector + 2) % 6] * (float)MAX_PIX_VAL);
  }
}

static void 
gst_v4l_csc_mat4_multiply(float p_mat14[4][4],
                          float p_mat24[4][4],
                          float p_mat_result_4[4][4])
{
  unsigned int row;
  unsigned int col;
  unsigned int i;
  float result_mat[4][4] = {{0.0, 0.0, 0.0, 0.0},
                            {0.0, 0.0, 0.0, 0.0},
                            {0.0, 0.0, 0.0, 0.0},
                            {0.0, 0.0, 0.0, 0.0}};
  if((NULL != p_mat14) && (NULL != p_mat24) && (NULL != p_mat_result_4))
  {
    for(row = 0; row < 4; row++)
    {
      for(col = 0; col < 4; col++)
      {
        for(i = 0; i < 4; i++)
        {
          result_mat[row][col] += (p_mat14[row][i] * p_mat24[i][col]);
        }
      }
    }
    memcpy(p_mat_result_4, result_mat, sizeof(result_mat));
  }
}

static void 
gst_v4l_csc_get_matrix_abs_max(float p_mat3[3][3],
                              float p_abs_mat[3][3],
                              float *p_max)
{
  unsigned int row;
  unsigned int col;
  float abs_value;
  *p_max = fabsf(p_mat3[0][0]);
  for (row = 0; row < 3; row++)
  {
    for (col = 0; col < 3; col++)
    {
      abs_value = fabsf(p_mat3[row][col]);
      p_abs_mat[row][col] = abs_value;
      if (*p_max < abs_value)
      {
        *p_max = abs_value;
      }
    }
  }
}

static unsigned int 
gst_v4l_csc_get_scale_factor_error(float p_imx6mat[3][3],
                              unsigned int scale_factor,
                              unsigned int norm_value,
                              unsigned int max_value)
{
  unsigned int row;
  unsigned int col;
  unsigned int co_eff;
  unsigned int scaled_coeff;
  unsigned int scale_error = 0;
  /*scale factor is the error introduced due to integral
   * scalar*/
  for (row = 0; row < 3; row++)
  {
    for (col = 0; col < 3; col++)
    {
      co_eff = (unsigned int)round((p_imx6mat[row][col] * (float)norm_value));
      scaled_coeff = ((co_eff + (scale_factor >> 1)) / scale_factor);
      if (scaled_coeff <= max_value)
      {
        if ((scaled_coeff * scale_factor) > co_eff)
        {
          scale_error += ((scaled_coeff * scale_factor) - co_eff);
        }
        else
        {
          scale_error += (co_eff - (scaled_coeff * scale_factor));
        }
      }
      else
      {
        scale_error += (scale_factor * (scaled_coeff - max_value));
      }
    }
  }
  return scale_error;
}

static void
gst_v4l_csc_get_min_max_scalefactors_imx6(unsigned int scale_factor,
                                      unsigned int *p_min_scale_factor,
                                      unsigned int *p_max_scale_factor)
{
  unsigned int idx;
  *p_min_scale_factor = 0;
  *p_max_scale_factor = 0;

  for (idx = 0; idx < NUM_SCALE_FACTORS_IMX6; idx++)
  {
    if (scale_factor <= valid_scale_factors[idx])
    {
      break;
    }
  }

  if (idx >= (NUM_SCALE_FACTORS_IMX6 -1))
  {
    *p_min_scale_factor = valid_scale_factors[NUM_SCALE_FACTORS_IMX6 - 1];
    *p_max_scale_factor = valid_scale_factors[NUM_SCALE_FACTORS_IMX6 - 1];
  }
  else if (0 == scale_factor)
  {
    *p_min_scale_factor = valid_scale_factors[0];
    *p_max_scale_factor = valid_scale_factors[0];
  }
  else
  {
    *p_min_scale_factor = valid_scale_factors[idx];
    *p_max_scale_factor = valid_scale_factors[idx + 1];
  }
}

static void 
gst_v4l_csc_normalise_forimx6(
                            float p_mat3[3][3],
                            unsigned int norm_value,
                            unsigned int max_value,
                            unsigned int scale_factor,
                            int p_imx6mat[3][3])
{
  unsigned int row;
  unsigned int col;
  unsigned int co_eff;
  unsigned int scaled_coeff;
  for (row = 0; row < 3; row++)
  {
    for (col = 0; col < 3; col++)
    {
      co_eff = (unsigned int)round((fabsf(p_mat3[row][col]) * (float)norm_value));
      scaled_coeff = ((co_eff + (scale_factor >> 1)) / scale_factor);
      p_imx6mat[row][col] = scaled_coeff;
      if (scaled_coeff > max_value)
      {
        p_imx6mat[row][col] = max_value;
      }
      if ((p_mat3[row][col] < 0) && (0 != p_imx6mat[row][col]))
      {
        p_imx6mat[row][col] = ((2 * (norm_value)) - p_imx6mat[row][col]);
      }
    }
  }
}

static unsigned int 
gst_v4l_csc_compute_imx6_csc_matrix(float p_mat3[3][3], unsigned int norm_value,
                        unsigned int max_value, int p_imx6mat[3][3])
{
  float abs_mat_max;
  float abs_mat[3][3];
  unsigned int max_mat_value = 0;
  unsigned int scale_factor = 0;
  unsigned int min_scale_factor = 0;
  unsigned int max_scale_factor = 0;
  unsigned int min_scale_factor_error = 0;
  unsigned int max_scale_factor_error = 0;

  gst_v4l_csc_get_matrix_abs_max(p_mat3, abs_mat, &abs_mat_max);

  max_mat_value = (unsigned int) (abs_mat_max * (float)norm_value);
  scale_factor = (max_mat_value / norm_value);
  if (scale_factor > valid_scale_factors[NUM_SCALE_FACTORS_IMX6 - 1])
  {
    GST_ERROR("Scale factor out of range, expect large errors with color space conversion of imx6\n");
  }

  gst_v4l_csc_get_min_max_scalefactors_imx6(scale_factor, &min_scale_factor, &max_scale_factor);
  GST_DEBUG("min and max Scale Factor : %u, %u\n", min_scale_factor, max_scale_factor);
  scale_factor = min_scale_factor;
  if (max_scale_factor != min_scale_factor)
  {
    min_scale_factor_error = gst_v4l_csc_get_scale_factor_error(abs_mat, min_scale_factor, norm_value, max_value);
    max_scale_factor_error = gst_v4l_csc_get_scale_factor_error(abs_mat, max_scale_factor, norm_value, max_value);
    GST_DEBUG("Scale factor errors: min:%u, max:%u\n", min_scale_factor_error, max_scale_factor_error);
    if (max_scale_factor_error < min_scale_factor_error)
    {
      scale_factor = max_scale_factor;
    }
  }
  gst_v4l_csc_normalise_forimx6(p_mat3, norm_value, max_value, scale_factor, p_imx6mat);

  GST_DEBUG("Scale Factor Used: %u\n", scale_factor);
  return scale_factor;
}

static enum imx6_csc_type
gst_v4l_csc_get_csc(int src_format, int sink_format)
{
  enum imx6_csc_type csc_conversion;

  GST_DEBUG("src_format: %d, sink_format:%d\n", src_format, sink_format);
  if ( (   (V4L2_PIX_FMT_YUV420 == src_format)
        || (V4L2_PIX_FMT_NV12 == src_format)
        || (V4L2_PIX_FMT_UYVY == src_format)
        || (V4L2_PIX_FMT_YUYV == src_format)  ) &&
       (   (V4L2_PIX_FMT_BGR32 == sink_format)
        || (V4L2_PIX_FMT_RGB32 == sink_format)
        || (V4L2_PIX_FMT_RGB565 == sink_format)
        || (V4L2_PIX_FMT_RGB24 == sink_format) ) )
  {
    csc_conversion = IMX6_CSC_RGB2YUV;
  }
  else if ( (   (V4L2_PIX_FMT_YUV420 == sink_format)
             || (V4L2_PIX_FMT_NV12 == sink_format)
             || (V4L2_PIX_FMT_UYVY == sink_format)
             || (V4L2_PIX_FMT_YUYV == sink_format)  ) &&
           (   (V4L2_PIX_FMT_BGR32 == src_format)
             || (V4L2_PIX_FMT_RGB32 == src_format)
             || (V4L2_PIX_FMT_RGB565 == src_format)
             || (V4L2_PIX_FMT_RGB24 == src_format) ) )
  {
    csc_conversion = IMX6_CSC_YUV2RGB;
  }
  else if (   (V4L2_PIX_FMT_BGR32 == src_format)
           || (V4L2_PIX_FMT_RGB32 == src_format)
           || (V4L2_PIX_FMT_RGB565 == src_format)
           || (V4L2_PIX_FMT_RGB24 == src_format) )
  {
    csc_conversion = IMX6_CSC_RGB2RGB;
  }
  else
  {
    csc_conversion = IMX6_CSC_YUV2YUV;
  }
  return csc_conversion;
}

static gboolean
gst_v4l_csc_colorParameterDefault(ColorProps *pCprops)
{
  return ((DEFAULT_HUE == pCprops->hue)
      && (DEFAULT_SATURATION == pCprops->saturation)
      && (DEFAULT_BRIGHTNESS == pCprops->brightness)
      && (DEFAULT_CONTRAST == pCprops->contrast)
      && (DEFAULT_HUE_OFFSET == pCprops->hue_offset)
      && (DEFAULT_SATURATION_OFFSET == pCprops->saturation_offset)
      && (DEFAULT_BRIGHTNESS_OFFSET == pCprops->brightness_offset));
}

static void
gst_v4l_csc_apply_offset(gint Hue_off, guint8 Sat_off,
                         gint Bright_off, float trnf_mat[4][4])
{
  if ((0 != Hue_off) || (0 != Sat_off) ||(0 != Bright_off))
  {
    short int R_off;
    short int G_off;
    short int B_off;
    float offset_mat[4][4] = IDENTITY_MAT4;
    gst_v4l_csc_hsv_to_rgb_offset(Hue_off, Sat_off, Bright_off,
                                     &R_off, &G_off, &B_off);
    offset_mat[3][0] = (float)R_off;
    offset_mat[3][1] = (float)G_off;
    offset_mat[3][2] = (float)B_off;

    gst_v4l_csc_mat4_multiply(trnf_mat, offset_mat, trnf_mat);
  }
}

static void
gst_v4l_csc_apply_contrast(guint8 contrast, float trnf_mat[4][4])
{
  if (DEFAULT_CONTRAST != contrast)
  {
    float contrast_mat[4][4] = IDENTITY_MAT4;
    /*contrast is implemented as multiplication of color component
     * and also a corresponding reduction in offset of the color.*/
    contrast_mat[0][0] = ((float)contrast / (float)100.0);
    contrast_mat[1][1] = contrast_mat[0][0];
    contrast_mat[2][2] = contrast_mat[0][0];
    contrast_mat[3][0] = (1.0 - contrast_mat[0][0]) * 128.0;
    contrast_mat[3][1] = contrast_mat[3][0];
    contrast_mat[3][2] = contrast_mat[3][0];
    gst_v4l_csc_mat4_multiply(trnf_mat, contrast_mat, trnf_mat);
  }
}

static void
gst_v4l_csc_apply_hue(gint hue, float trnf_mat[4][4])
{
  /*Compute matrix by rotating around grayscale diagonal of cube
   * in RGB color space*/
  if (DEFAULT_HUE != hue)
  {
    float hue_mat[4][4] = IDENTITY_MAT4;
    float cos_theta;
    float sin_theta;
    cos_theta = cos( (float)(hue * M_PI) / (float)180);
    sin_theta = sin( (float)(hue * M_PI) / (float)180);
    hue_mat[0][0] = cos_theta + (1.0 - cos_theta) / 3.0f;
    hue_mat[0][1] = ((1.0 - cos_theta) / 3.0f) - 0.57735 * sin_theta;
    hue_mat[0][2] = ((1.0 - cos_theta) / 3.0f) + 0.57735 * sin_theta;
    hue_mat[1][0] = hue_mat[0][2];
    hue_mat[1][1] = hue_mat[0][0];
    hue_mat[1][2] = hue_mat[0][1];
    hue_mat[2][0] = hue_mat[0][1];
    hue_mat[2][1] = hue_mat[0][2];
    hue_mat[2][2] = hue_mat[0][0];
    gst_v4l_csc_mat4_multiply(trnf_mat, hue_mat, trnf_mat);
  }
}


static void
gst_v4l_csc_apply_brightness(guint8 brightness, float trnf_mat[4][4])
{
  if (DEFAULT_BRIGHTNESS != brightness)
  {
    float value_mat[4][4] = IDENTITY_MAT4;
    /*Brightness is a simple multiplication of color components*/
    value_mat[0][0] = ((float)(brightness) / (float)(100));
    value_mat[1][1] = value_mat[0][0];
    value_mat[2][2] = value_mat[0][0];
    gst_v4l_csc_mat4_multiply(trnf_mat, value_mat, trnf_mat);
  }
}

static void
gst_v4l_csc_apply_saturation(enum imx6_csc_type col_conversion,
                      guint8 saturation, float trnf_mat[4][4])
{

  if (DEFAULT_SATURATION != saturation)
  {
    float sat_mat[4][4] = IDENTITY_MAT4;
    float trnslation_mat_pre[4][4] = {{1.0, 0.0, 0.0, 0.0},
                                      {0.0, 1.0, 0.0, 0.0},
                                      {0.0, 0.0, 1.0, 0.0},
                                      {0.0, -128.0, -128.0, 1.0}};
    float tranlation_mat_post[4][4] = {{1.0, 0.0, 0.0, 0.0},
                                  {0.0, 1.0, 0.0, 0.0},
                                  {0.0, 0.0, 1.0, 0.0},
                                  {0.0, 128.0, 128.0, 1.0}};
    sat_mat[1][1] = ((float)saturation / (float)100.0);
    sat_mat[2][2] = sat_mat[1][1];
    if (IMX6_CSC_RGB2RGB == col_conversion)
    {
      gst_v4l_csc_mat4_multiply(trnf_mat, rgb2yuv, trnf_mat);
      gst_v4l_csc_mat4_multiply(trnf_mat, trnslation_mat_pre, trnf_mat);
      gst_v4l_csc_mat4_multiply(trnf_mat, sat_mat, trnf_mat);
      gst_v4l_csc_mat4_multiply(trnf_mat, tranlation_mat_post, trnf_mat);
      gst_v4l_csc_mat4_multiply(trnf_mat, rgb2yuvinverse, trnf_mat);
    }
    else
    {
      gst_v4l_csc_mat4_multiply(trnf_mat, trnslation_mat_pre, trnf_mat);
      gst_v4l_csc_mat4_multiply(trnf_mat, sat_mat, trnf_mat);
      gst_v4l_csc_mat4_multiply(trnf_mat, tranlation_mat_post, trnf_mat);
    }
  }
}

static void 
gst_v4l_csc_setup_csc(int fd, enum imx6_csc_type col_conversion,
                      ColorProps *pCprops)
{
  struct imx6_csc csc = {0};
  float trans3x3[3][3];
  int imx6_csc[3][3];
  int offset[3] = {0,0,0};
  int i, j;
  int ret;
  unsigned int scale_factor;
  float trnf_mat[4][4] = IDENTITY_MAT4;

  if (   (!gst_v4l_csc_colorParameterDefault(pCprops))
      || (IMX6_CSC_RGB2YUV == col_conversion         )
      || (IMX6_CSC_YUV2RGB == col_conversion         ))
  {
    if (   (IMX6_CSC_YUV2RGB == col_conversion)
        || (IMX6_CSC_YUV2YUV == col_conversion)
        || (IMX6_CSC_RGB2RGB == col_conversion) )
    {
      gst_v4l_csc_apply_saturation(col_conversion, pCprops->saturation, trnf_mat);
    }
    if (   (IMX6_CSC_YUV2RGB == col_conversion)
        || (IMX6_CSC_YUV2YUV == col_conversion) )
    {
      gst_v4l_csc_mat4_multiply(trnf_mat, yuv2rgb, trnf_mat);
    }
    gst_v4l_csc_apply_brightness(pCprops->brightness, trnf_mat);
    gst_v4l_csc_apply_hue(pCprops->hue, trnf_mat);
    gst_v4l_csc_apply_contrast(pCprops->contrast, trnf_mat);
    gst_v4l_csc_apply_offset(pCprops->hue_offset, pCprops->saturation_offset,
                             pCprops->brightness_offset, trnf_mat);

    if (IMX6_CSC_YUV2YUV == col_conversion)
    {
      gst_v4l_csc_mat4_multiply(trnf_mat, yuv2rgbinverse, trnf_mat);
    }
    else if  (IMX6_CSC_RGB2YUV == col_conversion)
    {
      gst_v4l_csc_mat4_multiply(trnf_mat, rgb2yuv, trnf_mat);
      gst_v4l_csc_apply_saturation(col_conversion, pCprops->saturation, trnf_mat);
    }
  }
  /*Transpose the matrix to column major matrix*/
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
        trans3x3[i][j] = trnf_mat[j][i];
    }
  }

  GST_DEBUG("\nFloating point csc matrix\n\n");
  for (i = 0; i < 3; i++)
  {
    GST_DEBUG("%f, %f, %f\n", trans3x3[i][0], trans3x3[i][1], trans3x3[i][2]);
  }
  GST_DEBUG("%f, %f, %f\n", trnf_mat[3][0], trnf_mat[3][1], trnf_mat[3][2]);

  scale_factor = gst_v4l_csc_compute_imx6_csc_matrix (trans3x3, IMX6_CSC_IPUIC_NORM_VALUE,
                                        IMX6_CSC_IPUIC_MAX_VALUE, imx6_csc);

  for (i = 0; i < 3; i++)
  {
    offset[i] = (int)round(trnf_mat[3][i]);
    offset[i] *= (4 / scale_factor);
  }
  GST_DEBUG("\n\nimx6 ipu-ic csc matrix\n\n");
  for (i = 0; i < 3; i++)
  {
    GST_DEBUG("%d, %d, %d\n", imx6_csc[i][0], imx6_csc[i][1], imx6_csc[i][2]);
  }
  GST_DEBUG("%d, %d, %d\n", offset[0], offset[1], offset[2]);
  memcpy (csc.csc, imx6_csc, sizeof(imx6_csc));
  memcpy (csc.off, offset, sizeof(offset));
  csc.exp = scale_factor >= 4 ? 3 : scale_factor;
  csc.sat = IMX6_CSC_SAT_0_255;
  csc.enable = true;
  csc.type = col_conversion;
  ret = ioctl(fd, VIDIOC_IMX6_SET_CSC, &csc);
  if (ret)
    GST_ERROR("set csc failed");
}

static void
gst_v4l_csc_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "gst_v4l_csc",
    "Filter/Converter/Video",
    "gst_v4l_csc",
    "Michael Methner mmethner@de.adit-jv.com");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
}


static void
gst_v4l_csc_class_init (GstV4lCscClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->finalize = gst_v4l_csc_finalize;
  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_v4l_csc_change_state);

  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_v4l_csc_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_v4l_csc_get_property);

  g_object_class_install_property (gobject_class, PROP_DEVICENAME,
       g_param_spec_string("devicename", "devicename",
           "V4L devicename for mem2mem operation",
           DEFAULT_DEVICENAME, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_USE_PAD_ALLOC,
         g_param_spec_boolean("use-pad-alloc", "use-pad-alloc",
             "use-pad-alloc function on srcpad or push mmapped buffers",
             DEFAULT_USE_PAD_ALLOC, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NUM_OUTPUT_FRAMES,
       g_param_spec_uint("num-output-frames", "num-output-frames",
           "Number of output frames when using pad-alloc = FALSE",
           0x00000000, 0xFFFFFFFF, DEFAULT_NUM_OUTPUT_FRAMES, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HFLIP,
         g_param_spec_boolean("hflip", "hflip",
             "Perform horizontal flip (around Y axis)",
             DEFAULT_HFLIP, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_VFLIP,
         g_param_spec_boolean("vflip", "vflip",
             "Perform vertical flip (around X axis)",
             DEFAULT_VFLIP, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HUE,
       g_param_spec_int("hue", "hue",
           "color space rotation angle",
           color_prop_caps[0].min, color_prop_caps[0].max, DEFAULT_HUE,
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SATURATION,
       g_param_spec_int("saturation", "saturation",
           "saturation as defined by HSV color space",
           color_prop_caps[1].min, color_prop_caps[1].max, DEFAULT_SATURATION,
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BRIGHTNESS,
       g_param_spec_int("brightness", "brightness",
           "Brightness as defined by value in HSV color space",
           color_prop_caps[2].min, color_prop_caps[2].max, DEFAULT_BRIGHTNESS,
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CONTRAST,
       g_param_spec_int("contrast", "contrast",
           "contrast is the extent one can distiguish colors",
           color_prop_caps[3].min, color_prop_caps[3].max, DEFAULT_CONTRAST,
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HUE_OFFSET,
       g_param_spec_int("hue offset", "hue offset",
           "hue value for the color offset which is added to each pixel",
           color_prop_caps[4].min, color_prop_caps[4].max, DEFAULT_HUE_OFFSET,
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SATURATION_OFFSET,
       g_param_spec_int("saturation offset", "saturation offset",
           "saturation value for the color offset which is added to each pixel",
           color_prop_caps[5].min, color_prop_caps[5].max, DEFAULT_SATURATION_OFFSET,
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BRIGHTNESS_OFFSET,
       g_param_spec_int("brightness offset", "brightness offset",
           "brightness value for the color offset which is added to each pixel",
           color_prop_caps[6].min, color_prop_caps[6].max, DEFAULT_BRIGHTNESS_OFFSET,
           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PASSTHROUGH,
           g_param_spec_boolean("passthrough", "passthrough",
               "By passes processing if sink and src pad are of same capabilites",
               DEFAULT_PASSTHROUGH, G_PARAM_READWRITE));
}


static void
gst_v4l_csc_finalize(GObject * object)
{
  GstV4lCsc * v4l_csc = GST_V4L_CSC(object);

  g_mutex_free(v4l_csc->pool_lock);
  g_cond_free(v4l_csc->pool_data);
  v4l_csc->pool_lock = NULL;
  v4l_csc->pool_data = NULL;

  g_free(v4l_csc->devicename);
}


static void
gst_v4l_csc_init (GstV4lCsc * v4l_csc,
    GstV4lCscClass * gclass)
{
  GstPadTemplate *sink_pad_template;
  GstPadTemplate *src_pad_template;

  gclass = gclass;

  sink_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_CLASS(
      gclass), "sink");

  v4l_csc->sinkpad = gst_pad_new_from_template(sink_pad_template, "sink");
  gst_pad_use_fixed_caps(v4l_csc->sinkpad);
  gst_pad_set_chain_function(v4l_csc->sinkpad, GST_DEBUG_FUNCPTR(
      gst_v4l_csc_chain));
  gst_pad_set_setcaps_function(v4l_csc->sinkpad, GST_DEBUG_FUNCPTR(
      gst_v4l_csc_setcaps));

  src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_CLASS(
      gclass), "src");
  if (src_pad_template == NULL)
    {
      GST_ERROR("Could not get source pad template");
      return;
    }
  v4l_csc->srcpad = gst_pad_new_from_template(src_pad_template, "src");

  gst_pad_use_fixed_caps(v4l_csc->srcpad);

  gst_element_add_pad(GST_ELEMENT(v4l_csc), v4l_csc->sinkpad);
  gst_element_add_pad(GST_ELEMENT(v4l_csc), v4l_csc->srcpad);

  v4l_csc->num_output_frames = DEFAULT_NUM_OUTPUT_FRAMES;
  v4l_csc->use_pad_alloc = DEFAULT_USE_PAD_ALLOC;
  v4l_csc->devicename = g_strdup(DEFAULT_DEVICENAME);

  v4l_csc->pool_lock = g_mutex_new();
  v4l_csc->pool_data = g_cond_new();
  v4l_csc->buffer_pool = NULL;
  v4l_csc->CProps.hue  = DEFAULT_HUE;
  v4l_csc->CProps.saturation = DEFAULT_SATURATION;
  v4l_csc->CProps.brightness = DEFAULT_BRIGHTNESS;
  v4l_csc->CProps.contrast = DEFAULT_CONTRAST;
  v4l_csc->CProps.hue_offset = DEFAULT_HUE_OFFSET;
  v4l_csc->CProps.saturation_offset = DEFAULT_SATURATION_OFFSET;
  v4l_csc->CProps.brightness_offset = DEFAULT_BRIGHTNESS_OFFSET;
  v4l_csc->CProps.update_cprops = TRUE;
  v4l_csc->pass_through = FALSE;
}

static gint gst_v4l_csc_get_capped_color_prop(gint property, gint value)
{
  gint ret = value;
  if (value < color_prop_caps[property - PROP_HUE].min)
  {
    ret = color_prop_caps[property - PROP_HUE].min;
  }
  if (value > color_prop_caps[property - PROP_HUE].max)
  {
    ret = color_prop_caps[property - PROP_HUE].max;
  }
  return ret;
}

static void
gst_v4l_csc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstV4lCsc * v4l_csc = GST_V4L_CSC(object);

  switch (prop_id) {
    case PROP_USE_PAD_ALLOC:
      v4l_csc->use_pad_alloc = g_value_get_boolean(value);
      break;
    case PROP_NUM_OUTPUT_FRAMES:
        v4l_csc->num_output_frames = g_value_get_uint(value);
        break;
    case PROP_DEVICENAME:
        if(v4l_csc->devicename)
          {
            g_free(v4l_csc->devicename);
          }
        v4l_csc->devicename = g_value_dup_string (value);
        break;
    case PROP_HFLIP:
      v4l_csc->hflip = g_value_get_boolean(value);
      break;
    case PROP_VFLIP:
      v4l_csc->vflip = g_value_get_boolean(value);
      break;
    case PROP_HUE:
      v4l_csc->CProps.hue = \
        gst_v4l_csc_get_capped_color_prop(PROP_HUE,g_value_get_int(value));
      break;
    case PROP_SATURATION:
      v4l_csc->CProps.saturation = (guint8)\
        gst_v4l_csc_get_capped_color_prop(PROP_SATURATION,g_value_get_int(value));
      break;
    case PROP_BRIGHTNESS:
      v4l_csc->CProps.brightness = (guint8)\
        gst_v4l_csc_get_capped_color_prop(PROP_BRIGHTNESS,g_value_get_int(value));
      break;
    case PROP_CONTRAST:
      v4l_csc->CProps.contrast = (guint8)\
        gst_v4l_csc_get_capped_color_prop(PROP_CONTRAST,g_value_get_int(value));
      break;
    case PROP_HUE_OFFSET:
      v4l_csc->CProps.hue_offset = \
        gst_v4l_csc_get_capped_color_prop(PROP_HUE_OFFSET,g_value_get_int(value));
      break;
    case PROP_SATURATION_OFFSET:
      v4l_csc->CProps.saturation_offset = (guint8)\
        gst_v4l_csc_get_capped_color_prop(PROP_SATURATION_OFFSET,g_value_get_int(value));
      break;
    case PROP_BRIGHTNESS_OFFSET:
      v4l_csc->CProps.brightness_offset = \
        gst_v4l_csc_get_capped_color_prop(PROP_BRIGHTNESS_OFFSET,g_value_get_int(value));
      break;
    case PROP_PASSTHROUGH:
       v4l_csc->passthrough_mode = g_value_get_boolean(value);
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  if ((prop_id >= PROP_HUE) && (prop_id <= PROP_BRIGHTNESS_OFFSET))
  {
    v4l_csc->CProps.update_cprops = TRUE;
  }
}


static void
gst_v4l_csc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstV4lCsc * v4l_csc = GST_V4L_CSC(object);

  switch (prop_id) {
    case PROP_USE_PAD_ALLOC:
      g_value_set_boolean(value,v4l_csc->use_pad_alloc);
      break;
    case PROP_NUM_OUTPUT_FRAMES:
      g_value_set_uint(value, v4l_csc->num_output_frames);
      break;
    case PROP_DEVICENAME:
      g_value_set_string (value, v4l_csc->devicename);
      break;
    case PROP_HFLIP:
      g_value_set_boolean(value,v4l_csc->hflip);
      break;
    case PROP_VFLIP:
      g_value_set_boolean(value,v4l_csc->vflip);
      break;
    case PROP_HUE:
      g_value_set_int(value, v4l_csc->CProps.hue);
      break;
    case PROP_SATURATION:
      g_value_set_int(value, (gint)v4l_csc->CProps.saturation);
      break;
    case PROP_BRIGHTNESS:
      g_value_set_int(value, (gint)v4l_csc->CProps.brightness);
      break;
    case PROP_CONTRAST:
      g_value_set_int(value, (gint)v4l_csc->CProps.contrast);
      break;
    case PROP_HUE_OFFSET:
      g_value_set_int(value, v4l_csc->CProps.hue_offset);
      break;
    case PROP_SATURATION_OFFSET:
      g_value_set_int(value, (gint)v4l_csc->CProps.saturation_offset);
      break;
    case PROP_BRIGHTNESS_OFFSET:
      g_value_set_int(value, v4l_csc->CProps.brightness_offset);
      break;
    case PROP_PASSTHROUGH:
      g_value_set_boolean(value, v4l_csc->passthrough_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_v4l_csc_change_state(GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstV4lCsc * v4l_csc = GST_V4L_CSC(element);

  switch (transition)
  {
    case GST_STATE_CHANGE_NULL_TO_READY:
          {
            struct v4l2_capability cap;
            int retval;
            v4l_csc->fd = open(v4l_csc->devicename, O_RDWR,0);
            if(!v4l_csc->fd)
              {
                GST_ERROR("could not open device: %s", v4l_csc->devicename);
                return GST_STATE_CHANGE_FAILURE;
              }
            retval = ioctl(v4l_csc->fd, VIDIOC_QUERYCAP, &cap);
            if(retval != 0 ||
               !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
               !(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT))
              {
                GST_ERROR("device does not support all required features: 0x%x", cap.capabilities);
                return GST_STATE_CHANGE_FAILURE;
              }
            GST_LOG("device opened successfully. caps: 0x%x", cap.capabilities);
            v4l_csc->running = TRUE;
            break;
          }
    case GST_STATE_CHANGE_READY_TO_PAUSED:
          v4l_csc->pass_through = FALSE;
          v4l_csc->running = TRUE;
          break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
          break;
    default:
        break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state(element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
  {
    GST_ERROR("state change failure");
    return ret;
  }

  switch (transition)
  {
    case GST_STATE_CHANGE_READY_TO_NULL:
      {
        g_mutex_lock(v4l_csc->pool_lock);
        while (v4l_csc->buffer_pool)
          {
            GstBuffer * buffer = (GstBuffer *) v4l_csc->buffer_pool->data;
            v4l_csc->buffer_pool = g_slist_delete_link(v4l_csc->buffer_pool,
                v4l_csc->buffer_pool);
            gst_buffer_unref(buffer);
          }
        g_mutex_unlock(v4l_csc->pool_lock);


        if(v4l_csc->fd)
          {
            close(v4l_csc->fd);
          }

        v4l_csc->init = FALSE;
      }
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      v4l_csc->pass_through = FALSE;
      v4l_csc->running = FALSE;
      if(v4l_csc->fd)
        {
          enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
          int retval = ioctl(v4l_csc->fd, VIDIOC_STREAMOFF, &type);
          if(retval != 0)
            {
              GST_ERROR("VIDIOC_STREAMOFF failed (OUTPUT)");
            }
          type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
          retval = ioctl(v4l_csc->fd, VIDIOC_STREAMOFF, &type);
          if(retval != 0)
            {
              GST_ERROR("VIDIOC_STREAMOFF failed (CAPTURE)");
            }
        }
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      {

      }
      break;
    default:
      break;
  }

  return ret;
}

gboolean
gst_v4l_csc_get_colorformat(GstV4lCsc * v4l_csc,GstStructure * s,int * color_format)
{
  gboolean retval = TRUE;
  if(strcmp (gst_structure_get_name (s), "video/x-raw-yuv") == 0)
    {
      guint fourcc;
      gst_structure_get_fourcc (s, "format", &fourcc);

     /* Fixme: For YV12 format only pass through is supported.
      * No color conversion is supported from/to any format*/
      if(v4l_csc->passthrough_mode)
        {
          if (GST_MAKE_FOURCC('Y', 'V', '1', '2') == fourcc)
            {
              *color_format = V4L2_PIX_FMT_YVU420;
              return TRUE;
            }
       }

      switch (fourcc)
      {
      case GST_MAKE_FOURCC('I', '4', '2', '0'):
        *color_format = V4L2_PIX_FMT_YUV420;
        break;
      case GST_MAKE_FOURCC('U', 'Y', 'V', 'Y'):
        *color_format = V4L2_PIX_FMT_UYVY;
        break;
      case GST_MAKE_FOURCC('Y', 'U', 'Y', '2'):
        *color_format = V4L2_PIX_FMT_YUYV;
        break;
      case GST_MAKE_FOURCC('N', 'V', '1', '2'):
        *color_format = V4L2_PIX_FMT_NV12;
        break;
      case GST_MAKE_FOURCC('Y', 'U', 'Y', 'V'):
        *color_format = V4L2_PIX_FMT_YUYV;
        break;
      default:
        retval = FALSE;
      }
    }
  else if(strcmp (gst_structure_get_name (s), "video/x-raw-rgb") == 0)
    {
      gint depth;
      gst_structure_get_int (s, "depth", &depth);
      switch(depth)
      {
      case 16:
        *color_format = V4L2_PIX_FMT_RGB565;
        break;
      case 24:
        *color_format = V4L2_PIX_FMT_RGB24;
        break;
      case 32:
        {
          gint32 blue_mask;
          gst_structure_get_int (s, "blue_mask", &blue_mask);
          if(blue_mask == (gint32)0xFF000000)
            {

              *color_format = V4L2_PIX_FMT_BGR32;
            }
          else
            {
              *color_format = V4L2_PIX_FMT_RGB32;
            }
          break;
        }
      default:
        retval = FALSE;
      }
    }
  else
    {
      retval = FALSE;
    }

  return retval;
}

gint
gst_v4l_csc_negotiate_int(gint value,GstStructure * s, char * name)
{
  if(gst_structure_has_field_typed (s,name,GST_TYPE_INT_RANGE))
    {
      gst_structure_fixate_field_nearest_int (s,name,value);
    }
  else if(gst_structure_has_field_typed (s,name,G_TYPE_INT))
    {
      /* NOTHING TO DONE */
    }
  else
    {
      /* create field */
      GValue val;
      memset(&val,0,sizeof(val));
      g_value_init (&val, G_TYPE_INT);
      g_value_set_int (&val, value);
      gst_structure_set_value(s,name,&val);
      g_value_unset(&val);
    }

  if(!gst_structure_get_int(s, name, &value))
      {
        gst_structure_free(s);
        GST_WARNING("could not parse %s",name);
      }

  return value;
}


void
gst_v4l_csc_structure_set_fraction (
     GstStructure * s, char * name , gint n , gint d)

{
  GValue val;
  memset(&val,0,sizeof(val));
  g_value_init (&val, GST_TYPE_FRACTION);
  gst_value_set_fraction (&val, n, d);
  gst_structure_set_value(s,name,&val);
  g_value_unset(&val);
}


gboolean
gst_v4l_csc_set_src_format(GstV4lCsc * v4l_csc, GstStructure * s)
{
  int color_format;

  /* we need a own reference for modifications */
  s=gst_structure_copy(s);

  if(!gst_v4l_csc_get_colorformat(v4l_csc,s,&color_format))
    {
      gst_structure_free(s);
      GST_WARNING("unsupported colorformat");
      return FALSE;
    }

  v4l_csc->src_width = gst_v4l_csc_negotiate_int(v4l_csc->sink_width,s, "width");
  v4l_csc->src_height = gst_v4l_csc_negotiate_int(v4l_csc->sink_height,s, "height");
  gst_v4l_csc_structure_set_fraction (
      s,"framerate",v4l_csc->framerate_n,v4l_csc->framerate_d);
  gst_v4l_csc_structure_set_fraction (
      s,"pixel-aspect-ratio",v4l_csc->par_n,v4l_csc->par_d);
  v4l_csc->src_format = color_format;
  v4l_csc->src_bpp = gst_v4l_csc_get_bpp(v4l_csc->src_format);


  GST_LOG("negotiated src caps: %" GST_PTR_FORMAT,s);
  v4l_csc->src_caps = gst_caps_new_full(s, NULL);

  if(!gst_pad_set_caps(v4l_csc->srcpad, v4l_csc->src_caps))
    {
      gst_caps_unref(v4l_csc->src_caps);
      v4l_csc->src_caps = NULL;
      GST_WARNING("could not set src caps");
      return FALSE;
    }

  return TRUE;
}

gboolean
gst_v4l_csc_check_passthrough(GstV4lCsc * v4l_csc)
{
  GstCaps * src_caps = gst_pad_get_allowed_caps (v4l_csc->srcpad);
  guint size = gst_caps_get_size (src_caps);
  gboolean ret = FALSE;
  guint i;
  for(i=0;i<size;i++)
    {
      int color_format = 0;
      GstStructure * src_s = gst_caps_get_structure(src_caps, i);

      gst_v4l_csc_get_colorformat(v4l_csc,src_s,&color_format);

      if(v4l_csc->sink_format == color_format)
        {
          if(gst_v4l_csc_set_src_format(v4l_csc, src_s))
            {
              v4l_csc->pass_through = TRUE;
              GST_LOG("Sink and Src pad are same format,So do pass through");
              ret = TRUE;
              break;
            }
        }
	}
  gst_caps_unref(src_caps);
  return ret;
}

static gboolean
gst_v4l_csc_setcaps(GstPad *pad, GstCaps *caps)
{
  int color_format;
  GstStructure *s;
  GstElement * element = GST_PAD_PARENT(pad);
  GstV4lCsc * v4l_csc = GST_V4L_CSC(element);

  s = gst_caps_get_structure(caps, 0);

  GST_LOG("setting caps %" GST_PTR_FORMAT, s);

  if(!gst_structure_get_int(s, "width", &v4l_csc->sink_width))
      {
        GST_ERROR("could not parse width");
        return FALSE;
      }

  if(!gst_structure_get_int(s, "height", &v4l_csc->sink_height))
      {
        GST_ERROR("could not parse height");
        return FALSE;
      }

  if(!gst_structure_get_fraction(s,"pixel-aspect-ratio",&v4l_csc->par_n,&v4l_csc->par_d))
      {
        /* assume default */
        v4l_csc->par_n = 1;
        v4l_csc->par_d = 1;
      }

  if(!gst_structure_get_fraction(s,"framerate",&v4l_csc->framerate_n,&v4l_csc->framerate_d))
        {
          /* assume default */
          v4l_csc->framerate_n = 30;
          v4l_csc->framerate_d = 1;
        }

  if(!gst_v4l_csc_get_colorformat(v4l_csc,s,&color_format))
    {
      GST_ERROR("unsupported colorformat");
      return FALSE;
    }

  v4l_csc->sink_format = color_format;

  GST_LOG("resolution: %dx%d format: 0x%x",v4l_csc->sink_width,v4l_csc->sink_height,
      v4l_csc->sink_format);

  if(v4l_csc->passthrough_mode)
    {
      if(gst_v4l_csc_check_passthrough(v4l_csc))
        return TRUE;
    }

  /* negotiate downstream */
  GstCaps * src_caps = gst_pad_get_allowed_caps (v4l_csc->srcpad);
  gboolean negotiated=FALSE;
  guint size = gst_caps_get_size (src_caps);
  guint i;
  for(i=0;i<size;i++)
    {
      GstStructure * src_s = gst_caps_get_structure(src_caps, i);
      if(gst_v4l_csc_set_src_format(v4l_csc, src_s))
        {
          negotiated=TRUE;
          break;
        }
    }

  gst_caps_unref(src_caps);

  return negotiated;
}


static GstV4lM2mBuffer *
gst_v4l_csc_get_buffer_from_queue(GstV4lCsc * v4l_csc)
{
  g_mutex_lock(v4l_csc->pool_lock);
  while (!v4l_csc->buffer_pool)
    {
      GST_LOG("waiting for buffer ...");
      g_cond_wait(v4l_csc->pool_data, v4l_csc->pool_lock);
    }

  GstV4lM2mBuffer * buffer = (GstV4lM2mBuffer *) v4l_csc->buffer_pool->data;
  v4l_csc->buffer_pool = g_slist_delete_link(v4l_csc->buffer_pool,
      v4l_csc->buffer_pool);
  g_mutex_unlock(v4l_csc->pool_lock);

  return buffer;
}

static void
gst_v4l_csc_queue_buffer(GstV4lCsc * v4l_csc, GstV4lM2mBuffer * buffer)
{
  g_mutex_lock(v4l_csc->pool_lock);
  v4l_csc->buffer_pool = g_slist_append(
      v4l_csc->buffer_pool, buffer);
  g_cond_signal(v4l_csc->pool_data);
  g_mutex_unlock(v4l_csc->pool_lock);
}



static gboolean gst_v4l_csc_finalize_buffer(GstV4lM2mBuffer * buffer, GstElement * element)
{
  gboolean retval = FALSE;
  GstV4lCsc * v4l_csc = GST_V4L_CSC(element);

  if(v4l_csc->running)
    {
      gst_buffer_ref(GST_BUFFER(buffer));
      gst_v4l_csc_queue_buffer(v4l_csc, buffer);
      retval = TRUE;
    }
  else
    {
      GST_LOG("munmap %p (%d)",buffer, buffer->index);
      munmap(GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));

      GstBuffer *gst_buf = GST_BUFFER(buffer);
      gint buf_index = G_N_ELEMENTS (gst_buf->_gst_reserved) - 1;
      if(GST_BUFFER(buffer)->_gst_reserved[buf_index])
        {
          GST_LOG("meta release for %d", buf_index);
          gst_buffer_meta_free (GST_BUFFER(buffer)->_gst_reserved[buf_index]);
        }

      GST_BUFFER_DATA(buffer) = NULL;
      GST_BUFFER_SIZE(buffer) = 0;
      retval = FALSE;
    }

  return retval;
}

static GstV4lM2mBuffer *
gst_v4l_csc_buffer_new(GstV4lCsc * v4l_csc)
{
  GstV4lM2mBuffer * buffer;
  buffer = GST_V4L_M2M_BUFFER(gst_mini_object_new (GST_TYPE_V4L_M2M_BUFFER));

  buffer->parent = gst_object_ref(v4l_csc);
  buffer->finalize = &gst_v4l_csc_finalize_buffer;

  GST_LOG("created new GstV4lM2mBuffer: %p", buffer);
  return buffer;
}


gboolean
gst_v4l_csc_init_v4l(GstV4lCsc * v4l_csc)
{
  int ret;
  struct v4l2_format fmt;
  struct v4l2_requestbuffers reqbuf;
  struct v4l2_control ctrl;
  enum v4l2_buf_type type;

  memset(&fmt, 0, sizeof(fmt));
  memset(&reqbuf, 0, sizeof(reqbuf));


  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = v4l_csc->src_width;
  fmt.fmt.pix.height = v4l_csc->src_height;
  fmt.fmt.pix.pixelformat = v4l_csc->src_format;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;

  GST_LOG("src: %d x %d, format: 0x%x\n",fmt.fmt.pix.width,fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);

  ret = ioctl(v4l_csc->fd, VIDIOC_S_FMT, &fmt);
  if(ret != 0)
    {
      GST_ERROR("could not set output format");
      return FALSE;
    }

  fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  fmt.fmt.pix.width = v4l_csc->sink_width;
  fmt.fmt.pix.height = v4l_csc->sink_height;
  fmt.fmt.pix.pixelformat = v4l_csc->sink_format;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;

  GST_LOG("sink: %d x %d, format: 0x%x\n",fmt.fmt.pix.width,fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);

  ret = ioctl(v4l_csc->fd, VIDIOC_S_FMT, &fmt);
  if(ret != 0)
    {
      GST_ERROR("could not set capture format");
      return FALSE;
    }

  if(v4l_csc->vflip)
    {
      ctrl.id = V4L2_CID_VFLIP;
      ctrl.value = 1;
      ret = ioctl(v4l_csc->fd, VIDIOC_S_CTRL, &ctrl);
      if(ret != 0)
        {
          GST_ERROR("could not set vflip");
          return FALSE;
        }
    }

  if(v4l_csc->hflip)
    {
      ctrl.id = V4L2_CID_HFLIP;
      ctrl.value = 1;
      ret = ioctl(v4l_csc->fd, VIDIOC_S_CTRL, &ctrl);
      if(ret != 0)
        {
          GST_ERROR("could not set hflip");
          return FALSE;
        }
    }


  reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  reqbuf.memory = V4L2_MEMORY_USERPTR;
  reqbuf.count = 1;
  ret = ioctl(v4l_csc->fd, VIDIOC_REQBUFS, &reqbuf);
  if(ret != 0)
    {
      GST_ERROR("VIDIOC_REQBUFS failed (OUTPUT)");
      return FALSE;
    }

  if(v4l_csc->use_pad_alloc)
    {
      reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      reqbuf.memory = V4L2_MEMORY_USERPTR;
      ret = ioctl(v4l_csc->fd, VIDIOC_REQBUFS, &reqbuf);
      if(ret != 0)
        {
          GST_ERROR("VIDIOC_REQBUFS failed (CAPTURE)");
          return FALSE;
        }
    }
  else
    {
      int i;
      struct v4l2_buffer buf;
      memset(&buf,0,sizeof(buf));

      reqbuf.count = v4l_csc->num_output_frames;
      reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      reqbuf.memory = V4L2_MEMORY_MMAP;
      ret = ioctl(v4l_csc->fd, VIDIOC_REQBUFS, &reqbuf);
      if(ret != 0)
        {
          GST_ERROR("VIDIOC_REQBUFS failed (CAPTURE)");
          return FALSE;
        }

      for (i = 0; i < v4l_csc->num_output_frames; i++)
        {
          buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
          buf.memory = V4L2_MEMORY_MMAP;
          buf.index = i;

          ret = ioctl(v4l_csc->fd, VIDIOC_QUERYBUF, &buf);
          if(ret != 0)
            {
              GST_ERROR("VIDIOC_QUERYBUF failed (CAPTURE)");
              return FALSE;
            }

          GstV4lM2mBuffer * buffer = gst_v4l_csc_buffer_new(v4l_csc);
          buffer->index = buf.index;
          GST_BUFFER(buffer)->size = buf.length;
          GST_BUFFER(buffer)->data = mmap(NULL, buf.length, PROT_READ,
              MAP_SHARED, v4l_csc->fd, buf.m.offset);
          if(GST_BUFFER(buffer)->data == MAP_FAILED)
            {
              GST_ERROR("MMAP failed (CAPTURE)");
              return FALSE;
            }

          ret = ioctl (v4l_csc->fd, VIDIOC_IMX6_GET_PHYS, &buf);
          if(ret != 0)
            {
              /* We do software copy in next element (sink) */
              GST_ERROR ("VIDIOC_IMX6_GET_PHYS failed ");
            }
          else
            {
              /* We will update physical address of v4l2 buffer
               * to do hardware copy in next element (sink)
               */
              gint buf_index;
              GstBuffer *gst_buf = GST_BUFFER(buffer);
              buf_index = G_N_ELEMENTS (gst_buf->_gst_reserved) - 1;
              GstBufferMeta *meta = gst_buffer_meta_new ();
              meta->physical_data = (gpointer) (buf.m.offset);
              meta->priv = buffer;
              GST_BUFFER(buffer)->_gst_reserved[buf_index] = meta;
            }

          gst_v4l_csc_queue_buffer(v4l_csc,buffer);

        }
    }


  type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  ret = ioctl(v4l_csc->fd, VIDIOC_STREAMON, &type);
  if(ret != 0)
    {
      GST_ERROR("VIDIOC_STREAMON failed (OUTPUT)");
      return FALSE;
    }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ret = ioctl(v4l_csc->fd, VIDIOC_STREAMON, &type);
  if(ret != 0)
    {
      GST_ERROR("VIDIOC_STREAMON failed (CAPTURE)");
      return FALSE;
    }

  return TRUE;
}

static float
gst_v4l_csc_get_bpp(int format)
{
  float bpp = 0;
  switch(format)
  {
  case V4L2_PIX_FMT_YUV420:
  case V4L2_PIX_FMT_NV12:
    bpp=1.5;
    break;
  case V4L2_PIX_FMT_UYVY:
  case V4L2_PIX_FMT_YUYV:
    bpp=2.0f;
    break;
  case V4L2_PIX_FMT_BGR32:
  case V4L2_PIX_FMT_RGB32:
    bpp=4.0f;
    break;
  case V4L2_PIX_FMT_RGB565:
    bpp=2.0f;
    break;
  case V4L2_PIX_FMT_RGB24:
    bpp=3.0f;
    break;
  }

  return bpp;
}

static GstFlowReturn
gst_v4l_csc_chain(GstPad * sink, GstBuffer * buffer)
{
  GstElement * element = GST_PAD_PARENT(sink);
  GstV4lCsc * v4l_csc = GST_V4L_CSC(element);
  struct v4l2_buffer buf;
  int ret;
  enum imx6_csc_type color_conversion;

  if(v4l_csc->pass_through)
    {
      GstBuffer * outbuf = NULL;
      outbuf = gst_buffer_ref(buffer);
      GstFlowReturn gst_ret = gst_pad_push(v4l_csc->srcpad, outbuf);
      gst_buffer_unref(buffer);
      return gst_ret;
    }

  if(!v4l_csc->init)
    {
      if(!gst_v4l_csc_init_v4l(v4l_csc))
        {
          GST_ERROR("could not configure device");
          return GST_FLOW_ERROR;
        }
      v4l_csc->init = TRUE;
    }

  if (TRUE == v4l_csc->CProps.update_cprops)
  {
    color_conversion = gst_v4l_csc_get_csc(v4l_csc->src_format,
                                           v4l_csc->sink_format);
    gst_v4l_csc_setup_csc(v4l_csc->fd, color_conversion, &v4l_csc->CProps);
    v4l_csc->CProps.update_cprops = FALSE;
  }
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  buf.memory = V4L2_MEMORY_USERPTR;
  buf.m.userptr = (int)buffer->data;
  buf.length = buffer->size;
  GST_LOG("output: queueing: %p length: %d", (void*) buf.m.userptr, buf.length);
  ret = ioctl(v4l_csc->fd, VIDIOC_QBUF, &buf);
  if(ret != 0)
    {
      GST_ERROR("could not queue output buffer");
      return GST_FLOW_ERROR;
    }

  GstBuffer * outbuf = NULL;


  memset(&buf, 0, sizeof(buf));

  if(v4l_csc->use_pad_alloc)
    {
      guint32 buffer_size = (guint32)(v4l_csc->src_width* v4l_csc->src_height * v4l_csc->src_bpp);
      ret = gst_pad_alloc_buffer (v4l_csc->srcpad,0,buffer_size,
          v4l_csc->src_caps, &outbuf);
      if(ret != GST_FLOW_OK)
        {
          GST_ERROR("could not allocate output buffer");
          return GST_FLOW_ERROR;
        }
      GST_LOG("got buffer: %p size: %d",outbuf->data,outbuf->size);

      buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_USERPTR;
      buf.m.userptr = (int)outbuf->data;
      buf.length = outbuf->size;
    }
  else
    {
      GstV4lM2mBuffer * v4lbuf = gst_v4l_csc_get_buffer_from_queue(v4l_csc);
      outbuf = GST_BUFFER(v4lbuf);
      if(!outbuf->caps)
        {
          outbuf->caps = gst_caps_ref(v4l_csc->src_caps);
        }
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = v4lbuf->index;
    }

  ret = ioctl(v4l_csc->fd, VIDIOC_QBUF, &buf);
  if(ret != 0)
  {
    GST_ERROR("could not queue capture buffer");
    return GST_FLOW_ERROR;
  }
  GST_LOG("queued all buffers");

  memset(&buf, 0, sizeof(buf));
  buf.type        = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  buf.memory      = V4L2_MEMORY_USERPTR;
  ret = ioctl(v4l_csc->fd, VIDIOC_DQBUF, &buf);
  if(ret != 0)
    {
      GST_ERROR("could not dequeue output buffer");
      return GST_FLOW_ERROR;
    }
  outbuf->timestamp = buffer->timestamp;
  outbuf->duration = buffer->duration;
  gst_buffer_unref(buffer);

  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  ret = ioctl(v4l_csc->fd, VIDIOC_DQBUF, &buf);
  if(ret != 0)
    {
      GST_ERROR("could not dequeue capture buffer");
      return GST_FLOW_ERROR;
    }

  GST_LOG("dequeued capture and output buffer");

  return gst_pad_push(v4l_csc->srcpad, outbuf);
}




static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_v4l_csc_debug, "gst_v4l_csc",
      0, "V4L CSC");

  return gst_element_register (plugin, "v4l_csc", GST_RANK_PRIMARY,
      GST_TYPE_V4L_CSC);
 }


GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "gst_v4l_csc",
    "V4L mem2mem based colorconversion",
    plugin_init,
    "0.0.1",
    "Proprietary",
    "ADIT",
    "http://TBD/"
)


/*lint +e826*/
