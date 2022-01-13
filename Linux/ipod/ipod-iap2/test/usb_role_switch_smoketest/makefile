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
PROGRAM      = $(BINARY_DIR)/ipod-usb-role-switch-smoketest.out
SHAREDLIB    =


# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk

# -----------------------------------------------------------------------------
# Sub-Component settings (user mode)
# -----------------------------------------------------------------------------

INCLUDES     += -Iinclude_component -Iinclude -Iinc

LIBRARIES    += -lusb-1.0 -ludev -lkmod \
                -lrt -lusbg\
                -lipod-iap2-usb-role-switch \
                -lipod-dlt_logging -lipod-iap2-dlt_logging -lipod-iap1-dlt_logging\

PACKAGES     = $(SOURCE_PATH_UL)/platform/system/common \
               $(PROJECT_DIR)/../platform/iAP2/usb_role_switch \
               $(PROJECT_DIR)/../platform/ipod_logging/ipod_dlt_logging \
               $(PROJECT_DIR)/../platform/ipod_logging/iap2_dlt_logging \
               $(PROJECT_DIR)/../platform/ipod_logging/iap1_dlt_logging \

LINKAGES     =

SOURCES      = $(wildcard $(SOURCE_DIR)/*.c)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@$(MD) -p $(STAGING_PATH)/opt/platform
#	@install -m 755 ipod-iap2-smoketest.sh $(STAGING_PATH)/opt/platform/

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default_tail.mk
