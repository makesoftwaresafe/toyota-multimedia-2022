# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

LINT = splint
MCCABE = pmccabe

SUBDIRS = auto # tests

EXTRA_DIST = \
	fnmatch_loop.c getdate.y html-parse.c html-parse.h libsqlio.c \
	rpmcpio.c rpmcpio.h rpmgenbasedir.c rpmgenpkglist.c rpmgensrclist.c \
	rpmjsio.msg rpmtar.c rpmtar.h \
	tdir.c tfts.c tget.c tgit.c tglob.c thkp.c thtml.c tinv.c tkey.c \
	tmire.c todbc.c tput.c tpython.c trpmio.c tsexp.c tsvn.c tsw.c tybi.c \
	lookup3.c librpmio.vers testit.sh

EXTRA_PROGRAMS = rpmcpio rpmdpkg rpmtar rpmz
EXTRA_PROGRAMS += bdes thtml tinv tkey tmacro tpw
noinst_PROGRAMS =

EXTRA_PROGRAMS += bsdiff bspatch rpmborg rpmcurl \
	rpmgenbasedir rpmgenpkglist rpmgensrclist rpmgpg \
	rpmpbzip2 rpmpigz \
	tasn tbf tder tdir tfts tget tglob thkp tmagic \
	tmire todbc toid tperl tpython tput trpmio tsexp tsvn tsw ttcl tybi \
	dumpasn1 lookup3

noinst_PROGRAMS += b2sum tset tblake2b tblake2bp tblake2s tblake2sp
if WITH_LIBGIT2
noinst_PROGRAMS += tgit
else
EXTRA_PROGRAMS += tgit
endif 
if WITH_LIBTPM
noinst_PROGRAMS += ttpm
else
EXTRA_PROGRAMS += ttpm
endif 
if WITH_UNIXODBC
noinst_PROGRAMS += todbc
else
EXTRA_PROGRAMS += todbc
endif 
bin_PROGRAMS =
man_MANS =

BUILT_SOURCES = \
	getdate.c # rpmio.lcd

#XYZ:
#	mkdir -p $@
#
#XYZ/%.cpp: %.c
#	@echo $(SUFFIXES)
#	@cp $< $@

TESTS = tblake2b tblake2bp tblake2s tblake2sp
XFAIL_TESTS =
check_PROGRAMS = # tblake2b tblake2bp tblake2s tblake2sp
check_SCRIPTS =
dist_noinst_SCRIPTS = gengpg.sh testit.sh

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
	@WITH_DBSQL_CPPFLAGS@ \
	@WITH_FICL_CPPFLAGS@ \
	@WITH_FILE_CPPFLAGS@ \
	@WITH_GPSEE_CPPFLAGS@ \
	@WITH_LIBTPM_CPPFLAGS@ \
	@WITH_LIBGIT2_CPPFLAGS@ \
	@WITH_LUA_CPPFLAGS@ \
	@WITH_NEON_CPPFLAGS@ \
	@WITH_PCRE_CPPFLAGS@ \
	@WITH_POPT_CPPFLAGS@ \
	@WITH_RUBY_CPPFLAGS@ \
	@WITH_SYCK_CPPFLAGS@ \
	@WITH_XAR_CPPFLAGS@ \
	@WITH_XZ_CPPFLAGS@ \
	@WITH_ZLIB_CPPFLAGS@

AM_CFLAGS = $(OPENMP_CFLAGS)

RPMIO_LDADD_COMMON = \
	librpmio.la \
	$(top_builddir)/misc/librpmmisc.la \
	@LTLIBICONV@ \
	@LTLIBINTL@

RPM_LDADD_COMMON = \
	$(top_builddir)/build/librpmbuild.la \
	$(top_builddir)/lib/librpm.la \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(RPMIO_LDADD_COMMON)

vg =            $(LIBTOOL) --mode=execute vg

#pkglibdir = @USRLIBRPM@
#pkglib_LTLIBRARIES = libsqlio.la

pkgincdir = $(pkgincludedir)$(WITH_PATH_VERSIONED_SUFFIX)
pkginc_HEADERS = argv.h mire.h rpmzlog.h yarn.h \
	rpmbf.h rpmcb.h rpmio.h rpmlog.h rpmiotypes.h rpmmacro.h rpmpgp.h rpmsw.h
