# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: amra_daemon
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = amra_daemon

ARCHIVE      =
PROGRAM      = $(bindir)/AudioRoutingAdapter
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
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/AMRoutingAPI \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager

LIBRARIES    +=	-lm \
                -ldl \
                -ldlt \
                -lpthread \
                -lstdc++ \
                -lAudioManagerUtilities

PACKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.cpp)

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
