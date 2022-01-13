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
# Description:  Test for GENIVI Audio Manager including ALSA Routing Adapter
# Group:        audio,audiomanager,alsa
# Arch NOK:
# Board OK:
# Board NOK:
# Reset:        0
# Timeout:      75
# Check:        audio-play
# Host-Pre:
# Host-Post:
#
################################################################################

DIR=$(dirname $0)
source "${DIR}/smoketest.func" || exit 1

# initialize smoketest
FN_smoketest_init || FN_smoketest_error "Smoketest init failed."

# load dlt functionality
FN_smoketest_use "dlt" || FN_smoketest_error "Use dlt functionality failed."

# load audio functionality
FN_smoketest_use "audio" || FN_smoketest_error "Use audio functionality failed."

# define test variables
TEST_CMD="CommandPlugInTest"
TEST_CONF="${DIR}/amra_asound_test.conf"

################################################################################
# setup system

echo ${SMOKETEST_DELIMITER}
echo "Setup system for smoketest ...."

FN_execute "cp ${TEST_CONF} ${SMOKETEST_ALSA_TEST_CONFIG}"
FN_start_service "audiomanager.service"
FN_execute "eval echo \"ibase=16; 0000000000000000\" | aplay -Dvdev_app_mediaplayer -fdat -q"
FN_execute "eval echo \"ibase=16; 0000000000000000\" | aplay -Dvdev_app_interrupt -fdat -q"

################################################################################
# test

echo ${SMOKETEST_DELIMITER}
echo "Start smoketest ...."

FN_audio_start_record
FN_execute "CommandPlugInTest"
FN_audio_stop_record
FN_smoketest_result "0"

