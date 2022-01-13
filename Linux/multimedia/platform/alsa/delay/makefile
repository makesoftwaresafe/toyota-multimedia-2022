# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: delay
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = delay

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(libdir)/alsa-lib/libasound_module_pcm_delay.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I. \
                -I$(DEVEL_STAGING_FOLDER)$(includedir) \
                #

LIBRARIES    += -lasound \
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
	@install -m 755 $(BASE_DIR)/multimedia/test/alsa-basic-test.sh $(DEST_DIR)$(testdir)
	@install -m 755 $(BASE_DIR)/multimedia/test/alsa-plugin-delay-test.sh $(DEST_DIR)$(testdir)

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
