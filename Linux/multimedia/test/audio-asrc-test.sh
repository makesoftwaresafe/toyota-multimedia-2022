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
# Description:  This test checks audio asrc
# Group:        audio,alsa,alsa-asrc
# Arch NOK:     x86
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

grep -q "pcm.smoketest_asrc_playback" ${SMOKETEST_ALSA_TEST_CONFIG}
if [ "$?" != "0" ]; then
  echo -e "pcm.smoketest_asrc_playback{
	type arate
	slave {
		pcm entertainment_main
		rate 48000
	}
	converter asrc_imx
	converter_cfg {
		in_clk \"NONE\"
		out_clk \"HiFi\"
		pair_mask 0x02
		need_slaveclk 1
	}
} ">> ${SMOKETEST_ALSA_TEST_CONFIG}
fi

grep -q "pcm.smoketest_asrc_capture" ${SMOKETEST_ALSA_TEST_CONFIG}
if [ "$?" != "0" ]; then
  echo -e "pcm.smoketest_asrc_capture{
	type arate
	slave {
		pcm line_in
		rate 48000
	}
	converter asrc_imx
	converter_cfg {
		in_clk \"HiFi\"
		out_clk \"ASRCK1\"
		pair_mask 0x02
		need_slaveclk 1
	}
}" >> ${SMOKETEST_ALSA_TEST_CONFIG}
fi

echo ${SMOKETEST_DELIMITER}
echo "Using alsa test configuration '${SMOKETEST_ALSA_TEST_CONFIG}':"
cat ${SMOKETEST_ALSA_TEST_CONFIG}
echo ${SMOKETEST_DELIMITER}

################################################################################

FN_check_user "${TEST_USER}"
FN_audio_start_record "${TEST_USER}"
FN_user_execute "${TEST_USER}" "aplay -v -Dsmoketest_asrc_playback ${TEST_WAV}"
FN_audio_stop_record
FN_smoketest_result "0" "User '${TEST_USER}' tested."

