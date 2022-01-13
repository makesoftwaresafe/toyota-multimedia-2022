# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: gst_apx_sink
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = gst_apx_sink

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(libdir)/gstreamer-0.10/libgst_apx_sink.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 
C_FLAGS      += -Wshadow -DEGL_API_WAYLAND -DEGL_API_WL -DEGL_API_FB -DLINUX -DGL_GLEXT_PROTOTYPES
CPP_FLAGS    += -Wshadow -DEGL_API_WAYLAND -DLINUX -DGL_GLEXT_PROTOTYPES

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I./inc \
                -I$(DEVEL_STAGING_FOLDER)$(includedir) \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/gstreamer-0.10 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/glib-2.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/glib-2.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2

LIBRARIES    += -lm \
                -ldl \
                -lwayland-client \
                -lcompositor-shim \
                -lgstfsl-0.10 \
                -lgstreamer-0.10 \
                -lgstvideo-0.10 \
                -lgstbase-0.10 \
                -lglib-2.0 \
                -lgobject-2.0 \
                -lgthread-2.0 \
                -lgmodule-2.0 \
                -lxml2 \
                -lz \
                -lapx

PACKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.c)
SOURCES      += $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(testdir)
	@install -m 755 gst-video-out-apx-test.sh $(DEST_DIR)$(testdir)
	@install -m 755 gst-video-out-apx-software-test.sh $(DEST_DIR)$(testdir)
	@install -m 755 gst-video-out-apx-v4lsrctest.sh $(DEST_DIR)$(testdir)

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk

