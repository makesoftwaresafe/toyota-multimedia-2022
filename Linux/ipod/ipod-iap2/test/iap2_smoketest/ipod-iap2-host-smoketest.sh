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
# Description:  This test checks iap2 for host mode
# Group:        smartphone,iap2
# Arch NOK:     
# Board OK:     
# Board NOK:    
# Reset:        0
# Timeout:      75
# Check:        text
# Host-Pre:     
# Host-Post:    
#
################################################################################

DIR=$(dirname $0)
source "${DIR}/ipod-iap2-smoketest.sh" host
