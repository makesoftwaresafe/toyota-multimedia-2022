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
PROGRAM      =
SHAREDLIB    = $(libdir)/libaoap-$(SUBCOMP_NAME).so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk 

# -----------------------------------------------------------------------------
# Sub-Component settings (user mode)
# -----------------------------------------------------------------------------
#CPP_FLAGS      += -DADIT_LOGGING_PRINTF
CPP_FLAGS    += -DADIT_LOGGING_DLT 
CPP_FLAGS    += $(C_FLAGS)

INCLUDES     += -Iinclude_component -Iinclude -Iinc -Isrc \
                -I$(PROJECT_DIR)/common 

LIBRARIES    += -lusb-1.0 -ludev -ldlt -lpthread -lstdc++

SOURCES      = $(wildcard $(SOURCE_DIR)/*.cpp)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(includedir)/adit-components
	@install include/*.h $(DEST_DIR)$(includedir)/adit-components

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default_tail.mk
