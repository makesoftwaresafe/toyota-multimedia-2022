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
# Description:  This test checks audio alsa source
# Group:        audio,alsa,alsa-rate
# Arch NOK:
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      15
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
TEST_USER="test-adit-multimedia-audio"
TEST_WAV="/opt/platform/unit_tests/audio8k16S.wav"

################################################################################
# patch alsa test configuration if needed

grep -q "pcm.smoketest_src_linear_playback" ${SMOKETEST_ALSA_TEST_CONFIG}
if [ "$?" != "0" ]; then
  echo -e "pcm.smoketest_src_linear_playback {
	type rate
	slave {
		pcm entertainment_main
		rate 48000
	}
	converter linear
}" >> ${SMOKETEST_ALSA_TEST_CONFIG}
fi

grep -q "pcm.smoketest_src_adit_swsrc_playback" ${SMOKETEST_ALSA_TEST_CONFIG}
if [ "$?" != "0" ]; then
  echo -e "pcm.smoketest_src_adit_swsrc_playback {
	type rate
	slave {
		pcm entertainment_main
		rate 48000
	}
	converter adit_swsrc
}" >> ${SMOKETEST_ALSA_TEST_CONFIG}
fi

echo ${SMOKETEST_DELIMITER}
echo "Using alsa test configuration '${SMOKETEST_ALSA_TEST_CONFIG}':"
cat ${SMOKETEST_ALSA_TEST_CONFIG}
echo ${SMOKETEST_DELIMITER}

################################################################################

FN_check_user "${TEST_USER}"
FN_audio_start_record
FN_user_execute "${TEST_USER}" "aplay -v -Dsmoketest_src_linear_playback ${TEST_WAV}"
FN_user_execute "${TEST_USER}" "aplay -v -Dsmoketest_src_adit_swsrc_playback $TEST_WAV"
FN_audio_stop_record

FN_smoketest_result "0" "User '${TEST_USER}' tested."

