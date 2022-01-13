# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

LINT = splint
MCCABE = pmccabe
DB_SQL_CODEGEN = ../db3/db_sql_codegen

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
	@WITH_XAR_CPPFLAGS@ \
	@WITH_ZLIB_CPPFLAGS@

AM_CFLAGS = $(OPENMP_CFLAGS)

EXTRA_DIST = \
	db3.c sqlite.c db_emu.h librpmdb.vers bdb.sql libsqldb.c \
	logio.awk logio.src logio_recover_template logio_template logio.c \
	logio_rec.c logio_auto.c logio_autop.c logio_auto.h

EXTRA_PROGRAMS = logio tjfn # tbdb

RPMMISC_LDADD_COMMON = \
	$(top_builddir)/misc/librpmmisc.la \
	@LTLIBICONV@ \
	@LTLIBINTL@

RPMIO_LDADD_COMMON = \
	$(top_builddir)/rpmio/librpmio.la \
	$(RPMMISC_LDADD_COMMON)

RPMDB_LDADD_COMMON = \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(RPMIO_LDADD_COMMON)

pkgincdir = $(pkgincludedir)$(WITH_PATH_VERSIONED_SUFFIX)
pkginc_HEADERS = pkgio.h rpmdb.h rpmevr.h rpmns.h rpmtag.h rpmtypes.h
noinst_HEADERS = \
	fprint.h header_internal.h legacy.h rpmdpkg.h rpmlio.h rpmmdb.h \
	rpmrepo.h rpmtd.h rpmtxn.h rpmwf.h signature.h

#pkglibdir =		@USRLIBRPM@
#pkglib_LTLIBRARIES =	libsqldb.la

mylibs = \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(top_builddir)/misc/librpmmisc.la

logio_LSOURCES = # logio_rec.c $(logio_BUILT)

#XYZ:
#	mkdir -p $@
#
#XYZ/%.cpp: %.c
#	@echo $(SUFFIXES)
#	@cp $< $@

usrlibdir = $(libdir)
usrlib_LTLIBRARIES = librpmdb.la
# XXX rpmrepo.c needs to include templates from scripts/*
# XXX $(LOGIO_LSOURCES) needs access to db_int.h w db-5.1.19 and older
librpmdb_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/db -I$(top_builddir)/db \
	-I$(top_srcdir)/db/src -I$(top_builddir)/db/src \
	-I$(top_srcdir)/scripts -I$(top_builddir)/scripts \
	$(CPPFLAGS)
librpmdb_la_SOURCES = hdrfmt.c rpmrepo.c tagtbl.c \
	dbconfig.c \
	fprint.c \
	hdrNVR.c \
	header.c \
	header_internal.c \
	legacy.c \
	merge.c \
	package.c \
	pkgio.c \
	poptDB.c \
	rpmdb.c \
	rpmdpkg.c \
	rpmevr.c \
	rpmlio.c \
	rpmmdb.c \
	rpmns.c \
	rpmtd.c \
	rpmtxn.c \
	rpmwf.c \
	signature.c \
	tagname.c \
	$(logio_LSOURCES)

librpmdb_la_LDFLAGS = -release $(LT_CURRENT).$(LT_REVISION)
if HAVE_LD_VERSION_SCRIPT
librpmdb_la_LDFLAGS += -Wl,@LD_VERSION_SCRIPT_FLAG@,@top_srcdir@/rpmdb/librpmdb.vers
endif
librpmdb_la_LIBADD = $(DBLIBOBJS)
if ENABLE_BUILD_INTLIBDEP
librpmdb_la_LIBADD += \
	$(top_builddir)/rpmio/librpmio.la \
	$(top_builddir)/misc/librpmmisc.la
endif
if ENABLE_BUILD_MAXEXTLIBDEP
librpmdb_la_LDFLAGS += $(LDFLAGS)
librpmdb_la_LIBADD  += $(LIBS)
endif
librpmdb_la_DEPENDENCIES = $(DBLIBOBJS)
librpmdb.la: $(librpmdb_la_OBJECTS) $(librpmdb_la_DEPENDENCIES) 
	$(librpmdb_la_LINK) -rpath $(usrlibdir) $(librpmdb_la_OBJECTS) $(librpmdb_la_LIBADD)

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

