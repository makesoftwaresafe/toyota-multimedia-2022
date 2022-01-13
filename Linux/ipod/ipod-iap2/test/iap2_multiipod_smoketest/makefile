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

INCLUDES     += -Iinclude \
                -I$(PROJECT_DIR)/../platform/iAP2/link/iAP2Link \
                -I$(PROJECT_DIR)/../platform/iAP2/link/iAP2Utility \
                -I$(PROJECT_DIR)/../platform/iAP2/link/iAP2UtilityImplementation \
                -I$(PROJECT_DIR)/../test/iAP2/iap2_test_bt_utility/include \
                -I$(OE_STAGING_FOLDER)/usr/include/dbus-1.0 \
                -I$(OE_STAGING_FOLDER)/usr/include/glib-2.0 \
                -I$(OE_STAGING_FOLDER)/usr/include/libxml2

ifeq ($(GSTREAMER_VERSION),1.0)
INCLUDES     += -I$(OE_STAGING_FOLDER)/usr/include/gstreamer-1.0 \
                -I$(OE_STAGING_FOLDER)/usr/lib$(LIB_SUFFIX)/gstreamer-1.0/include
else
INCLUDES     += -I$(OE_STAGING_FOLDER)/usr/include/gstreamer-0.10
endif

INCLUDES     += -I$(OE_STAGING_FOLDER)/usr/lib$(LIB_SUFFIX)/dbus-1.0/include \

INCLUDES     += -I$(OE_STAGING_FOLDER)/usr/lib$(LIB_SUFFIX)/glib-2.0/include
 
LIBRARIES    += -lusb-1.0 -ludev -lpfcfg -lipod-iap2_test_utility\
                -lipod-iap2-linklayer -lipod-iap2-interface -lipod-iap2-session -lipod-iap2-transport -lipod-iap2-dlt_logging \
                -lipod-iap2-ffs-gadget \
                -lipod-iap2_usb_device_mode_plugin -lipod-iap2_usb_multi_host_mode_plugin -lipod-iap2_usb_host_mode_plugin -lipod-dlt_logging -lipod-iap1-dlt_logging\
                -lipod-iap2_bluetooth_plugin -lipod-iap2-usb-role-switch \
                -lipod-iap2_over_carplay_plugin \
                -lgobject-2.0 \
                -lgmodule-2.0 \
                -lxml2 \
                -lglib-2.0 \
                -lpthread \
                -lrt \
                -lusbg \
                -lipod-iap2_test_bt_utility \
                -ldbus-1 \
 
ifeq ($(GSTREAMER_VERSION),1.0)
LIBRARIES    += -lgstreamer-1.0
else
LIBRARIES    += -lgstreamer-0.10 -lgstbase-0.10
endif 

PACKAGES     = $(SOURCE_PATH_UL)/platform/system/common \
               $(PROJECT_DIR)/../platform/ipod_authentication/authentication \
               $(PROJECT_DIR)/../platform/iAP2/interface \
               $(PROJECT_DIR)/../platform/iAP2/transport \
               $(PROJECT_DIR)/../test/iAP2/iap2_test_utility \
               $(PROJECT_DIR)/../platform/iAP2/usb_role_switch \
               $(PROJECT_DIR)/../platform/ipod_logging/ipod_dlt_logging \
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

clean \
help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include $(BASE_DIR)/tools/config/mk/default_tail.mk
