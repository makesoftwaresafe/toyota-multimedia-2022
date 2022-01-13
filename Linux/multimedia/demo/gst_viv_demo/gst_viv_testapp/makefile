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
SUBCOMP_NAME = gst_viv_testapp

ARCHIVE      =
PROGRAM      = $(BINARY_DIR)/gst_viv_testapp
SHAREDLIB    = 

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 
C_FLAGS      += -g -DEGL_API_FB -DLINUX -DGL_GLEXT_PROTOTYPES # TODO: don't use -g in RELEASE !!!

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I./inc \
                -I./../gst_viv_app_sink/inc \
                -I$(OE_STAGING_FOLDER)/usr/include/gstreamer-0.10 \
                -I$(OE_STAGING_FOLDER)/usr/include/glib-2.0 \
                -I$(OE_STAGING_FOLDER)/usr/lib/glib-2.0/include \
                -I$(OE_STAGING_FOLDER)/usr/include/libxml2 \
                #

LIBRARIES    += -L$(USRLIB_DIR)/gstreamer-0.10 \
                -L$(OE_STAGING_FOLDER)/usr/lib/fb_x11 \
                -lgst_viv_app_sink \
                -lpthread \
                -lm \
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
                -lz \
                #

LIBRARIES    += -Wl,--rpath-link=$(OE_STAGING_FOLDER)/usr/lib 

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
