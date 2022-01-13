# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: Audio Backend Smoketest
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = testdaemon

ARCHIVE      =
PROGRAM      = $(bindir)/audioif_smoketest
SHAREDLIB    = 

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I./inc/ \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/adit-components \
                #

LIBRARIES    += -laudio \
                -ldl \
                -lstdc++ \
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
	@install -d $(DEST_DIR)$(testdir)
	@install -m 755 $(BASE_DIR)/multimedia/test/abalsa_smoketest_devices.csv $(DEST_DIR)$(testdir)
	@install -m 755 $(BASE_DIR)/multimedia/test/abalsa-smoketest.sh $(DEST_DIR)$(testdir)

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