tagtbl.c: Makefile $(top_srcdir)/rpmdb/rpmtag.h 
	@echo '#include "system.h"' > $@
	@echo '#define _RPMTAG_INTERNAL' >> $@
	@echo '#include <rpmtag.h>' >> $@
	@echo '#include "debug.h"' >> $@
	@echo '' >> $@
	@echo '/*@access headerTagTableEntry @*/' >> $@
	@echo '' >> $@
	@echo '/*@observer@*/ /*@unchecked@*/' >> $@
	@echo 'static const struct headerTagTableEntry_s _rpmTagTable[] = {' >> $@
	${AWK} '/[ \t](RPMTAG_[A-Z0-9]*)[ \t]+([0-9]*)/ && !/internal/ {tt = "NULL"; ta = "ANY"; if ($$5 == "c") {tt = "UINT8"; ta = "SCALAR"} if ($$5 == "c[]") {tt = "UINT8"; ta = "ARRAY"} if ($$5 == "h") {tt = "UINT16"; ta = "SCALAR"} if ($$5 == "h[]") {tt = "UINT16"; ta = "ARRAY"} if ($$5 == "i") {tt = "UINT32"; ta = "SCALAR"} if ($$5 == "i[]") {tt = "UINT32"; ta = "ARRAY"} if ($$5 == "l") {tt = "UINT64"; ta = "SCALAR"} if ($$5 == "l[]") {tt = "UINT64"; ta = "ARRAY"} if ($$5 == "s") {tt = "STRING"; ta = "SCALAR"} if ($$5 == "s[]") {tt = "STRING_ARRAY"; ta = "ARRAY"} if ($$5 == "s{}") {tt = "I18NSTRING"; ta = "SCALAR"} if ($$5 == "x") {tt = "BIN"; ta = "SCALAR"} if ($$2 == "=") { printf("\t{ \"%s\",	%s	RPM_%s_TYPE + RPM_%s_RETURN_TYPE },\n", $$1, $$3, tt, ta) } else { printf("\t{ \"%s\",	%s,	RPM_%s_TYPE + RPM_%s_RETURN_TYPE  },\n", $$2, $$3, tt, ta) } }' < ${top_srcdir}/rpmdb/rpmtag.h | LC_ALL=C sort >> $@
	@echo '	{ NULL, 0, 0 }' >> $@
	@echo '};' >> $@
	@echo '' >> $@
	@echo '/*@observer@*/ /*@unchecked@*/' >> $@
	@echo 'headerTagTableEntry rpmTagTable = _rpmTagTable;' >> $@
	@echo '' >> $@
	@echo '/*@unchecked@*/' >> $@
	@echo 'int rpmTagTableSize = sizeof(_rpmTagTable) / sizeof(_rpmTagTable[0]) - 1;' >> $@

varlibrpm =	$(varprefix)/lib/rpm
varlibwdj =	$(varprefix)/lib/wdj
varcacherpm =	$(varprefix)/cache/rpm
varcachewdj =	$(varprefix)/cache/wdj
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(varlibrpm)
	$(mkinstalldirs) $(DESTDIR)$(varlibrpm)/log
	$(mkinstalldirs) $(DESTDIR)$(varlibrpm)/tmp
	[ -f $(DESTDIR)$(varlibrpm)/DB_CONFIG ] || $(install_sh_DATA) DB_CONFIG $(DESTDIR)$(varlibrpm)
	$(mkinstalldirs) $(DESTDIR)$(varlibwdj)
	$(mkinstalldirs) $(DESTDIR)$(varlibwdj)/git
	$(mkinstalldirs) $(DESTDIR)$(varlibwdj)/svn
	$(mkinstalldirs) $(DESTDIR)$(varlibwdj)/sql
	$(mkinstalldirs) $(DESTDIR)$(varlibwdj)/l10n
	$(mkinstalldirs) $(DESTDIR)$(varcacherpm)
	$(mkinstalldirs) $(DESTDIR)$(varcachewdj)
	$(mkinstalldirs) $(DESTDIR)$(pkgincdir)

