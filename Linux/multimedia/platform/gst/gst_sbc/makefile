# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: gst_sbc
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = gst_sbc

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(libdir)/gstreamer-0.10/libgstbluetooth.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 


# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------
C_FLAGS += -DHAVE_CONFIG_H

INCLUDES     += -I./src \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/gstreamer-0.10 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/glib-2.0 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dbus-1.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/glib-2.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/dbus-1.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/sbc \
                #

LIBRARIES    += -ldbus-1 \
                -lrt \
                -lgstaudio-0.10 \
                -lgstpbutils-0.10 \
                -lgstinterfaces-0.10 \
                -lgstrtp-0.10 \
                -lgstbase-0.10 \
                -lgstreamer-0.10 \
                -lgobject-2.0 \
                -lffi \
                -lgmodule-2.0 \
                -lgthread-2.0 \
                -lglib-2.0 \
                -lpthread \
                -lxml2 \
                -ldl \
                -lz \
                -lm \
                -lsbc \
                #



PACKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.c)
SOURCES      += $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
