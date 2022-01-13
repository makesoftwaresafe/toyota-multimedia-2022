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
# Description:  This test checks aoap-discovery device detection 
# Group:        smartphone,aoap
# Arch NOK:     
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      90
# Check:        
# Host-Pre:     
# Host-Post:    
#
################################################################################
#
# Expected result:
#   AOAP capable devices (USB) detected and possibly switched to AOAP mode.
#
#   Device, like a mobile device (e.g. Android Device) shall be detected and switched.
#
# exit codes:
#   0 - Test Passed
#   1 - Test Failed (tbd reason ?)
#
#  Requirements:
#  - Android Phones with Android version 5.1 and higher
################################################################################

DIR=$(dirname $0)
source "${DIR}/smoketest.func" || exit 1


# init smoketest
FN_smoketest_init || FN_smoketest_error "Smoketest init failed."

# load dlt functionality
FN_smoketest_use "dlt" || FN_smoketest_error "Use dlt functionality failed."

# define test variables
TEST_CFGFILE=
TEST_TIMEOUT="40"
TEST_DUMP_DELAY="5"
TEST_CMD="aoap_test"
TEST_CMD_OPT="$@ -t ${TEST_TIMEOUT} 2>&1"
TEST_CMD_BLACKLIST="-b 0951 -b 0cf3 -b 0b05"
TEST_CMD_ADD="${TEST_CMD_BLACKLIST} -m Android -s 000000123456789"
TEST_PID=""

################################################################################

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

echo "---"
echo "Check for connected USB devices:"
lsusb
echo "---"

# execute test
FN_execute "eval ${TEST_CMD} ${TEST_CMD_ADD:-} ${TEST_CMD_OPT}" "1"
TEST_PID="${SMOKETEST_EXTRA_PID}"

wait ${TEST_PID}
TEST_RETURN="$?"

FN_smoketest_result "${TEST_RETURN}"
