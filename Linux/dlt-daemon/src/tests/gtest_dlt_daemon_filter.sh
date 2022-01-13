#!/bin/sh
########################################################################################
#file            : gtest_dlt_daemon_filter.sh.sh
#
#Description     : filter unit test preparation
#
#Author Name     : Onkar Palkar
#Email Id        : onkar.palkar@wipro.com
#
#History         : Last modified date : 05/12/2016
########################################################################################
#
# Function:    -cleanup()
#
# Description    -Delete dlt_message_filter.conf file from tmp folder if present
#
# Return    -Zero on success
#        -Non zero on failure
#

cleanup()
{
    tmpPath=/tmp
    cd $tmpPath
    rm -rf $tmpPath/dlt_message_filter.conf
    return 0
}

#
# Function:     -setupTest()
#
# Description   -create dlt_message_filter.conf file
#
# Return        -Zero on success
#               -Non zero on failure
#
setupTest()
{
    touch $tmpPath/dlt_message_filter.conf
    if [ $? -eq '1' ]
    then
        echo "Error in creating dlt_message_filter.conf file"
        return 1
    fi
        echo "[General]" >>$tmpPath/dlt_message_filter.conf
        echo "Name = Genivi-LogMode" >>$tmpPath/dlt_message_filter.conf
        echo "DefaultLevel = 100" >>$tmpPath/dlt_message_filter.conf
        echo "[Filter1]" >>$tmpPath/dlt_message_filter.conf
        echo "Name            = Off" >>$tmpPath/dlt_message_filter.conf
        echo "Level           = 0" >>$tmpPath/dlt_message_filter.conf
        echo "Clients         = NONE" >>$tmpPath/dlt_message_filter.conf
        echo "ControlMessages = NONE" >>$tmpPath/dlt_message_filter.conf
        echo "Injections      = NONE" >>$tmpPath/dlt_message_filter.conf
        echo "[Filter2]" >>$tmpPath/dlt_message_filter.conf
        echo "Name            = Internal" >>$tmpPath/dlt_message_filter.conf
        echo "Level           = 25" >>$tmpPath/dlt_message_filter.conf
        echo "Clients         = TRACE, LOGSTORAGE" >>$tmpPath/dlt_message_filter.conf
        echo "ControlMessages = NONE" >>$tmpPath/dlt_message_filter.conf
        echo "Injections      = NONE" >>$tmpPath/dlt_message_filter.conf
        echo "[Filter3]" >>$tmpPath/dlt_message_filter.conf
        echo "Name            = External" >>$tmpPath/dlt_message_filter.conf
        echo "Level           = 50" >>$tmpPath/dlt_message_filter.conf
        echo "Clients         = Serial, TCP" >>$tmpPath/dlt_message_filter.conf
        echo "ControlMessages = *" >>$tmpPath/dlt_message_filter.conf
        echo "Injections      = *" >>$tmpPath/dlt_message_filter.conf
        echo "[Filter4]" >>$tmpPath/dlt_message_filter.conf
        echo "Name            = Both" >>$tmpPath/dlt_message_filter.conf
        echo "Level           = 100" >>$tmpPath/dlt_message_filter.conf
        echo "Clients         = *" >>$tmpPath/dlt_message_filter.conf
        echo "ControlMessages = *" >>$tmpPath/dlt_message_filter.conf
        echo "Injections      = *" >>$tmpPath/dlt_message_filter.conf
    return 0
}
#main function
########################################################################################
cleanup
setupTest
