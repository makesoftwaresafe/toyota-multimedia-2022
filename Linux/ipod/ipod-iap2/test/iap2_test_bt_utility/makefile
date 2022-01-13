# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: conformance
#              Builds executable
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = $(notdir $(shell pwd))

ARCHIVE      =
PROGRAM      =
SHAREDLIB    = $(LIBRARY_DIR)/libipod-$(SUBCOMP_NAME).so

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------

INCLUDES    += -Iinclude \
               -I$(OE_STAGING_FOLDER)/usr/include/dbus-1.0 \
               -I$(OE_STAGING_FOLDER)/usr/include/glib-2.0 \
               -I$(OE_STAGING_FOLDER)/usr/include/dbus-1.0 \

INCLUDES    += -I$(OE_STAGING_FOLDER)/usr/lib$(LIB_SUFFIX)/glib-2.0/include
INCLUDES    += -I$(OE_STAGING_FOLDER)/usr/lib$(LIB_SUFFIX)/dbus-1.0/include \

LIBRARIES   += -ldbus-1 \
               -lglib-2.0 \
               -ldbus-glib-1 \
               -lpthread \
               -lbluetooth

PACKAGES     = $(SOURCE_PATH_UL)/platform/system/common \
               $(PROJECT_DIR)/../platform/ipod_logging/ipod_dlt_logging \
               $(PROJECT_DIR)/../platform/iAP2/iap2_dlt_logging \
               $(PROJECT_DIR)/../test/iAP2/iap2_test_utility \
               $(PROJECT_DIR)/../platform/iAP2/interface \
               $(PROJECT_DIR)/../platform/iAP2/transport \
               $(PROJECT_DIR)/../platform/ipod_logging/ipod_dlt_logging \
               $(PROJECT_DIR)/../platform/iAP2/iap2_dlt_logging

LINKAGES     =

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
