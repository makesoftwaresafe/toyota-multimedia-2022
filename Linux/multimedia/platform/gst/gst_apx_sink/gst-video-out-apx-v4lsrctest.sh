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
# Arch NOK:     
# Board OK:     
# Board NOK:    imx6q-sd
# Reset:        0
# Timeout:      45
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
SMOKETEST_LAYER_WIDTH=800
SMOKETEST_LAYER_HEIGHT=480
SMOKETEST_DUMP_DELAY="3" # time for process to start and after that the dump will be done
SMOKETEST_TIME="10" # time after test will be killed with SIGKILL

# define test variables
TEST_CMD="${SMOKETEST_GST_LAUNCH}"
TEST_SEARCH_DIR="/sys/class/video4linux"
TEST_CAM_DEV=""

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
echo "GSTREAMER VIDEO out for ${SMOKETEST_GBE} using v4lsrc plugin"
echo "expected result: Image captured by camera sensor displayed on screen"
echo ""

# cleanup before starting
FN_smoketest_cleanup_extra

FN_start_service "early-devices-setup.service"
FN_graphics_create_layer "${SMOKETEST_GBE}"

for DEV in $(ls ${TEST_SEARCH_DIR}); do
  NAME=$(cat ${TEST_SEARCH_DIR}/${DEV}/name)
  if [ "${NAME}" == "Mxc Camera" ]; then
    TEST_CAM_DEV="/dev/${DEV}"
    break;
  fi
  if [ "${NAME}" == "mx6-camera" ]; then
    TEST_CAM_DEV="/dev/${DEV}"
    break;
  fi
  if [[ "${NAME}" = "ipu"*"_csi"*" capture" ]]; then
    TEST_CAM_DEV="/dev/${DEV}"
    break;
  fi
done

if [ "${TEST_CAM_DEV}" = "" ]; then
  FN_smoketest_error "Mxc Camera not found."
fi

echo ${SMOKETEST_DELIMITER}
echo "using camera device '${TEST_CAM_DEV}'"
echo "Starting the video streaming in background ..."

FN_execute "${TEST_CMD} mfw_v4lsrc pix-fmt=9 device=${TEST_CAM_DEV} ! gst_apx_sink display-width=${SMOKETEST_LAYER_WIDTH} display-height=${SMOKETEST_LAYER_HEIGHT} --gst-debug=*:2" "1"
APX_V4L_TEST_PID="${SMOKETEST_EXTRA_PID}"
FN_graphics_dump "${SMOKETEST_GBE}" "camera" "${SMOKETEST_DUMP_DELAY}"

echo ${SMOKETEST_DELIMITER}
echo "Sending SIGINT to close the application..."

FN_kill_pid "${APX_V4L_TEST_PID}" "SIGINT"

# destroy layer
FN_graphics_destroy_layer "${SMOKETEST_GBE}"

FN_smoketest_result "$?"

