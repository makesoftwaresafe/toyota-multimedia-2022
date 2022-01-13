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
# Description:  This test checks dlt logstorage
# Group:        base,dlt,usb
# Arch NOK:     
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      9
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
#FN_start_service "dlt-system.service"

MAX_WAIT_TIME=10 # max wait time in seconds for device

DLT_LOG_MESSAGES_PATTERN="Test.*.dlt"	# pattern
DLT_LOG_MESSAGES_PATTERN_NONVERBOSE="Nonverbose.*.dlt"
DLT_MESSAGE_TEXT="DLT Log Storage Test 10"
DLT_MESSAGE_COUNT=20

TEST_APP=dlt-test-logstorage

DLT_LOG_CONFIG_FILE=dlt_logstorage.conf

################################################################################

function InitTest()
{
	echo "DLT Log storage Test:"; echo
	echo "* app '$TEST_APP' will be started"
	echo "    > Plug in USB stick (with $DLT_LOG_CONFIG_FILE file present in top folder) into Target"
	echo "    > copied from 'dlt-daemon.git/smoketest/$DLT_LOG_CONFIG_FILE'"; echo ; echo

	TMP=$(find /media -name "${DLT_LOG_CONFIG_FILE}" -maxdepth 2 -mindepth 2 -type f | head -n 1)
	if [ -z "${TMP}" ]; then
		FN_smoketest_error "No media containing '${DLT_LOG_CONFIG_FILE}' found"
	fi
	DLT_LOGSTORAGE_DIR=$(dirname "${TMP}")

	if [ ! -e ${DLT_LOGSTORAGE_DIR} ]; then
		CleanTest "Log storage directory '${DLT_LOGSTORAGE_DIR}' does not exist. Please insert USB mass storage with '${DLT_LOG_CONFIG_FILE}' file."
	fi
	if [ ! -f ${DLT_LOGSTORAGE_DIR}/${DLT_LOG_CONFIG_FILE} ]; then
		CleanTest "DLT config file does not exist. Please insert USB mass storage with '${DLT_LOG_CONFIG_FILE}' file in top folder."
	fi

    FN_start_service dlt-logstorage-ctrl
	sleep 1
}

################################################################################                               
                                                                                                               
function CleanOldLogs()                                                                                        
{                                                                                                              

        # clean old content of USB stick                                                                       
        rm -f ${DLT_LOGSTORAGE_DIR}/*.dlt                                                                          
        echo; echo "Content of ${DLT_LOGSTORAGE_DIR}:"                                                             
        ls ${DLT_LOGSTORAGE_DIR}                                                                                   
	echo
                                                                                                               
}                                                                                                              

################################################################################

function ExecuteTest()
{
	#CleanOldLogs


	echo; echo "Running $TEST_APP ..."
	$TEST_APP > /tmp/$(basename $0).log || CleanTest "DLT test log storage failed."
	sleep 1

	ls ${DLT_LOGSTORAGE_DIR}/${DLT_LOG_MESSAGES_PATTERN} >/dev/null
	if [ "$?" != "0" ]; then
		ls -l ${DLT_LOGSTORAGE_DIR}
		CleanTest "DLT storage files '${DLT_LOGSTORAGE_DIR}/${DLT_LOG_MESSAGES_PATTERN}' do not exist."
	fi

	SUM_COUNT=0 	# counting each Test.1.dlt Test.2.dlt file
	for F in ${DLT_LOGSTORAGE_DIR}/${DLT_LOG_MESSAGES_PATTERN}; do
		MY_COUNT=$( dlt-convert ${F} -c | sed -e "s/^Total.*: //" )
		SUM_COUNT=$(expr ${SUM_COUNT} + ${MY_COUNT} )
	done
	echo "DLT messages count is ${SUM_COUNT} shall ${DLT_MESSAGE_COUNT}"

	if [[ $SUM_COUNT -lt ${DLT_MESSAGE_COUNT} ]]; then
		CleanTest "Log file '${DLT_LOG_MESSAGES_PATTERN}' does not contain $DLT_MESSAGE_COUNT messages"
	fi

	echo; echo "test content of DLT messages"
	dlt-convert ${DLT_LOGSTORAGE_DIR}/${DLT_LOG_MESSAGES_PATTERN} -a | grep -q "$DLT_MESSAGE_TEXT"
	if [ "$?" != "0" ]; then
		CleanTest "Log file '${DLT_LOG_MESSAGES_PATTERN}' does not contain '$DLT_MESSAGE_TEXT' content"
        fi

	echo
}

################################################################################

################################################################################

function ExecuteTestNonVerbose()
{
	echo; echo "Running NONVERBOSE Test application..."

	dlt-test-non-verbose -l > /tmp/$(basename $0).log || CleanTest "DLT test log storage non verbose failed."
	sleep 1

	# Check only if file exists, in non-verbose logging its not possible to predict number of messages
	ls ${DLT_LOGSTORAGE_DIR}/${DLT_LOG_MESSAGES_PATTERN_NONVERBOSE} >/dev/null
	if [ "$?" != "0" ]; then
		ls -l ${DLT_LOGSTORAGE_DIR}
		CleanTest "DLT storage files '${DLT_LOGSTORAGE_DIR}/${DLT_LOG_MESSAGES_PATTERN_NONVERBOSE}' do not exist."
	fi

	echo
}

################################################################################
function CleanTest()
{
	
	if [ "${1}" != "" ]; then
		FN_smoketest_error "${1}"
	fi


	CleanOldLogs # in case: passed


}

################################################################################

InitTest
ExecuteTestNonVerbose
ExecuteTest
CleanTest

FN_smoketest_result "0"
	
