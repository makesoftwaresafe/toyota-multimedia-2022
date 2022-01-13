# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: arate
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ../../..
SUBCOMP_NAME = amra_dbus_hotplug

ARCHIVE      =
PROGRAM      = $(BINARY_DIR)/TestHotplugInterface
SHAREDLIB    =

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -Iinclude \
		-I$(OE_STAGING_FOLDER)/usr/include/audiomanager \
		-I$(OE_STAGING_FOLDER)/usr/include/dlt \
		-I$(COMP_DIR)/amra_audio_server/include \
		-I$(COMP_DIR)/amra_audio_server/amra_dbus_hotplug/include \
		-I$(OE_STAGING_FOLDER)/usr/include/dbus-1.0 \
		-I$(OE_STAGING_FOLDER)/usr/lib/dbus-1.0/include \
		-I$(OE_STAGING_FOLDER)/usr/include/AMRoutingAPI


LIBRARIES    += -lstdc++ -ldbus-1 -lAmRaDBusHotplug -lAudioManagerRouting -ldlt

PACKAGES     =

SOURCES      += $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(COMMAND_STUB) $(ARCHIVE) $(SHAREDLIB) $(PROGRAM)

install \
clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
