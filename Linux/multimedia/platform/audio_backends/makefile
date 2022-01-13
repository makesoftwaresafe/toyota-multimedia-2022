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
SUBCOMP_DEP2  = abalsa

SUBCOMPONENTS = $(SUBCOMP_DEP1) $(SUBCOMP_DEP2)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

$(SUBTARGETS) \
all:$(SUBCOMPONENTS) 

$(SUBCOMP_DEP1): $(SUBCOMP_DEP2)

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
