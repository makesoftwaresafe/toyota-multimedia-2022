/* GStreamer
 *
 * Copyright (C) 2009 Nokia Corporation and its subsidary(-ies)
 *               contact: <stefan.kost@nokia.com>
 * Copyright (C) 2012 Cisco Systems, Inc
 *               Authors: Kelley Rogers <kelro@cisco.com>
 *               Havard Graff <hgraff@cisco.com>
 * Copyright (C) 2013-2016 Pexip AS
 *               Stian Selnes <stian@pexip>
 *               Havard Graff <havard@pexip>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/check/gstcheck.h>
#include <gst/check/gsttestclock.h>
#include <gst/check/gstharness.h>

#include <gst/rtp/gstrtpbuffer.h>

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;
/* we also have a list of src buffers */
static GList *inbuffers = NULL;
static gint num_dropped = 0;

#define RTP_CAPS_STRING    \
    "application/x-rtp, "               \
    "media = (string)audio, "           \
    "payload = (int) 0, "               \
    "clock-rate = (int) 8000, "         \
    "encoding-name = (string)PCMU"

#define RTP_FRAME_SIZE 20

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "clock-rate = (int) [ 1, 2147483647 ]")
    );

static void
buffer_dropped (gpointer data, GstMiniObject * obj)
{
  GST_DEBUG ("dropping buffer %p", obj);
  num_dropped++;
}

