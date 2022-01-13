/**
* \file: xsub.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* DivX XSUB subtitle decoder. GStreamer entry point and common code.
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
#include <ctype.h>

#include "gstxsubdecode.h"
#include "gstxsubsink.h"
#include "gstxsubparse.h"

#include "xsub.h"

/* Utility functions */
#define TIME_PARSER_FINAL_STATE 11

static gboolean
parse_time (const guint8 const *buffer, GstClockTime *parsed_time)
{
  int state = 0;
  int current = 0;
  struct timeval tv = { 0, 0 };
  gboolean retval = FALSE;

  while (state <= TIME_PARSER_FINAL_STATE)
  {
    switch (state)
    {
      case 0:
        if (isdigit(buffer[state]))
        {
          current = buffer[state] - '0';
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 1:
        if (isdigit(buffer[state]))
        {
          current *= 10;
          current += buffer[state] - '0';
          tv.tv_sec += current * 3600;
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 2:
        if (buffer[state] == ':')
        {
          current = 0;
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 3:
        if (isdigit(buffer[state]) && (buffer[state] - '0' < 6))
        {
          current = buffer[state] - '0';
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 4:
        if (isdigit(buffer[state]))
        {
          current *= 10;
          current += buffer[state] - '0';
          tv.tv_sec += current * 60;
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 5:
        if (buffer[state] == ':')
        {
          current = 0;
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 6:
        if (isdigit(buffer[state]) && (buffer[state] - '0' < 6))
        {
          current = buffer[state] - '0';
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 7:
        if (isdigit(buffer[state]))
        {
          current *= 10;
          current += buffer[state] - '0';
          tv.tv_sec += current;
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 8:
        if (buffer[state] == '.')
        {
          current = 0;
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 9:
        if (isdigit(buffer[state]))
        {
          current = buffer[state] - '0';
          state++;
        } else {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 10:
        if (isdigit(buffer[state]))
        {
          current *= 10;
          current += buffer[state] - '0';
          state++;
        }
        else
        {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      case 11:
        if (isdigit(buffer[state]))
        {
          current *= 10;
          current += buffer[state] - '0';
          tv.tv_usec = current * 1000;
          state++;
          *parsed_time = GST_TIMEVAL_TO_TIME(tv);
          retval = TRUE;
        } else {
          state = TIME_PARSER_FINAL_STATE + 1;
        }
        break;
      default:
        state = TIME_PARSER_FINAL_STATE + 1;
        break;
    }
  }

  return retval;
}

/* Parse the duration string.
 * String has format [HH:MM:SS.DDD-hh:mm:ss.ddd] */
gboolean
gst_xsub_parse_duration (guint8 *buffer, GstClockTime *start_out, GstClockTime *duration_out)
{
  gboolean retval;
  GstClockTime start;
  GstClockTime duration;

  *start_out = GST_CLOCK_TIME_NONE;
  *duration_out = GST_CLOCK_TIME_NONE;

  retval = (buffer[0] == '[') &&
           (buffer[13] == '-') &&
           (buffer[26] == ']');

  if (retval == FALSE)
  {
    return FALSE;
  }

  retval = parse_time (&(buffer[1]), &start);
  if (retval == FALSE)
  {
    return FALSE;
  }

  retval = parse_time (&(buffer[14]), &duration);
  if (retval == FALSE)
  {
    return FALSE;
  }

  *start_out = start;
  *duration_out = GST_CLOCK_DIFF(start, duration);

  return TRUE;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
xsub_init (GstPlugin * xsubdecode)
{

  if (!(gst_element_register (xsubdecode, "xsubdecode", (int) GST_RANK_PRIMARY,
      GST_TYPE_XSUBDECODE)) ||
      !(gst_element_register (xsubdecode, "xsubsink", (int) GST_RANK_PRIMARY,
      GST_TYPE_XSUBSINK)) ||
      !(gst_element_register (xsubdecode, "xsubparse", (int) GST_RANK_PRIMARY,
      GST_TYPE_XSUBPARSE)))
    return FALSE;

  return TRUE;
}

/* Register XSUB plugin */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "xsub",
    "XSUB and XSUB+ DivX subtitle handling",
    xsub_init,
    "0.1",
    "Proprietary",
    "ADIT-JV",
    "http://TBD/"
)

