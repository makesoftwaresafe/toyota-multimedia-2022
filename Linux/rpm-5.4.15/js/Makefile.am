# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

LINT = splint

EXTRA_DIST = ngsr.c minimal.c \
	src \
	tscripts \
	words \
	legacy.cudf legacy-sol.cudf

EXTRA_PROGRAMS = ngsr

AM_CPPFLAGS = -I. \
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
	@WITH_GPSEE_CPPFLAGS@ \
	@WITH_LUA_CPPFLAGS@ \
	@WITH_MOZJS185_CPPFLAGS@ \
	@WITH_NEON_CPPFLAGS@ \
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

pkglibdir =     @USRLIBRPM@/lib
pkgbindir =     @USRLIBRPM@/bin
moduledir =	@GPSEE_MODULES_PATH@

#pkgbin_PROGRAMS =	ngsr

module_LTLIBRARIES =	\
	rpmbf_module.la

noinst_LTLIBRARIES =	\
	librpmjsm.la

EXTRA_LTLIBRARIES =	\
	rpmaug_module.la \
	rpmbc_module.la \
	rpmcudf_module.la \
	rpmdb_module.la \
	rpmdbc_module.la \
	rpmdbe_module.la \
	rpmmpf_module.la \
	rpmseq_module.la \
	rpmtxn_module.la \
	rpmdc_module.la \
	rpmdig_module.la \
	rpmdir_module.la \
	rpmds_module.la \
	rpmfc_module.la \
	rpmfi_module.la \
	rpmfts_module.la \
	rpmgi_module.la \
	rpmhdr_module.la \
	rpmio_module.la \
	rpmiob_module.la \
	rpmmc_module.la \
	rpmmg_module.la \
	rpmmi_module.la \
	rpmmpw_module.la \
	rpmps_module.la \
	rpmsm_module.la \
	rpmsp_module.la \
	rpmst_module.la \
	rpmsw_module.la \
	rpmsx_module.la \
	rpmsys_module.la \
	rpmte_module.la \
	rpmts_module.la \
	rpmxar_module.la \
	syck_module.la \
	uuid_module.la

