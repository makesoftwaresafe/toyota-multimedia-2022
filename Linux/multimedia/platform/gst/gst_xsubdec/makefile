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
SUBCOMP_NAME = gst_xsubdec

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(USRLIB_DIR)/gstreamer-0.10/libgstxsubdec.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 
C_FLAGS      += -g		# TODO: don't use -g in RELEASE !!!

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I./inc \
                -I$(OE_STAGING_FOLDER)/usr/include/gstreamer-0.10 \
                -I$(OE_STAGING_FOLDER)/usr/include/glib-2.0 \
                -I$(OE_STAGING_FOLDER)/usr/lib/glib-2.0/include \
                -I$(OE_STAGING_FOLDER)/usr/include/libxml2 \
                #

LIBRARIES    += -lpthread \
                -lm \
                -lrt \
                -ldl \
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


PACKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.c)
SOURCES      += $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(MD) $(STAGING_PATH)/usr/lib/gstreamer-0.10
	@install -m 755 $(SHAREDLIB) $(STAGING_PATH)/usr/lib/gstreamer-0.10

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
