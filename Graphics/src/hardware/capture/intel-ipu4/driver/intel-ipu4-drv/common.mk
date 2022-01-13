ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=IPU4 driver
endef

EXTRA_INCVPATH += $(PROJECT_ROOT)/public/intel/ipu4
EXTRA_INCVPATH += $(PROJECT_ROOT)/include
EXTRA_INCVPATH += $(PROJECT_ROOT)/include/x86
EXTRA_INCVPATH += $(PROJECT_ROOT)/include/uapi

EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/buffer/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/cell/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/vied
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/vied/vied
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/support
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/cell_program_load/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/device_access/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/devices/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/fw_load/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/isys_infobits/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/isysapi/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/pkg_dir/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/port/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/regmem/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/regmem/src
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/server_init/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/support
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/syscom/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/trace/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/utils/system_defs
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/vied/vied
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/devices/isys/bxtB0
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/cse/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600/reg_dump/src/isys/bxtB0_gen_reg_dump

EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/client_pkg/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/fw_abi_common_types
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/fw_abi_common_types/cpu
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psys_private_pg/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psys_server/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/data/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/device/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/dynamic/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/kernel/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/param/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/resource_model/bxtB0
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/sim/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/static/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/vied_nci_acb/interface
EXTRA_INCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/vied_parameters/interface

EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/buffer/src/cpu
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/cell/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/cell_program_load/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/config/
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/device_access/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/devices/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/fw_load/src/xmem
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/isys_infobits/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/isysapi/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/reg_dump/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/server_init/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/syscom/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/pkg_dir/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600/port/src

EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/client_pkg/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/devices/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/devices/psys/bxtB0
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psys_server/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/data/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/device/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/dynamic/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/kernel/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/param/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/psys_server_manifest/bxtB0
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/resource_model/bxtB0
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/sim/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/psysapi/static/src
EXTRA_SRCVPATH += $(PROJECT_ROOT)/lib2600psys/lib/vied_parameters/src

CCFLAGS += -g -O0
# moved from file to makefile in ec16
CCFLAGS += -DHRT_HOST
# moved from file to makefile in ec16
CCFLAGS += -DPARAMETER_INTERFACE_V2

# flags from autoconf.h that are common
CCFLAGS += -DCONFIG_X86 -DCONFIG_SMP -DCONFIG_PCI -DCONFIG_I2C_MODULE -DCONFIG_OPTIMIZE_INLINING
CCFLAGS += -DCONFIG_X86_DEV_DMA_OPS -DCONFIG_IOMMU_API -DCONFIG_COMMON_CLK
CCFLAGS += -DCONFIG_MEDIA_CONTROLLER -DCONFIG_VIDEO_V4L2_SUBDEV_API -DCONFIG_FW_LOADER -DCONFIG_PM

LDFLAGS += -M -lc

# Report unresolved symbols at build time
LDFLAGS += -Wl,--unresolved-symbols=report-all

NAME=intel-ipu4drv

include $(MKFILES_ROOT)/qmacros.mk

cpu_variants:=$(if $(filter arm,$(CPU)),v7,$(if $(filter ppc,$(CPU)),spe))
CPU_VARIANT:=$(CPUDIR)$(subst $(space),,$(foreach v,$(filter $(cpu_variants),$(VARIANT_LIST)),_$(v)))
CCFLAGS += $(CCFLAGS_$(CPU_VARIANT))  $(CCFLAGS_@$(basename $@))  \
		   $(CCFLAGS_$(CPU_VARIANT)_@$(basename $@))  $(CCFLAGS_D)

include $(MKFILES_ROOT)/qtargets.mk

LIBS += cacheS

ifneq ($(wildcard $(foreach dir,$(LIBVPATH),$(dir)/libpci.so)),)
	LIBS += pci
	CCFLAGS+=-DLIBPCI
endif

# Copy IPU4 firmware to stage folder
FW_DEST = $(INSTALL_ROOT_nto)/lib/firmware/intel/ipu4/
FW_SOURCE = $(CURDIR)/../../../firmware/ipu4_cpd_b0.bin
all:
	$(CP_HOST) $(FW_SOURCE) $(FW_DEST)