librpmjsm_la_CFLAGS =	-fno-strict-aliasing
librpmjsm_la_LDFLAGS =	-module -avoid-version
librpmjsm_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmaug_module_la_SOURCES =	rpmaug-js.c
rpmaug_module_la_CFLAGS =	-fno-strict-aliasing
rpmaug_module_la_LDFLAGS =	-module -avoid-version
rpmaug_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmbc_module_la_SOURCES =	rpmbc-js.c
rpmbc_module_la_CFLAGS =	-fno-strict-aliasing
rpmbc_module_la_LDFLAGS =	-module -avoid-version
rpmbc_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmbf_module_la_SOURCES =	rpmbf-js.c
rpmbf_module_la_CFLAGS =	-fno-strict-aliasing
rpmbf_module_la_LDFLAGS =	-module -avoid-version
rpmbf_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmcudf_module_la_SOURCES =	rpmcudf-js.c
rpmcudf_module_la_CFLAGS =	-fno-strict-aliasing
rpmcudf_module_la_LDFLAGS =	-module -avoid-version
rpmcudf_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmdb_module_la_SOURCES =	rpmdb-js.c
rpmdb_module_la_CFLAGS =	-fno-strict-aliasing
rpmdb_module_la_LDFLAGS =	-module -avoid-version
rpmdb_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmdbc_module_la_SOURCES =	rpmdbc-js.c
rpmdbc_module_la_CFLAGS =	-fno-strict-aliasing
rpmdbc_module_la_LDFLAGS =	-module -avoid-version
rpmdbc_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmdbe_module_la_SOURCES =	rpmdbe-js.c
rpmdbe_module_la_CFLAGS =	-fno-strict-aliasing
rpmdbe_module_la_LDFLAGS =	-module -avoid-version
rpmdbe_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmmpf_module_la_SOURCES =	rpmmpf-js.c
rpmmpf_module_la_CFLAGS =	-fno-strict-aliasing
rpmmpf_module_la_LDFLAGS =	-module -avoid-version
rpmmpf_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmseq_module_la_SOURCES =	rpmseq-js.c
rpmseq_module_la_CFLAGS =	-fno-strict-aliasing
rpmseq_module_la_LDFLAGS =	-module -avoid-version
rpmseq_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmtxn_module_la_SOURCES =	rpmtxn-js.c
rpmtxn_module_la_CFLAGS =	-fno-strict-aliasing
rpmtxn_module_la_LDFLAGS =	-module -avoid-version
rpmtxn_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmdc_module_la_SOURCES =	rpmdc-js.c
rpmdc_module_la_CFLAGS =	-fno-strict-aliasing
rpmdc_module_la_LDFLAGS =	-module -avoid-version
rpmdc_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmdig_module_la_SOURCES =	rpmdig-js.c
rpmdig_module_la_CFLAGS =	-fno-strict-aliasing
rpmdig_module_la_LDFLAGS =	-module -avoid-version
rpmdig_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmdir_module_la_SOURCES =	rpmdir-js.c
rpmdir_module_la_CFLAGS =	-fno-strict-aliasing
rpmdir_module_la_LDFLAGS =	-module -avoid-version
rpmdir_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmds_module_la_SOURCES =	rpmds-js.c
rpmds_module_la_CFLAGS =	-fno-strict-aliasing
rpmds_module_la_LDFLAGS =	-module -avoid-version
rpmds_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmfc_module_la_SOURCES =	rpmfc-js.c
rpmfc_module_la_CFLAGS =	-fno-strict-aliasing
rpmfc_module_la_LDFLAGS =	-module -avoid-version
rpmfc_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmfi_module_la_SOURCES =	rpmfi-js.c
rpmfi_module_la_CFLAGS =	-fno-strict-aliasing
rpmfi_module_la_LDFLAGS =	-module -avoid-version
rpmfi_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmfts_module_la_SOURCES =	rpmfts-js.c
rpmfts_module_la_CFLAGS =	-fno-strict-aliasing
rpmfts_module_la_LDFLAGS =	-module -avoid-version
rpmfts_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmgi_module_la_SOURCES =	rpmgi-js.c
rpmgi_module_la_CFLAGS =	-fno-strict-aliasing
rpmgi_module_la_LDFLAGS =	-module -avoid-version
rpmgi_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmhdr_module_la_SOURCES =	rpmhdr-js.c
rpmhdr_module_la_CFLAGS =	-fno-strict-aliasing
rpmhdr_module_la_LDFLAGS =	-module -avoid-version
rpmhdr_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmio_module_la_SOURCES =	rpmio-js.c
rpmio_module_la_CFLAGS =	-fno-strict-aliasing
rpmio_module_la_LDFLAGS =	-module -avoid-version
rpmio_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmiob_module_la_SOURCES =	rpmiob-js.c
rpmiob_module_la_CFLAGS =	-fno-strict-aliasing
rpmiob_module_la_LDFLAGS =	-module -avoid-version
rpmiob_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmmc_module_la_SOURCES =	rpmmc-js.c
rpmmc_module_la_CFLAGS =	-fno-strict-aliasing
rpmmc_module_la_LDFLAGS =	-module -avoid-versmcn
rpmmc_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmmg_module_la_SOURCES =	rpmmg-js.c
rpmmg_module_la_CFLAGS =	-fno-strict-aliasing
rpmmg_module_la_LDFLAGS =	-module -avoid-versmgn
rpmmg_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmmi_module_la_SOURCES =	rpmmi-js.c
rpmmi_module_la_CFLAGS =	-fno-strict-aliasing
rpmmi_module_la_LDFLAGS =	-module -avoid-versmin
rpmmi_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmmpw_module_la_SOURCES =	rpmmpw-js.c
rpmmpw_module_la_CFLAGS =	-fno-strict-aliasing
rpmmpw_module_la_LDFLAGS =	-module -avoid-versmin
rpmmpw_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmps_module_la_SOURCES =	rpmps-js.c
rpmps_module_la_CFLAGS =	-fno-strict-aliasing
rpmps_module_la_LDFLAGS =	-module -avoid-versmin
rpmps_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmsm_module_la_SOURCES =	rpmsm-js.c
rpmsm_module_la_CFLAGS =	-fno-strict-aliasing
rpmsm_module_la_LDFLAGS =	-module -avoid-versmin
rpmsm_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmsp_module_la_SOURCES =	rpmsp-js.c
rpmsp_module_la_CFLAGS =	-fno-strict-aliasing
rpmsp_module_la_LDFLAGS =	-module -avoid-versmin
rpmsp_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmst_module_la_SOURCES =	rpmst-js.c
rpmst_module_la_CFLAGS =	-fno-strict-aliasing
rpmst_module_la_LDFLAGS =	-module -avoid-verssysn
rpmst_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmsw_module_la_SOURCES =	rpmsw-js.c
rpmsw_module_la_CFLAGS =	-fno-strict-aliasing
rpmsw_module_la_LDFLAGS =	-module -avoid-versmin
rpmsw_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmsx_module_la_SOURCES =	rpmsx-js.c
rpmsx_module_la_CFLAGS =	-fno-strict-aliasing
rpmsx_module_la_LDFLAGS =	-module -avoid-versmin
rpmsx_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmsys_module_la_SOURCES =	rpmsys-js.c
rpmsys_module_la_CFLAGS =	-fno-strict-aliasing
rpmsys_module_la_LDFLAGS =	-module -avoid-verssysn
rpmsys_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmte_module_la_SOURCES =	rpmte-js.c
rpmte_module_la_CFLAGS =	-fno-strict-aliasing
rpmte_module_la_LDFLAGS =	-module -avoid-verssysn
rpmte_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmts_module_la_SOURCES =	rpmts-js.c
rpmts_module_la_CFLAGS =	-fno-strict-aliasing
rpmts_module_la_LDFLAGS =	-module -avoid-verssysn
rpmts_module_la_LIBADD =	$(RPM_LDADD_COMMON)

