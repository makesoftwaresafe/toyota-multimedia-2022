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

COMPONENT     = T_AOAP
COMP_NAME     = test/aoap

# -----------------------------------------------------------------------------
# Base Directory
# -----------------------------------------------------------------------------

BASE_DIR      = $(COMP_DIR)/../../..
PROJECT_DIR   = $(COMP_DIR)/../..

# -----------------------------------------------------------------------------
# Include all global settings
# -----------------------------------------------------------------------------

# deactivate linkage of trace
NO_TRACELIB_DEPENDENCY += $(COMPONENT)

include $(BASE_DIR)/tools/config/mk/default.mk

# -----------------------------------------------------------------------------
# Component Settings
# -----------------------------------------------------------------------------

