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
SUBCOMP_NAME = amcp_test

ARCHIVE      =
PROGRAM      = $(bindir)/CommandPlugInTest
SHAREDLIB    = 
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
                -I../amcp_dbus_wrp/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager/AudioManagerUtilities \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dlt \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dbus-1.0 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2 \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/glib-2.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/glib-2.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/dbus-1.0/include

ifeq ($(GSTREAMER_VERSION),1.0)
INCLUDES     += -I$(OE_STAGING_FOLDER)$(includedir)/gstreamer-1.0 \
                -I$(OE_STAGING_FOLDER)$(libdir)/gstreamer-1.0/include
else
# by default, use gstreamer-0.10
INCLUDES     += -I$(OE_STAGING_FOLDER)$(includedir)/gstreamer-0.10
endif

#LIBRARY_DIRS += -L$(STAGING_PATH_LIB)

LIBRARIES    +=	-lm \
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
                -lAudioManagerCommand \
                -lAudioManagerUtilities

ifeq ($(GSTREAMER_VERSION),1.0)
LIBRARIES    += -lgstreamer-1.0
else
# by default, use gstreamer-0.10
LIBRARIES    += -lgstreamer-0.10
endif

PACKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(testdir)/unit_tests
	@install -m 755 $(BASE_DIR)/multimedia/test/amracp-systembus-smoketest.sh $(DEST_DIR)$(testdir)
	@install -m 644 $(BASE_DIR)/multimedia/test/amra_asound_test.conf $(DEST_DIR)$(testdir)
	@install -m 644 $(BASE_DIR)/multimedia/test/audio_files/*.wav $(DEST_DIR)$(testdir)/unit_tests

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