rpmxar_module_la_SOURCES =	rpmxar-js.c
rpmxar_module_la_CFLAGS =	-fno-strict-aliasing
rpmxar_module_la_LDFLAGS =	-module -avoid-verssysn
rpmxar_module_la_LIBADD =	$(RPM_LDADD_COMMON)

syck_module_la_SOURCES =	syck-js.c
syck_module_la_CFLAGS =		-fno-strict-aliasing
syck_module_la_LDFLAGS =	-module -avoid-version
syck_module_la_LIBADD =		$(RPM_LDADD_COMMON)

uuid_module_la_SOURCES =	uuid-js.c
uuid_module_la_CFLAGS =		-fno-strict-aliasing
uuid_module_la_LDFLAGS =	-module -avoid-version
uuid_module_la_LIBADD =		$(RPM_LDADD_COMMON)

noinst_HEADERS = \
	rpmaug-js.h rpmbc-js.h rpmbf-js.h rpmcudf-js.h rpmdb-js.h rpmdbc-js.h rpmdbe-js.c rpmdc-js.h rpmdig-js.h \
	rpmdir-js.h rpmds-js.h rpmfc-js.h rpmfi-js.h rpmfts-js.h rpmgi-js.h \
	rpmhdr-js.h rpmio-js.h rpmiob-js.h rpmmc-js.h rpmmg-js.h rpmmi-js.h \
	rpmmpf-js.h rpmmpw-js.h rpmps-js.h rpmseq-js.h rpmsm-js.h rpmsp-js.h rpmst-js.h rpmsw-js.h \
	rpmsx-js.h rpmsys-js.h rpmte-js.h rpmts-js.h rpmtxn-js.h rpmxar-js.h \
	syck-js.h uuid-js.h

