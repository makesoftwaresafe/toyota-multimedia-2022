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
# Description:  This test checks Audio Backend Alsa
# Group:        audio,alsa
# Arch NOK:     
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      150
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
TEST_CMD="audioif_smoketest"
TEST_DEV="${DIR}/abalsa_smoketest_devices.csv"
TEST_USER="test-adit-multimedia-audio"

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
FN_user_execute "${TEST_USER}" "${TEST_CMD} ${TEST_DEV}"
FN_smoketest_result "0" "User '${TEST_USER}' tested."

