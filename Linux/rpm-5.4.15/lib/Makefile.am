# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

LINT = splint
MCCABE = pmccabe

SUBDIRS = auto # tests

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/build \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	-I$(top_srcdir)/misc \
	@WITH_BEECRYPT_CPPFLAGS@ \
	@WITH_DB_CPPFLAGS@ \
	@WITH_FILE_CPPFLAGS@ \
	@WITH_LUA_CPPFLAGS@ \
	@WITH_PCRE_CPPFLAGS@ \
	@WITH_POPT_CPPFLAGS@ \
	@WITH_SYCK_CPPFLAGS@ \
	@WITH_XAR_CPPFLAGS@ \
	@WITH_ZLIB_CPPFLAGS@

AM_CFLAGS = $(OPENMP_CFLAGS)

RPMMISC_LDADD_COMMON = \
	$(top_builddir)/misc/librpmmisc.la \
	@LTLIBINTL@
RPMIO_LDADD_COMMON = \
	$(top_builddir)/rpmio/librpmio.la \
	$(RPMMISC_LDADD_COMMON)
RPMDB_LDADD_COMMON = \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(RPMIO_LDADD_COMMON)
RPM_LDADD_COMMON = \
	$(top_builddir)/lib/librpm.la \
	$(RPMDB_LDADD_COMMON)
RPMBUILD_LDADD_COMMON = \
	$(top_builddir)/build/librpmbuild.la \
	$(RPM_LDADD_COMMON)

EXTRA_DIST = libsql.c librpm.vers

EXTRA_PROGRAMS =
noinst_PROGRAMS =

EXTRA_PROGRAMS += tevr tgi tsbt

#XYZ:
#	mkdir -p $@
#
#XYZ/%.cpp: %.c
#	@echo $(SUFFIXES)
#	@cp $< $@

#pkglibdir = @USRLIBRPM@
#pkglib_LTLIBRARIES = libsql.la

pkgincdir = $(pkgincludedir)$(WITH_PATH_VERSIONED_SUFFIX)
pkginc_HEADERS = \
	rpmcli.h rpmversion.h rpmds.h rpmfi.h rpmgi.h rpmps.h \
	rpmrc.h rpmte.h rpmts.h rpm4compat.h rpm46compat.h
noinst_HEADERS = \
	filetriggers.h fs.h fsm.h manifest.h misc.h psm.h rpmal.h \
	rpmfc.h rpmlib.h rpmlock.h rpmluaext.h rpmrollback.h

usrlibdir = $(libdir)
usrlib_LTLIBRARIES = librpm.la
librpm_la_SOURCES = \
	formats.c fsm.c order.c poptI.c psm.c \
	depends.c \
	filetriggers.c \
	fs.c \
	manifest.c \
	misc.c \
	poptALL.c \
	poptQV.c \
	query.c \
	rpmal.c \
	rpmchecksig.c \
	rpmds.c \
	rpmfc.c \
	rpmfi.c \
	rpmgi.c \
	rpminstall.c \
	rpmlock.c \
	rpmluaext.c \
	rpmps.c \
	rpmrc.c \
	rpmrollback.c \
	rpmte.c \
	rpmts.c \
	rpmversion.c \
	transaction.c \
	verify.c

librpm_la_LDFLAGS = -release $(LT_CURRENT).$(LT_REVISION)
if HAVE_LD_VERSION_SCRIPT
librpm_la_LDFLAGS += -Wl,@LD_VERSION_SCRIPT_FLAG@,@top_srcdir@/lib/librpm.vers
endif
librpm_la_LIBADD =
if ENABLE_BUILD_INTLIBDEP
librpm_la_LIBADD += \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(top_builddir)/rpmio/librpmio.la \
	$(top_builddir)/misc/librpmmisc.la
endif
if ENABLE_BUILD_MAXEXTLIBDEP
librpm_la_LDFLAGS += $(LDFLAGS)
librpm_la_LIBADD  += $(LIBS)
endif
librpm.la: $(librpm_la_OBJECTS) $(librpm_la_DEPENDENCIES) 
	$(librpm_la_LINK) -rpath $(usrlibdir) $(librpm_la_OBJECTS) $(librpm_la_LIBADD)

#libsql_la_SOURCES	= libsql.c
#libsql_la_LIBADD	= librpm.la $(RPMDB_LDADD_COMMON)

install-data-hook:
if !ENABLE_BUILD_LAFILES
	-for l in $(usrlib_LTLIBRARIES); do \
	    rm -f $(DESTDIR)$(usrlibdir)/$${l}; \
	done
endif
if WITH_PATH_VERSIONED
	-for l in $(usrlib_LTLIBRARIES); do \
	    base=`echo "$${l}" | sed -e 's;\.la$$;;'`; \
	    if [ -f $(DESTDIR)$(usrlibdir)/$${base}.a ]; then \
	        mv $(DESTDIR)$(usrlibdir)/$${base}.a \
	           $(DESTDIR)$(usrlibdir)/$${base}-$(VERSION).a; \
	    fi; \
	done
endif

check-local:

BUILT_SOURCES = # rpmlib.lcd

rpmlib.lcd: Makefile.am ${librpm_la_SOURCES} ${pkginc_HEADERS} ${noinst_HEADERS}
	-lclint ${DEFS} ${INCLUDES} ${librpm_la_SOURCES} -dump $@ 2>/dev/null

.PHONY:	sources
sources:
	@echo $(librpm_la_SOURCES:%=lib/%)

.PHONY:	lint
lint:
	$(LINT) $(DEFS) $(INCLUDES) $(librpm_la_SOURCES)

.PHONY:	mccabe
mccabe:
	$(MCCABE) $(librpm_la_SOURCES) | sort -n -r | head -n 20

.PHONY:	lcov-reset	# run lcov from scratch, always
lcov-reset:
	make lcov-run
	make lcov-report

.PHONY:	lcov		# run lcov from scratch if the dir is not there
lcov:
	make lcov-reset

.PHONY:	lcov-run	# reset run coverage tests
lcov-run:
	@-rm -rf lcov
	find . -name "*.gcda" -exec rm {} \;
	make check

.PHONY:	lcov-report	# generate report based on current coverage data
lcov-report:
	mkdir lcov
	lcov --directory . --capture --output-file lcov/lcov.info
	lcov -l lcov/lcov.info | grep -v "`cd $(top_srcdir) && pwd`" | cut -d: -f1 > lcov/remove
	lcov -r lcov/lcov.info `cat lcov/remove` > lcov/lcov.cleaned.info
	rm lcov/remove
	mv lcov/lcov.cleaned.info lcov/lcov.info
	genhtml -t "$(PACKAGE_STRING)" -o lcov lcov/lcov.info

#.PHONY:	lcov-upload
#lcov-upload: lcov
#	rsync -rvz -e ssh --delete lcov/* ???

tevr_SOURCES = tevr.c
tevr_DEPENDENCIES = librpm.la
tevr_LDADD = $(RPM_LDADD_COMMON)

tgi_SOURCES = tgi.c
tgi_DEPENDENCIES = librpm.la
tgi_LDADD = $(RPM_LDADD_COMMON)

tsbt_SOURCES = tsbt.c
tsbt_DEPENDENCIES = librpm.la
tsbt_LDADD = $(RPM_LDADD_COMMON)
