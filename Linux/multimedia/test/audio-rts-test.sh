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
# Description:  This test checks audio rts
# Group:        audio,audio-rts
# Arch NOK:     
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

# load audio functionality
FN_smoketest_use "audio" || FN_smoketest_error "Use audio functionality failed."

# init alsa test config
FN_audio_alsa_init_test_config || FN_smoketest_error "No alsa test configuration."

# define test variables
TEST_CMD="rtsd"
TEST_COUNT="10"
TEST_DEV="${DIR}/rts_smoketest_devices.csv"
TEST_DEV_LAZY="${DIR}/rts_smoketest_devices_lazy.csv"
TEST_PREFILL="5"       # default for real time capable audio streams
TEST_PREFILL_LAZY="96" # lazy setting for NON real time capable audio streams (e.g. VBOX)
TEST_USER="test-adit-multimedia-audio"

export RTS_LOGLEVEL="2"
export RTS_FEAT_INJECT_XRUN="0"
export RTS_PLAYBACK_PREFILL="${TEST_PREFILL}"

# use lazy settings on vbox
if [ "${SMOKETEST_PRODUCT}" == "vbox" ] || [ "${SMOKETEST_PRODUCT}" == "sim" ]; then
  TEST_DEV=${TEST_DEV_LAZY}
  export RTS_PLAYBACK_PREFILL="${TEST_PREFILL_LAZY}"
  export RTS_EXTRA_TIMEOUT=100
fi

################################################################################
# re-define extra cleanup which will be automatically called on normal cleanup

function FN_smoketest_cleanup_extra()
{
  echo "  ${FUNCNAME} ..."

  # if the test command is still running - kill it
  killall --quiet -9 ${TEST_CMD} &>/dev/null || true

  echo "  ${FUNCNAME} done."
}

################################################################################

# cleanup before starting
FN_smoketest_cleanup_extra

FN_check_user "${TEST_USER}"
FN_user_execute "${TEST_USER}" "${TEST_CMD} ${TEST_DEV_LAZY}" "1"
TEST_PID="${SMOKETEST_EXTRA_PID}"

echo "RTSD started with PID '${TEST_PID}' -- periodic output should show 0 xruns ..."
while [ ${TEST_COUNT} -gt 0 ]; do
  sleep 2
  kill -USR1 ${TEST_PID} || FN_smoketest_error "rtsd ended unexpectedly."
  TEST_COUNT=$(expr ${TEST_COUNT} - 1)
done

#rtsd must still exist - otherwise error
FN_check_pid "${TEST_PID}"
FN_kill_pid "${TEST_PID}" "INT"
FN_smoketest_result "$?" "User '${TEST_USER}' tested."

