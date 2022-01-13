# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

LINT = splint

EXTRA_DIST = system-py.h rpmdebug-py.c rpm/__init__.py

AM_CPPFLAGS = -I. \
	-I$(top_srcdir) \
	-I$(top_srcdir)/build \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	-I$(top_srcdir)/misc \
	@WITH_DB_CPPFLAGS@ \
	@WITH_LUA_CPPFLAGS@ \
	@WITH_FILE_CPPFLAGS@ \
	@WITH_PCRE_CPPFLAGS@ \
	@WITH_POPT_CPPFLAGS@ \
	@WITH_XAR_CPPFLAGS@ \
	@WITH_ZLIB_CPPFLAGS@ \
	-I@WITH_PYTHON_INCDIR@

noinst_HEADERS = header-py.h \
	rpmal-py.h rpmds-py.h rpmdb-py.h rpmfd-py.h rpmfts-py.h \
	rpmfi-py.h rpmkeyring-py.h rpmmacro-py.h rpmmi-py.h \
	rpmps-py.h rpmte-py.h rpmts-py.h \
	spec-py.h

pkgpyexec_LTLIBRARIES = _rpmmodule.la
pkgpyexec_SCRIPTS = rpm/__init__.py

_rpmmodule_la_CFLAGS = -fno-strict-aliasing
_rpmmodule_la_LDFLAGS = -module -avoid-version
_rpmmodule_la_LIBADD = \
	$(top_builddir)/build/librpmbuild.la \
	$(top_builddir)/lib/librpm.la \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(top_builddir)/rpmio/librpmio.la \
	$(top_builddir)/misc/librpmmisc.la \
	@LTLIBINTL@ \
	-lpython$(PYTHON_VERSION)

_rpmmodule_la_SOURCES = \
	rpmmodule.c header-py.c \
	rpmal-py.c rpmds-py.c rpmdb-py.c rpmfd-py.c rpmfts-py.c\
	rpmfi-py.c rpmkeyring-py.c rpmmacro-py.c rpmmi-py.c rpmps-py.c \
	rpmtd-py.c rpmte-py.c rpmts-py.c spec-py.c

# rpmmodule.c header-py.c \
# 	rpmal-py.c rpmds-py.c rpmdb-py.c rpmfd-py.c rpmfi-py.c rpmmi-py.c \
# 	rpmmacro-py.c rpmps-py.c rpmte-py.c rpmts-py.c
# rpmmodule.c header-py.c
splint_srcs = \
 	rpmal-py.c rpmds-py.c rpmdb-py.c rpmfd-py.c rpmfts-py.c \
	rpmfi-py.c rpmkeyring-py.c rpmmacro-py.c rpmmi-py.c rpmps-py.c \
	rpmtd-py.c rpmte-py.c rpmts-py.c spec-py.c

.PHONY:	lint
lint:
	$(LINT) $(DEFS) $(INCLUDES) $(splint_srcs)

# FIXME: using .libs/ is not really correct, but whatever
#check-local:
#	@export PYTHONPATH=$(PYTHONPATH):$(shell pwd):$(shell pwd)/.libs/ && \
#	cd test && python test_rpm.py
