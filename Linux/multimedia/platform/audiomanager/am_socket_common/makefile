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
SUBCOMP_NAME = am_socket_common

ARCHIVE      = $(libdir)/libam_socket_common.a
PROGRAM      =
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
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager/ \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager/AudioManagerUtilities \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dlt \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dbus-1.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/dbus-1.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2

LIBRARIES    += -lm \
                -ldl \
                -ldlt \
                -ldbus-1 \
                -lz \
                -lxml2 \
                -lstdc++

PACKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.c)
SOURCES      += $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(includedir)/AMCommonSocket
	@install -m 644 include/*.h $(DEST_DIR)$(includedir)/AMCommonSocket

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