static GstElement *
setup_jitterbuffer (gint num_buffers)
{
  GstElement *jitterbuffer;
  GstClock *clock;
  GstBuffer *buffer;
  GstCaps *caps;
  /* a 20 sample audio block (2,5 ms) generated with
   * gst-launch audiotestsrc wave=silence blocksize=40 num-buffers=3 !
   *    "audio/x-raw,channels=1,rate=8000" ! mulawenc ! rtppcmupay !
   *     fakesink dump=1
   */
  guint8 in[] = {
    /* first 4 bytes are rtp-header, next 4 bytes are timestamp */
    0x80, 0x80, 0x1c, 0x24, 0x46, 0xcd, 0xb7, 0x11, 0x3c, 0x3a, 0x7c, 0x5b,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };
  GstClockTime ts = G_GUINT64_CONSTANT (0);
  GstClockTime tso = gst_util_uint64_scale (RTP_FRAME_SIZE, GST_SECOND, 8000);
  /*guint latency = GST_TIME_AS_MSECONDS (num_buffers * tso); */
  gint i;

  GST_DEBUG ("setup_jitterbuffer");
  jitterbuffer = gst_check_setup_element ("rtpjitterbuffer");
  /* we need a clock here */
  clock = gst_system_clock_obtain ();
  gst_element_set_clock (jitterbuffer, clock);
  gst_object_unref (clock);
  /* setup latency */
  /* latency would be 7 for 3 buffers here, default is 200
     g_object_set (G_OBJECT (jitterbuffer), "latency", latency, NULL);
     GST_INFO_OBJECT (jitterbuffer, "set latency to %u ms", latency);
   */

  mysrcpad = gst_check_setup_src_pad (jitterbuffer, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (jitterbuffer, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  /* create n buffers */
  caps = gst_caps_from_string (RTP_CAPS_STRING);
  gst_check_setup_events (mysrcpad, jitterbuffer, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  for (i = 0; i < num_buffers; i++) {
    buffer = gst_buffer_new_and_alloc (sizeof (in));
    gst_buffer_fill (buffer, 0, in, sizeof (in));
    GST_BUFFER_DTS (buffer) = ts;
    GST_BUFFER_PTS (buffer) = ts;
    GST_BUFFER_DURATION (buffer) = tso;
    gst_mini_object_weak_ref (GST_MINI_OBJECT (buffer), buffer_dropped, NULL);
    GST_DEBUG ("created buffer: %p", buffer);

    if (!i)
      GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DISCONT);

    inbuffers = g_list_append (inbuffers, buffer);

    /* hackish way to update the rtp header */
    in[1] = 0x00;
    in[3]++;                    /* seqnumber */
    in[7] += RTP_FRAME_SIZE;    /* inc. timestamp with framesize */
    ts += tso;
  }
  num_dropped = 0;

  return jitterbuffer;
}

static GstStateChangeReturn
start_jitterbuffer (GstElement * jitterbuffer)
{
  GstStateChangeReturn ret;
  GstClockTime now;
  GstClock *clock;

  clock = gst_element_get_clock (jitterbuffer);
  now = gst_clock_get_time (clock);
  gst_object_unref (clock);

  gst_element_set_base_time (jitterbuffer, now);
  ret = gst_element_set_state (jitterbuffer, GST_STATE_PLAYING);

  return ret;
}

static void
cleanup_jitterbuffer (GstElement * jitterbuffer)
{
  GST_DEBUG ("cleanup_jitterbuffer");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  g_list_free (inbuffers);
  inbuffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (jitterbuffer);
  gst_check_teardown_sink_pad (jitterbuffer);
  gst_check_teardown_element (jitterbuffer);
}

static void
check_jitterbuffer_results (GstElement * jitterbuffer, gint num_buffers)
{
  GstBuffer *buffer;
  GList *node;
  GstClockTime ts = G_GUINT64_CONSTANT (0);
  GstClockTime tso = gst_util_uint64_scale (RTP_FRAME_SIZE, GST_SECOND, 8000);
  GstMapInfo map;
  guint16 prev_sn = 0, cur_sn;
  guint32 prev_ts = 0, cur_ts;

  /* sleep for twice the latency */
  g_usleep (400 * 1000);

  GST_INFO ("of %d buffer %d/%d received/dropped", num_buffers,
      g_list_length (buffers), num_dropped);
  /* if this fails, not all buffers have been processed */
  fail_unless_equals_int ((g_list_length (buffers) + num_dropped), num_buffers);

  /* check the buffer list */
  fail_unless_equals_int (g_list_length (buffers), num_buffers);
  for (node = buffers; node; node = g_list_next (node)) {
    fail_if ((buffer = (GstBuffer *) node->data) == NULL);
    fail_if (GST_BUFFER_PTS (buffer) != ts);
    fail_if (GST_BUFFER_DTS (buffer) != ts);
    gst_buffer_map (buffer, &map, GST_MAP_READ);
    cur_sn = ((guint16) map.data[2] << 8) | map.data[3];
    cur_ts = ((guint32) map.data[4] << 24) | ((guint32) map.data[5] << 16) |
        ((guint32) map.data[6] << 8) | map.data[7];
    gst_buffer_unmap (buffer, &map);

    if (node != buffers) {
      fail_unless (cur_sn > prev_sn);
      fail_unless (cur_ts > prev_ts);

      prev_sn = cur_sn;
      prev_ts = cur_ts;
    }
    ts += tso;
  }
}

GST_START_TEST (test_push_forward_seq)
{
  GstElement *jitterbuffer;
  const guint num_buffers = 3;
  GstBuffer *buffer;
  GList *node;

  jitterbuffer = setup_jitterbuffer (num_buffers);
  fail_unless (start_jitterbuffer (jitterbuffer)
      == GST_STATE_CHANGE_SUCCESS, "could not set to playing");

  /* push buffers: 0,1,2, */
  for (node = inbuffers; node; node = g_list_next (node)) {
    buffer = (GstBuffer *) node->data;
    fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  }

  /* check the buffer list */
  check_jitterbuffer_results (jitterbuffer, num_buffers);

  /* cleanup */
  cleanup_jitterbuffer (jitterbuffer);
}

GST_END_TEST;

GST_START_TEST (test_push_backward_seq)
{
  GstElement *jitterbuffer;
  const guint num_buffers = 4;
  GstBuffer *buffer;
  GList *node;

  jitterbuffer = setup_jitterbuffer (num_buffers);
  fail_unless (start_jitterbuffer (jitterbuffer)
      == GST_STATE_CHANGE_SUCCESS, "could not set to playing");

  /* push buffers: 0,3,2,1 */
  buffer = (GstBuffer *) inbuffers->data;
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  for (node = g_list_last (inbuffers); node != inbuffers;
      node = g_list_previous (node)) {
    buffer = (GstBuffer *) node->data;
    fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  }

  /* check the buffer list */
  check_jitterbuffer_results (jitterbuffer, num_buffers);

  /* cleanup */
  cleanup_jitterbuffer (jitterbuffer);
}

GST_END_TEST;

GST_START_TEST (test_push_unordered)
{
  GstElement *jitterbuffer;
  const guint num_buffers = 4;
  GstBuffer *buffer;

  jitterbuffer = setup_jitterbuffer (num_buffers);
  fail_unless (start_jitterbuffer (jitterbuffer)
      == GST_STATE_CHANGE_SUCCESS, "could not set to playing");

  /* push buffers; 0,2,1,3 */
  buffer = (GstBuffer *) inbuffers->data;
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  buffer = g_list_nth_data (inbuffers, 2);
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  buffer = g_list_nth_data (inbuffers, 1);
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  buffer = g_list_nth_data (inbuffers, 3);
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

  /* check the buffer list */
  check_jitterbuffer_results (jitterbuffer, num_buffers);

  /* cleanup */
  cleanup_jitterbuffer (jitterbuffer);
}

GST_END_TEST;

GST_START_TEST (test_basetime)
{
  GstElement *jitterbuffer;
  const guint num_buffers = 3;
  GstBuffer *buffer;
  GList *node;
  GstClockTime tso = gst_util_uint64_scale (RTP_FRAME_SIZE, GST_SECOND, 8000);

  jitterbuffer = setup_jitterbuffer (num_buffers);
  fail_unless (start_jitterbuffer (jitterbuffer)
      == GST_STATE_CHANGE_SUCCESS, "could not set to playing");

  /* push buffers: 2,1,0 */
  for (node = g_list_last (inbuffers); node; node = g_list_previous (node)) {
    buffer = (GstBuffer *) node->data;
    fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  }

  /* sleep for twice the latency */
  g_usleep (400 * 1000);

  /* if this fails, not all buffers have been processed */
  fail_unless_equals_int ((g_list_length (buffers) + num_dropped), num_buffers);

  buffer = (GstBuffer *) buffers->data;
  fail_unless (GST_BUFFER_DTS (buffer) != (num_buffers * tso));
  fail_unless (GST_BUFFER_PTS (buffer) != (num_buffers * tso));

  /* cleanup */
  cleanup_jitterbuffer (jitterbuffer);
}

GST_END_TEST;

static GstCaps *
request_pt_map (GstElement * jitterbuffer, guint pt)
{
  fail_unless (pt == 0);

  return gst_caps_from_string (RTP_CAPS_STRING);
}

GST_START_TEST (test_clear_pt_map)
{
  GstElement *jitterbuffer;
  const guint num_buffers = 10;
  gint i;
  GstBuffer *buffer;
  GList *node;

  jitterbuffer = setup_jitterbuffer (num_buffers);
  fail_unless (start_jitterbuffer (jitterbuffer)
      == GST_STATE_CHANGE_SUCCESS, "could not set to playing");

  g_signal_connect (jitterbuffer, "request-pt-map", (GCallback)
      request_pt_map, NULL);

  /* push buffers: 0,1,2, */
  for (node = inbuffers, i = 0; node && i < 3; node = g_list_next (node), i++) {
    buffer = (GstBuffer *) node->data;
    fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  }

  g_usleep (400 * 1000);

  g_signal_emit_by_name (jitterbuffer, "clear-pt-map", NULL);

  for (; node && i < 10; node = g_list_next (node), i++) {
    buffer = (GstBuffer *) node->data;
    fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  }

  /* check the buffer list */
  check_jitterbuffer_results (jitterbuffer, num_buffers);

  /* cleanup */
  cleanup_jitterbuffer (jitterbuffer);
}

GST_END_TEST;

#define PCMU_BUF_CLOCK_RATE 8000
#define PCMU_BUF_PT 0
#define PCMU_BUF_SSRC 0x01BADBAD
#define PCMU_BUF_MS  20
#define PCMU_BUF_DURATION (PCMU_BUF_MS * GST_MSECOND)
#define PCMU_BUF_SIZE (64000 * PCMU_BUF_MS / 1000)
#define PCMU_RTP_TS_DURATION (PCMU_BUF_CLOCK_RATE * PCMU_BUF_MS / 1000)

typedef struct
{
  GstElement *jitter_buffer;
  GstPad *test_sink_pad, *test_src_pad;
  GstClock *clock;
  GAsyncQueue *buf_queue;
  GAsyncQueue *sink_event_queue;
  GAsyncQueue *src_event_queue;
  gint lost_event_count;
  gint rtx_event_count;
} TestData;

static GstCaps *
generate_caps (void)
{
  return gst_caps_new_simple ("application/x-rtp",
      "media", G_TYPE_STRING, "audio",
      "clock-rate", G_TYPE_INT, PCMU_BUF_CLOCK_RATE,
      "encoding-name", G_TYPE_STRING, "PCMU",
      "payload", G_TYPE_INT, PCMU_BUF_PT,
      "ssrc", G_TYPE_UINT, PCMU_BUF_SSRC, NULL);
}

static GstBuffer *
generate_test_buffer_full (GstClockTime gst_ts,
    gboolean marker_bit, guint seq_num, guint32 rtp_ts)
{
  GstBuffer *buf;
  guint8 *payload;
  guint i;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  buf = gst_rtp_buffer_new_allocate (PCMU_BUF_SIZE, 0, 0);
  GST_BUFFER_DTS (buf) = gst_ts;
  GST_BUFFER_PTS (buf) = gst_ts;

  gst_rtp_buffer_map (buf, GST_MAP_READWRITE, &rtp);
  gst_rtp_buffer_set_payload_type (&rtp, PCMU_BUF_PT);
  gst_rtp_buffer_set_marker (&rtp, marker_bit);
  gst_rtp_buffer_set_seq (&rtp, seq_num);
  gst_rtp_buffer_set_timestamp (&rtp, rtp_ts);
  gst_rtp_buffer_set_ssrc (&rtp, PCMU_BUF_SSRC);

  payload = gst_rtp_buffer_get_payload (&rtp);
  for (i = 0; i < PCMU_BUF_SIZE; i++)
    payload[i] = 0xff;

  gst_rtp_buffer_unmap (&rtp);

  return buf;
}

static GstBuffer *
generate_test_buffer (guint seq_num)
{
  return generate_test_buffer_full (seq_num * PCMU_BUF_DURATION,
      TRUE, seq_num, seq_num * PCMU_RTP_TS_DURATION);
}

static GstBuffer *
generate_test_buffer_rtx (GstClockTime dts, guint seq_num)
{
  GstBuffer *buffer = generate_test_buffer_full (dts, TRUE, seq_num,
      seq_num * PCMU_RTP_TS_DURATION);
  GST_BUFFER_FLAG_SET (buffer, GST_RTP_BUFFER_FLAG_RETRANSMISSION);
  return buffer;
}

static gint
get_rtp_seq_num (GstBuffer * buf)
{
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  gint seq;
  gst_rtp_buffer_map (buf, GST_MAP_READ, &rtp);
  seq = gst_rtp_buffer_get_seq (&rtp);
  gst_rtp_buffer_unmap (&rtp);
  return seq;
}

static void
verify_lost_event (GstEvent * event, guint16 expected_seqnum,
    GstClockTime expected_timestamp, GstClockTime expected_duration)
{
  const GstStructure *s = gst_event_get_structure (event);
  const GValue *value;
  guint32 seqnum;
  GstClockTime timestamp;
  GstClockTime duration;

  fail_unless (event != NULL);
  fail_unless (gst_structure_get_uint (s, "seqnum", &seqnum));

  value = gst_structure_get_value (s, "timestamp");
  g_assert (value && G_VALUE_HOLDS_UINT64 (value));
  timestamp = g_value_get_uint64 (value);

  value = gst_structure_get_value (s, "duration");
  fail_unless (value && G_VALUE_HOLDS_UINT64 (value));
  duration = g_value_get_uint64 (value);

  fail_unless_equals_int (expected_seqnum, seqnum);
  fail_unless_equals_int (expected_timestamp, timestamp);
  fail_unless_equals_int (expected_duration, duration);

  gst_event_unref (event);
}

static void
verify_rtx_event (GstEvent * event, guint32 expected_seqnum,
    GstClockTime expected_timestamp, guint expected_delay,
    GstClockTime expected_spacing)
{
  const GstStructure *s = gst_event_get_structure (event);
  const GValue *value;
  guint32 seqnum;
  GstClockTime timestamp, spacing;
  guint delay;

  fail_unless (event);
  fail_unless (gst_structure_get_uint (s, "seqnum", &seqnum));

  value = gst_structure_get_value (s, "running-time");
  fail_unless (value && G_VALUE_HOLDS_UINT64 (value));
  timestamp = g_value_get_uint64 (value);

  value = gst_structure_get_value (s, "delay");
  fail_unless (value && G_VALUE_HOLDS_UINT (value));
  delay = g_value_get_uint (value);

  value = gst_structure_get_value (s, "packet-spacing");
  fail_unless (value && G_VALUE_HOLDS_UINT64 (value));
  spacing = g_value_get_uint64 (value);

  fail_unless_equals_int (expected_seqnum, seqnum);
  fail_unless_equals_int (expected_timestamp, timestamp);
  fail_unless_equals_int (expected_delay, delay);
  fail_unless_equals_int (expected_spacing, spacing);

  gst_event_unref (event);
}

static gboolean
verify_jb_stats (GstElement * jb, GstStructure * expected)
{
  gboolean ret;
  GstStructure *actual;
  g_object_get (jb, "stats", &actual, NULL);

  ret = gst_structure_is_subset (actual, expected);

  if (!ret) {
    gchar *e_str = gst_structure_to_string (expected);
    gchar *a_str = gst_structure_to_string (actual);
    fail_unless (ret, "%s is not a subset of %s", e_str, a_str);
    g_free (e_str);
    g_free (a_str);
  }

  gst_structure_free (expected);
  gst_structure_free (actual);

  return ret;
}

GST_START_TEST (test_only_one_lost_event_on_large_gaps)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstClockID id, test_id;
  GstBuffer *out_buf;
  GstEvent *out_event;
  gint jb_latency_ms = 200;
  gint num_lost_events = jb_latency_ms / PCMU_BUF_MS;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);
  /* Need to set max-misorder-time and max-dropout-time to 0 so the
   * jitterbuffer does not base them on packet rate calculations.
   * If it does, out gap is big enough to be considered a new stream and
   * we wait for a few consecutive packets just to be sure
   */
  g_object_set (h->element, "do-lost", TRUE, "latency", jb_latency_ms,
      "max-misorder-time", 0, "max-dropout-time", 0, NULL);

  /* push the first buffer in */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (0)));

  /* wait for the first buffer to be synced to timestamp + latency */
  gst_test_clock_wait_for_next_pending_id (testclock, &id);

  /* increase the time to timestamp + latency and release the wait */
  gst_test_clock_set_time (testclock, jb_latency_ms * GST_MSECOND);
  test_id = gst_test_clock_process_next_clock_id (testclock);
  fail_unless (id == test_id);
  gst_clock_id_unref (test_id);
  gst_clock_id_unref (id);

  /* check for the buffer coming out that was pushed in */
  out_buf = gst_harness_pull (h);
  fail_unless_equals_uint64 (0, GST_BUFFER_DTS (out_buf));
  fail_unless_equals_uint64 (0, GST_BUFFER_PTS (out_buf));
  gst_buffer_unref (out_buf);

  /* move time ahead to just before 10 seconds */
  gst_test_clock_set_time (testclock, 10 * GST_SECOND - 1);

  /* check that we have no pending waits */
  fail_unless_equals_int (0, gst_test_clock_peek_id_count (testclock));

  /* a buffer now arrives perfectly on time */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (500)));

  /* release the wait, advancing the clock to 10 sec */
  fail_unless (gst_harness_crank_single_clock_wait (h));

  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (int i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* we should now receive a packet-lost-event for buffers 1 through 489 ... */
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 1, 1 * PCMU_BUF_DURATION,
      PCMU_BUF_DURATION * 489);

  /* ... as well as 490 (since at 10 sec 490 is too late) */
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 490, 490 * PCMU_BUF_DURATION,
      PCMU_BUF_DURATION);

  /* we get as many lost events as the the number of *
   * buffers the jitterbuffer is able to wait for */
  for (int i = 1; i < num_lost_events; i++) {
    fail_unless (gst_harness_crank_single_clock_wait (h));
    out_event = gst_harness_pull_event (h);
    verify_lost_event (out_event, 490 + i, (490 + i) * PCMU_BUF_DURATION,
        PCMU_BUF_DURATION);
  }

  /* and then the buffer is released */
  out_buf = gst_harness_pull (h);
  fail_unless (GST_BUFFER_FLAG_IS_SET (out_buf, GST_BUFFER_FLAG_DISCONT));
  fail_unless_equals_int (500, get_rtp_seq_num (out_buf));
  fail_unless_equals_uint64 (10 * GST_SECOND, GST_BUFFER_DTS (out_buf));
  fail_unless_equals_uint64 (10 * GST_SECOND, GST_BUFFER_PTS (out_buf));
  gst_buffer_unref (out_buf);

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-lost", G_TYPE_UINT64, (guint64) 499, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_two_lost_one_arrives_in_time)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstClockID id;
  GstBuffer *out_buf;
  GstEvent *out_event;
  gint jb_latency_ms = 100;     /* FIXME: setting this to 10 produces a
                                 * strange result (30ms lost event),
                                 * find out why! */
  GstClockTime buffer_time;
  gint b;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);
  g_object_set (h->element, "do-lost", TRUE, "latency", jb_latency_ms, NULL);

  /* push the first buffer through */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (0)));
  fail_unless (gst_harness_crank_single_clock_wait (h));
  gst_buffer_unref (gst_harness_pull (h));

  /* push some buffers arriving in perfect time! */
  for (b = 1; b < 3; b++) {
    buffer_time = b * PCMU_BUF_DURATION;
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (b)));

    /* check for the buffer coming out that was pushed in */
    out_buf = gst_harness_pull (h);
    fail_unless_equals_uint64 (buffer_time, GST_BUFFER_DTS (out_buf));
    fail_unless_equals_uint64 (buffer_time, GST_BUFFER_PTS (out_buf));
    gst_buffer_unref (out_buf);
  }

  /* hop over 2 packets and make another one (gap of 2) */
  b = 5;
  buffer_time = b * PCMU_BUF_DURATION;
  gst_harness_push (h, generate_test_buffer (b));

  /* verify that the jitterbuffer now wait for the latest moment it can push */
  /* the first lost buffer (buffer 3) out on
   * (buffer-timestamp (60) + latency (100) = 160) */
  gst_test_clock_wait_for_next_pending_id (testclock, &id);
  fail_unless_equals_uint64 (3 * PCMU_BUF_DURATION +
      jb_latency_ms * GST_MSECOND, gst_clock_id_get_time (id));
  gst_clock_id_unref (id);

  /* let the time expire... */
  fail_unless (gst_harness_crank_single_clock_wait (h));

  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (int i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* we should now receive a packet-lost-event for buffer 3 */
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 3, 3 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);

  /* buffer 4 now arrives just in time (time is 70, buffer 4 expires at 90) */
  b = 4;
  buffer_time = b * PCMU_BUF_DURATION;
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (b)));

  /* verify that buffer 4 made it through! */
  out_buf = gst_harness_pull (h);
  fail_unless (GST_BUFFER_FLAG_IS_SET (out_buf, GST_BUFFER_FLAG_DISCONT));
  fail_unless_equals_int (4, get_rtp_seq_num (out_buf));
  gst_buffer_unref (out_buf);

  /* and see that buffer 5 now arrives in a normal fashion */
  out_buf = gst_harness_pull (h);
  fail_unless (!GST_BUFFER_FLAG_IS_SET (out_buf, GST_BUFFER_FLAG_DISCONT));
  fail_unless_equals_int (5, get_rtp_seq_num (out_buf));
  gst_buffer_unref (out_buf);

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) 5,
              "num-lost", G_TYPE_UINT64, (guint64) 1, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_late_packets_still_makes_lost_events)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstBuffer *out_buf;
  GstEvent *out_event;
  gint jb_latency_ms = 100;
  GstClockTime buffer_time;
  gint b;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);
  g_object_set (h->element, "do-lost", TRUE, "latency", jb_latency_ms, NULL);

  /* advance the clock with 10 seconds */
  gst_test_clock_set_time (testclock, 10 * GST_SECOND);

  /* push the first buffer through */
  gst_buffer_unref (gst_harness_push_and_pull (h, generate_test_buffer (0)));

  /* push some buffers arriving in perfect time! */
  for (b = 1; b < 3; b++) {
    buffer_time = b * PCMU_BUF_DURATION;
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (b)));

    /* check for the buffer coming out that was pushed in */
    out_buf = gst_harness_pull (h);
    fail_unless_equals_uint64 (buffer_time, GST_BUFFER_DTS (out_buf));
    fail_unless_equals_uint64 (buffer_time, GST_BUFFER_PTS (out_buf));
    gst_buffer_unref (out_buf);
  }

  /* hop over 2 packets and make another one (gap of 2) */
  b = 5;
  buffer_time = b * PCMU_BUF_DURATION;
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (b)));

  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (int i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* we should now receive packet-lost-events for buffer 3 and 4 */
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 3, 3 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 4, 4 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);

  /* verify that buffer 5 made it through! */
  out_buf = gst_harness_pull (h);
  fail_unless (GST_BUFFER_FLAG_IS_SET (out_buf, GST_BUFFER_FLAG_DISCONT));
  fail_unless_equals_int (5, get_rtp_seq_num (out_buf));
  gst_buffer_unref (out_buf);

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) 4,
              "num-lost", G_TYPE_UINT64, (guint64) 2, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;


