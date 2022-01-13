#!/bin/bash

################################################################################
# This source code is proprietary of ADIT
# Copyright (C) Advanced Driver Information Technology GmbH
# All rights reserved
################################################################################

DIR=$(dirname $0)
source "${DIR}/smoketest.func" || exit 1
source "${DIR}/ipod-auth.func" || FN_smoketest_error "Sourcing '${DIR}/ipod-auth.func' failed."

# initialize smoketest
FN_smoketest_init || FN_smoketest_error "Smoketest init failed."

# load audio functionality
if test "$1" != "host"; then
    FN_smoketest_use "audio" || FN_smoketest_error "Use audio functionality failed."
else
    echo "iAP2 smoketest with hostmode doesn't output sound."
fi

# load dlt functionality
FN_smoketest_use "dlt" || FN_smoketest_error "Use dlt functionality failed."

# define test variables
TEST_ROLE="${1:-gadget}"
TEST_USER="${2:-root}"

################################################################################

# initialize ipod environment
FN_ipod_startup || FN_smoketest_error "IPOD startup failed."

FN_check_user "${TEST_USER}"

if test "${TEST_ROLE}" != "host"; then
    FN_audio_start_record "${TEST_USER}"
fi

if test "${TEST_ROLE}" == "host"; then
    FN_smoketest_exclude "family" "oracle-virtualbox"
fi

# launch iap2 smoketest
FN_user_execute "${TEST_USER}" "ipod-iap2_smoketest.out ${TEST_ROLE} ai nopo ${TEST_USER} ${TEST_USER}"

if test "${TEST_ROLE}" != "host"; then
    FN_audio_stop_record
fi

# finalize ipod environment
FN_ipod_exit

FN_smoketest_result "0" "User '${TEST_USER}' tested."

