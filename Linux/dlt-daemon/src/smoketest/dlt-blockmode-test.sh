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
# Description:  This test checks dlt blockmode
# Group:        base,dlt
# Arch NOK:     
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      30
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

# application for setting and unsetting DLT block mode

DLT_CONVERT_CMD="dlt-convert"
DLT_CLIENT_CMD="dlt-receive"
DLT_BLOCKMODE_CMD="dlt-blockmode-ctrl"

LOGFILE="${SMOKETEST_TMP_FILE}.log"
JOURNALD_FILTER="${SMOKETEST_TMP_FILE}.filter"
JOURNALD_TXT="${SMOKETEST_TMP_FILE}.txt"
JOURNALD_LOG="${SMOKETEST_TMP_FILE}.dlt"


################################################################################

function CleanTest()
{
  if [ "${1}" != "" ]; then
    FN_smoketest_error "${1}"
  fi
}

function RequestClient()
{
    MODE=${1}

    if [ $MODE = "start" ]
    then
        ${DLT_CLIENT_CMD} -a localhost -B & >${LOGFILE} 2>&1 || CleanTest "Starting Client failed."
    else
        killall ${DLT_CLIENT_CMD} || CleanTest "Stopping Client failed."
    fi
}

function SetDLTBlockmode()
{
    MODE=${1}

    echo "Setting DLT to Block Mode = ${MODE} "
    ${DLT_BLOCKMODE_CMD} ${MODE} >${LOGFILE} 2>&1 || CleanTest "Set block mode ${MODE} failed."
    sleep 4

    ${DLT_BLOCKMODE_CMD} -s >${LOGFILE} 2>&1 || CleanTest "Get block mode failed."
    sleep 1

    if [ $MODE = "-e" ]
    then
        grep -e "Daemon BlockMode status: BLOCKING" -q ${LOGFILE} || CleanTest "Search for Block Mode = 1 failed."
    else
        grep -e "Daemon BlockMode status: NON-BLOCKING" -q ${LOGFILE} || CleanTest "Search for Block Mode = 0 failed."
    fi
}

################################################################################

#echo 'SYS JOUR' > ${JOURNALD_FILTER}
echo '' > ${JOURNALD_FILTER}

################################################################################

CleanTest

sleep 1

echo "Starting Client"
RequestClient start
sleep 4 # to be sure receive is started

echo "Setting to Block mode"
SetDLTBlockmode -e

# echo "Setting back to Non Block mode"
SetDLTBlockmode -d

echo "Stopping Client"
RequestClient stop
sleep 2 # to be sure receive is stopped

CleanTest

FN_smoketest_result "0"

