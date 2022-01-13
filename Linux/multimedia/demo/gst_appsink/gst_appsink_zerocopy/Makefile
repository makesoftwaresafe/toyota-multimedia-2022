# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = $(notdir $(shell pwd))

ARCHIVE      =
PROGRAM      = $(bindir)/$(SUBCOMP_NAME)
SHAREDLIB    =

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk

# -----------------------------------------------------------------------------
# Sub-Component settings (user mode)
# -----------------------------------------------------------------------------

INCLUDES      = -Iinclude \
                -I$(DEVEL_STAGING_FOLDER)$(includedir) \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/ilm \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/glib-2.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/glib-2.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/glib-2.0 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/glib-2.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/gstreamer-1.0 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/gstreamer-1.0/include/ \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/gstreamer-1.0/include/ \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/drm

LIBRARIES    += -lGLESv2 \
                -lEGL \
                -lwayland-client \
                -livi-application \
                -lwayland-egl \
                -lpthread \
                -lm \
                -lgstreamer-1.0 \
                -lgstvideo-1.0 \
                -lgstapp-1.0 \
                -lgstallocators-1.0 \
                -lgobject-2.0 \
                -lglib-2.0

SOURCES       = $(wildcard $(SOURCE_DIR)/*.c)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@$(MD) -p $(DEST_DIR)${testdir}
	@install -m 755 gst-appsink-glrenderer-zerocopy-capture.sh $(DEST_DIR)${testdir}/
	@install -m 755 gst-appsink-glrenderer-zerocopy-playback.sh $(DEST_DIR)${testdir}/

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default_tail.mk
