# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Userland makefile
#
# Description: This file calls sub makefiles for all sections of the userland.
# -----------------------------------------------------------------------------

BASE_DIR      = ..
UL_NAME       = $(notdir $(shell pwd))

SECTIONS      = platform \
                test

# -----------------------------------------------------------------------------
# Include all global settings and functions
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default.mk

# -----------------------------------------------------------------------------
# Userland specific rules and functions
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_ul.mk
