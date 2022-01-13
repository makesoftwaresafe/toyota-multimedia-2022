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
# Description:  This test checks gstreamer audio out
# Group:        audio,gstreamer
# Arch NOK:
# Board OK:
# Board NOK:
# Reset:        0
# Timeout:      120
# Check:        audio
# Host-Pre:
# Host-Post:
#
################################################################################

DIR=$(dirname $0)
source "${DIR}/smoketest.func" || exit 1

# initialize smoketest
FN_smoketest_init || FN_smoketest_error "Smoketest init failed."

# load audio functionality
FN_smoketest_use "audio" || FN_smoketest_error "Use audio failed."

# load gstreamer functionality
FN_smoketest_use "gstreamer" || FN_smoketest_error "Use gstreamer functionality failed."

# define test variables
TEST_CMD="${SMOKETEST_GST_LAUNCH}"
TEST_DEVICE="device=entertainment_main"
TEST_DIR="/opt/platform/unit_tests"
TEST_LIST="smoketest_audio.ogg smoketest_audio.wav smoketest_audio.aac smoketest_audio.mp3"
TEST_USER="test-adit-multimedia-audio"

# add more possbile test cases for Gen4

if [ "${SMOKETEST_FAMILY}" == "salvator-x" ] || [ "${SMOKETEST_FAMILY}" == "gr-mrb-64" ] || [ "${SMOKETEST_PRODUCT}" == "sim" ]; then
  TEST_LIST="smoketest_audio.flac smoketest_audio.opus ${TEST_LIST}"
fi

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

# cleanup before starting
FN_smoketest_cleanup_extra

FN_check_user "${TEST_USER}"
FN_audio_start_record "${TEST_USER}"

for TEST_FILE in ${TEST_LIST}; do
  FN_user_execute "${TEST_USER}" "${TEST_CMD} filesrc location=${TEST_DIR}/${TEST_FILE} ! decodebin ! audioconvert ! alsasink ${TEST_DEVICE}"
done

FN_audio_stop_record
FN_smoketest_result "0" "User '${TEST_USER}' tested."

