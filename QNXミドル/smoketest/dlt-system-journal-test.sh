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
# Description:  This test checks dlt journal
# Group:        base,dlt
# Arch NOK:     
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      15
# Check:        text
# Host-Pre:     
# Host-Post:    
#
################################################################################

DIR=$(dirname $0)
source "${DIR}/smoketest.func" || exit 1

FN_smoketest_init
FN_stop_service "dlt.service"
FN_start_service "dlt.service"
FN_start_service "dlt-system.service"

# script for sending syslog journald message with all priorities (except 'debug') to DLT

DLT_CONVERT_CMD="dlt-convert"
DLT_RECEIVE_CMD="dlt-receive"

JOURNALD_FILTER="${SMOKETEST_TMP_FILE}.filter"
JOURNALD_TXT="${SMOKETEST_TMP_FILE}.txt"
JOURNALD_LOG="${SMOKETEST_TMP_FILE}.dlt"

TARGET_IP=127.0.0.1

################################################################################

function CleanTest()
{
  killall --quiet ${DLT_RECEIVE_CMD} 2>&1 >/dev/null
  # rm -f ${JOURNALD_FILTER} ${JOURNALD_LOG}

  if [ "${1}" != "" ]; then
    FN_smoketest_error "${1}"
  fi
}

################################################################################

# set filter for dlt
echo 'SYS JOUR' > ${JOURNALD_FILTER}

################################################################################

CleanTest

# start dlt receive
${DLT_RECEIVE_CMD} ${TARGET_IP} -a -f ${JOURNALD_FILTER} -o ${JOURNALD_LOG} >/dev/null &
DLT_RECEIVE_CMD_PID=$!

# to be sure receive is started
while [ ! -d /proc/${DLT_RECEIVE_CMD_PID} ]; do
  sleep 1
done

# send messages
echo "sending journald messages ..."
LIST="emerg alert crit err warn notice info"
for VALUE in ${LIST}; do
	logger -p local0.${VALUE} "dlt journald test ${VALUE}" || CleanTest "Log journal value '${VALUE}' failed."
done

# wait some time so that journal can be written
sleep 2

# get journal messages via dlt
${DLT_CONVERT_CMD} -a ${JOURNALD_LOG} > ${JOURNALD_TXT} || CleanTest "${DLT_CONVERT_CMD} failed."

# check values
for VALUE in ${LIST}; do
	grep -q -i "journald test ${VALUE}" ${JOURNALD_TXT} || CleanTest "Journal value '${VALUE}' not found."
done

CleanTest

################################################################################

FN_smoketest_result "0"

