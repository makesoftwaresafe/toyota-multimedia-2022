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
SUBCOMP_NAME = amrp_dbus_wrp

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(libdir)/libAudioManagerRouting.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -Iinclude \
                -I../am_dbus_common/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager/AudioManagerUtilities \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dlt \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/dbus-1.0 \
                -I$(DEVEL_STAGING_FOLDER)$(libdir)/dbus-1.0/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2

LIBRARIES   +=  -ldlt \
                -ldbus-1 \
                -lam_dbus_common \
                -lstdc++ \
                -lpthread \
                -lz \
                -lxml2 \
                -lAudioManagerUtilities

PACKAGES     =

SOURCES      += $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(SHAREDLIB) $(PROGRAM)

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(includedir)/AMRoutingAPI
	@install -m 644 include/IAmRoutingClient.h $(DEST_DIR)$(includedir)/AMRoutingAPI

clean: 
	-@$(call subcomp_$(MAKECMDGOALS))

help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
