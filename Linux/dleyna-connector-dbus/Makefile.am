connectordir = $(libdir)/dleyna-1.0/connectors

AM_CFLAGS =	$(GLIB_CFLAGS)				\
		$(GIO_CFLAGS)				\
		$(DLEYNA_CORE_CFLAGS)			\
		-include config.h
		
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

connector_LTLIBRARIES = libdleyna-connector-dbus.la

libdleyna_connector_dbus_la_LDFLAGS =	-avoid-version	\
					-no-undefined	\
					-module


libdleyna_connector_dbus_la_SOURCES =	src/connector-dbus.c

libdleyna_connector_dbus_la_LIBADD =	$(GLIB_LIBS)		\
					$(GIO_LIBS)		\
					$(DLEYNA_CORE_LIBS)

MAINTAINERCLEANFILES =	Makefile.in		\
			aclocal.m4		\
			configure		\
			config.h.in		\
			config.h.in~		\
			build-aux/depcomp	\
			build-aux/compile	\
			build-aux/missing	\
			build-aux/install-sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dleyna-connector-dbus-1.0.pc

CLEANFILES = $(pkgconfig_DATA)
DISTCLEANFILES = $(pkgconfig_DATA)

maintainer-clean-local:
	rm -rf build-aux