GST_START_TEST (test_num_late_when_considered_lost_arrives)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  gboolean do_lost = __i__ != 0;

  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-lost", do_lost, "latency", 100, NULL);

  /* push the first buffer through */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (0)));
  /* sync on the first packet */
  gst_harness_crank_single_clock_wait (h);

  /* gap of 1 */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (2)));

  /* crank to output lost-event */
  gst_harness_crank_single_clock_wait (h);

  if (do_lost) {
    /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
    for (gint i = 0; i < 3; i++)
      gst_event_unref (gst_harness_pull_event (h));

    /* we should now receive packet-lost-events for buffer 1 */
    verify_lost_event (gst_harness_pull_event (h),
        1, 1 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);
  }

  /* pull out buffers to ensure determinism */
  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));

  /* we have one lost packet in the stats */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) 2,
              "num-lost", G_TYPE_UINT64, (guint64) 1,
              "num-late", G_TYPE_UINT64, (guint64) 0, NULL)));

  /* buffer 1 now arrives (too late) */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (1)));

  /* and this increments num-late */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) 2,
              "num-lost", G_TYPE_UINT64, (guint64) 1,
              "num-late", G_TYPE_UINT64, (guint64) 1, NULL)));

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_all_packets_are_timestamped_zero)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstBuffer *out_buf;
  GstEvent *out_event;
  gint jb_latency_ms = 100;
  gint b;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);
  g_object_set (h->element, "do-lost", TRUE, "latency", jb_latency_ms, NULL);

  /* advance the clock with 10 seconds */
  gst_test_clock_set_time (testclock, 10 * GST_SECOND);

  /* push the first buffer through */
  gst_buffer_unref (gst_harness_push_and_pull (h, generate_test_buffer (0)));

  /* push some buffers in, all timestamped 0 */
  for (b = 1; b < 3; b++) {
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h,
            generate_test_buffer_full (0 * GST_MSECOND, TRUE, b, 0)));

    /* check for the buffer coming out that was pushed in */
    out_buf = gst_harness_pull (h);
    fail_unless_equals_uint64 (0, GST_BUFFER_DTS (out_buf));
    fail_unless_equals_uint64 (0, GST_BUFFER_PTS (out_buf));
    gst_buffer_unref (out_buf);
  }

  /* hop over 2 packets and make another one (gap of 2) */
  b = 5;
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h,
          generate_test_buffer_full (0 * GST_MSECOND, TRUE, b, 0)));

  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (int i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* we should now receive packet-lost-events for buffer 3 and 4 */
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 3, 0, 0);
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 4, 0, 0);

  /* verify that buffer 5 made it through! */
  out_buf = gst_harness_pull (h);
  fail_unless (GST_BUFFER_FLAG_IS_SET (out_buf, GST_BUFFER_FLAG_DISCONT));
  fail_unless_equals_int (5, get_rtp_seq_num (out_buf));
  gst_buffer_unref (out_buf);

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) 4,
              "num-lost", G_TYPE_UINT64, (guint64) 2, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_reorder_of_non_equidistant_packets)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 5;
  GstClockID pending_id;
  GstClockTime time;
  gint seq, frame;
  gint num_init_frames = 1;
  const GstClockTime frame_dur = PCMU_BUF_DURATION;
  const guint32 frame_rtp_ts_dur = PCMU_RTP_TS_DURATION;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);
  g_object_set (h->element, "do-lost", TRUE, "latency", latency_ms, NULL);

  for (frame = 0, seq = 0; frame < num_init_frames; frame++, seq += 2) {
    /* Push a couple of packets with identical timestamp, typical for a video
     * stream where one frame generates multiple packets. */
    gst_harness_set_time (h, frame * frame_dur);
    gst_harness_push (h, generate_test_buffer_full (frame * frame_dur, FALSE,
            seq, frame * frame_rtp_ts_dur));
    gst_harness_push (h, generate_test_buffer_full (frame * frame_dur, TRUE,
            seq + 1, frame * frame_rtp_ts_dur));

    if (frame == 0)
      /* deadline for buffer 0 expires */
      gst_harness_crank_single_clock_wait (h);

    gst_buffer_unref (gst_harness_pull (h));
    gst_buffer_unref (gst_harness_pull (h));
  }

  /* Finally push the last frame reordered */
  gst_harness_set_time (h, frame * frame_dur);
  gst_harness_push (h, generate_test_buffer_full (frame * frame_dur, TRUE,
          seq + 1, frame * frame_rtp_ts_dur));

  /* Check the scheduled lost timer. The expected arrival of this packet
   * should be assumed to be the same as the last packet received since we
   * don't know wether the missing packet belonged to this or previous
   * frame. */
  gst_test_clock_wait_for_next_pending_id (testclock, &pending_id);
  time = gst_clock_id_get_time (pending_id);
  fail_unless_equals_int64 (time, frame * frame_dur + latency_ms * GST_MSECOND);
  gst_clock_id_unref (pending_id);

  /* And then missing packet arrives just in time */
  gst_harness_set_time (h, time - 1);
  gst_harness_push (h, generate_test_buffer_full (time - 1, FALSE, seq,
          frame * frame_rtp_ts_dur));

  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_loss_equidistant_spacing_with_parameter_packets)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstEvent *event;
  gint latency_ms = 5;
  gint seq, frame;
  gint num_init_frames = 10;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);
  g_object_set (h->element, "do-lost", TRUE, "latency", latency_ms, NULL);

  /* drop stream-start, caps, segment */
  for (int i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  for (frame = 0, seq = 0; frame < num_init_frames; frame++, seq++) {
    gst_harness_set_time (h, frame * PCMU_BUF_DURATION);
    gst_harness_push (h, generate_test_buffer_full (frame * PCMU_BUF_DURATION,
            TRUE, seq, frame * PCMU_RTP_TS_DURATION));

    if (frame == 0)
      /* deadline for buffer 0 expires */
      gst_harness_crank_single_clock_wait (h);

    gst_buffer_unref (gst_harness_pull (h));
  }

  /* Push three packets with same rtptime, simulating parameter packets +
   * frame. This should not disable equidistant mode as it is common for
   * certain audio codecs. */
  for (gint i = 0; i < 3; i++) {
    gst_harness_set_time (h, frame * PCMU_BUF_DURATION);
    gst_harness_push (h, generate_test_buffer_full (frame * PCMU_BUF_DURATION,
            i == 2, seq++, frame * PCMU_RTP_TS_DURATION));
    gst_buffer_unref (gst_harness_pull (h));
  }
  frame++;

  /* Finally push the last packet introducing a gap */
  gst_harness_set_time (h, frame * PCMU_BUF_DURATION);
  gst_harness_push (h, generate_test_buffer_full (frame * PCMU_BUF_DURATION,
          TRUE, seq + 1, frame * PCMU_RTP_TS_DURATION));

  /* Check that the lost event has been generated assuming equidistant
   * spacing. */
  event = gst_harness_pull_event (h);
  verify_lost_event (event, seq,
      frame * PCMU_BUF_DURATION - PCMU_BUF_DURATION / 2, PCMU_BUF_DURATION / 2);

  gst_buffer_unref (gst_harness_pull (h));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;


static void
gst_test_clock_set_time_and_process (GstTestClock * testclock,
    GstClockTime time)
{
  GstClockID id, tid;
  gst_test_clock_wait_for_next_pending_id (testclock, &id);
  gst_test_clock_set_time (testclock, time);
  tid = gst_test_clock_process_next_clock_id (testclock);
  g_assert (tid == id);
  gst_clock_id_unref (tid);
  gst_clock_id_unref (id);
}

GST_START_TEST (test_rtx_expected_next)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstBuffer *out_buf;
  GstEvent *out_event;
  gint jb_latency_ms = 200;
  const GstClockTime rtx_retry_timeout = 40 * GST_MSECOND;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);

  g_object_set (h->element, "do-lost", TRUE, NULL);
  g_object_set (h->element, "do-retransmission", TRUE, NULL);
  g_object_set (h->element, "latency", jb_latency_ms, NULL);
  g_object_set (h->element, "rtx-retry-period", 120, NULL);

  /* push the first buffer in */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (0)));

  gst_harness_set_time (h, 20 * GST_MSECOND);
  gst_harness_wait_for_clock_id_waits (h, 1, 60);

  /* put second buffer, the jitterbuffer should now know that the packet
   * spacing is 20ms and should ask for retransmission of seqnum 2 in
   * 20ms+10ms because 2*jitter==0 and 0.5*packet_spacing==10ms */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (1)));

  gst_test_clock_set_time_and_process (testclock, 50 * GST_MSECOND);

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));
  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (int i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  out_event = gst_harness_pull_upstream_event (h);
  verify_rtx_event (out_event, 2, rtx_retry_timeout, 10, PCMU_BUF_DURATION);

  /* now we wait for the next timeout, all following timeouts 40ms in the
   * future because this is rtx-retry-timeout */
  gst_test_clock_set_time_and_process (testclock, 90 * GST_MSECOND);
  out_event = gst_harness_pull_upstream_event (h);
  verify_rtx_event (out_event, 2, rtx_retry_timeout, 50, PCMU_BUF_DURATION);

  gst_test_clock_set_time_and_process (testclock, 130 * GST_MSECOND);
  out_event = gst_harness_pull_upstream_event (h);
  verify_rtx_event (out_event, 2, rtx_retry_timeout, 90, PCMU_BUF_DURATION);

  gst_test_clock_set_time_and_process (testclock, 200 * GST_MSECOND);
  out_buf = gst_harness_pull (h);
  fail_unless_equals_int (0, get_rtp_seq_num (out_buf));
  gst_buffer_unref (out_buf);

  gst_test_clock_set_time_and_process (testclock, 240 * GST_MSECOND);
  /* we should now receive a packet-lost-event for buffer 2 */
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 2, 40 * GST_MSECOND, PCMU_BUF_DURATION);

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_two_missing)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 200;
  gint rtx_delay_ms;
  GstClockTime last_rtx_request, now;

  testclock = gst_harness_get_testclock (h);
  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-retransmission", TRUE, "latency", latency_ms,
      NULL);

  for (gint i = 0; i <= latency_ms / PCMU_BUF_MS; i++) {
    gst_test_clock_set_time (testclock, i * PCMU_BUF_DURATION);
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
    gst_harness_wait_for_clock_id_waits (h, 1, 60);
  }

  gst_harness_crank_single_clock_wait (h);
  fail_unless_equals_int64 (latency_ms * GST_MSECOND,
      gst_clock_get_time (GST_CLOCK (testclock)));

  for (gint i = 0; i <= latency_ms / PCMU_BUF_MS; i++)
    gst_buffer_unref (gst_harness_pull (h));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));

  /*
     The expected sequence of buffers is this:
     ____   ____   ____   ____
     ... | 10 | | 11 | | 12 | | 13 |
     ????????????   ????????????   ????????????   ????????????
     200ms  220ms  240ms  260ms

     But instead we get this:
     ____    _ _    _ _   ____
     ... | 10 |  |   |  |   | | 13 |
     ????????????    - -    - -   ????????????
     200ms                260ms

     Now it is important to note that the next thing that happens is that
     the RTX timeout for packet 11 will happen at time 230ms, so we crank
     the timer thread to advance the time to this: */
  gst_harness_crank_single_clock_wait (h);
  rtx_delay_ms = PCMU_BUF_MS / 2;
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      11, 11 * PCMU_BUF_DURATION, rtx_delay_ms, PCMU_BUF_DURATION);
  last_rtx_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (last_rtx_request,
      11 * PCMU_BUF_DURATION + rtx_delay_ms * GST_MSECOND);
  gst_harness_wait_for_clock_id_waits (h, 1, 60);

  /* The next scheduled RTX for packet 11 is now at 230 + 40 = 270ms,
     so the next thing that happens is that buffer 13 arrives in perfect time: */
  now = 13 * PCMU_BUF_DURATION;
  gst_harness_set_time (h, now);
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h,
          generate_test_buffer_full (now, TRUE, 13,
              13 * PCMU_RTP_TS_DURATION)));

  /*

     This will estimate the dts on the two missing packets to:
     ____   ____
     ... | 11 | | 12 | ...
     ????????????   ????????????
     220ms  240ms

     And given their regular interspacing of 20ms, it will schedule two RTX
     timers for them like so:

     ____   ____
     ... | 11 | | 12 | ...
     ????????????   ????????????
     230ms  250ms

     There are however two problems, packet 11 we have already sent one RTX for
     and its timeout is currently at 270ms, so we should not tamper with that,
     and as for packet 12, 250ms has already expired, so we now expect to see
     an rtx-event being sent for packet 12 immediately: */
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      12, 12 * PCMU_BUF_DURATION, rtx_delay_ms, PCMU_BUF_DURATION);

  /* and another crank will see the second RTX event being sent for packet 11 */
  gst_harness_crank_single_clock_wait (h);
  rtx_delay_ms += 40;
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      11, 11 * PCMU_BUF_DURATION, rtx_delay_ms, PCMU_BUF_DURATION);
  last_rtx_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (last_rtx_request,
      11 * PCMU_BUF_DURATION + rtx_delay_ms * GST_MSECOND);

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (text_rtx_two_missing_early)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 30;
  GstClockTime last_rtx_request, now;

  testclock = gst_harness_get_testclock (h);
  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-retransmission", TRUE, "latency", latency_ms,
      NULL);

  for (gint i = 0; i <= latency_ms / PCMU_BUF_MS; i++) {
    gst_test_clock_set_time (testclock, i * PCMU_BUF_DURATION);
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
    gst_harness_wait_for_clock_id_waits (h, 1, 60);
  }

  gst_harness_crank_single_clock_wait (h);
  fail_unless_equals_int64 (latency_ms * GST_MSECOND,
      gst_clock_get_time (GST_CLOCK (testclock)));

  for (gint i = 0; i <= latency_ms / PCMU_BUF_MS; i++)
    gst_buffer_unref (gst_harness_pull (h));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));

  /*
     The expected sequence of buffers is this:
     ___   ___   ___   ___   ___
     | 0 | | 1 | | 2 | | 3 | | 4 |
     ?????????   ?????????   ?????????   ?????????   ?????????
     0ms  20ms  40ms  60ms  80ms

     But instead we get this:
     ___   ___   _ _   _ _   ___
     | 0 | | 1 | |   | |   | | 4 |
     ?????????   ?????????   ??? ???   ??? ???   ?????????
     0ms  20ms              41ms

   */

  now = 41 * GST_MSECOND;
  gst_harness_set_time (h, now);
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h,
          generate_test_buffer_full (now, TRUE, 4, 4 * PCMU_RTP_TS_DURATION)));

  /*

     With the now calculated packet-spacing of (41-20) / 3 = 7,
     giving us these expected times:
     ___   ___   ___   ___   ___
     | 0 | | 1 | | 2 | | 3 | | 4 |
     ?????????   ?????????   ?????????   ?????????   ?????????
     0ms  20ms  27ms  34ms  41ms

     For RTX, the inital RTX-timeouts for the missing buffers are
     the expected arrival time + half the packet-spacing time, like this:
     ___   ___
     | 2 | | 3 |
     ?????????   ?????????
     50ms  70ms

     But since we have re-calculated the estimated arrival-time
     of these buffers, we have to adjust the RTX timeout as well,
     and we use the original delay (packet-spacing / 2) = 10ms,
     and add it on:
     ___   ___
     | 2 | | 3 |
     ?????????   ?????????
     37ms  44ms

     Also note that the first RTX request is now scheduled for a
     time that is prior to NOW (37ms < 41ms), so it will be sent straight
     away without us needing to "crank" the timer-thread

   */

  /* The RTX request for packet 2 has timestamp 27ms and delay 10ms */
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      2, 27 * GST_MSECOND, 10, PCMU_BUF_DURATION);
  /* and is sent immediately after packet 4 arrives (41ms) */
  last_rtx_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (last_rtx_request, now);

  /* crank the timer thread */
  gst_harness_crank_single_clock_wait (h);

  /* The RTX request for packet 3 has timestamp 34ms and delay 10ms */
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      3, 34 * GST_MSECOND, 10, PCMU_BUF_DURATION);
  /* and is sent at 44ms */
  last_rtx_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (last_rtx_request, 44 * GST_MSECOND);

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_packet_delay)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstBuffer *in_buf, *out_buf;
  GstEvent *out_event;
  gint jb_latency_ms = 200;
  gint i;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);

  g_object_set (h->element, "do-lost", TRUE, NULL);
  g_object_set (h->element, "do-retransmission", TRUE, NULL);
  g_object_set (h->element, "latency", jb_latency_ms, NULL);
  g_object_set (h->element, "rtx-retry-period", 120, NULL);

  /* push the first buffer in */
  in_buf = generate_test_buffer (0);
  GST_BUFFER_FLAG_SET (in_buf, GST_BUFFER_FLAG_DISCONT);
  gst_harness_push (h, in_buf);

  /* put second buffer, the jitterbuffer should now know that the packet
   * spacing is 20ms and should ask for retransmission of seqnum 2 in
   * 20ms+10ms because 2*jitter==0 and 0.5*packet_spacing==10ms */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (1)));

  /* push buffer 8, 2 -> 7 are missing now. note that the rtp time is the same
   * as packet 1 because it was part of a fragmented payload. This means that
   * the estimate for 2 could be refined now to 20ms. also packet 2, 3 and 4
   * are exceeding the max allowed reorder distance and should request a
   * retransmission right away */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer_full (20 * GST_MSECOND, TRUE, 8,
              8 * PCMU_RTP_TS_DURATION)));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));
  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (int i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* we should now receive retransmission requests for 2 -> 5 */
  out_event = gst_harness_pull_upstream_event (h);
  verify_rtx_event (out_event, 2, 20 * GST_MSECOND, 17, PCMU_BUF_DURATION);

  for (i = 3; i < 5; i++) {
    GST_DEBUG ("popping %d", i);
    out_event = gst_harness_pull_upstream_event (h);
    verify_rtx_event (out_event, i, 20 * GST_MSECOND, 17, PCMU_BUF_DURATION);
  }
  fail_unless_equals_int (0, gst_harness_upstream_events_in_queue (h));

  /* push 9, this should immediately request retransmission of 5 */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer_full (20 * GST_MSECOND, TRUE, 9,
              9 * PCMU_RTP_TS_DURATION)));

  /* we should now receive retransmission requests for 5 */
  out_event = gst_harness_pull_upstream_event (h);
  verify_rtx_event (out_event, 5, 20 * GST_MSECOND, 17, PCMU_BUF_DURATION);

  /* wait for timeout for rtx 6 -> 7 */
  gst_test_clock_set_time_and_process (testclock, 60 * GST_MSECOND);

  for (i = 6; i < 8; i++) {
    GST_DEBUG ("popping %d", i);
    out_event = gst_harness_pull_upstream_event (h);
    verify_rtx_event (out_event, i, 20 * GST_MSECOND, 17, PCMU_BUF_DURATION);
  }

  /* churn through 7 sync_times until the new buffer gets pushed out */
  for (i = 0; i < 7; i++)
    gst_harness_crank_single_clock_wait (h);

  /* verify that buffer 0 and 1 made it through! */
  for (i = 0; i < 2; i++) {
    out_buf = gst_harness_pull (h);
    if (i == 0)
      fail_unless (GST_BUFFER_FLAG_IS_SET (out_buf, GST_BUFFER_FLAG_DISCONT));
    fail_unless_equals_int (i, get_rtp_seq_num (out_buf));
    gst_buffer_unref (out_buf);
  }

  for (i = 2; i < 8; i++) {
    GST_DEBUG ("popping lost event %d", i);
    out_event = gst_harness_pull_event (h);
    verify_lost_event (out_event, i, 20 * GST_MSECOND, 0);
  }

  /* verify that buffer 8 made it through! */
  for (i = 8; i < 10; i++) {
    GST_DEBUG ("popping buffer %d", i);
    out_buf = gst_harness_pull (h);
    if (i == 8)
      g_assert (GST_BUFFER_FLAG_IS_SET (out_buf, GST_BUFFER_FLAG_DISCONT));
    fail_unless_equals_int (i, get_rtp_seq_num (out_buf));
    gst_buffer_unref (out_buf);
  }

  GST_DEBUG ("popping lost event 10");
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 10, 40 * GST_MSECOND, PCMU_BUF_DURATION);

  fail_unless_equals_int (0, gst_harness_events_in_queue (h));
  fail_unless_equals_int (15, gst_harness_upstream_events_in_queue (h));

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-lost", G_TYPE_UINT64, (guint64) 7,
              "rtx-count", G_TYPE_UINT64, (guint64) 21,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) 0, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_buffer_arrives_just_in_time)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 5 * PCMU_BUF_MS;
  gint num_init_buffers = latency_ms / PCMU_BUF_MS + 1;
  GstBuffer *buffer;
  GstClockTime now, last_rtx_request;

  testclock = gst_harness_get_testclock (h);
  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-retransmission", TRUE, "latency", latency_ms,
      "rtx-max-retries", 1, NULL);

  /* Push/pull buffers and advance time past buffer 0's timeout (in order to
   * simplify the test) */
  for (gint i = 0; i < num_init_buffers; i++) {
    gst_test_clock_set_time (testclock, i * PCMU_BUF_DURATION);
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
    gst_harness_wait_for_clock_id_waits (h, 1, 60);
  }

  gst_harness_crank_single_clock_wait (h);
  fail_unless_equals_int64 (latency_ms * GST_MSECOND,
      gst_clock_get_time (GST_CLOCK (testclock)));

  for (gint i = 0; i < num_init_buffers; i++)
    gst_buffer_unref (gst_harness_pull (h));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));

  /* Crank clock to send retransmission events requesting seqnum 6 which has
   * not arrived yet. */
  gst_harness_crank_single_clock_wait (h);
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      6, 6 * PCMU_BUF_DURATION, 10, PCMU_BUF_DURATION);

  last_rtx_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (last_rtx_request, 130 * GST_MSECOND);

  /* seqnum 6 arrives just before it times out and is considered lost */
  now = 200 * GST_MSECOND;
  gst_test_clock_set_time (testclock, now);
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_rtx (now, 6)));
  buffer = gst_harness_pull (h);
  fail_unless_equals_int (6, get_rtp_seq_num (buffer));
  gst_buffer_unref (buffer);

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers + 1,
              "num-lost", G_TYPE_UINT64, (guint64) 0,
              "rtx-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-per-packet", G_TYPE_DOUBLE, 1.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) (now - last_rtx_request),
              NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_buffer_arrives_too_late)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 5 * PCMU_BUF_MS;
  gint num_init_buffers = latency_ms / PCMU_BUF_MS + 1;
  GstClockTime now, last_rtx_request;

  testclock = gst_harness_get_testclock (h);
  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-retransmission", TRUE, "latency", latency_ms,
      "do-lost", TRUE, "rtx-max-retries", 1, NULL);

  /* Push/pull buffers and advance time past buffer 0's timeout (in order to
   * simplify the test) */
  for (gint i = 0; i < num_init_buffers; i++) {
    gst_test_clock_set_time (testclock, i * PCMU_BUF_DURATION);
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
    gst_harness_wait_for_clock_id_waits (h, 1, 60);
  }

  gst_harness_crank_single_clock_wait (h);
  fail_unless_equals_int64 (latency_ms * GST_MSECOND,
      gst_clock_get_time (GST_CLOCK (testclock)));

  for (gint i = 0; i < num_init_buffers; i++)
    gst_buffer_unref (gst_harness_pull (h));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));
  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (gint i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* Crank clock to send retransmission events requesting seqnum 6 which has
   * not arrived yet. */
  gst_harness_crank_single_clock_wait (h);
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      6, 6 * PCMU_BUF_DURATION, 10, PCMU_BUF_DURATION);

  last_rtx_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (last_rtx_request, 130 * GST_MSECOND);

  /* seqnum 6 is considered lost */
  gst_harness_crank_single_clock_wait (h);
  verify_lost_event (gst_harness_pull_event (h), 6,
      6 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);

  /* seqnum 6 arrives too late */
  now = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_rtx (now, 6)));

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers,
              "num-lost", G_TYPE_UINT64, (guint64) 1,
              "num-late", G_TYPE_UINT64, (guint64) 1,
              "num-duplicates", G_TYPE_UINT64, (guint64) 0,
              "rtx-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 0,
              "rtx-per-packet", G_TYPE_DOUBLE, 1.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) (now - last_rtx_request),
              NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_original_buffer_does_not_update_rtx_stats)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 5 * PCMU_BUF_MS;
  gint num_init_buffers = latency_ms / PCMU_BUF_MS + 1;
  GstBuffer *buffer;
  GstClockTime now, last_rtx_request;

  testclock = gst_harness_get_testclock (h);
  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-retransmission", TRUE, "latency", latency_ms,
      "rtx-max-retries", 1, NULL);

  /* Push/pull buffers and advance time past buffer 0's timeout (in order to
   * simplify the test) */
  for (gint i = 0; i < num_init_buffers; i++) {
    gst_test_clock_set_time (testclock, i * PCMU_BUF_DURATION);
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
    gst_harness_wait_for_clock_id_waits (h, 1, 60);
  }

  gst_harness_crank_single_clock_wait (h);
  fail_unless_equals_int64 (latency_ms * GST_MSECOND,
      gst_clock_get_time (GST_CLOCK (testclock)));

  for (gint i = 0; i < num_init_buffers; i++)
    gst_buffer_unref (gst_harness_pull (h));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));

  /* Crank clock to send retransmission events requesting seqnum 6 which has
   * not arrived yet. */
  gst_harness_crank_single_clock_wait (h);
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      6, 6 * PCMU_BUF_DURATION, 10, PCMU_BUF_DURATION);

  last_rtx_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (last_rtx_request, 130 * GST_MSECOND);

  /* ORIGINAL seqnum 6 arrives just before it times out and is considered
   * lost. */
  now = 200 * GST_MSECOND;
  gst_test_clock_set_time (testclock, now);
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_full (now, TRUE, 6, 6 * PCMU_RTP_TS_DURATION)));
  buffer = gst_harness_pull (h);
  fail_unless_equals_int (6, get_rtp_seq_num (buffer));
  gst_buffer_unref (buffer);

  /* The original buffer does not count in the RTX stats. */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers + 1,
              "num-lost", G_TYPE_UINT64, (guint64) 0,
              "num-late", G_TYPE_UINT64, (guint64) 0,
              "num-duplicates", G_TYPE_UINT64, (guint64) 0,
              "rtx-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 0,
              "rtx-per-packet", G_TYPE_DOUBLE, 0.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) 0, NULL)));

  /* Now the retransmitted packet arrives and stats should be updated. Note
   * that the buffer arrives in time and should not be considered late, but
   * a duplicate. */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_rtx (now, 6)));
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers + 1,
              "num-lost", G_TYPE_UINT64, (guint64) 0,
              "num-late", G_TYPE_UINT64, (guint64) 0,
              "num-duplicates", G_TYPE_UINT64, (guint64) 1,
              "rtx-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 0,
              "rtx-per-packet", G_TYPE_DOUBLE, 1.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) (now - last_rtx_request),
              NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_duplicate_packet_updates_rtx_stats)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 5 * PCMU_BUF_MS;
  gint num_init_buffers = latency_ms / PCMU_BUF_MS + 1;
  GstClockTime now, rtx_request_6, rtx_request_7;
  gint rtx_delay_ms;

  testclock = gst_harness_get_testclock (h);
  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-retransmission", TRUE, "latency", latency_ms,
      NULL);

  /* Push/pull buffers and advance time past buffer 0's timeout (in order to
   * simplify the test) */
  for (gint i = 0; i < num_init_buffers; i++) {
    gst_test_clock_set_time (testclock, i * PCMU_BUF_DURATION);
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
    gst_harness_wait_for_clock_id_waits (h, 1, 60);
  }

  gst_harness_crank_single_clock_wait (h);
  fail_unless_equals_int64 (latency_ms * GST_MSECOND,
      gst_clock_get_time (GST_CLOCK (testclock)));

  for (gint i = 0; i < num_init_buffers; i++)
    gst_buffer_unref (gst_harness_pull (h));

  /* Drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));

  /* Push packet 8 so that 6 and 7 is missing */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (8)));

  /* Wait for NACKs on 6 and 7 */
  gst_harness_crank_single_clock_wait (h);
  rtx_delay_ms = 10;
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      6, 6 * PCMU_BUF_DURATION, rtx_delay_ms, PCMU_BUF_DURATION);
  rtx_request_6 = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (rtx_request_6,
      6 * PCMU_BUF_DURATION + rtx_delay_ms * GST_MSECOND);

  gst_harness_crank_single_clock_wait (h);
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      7, 7 * PCMU_BUF_DURATION, rtx_delay_ms, PCMU_BUF_DURATION);
  rtx_request_7 = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (rtx_request_7,
      7 * PCMU_BUF_DURATION + rtx_delay_ms * GST_MSECOND);

  /* Original packet 7 arrives */
  now = 150 * GST_MSECOND;
  gst_test_clock_set_time (testclock, now);
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_full (now, TRUE, 7, 7 * PCMU_RTP_TS_DURATION)));

  /* We're still waiting for packet 6, so 7 should not be pushed */
  gst_harness_wait_for_clock_id_waits (h, 1, 60);
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 0);

  /* The original buffer does not count in the RTX stats. */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-lost", G_TYPE_UINT64, (guint64) 0,
              "num-late", G_TYPE_UINT64, (guint64) 0,
              "num-duplicates", G_TYPE_UINT64, (guint64) 0,
              "rtx-count", G_TYPE_UINT64, (guint64) 2,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 0,
              "rtx-per-packet", G_TYPE_DOUBLE, 0.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) 0, NULL)));

  /* Push RTX packet 7. Should be dropped as duplicate but update RTX stats. */
  now = 160 * GST_MSECOND;
  gst_test_clock_set_time (testclock, now);
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_rtx (now, 7)));
  gst_harness_wait_for_clock_id_waits (h, 1, 60);
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 0);

  /* Check RTX stats with updated num-duplicates and rtx-rtt fields */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers,
              "num-lost", G_TYPE_UINT64, (guint64) 0,
              "num-late", G_TYPE_UINT64, (guint64) 0,
              "num-duplicates", G_TYPE_UINT64, (guint64) 1,
              "rtx-count", G_TYPE_UINT64, (guint64) 2,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 0,
              "rtx-per-packet", G_TYPE_DOUBLE, 1.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) (now - rtx_request_7),
              NULL)));

  /* RTX packet 6 arrives, both 6, 7 and 8 is ready to be pulled */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_rtx (now, 6)));

  for (gint i = 6; i <= 8; i++) {
    GstBuffer *buf = gst_harness_pull (h);
    fail_unless_equals_int (i, get_rtp_seq_num (buf));
    gst_buffer_unref (buf);
  }

  /* RTX stats is updated with success count increased. */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers + 3,
              "num-lost", G_TYPE_UINT64, (guint64) 0,
              "num-late", G_TYPE_UINT64, (guint64) 0,
              "num-duplicates", G_TYPE_UINT64, (guint64) 1,
              "rtx-count", G_TYPE_UINT64, (guint64) 2,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-per-packet", G_TYPE_DOUBLE, 1.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64)
              /* Use the rtx-rtt formula. Can be subject to change though. */
              ((now - rtx_request_6) + 47 * (now - rtx_request_7)) / 48,
              NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_buffer_arrives_after_lost_updates_rtx_stats)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 5 * PCMU_BUF_MS;
  gint num_init_buffers = latency_ms / PCMU_BUF_MS + 1;
  GstClockTime now, last_rtx_request;

  testclock = gst_harness_get_testclock (h);
  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-retransmission", TRUE, "latency", latency_ms,
      "do-lost", TRUE, "rtx-max-retries", 1, NULL);

  /* Push/pull buffers and advance time past buffer 0's timeout (in order to
   * simplify the test) */
  for (gint i = 0; i < num_init_buffers; i++) {
    gst_test_clock_set_time (testclock, i * PCMU_BUF_DURATION);
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
    gst_harness_wait_for_clock_id_waits (h, 1, 60);
  }

  gst_harness_crank_single_clock_wait (h);
  fail_unless_equals_int64 (latency_ms * GST_MSECOND,
      gst_clock_get_time (GST_CLOCK (testclock)));

  for (gint i = 0; i < num_init_buffers; i++)
    gst_buffer_unref (gst_harness_pull (h));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));
  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (gint i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* Crank clock to send retransmission events requesting seqnum 6 which has
   * not arrived yet. */
  gst_harness_crank_single_clock_wait (h);
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      6, 6 * PCMU_BUF_DURATION, 10, PCMU_BUF_DURATION);

  last_rtx_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (last_rtx_request, 130 * GST_MSECOND);

  /* seqnum 6 is considered lost */
  gst_harness_crank_single_clock_wait (h);
  verify_lost_event (gst_harness_pull_event (h), 6,
      6 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);

  /* seqnum 6 arrives too late */
  now = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_rtx (now, 6)));

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers,
              "num-lost", G_TYPE_UINT64, (guint64) 1,
              "num-late", G_TYPE_UINT64, (guint64) 1,
              "num-duplicates", G_TYPE_UINT64, (guint64) 0,
              "rtx-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 0,
              "rtx-per-packet", G_TYPE_DOUBLE, 1.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) (now - last_rtx_request),
              NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_rtt_larger_than_retry_timeout)
{
  /* When RTT is larger than retry period we will send two or more requests
   * before receiving any retransmission packets */
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  gint latency_ms = 100;
  gint num_init_buffers = latency_ms / PCMU_BUF_MS + 1;
  gint rtx_retry_timeout_ms = 20;
  gint rtx_delay_ms = 10;
  gint rtt = rtx_retry_timeout_ms * GST_MSECOND + 1;
  GstClockTime now, first_request, second_request;

  testclock = gst_harness_get_testclock (h);
  gst_harness_set_src_caps (h, generate_caps ());
  g_object_set (h->element, "do-retransmission", TRUE, "latency", latency_ms,
      "rtx-retry-timeout", rtx_retry_timeout_ms, NULL);

  /* Push/pull buffers and advance time past buffer 0's timeout (in order to
   * simplify the test) */
  for (gint i = 0; i < num_init_buffers; i++) {
    gst_test_clock_set_time (testclock, i * PCMU_BUF_DURATION);
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
    gst_harness_wait_for_clock_id_waits (h, 1, 60);
  }

  gst_harness_crank_single_clock_wait (h);
  fail_unless_equals_int64 (latency_ms * GST_MSECOND,
      gst_clock_get_time (GST_CLOCK (testclock)));

  for (gint i = 0; i < num_init_buffers; i++)
    gst_buffer_unref (gst_harness_pull (h));

  /* Drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));

  /* Wait for first NACK on 6 */
  gst_harness_crank_single_clock_wait (h);
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      6, 6 * PCMU_BUF_DURATION, rtx_delay_ms, PCMU_BUF_DURATION);
  first_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (first_request,
      6 * PCMU_BUF_DURATION + rtx_delay_ms * GST_MSECOND);

  /* Packet 7 arrives in time (so that we avoid its EXPECTED timers to
   * interfer with our test) */
  gst_test_clock_set_time (testclock, 7 * PCMU_BUF_DURATION);
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (7)));

  /* Simulating RTT > rtx-retry-timeout, we send a new NACK before receiving
   * the RTX packet. Wait for second NACK on 6 */
  gst_harness_crank_single_clock_wait (h);
  rtx_delay_ms += rtx_retry_timeout_ms;
  verify_rtx_event (gst_harness_pull_upstream_event (h),
      6, 6 * PCMU_BUF_DURATION, rtx_delay_ms, PCMU_BUF_DURATION);
  second_request = gst_clock_get_time (GST_CLOCK (testclock));
  fail_unless_equals_int64 (second_request,
      6 * PCMU_BUF_DURATION + rtx_delay_ms * GST_MSECOND);

  /* The first retransmitted packet arrives */
  now = first_request + rtt;
  gst_test_clock_set_time (testclock, now);
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_rtx (now, 6)));

  /* Pull packet 6 and 7 */
  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));

  /* Stats should be updated. Note that RTT is not updated since we cannot be
   * sure whether the RTX packet is in response to the first or second NACK. */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers + 2,
              "num-lost", G_TYPE_UINT64, (guint64) 0,
              "num-late", G_TYPE_UINT64, (guint64) 0,
              "num-duplicates", G_TYPE_UINT64, (guint64) 0,
              "rtx-count", G_TYPE_UINT64, (guint64) 2,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-per-packet", G_TYPE_DOUBLE, 2.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) 0, NULL)));

  /* Packet 8 arrives in time */
  gst_test_clock_set_time (testclock, 8 * PCMU_BUF_DURATION);
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (8)));
  gst_buffer_unref (gst_harness_pull (h));

  /* Now the second retransmitted packet arrives */
  now = second_request + rtt;
  gst_test_clock_set_time (testclock, now);
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          generate_test_buffer_rtx (now, 6)));

  /* The stats is updated with the correct RTT. */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) num_init_buffers + 3,
              "num-lost", G_TYPE_UINT64, (guint64) 0,
              "num-late", G_TYPE_UINT64, (guint64) 0,
              "num-duplicates", G_TYPE_UINT64, (guint64) 1,
              "rtx-count", G_TYPE_UINT64, (guint64) 2,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 1,
              "rtx-per-packet", G_TYPE_DOUBLE, 2.0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) rtt, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtx_no_request_if_time_past_retry_period)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  const gint latency_ms = 200;
  const gint retry_period_ms = 120;
  GstTestClock *testclock;
  GstClockID pending_id, processed_id;
  GstClockTime time;
  GstEvent *event;
  gint i;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);

  g_object_set (h->element, "do-lost", TRUE, NULL);
  g_object_set (h->element, "do-retransmission", TRUE, NULL);
  g_object_set (h->element, "latency", latency_ms, NULL);
  g_object_set (h->element, "rtx-retry-period", retry_period_ms, NULL);

  /* push the first couple of buffers */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (0)));

  gst_harness_set_time (h, 1 * PCMU_BUF_DURATION);
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (1)));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));
  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* Wait for the first EXPECTED timer to be scheduled */
  gst_test_clock_wait_for_next_pending_id (testclock, &pending_id);
  time = gst_clock_id_get_time (pending_id);
  fail_unless_equals_int64 (time, 2 * PCMU_BUF_DURATION + 10 * GST_MSECOND);

  /* Let the first EXPECTED timer time out and be sent. However, set the 'now'
   * time to be past the retry-period simulating that the jitterbuffer has too
   * much to do and is not able to process all timers in real-time. In this
   * case the jitterbuffer should not schedule a new EXPECTED timer as that
   * would just make matters worse (more unnecessary processing of a request
   * that is already too late to be valuable). In practice this typically
   * happens for high loss networks with low RTT. */
  gst_test_clock_set_time (testclock,
      2 * PCMU_BUF_DURATION + retry_period_ms * GST_MSECOND + 1);
  processed_id = gst_test_clock_process_next_clock_id (testclock);
  fail_unless (pending_id == processed_id);
  gst_clock_id_unref (pending_id);
  gst_clock_id_unref (processed_id);

  /* Verify the event. It could be argued that this request is already too
   * late and unnecessary. However, in order to keep things simple (for now)
   * we just keep the already scehduled EXPECTED timer, but refrain from
   * scheduled another EXPECTED timer */
  event = gst_harness_pull_upstream_event (h);
  verify_rtx_event (event, 2, 2 * PCMU_BUF_DURATION, 10, PCMU_BUF_DURATION);

  /* "crank" to reach the DEADLINE for packet 0 */
  gst_harness_crank_single_clock_wait (h);
  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));

  fail_unless_equals_int (0, gst_harness_upstream_events_in_queue (h));
  fail_unless_equals_int (0, gst_harness_events_in_queue (h));

  /* "crank" to time out the LOST event */
  gst_harness_crank_single_clock_wait (h);
  event = gst_harness_pull_event (h);
  verify_lost_event (event, 2, 2 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_gap_exceeds_latency)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  const gint jb_latency_ms = 200;

  gint i;
  GstEvent *out_event;
  GstBuffer *out_buf;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);

  g_object_set (h->element, "do-lost", TRUE, NULL);
  g_object_set (h->element, "do-retransmission", TRUE, NULL);
  g_object_set (h->element, "latency", jb_latency_ms, NULL);
  g_object_set (h->element, "rtx-retry-period", 120, NULL);

  /* push the first buffer in */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (0)));

  gst_harness_set_time (h, 1 * PCMU_BUF_DURATION);
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (1)));

  /* drop reconfigure event */
  gst_event_unref (gst_harness_pull_upstream_event (h));
  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /*  Allow seqnum 2 to be declared lost */
  for (i = 0; i < 3; i++) {
    gst_harness_crank_single_clock_wait (h);
    out_event = gst_harness_pull_upstream_event (h);
    verify_rtx_event (out_event,
        2, 2 * PCMU_BUF_DURATION, 10 + 40 * i, PCMU_BUF_DURATION);
  }

  /* buffer 0 & 1 */
  gst_harness_crank_single_clock_wait (h);
  for (i = 0; i < 2; i++) {
    out_buf = gst_harness_pull (h);
    fail_unless_equals_int (i, get_rtp_seq_num (out_buf));
    gst_buffer_unref (out_buf);
  }

  /* lost event */
  gst_harness_crank_single_clock_wait (h);
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 2, 2 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);

  /*  Now data comes in again, a "bulk" lost packet is created for 3 -> 5 */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (16)));

  /* Manually check the first rtx event */
  out_event = gst_harness_pull_upstream_event (h);
  verify_rtx_event (out_event, 6, 6 * PCMU_BUF_DURATION, 10, PCMU_BUF_DURATION);
  /* Go throught the rest of rtx events. A bit more relaxed since order is
   * partly an implentation detail. */
  for (i = 0; i < 12; i++) {
    const GstStructure *s;
    guint seqnum, retry;

    fail_unless (out_event = gst_harness_pull_upstream_event (h));
    fail_unless (s = gst_event_get_structure (out_event));
    fail_unless (gst_structure_get_uint (s, "seqnum", &seqnum));
    fail_unless (gst_structure_get_uint (s, "retry", &retry));
    fail_unless (seqnum >= 6 && seqnum <= 12);

    verify_rtx_event (out_event, seqnum, seqnum * PCMU_BUF_DURATION,
        10 + retry * 40, PCMU_BUF_DURATION);
  }

  fail_unless_equals_int (0, gst_harness_upstream_events_in_queue (h));
  fail_unless_equals_int (0, gst_harness_events_in_queue (h));
  fail_unless_equals_int (0, gst_harness_buffers_in_queue (h));

  for (i = 8; i < 16; i++) {
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (i)));
  }

  gst_harness_crank_single_clock_wait (h);
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 3, 3 * PCMU_BUF_DURATION,
      3 * PCMU_BUF_DURATION);

  gst_harness_crank_single_clock_wait (h);
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 6, 6 * PCMU_BUF_DURATION,
      1 * PCMU_BUF_DURATION);

  gst_harness_crank_single_clock_wait (h);
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 7, 7 * PCMU_BUF_DURATION,
      1 * PCMU_BUF_DURATION);

  /* 8 */
  for (i = 8; i <= 16; i++) {
    GstBuffer *out_buf = gst_harness_pull (h);
    GST_DEBUG ("pop %d", i);
    fail_unless_equals_int (i, get_rtp_seq_num (out_buf));
    gst_buffer_unref (out_buf);
  }

  fail_unless_equals_int (0, gst_harness_upstream_events_in_queue (h));
  fail_unless_equals_int (0, gst_harness_events_in_queue (h));
  fail_unless_equals_int (0, gst_harness_buffers_in_queue (h));

  /* rtx x 3 */
  for (i = 0; i < 3; i++) {
    gst_harness_crank_single_clock_wait (h);
    out_event = gst_harness_pull_upstream_event (h);
    verify_rtx_event (out_event,
        17, 17 * PCMU_BUF_DURATION, 10 + 40 * i, PCMU_BUF_DURATION);
  }

  /* lost event for 17 */
  gst_harness_crank_single_clock_wait (h);
  out_event = gst_harness_pull_event (h);
  verify_lost_event (out_event, 17, 17 * PCMU_BUF_DURATION, PCMU_BUF_DURATION);

  fail_unless_equals_int (0, gst_harness_upstream_events_in_queue (h));
  fail_unless_equals_int (0, gst_harness_events_in_queue (h));
  fail_unless_equals_int (0, gst_harness_buffers_in_queue (h));

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) 11,
              "num-lost", G_TYPE_UINT64, (guint64) 7,
              "rtx-count", G_TYPE_UINT64, (guint64) 19,
              "rtx-success-count", G_TYPE_UINT64, (guint64) 0,
              "rtx-rtt", G_TYPE_UINT64, (guint64) 0, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_deadline_ts_offset)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstClockID id;
  const gint jb_latency_ms = 10;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);

  g_object_set (h->element, "latency", jb_latency_ms, NULL);

  /* push the first buffer in */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (0)));

  /* wait_next_timeout() syncs on the deadline timer */
  gst_test_clock_wait_for_next_pending_id (testclock, &id);
  fail_unless_equals_uint64 (jb_latency_ms * GST_MSECOND,
      gst_clock_id_get_time (id));
  gst_clock_id_unref (id);

  /* add ts-offset while waiting */
  g_object_set (h->element, "ts-offset", 20 * GST_MSECOND, NULL);

  gst_test_clock_set_time_and_process (testclock, jb_latency_ms * GST_MSECOND);

  /* wait_next_timeout() syncs on the new deadline timer */
  gst_test_clock_wait_for_next_pending_id (testclock, &id);
  fail_unless_equals_uint64 ((20 + jb_latency_ms) * GST_MSECOND,
      gst_clock_id_get_time (id));
  gst_clock_id_unref (id);

  /* now make deadline timer timeout */
  gst_test_clock_set_time_and_process (testclock,
      (20 + jb_latency_ms) * GST_MSECOND);

  gst_buffer_unref (gst_harness_pull (h));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_dts_gap_larger_than_latency)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstEvent *out_event;
  gint jb_latency_ms = 100;
  GstClockTime dts_after_gap = (jb_latency_ms + 1) * GST_MSECOND;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);
  g_object_set (h->element, "do-lost", TRUE, "latency", jb_latency_ms, NULL);

  /* push first buffer through */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (0)));
  fail_unless (gst_harness_crank_single_clock_wait (h));
  gst_buffer_unref (gst_harness_pull (h));

  /* Push packet with DTS larger than latency */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer_full (dts_after_gap,
              TRUE, 5, 5 * PCMU_RTP_TS_DURATION)));

  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (int i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* Time out and verify lost events */
  for (gint i = 1; i < 5; i++) {
    GstClockTime dur = dts_after_gap / 5;
    fail_unless (gst_harness_crank_single_clock_wait (h));
    out_event = gst_harness_pull_event (h);
    verify_lost_event (out_event, i, i * dur, dur);
  }

  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-lost", G_TYPE_UINT64, (guint64) 4, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_push_big_gap)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstBuffer *buf;
  const gint num_consecutive = 5;
  gint i;

  gst_harness_set_src_caps (h, generate_caps ());

  for (i = 0; i < num_consecutive; i++)
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (1000 + i)));

  fail_unless (gst_harness_crank_single_clock_wait (h));

  for (i = 0; i < num_consecutive; i++) {
    GstBuffer *buf = gst_harness_pull (h);
    fail_unless_equals_int (1000 + i, get_rtp_seq_num (buf));
    gst_buffer_unref (buf);
  }

  /* Push more packets from a different sequence number domain
   * to trigger "big gap" logic. */
  for (i = 0; i < num_consecutive; i++)
    fail_unless_equals_int (GST_FLOW_OK,
        gst_harness_push (h, generate_test_buffer (20000 + i)));

  fail_unless (gst_harness_crank_single_clock_wait (h));

  for (i = 0; i < num_consecutive; i++) {
    GstBuffer *buf = gst_harness_pull (h);
    fail_unless_equals_int (20000 + i, get_rtp_seq_num (buf));
    gst_buffer_unref (buf);
  }

  /* Final buffer should be pushed straight through */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer (20000 + num_consecutive)));
  buf = gst_harness_pull (h);
  fail_unless_equals_int (20000 + num_consecutive, get_rtp_seq_num (buf));
  gst_buffer_unref (buf);

  gst_harness_teardown (h);
}