noinst_HEADERS = \
	ar.h bson.h cpio.h crc.h envvar.h fnmatch.h fts.h glob.h iosm.h \
	blake2.h blake2-impl.h blake2-rpm.h blake2-kat.h \
	arirang.h blake.h bmw.h chi.h cubehash.h echo.h edon-r.h fugue.h \
	groestl.h hamsi.h jh.h keccak.h lane.h luffa.h md2.h md6.h mongo.h \
	radiogatun.h \
	salsa10.h salsa20.h shabal.h shavite3.h simd.h skein.h tib3.h tiger.h \
	poptIO.h rpmacl.h rpmasn.h rpmaug.h rpmbag.h rpmbc.h rpmbz.h \
	rpmcdsa.h rpmcudf.h rpmcvs.h rpmdav.h rpmdir.h rpmficl.h rpmgc.h rpmgit.h rpmhash.h \
	rpmhkp.h rpmhook.h rpmio_internal.h rpmjni.h rpmjs.h rpmjsio.h \
	rpmkeyring.h \
	rpmku.h rpmltc.h rpmlua.h rpmmg.h rpmmrb.h rpmnix.h rpmnss.h \
	rpmodbc.h rpmperl.h rpmpython.h rpmruby.h rpmsm.h rpmsp.h \
	rpmsq.h rpmsql.h rpmsquirrel.h rpmssl.h rpmsvn.h rpmsx.h rpmsyck.h \
	rpmtcl.h rpmtpm.h rpmurl.h rpmuuid.h rpmxar.h rpmz.h rpmzq.h \
	set.h tar.h ugid.h rpmio-stub.h

usrlibdir = $(libdir)
usrlib_LTLIBRARIES = librpmio.la

librpmio_la_SOURCES = \
	getdate.c \
	blake2b.c blake2bp.c blake2s.c blake2sp.c blake2-rpm.c \
	arirang.c blake.c bmw.c chi.c cubehash.c echo.c edon-r.c fugue.c \
	groestl.c hamsi.c jh.c keccak.c lane.c luffa.c md2.c md6.c radiogatun.c\
	salsa10.c salsa20.c shabal.c shavite3.c simd.c skein.c tib3.c tiger.c \
	rpmgit.c rpmio-stub.c \
	rpmjs.c rpmjsio.c rpmkeyring.c \
	rpmnix.c rpmodbc.c rpmsql.c set.c \
	ar.c \
	argv.c \
	bson.c \
	bzdio.c \
	cipher.c \
	cpio.c \
	crc.c \
	digest.c \
	fnmatch.c \
	fts.c \
	glob.c \
	gzdio.c \
	iosm.c \
	lsyck.c \
	macro.c \
	mire.c \
	mongo.c \
	mount.c \
	poptIO.c \
	rpmacl.c \
	rpmasn.c \
	rpmaug.c \
	rpmbag.c \
	rpmbc.c \
	rpmbf.c \
	rpmcdsa.c \
	rpmcudf.c \
	rpmcvs.c \
	rpmdav.c \
	rpmdir.c \
	rpmficl.c \
	rpmgc.c \
	rpmhash.c \
	rpmhkp.c \
	rpmhook.c \
	rpmio.c \
	rpmiob.c \
	rpmjni.cc \
	rpmku.c \
	rpmlog.c \
	rpmltc.c \
	rpmlua.c \
	rpmmalloc.c \
	rpmmg.c \
	rpmmrb.c \
	rpmnss.c \
	rpmperl.c \
	rpmpgp.c \
	rpmpython.c \
	rpmrpc.c \
	rpmruby.c \
	rpmsm.c \
	rpmsp.c \
	rpmsq.c \
	rpmsquirrel.c \
	rpmssl.c \
	rpmsvn.c \
	rpmsw.c \
	rpmsx.c \
	rpmsyck.c \
	rpmtcl.c \
	rpmtpm.c \
	rpmuuid.c \
	rpmxar.c \
	rpmzlog.c \
	rpmzq.c \
	strcasecmp.c \
	strtolocale.c \
	tar.c \
	ugid.c \
	url.c \
	xzdio.c \
	yarn.c

