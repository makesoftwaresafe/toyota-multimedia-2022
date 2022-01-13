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
# Description:  This test checks gstreamer audio codec mp3
# Group:        audio,gstreamer
# Arch NOK:     x86
# Board OK:
# Board NOK:
# Reset:        0
# Timeout:      30
# Check:        text
# Host-Pre:
# Host-Post:
#
################################################################################

DIR=$(dirname $0)
source "${DIR}/smoketest.func" || exit 1

# initialize smoketest
FN_smoketest_init || FN_smoketest_error "Smoketest init failed."

# load gstreamer functionality
FN_smoketest_use "gstreamer" || FN_smoketest_error "Use gstreamer functionality failed."

# define test variables
TEST_CMD="${SMOKETEST_GST_LAUNCH}"
TEST_DIR="/opt/platform/unit_tests"
TEST_LIST="smoketest_audio.wav"
TEST_MP3=""

if [ "${SMOKETEST_FAMILY}" == "gr-mrb-64" ] || [ "${SMOKETEST_FAMILY}" == "salvator-x" ] || [ "${SMOKETEST_PRODUCT}" == "sim" ]; then
  TEST_MP3_ENCODER_PLUGIN="sasken_mp3enc"
elif [ "${SMOKETEST_FAMILY}" == "mx6q" ] || [ "${SMOKETEST_PRODUCT}" == "vbox" ]; then
  TEST_MP3_ENCODER_PLUGIN="mfw_mp3encoder"
else
  FN_smoketest_error "No MP3 encoder defined."
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

# The transcoding from WAV to MP3 work with the MP3 encoder only for 32, 44.1 und 48 kHz sample rates.
#   If you use the Plugin "audioresample" it will work.

for TEST_WAV in ${TEST_LIST}; do
  TEST_FILE="${TEST_DIR}/${TEST_WAV}"
  TEST_MP3="${SMOKETEST_TMP_DIR}/${TEST_WAV}.mp3"

  rm -f ${TEST_MP3}

  FN_execute "${TEST_CMD} filesrc location=${TEST_FILE} ! decodebin ! audioconvert ! audioresample ! ${TEST_MP3_ENCODER_PLUGIN} ! filesink location=${TEST_MP3}"

  if [ ! -s "${TEST_MP3}" ]; then
    FN_smoketest_error "Convertion for '${TEST_FILE}' to '${TEST_MP3}' failed."
  fi
done

# playback created files
for TEST_FILE in ${TEST_LIST}; do
  FN_execute "${TEST_CMD} filesrc location=${SMOKETEST_TMP_DIR}/${TEST_FILE}.mp3 ! decodebin ! alsasink device=entertainment_main"
done

FN_smoketest_result "0"

