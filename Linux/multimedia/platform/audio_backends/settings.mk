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

COMPONENT     = AUDIO_BACKEND
COMP_NAME     = platform/audio_backends

# -----------------------------------------------------------------------------
# Base Directory
# -----------------------------------------------------------------------------

BASE_DIR      = $(COMP_DIR)/../../..

# -----------------------------------------------------------------------------
# Component Settings
# -----------------------------------------------------------------------------

C_FLAGS       = -fpic
CPP_FLAGS     = -fpic
LD_FLAGS      = -Wl,--rpath-link=$(OE_STAGING_FOLDER)/usr/lib
#SO_FLAGS      = -Wl,--unresolved-symbols=report-all
SO_FLAGS      =

C_DEFINES     =  -DPIC
C_DEFINES    += 
CPP_DEFINES   = $(C_DEFINES)


# -----------------------------------------------------------------------------
# Include all global settings
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default.mk
