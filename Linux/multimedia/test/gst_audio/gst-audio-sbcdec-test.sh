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
# Description:  This test checks gstreamer sbc decoder
# Group:        audio,gstreamer,gst-sbc
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
TEST_CMD="${SMOKETEST_GST_LAUNCH}"
TEST_DIR="/opt/platform/unit_tests"
TEST_LIST="smoketest_audio.sbc"
TEST_USER="test-adit-multimedia-audio"
TEST_SBC_DECODER_PLUGIN="decodebin"

if [ "${SMOKETEST_FAMILY}" == "mx6q" ] || [ "${SMOKETEST_PRODUCT}" == "vbox" ]; then
  TEST_SBC_DECODER_PLUGIN="sbcdec"
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
  FN_user_execute	"${TEST_USER}" "${TEST_CMD} filesrc location=${TEST_DIR}/${TEST_FILE} ! ${TEST_SBC_DECODER_PLUGIN} ! alsasink device=entertainment_main"
done

FN_audio_stop_record
FN_smoketest_result "0" "User '${TEST_USER}' tested."