librpmio_la_LDFLAGS = -release $(LT_CURRENT).$(LT_REVISION)
if HAVE_LD_VERSION_SCRIPT
librpmio_la_LDFLAGS += -Wl,@LD_VERSION_SCRIPT_FLAG@,@top_srcdir@/rpmio/librpmio.vers
endif
librpmio_la_LIBADD = -lm
if ENABLE_BUILD_INTLIBDEP
librpmio_la_LIBADD += \
	$(top_builddir)/misc/librpmmisc.la
endif
if ENABLE_BUILD_MAXEXTLIBDEP
librpmio_la_LDFLAGS += $(LDFLAGS)
#librpmio_la_LIBADD  += $(LIBS)
endif
#librpmio.la: $(librpmio_la_OBJECTS) $(librpmio_la_DEPENDENCIES) 
#	$(librpmio_la_LINK) -rpath $(usrlibdir) $(librpmio_la_OBJECTS) $(librpmio_la_LIBADD)

#libsqlio_la_SOURCES	= libsqlio.c
#libsqlio_la_LIBADD	= $(RPMIO_LDADD_COMMON)

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

getdate.c: getdate.y
	@echo expect 10 shift/reduce conflicts
	$(YACC) $(srcdir)/getdate.y
	-@if test -f y.tab.c; then \
	 { echo "/*@-globstate -internalglobs -statictrans -unqualifiedtrans -noparams @*/";\
	   echo "/*@-retvalint -usedef -varuse -nullderef -nullassign @*/";\
	   echo "/*@-readonlytrans -modunconnomods -compdef -noeffectuncon @*/";\
	   echo "/*@-globs -evalorderuncon -modobserveruncon -modnomods @*/";\
	   echo "/*@-noeffect -branchstate -sizeoftype -usereleased @*/";\
	   echo "/*@+charint -exportheader +longunsignedintegral -redecl -type -warnlintcomments @*/";\
	   echo "/*@unused@*/";\
	   sed	-e 's,y.tab.c,getdate.c,' y.tab.c \
		-e 's,^YYSTYPE ,static &,' \
		-e 's,^short ,static &,' \
		-e 's,^const short ,static &,' \
		-e 's,^int yydebug,/*@unused@*/ static &,' \
		-e 's,^int ,static &,' ;\
	   echo "/*@=charint =exportheader =longunsignedintegral =redecl =type =warnlintcomments @*/";\
	   echo "/*@=noeffect =branchstate =sizeoftype =usereleased @*/";\
	   echo "/*@=globs =evalorderuncon =modobserveruncon =modnomods @*/";\
	   echo "/*@=readonlytrans =modunconnomods =compdef =noeffectuncon @*/";\
	   echo "/*@=retvalint =usedef =varuse =nullderef =nullassign @*/";\
	   echo "/*@=globstate =internalglobs =statictrans =unqualifiedtrans =noparams @*/";\
	 } > getdate.c ;\
	   rm -f y.tab.c; \
	else \
	   if test -f getdate.tab.c ; then \
	       mv getdate.tab.c getdate.c ; \
	   else \
	       echo '*** Unable to create getdate.c' ;\
	   fi ;\
	fi

rpmio.lcd: Makefile.am ${librpmio_la_SOURCES} ${pkginc_HEADERS} ${noinst_HEADERS}
	-lclint -load ../beecrypt/beecrypt.lcd $(DEFS) $(INCLUDES) $(librpmio_la_SOURCES) -dump $@ 2>/dev/null

.PHONY:	sources
sources:
	@echo $(librpmio_la_SOURCES:%=rpmio/%)

.PHONY:	lint
lint:
	$(LINT) $(DEFS) $(INCLUDES) $(librpmio_la_SOURCES)

.PHONY:        mccabe
mccabe:
	$(MCCABE) $(librpmio_la_SOURCES) | sort -n -r | head -n 20

.PHONY:	lint_rpmcpio
lint_rpmcpio:
	$(LINT) -f .splintrc_rpmcpio $(DEFS) $(INCLUDES) rpmcpio.c

.PHONY:	lint_rpmpbzip2
lint_rpmpbzip2:
	$(LINT) -f .splintrc_rpmpbzip2 $(DEFS) $(INCLUDES) rpmpbzip2.c

.PHONY:	lint_rpmpigz
lint_rpmpigz:
	$(LINT) -f .splintrc_rpmpigz $(DEFS) $(INCLUDES) rpmpigz.c

