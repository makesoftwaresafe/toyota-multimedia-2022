# Top level Makefile for rpm

AUTOMAKE_OPTIONS = 1.4 foreign
ACLOCAL_AMFLAGS = -I m4

AM_DISTCHECK_CONFIGURE_FLAGS = \
        --verbose \
	--prefix=/usr \
	--with-db \
	--with-dbsql \
	--without-db-tools-intergrated \
	--with-zlib \
	--with-bzip2 \
	--with-xz \
        --with-file \
        --with-path-magic=/usr/share/file/magic \
        --without-lua \
        --with-tcl \
        --without-sqlite \
        --without-syck \
        --with-readline \
        --without-augeas \
        --with-beecrypt=internal \
        --with-java \
        --with-openssl \
        --with-nss \
        --with-gcrypt \
        --with-tomcrypt \
        --without-libtpm \
        --without-libtasn1 \
        --without-pakchois \
        --without-gnutls \
        --with-neon=internal \
        --without-libproxy \
        --with-expat \
        --with-pcre \
        --enable-utf \
        --with-uuid=system \
        --with-attr \
        --with-acl \
        --without-xar \
        --with-popt=external \
        --with-keyutils \
        --with-pthreads \
        --with-libelf \
        --without-cudf \
        --without-ficl \
        --without-aterm \
        --without-nix \
        --without-bash \
        --without-rc \
        --without-js \
        --without-gpsee \
        --without-python \
        --without-pythonembed \
        --without-perl \
        --without-perlembed \
        --without-ruby \
        --with-mruby \
        --with-jvm \
        --without-selinux \
        --without-sepol \
        --without-semanage \
        --with-libgit2 \
        --with-squirrel \
        --with-build-extlibdep \
        --with-build-maxextlibdep \
        --with-valgrind \
        --enable-build-pic \
        --enable-build-versionscript \
        --enable-build-warnings \
        --enable-build-debug \
        --enable-maintainer-mode

LINT = splint
MCCABE = pmccabe

EXTRA_DIST = CHANGES CREDITS Doxyheader INSTALL INSTALL.developer cpuinfo.yaml \
	autodeps autogen.sh \
	syck js \
	lua \
	ruby \
	macros \
	perl \
	pubkeys/JBJ-GPG-KEY \
	rpm.magic rpmqv.c rpmqv.cc \
	python \
	tests/arbitrarytag-1.0-1.src.rpm \
	tests/cudf-test-1-0.src.rpm \
	tests/devtool-sanity-1.0-1.src.rpm \
	tests/dir2symlink-1-1.src.rpm \
	tests/edos-test-1-0.src.rpm \
	tests/probes-test-1-0.src.rpm \
	tests/simplestRPMv3-1.0-2.aix5.3.noarch.rpm \
	tests/triggers-D-1.0-1.src.rpm \
	tests/triggers-DP-1.0-1.src.rpm \
	tests/triggers-F-1.0-1.src.rpm \
	tests/triggers-FP-1.0-1.src.rpm \
	tests/triggers-N-1.0-1.src.rpm \
	tests/triggers-NA-1.0-1.src.rpm \
	tests/triggers-P-1.0-1.src.rpm

SUBDIRS =
if USE_NLS
SUBDIRS += \
	po
endif
SUBDIRS += \
	@WITH_DB_SUBDIR@ \
	@WITH_ZLIB_SUBDIR@ \
	@WITH_PCRE_SUBDIR@ \
	@WITH_POPT_SUBDIR@ \
	@WITH_LUA_SUBDIR@ \
	@WITH_BEECRYPT_SUBDIR@ \
	@WITH_NEON_SUBDIR@ \
	@WITH_SPIDERMONKEY_SUBDIR@ \
	@WITH_GPSEE_SUBDIR@ \
	@WITH_LIBTPM_SUBDIR@ \
	@WITH_LIBGIT2_SUBDIR@ \
	@WITH_FILE_SUBDIR@ \
	@WITH_BASH_SUBDIR@ \
	@WITH_RC_SUBDIR@ \
	@WITH_SYCK_SUBDIR@ \
	@WITH_XAR_SUBDIR@ \
	@WITH_XZ_SUBDIR@ \
	misc \
	rpmio \
	rpmdb \
	lib \
	build \
	rpmconstant \
	@WITH_JS_SUBDIR@ \
	@WITH_RUBY_SUBDIR@ \
	@WITH_PYTHON_SUBDIR@ \
	@WITH_PERL_SUBDIR@ \
	@WITH_PERL_URPM_SUBDIR@ \
	tools \
	scripts \
	doc \
	. \
	tests

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/build \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	-I$(top_srcdir)/misc \
	@WITH_DB_CPPFLAGS@ \
	@WITH_ZLIB_CPPFLAGS@ \
	@WITH_LUA_CPPFLAGS@ \
	@WITH_FILE_CPPFLAGS@ \
	@WITH_PCRE_CPPFLAGS@ \
	@WITH_POPT_CPPFLAGS@ \
	@WITH_XAR_CPPFLAGS@ \
	@WITH_XZ_CPPFLAGS@

