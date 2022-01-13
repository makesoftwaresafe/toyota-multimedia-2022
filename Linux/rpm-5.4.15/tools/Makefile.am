# Makefile for rpm tools.

AUTOMAKE_OPTIONS = 1.4 foreign

LINT = splint

SUBDIRS = # tests

AM_CPPFLAGS = -I. \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	-I$(top_srcdir)/misc \
	-I$(top_srcdir)/build \
	@WITH_DB_CPPFLAGS@ \
	@WITH_BEECRYPT_CPPFLAGS@ \
	@WITH_PCRE_CPPFLAGS@ \
	@WITH_POPT_CPPFLAGS@ \
	@WITH_XAR_CPPFLAGS@

AM_CFLAGS = $(OPENMP_CFLAGS)

EXTRA_DIST =	hashtab.h bsdiff.1 bspatch.1 dotgraph.hh

EXTRA_PROGRAMS = nix-copy-closure nix-env nix-hash nix-install-package \
	nix-instantiate nix-store nix-worker \
	xiu-instantiate xiu-store
noinst_PROGRAMS =

EXTRA_PROGRAMS += augtool cudftool dbconvert debugedit \
	nix-build nix-channel nix-collect-garbage \
	nix-log2xml nix-prefetch-url nix-pull nix-push \
	xiu-echo xiu-hash \
	roto rpmkey sandbox semodule spooktool

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

bin_PROGRAMS =		rpm2cpio

pkgbindir =		@USRLIBRPM@/bin
pkgbin_PROGRAMS =	\
	@WITH_AUGEAS_AUGTOOL@ chroot cp @WITH_CUDF_CUDFTOOL@ find mtree \
	@WITH_SEMANAGE_SEMODULE@ wget \
	rpmcache rpmdigest rpmrepo rpmspecdump \
	rpmcmp rpmdeps sqlite3 @WITH_KEYUTILS_RPMKEY@ @WITH_LIBELF_DEBUGEDIT@
if WITH_DB
pkgbin_PROGRAMS +=	dbconvert
endif
dist_man_MANS =		rpmgrep.1

noinst_PROGRAMS +=	grep

augtool_SOURCES =	augtool.c
augtool_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
augtool_LDADD =		$(RPMIO_LDADD_COMMON)

chroot_SOURCES =	chroot.c
chroot_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
chroot_LDADD =		$(RPMIO_LDADD_COMMON)

cp_SOURCES =		cp.c
cp_LDFLAGS =		@LDFLAGS_STATIC@ $(LDFLAGS)
cp_LDADD =		$(RPMIO_LDADD_COMMON)

cudftool_SOURCES =	cudftool.c
cudftool_CPPFLAGS =	$(AM_CPPFLAGS) $(CPPFLAGS) $(shell pkg-config --cflags glib-2.0)
cudftool_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS) $(shell pkg-config --libs glib-2.0)
cudftool_LDADD =	$(RPMIO_LDADD_COMMON)

dbconvert_SOURCES =	dbconvert.c
dbconvert_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
dbconvert_LDADD =	$(RPM_LDADD_COMMON)

debugedit_SOURCES =	debugedit.c hashtab.c
debugedit_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
debugedit_LDADD =	$(RPM_LDADD_COMMON)

nix_build_SOURCES =	nix-build.c
nix_build_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
nix_build_LDADD =	$(RPMIO_LDADD_COMMON)

nix_channel_SOURCES =	nix-channel.c
nix_channel_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
nix_channel_LDADD =	$(RPMIO_LDADD_COMMON)

nix_collect_garbage_SOURCES =	nix-collect-garbage.c
nix_collect_garbage_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
nix_collect_garbage_LDADD =	$(RPMIO_LDADD_COMMON)

nix_copy_closure_SOURCES =	nix-copy-closure.c
nix_copy_closure_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
nix_copy_closure_LDADD =	$(RPMIO_LDADD_COMMON)

nix_env_SOURCES =	nix-env.cc profiles.cc
nix_env_LDADD =		-L/usr/lib/nix -lmain -lexpr -lstore -lutil -lformat \
        /usr/lib/nix/libATerm.a
