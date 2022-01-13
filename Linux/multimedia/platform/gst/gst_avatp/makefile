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
SUBCOMP_NAME = gst_avatp

ARCHIVE      =
PROGRAM      =
ifeq ($(GSTREAMER_VERSION),1.0)
SHAREDLIB    = $(libdir)/gstreamer-1.0/libgstavatp.so
else
SHAREDLIB    = $(libdir)/gstreamer-0.10/libgstavatp.so
endif

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I./inc

ifeq ($(GSTREAMER_VERSION),1.0)
INCLUDES     +=  -I$(DEVEL_STAGING_FOLDER)$(includedir)/gstreamer-1.0
else
INCLUDES     +=  -I$(DEVEL_STAGING_FOLDER)$(includedir)/gstreamer-0.10
endif

INCLUDES     += -I$(DEVEL_STAGING_FOLDER)$(includedir)/glib-2.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/glib-2.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2 \
                #

LIBRARIES    += -lpthread \
                -lm \
                -lrt \
                -ldl
ifeq ($(GSTREAMER_VERSION),1.0)
LIBRARIES    += -lgstreamer-1.0 \
                -lgstbase-1.0
else
LIBRARIES    += -lgstreamer-0.10 \
                -lgstbase-0.10
endif
LIBRARIES    += -lglib-2.0 \
                -lgobject-2.0 \
                -lgthread-2.0 \
                -lgmodule-2.0 \
                -lxml2 \
                -lz
ifeq ($(GSTREAMER_VERSION),1.0)
LIBRARIES    += -lgstrtp-1.0
else
LIBRARIES    += -lgstrtp-0.10
endif
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
ifeq ($(GSTREAMER_VERSION),1.0)
	@$(MD) $(DEST_DIR)$(includedir)/gstreamer-1.0
	@install -m 644 src/gstavatp*.h   $(DEST_DIR)$(includedir)/gstreamer-1.0
	@install -m 644 src/avatp_avtp.h  $(DEST_DIR)$(includedir)/gstreamer-1.0
else
	@$(MD) $(DEST_DIR)$(includedir)/gstreamer-0.10
	@install -m 644 src/gstavatp*.h   $(DEST_DIR)$(includedir)/gstreamer-0.10
	@install -m 644 src/avatp_avtp.h  $(DEST_DIR)$(includedir)/gstreamer-0.10
endif

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
