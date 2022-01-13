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
# Description:  Smoketest to demonstrate how zero-copy is achieved in rendering a
#               decoded video-frame using opengl
# Group:        video,opengl,wayland
# Arch NOK:
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

# smoketest variables
SMOKETEST_DUMP_DELAY="2" # wait time before dump
SMOKETEST_DURATION="3"
SMOKETEST_LAYER_ID="3000"
SMOKETEST_MAX_WAIT_TIME="40" # factor of sleep time as max wait time in sec - this is needed for x86

# test command
TEST_CMD="/usr/bin/gst_appsink_zerocopy"

# test variables
TEST_MOVIE="/opt/platform/unit_tests/01_189_libx264_1920x1080_6M.mp4"
DISP_WIDTH=1920
DISP_HEIGHT=1080

################################################################################
# re-define extra cleanup which will be automatically called on normal cleanup

function FN_smoketest_cleanup_extra()
{
  echo "  ${FUNCNAME} ..."

  # kill all running test commands
	killall --quiet -9 ${TEST_CMD} &>/dev/null || true

  echo "  ${FUNCNAME} done."
}

################################################################################

# cleanup before start
FN_smoketest_cleanup_extra

#Indicating that below gstreamer testcase is only for RCAR3
FN_smoketest_allow "product" "rcar3"

# create layer
FN_graphics_create_layer "${SMOKETEST_GBE}"

# start test in background
FN_execute "eval ${TEST_CMD} decode ${TEST_MOVIE} ${DISP_WIDTH} ${DISP_HEIGHT}" "1"
TEST_PID="${SMOKETEST_EXTRA_PID}"

echo "Sleeping ${SMOKETEST_DURATION} second(s) and abort '${TEST_PID}' log '${SMOKETEST_LOG_FILE}' ..."
sleep ${SMOKETEST_DURATION}

# dump graphic data and wait for test pid
FN_graphics_dump "${SMOKETEST_GBE}" "weston" "${SMOKETEST_DUMP_DELAY}"

FN_check_pid "${TEST_PID}" # test should still run - otherwise error
kill -SIGABRT ${TEST_PID}

wait ${TEST_PID} &>/dev/null
RET=$?

# destroy layer
FN_graphics_destroy_layer "${SMOKETEST_GBE}"

# value 0 or 134 allowed (SIGABRT yields 134)
if [[ ${RET} == "0" || ${RET} == "134" ]]; then
  FN_smoketest_result "0" "Return code: ${RET} is allowed"
else
  FN_smoketest_result "1" "Return code: ${RET} is not allowed"
fi