.PHONY:	lint_rpmtar
lint_rpmtar:
	$(LINT) -f .splintrc_rpmtar $(DEFS) $(INCLUDES) rpmtar.c

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

#TESTS += RunGrepTest
#dist_noinst_SCRIPTS += RunGrepTest

#EXTRA_DIST += \
#  testdata/grepinput \
#  testdata/grepinput8 \
#  testdata/grepinputv \
#  testdata/grepinputx \
#  testdata/greplist \
#  testdata/grepoutput \
#  testdata/grepoutput8 \
#  testdata/grepoutputN \
#  testdata/testinput1 \
#  testdata/testinput2 \
#  testdata/testinput3 \
#  testdata/testinput4 \
#  testdata/testinput5 \
#  testdata/testinput6 \
#  testdata/testinput7 \
#  testdata/testinput8 \
#  testdata/testinput9 \
#  testdata/testinput10 \
#  testdata/testoutput1 \
#  testdata/testoutput2 \
#  testdata/testoutput3 \
#  testdata/testoutput4 \
#  testdata/testoutput5 \
#  testdata/testoutput6 \
#  testdata/testoutput7 \
#  testdata/testoutput8 \
#  testdata/testoutput9 \
#  testdata/testoutput10 \
#  testdata/wintestinput3 \
#  testdata/wintestoutput3

b2sum_SOURCES = b2sum.c
b2sum_LDADD = $(RPMIO_LDADD_COMMON)

bdes_SOURCES =	bdes.c
bdes_LDADD =	$(RPMIO_LDADD_COMMON)

bsdiff_SOURCES = bsdiff.c
bsdiff_LDADD = $(RPMIO_LDADD_COMMON)

bspatch_SOURCES = bspatch.c
bspatch_LDADD = $(RPMIO_LDADD_COMMON)

rpmborg_SOURCES = rpmborg.c
rpmborg_LDADD = $(RPMIO_LDADD_COMMON)

rpmcpio_SOURCES = rpmcpio.c
rpmcpio_LDADD = $(RPM_LDADD_COMMON)

rpmcurl_SOURCES = rpmcurl.c
rpmcurl_LDADD = $(RPMIO_LDADD_COMMON)

rpmdpkg_SOURCES = rpmdpkg.c
rpmdpkg_LDADD = $(RPM_LDADD_COMMON)

rpmgenbasedir = rpmgenbasedir.c
rpmgenbasedir_LDADD = $(RPMIO_LDADD_COMMON)

rpmgenpkglist = rpmgenpkglist.c
rpmgenpkglist_LDADD = $(RPMIO_LDADD_COMMON)

rpmgensrclist = rpmgensrclist.c
rpmgensrclist_LDADD = $(RPMIO_LDADD_COMMON)

rpmgpg_SOURCES = rpmgpg.c
rpmgpg_LDADD = $(RPMIO_LDADD_COMMON)

rpmpbzip2_SOURCES = rpmpbzip2.c
rpmpbzip2_LDADD = $(RPMIO_LDADD_COMMON)

rpmpigz_SOURCES = rpmpigz.c
rpmpigz_LDADD = $(RPMIO_LDADD_COMMON)

rpmtar_SOURCES = rpmtar.c
rpmtar_LDADD = $(RPM_LDADD_COMMON)

rpmz_SOURCES = rpmz.c
rpmz_LDADD = $(RPMIO_LDADD_COMMON)

tasn_SOURCES = tasn.c
tasn_LDADD = $(RPMIO_LDADD_COMMON) -ltasn1

tbf_SOURCES = tbf.cc
tbf_LDADD = $(RPMIO_LDADD_COMMON)

tblake2b_SOURCES =
tblake2b_LDADD = tblake2b.o
tblake2b.o:  blake2b.c
	@$(LTCOMPILE) -I${top_srcdir} -DBLAKE2B_SELFTEST -o $@ -c $<

tblake2bp_SOURCES =
tblake2bp_LDADD = tblake2bp.o blake2b.o
tblake2bp.o:  blake2bp.c
	@$(LTCOMPILE) -I${top_srcdir} -DBLAKE2BP_SELFTEST -o $@ -c $<

tblake2s_SOURCES =
tblake2s_LDADD = tblake2s.o
tblake2s.o:  blake2s.c
	@$(LTCOMPILE) -I${top_srcdir} -DBLAKE2S_SELFTEST -o $@ -c $<

