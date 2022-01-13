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

COMP_DIR     = ../..
SUBCOMP_NAME = amra_dbus_hotplug

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(libdir)/libAmRaDBusHotplug.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -Iinclude \
                -I$(COMP_DIR)/am_dbus_common/include \
                -I$(COMP_DIR)/amra_audio_server/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dlt \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dbus-1.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/dbus-1.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2 \

LIBRARIES    += -lstdc++ -ldbus-1

PACKAGES     =

SOURCES      += $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(COMMAND_STUB) $(ARCHIVE) $(SHAREDLIB) $(PROGRAM)

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(includedir)/kpra
	@install -m 644 include/IRaHotplugClient.h $(DEST_DIR)$(includedir)/kpra
	@install -d $(DEST_DIR)$(datadir)/audiomanager
	@install -m 644 include/HotplugReceiver.xml $(DEST_DIR)$(datadir)/audiomanager
	@install -m 644 include/HotplugSender.xml $(DEST_DIR)$(datadir)/audiomanager

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
