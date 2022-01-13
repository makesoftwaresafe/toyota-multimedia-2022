LIST=VARIANT
ifndef QRECURSE
QRECURSE=recurse.mk
ifdef QCONFIG
QRDIR=$(dir $(QCONFIG))
endif
endif
CCFLAGS += -g -DNDEBUG
include $(QRDIR)$(QRECURSE)
