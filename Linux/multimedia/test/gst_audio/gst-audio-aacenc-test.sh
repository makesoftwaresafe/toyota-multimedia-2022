#!/bin/bash

################################################################################
#This software has been developed by Advanced Driver Information Technology.
#Copyright(c) 2019 Advanced Driver Information Technology GmbH,
#Advanced Driver Information Technology Corporation, Robert Bosch GmbH,
#Robert Bosch Car Multimedia GmbH and DENSO Corporation.
#All rights reserved.
################################################################################

################################################################################
#
# --- AUTOMATED SMOKETEST ---
#
# Description:  This test checks gstreamer audio codec aac
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
TEST_LIST="audio32k2ch16S.wav"
TEST_AAC=""
CHECKSUM_AAC="6346a90339c91a6ee24c2a2e71bab341"
CHECKSUM_ENC_AAC=""
BITRATE="128"
MODE="1"


if [ "${SMOKETEST_FAMILY}" == "gr-mrb-64" ] || [ "${SMOKETEST_FAMILY}" == "rcar-gen3" ] || [ "${SMOKETEST_PRODUCT}" == "sim" ]; then
  TEST_AAC_ENCODER_PLUGIN="sasken_aaclcenc"
else
  FN_smoketest_error "No AAC encoder defined."
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

# The encoding from WAV to AAC 
for TEST_AAC in ${TEST_LIST}; do
  TEST_FILE="${TEST_DIR}/${TEST_AAC}"
  TEST_AAC="${SMOKETEST_TMP_DIR}/${TEST_AAC}.aac"

  rm -f ${TEST_AAC}

  FN_execute "${TEST_CMD} filesrc location=${TEST_FILE} !  audio/x-raw,  format=S16LE,channels=2,rate=32000 !  audioconvert ! audioresample ! ${TEST_AAC_ENCODER_PLUGIN} bitrate=${BITRATE} bitrate-mode=${MODE} ! filesink location=${TEST_AAC}"

  if [ ! -s "${TEST_AAC}" ]; then
    FN_smoketest_error "Convertion for '${TEST_FILE}' to '${TEST_AAC}' failed."
  fi
done
 
  CHECKSUM_ENC_AAC=`md5sum ${TEST_AAC} | awk '{print $1}'`

  if [ "${CHECKSUM_AAC}" == "${CHECKSUM_ENC_AAC}" ]; then
	echo "CHECKSUM -> match"
  else
	FN_smoketest_error "CHECKSUM -> Mismatch"
  fi

# playback created files
  FN_execute "${TEST_CMD} filesrc location=${TEST_AAC} ! decodebin ! alsasink device=entertainment_main"

FN_smoketest_result "0"

