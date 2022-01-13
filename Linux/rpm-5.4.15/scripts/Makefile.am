# Makefile for rpm scripts.

AUTOMAKE_OPTIONS = 1.4 foreign

HELPERS = \
	makeshlibs

EXTRA_DIST = api-sanity-autotest.pl api-sanity-checker.pl \
	brp-compress brp-python-bytecompile brp-java-gcjcompile \
	brp-strip brp-strip-comment-note brp-nobuildrootpath \
	brp-strip-shared brp-strip-static-archive brp-sparc64-linux \
	brp-implant-ident-static brp-java-repack-jars \
	check-files cross-build dbconvert.sh \
	deb_Packages deb_Sources \
	deb_control deb_md5sums deb_postinst deb_postrm deb_preinst deb_prerm \
	executabledeps.sh \
	find-debuginfo.sh find-lang.sh find-prov.pl find-req.pl \
	find-provides.ksyms find-requires.ksyms kmod-deps.sh kmodtool \
	cpanflute cpanflute2 Specfile.pm \
	find-provides.perl find-requires.perl \
	find-provides.php find-requires.php \
	freshen.sh gendiff getpo.sh http.req \
	check-java-closure.sh java.prov.sh java.req.sh \
	gstreamer.sh javadeps.sh libtooldeps.sh mgo \
	mono-find-provides mono-find-requires \
	check-multiarch-files multiarch-platform multiarch-dispatch \
	multiarch-dispatch.h mkmultiarch gem_helper.rb \
	URPM-Makefile.PL.in URPM-Makefile.am \
	nix_meta \
	osgideps.pl pkgconfigdeps.sh \
	perldeps.pl perl.prov perl.req pythondeps.sh pythoneggs.py \
	php.prov php.req pom2spec psq_pkgng_yaml rpm2cpio \
	rpmdb_loadcvt rpmdiff rpmdiff.cgi \
	rpm.daily rpm.log rpm.xinetd \
	rpmsort rt-reloc-wrapper.sh ruby-config rubygems.rb symset-table \
	sql.prov sql.req symclash.py symclash.sh tcl.req tgpg trpm u_pkg.sh \
	vcheck vpkg-provides.sh vpkg-provides2.sh \
	wdj_srpm wdj_srpm_mongo wdj_l10n_sqlite \
	wnh_filelists_mongo wnh_other_mongo wnh_primary_mongo \
	wnh_filelists_yaml wnh_other_yaml wnh_primary_yaml \
	xiu_stdenv \
	yum_filelists_sqlite yum_other_sqlite yum_primary_sqlite \
	yum_filelists_xml yum_other_xml yum_primary_xml \
	find-php-provides find-php-requires \
	${HELPERS}

all:

noinst_SCRIPTS =

noinst_SCRIPTS += multiarch-dispatch multiarch-platform
bin_SCRIPTS = gendiff


noinst_SCRIPTS += check-multiarch-files mkmultiarch
pkgdatadir =		@USRLIBRPM@
pkgdata_SCRIPTS = \
	brp-compress brp-python-bytecompile brp-java-gcjcompile \
	brp-strip brp-strip-comment-note brp-nobuildrootpath \
	brp-strip-shared brp-strip-static-archive brp-sparc64-linux \
	check-files cross-build dbconvert.sh executabledeps.sh \
	find-debuginfo.sh find-lang.sh find-prov.pl find-req.pl \
	find-provides.perl \
	find-requires.perl gem_helper.rb getpo.sh http.req \
	kmod-deps.sh gstreamer.sh javadeps.sh libtooldeps.sh \
	mono-find-provides mono-find-requires \
	osgideps.pl pkgconfigdeps.sh \
	perldeps.pl perl.prov perl.req pythondeps.sh pythoneggs.py \
	php.prov php.req \
	rpm2cpio rpmdb_loadcvt \
	rpm.daily rpm.log rpm.xinetd \
	rubygems.rb \
	tgpg u_pkg.sh \
	vcheck vpkg-provides.sh vpkg-provides2.sh


pkglibbindir =		@USRLIBRPM@/bin
pkglibbin_SCRIPTS = api-sanity-autotest.pl api-sanity-checker.pl mgo pom2spec

pkglibhelperdir =	@USRLIBRPM@/helpers
pkglibhelper_SCRIPTS = 	${HELPERS}

pkglibqfdir =		@USRLIBRPM@/qf
pkglibqf_DATA = \
	deb_Packages deb_Sources \
	deb_control deb_md5sums deb_postinst deb_postrm deb_preinst deb_prerm \
	nix_meta psq_pkgng_yaml \
	wdj_srpm wdj_srpm_mongo wdj_l10n_sqlite \
	wnh_filelists_mongo wnh_other_mongo wnh_primary_mongo \
	wnh_filelists_yaml wnh_other_yaml wnh_primary_yaml \
	xiu_stdenv \
	yum_filelists_sqlite yum_other_sqlite yum_primary_sqlite \
	yum_filelists_xml yum_other_xml yum_primary_xml

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rpm.pc

pkgincdir = $(includedir)
noinst_HEADERS = multiarch-dispatch.h

install-exec-hook:
if WITH_PATH_VERSIONED
	-for p in $(bin_SCRIPTS); do \
	    mv $(DESTDIR)$(bindir)/$${p}$(EXEEXT) \
	       $(DESTDIR)$(bindir)/$${p}-$(VERSION)$(EXEEXT); \
	done
endif
install-data-hook:
if WITH_PATH_VERSIONED
	-for p in $(pkgconfig_DATA); do \
	    mv $(DESTDIR)$(pkgconfigdir)/$${p} \
	       `echo "$(DESTDIR)$(pkgconfigdir)/$${p}" | sed 's/\.\([^.]*\)$$/-$(VERSION).\1/'`; \
	done
endif

