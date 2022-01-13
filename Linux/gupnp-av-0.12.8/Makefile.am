SUBDIRS = libgupnp-av tests doc vala data

pkgconfig_DATA= gupnp-av-1.0.pc
pkgconfigdir = $(libdir)/pkgconfig

ACLOCAL_AMFLAGS=-I m4 ${ACLOCAL_FLAGS}

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing config.h.in
