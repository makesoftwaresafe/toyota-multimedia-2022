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
# Description:  This test checks the JACK sound server
# Group:        audio,alsa,jack
# Arch NOK:     
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      35
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

################################################################################
# re-define extra cleanup which will be automatically called on normal cleanup

function FN_smoketest_cleanup_extra()
{
  echo "  ${FUNCNAME} ..."

  # TODO only execute on failure
  FN_service_status "jack.service"

  # remove alsa configuration for jack to restore default behaviour
  FN_stop_service "jack.service"
  FN_execute "rm ${SMOKETEST_ALSA_TEST_CONFIG}"

  echo "  ${FUNCNAME} done."
}

################################################################################

TEST_CONF="${DIR}/jack_asound_test.conf"
FN_execute "cp ${TEST_CONF} ${SMOKETEST_ALSA_TEST_CONFIG}"

FN_start_service "jack.service"

# use system wide jack server
export JACK_PROMISCUOUS_SERVER=1

# Allow memory locking and real-time priority
ulimit -r99 -l100000

# wait for jack server startup
# TODO remove when jack server uses sd_notify
FN_execute "jack_wait --wait --timeout 2"


# execute mixer smoketest with jack as mixer
source "${DIR}/audio-mixer.func" || exit 1

FN_smoketest_result "0" "User '${TEST_USER}' tested."

