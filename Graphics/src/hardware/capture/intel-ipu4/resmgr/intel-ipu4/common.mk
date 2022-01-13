#
# Copyright  2016, QNX Software Systems Ltd.  All Rights Reserved
#
# This source code has been published by QNX Software Systems Ltd.
# (QSSL).  However, any use, reproduction, modification, distribution
# or transfer of this software, or any software which includes or is
# based upon any of this code, is only permitted under the terms of
# the QNX Open Community License version 1.0 (see licensing.qnx.com for
# details) or as otherwise expressly authorized by a written license
# agreement from QSSL.  For more information, please email licensing@qnx.com.
#
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=IPU4 driver API Library
endef

NAME=intel-ipu4

INSTALLDIR=usr/lib

CCFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE

PUBLIC_INCVPATH=$(PROJECT_ROOT)/public $(PROJECT_ROOT)/private


#===== LIBS - a space-separated list of library items to be included in the link.
include $(MKFILES_ROOT)/qmacros.mk
#LIBS +=

#QNX internal start
ifeq ($(filter g, $(VARIANT_LIST)),g)
DEBUG_SUFFIX=_g
LIB_SUFFIX=_g
else
DEBUG_SUFFIX=$(filter-out $(VARIANT_BUILD_TYPE) le be,$(VARIANT_LIST))
ifeq ($(DEBUG_SUFFIX),)
DEBUG_SUFFIX=_r
else
DEBUG_SUFFIX:=_$(DEBUG_SUFFIX)
endif
endif

include $(MKFILES_ROOT)/qtargets.mk
