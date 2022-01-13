# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: gst_demo_player
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ./
SUBCOMP_NAME = gst_demo_player

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------

PROGRAM      = $(BINARY_DIR)/gst_demo_player


INCLUDES     += -Iinc \
                -I$(OE_STAGING_FOLDER)/usr/include/gstreamer-0.10 \
                -I$(OE_STAGING_FOLDER)/usr/include/glib-2.0 \
                -I$(OE_STAGING_FOLDER)/usr/lib/glib-2.0/include \
                -I$(OE_STAGING_FOLDER)/usr/include/libxml2 \


LIBRARIES     += -lm \
                -lrt \
                -ldl \
                -lGLESv2 \
                -lEGL \
                -lGAL \
                -lgstreamer-0.10 \
                -lgstvideo-0.10 \
                -lgstbase-0.10 \
                -lglib-2.0 \
                -lgobject-2.0 \
                -lgthread-2.0 \
                -lgmodule-2.0 \
                -lxml2 \
                -lz


SOURCES		 = $(wildcard $(SOURCE_DIR)/*.c)

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
include $(BASE_DIR)/tools/config/mk/default_tail.mk
