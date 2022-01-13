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
SUBCOMP_NAME = audio_mock

ARCHIVE      =
PROGRAM      = 	$(bindir)/audio_mock
SHAREDLIB    = 

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------

INCLUDES    += -I. \
               -Iinclude \
               -I../amra_alsa/include \
               -I$(DEVEL_STAGING_FOLDER)$(includedir) \
               -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager \

LIBRARY_DIRS += -L$(STAGING_PATH_LIB) 

LIBRARIES    += -lm \
                -ldl \
                -ldlt \
                -lstdc++ \
                -lpthread \
                -lz \
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
