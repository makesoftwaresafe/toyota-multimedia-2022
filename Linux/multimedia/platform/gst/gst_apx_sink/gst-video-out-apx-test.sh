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
# Description:  This test checks gstreamer video out apx
# Group:        video,gstreamer,gst-apx
# Arch NOK:     x86
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      60
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

# load graphics functionality - will switch also graphic backend with option
FN_smoketest_use "graphics" "${SMOKETEST_GBE}" || FN_smoketest_error "Use graphics with '${SMOKETEST_GBE}' failed."

# load gstreamer functionality
FN_smoketest_use "gstreamer" || FN_smoketest_error "Use gstreamer functionality failed."

# smoketest variables
SMOKETEST_LAYER_ID=3000
SMOKETEST_LAYER_WIDTH=800
SMOKETEST_LAYER_HEIGHT=480
SMOKETEST_SCREEN_NUM=${SCREEN_NUM_LVDS0}
SMOKETEST_SURFACE_ID=40
SMOKETEST_MAX_WAIT_TIME="30"

# define test variables
TEST_CMD="${SMOKETEST_GST_LAUNCH}"
TEST_DUMP_DELAY="3" # time for process to start - test should show minimum 2 seconds a picture
TEST_MOVIE="/opt/platform/unit_tests/01_189_libxvid_1920x1080_6M.mp4"
TEST_USER="test-adit-multimedia"

################################################################################
# re-define extra cleanup which will be automatically called on normal cleanup

function FN_smoketest_cleanup_extra()
{
  echo "  ${FUNCNAME} ..."

  # if the test command is still running - kill it
  killall --quiet -9 ${TEST_CMD} &> /dev/null || true

  echo "  ${FUNCNAME} done."
}

################################################################################

echo ""
echo "GSTREAMER VIDEO out for ${SMOKETEST_GBE}"
echo "expected result: movie showing on screen"
echo ""

# cleanup before starting
FN_smoketest_cleanup_extra

FN_check_user "${TEST_USER}" "1"
FN_start_service "early-devices-setup.service"
FN_start_service "layer-management-wayland.service"
FN_graphics_create_layer "${SMOKETEST_GBE}"

# in case user is a member of audio group test also with audio
id ${TEST_USER} | grep -i "(audio)"

if [ "$?" == "0" ]; then
  echo "  check with audio ..."
  FN_user_execute "${TEST_USER}" "${TEST_CMD} playbin2 uri=file://${TEST_MOVIE} video-sink=\"gst_apx_sink\" audio-sink=\"alsasink device=entertainment_main\" --gst-debug=*:1" "1"
  FN_graphics_dump_waitforpid "$!" "${SMOKETEST_GBE}" "with-audio" "${TEST_DUMP_DELAY}"
fi

echo "  check without audio ..."
FN_user_execute "${TEST_USER}" "${TEST_CMD} filesrc location=${TEST_MOVIE} ! decodebin ! gst_apx_sink --gst-debug=*:1" "1"
FN_graphics_dump_waitforpid "$!" "${SMOKETEST_GBE}" "without-audio" "${TEST_DUMP_DELAY}"

# destroy layer
FN_graphics_destroy_layer "${SMOKETEST_GBE}"

FN_smoketest_result "0"

