# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Component makefile
#
# Description: This file calls sub makefiles for all subcomponents.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = .

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Components
# -----------------------------------------------------------------------------
# start ONLY HERE dynamic evaluation of tooling settings!

SUBCOMP_DEP1  = impl

SUBCOMPONENTS = $(SUBCOMP_DEP1)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

.PHONY: $(PHONY) $(SUBCOMPONENTS)

all: $(SUBCOMPONENTS)

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

