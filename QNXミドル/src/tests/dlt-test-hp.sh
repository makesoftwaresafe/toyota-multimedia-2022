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
# Description:  This test checks dlt hp log functionality
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

function CleanTest()
{
    /bin/rm -r /tmp/log
    /bin/rm /tmp/hplogtest.log
}

function SetupTest()
{
    echo "Check if HP log test application and Control application is present"

    echo "Check if DLT daemon is running"
    /bin/pidof dlt-daemon >/dev/null
    if [[ $? -ne 0 ]] ; then
        echo "DLT daemon not running starting now"
        /usr/bin/dlt-daemon -d
        sleep 4
    fi

    echo "Create directory /tmp/log/dlt/hp"
    /bin/mkdir -p /tmp/log/dlt/hp

    echo "Setting up buffer values"
    export "DLT_NW_TRACE_HP_CFG"=4
}

function RunTest()
{
    echo "Execute HP log application"
    /usr/bin/dlt-test-hp &
    sleep 1

    echo "Get snapshot"
    /usr/bin/dlt-get-hplog -i -t DLHP,TEST >> /tmp/hplogtest.log
    sleep 2

}

function VerifyTest()
{
    echo "Check if DLT file is created"
    file="/tmp/log/dlt/dlt_IDisTEST_IDisDLHP.dlt"
    if [ -f "$file" ]
    then
        echo "$file found : test passed"
    else
        echo "$file not found test failed"
    fi
}

SetupTest

RunTest

VerifyTest

CleanTest
