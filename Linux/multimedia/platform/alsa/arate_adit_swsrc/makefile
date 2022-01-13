# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: arate_asrc_imx
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = arate_adit_swsrc

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(libdir)/alsa-lib/libasound_module_arate_adit_swsrc.so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------


INCLUDES     += -I../../adit_swsrc/adit_swsrc/include \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/alsa \
                #

LIBRARIES    += -lasound \
                -lpthread \
                -ldl \
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
	@install -m 755 $(BASE_DIR)/multimedia/test/alsa-plugin-swasrc-test.sh $(DEST_DIR)$(testdir)

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))


# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
