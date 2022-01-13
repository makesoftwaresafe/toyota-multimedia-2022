#!/bin/bash

################################################################################
# This source code is proprietary of ADIT
# Copyright (C) Advanced Driver Information Technology GmbH
# All rights reserved
################################################################################

################################################################################
#
# --- AUTOMATED SMOKETEST ---
# 
# Description:  This test checks gstreamer video out apx v4l src
# Group:        video,gstreamer,gst-apx
# Arch NOK:     x86
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      90
# Check:        video
# Host-Pre:     
# Host-Post:    
#
################################################################################

DIR=$(dirname $0)
source "${DIR}/smoketest.func" || exit 1

# set graphics backend
SMOKETEST_GBE="wayland"

# initialize smoketest
FN_smoketest_init || FN_smoketest_error "Smoketest init failed."

# load dlt functionality
FN_smoketest_use "dlt" || FN_smoketest_error "Use dlt functionality failed."

# load graphics functionality - will switch also graphic backend with option
FN_smoketest_use "graphics" "${SMOKETEST_GBE}" || FN_smoketest_error "Use graphics with '${SMOKETEST_GBE}' failed."

# load gstreamer functionality
FN_smoketest_use "gstreamer" || FN_smoketest_error "Use gstreamer functionality failed."

# smoketest variables
SMOKETEST_LAYER_ID=3000
SMOKETEST_LAYER_WIDTH=800
SMOKETEST_LAYER_HEIGHT=480

# define test variables
TEST_CMD="${SMOKETEST_GST_LAUNCH}"
TEST_USER="test-adit-multimedia-no-vpu"
TEST_COLORS="red green blue white"
TEST_FORMATS="YV12 NV12 YUY2 UYVY"
TEST_BUFFERS="num-buffers=75"
TEST_DUMP_DELAY="2" # time for process to start - test should show minimum 2 seconds a picture

# gstreamer definitions
COMMON_CAPS="framerate=\"(fraction)25/1\",width=${SMOKETEST_LAYER_WIDTH},height=${SMOKETEST_LAYER_HEIGHT}"
SINK="gst_apx_sink display-width=${SMOKETEST_LAYER_WIDTH} display-height=${SMOKETEST_LAYER_HEIGHT}"

export VPU_ENABLE_DIVX="1"

################################################################################
# re-define extra cleanup which will be automatically called on normal cleanup

function FN_smoketest_cleanup_extra()
{
  echo "  ${FUNCNAME} ..."

  # if the test command is still running - kill it
  killall --quiet -9 ${TEST_CMD} &> /dev/null || true

  echo "  ${FUNCNAME} - done."
}

################################################################################

echo ""
echo "GSTREAMER VIDEO out for APX API, software buffer rendering for ${SMOKETEST_GBE}"
echo "expected result: several test videos showing on screen"
echo ""

# cleanup before starting
FN_smoketest_cleanup_extra

FN_check_user "${TEST_USER}" "1"
FN_start_service "layer-management-wayland.service"
FN_graphics_create_layer "${SMOKETEST_GBE}"

echo ${SMOKETEST_DELIMITER}
echo "Testing RGBA for initialization (no screen dump) - this should prevent timing issues"
FN_user_execute "${TEST_USER}" "${TEST_CMD} videotestsrc ${TEST_BUFFERS} ! video/x-raw-rgb,depth=32,${COMMON_CAPS} ! ${SINK}"

echo ${SMOKETEST_DELIMITER}
echo "Testing RGBA"
FN_user_execute "${TEST_USER}" "${TEST_CMD} videotestsrc ${TEST_BUFFERS} ! video/x-raw-rgb,depth=32,${COMMON_CAPS} ! ${SINK}" "1"
FN_graphics_dump_waitforpid "$!" "${SMOKETEST_GBE}" "rgba" "${TEST_DUMP_DELAY}"

echo ${SMOKETEST_DELIMITER}
echo "Testing RGB565"
FN_user_execute "${TEST_USER}" "${TEST_CMD} videotestsrc ${TEST_BUFFERS} ! video/x-raw-rgb,depth=16,bpp=16,${COMMON_CAPS} ! ${SINK}" "1"
FN_graphics_dump_waitforpid "$!" "${SMOKETEST_GBE}" "rgb565" "${TEST_DUMP_DELAY}"

echo ${SMOKETEST_DELIMITER}
# Testing for SWGIII-4529
for COLOR in ${TEST_COLORS}; do
  # initialize pipeline with first color - may take longer
  if [ "${FIRST_COLOR:-}" == "" ]; then
    FIRST_COLOR="${COLOR}"
    echo "Testing first color '${COLOR}' image (no screen dump) - this should prevent timing issues"
    FN_user_execute "${TEST_USER}" "${TEST_CMD} videotestsrc pattern=${COLOR} ${TEST_BUFFERS} ! cairotextoverlay text=${COLOR} ! ffmpegcolorspace ! video/x-raw-rgb,depth=32,${COMMON_CAPS} ! ${SINK}"
    echo ${SMOKETEST_DELIMITER}
  fi

  echo "Testing color '${COLOR}'"
  FN_user_execute "${TEST_USER}" "${TEST_CMD} videotestsrc pattern=${COLOR} ${TEST_BUFFERS} ! cairotextoverlay text=${COLOR} ! ffmpegcolorspace ! video/x-raw-rgb,depth=32,${COMMON_CAPS} ! ${SINK}" "1"
  FN_graphics_dump_waitforpid "$!" "${SMOKETEST_GBE}" "color-${COLOR}" "${TEST_DUMP_DELAY}"
done

echo ${SMOKETEST_DELIMITER}
for FORMAT in ${TEST_FORMATS}; do
  # initialize pipeline with first format - may take longer
  if [ "${FIRST_FORMAT:-}" == "" ]; then
    FIRST_FORMAT="${FORMAT}"
    echo "Testing first format '${FORMAT}' (no screen dump) - this should prevent timing issues"
    FN_user_execute "${TEST_USER}" "${TEST_CMD} videotestsrc ${TEST_BUFFERS} ! video/x-raw-yuv,format=\"(fourcc)${FORMAT}\",${COMMON_CAPS} ! ${SINK}"
    echo ${SMOKETEST_DELIMITER}
  fi

  echo "Testing format '${FORMAT}'"
  FN_user_execute "${TEST_USER}" "${TEST_CMD} videotestsrc ${TEST_BUFFERS} ! video/x-raw-yuv,format=\"(fourcc)${FORMAT}\",${COMMON_CAPS} ! ${SINK}" "1"
  FN_graphics_dump_waitforpid "$!" "${SMOKETEST_GBE}" "format-${FORMAT}" "${TEST_DUMP_DELAY}"
done

# destroy layer
FN_graphics_destroy_layer "${SMOKETEST_GBE}"

FN_smoketest_result "0"