check-local:

clean-local:
	rm -f *.o tagtbl.c

uninstall-local:
	rm -f $(DESTDIR)$(varlibrpm)/DB_CONFIG

splint_SRCS = \
	dbconfig.c fprint.c \
	hdrfmt.c hdrNVR.c header.c header_internal.c legacy.c merge.c \
	pkgio.c poptDB.c rpmdb.c rpmdpkg.c rpmevr.c rpmlio.c rpmns.c rpmtd.c \
	rpmtxn.c rpmwf.c signature.c tagname.c tagtbl.c

rpmdb.lcd: Makefile.am ${splint_SRCS} ${pkginc_HEADERS} ${noinst_HEADERS}
	-splint ${DEFS} ${INCLUDES} ${splint_SRCS} -dump $@ 2>/dev/null

.PHONY:	sources
sources:
	@echo $(splint_SRCS:%=rpmdb/%)

.PHONY:	lint
lint:
	$(LINT) $(DEFS) $(INCLUDES) $(splint_SRCS) $(DBLIBSRCS)

.PHONY:	mccabe
mccabe:
	$(MCCABE) $(librpmdb_la_SOURCES) $(DBLIBSRCS) | sort -n -r | head -n 10

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

logio_BUILT = logio_auto.c logio_autop.c logio_auto.h logio_template
$(logio_BUILT):	logio.awk logio.src logio_recover_template
	@test -e $(builddir)/logio_recover_template || \
		cp -p $(srcdir)/logio_recover_template \
		      $(builddir)/logio_recover_template
	@rm -f $(logio_BUILT)
	@$(AWK) -f $(srcdir)/logio.awk \
		-v header_file=logio_auto.h \
		-v print_file=logio_autop.c \
		-v source_file=logio_auto.c \
		-v template_file=logio_template \
	< $(srcdir)/logio.src

logio_SOURCES = logio.c logio.h
logio_LDADD = $(mylibs)

#libsqldb_la_SOURCES	= libsqldb.c # sqlite.c
#libsqldb_la_LIBADD	= $(RPMIO_LDADD_COMMON)

BUILT_SOURCES = tagtbl.c $(logio_BUILT)
if WITH_DBSQL_CODEGEN
#BUILT_SOURCES += tbdb.c bdb.c bdb.h
endif

tjfn_SOURCES = tjfn.c
tjfn_LDADD = $(mylibs)

#tbdb_SOURCES = tbdb.c bdb.c
#tbdb_LDADD = $(mylibs)

#tbdb.c bdb.c bdb.h: bdb.sql
#	$(DB_SQL_CODEGEN) -i bdb.sql -t tbdb.c

tdbi: tdbi.o librpmdb.la
	$(LINK) @LDFLAGS_STATIC@ -o $@ $< $(mylibpaths) $(mylibs) $(LIBS)

tdpkg: tdpkg.o librpmdb.la
	$(LINK) @LDFLAGS_STATIC@ -o $@ $< $(mylibpaths) $(mylibs) $(LIBS)

tmpkg: tmpkg.o librpmdb.la
	$(LINK) @LDFLAGS_STATIC@ -o $@ $< $(mylibpaths) $(mylibs) $(LIBS)

tsql: tsql.o librpmdb.la
	$(LINK) @LDFLAGS_STATIC@ -o $@ $< $(mylibpaths) $(mylibs) $(LIBS)

tpkgadd: tpkgadd.o librpmdb.la
	$(LINK) @LDFLAGS_STATIC@ -o $@ $< $(mylibpaths) $(mylibs) $(LIBS)

trepomd: trepomd.o librpmdb.la
	$(LINK) @LDFLAGS_STATIC@ -o $@ $< $(mylibpaths) $(mylibs) $(LIBS)

ttag: ttag.o librpmdb.la
	$(LINK) @LDFLAGS_STATIC@ -o $@ $< $(mylibpaths) $(mylibs) $(LIBS)
