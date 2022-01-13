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

SUBCOMP_DEP1  = gst_appsink_zerocopy
SUBCOMP_DEP2  =
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

install: all
	@$(call comp_$(MAKECMDGOALS),$(SUBCOMPONENTS))

clean \
help \
info:
	-@$(call comp_$(MAKECMDGOALS),$(SUBCOMPONENTS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_comp.mk