GST_END_TEST;

typedef struct
{
  guint seqnum_offset;
  guint late_buffer;
} TestLateArrivalInput;

static const TestLateArrivalInput
    test_considered_lost_packet_in_large_gap_arrives_input[] = {
  {0, 1}, {0, 2}, {65535, 1}, {65535, 2}, {65534, 1}, {65534, 2}
};

GST_START_TEST (test_considered_lost_packet_in_large_gap_arrives)
{
  GstHarness *h = gst_harness_new ("rtpjitterbuffer");
  GstTestClock *testclock;
  GstClockID id;
  GstBuffer *buffer;
  gint jb_latency_ms = 20;
  GstEvent *event;
  const TestLateArrivalInput *test_input =
      &test_considered_lost_packet_in_large_gap_arrives_input[__i__];
  guint seq_offset = test_input->seqnum_offset;
  guint late_buffer = test_input->late_buffer;

  gst_harness_set_src_caps (h, generate_caps ());
  testclock = gst_harness_get_testclock (h);
  g_object_set (h->element, "do-lost", TRUE, "latency", jb_latency_ms, NULL);

  /* first push buffer 0 */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer_full (0 * PCMU_BUF_DURATION,
              TRUE, 0 + seq_offset, 0 * PCMU_RTP_TS_DURATION)));
  fail_unless (gst_harness_crank_single_clock_wait (h));
  gst_buffer_unref (gst_harness_pull (h));

  /* drop GstEventStreamStart & GstEventCaps & GstEventSegment */
  for (gint i = 0; i < 3; i++)
    gst_event_unref (gst_harness_pull_event (h));

  /* hop over 3 packets, and push buffer 4 (gap of 3) */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, generate_test_buffer_full (4 * PCMU_BUF_DURATION,
              TRUE, 4 + seq_offset, 4 * PCMU_RTP_TS_DURATION)));

  /* the jitterbuffer should be waiting for the timeout of a "large gap timer"
   * for buffer 1 and 2 */
  gst_test_clock_wait_for_next_pending_id (testclock, &id);
  fail_unless_equals_uint64 (1 * PCMU_BUF_DURATION +
      jb_latency_ms * GST_MSECOND, gst_clock_id_get_time (id));
  gst_clock_id_unref (id);

  /* now buffer 1 sneaks in before the lost event for buffer 1 and 2 is
   * processed */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h,
          generate_test_buffer_full (late_buffer * PCMU_BUF_DURATION, TRUE,
              late_buffer + seq_offset, late_buffer * PCMU_RTP_TS_DURATION)));

  /* time out for lost packets 1 and 2 (one event, double duration) */
  fail_unless (gst_harness_crank_single_clock_wait (h));
  event = gst_harness_pull_event (h);
  verify_lost_event (event, 1 + seq_offset, 1 * PCMU_BUF_DURATION,
      2 * PCMU_BUF_DURATION);

  /* time out for lost packets 3 */
  fail_unless (gst_harness_crank_single_clock_wait (h));
  event = gst_harness_pull_event (h);
  verify_lost_event (event, 3 + seq_offset, 3 * PCMU_BUF_DURATION,
      1 * PCMU_BUF_DURATION);

  /* buffer 4 is pushed as normal */
  buffer = gst_harness_pull (h);
  fail_unless_equals_int ((4 + seq_offset) & 0xffff, get_rtp_seq_num (buffer));
  gst_buffer_unref (buffer);

  /* we have lost 3, and one of them arrived eventually, but too late */
  fail_unless (verify_jb_stats (h->element,
          gst_structure_new ("application/x-rtp-jitterbuffer-stats",
              "num-pushed", G_TYPE_UINT64, (guint64) 2,
              "num-lost", G_TYPE_UINT64, (guint64) 3,
              "num-late", G_TYPE_UINT64, (guint64) 1, NULL)));

  gst_object_unref (testclock);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_performance)
{
  GstHarness *h =
      gst_harness_new_parse
      ("rtpjitterbuffer do-lost=1 do-retransmission=1 latency=1000");
  GTimer *timer = g_timer_new ();
  const gdouble test_duration = 2.0;
  guint buffers_pushed = 0;
  guint buffers_received;

  gst_harness_set_src_caps (h, generate_caps ());
  gst_harness_use_systemclock (h);

  while (g_timer_elapsed (timer, NULL) < test_duration) {
    /* Simulate 1ms packets */
    guint n = buffers_pushed * 2;       // every packet also produces a gap
    guint16 seqnum = n & 0xffff;
    guint32 rtp_ts = n * 8;
    GstClockTime dts = n * GST_MSECOND;
    gst_harness_push (h, generate_test_buffer_full (dts, TRUE, seqnum, rtp_ts));
    buffers_pushed++;
    g_usleep (G_USEC_PER_SEC / 10000);
  }
  g_timer_destroy (timer);

  buffers_received = gst_harness_buffers_received (h);
  GST_INFO ("Pushed %d, received %d (%.1f%%)", buffers_pushed, buffers_received,
      100.0 * buffers_received / buffers_pushed);

  gst_harness_teardown (h);
}