nix_env_LDFLAGS =	-Wl,-rpath -Wl,/usr/lib/nix

nix_hash_SOURCES =	nix-hash.cc
nix_hash_LDADD =	-L/usr/lib/nix -lmain -lexpr -lstore -lutil -lformat \
        /usr/lib/nix/libATerm.a
nix_hash_LDFLAGS =	-Wl,-rpath -Wl,/usr/lib/nix

nix_install_package_SOURCES =	nix-install-package.c
nix_install_package_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
nix_install_package_LDADD =	$(RPMIO_LDADD_COMMON)

nix_instantiate_SOURCES =	nix-instantiate.cc
nix_instantiate_LDADD =	-L/usr/lib/nix -lmain -lexpr -lstore -lutil -lformat \
        /usr/lib/nix/libATerm.a
nix_instantiate_LDFLAGS =	-Wl,-rpath -Wl,/usr/lib/nix

nix_log2xml_SOURCES =	log2xml.cc
nix_log2xml_LDADD =	# -L/usr/lib/nix -lmain -lexpr -lstore -lutil -lformat  /usr/lib/nix/libATerm.a
nix_log2xml_LDFLAGS =	# -Wl,-rpath -Wl,/usr/lib/nix

nix_prefetch_url_SOURCES =	nix-prefetch-url.c
nix_prefetch_url_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
nix_prefetch_url_LDADD =	$(RPMIO_LDADD_COMMON)

nix_pull_SOURCES =	nix-pull.c
nix_pull_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
nix_pull_LDADD =	$(RPMIO_LDADD_COMMON)

nix_push_SOURCES =	nix-push.c
nix_push_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
nix_push_LDADD =	$(RPMIO_LDADD_COMMON)

nix_store_SOURCES =	nix-store.cc dotgraph.cc
nix_store_LDADD =	-L/usr/lib/nix -lmain -lexpr -lstore -lutil -lformat \
        /usr/lib/nix/libATerm.a
nix_store_LDFLAGS =	-Wl,-rpath -Wl,/usr/lib/nix

nix_worker_SOURCES =	nix-worker.cc
nix_worker_LDADD =	-L/usr/lib/nix -lmain -lexpr -lstore -lutil -lformat \
        /usr/lib/nix/libATerm.a
nix_worker_LDFLAGS =	-Wl,-rpath -Wl,/usr/lib/nix

roto_SOURCES =		roto.c
roto_LDFLAGS =		@LDFLAGS_STATIC@ $(LDFLAGS)
roto_LDADD =		$(RPMDB_LDADD_COMMON)

rpmcache_SOURCES =	rpmcache.c
rpmcache_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
rpmcache_LDADD =	$(RPM_LDADD_COMMON)

rpmcmp_SOURCES =	rpmcmp.c
rpmcmp_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
rpmcmp_LDADD =		$(RPMDB_LDADD_COMMON)

rpmdeps_SOURCES =	rpmdeps.c
rpmdeps_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
rpmdeps_LDADD =		$(RPM_LDADD_COMMON)

rpmdigest_SOURCES =	rpmdigest.c
rpmdigest_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
rpmdigest_LDADD =	$(RPMIO_LDADD_COMMON)

find_SOURCES =		rpmfind.c
find_LDFLAGS =		@LDFLAGS_STATIC@ $(LDFLAGS)
find_LDADD =		$(RPMIO_LDADD_COMMON)

grep_SOURCES =		rpmgrep.c
grep_LDFLAGS =		@LDFLAGS_STATIC@ $(LDFLAGS)
grep_LDADD =		$(RPMIO_LDADD_COMMON)

mtree_SOURCES =		rpmmtree.c
mtree_LDFLAGS =		@LDFLAGS_STATIC@ $(LDFLAGS)
mtree_LDADD =		$(RPM_LDADD_COMMON)

rpmrepo_SOURCES =	rpmrepo.c
rpmrepo_CPPFLAGS =	$(AM_CPPFLAGS) -I$(top_srcdir)/scripts -I$(top_builddir)/scripts $(CPPFLAGS)
rpmrepo_LDADD =		$(RPM_LDADD_COMMON)

