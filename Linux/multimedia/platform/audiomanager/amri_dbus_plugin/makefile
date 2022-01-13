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

COMP_DIR     = ..
SUBCOMP_NAME = amri_dbus_plugin

ARCHIVE      =
PROGRAM      = 
SHAREDLIB    =  $(libdir)/audiomanager/routing/libamri_dbus_plugin.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I. \
                -Iinclude \
                -I../am_dbus_common/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager/AudioManagerUtilities \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dlt \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dbus-1.0 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/dbus-1.0/include

LIBRARIES    += -lm \
                -ldl \
                -ldlt \
                -ldbus-1 \
                -lam_dbus_common \
                -lz \
                -lxml2 \
                -lAudioManagerUtilities \
                -lstdc++

PACKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(sysconfdir)/dbus-1/system.d
	@install -m 644 conf/amrp_dbus.conf $(DEST_DIR)$(sysconfdir)/dbus-1/system.d
	@install -d $(DEST_DIR)$(sysconfdir)/audiomanager/routing
	@install -m 644 conf/dbus.xml $(DEST_DIR)$(sysconfdir)/audiomanager/routing

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
