# -----------------------------------------------------------------------------
#        (c) ADIT - Advanced Driver Information Technology JV
# -----------------------------------------------------------------------------
# Title:       Subcomponent makefile
#
# Description: Makefile for the subcomponent: amra_alsa
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Base settings
# -----------------------------------------------------------------------------

COMP_DIR     = ..
SUBCOMP_NAME = amra_audio_server

ARCHIVE      =
PROGRAM      = $(bindir)/KPRoutingAdaptor
SHAREDLIB    =

GENERATED_DIR = $(shell pwd)/$(TARGET_DIR)/$(VARIANT)/$(COMP_NAME)/$(SUBCOMP_NAME)/generated
IAS_AUDIO_ROUTING_STUB_CPP = $(GENERATED_DIR)/output/audio/rpcontroller/idl/IasAudioRoutingStub.cpp
IAS_AUDIO_PROCESSING_STUB_CPP = $(GENERATED_DIR)/output/audio/rpcontroller/idl/IasAudioProcessingStub.cpp
IAS_AUDIO_SETUP_STUB_CPP = $(GENERATED_DIR)/output/audio/rpcontroller/idl/IasAudioSetupStub.cpp
IAS_AUDIO_ROUTING_STUB_HPP = $(GENERATED_DIR)/output/audio/rpcontroller/idl/IasAudioRoutingStub.hpp
IAS_AUDIO_PROCESSING_STUB_HPP = $(GENERATED_DIR)/output/audio/rpcontroller/idl/IasAudioProcessingStub.hpp
IAS_AUDIO_SETUP_STUB_HPP = $(GENERATED_DIR)/output/audio/rpcontroller/idl/IasAudioSetupStub.hpp
IAS_INCLUDES = $(GENERATED_DIR)/.ias_includes
IDL_GENERATE = $(GENERATED_DIR)/.idl_generate
IAS_SYSROOT = $(shell pwd)/$(DEVEL_STAGING_FOLDER)
UFIPC_CODE_GENERATOR = $(IAS_SYSROOT)/opt/build_tools/bin/systembus_code_generator.jar
INCLUDE_PATHS = "$(shell pwd)/include;$(IAS_SYSROOT)/opt/sdk/include;$(IAS_SYSROOT)/usr/include;$(GENERATED_DIR)/output/audio/rpcontroller/idl;"
GENERATED = $(SOURCE_DIR)/IasAudioRoutingStub.cpp $(SOURCE_DIR)/IasAudioProcessingStub.cpp

# -----------------------------------------------------------------------------
# Component settings
# -----------------------------------------------------------------------------

include $(COMP_DIR)/settings.mk

# -----------------------------------------------------------------------------
# Sub-Component settings
# -----------------------------------------------------------------------------

# diasble the multi-threaded build as
# the dependency on generated code is not correctly resolved for static analysis
MAKEFLAGS       = --jobs 1

# LD_FLAGS += -L$(bindir)

INCLUDES += -I. \
            -Iinclude \
            -I$(COMP_DIR)/amra_audio_server/amra_dbus_hotplug/include \
            -I../amrp_dbus_wrp/include \
            -I$(GENERATED_DIR)/output \
            -I$(DEVEL_STAGING_FOLDER)$(includedir)/audiomanager \
            -I$(DEVEL_STAGING_FOLDER)$(includedir)/dlt \
            -I$(DEVEL_STAGING_FOLDER)$(includedir)/dbus-1.0 \
            -I$(DEVEL_STAGING_FOLDER)$(libdir)/dbus-1.0/include \
            -I$(DEVEL_STAGING_FOLDER)$(includedir)/libxml2 \
            -I$(DEVEL_STAGING_FOLDER)$(includedir)/AMCommonDBus \

LIBRARIES    += -lstdc++ \
                -lpthread \
                -ldlt \
                -lias-log_and_trace-common \
                -lxml2 \
                -lias-systembus-ufipc_helper \
                -lias-core_libraries-foundation \
                -lias-systembus-ufipc \
                -lias-audio-rpcontroller \
                -lias-audio-rtprocessingfw \
                -lAudioManagerRouting \
                -lAmRaDBusHotplug

PACKAGES     =

