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

SUBCOMP_DEP1  = arate
ifeq ($(TARGET_PROCESSOR),arm) #GOAL or MAKE_GOAL would be better but unfortunately not set when running LINT
SUBCOMP_DEP2  = arate_asrc_imx
else
SUBCOMP_DEP2  =
endif
SUBCOMP_DEP3  = arate_adit_swsrc
SUBCOMP_DEP4  = delay loop lv2 rate_adit_swsrc

SUBCOMPONENTS = $(SUBCOMP_DEP1) $(SUBCOMP_DEP2) $(SUBCOMP_DEP3) $(SUBCOMP_DEP4)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

$(SUBTARGETS) \
all:$(SUBCOMPONENTS) 

$(SUBCOMP_DEP2): $(SUBCOMP_DEP1)

$(SUBCOMP_DEP3): $(SUBCOMP_DEP1)

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
