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
# Description:  This test checks alsa plugin loop
# Group:        audio,alsa,alsa-loop
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

# init alsa test config
FN_audio_alsa_init_test_config || FN_smoketest_error "No alsa test configuration."

# define test variables
TEST_CMD="rtsd"
TEST_DEV="${DIR}/alsa_loop_smoketest_devices.csv"
TEST_USER1="test-adit-multimedia-audio"
TEST_USER2="test-adit-multimedia"
TEST_WAV="/opt/platform/unit_tests/audio8k16S.wav"
TEST_PREFILL_DEFAULT="4" # default for real time capable audio streams
TEST_PREFILL_LAZY="16"   # lazy setting for NON real time capable audio streams (e.g. VBOX)
TEST_PID=""

export RTS_LOGLEVEL="1"
export RTS_PLAYBACK_PREFILL="${TEST_PREFILL_DEFAULT}"

# use lazy settings on vbox
if [ "${SMOKETEST_PRODUCT}" == "vbox" ] || [ "${SMOKETEST_PRODUCT}" == "sim" ]; then
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
# patch alsa test configuration if needed

grep -q "pcm.loop_smoketest_wr" ${SMOKETEST_ALSA_TEST_CONFIG}
if [ "$?" != "0" ]; then
  echo -e "pcm.loop_smoketest_wr {
	type loop
	ipc_perm 0660
	ipc_gid 29
	device 0
	subdevice 0
	format S16_LE
	rate 8000
	channels 2
	buffer_size 192
	period_size 64 
	force_reset 1
}" >> ${SMOKETEST_ALSA_TEST_CONFIG}
fi

grep -q -F "pcm.loop_smoketest_rd" ${SMOKETEST_ALSA_TEST_CONFIG}
if [ "$?" != "0" ]; then
  echo -e "pcm.loop_smoketest_rd {
	type loop
	ipc_perm 0660
	ipc_gid 29
	device 1
	subdevice 0
	format S16_LE
	rate 8000
	channels 2
	buffer_size 192
	period_size 64
	force_reset 1
}" >> ${SMOKETEST_ALSA_TEST_CONFIG}
fi

grep -q "pcm.loop_smoketest_fwd" ${SMOKETEST_ALSA_TEST_CONFIG}
if [ "$?" != "0" ]; then
  echo -e "pcm.loop_smoketest_fwd {
	type empty
	slave.pcm entertainment_main	
}" >> ${SMOKETEST_ALSA_TEST_CONFIG}
fi  

echo ${SMOKETEST_DELIMITER}
echo "Using alsa test configuration '${SMOKETEST_ALSA_TEST_CONFIG}':"
cat ${SMOKETEST_ALSA_TEST_CONFIG}
echo ${SMOKETEST_DELIMITER}

################################################################################

# cleanup before starting
FN_smoketest_cleanup_extra

FN_check_user "${TEST_USER1}"
FN_check_user "${TEST_USER2}"

FN_audio_start_record
FN_user_execute "${TEST_USER1}" "${TEST_CMD} ${TEST_DEV}" "1"
TEST_PID="${SMOKETEST_EXTRA_PID}"

FN_user_execute "${TEST_USER2}" "aplay -v -N -Dloop_smoketest_wr -c2 -r8000 -fS16_LE ${TEST_WAV}"

wait ${TEST_PID}
if [ "$?" != "0" ]; then
  FN_smoketest_error "Test '${TEST_CMD} ${TEST_DEV}' failed." 
fi

FN_audio_stop_record
FN_smoketest_result "0" "User '${TEST_USER1}' and '${TEST_USER2}' tested."