librpmjsm_la_SOURCES = \
	rpmaug-js.c rpmbc-js.c rpmbf-js.c rpmcudf-js.c rpmdb-js.c rpmdbc-js.c rpmdbe-js.c rpmdc-js.c rpmdig-js.c \
	rpmdir-js.c rpmds-js.c rpmfc-js.c rpmfi-js.c rpmfts-js.c rpmgi-js.c \
	rpmhdr-js.c rpmio-js.c rpmiob-js.c rpmmc-js.c rpmmg-js.c rpmmi-js.c \
	rpmmpf-js.c rpmmpw-js.c rpmps-js.c rpmseq-js.c rpmsm-js.c rpmsp-js.c rpmst-js.c rpmsw-js.c \
	rpmsx-js.c rpmsys-js.c rpmte-js.c rpmts-js.c rpmtxn-js.c rpmxar-js.c \
	syck-js.c uuid-js.c

ngsr_SOURCES =		ngsr.c
ngsr_LDADD =		$(RPM_LDADD_COMMON)

.PHONY:	lint
lint:
	$(LINT) $(DEFS) $(INCLUDES) $(librpmjs_la_sources)

vg =	$(abs_top_builddir)/libtool --mode=execute \
	valgrind -v --tool=memcheck \
	--leak-check=full --show-reachable=yes --track-origins=yes \
	--suppressions="$(abs_top_builddir)/js/.gsr_suppress" \
	--gen-suppressions=all

gsr =		$(abs_top_builddir)/js/ngsr -ddddddC

check-gsr: ngsr
	@echo "=== $@ ==="
	-$(vg) $(gsr) -c 'print("yadda-yadda")'

check-vg:
	make -C ../gpsee gsr='$(vg) $(gsr)' check

gpsee_tests_dir =	../gpsee/tests
check-binary:	ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/binary" gsr='$(vg) $(gsr)' check
check-crashers:	ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/crashers" gsr='$(vg) $(gsr)' check
check-error_reporting: ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/error_reporting" gsr='$(vg) $(gsr)' check
check-gffi:	ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/gffi" gsr='$(vg) $(gsr)' check
check-gpsee-core: ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/gpsee-core" gsr='$(vg) $(gsr)' check
check-jit:	ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/jit" gsr='$(vg) $(gsr)' check
check-modules:	ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/modules" gsr='$(vg) $(gsr)' check
check-net:	ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/net" gsr='$(vg) $(gsr)' check
check-shellalike: ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/shellalike" gsr='$(vg) $(gsr)' check
check-system:	ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/system" gsr='$(vg) $(gsr)' check
check-warnings:	ngsr
	@echo "=== $@ ==="
	make -C "$(gpsee_tests_dir)/system" gsr='$(vg) $(gsr)' check

check-local:	check-gsr

.PHONY:	lcov-reset	# run lcov from scratch, always
lcov-reset:
	make lcov-run
	make lcov-report

# run lcov from scratch if the dir is not there
lcov:
	make lcov-reset

.PHONY:	lcov-run	# reset run coverage tests
lcov-run:
	@-rm -rf lcov
	find .. -name "*.gcda" -exec rm {} \;
	make check

.PHONY:	lcov-report	# generate report based on current coverage data
lcov-report:
	mkdir lcov
	-lcov -d ./src -d . --capture --output-file lcov/lcov.info
	lcov -l lcov/lcov.info | grep -v "`cd $(top_srcdir) && pwd`" | cut -d: -f1 > lcov/remove
	lcov -r lcov/lcov.info `cat lcov/remove` > lcov/lcov.cleaned.info
	rm lcov/remove
	mv lcov/lcov.cleaned.info lcov/lcov.info
	genhtml -t "$(PACKAGE_STRING)" -o lcov lcov/lcov.info

.PHONY:	lcov-upload
lcov-upload: lcov
	mkdir -p /var/ftp/pub/jbj/rpm-$(VERSION)-lcov
	tar -C lcov -cf - . | (cd /var/ftp/pub/jbj/rpm-$(VERSION)-lcov; tar xf -)
#	rsync -rvz -e ssh --delete lcov/* ???
