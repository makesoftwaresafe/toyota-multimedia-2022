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
PROGRAM      = $(BINARY_DIR)/ipod-$(SUBCOMP_NAME).out
SHAREDLIB    =


# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk

# -----------------------------------------------------------------------------
# Sub-Component settings (user mode)
# -----------------------------------------------------------------------------

INCLUDES     += -Iinclude

LIBRARIES    += -lusb-1.0 -ludev -lipod-iap2_test_utility\
                -lipod-iap2-linklayer -lipod-iap2-interface -lipod-iap2-session -lipod-iap2-transport -lipod-iap2-dlt_logging \
                -lipod-iap2-ffs-gadget \
                -lipod-iap2_usb_device_mode_plugin -lipod-iap2_usb_multi_host_mode_plugin -lipod-iap2_usb_host_mode_plugin -lipod-dlt_logging -lipod-iap1-dlt_logging\
                -lipod-iap2_bluetooth_plugin -lipod-iap2-usb-role-switch \
                -lipod-iap2_over_carplay_plugin \
                -lpthread \
                -lrt \
                -lusbg \
                -lpfcfg 

PACKAGES     = $(SOURCE_PATH_UL)/platform/system/common \
               $(PROJECT_DIR)/../platform/iAP2/interface \
               $(PROJECT_DIR)/../platform/iAP2/transport \
               $(PROJECT_DIR)/../platform/iAP2/usb_role_switch \
               $(PROJECT_DIR)/../platform/ipod_logging/ipod_dlt_logging \
               $(PROJECT_DIR)/../test/iAP2/iap2_test_utility \
               $(PROJECT_DIR)/../platform/ipod_logging/iap2_dlt_logging \
               $(PROJECT_DIR)/../platform/ipod_logging/iap1_dlt_logging \

LINKAGES     = $(AUTH_NAME) $(PLUG_NAME_AUTHCOM) 

SOURCES      = $(wildcard $(SOURCE_DIR)/*.c)

# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

install:
	@$(call subcomp_install)
	@$(MD) -p $(STAGING_PATH)/opt/platform

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default_tail.mk
