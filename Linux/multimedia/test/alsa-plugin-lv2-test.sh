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
# Description:  This test checks alsa plugin lv2
# Group:        audio,alsa,alsa-lv2
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
TEST_USER="test-adit-multimedia-audio"
TEST_WAV="/opt/platform/unit_tests/audio8k16S.wav"

################################################################################
# patch alsa test configuration if needed

grep -q "pcm.lv2_smoketest" ${SMOKETEST_ALSA_TEST_CONFIG}
if [ "$?" != "0" ]; then
  echo -e "pcm.lv2_smoketest{
	type lfloat
        slave.format FLOAT
	slave.pcm {
	type lv2
	uri "http://lv2plug.in/plugins/eg-amp"
	slave.pcm entertainment_main
	}
}" >> ${SMOKETEST_ALSA_TEST_CONFIG}
fi

echo ${SMOKETEST_DELIMITER}
echo "Using alsa test configuration '${SMOKETEST_ALSA_TEST_CONFIG}':"
cat ${SMOKETEST_ALSA_TEST_CONFIG}
echo ${SMOKETEST_DELIMITER}

################################################################################

FN_check_user "${TEST_USER}"
FN_audio_start_record
FN_user_execute "${TEST_USER}" "aplay -v -Dlv2_smoketest ${TEST_WAV}"
FN_audio_stop_record
FN_smoketest_result "0" "User '${TEST_USER}' tested."

