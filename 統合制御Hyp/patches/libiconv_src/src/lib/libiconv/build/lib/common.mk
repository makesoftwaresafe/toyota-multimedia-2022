ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=libiconv
endef


INSTALLDIR=usr/lib

NAME=iconv

NULLSTRING :=
INCVPATH = $(NULLSTRING)
# Need this set for hinstall to do something.
#PUBLIC_INCVPATH = $(space)

BLDROOT= $(abspath $(CURDIR)/../../../..)


EXTRA_SRCVPATH+=	$(BLDROOT)/dist/libiconv-1.12/lib		\
			$(BLDROOT)/dist/libiconv-1.12/libcharset/lib
EXTRA_INCVPATH+=	$(PRODUCT_ROOT)/lib

CCFLAGS+= -DLIBDIR=\"/usr/lib\" -DBUILDING_LIBICONV -DBUILDING_DLL \
	  -DENABLE_RELOCATABLE=1 -DIN_LIBRARY -DNO_XMALLOC \
	  -Dset_relocation_prefix=libiconv_set_relocation_prefix  \
	  -Drelocate=libiconv_relocate -DHAVE_CONFIG_H



SRCS=	iconv.c localcharset.c relocatable.c


include $(MKFILES_ROOT)/qtargets.mk
