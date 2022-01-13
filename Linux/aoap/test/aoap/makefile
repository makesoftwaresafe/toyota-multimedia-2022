# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Component makefile
#
# Description: This file calls sub makefiles for all subcomponents.
# -----------------------------------------------------------------------------

COMP_DIR      = .

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Components
# -----------------------------------------------------------------------------
# start ONLY HERE dynamic evaluation of tooling settings!

SUBCOMP_DEP1  = aoap_test
SUBCOMP_DEP2  = aoap-device-detection
SUBCOMP_DEP3  =

SUBCOMPONENTS = $(SUBCOMP_DEP1) $(SUBCOMP_DEP2) $(SUBCOMP_DEP3) 
# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

.PHONY: $(PHONY) $(SUBCOMPONENTS)

all: $(SUBCOMPONENTS)

$(SUBCOMP_DEP1): $(SUBCOMP_DEP2)

$(SUBCOMP_DEP2): $(SUBCOMP_DEP3)

$(SUBCOMPONENTS):
	$(MAKE) $(MAKECMDGOALS) GOAL=$(GOAL) -C $@

install:
	@$(call comp_$(MAKECMDGOALS),$(SUBCOMPONENTS))

clean \
help \
info:
	-@$(call comp_$(MAKECMDGOALS),$(SUBCOMPONENTS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_comp.mk