rpmspecdump_SOURCES =	rpmspecdump.c
rpmspecdump_LDADD =	$(RPMBUILD_LDADD_COMMON)

semodule_SOURCES =	semodule.c
semodule_LDADD =	$(RPMIO_LDADD_COMMON)

sandbox_SOURCES =	sandbox.c
sandbox_LDADD =		$(RPMIO_LDADD_COMMON)

spooktool_SOURCES =	spooktool.c
spooktool_LDADD =	$(RPMIO_LDADD_COMMON)

sqlite3_SOURCES =	sqlite3.c
sqlite3_LDADD =		$(RPMIO_LDADD_COMMON)

wget_SOURCES =		rpmwget.c
wget_LDADD =		$(RPMIO_LDADD_COMMON)

xiu_echo_SOURCES =	xiu-echo.c
xiu_echo_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
xiu_echo_LDADD =	$(RPMIO_LDADD_COMMON)

xiu_hash_SOURCES =	xiu-hash.c
xiu_hash_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
xiu_hash_LDADD =	$(RPMIO_LDADD_COMMON)

xiu_instantiate_SOURCES	= xiu-instantiate.c
xiu_instantiate_LDADD	= $(RPMIO_LDADD_COMMON) # -L/usr/lib/nix -lmain -lexpr -lstore -lutil -lformat /usr/lib/nix/libATerm.a
xiu_instantiate_LDFLAGS = @LDFLAGS_STATIC@ $(LDFLAGS) # -Wl,-rpath -Wl,/usr/lib/nix

xiu_store_SOURCES	= xiu-store.c
xiu_store_LDADD		= $(RPMIO_LDADD_COMMON) # -L/usr/lib/nix -lmain -lexpr -lstore -lutil -lformat /usr/lib/nix/libATerm.a
xiu_store_LDFLAGS	= @LDFLAGS_STATIC@ $(LDFLAGS) # -Wl,-rpath -Wl,/usr/lib/nix

##
## Traditional rpm2cpio
##
rpm2cpio_SOURCES =	rpm2cpio.c
rpm2cpio_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
rpm2cpio_LDADD =	$(LDFLAGS) $(RPM_LDADD_COMMON)

##
##  keyctl(1) clone
##
rpmkey_SOURCES =        rpmkey.c
rpmkey_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
rpmkey_LDADD =          $(LDFLAGS) $(RPMIO_LDADD_COMMON) -lkeyutils

##
## XAR <-> RPM package converter
##
if WITH_XAR
pkgbin_PROGRAMS += txar
txar_SOURCES =		txar.c
txar_LDADD =		$(RPM_LDADD_COMMON) @WITH_XAR_LDFLAGS@
txar_LIBS =		@WITH_XAR_LIBS@
endif

##
##  provide Berkeley-DB tools
##
###if WITH_DB_INTERNAL
###if WITH_DB_TOOLS_INTEGRATED
###pkgbin_PROGRAMS += db_tool
###db_tool_SOURCES = db_tool.c
###db_tool_CPPFLAGS = \
###	-I$(top_builddir)/db3 \
###	-I$(top_builddir)/db
###if WITH_DB_RPC
###db_tool_CPPFLAGS += \
###    -DWITH_DB_RPC
###endif
###db_tool_LDFLAGS =	@LDFLAGS_STATIC@ $(LDFLAGS)
###db_tool_LDADD =
#   for db_recover
###db_tool_LDADD += \
###	$(RPMDB_LDADD_COMMON)
#   for all db_xxx
###db_tool_LDADD += \
###	$(top_builddir)/db3/util_cache.lo \
###	$(top_builddir)/db3/util_log.lo \
###	$(top_builddir)/db3/util_sig.lo
#   for db_printlog
###db_tool_LDADD += \
###	$(top_builddir)/db3/btree_autop.lo \
###	$(top_builddir)/db3/crdel_autop.lo \
###	$(top_builddir)/db3/db_autop.lo \
###	$(top_builddir)/db3/dbreg_autop.lo \
###	$(top_builddir)/db3/fileops_autop.lo \
###	$(top_builddir)/db3/hash_autop.lo \
###	$(top_builddir)/db3/qam_autop.lo \
###	$(top_builddir)/db3/rep_auto.lo \
###	$(top_builddir)/db3/txn_autop.lo
###if WITH_DB_RPC
#   for db_svc
###db_tool_LDADD += \
###	$(top_builddir)/db3/db_server_proc.lo \
###	$(top_builddir)/db3/db_server_svc.lo \
###	$(top_builddir)/db3/gen_db_server.lo
###endif
###db_tool_LDADD += \
###	$(top_builddir)/db3/libdb-5.0.la
###endif

