# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: amap_dbus_wrp
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = amap_dbus_wrp

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(libdir)/libapplicationclient.so

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
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/glib-2.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/glib-2.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/dbus-1.0/include

LIBRARIES    += -lm \
                -ldl \
                -ldlt \
                -ldbus-1 \
                -lpthread \
                -lstdc++ \
                -lgobject-2.0 \
                -lglib-2.0 \
                -lam_dbus_common \
                -lz	\
                -lxml2 \
                -lAudioManagerUtilities

PACKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(includedir)/ApplicationClient
	@install -m 644 include/IAmApplicationClient.h $(DEST_DIR)$(includedir)/ApplicationClient

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
