include $(top_srcdir)/Makefile.decl

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = libmediaart docs

if HAVE_UNIT_TESTS
SUBDIRS += tests
endif

pkgconfig_DATA = libmediaart-@LIBMEDIAART_API_VERSION@.pc
pkgconfigdir = $(libdir)/pkgconfig

libmediaart-$(LIBMEDIAART_API_VERSION).pc: libmediaart.pc
	cp $< $@

EXTRA_DIST += \
	libmediaart.doap \
	libmediaart.pc.in \
        tap-driver.sh \
        tap-test

CLEANFILES += \
	libmediaart-@LIBMEDIAART_API_VERSION@.pc

DISTCHECK_CONFIGURE_FLAGS =                            \
	--enable-unit-tests                            \
	--enable-introspection                         \
	--enable-gtk-doc