AM_CFLAGS = $(OPENMP_CFLAGS)

myLDADD = \
	$(top_builddir)/build/librpmbuild.la \
	$(top_builddir)/lib/librpm.la \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(top_builddir)/rpmio/librpmio.la \
	$(top_builddir)/misc/librpmmisc.la \
	@LTLIBINTL@ \
	-lstdc++

bin_PROGRAMS = 		rpm rpmbuild

install-exec-hook:
if WITH_PATH_VERSIONED
	-for p in $(bin_PROGRAMS); do \
	    mv $(DESTDIR)$(bindir)/$${p}$(EXEEXT) \
	       $(DESTDIR)$(bindir)/$${p}-$(VERSION)$(EXEEXT); \
	done
endif

pkgdatadir =		@USRLIBRPM@
pkgdata_DATA = rpmpopt macros/macros macros/macros.rpmbuild cpuinfo.yaml

pkgbindir =	$(pkgdatadir)/bin
pkgbin_SCRIPTS = install-sh mkinstalldirs

pkgcfgdir =	$(pkgdatadir)/macros.d
pkgcfg_DATA = \
	macros/cmake macros/gstreamer macros/java macros/kernel macros/libtool \
	macros/mandriva macros/mono macros/perl macros/pkgconfig macros/php \
	macros/python macros/ruby macros/selinux macros/tcl

noinst_HEADERS = build.h debug.h system.h

rpm_SOURCES =		build.c
rpm_LDFLAGS =		@LDFLAGS_STATIC@ $(LDFLAGS)
rpm_LDADD =		rpm.o $(myLDADD)
rpm.o:	$(top_srcdir)/rpmqv.c
	$(COMPILE) -DIAM_RPMBT -DIAM_RPMDB -DIAM_RPMEIU -DIAM_RPMK -DIAM_RPMQV -o $@ -c $(top_srcdir)/rpmqv.cc

rpmbuild_SOURCES =	build.c
rpmbuild_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
rpmbuild_LDADD =	rpmbuild.o $(myLDADD)
rpmbuild.o: $(top_srcdir)/rpmqv.c
	$(COMPILE) -DIAM_RPMBT -o $@ -c $(top_srcdir)/rpmqv.cc

.PHONY:	splint
splint:
	splint \
	    -load build/rpmbuild.lcd \
	    -load lib/rpmlib.lcd \
	    -load rpmdb/rpmdb.lcd \
	    -load rpmio/rpmio.lcd \
	    -load popt/popt.lcd \
		$(DEFS) $(INCLUDES) rpmqv.c $(rpmbuild_SOURCES)

.PHONY:	lint
lint:
	$(LINT) -Dlint $(DEFS) $(INCLUDES) rpmqv.c $(rpmbuild_SOURCES) \
		`make -s sources -C build` \
		`make -s sources -C lib` \
		`make -s sources -C rpmdb` \
		`make -s sources -C rpmio` \
		`make -s sources -C file/src`

.PHONY:	mccabe
mccabe:
	@$(MCCABE) rpmqv.c $(rpmbuild_SOURCES) \
		`make -s sources -C build` \
		`make -s sources -C lib` \
		`make -s sources -C rpmdb` \
		`make -s sources -C rpmio` | sort -nr | head -n 30

.PHONY: updatepo
updatepo:
	rsync -Lrtvz  translationproject.org::tp/latest/rpm/  po


.PHONY: cppcheck
cppcheck:
	cppcheck --enable=all -q $(top_srcdir)


CVSTAG = r$(subst .,-,$(VERSION))

pkgsrcdir = @PKGSRCDIR@

DBPATH = @DBPATH@

install-data-local:
	@$(mkdir_p) $(DESTDIR)$(DBPATH)
	@$(mkdir_p) $(DESTDIR)$(varprefix)/spool/repackage
	@for dir in BUILD RPMS SOURCES SPECS SRPMS; do\
	    $(mkdir_p) $(DESTDIR)$(pkgsrcdir)/$$dir;\
	done
	@case "@host_cpu@" in \
	*86)	$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/i386 ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/i486 ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/i586 ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/i686 ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/athlon ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/pentium3 ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/pentium4 ;;\
	armv3l)	$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv3l ;;\
	armv4b)	$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv4b ;;\
	armv4l)	$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv3l ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv4l ;;\
	armv5teb) $(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv4b ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv5teb ;;\
	armv5tel) $(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv3l ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv4l ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/armv5tel ;;\
	alpha*) $(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/alpha ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/alphaev6 ;;\
	sparc*) $(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/sparc ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/sparcv8 ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/sparcv9 ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/sparc64 ;;\
	ia64*)	$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/ia64 ;;\
	s390*)	$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/s390 ;;\
	mipsel*) $(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/mipsel ;;\
	mips*)  $(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/mips ;;\
	powerpc*) $(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/ppc ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/ppciseries ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/ppcpseries ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/ppc64 ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/ppc64iseries ;\
		$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/ppc64pseries ;;\
	*)	$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/@host_cpu@ ;;\
	esac
	@$(mkdir_p) $(DESTDIR)$(pkgsrcdir)/RPMS/noarch

