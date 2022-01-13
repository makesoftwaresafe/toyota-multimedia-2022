# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: gst_viv_app_sink
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = gst_v4l_csc

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(LIBRARY_DIR)/libgst_v4l_csc.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# TODO: fix " error: 'g_mutex_free' is deprecated "
C_CFLAGS += -Wno-deprecated-declarations

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I./inc \
                -I$(DEVEL_STAGING_FOLDER)/usr/include/gstreamer-0.10 \
                -I$(DEVEL_STAGING_FOLDER)/usr/include/glib-2.0 \
                -I$(DEVEL_STAGING_FOLDER)/usr/lib/glib-2.0/include \
                -I$(DEVEL_STAGING_FOLDER)/usr/include/libxml2 \
                -I$(DEVEL_STAGING_FOLDER)$(exec_prefix)/uapi/include

LIBRARIES    += -lpthread \
                -lm \
                -lrt \
                -ldl \
                -lgstreamer-0.10 \
                -lgstbase-0.10 \
                -lglib-2.0 \
                -lgobject-2.0 \
                -lgthread-2.0 \
                -lgmodule-2.0 \
                -lgstfsl-0.10 \
                -lxml2 \
                -lz \
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
