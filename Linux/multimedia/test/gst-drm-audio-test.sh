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
# Description:  This test checks gstreamer drm audio
# Group:        audio,gstreamer,drm
# Arch NOK:     
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      30
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
FN_smoketest_use "audio" || FN_smoketest_error "Use audio functionality failed."

# load gstreamer functionality
FN_smoketest_use "gstreamer" || FN_smoketest_error "Use gstreamer functionality failed."

# define test variables
TEST_CMD="${DIR}/gst_drm_testapp"
TEST_DIR="${DIR}/unit_tests"
TEST_USER="test-adit-multimedia-audio"
TEST_LIST="test.DRM30.10000bps.9600Hz.drm A-A-C_Song_48000_mono_sbr0_ps0_csr12_sf500_10kbit_mono.drm"

export GST_REGISTRY="${SMOKETEST_TMP_DIR}/.gstreamer/registry.bin"

################################################################################
# re-define extra cleanup which will be automatically called on normal cleanup

function FN_smoketest_cleanup_extra()
{
  echo "  ${FUNCNAME} ..."

  # if the test command is still running - kill it
  killall --quiet -9 ${TEST_CMD} &>/dev/null || true

  # delete gstreamer registry
  rm -f ${GST_REGISTRY}

  echo "  ${FUNCNAME} done."
}

################################################################################

if [ ! -f ${TEST_CMD} ]; then
  FN_smoketest_error "Test '${TEST_CMD}' not found."
fi

# cleanup before starting
FN_smoketest_cleanup_extra

# build gstreamer registry
FN_user_execute "${TEST_USER}" "${SMOKETEST_GST_INSPECT} >/dev/null 2>&1"

FN_audio_start_record "${TEST_USER}"

for TEST in ${TEST_LIST}; do
  TEST_FILE="${TEST_DIR}/${TEST}"

  if [ ! -f "${TEST_FILE}" ]; then
    FN_smoketest_error "Test file '${TEST_FILE}' not found."
  fi

  FN_user_execute "${TEST_USER}" "${TEST_CMD} -d entertainment_main -i ${TEST_FILE} -l 20"
  echo
done

FN_audio_stop_record
FN_smoketest_result "0" "User '${TEST_USER}' tested."

