# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: gst_adit_audioremsample
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = gst_adit_audioresample

ARCHIVE      =
PROGRAM      =
ifeq ($(GSTREAMER_VERSION),1.0)
SHAREDLIB    = $(libdir)/gstreamer-1.0/libgst_adit_audioresample.so
else
SHAREDLIB    = $(libdir)/gstreamer-0.10/libgst_adit_audioresample.so
endif

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------

includedir = /usr/include

ifeq ($(GSTREAMER_VERSION),1.0)
INCLUDES     += -I$(DEVEL_STAGING_FOLDER)$(includedir)/gstreamer-1.0 \
		-I$(DEVEL_STAGING_FOLDER)/usr/lib$(LIB_SUFFIX)/gstreamer-1.0/include \
		#
else
INCLUDES     += -I$(DEVEL_STAGING_FOLDER)$(includedir)/gstreamer-0.10 \
		-I$(DEVEL_STAGING_FOLDER)/usr/lib$(LIB_SUFFIX)/gstreamer-0.10/include \
		#
endif

INCLUDES     += -Iinc \
		-I$(DEVEL_STAGING_FOLDER)$(includedir)/glib-2.0 \
		-I$(DEVEL_STAGING_FOLDER)$(libdir)/glib-2.0/include \
		#

ifeq ($(GSTREAMER_VERSION),1.0)
LIBRARIES    += -lgstreamer-1.0 \
                -lgstbase-1.0
else
LIBRARIES    += -lgstreamer-0.10 \
                -lgstbase-0.10
endif

LIBRARIES    += -lgobject-2.0 \
		-lglib-2.0 \
		-lpthread \
		-lstdc++ \
		-ldl \
		-lm

PACKAGES     =

SOURCES      += $(wildcard $(SOURCE_DIR)/*.c)

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