GST_END_TEST;

static Suite *
rtpjitterbuffer_suite (void)
{
  Suite *s = suite_create ("rtpjitterbuffer");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_push_forward_seq);
  tcase_add_test (tc_chain, test_push_backward_seq);
  tcase_add_test (tc_chain, test_push_unordered);
  tcase_add_test (tc_chain, test_basetime);
  tcase_add_test (tc_chain, test_clear_pt_map);

  tcase_add_test (tc_chain, test_only_one_lost_event_on_large_gaps);
  tcase_add_test (tc_chain, test_two_lost_one_arrives_in_time);
  tcase_add_test (tc_chain, test_late_packets_still_makes_lost_events);
  tcase_add_test (tc_chain, test_all_packets_are_timestamped_zero);
  tcase_add_loop_test (tc_chain, test_num_late_when_considered_lost_arrives, 0,
      2);
  tcase_add_test (tc_chain, test_reorder_of_non_equidistant_packets);
  tcase_add_test (tc_chain,
      test_loss_equidistant_spacing_with_parameter_packets);

  tcase_add_test (tc_chain, test_rtx_expected_next);
  tcase_add_test (tc_chain, test_rtx_two_missing);
  tcase_add_test (tc_chain, text_rtx_two_missing_early);
  tcase_add_test (tc_chain, test_rtx_packet_delay);
  tcase_add_test (tc_chain, test_rtx_buffer_arrives_just_in_time);
  tcase_add_test (tc_chain, test_rtx_buffer_arrives_too_late);
  tcase_add_test (tc_chain, test_rtx_original_buffer_does_not_update_rtx_stats);
  tcase_add_test (tc_chain, test_rtx_duplicate_packet_updates_rtx_stats);
  tcase_add_test (tc_chain,
      test_rtx_buffer_arrives_after_lost_updates_rtx_stats);
  tcase_add_test (tc_chain, test_rtx_rtt_larger_than_retry_timeout);
  tcase_add_test (tc_chain, test_rtx_no_request_if_time_past_retry_period);

  tcase_add_test (tc_chain, test_gap_exceeds_latency);
  tcase_add_test (tc_chain, test_deadline_ts_offset);
  tcase_add_test (tc_chain, test_dts_gap_larger_than_latency);
  tcase_add_test (tc_chain, test_push_big_gap);

  tcase_add_loop_test (tc_chain,
      test_considered_lost_packet_in_large_gap_arrives, 0,
      G_N_ELEMENTS (test_considered_lost_packet_in_large_gap_arrives_input));

  tcase_add_test (tc_chain, test_performance);

  return s;
}

GST_CHECK_MAIN (rtpjitterbuffer);
