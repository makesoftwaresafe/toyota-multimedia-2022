# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = $(notdir $(shell pwd))

ARCHIVE      =
PROGRAM      = $(bindir)/$(SUBCOMP_NAME)
SHAREDLIB    =

# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk

# -----------------------------------------------------------------------------
# Sub-Component settings (user mode)
# -----------------------------------------------------------------------------
#C_FLAGS      += -DADIT_LOGGING_PRINTF
C_FLAGS      += -DADIT_LOGGING_DLT
CPP_FLAGS    += $(C_FLAGS)

INCLUDES     += -Iinclude -Isrc \
                -I$(PROJECT_DIR)/common \
                -I$(DEVEL_STAGING_FOLDER)$(includedir)/adit-components

LIBRARIES    += -laoap-usb-discoverer \
                -laoap \
                -lusb-1.0 \
                -ludev \
                -lgcc \
                -lstdc++ \
                -ldlt \
                -lpthread \
                -lrt

SOURCES      = $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@install -m 755 -d $(DEST_DIR)/$(testdir)
	@install -m 755 aoap-discovery-smoketest.sh $(DEST_DIR)$(testdir)

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default_tail.mk
