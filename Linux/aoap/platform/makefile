# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Section makefile
#
# Description: This file calls sub makefiles for all components of the section. 
# -----------------------------------------------------------------------------

BASE_DIR      = ../..
SECTION_NAME  = $(notdir $(shell pwd))

COMP_DEP1     = aoap
COMP_DEP2     = aoap-discovery
COMP_DEP3     = 

COMPONENTS    = $(COMP_DEP1) $(COMP_DEP2) $(COMP_DEP3)

# -----------------------------------------------------------------------------
# Include all global settings and functions
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default.mk

# -----------------------------------------------------------------------------
# Section specific rules and functions
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_sec.mk

$(COMP_DEP1): $(COMP_DEP2)

$(COMP_DEP2): $(COMP_DEP3)
