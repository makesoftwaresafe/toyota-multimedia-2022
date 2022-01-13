# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Component Globals default makefile 'settings.mk'
#
# Description: This is the default makefile which contains
#              all basic settings needed for the component.
# -----------------------------------------------------------------------------
# Requirement: COMP_DIR, SUBCOMP_NAME
# -----------------------------------------------------------------------------

COMPONENT     = GST_VIV_DEMO
COMP_NAME     = demo/gst_viv_demo

# -----------------------------------------------------------------------------
# Base Directory
# -----------------------------------------------------------------------------

BASE_DIR      = $(COMP_DIR)/../../..

# -----------------------------------------------------------------------------
# Component Settings
# -----------------------------------------------------------------------------

C_FLAGS       = -fpic
CPP_FLAGS     =
LD_FLAGS      =
SO_FLAGS      =

C_DEFINES     =  -DPIC
CPP_DEFINES   = $(C_DEFINES)


# -----------------------------------------------------------------------------
# Include all global settings
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default.mk