SOURCES      += $(SOURCE_DIR)/CRaAMRoutingClient.cpp \
                $(SOURCE_DIR)/CRaASClient.cpp \
                $(SOURCE_DIR)/CRaConfigManager.cpp \
                $(SOURCE_DIR)/CRaRequestManager.cpp \
                $(SOURCE_DIR)/CContextManager.cpp \
                $(SOURCE_DIR)/IAsAudioClient.cpp \
                $(SOURCE_DIR)/IAsAudioProcessingClient.cpp \
                $(SOURCE_DIR)/IAsAudioRoutingClient.cpp \
                $(SOURCE_DIR)/IasAudioSetupClient.cpp \
                $(SOURCE_DIR)/IasAudioRoutingStub.cpp \
                $(SOURCE_DIR)/IasAudioProcessingStub.cpp \
                $(SOURCE_DIR)/IasAudioSetupStub.cpp \
                $(SOURCE_DIR)/CRaHotplugReceiver.cpp \
                $(SOURCE_DIR)/main.cpp


# -----------------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------------

all: dbus_hotplud $(GENERATED) $(ARCHIVE) $(PROGRAM) $(SHAREDLIB)

$(GENERATED):
	$(MD) $(GENERATED_DIR)/output
	echo $(INCLUDE_PATHS) > $(IAS_INCLUDES)
	echo 'STUB;audio;rpcontroller;$(IAS_SYSROOT)/opt/sdk/include/audio/rpcontroller/idl;IasAudioRouting.idl' > $(IDL_GENERATE)
	echo 'STUB;audio;rpcontroller;$(IAS_SYSROOT)/opt/sdk/include/audio/rpcontroller/idl;IasAudioProcessing.idl' >> $(IDL_GENERATE)
	echo 'STUB;audio;rpcontroller;$(IAS_SYSROOT)/opt/sdk/include/audio/rpcontroller/idl;IasAudioSetup.idl' >> $(IDL_GENERATE)
	/usr/bin/java -jar $(UFIPC_CODE_GENERATOR) $(GENERATED_DIR) $(GENERATED_DIR)/output
	cp $(IAS_AUDIO_ROUTING_STUB_CPP) $(SOURCE_DIR)/IasAudioRoutingStub.cpp
	cp $(IAS_AUDIO_PROCESSING_STUB_CPP) $(SOURCE_DIR)/IasAudioProcessingStub.cpp
	cp $(IAS_AUDIO_SETUP_STUB_CPP) $(SOURCE_DIR)/IasAudioSetupStub.cpp

install:
	@$(call subcomp_install)
	@install -d $(DEST_DIR)$(sysconfdir)/dbus-1/system.d
	@install -m 644 conf/routingadapter.conf $(DEST_DIR)$(sysconfdir)/dbus-1/system.d
	@install -d $(DEST_DIR)$(sysconfdir)/routingadapter
	@install -m 644 conf/KPAudioDomain.xml $(DEST_DIR)$(sysconfdir)/routingadapter
	@install -d $(DEST_DIR)$(includedir)/kpra
	@install -m 644 include/IRaHotplugSend.h $(DEST_DIR)$(includedir)/kpra
	@install -m 644 include/IRaHotplugReceive.h $(DEST_DIR)$(includedir)/kpra
	@install -d $(DEST_DIR)$(systemd_system_unitdir)
	@install -m 644 conf/routingadapter.service $(DEST_DIR)$(systemd_system_unitdir)
	make -C amra_dbus_hotplug install


clean:
	-@$(call subcomp_clean)
	$(RM) -r $(GENERATED_DIR) 
	$(RM) $(SOURCE_DIR)/IasAudioRoutingStub.cpp
	$(RM) $(SOURCE_DIR)/IasAudioProcessingStub.cpp
	$(RM) $(SOURCE_DIR)/IasAudioSetupStub.cpp
	make -C amra_dbus_hotplug clean


help \
info:
	-@$(call subcomp_$(MAKECMDGOALS))

$(PROGRAM): dbus_hotplud

dbus_hotplud:
	make -C amra_dbus_hotplug

# -----------------------------------------------------------------------------
# Include implicite rules
# -----------------------------------------------------------------------------

include ${BASE_DIR}/tools/config/mk/default_tail.mk