if WITH_DBSQL
pkgbin_PROGRAMS +=	dbsql
dbsql_SOURCES	= dbsql.c
dbsql_LDFLAGS	= @LDFLAGS_STATIC@ $(LDFLAGS)
dbsql_LDADD = \
	$(RPMIO_LDADD_COMMON)
endif
###if WITH_DBSQL_CODEGEN
###pkgbin_PROGRAMS +=	db_sql_codegen
###db_sql_codegen_SOURCES	= ../db/db_sql_codegen/db_sql_codegen.c
###db_sql_codegen_LDFLAGS	= @LDFLAGS_STATIC@ $(LDFLAGS)
###db_sql_codegen_LDADD = \
###	$(top_builddir)/db3/parse.lo \
###	$(top_builddir)/db3/preparser.lo \
###	$(top_builddir)/db3/parsefuncs.lo \
###	$(top_builddir)/db3/tokenize.lo \
###	$(top_builddir)/db3/sqlprintf.lo \
###	$(top_builddir)/db3/buildpt.lo \
###	$(top_builddir)/db3/utils.lo \
###	$(top_builddir)/db3/generate.lo \
###	$(top_builddir)/db3/generate_test.lo \
###	$(top_builddir)/db3/generation_utils.lo \
###	$(top_builddir)/db3/generate_verification.lo \
###	$(top_builddir)/db3/hint_comment.lo \
###	$(top_builddir)/db3/libdb-5.0.la
###endif
###endif

###if WITH_DB_INTERNAL
###my_DB_TOOLS = db_stat db_load db_dump db_archive db_checkpoint db_deadlock db_recover db_upgrade db_verify db_hotbackup db_printlog db_log_verify
###if WITH_DB_RPC
###my_DB_TOOLS += db_svc
###endif
###endif

###install-data-local:
###	@[ -d $(DESTDIR)$(pkgbindir) ] || $(mkinstalldirs) $(DESTDIR)$(pkgbindir)
###if WITH_DB_INTERNAL
###if WITH_DB_TOOLS_INTEGRATED
###	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) \
###		$(builddir)/db_tool $(DESTDIR)$(pkgbindir)/db_tool
###	for tool in . $(my_DB_TOOLS); do \
###		test ".$$tool" = .. && continue; \
###		rm -f $(DESTDIR)$(pkgbindir)/$$tool; \
###		ln $(DESTDIR)$(pkgbindir)/db_tool $(DESTDIR)$(pkgbindir)/$$tool; \
###	done
###else
###	for tool in . $(my_DB_TOOLS); do \
###		test ".$$tool" = .. && continue; \
###		$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) \
###		$(top_builddir)/$(WITH_DB_SUBDIR)/$$tool $(DESTDIR)$(pkgbindir)/$$tool; \
###	done
###endif
###endif

check-local:
	-./rpmdeps --perl > /dev/null 2>&1

.PHONY: lint_rpmgrep
lint_rpmgrep:
	$(LINT) -f .splintrc_rpmgrep $(DEFS) $(INCLUDES) rpmgrep.c

.PHONY:	lint_rpmmtree
lint_rpmmtree:
	$(LINT) -f .splintrc_rpmmtree $(DEFS) $(INCLUDES) rpmmtree.c

.PHONY:	lint_rpmrepo
lint_rpmrepo:
	$(LINT) -f .splintrc_rpmrepo $(DEFS) $(INCLUDES) rpmrepo.c

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