tblake2sp_SOURCES =
tblake2sp_LDADD = tblake2sp.o blake2s.o
tblake2sp.o:  blake2sp.c
	@$(LTCOMPILE) -I${top_srcdir} -DBLAKE2SP_SELFTEST -o $@ -c $<

tder_SOURCES = tder.c
tder_LDADD = $(RPMIO_LDADD_COMMON)

tdir_SOURCES = tdir.c
tdir_LDADD = $(RPMIO_LDADD_COMMON)

tfts_SOURCES = tfts.c
tfts_LDADD = $(RPMIO_LDADD_COMMON)

tget_SOURCES = tget.c
tget_LDADD = $(RPMIO_LDADD_COMMON)

tgit_SOURCES = tgit.c
tgit_LDADD = $(RPMIO_LDADD_COMMON)

tglob_SOURCES = tglob.c
tglob_LDADD = $(RPMIO_LDADD_COMMON)

thkp_SOURCES = thkp.c
thkp_LDADD = $(RPMIO_LDADD_COMMON)

thtml_SOURCES = thtml.c
thtml_CFLAGS = $(CFLAGS) -I/usr/include/libxml2/
thtml_LDADD = $(RPMIO_LDADD_COMMON)

tinv_SOURCES = tinv.c
tinv_LDADD = $(RPMIO_LDADD_COMMON)

tkey_SOURCES = gengpg.h tkey.c rpmbc.c rpmgc.c rpmnss.c rpmssl.c rpmltc.c
tkey_CFLAGS  = $(CFLAGS)
tkey_LDADD = $(RPMIO_LDADD_COMMON) -lgcrypt
gengpg.h:
	-sh ./gengpg.sh > gengpg.h

tmacro_SOURCES =
tmacro_LDADD = tmacro.o $(RPMIO_LDADD_COMMON)
tmacro.o:  macro.c
	@$(LTCOMPILE) -DDEBUG_MACROS -o $@ -c $<

tmagic_SOURCES = tmagic.c
tmagic_LDADD = $(RPMIO_LDADD_COMMON)

tmire_SOURCES = tmire.c
tmire_LDADD = $(RPMIO_LDADD_COMMON)

todbc_SOURCES = todbc.c
todbc_LDADD = $(RPMIO_LDADD_COMMON)

toid_SOURCES = toid.c
toid_LDADD = $(RPMIO_LDADD_COMMON)

tperl_SOURCES = tperl.c
tperl_LDADD = $(RPMIO_LDADD_COMMON)

tpython = tpython.c
tpython_LDADD = $(RPMIO_LDADD_COMMON)

tput_SOURCES = tput.c
tput_LDADD = $(RPMIO_LDADD_COMMON)

tpw_SOURCES = tpw.c
tpw_LDADD = $(RPM_LDADD_COMMON) $(RPMIO_LDADD_COMMON)

trpmio_SOURCES = trpmio.c
trpmio_LDADD = $(RPMIO_LDADD_COMMON)

tset_SOURCES =
tset_LDADD = tset.o $(RPMIO_LDADD_COMMON)
tset.o:  set.c
	@$(LTCOMPILE) -DSELF_TEST --std=c99 -o $@ -c $<

tsexp_SOURCES = tsexp.c
tsexp_LDADD = $(RPMIO_LDADD_COMMON)

tsvn_SOURCES = tsvn.c
tsvn_LDADD = $(RPMIO_LDADD_COMMON)

tsw_SOURCES = tsw.c
tsw_LDADD = $(RPMIO_LDADD_COMMON)

ttcl_SOURCES = ttcl.c
ttcl_LDADD = $(RPMIO_LDADD_COMMON) -ltcl

tybi_SOURCES = tybi.c
tybi_LDADD = $(RPMIO_LDADD_COMMON)

ttpm_SOURCES = ttpm.c
ttpm_LDADD = $(RPMIO_LDADD_COMMON)

dumpasn1_SOURCES = dumpasn1.c
dumpasn1_LDADD = $(RPMIO_LDADD_COMMON)

lookup3_SOURCES = lookup3.c
lookup3_CFLAGS	= $(CFLAGS) -D_JLU3_SELFTEST
lookup3_LDADD = $(RPMIO_LDADD_COMMON)

