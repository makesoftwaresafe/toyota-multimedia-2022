ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=libiconv
endef


INSTALLDIR=usr/lib

PRE_TARGET= .pre_hinstall

include ../../seedhdr.mk


NAME=charset

NULLSTRING :=
INCVPATH = $(NULLSTRING)
# Need this set for hinstall to do something.
PUBLIC_INCVPATH = $(space)

BLDROOT= $(abspath $(CURDIR)/../../../..)

EXTRA_SRCVPATH+=	$(BLDROOT)/dist/libiconv-1.12/libcharset/lib
EXTRA_INCVPATH+=	$(PRODUCT_ROOT)/libcharset/include \
			$(PRODUCT_ROOT)/libcharset



# Public headers.
INCS=	libcharset.h localcharset.h

CCFLAGS+= -DLIBDIR=\"/usr/lib\" -DBUILDING_LIBCHARSET -DBUILDING_DLL \
	  -DENABLE_RELOCATABLE=1 -DIN_LIBRARY -DNO_XMALLOC \
	  -Dset_relocation_prefix=libcharset_set_relocation_prefix  \
	  -Drelocate=libcharset_relocate -DHAVE_CONFIG_H


SRCS=	localcharset.c relocatable.c


include $(MKFILES_ROOT)/qtargets.mk

clean: .pre_huninstall

# override TARGET_HINSTALL from qmacros.mk
define TARGET_HINSTALL
	@-$(CP_HOST) $(BLDROOT)/dist/libiconv-1.12/libcharset/include/libcharset.h.in $(INSTALL_ROOT_HDR)/libcharset.h
	@-$(CP_HOST) $(BLDROOT)/dist/libiconv-1.12/libcharset/include/localcharset.h.in $(INSTALL_ROOT_HDR)/localcharset.h
endef


# override TARGET_HUNINSTALL from qmacros.mk
define TARGET_HUNINSTALL
	@-$(RM_HOST) $(INSTALL_ROOT_HDR)/libcharset.h
	@-$(RM_HOST) $(INSTALL_ROOT_HDR)/localcharset.h
endef