check-local:

# XXX W2DO: refactor to {js,ruby,python}/Makefile.am
distclean-local:
	[ -d js ] && rm -rf js/Makefile js/.deps
	[ -d ruby ] && rm -rf ruby/Makefile ruby/.deps
	[ -d python ] && rm -rf python/Makefile python/.deps

# XXX MacOSX: -I$(distdir)/_inst/include/db60 in flags w "make distcheck"
distcheck-hook:
	$(mkdir_p) $(distdir)/_inst/include	# XXX <-- HACK ALERT
	$(LN_S) $(prefix)/include/db60 $(distdir)/_inst/include/db60

# --- scan.coverity.com manual submission in $(PACKAGE).tgz
COVDIR =	cov-int
COVBUILD =	cov-build --dir $(COVDIR)
COVDESC =	$(shell date -u "+%Y%m%d.%H%M%S")
COVFILE =	$(PACKAGE)-scan.tgz
COVSITE =	https://scan.coverity.com/builds?project=rpm
covscan:
	rm -rf $(COVDIR) $(PACKAGE).tgz
	make clean
	$(COVBUILD) make -j4
	tar czf $(COVFILE) $(COVDIR)
	curl --form project=$(PACKAGE) \
	     --form token=9FhGnFpviyNVjN5kBmAqSQ \
	     --form email=n3npq@mac.com \
	     --form file=@$(COVFILE) \
	     --form version=$(PACKAGE_VERSION) \
	     --form description=$(COVDESC) \
	     $(COVSITE)

.PHONY: tar
tar:
	rm -rf /tmp/rpm-$(VERSION)
	$(MAKE) DESTDIR=/tmp/rpm-$(VERSION) install
	cd /tmp/rpm-$(VERSION) ; tar cvf /tmp/rpm-$(VERSION).tar .

.PHONY: noconfig
noconfig:
	find . -name "Makefile" -exec rm {} \; 
	rm -f *gz *rpm config.*

.PHONY: archive
archive: 
	@cvs -d `cat CVS/Root` diff 2>&1 > /dev/null || { \
	    echo " " ; \
	    echo "==> Please resolve differences between the repository and" ; \
	    echo "==> your rpm check out before tagging." ; \
	    echo " " ; \
	    cvs -n up ; \
	    echo " " ; \
	    exit 1 ; \
	}
	@cvs -d `cat CVS/Root` -Q tag -F $(CVSTAG) .
	@make dist
	@echo " "
	@echo "The final archive is ./rpm-$(VERSION).tar.gz."

.PHONY:	nextsrpm
nextsrpm: all archive
	@sudo ./rpm -ta rpm-$(VERSION).tar.gz

.PHONY:	doxygen
doxygen @WITH_APIDOCS_TARGET@: Doxyfile rpmpopt macros/macros
	rm -rf $@
	mkdir -p $@
	- [ X"@__DOXYGEN@" != Xno ] && @__DOXYGEN@

# --- LCOV 1.10 in use
.PHONY:	lcov-reset	# run lcov from scratch, always
lcov-reset:
	$(MAKE) lcov-run
	$(MAKE) lcov-report

# run lcov from scratch if the dir is not there
lcov:
	$(MAKE) lcov-reset

.PHONY:	lcov-run	# reset run coverage tests
lcov-run:
	@rm -rf lcov
	lcov -d . --zerocounters
	$(MAKE) check
	$(MAKE) -C tests clean test

.PHONY:	lcov-report	# generate report based on current coverage data
lcov-report:
	@${mkdir_p} lcov
	@find rpmio/auto -name '*.gcda' -exec rm -f {} \;
	@find rpmdb/auto -name '*.gcda' -exec rm -f {} \;
	@find lib/auto -name '*.gcda' -exec rm -f {} \;
	@find build/auto -name '*.gcda' -exec rm -f {} \;
	lcov -q --no-external \
	  -d popt \
	  -d lua/local \
	  -d lua \
	  -d beecrypt \
	  -d misc \
	  -d rpmio \
	  -d rpmdb \
	  -d lib \
	  -d build \
	  -d tools \
	  -d tests/mongo \
	  -d tests \
	  -d . \
	  --capture --output-file lcov/lcov.info
	genhtml -q -t "${PACKAGE_STRING}" -o lcov lcov/lcov.info
	tar czf lcov.tgz lcov

.PHONY:	lcov-upload
lcov-upload: lcov
#	rsync -rvz -e ssh --delete lcov/* ???

cscope:
	cscope -b -R

cref: ctags cscope
