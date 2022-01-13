# This is an automatically generated record.
# The area between QNX Internal Start and QNX Internal End is controlled by
# the QNX IDE properties.

ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION= QNX service for IPU4 driver
endef

CCFLAGS += -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS
CCFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
DEBUG= -g

USEFILE=$(PROJECT_ROOT)/ipu4.use

include $(MKFILES_ROOT)/qmacros.mk

cpu_variants:=$(if $(filter arm,$(CPU)),v7,$(if $(filter ppc,$(CPU)),spe))
CPU_VARIANT:=$(CPUDIR)$(subst $(space),,$(foreach v,$(filter $(cpu_variants),$(VARIANT_LIST)),_$(v)))
CCFLAGS += $(CCFLAGS_$(CPU_VARIANT))  $(CCFLAGS_@$(basename $@))  \
		   $(CCFLAGS_$(CPU_VARIANT)_@$(basename $@))  $(CCFLAGS_D)

include $(MKFILES_ROOT)/qtargets.mk

LIBS+=intel-ipu4drv
LIBS_armle_v7+= $(LIBS)

ifneq ($(wildcard $(foreach dir,$(LIBVPATH),$(dir)/libslog2.so)),)
	LIBS += slog2
	CCFLAGS+=-DLIBSLOG2
endif

ifneq ($(wildcard $(foreach dir,$(LIBVPATH),$(dir)/liblogin.a)),)
	LIBS += loginS
endif

OPTIMIZE_TYPE_g=none
OPTIMIZE_TYPE=$(OPTIMIZE_TYPE_$(filter g, $(VARIANTS)))
