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

SUBCOMP_DEP1  = am_dbus_common
SUBCOMP_DEP2  = amri_dbus_plugin
SUBCOMP_DEP3  = amcp_dbus_plugin
SUBCOMP_DEP4  = amrp_dbus_wrp
SUBCOMP_DEP5  = amcp_dbus_wrp
SUBCOMP_DEP6  = amap_dbus_wrp
SUBCOMP_DEP7  = amra_alsa_proxy amra_rts_proxy
SUBCOMP_DEP8  = am_socket_common
SUBCOMP_DEP9  = amri_socket_plugin

SUBCOMPONENTS = $(SUBCOMP_DEP1) $(SUBCOMP_DEP2) $(SUBCOMP_DEP3) $(SUBCOMP_DEP4) \
				$(SUBCOMP_DEP5) $(SUBCOMP_DEP6) $(SUBCOMP_DEP7) $(SUBCOMP_DEP8) \
				$(SUBCOMP_DEP9)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

$(SUBTARGETS) \
all:$(SUBCOMPONENTS) 

$(SUBCOMP_DEP2): $(SUBCOMP_DEP1)
$(SUBCOMP_DEP3): $(SUBCOMP_DEP1)
$(SUBCOMP_DEP4): $(SUBCOMP_DEP2)
$(SUBCOMP_DEP5): $(SUBCOMP_DEP3)
$(SUBCOMP_DEP6): $(SUBCOMP_DEP2)
$(SUBCOMP_DEP6): $(SUBCOMP_DEP3)
$(SUBCOMP_DEP9): $(SUBCOMP_DEP8)

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
