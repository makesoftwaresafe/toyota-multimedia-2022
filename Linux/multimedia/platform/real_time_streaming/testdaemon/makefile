# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: rts
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = testdaemon

ARCHIVE      =
PROGRAM      = $(bindir)/rtsd
SHAREDLIB    = 

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I. \
                -I ../rts/inc/ 
                #

LIBRARIES    += -lrtstream \
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
	@install -m 644 $(BASE_DIR)/multimedia/test/rts_smoketest_devices.csv $(DEST_DIR)$(testdir)
	@install -m 644 $(BASE_DIR)/multimedia/test/rts_smoketest_devices_lazy.csv $(DEST_DIR)$(testdir)
	@install -m 755 $(BASE_DIR)/multimedia/test/audio-rts-test.sh $(DEST_DIR)$(testdir)

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
