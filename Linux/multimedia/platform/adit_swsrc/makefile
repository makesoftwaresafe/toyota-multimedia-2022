# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Component makefile
#
# Description: This file calls sub makefiles for all subcomponents.
# -----------------------------------------------------------------------------
 
COMP_DIR      = .

SUBCOMP_DEP1  = adit_swsrc
SUBCOMP_DEP2  =
SUBCOMP_DEP3  =

SUBCOMPONENTS = $(SUBCOMP_DEP1) $(SUBCOMP_DEP2) $(SUBCOMP_DEP3)

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

.PHONY: $(PHONY) $(SUBCOMPONENTS)

all: $(SUBCOMPONENTS)

$(SUBCOMP_DEP1): $(SUBCOMP_DEP2)

$(SUBCOMP_DEP2): $(SUBCOMP_DEP3)

$(SUBCOMPONENTS):
	$(MAKE) $(MAKECMDGOALS) GOAL=$(GOAL) -C $@

install \
clean \
help \
info:
	-@$(call comp_$(MAKECMDGOALS),$(SUBCOMPONENTS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_comp.mk
