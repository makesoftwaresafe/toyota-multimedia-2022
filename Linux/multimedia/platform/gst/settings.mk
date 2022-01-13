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

COMPONENT     = GST_PLUGINS
COMP_NAME     = platform/gst

NO_TRACELIB_DEPENDENCY += $(COMPONENT)

# -----------------------------------------------------------------------------
# Base Directory
# -----------------------------------------------------------------------------

BASE_DIR      = $(COMP_DIR)/../../..

# -----------------------------------------------------------------------------
# Component Settings
# -----------------------------------------------------------------------------

C_FLAGS       = -fpic
CPP_FLAGS     =
LD_FLAGS      = -Wl,--rpath-link=$(OE_STAGING_FOLDER)/usr/lib
SO_FLAGS      =

C_DEFINES     =  -DPIC
C_DEFINES    += 
CPP_DEFINES   = $(C_DEFINES)


# -----------------------------------------------------------------------------
# Include all global settings
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default.mk
