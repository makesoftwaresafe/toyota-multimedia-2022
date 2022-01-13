LIST=CPU
ifndef QRECURSE
QRECURSE=recurse.mk
ifdef QCONFIG
QRDIR=$(dir $(QCONFIG))
endif
endif
CCFLAGS += -g -DNDEBUG
include $(QRDIR)$(QRECURSE)
