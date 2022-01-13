#!/bin/bash

################################################################################
# This source code is proprietary of ADIT
# Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
# All rights reserved
################################################################################

################################################################################
#
# --- AUTOMATED SMOKETEST ---
# 
# Description:  This test checks alsa basic functions
# Group:        audio,alsa,alsa-basic
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

# define test variables
TEST_USER="test-adit-multimedia-audio"
TEST_WAV="/opt/platform/unit_tests/audio8k16S.wav"

################################################################################

FN_check_user "${TEST_USER}"
FN_audio_start_record
FN_user_execute "${TEST_USER}" "aplay -v -Dentertainment_main ${TEST_WAV}"
FN_audio_stop_record
FN_smoketest_result "0" "User '${TEST_USER}' tested."

