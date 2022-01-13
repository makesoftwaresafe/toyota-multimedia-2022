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

SUBCOMP_DEP1  = testdaemon
SUBCOMP_DEP2  = rts
SUBCOMP_DEP3  = 
SUBCOMP_DEP4  = 

SUBCOMPONENTS = $(SUBCOMP_DEP1) $(SUBCOMP_DEP2) $(SUBCOMP_DEP3) $(SUBCOMP_DEP4)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

$(SUBTARGETS) \
all:$(SUBCOMPONENTS) 

$(SUBCOMP_DEP1): $(SUBCOMP_DEP2)

$(SUBCOMP_DEP2): $(SUBCOMP_DEP3)

$(SUBCOMP_DEP3): $(SUBCOMP_DEP4)

$(SUBCOMPONENTS):
	@$(MAKE) $(MAKECMDGOALS) GOAL=$(GOAL) -C $@

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
