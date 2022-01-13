## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST =	\
	genpgp.sh \
	genssl.sh \
	gpsee/gffi.js \
	ref/.alldigests \
	ref/DB_CONFIG \
	ref/edos.manifest \
	ref/edos.qa.1 \
	ref/edos.qa.2 \
	ref/filelists.xml \
	ref/hkp \
	ref/manifest \
	ref/other.xml \
	ref/popt.Kv \
	ref/popt.Kv-beecrypt \
	ref/popt.Kv-nss \
	ref/popt.Kv-ssl \
	ref/popt.xml \
	ref/popt.yaml \
	ref/primary.xml \
	ref/querytags \
	ref/repomd.xml \
	ref/rpmdb.sql \
	ref/showrc \
	ref/sqldb-basic.out \
	ref/sqldb-basic.sql \
	ref/version \
	spew spew.conf \
	tpgp.c tssl.c

EXTRA_PROGRAMS =
EXTRA_PROGRAMS += tkey
noinst_PROGRAMS =

EXTRA_PROGRAMS += tecdsa thkp tpgp trsa tssl

SUBDIRS = . mongo

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
	@WITH_ZLIB_CPPFLAGS@ \
	@WITH_LUA_CPPFLAGS@ \
	@WITH_FILE_CPPFLAGS@ \
	@WITH_PCRE_CPPFLAGS@ \
	@WITH_POPT_CPPFLAGS@ \
	@WITH_SYCK_CPPFLAGS@ \
	@WITH_XAR_CPPFLAGS@

RPMIO_LDADD = \
	$(top_builddir)/rpmio/librpmio.la \
	$(top_builddir)/misc/librpmmisc.la \
	@LTLIBINTL@

RPMDB_LDADD = \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(RPMIO_LDADD)

RPM_LDADD = \
	$(top_builddir)/build/librpmbuild.la \
	$(top_builddir)/lib/librpm.la \
	$(RPMDB_LDADD)

macros =	$(abs_top_builddir)/macros/macros:$(testdir)/macros
rpmpopt =	$(abs_top_builddir)/rpmpopt

rpm =		$(abs_top_builddir)/rpm --rpmpopt=$(rpmpopt) --macros $(macros)
rpmbuild =	$(abs_top_builddir)/rpmbuild --rpmpopt=$(rpmpopt) --macros $(macros)
rpm2cpio =	$(abs_top_builddir)/tools/rpm2cpio
rpmcache =	$(abs_top_builddir)/tools/rpmcache --rpmpopt=$(rpmpopt) --macros $(macros)
rpmdigest =	$(abs_top_builddir)/tools/rpmdigest
rpmrepo =	$(abs_top_builddir)/tools/rpmrepo
rpmsql =	$(abs_top_builddir)/tools/sqlite3
rpmsolver =	${rpm} -D '_dbpath $(testdir)/tmp/cachedb'
rpmsolverA =	${rpm} -D '_dbpath %{_solve_A}'
rpmsolverB =	${rpm} -D '_dbpath %{_solve_B}'

poky_topdir =	/X/src/poky
poky_deploy =	$(poky_topdir)/build/tmp/deploy/rpm
poky_pkgsdirA =	$(poky_deploy)/i586
poky_pkgsdirB =	$(poky_deploy)/all
poky_pkgsdirC =	$(poky_deploy)/qemux86
poky_options =	--noaid --nodeps --noorder --noscripts --notriggers --noparentdirs --nolinktos --stats
poky_force =	--replacepkgs --replacefiles --oldpackage
poky_solverA =	${rpm} -i $(poky_force) -D '_dbpath %{_solve_A}' --justdb $(poky_options)
poky_solverB =	${rpm} -i $(poky_force) -D '_dbpath %{_solve_B}' --justdb $(poky_options)
poky_solverC =	${rpm} -i $(poky_force) -D '_dbpath %{_solve_C}' --justdb $(poky_options)

tdsa =		./tdsa
tecdsa =	./tecdsa
thkp =		./thkp -H localhost
tkey =		./tkey
tpgp =		./tpgp --macros $(macros)
trsa =		./trsa
tssl =		./tssl --macros $(macros)

chroot =	$(abs_top_builddir)/tools/chroot
cp =		$(abs_top_builddir)/tools/cp
echo =		/bin/echo
find =		$(abs_top_builddir)/tools/find
grep =		$(abs_top_builddir)/tools/grep
lua =		$(abs_top_builddir)/lua/lua
mtree =		$(abs_top_builddir)/tools/mtree
ngsr =		$(abs_top_builddir)/js/ngsr
rc =		$(abs_top_builddir)/tools/rc
todbc =		${abs_top_builddir}/rpmio/todbc
#wget =		$(abs_top_builddir)/tools/wget -nv
wget =		wget -nv

vg =		$(LIBTOOL) --mode=execute vg 

sudo =		sudo

rpmdb =		$(testdir)/tmp/rpmdb
rpmdb_Tables = \
	$(rpmdb)/Arch	\
	$(rpmdb)/Basenames	\
	$(rpmdb)/Conflictname	\
	$(rpmdb)/Dirnames	\
	$(rpmdb)/Filedigests	\
	$(rpmdb)/Filepaths	\
	$(rpmdb)/Group	\
	$(rpmdb)/Installtid	\
	$(rpmdb)/Name	\
	$(rpmdb)/Nvra	\
	$(rpmdb)/Obsoletename	\
	$(rpmdb)/Os	\
	$(rpmdb)/Packagecolor	\
	$(rpmdb)/Providename	\
	$(rpmdb)/Pubkeys	\
	$(rpmdb)/Release	\
	$(rpmdb)/Requirename	\
	$(rpmdb)/Seqno	\
	$(rpmdb)/Sha1header	\
	$(rpmdb)/Sigmd5	\
	$(rpmdb)/Sourcepkgid	\
	$(rpmdb)/Triggername	\
	$(rpmdb)/Version

db_archive =	@__DB_ARCHIVE@	-h $(rpmdb) -v
db_checkpoint =	@__DB_CHECKPOINT@ -h $(rpmdb) -v
db_dump =	@__DB_DUMP@	-h $(rpmdb)
db_load =	@__DB_LOAD@	-h $(rpmdb)
db_log_verify =	@__DB_LOG_VERIFY@ -h $(rpmdb)
db_printlog =	@__DB_PRINTLOG@	-h $(rpmdb)
db_recover =	@__DB_RECOVER@	-h $(rpmdb) -v
db_stat =	@__DB_STAT@	-h $(rpmdb)
db_upgrade =	@__DB_UPGRADE@	-h $(rpmdb) -v
db_verify =	@__DB_VERIFY@	-h $(rpmdb)

dbsql	 =	$(abs_top_builddir)/tools/dbsql

chattr =	@__CHATTR@
cpio =		@__CPIO@
diff =		@__DIFF@
file =		@__FILE@
gpg =		@__GPG@
gsr =		@__GSR@
ln =		@__LN@
mkdir =		@__MKDIR@
mongo =		@__MONGO@
# XXX mongo.rpm5.org
mongoserver =	198.178.231.189
mv =		@__MV@
rm =		@__RM@
rmdir =		@__RMDIR@
sqlite3 =	@__SQLITE3@
tar =		@__TAR@

#tdsa_SOURCES = tdsa.c 
#tdsa_LDADD = $(RPMIO_LDADD)

tecdsa_SOURCES = tecdsa.c
tecdsa_LDADD = $(RPMIO_LDADD)

thkp_SOURCES = thkp.c
thkp_LDADD = $(RPMIO_LDADD)

tkey_SOURCES = tkey.c
tkey_LDADD = $(RPM_LDADD)
tkey.c: genpgp.h

tpgp_SOURCES = tpgp.c
tpgp_LDADD = $(RPM_LDADD)
tpgp.c: genpgp.h
genpgp.h: genpgp.sh
	-sh $< > $@

trsa_SOURCES = trsa.c
trsa_LDADD = $(RPMIO_LDADD)

tssl_SOURCES = genssl.h tssl.c
tssl_LDADD = $(RPM_LDADD)
tssl.c: genssl.h
genssl.h: genssl.sh
	-touch $@
#	-sh $< > $@

.PHONY:	check-init
check-init: macros
	@echo "=== $@ ==="
	@rm -rf tmp/hrmib tmp/repackage
	@mkdir -p tmp/hrmib/ tmp/repackage/
	@${rpm} -v --version	> tmp/version
	@-diff -u tmp/version ref/version || cp tmp/version ref/version
	@${rpm} -v --querytags	> tmp/querytags
	@-diff -u tmp/querytags ref/querytags || cp tmp/querytags ref/querytags
	@${rpm} -v --showrc | sed -e "s,$(abs_top_builddir),..,g" > tmp/showrc
	@-diff -u tmp/showrc ref/showrc || cp tmp/showrc ref/showrc
#	@${rpmdigest} --alldigests ref/[^C]* > tmp/.alldigests
#	@-diff -u tmp/.alldigests ref/.alldigests || cp tmp/.alldigests ref/.alldigests
#	@-${mtree} -c -p ref | ${mtree} -p ref	# XXX mtime usecs need fixing

POPTURI =	http://rpm5.org/files/popt/
POPTPKGS =	\
	popt-1.14-1.src.rpm \
	popt-1.14-1.i386.rpm

$(POPTPKGS):
	${wget} $(POPTURI)$@

.PHONY:	check-pubkeys
check-pubkeys: tpgp thkp $(POPTPKGS)
	@echo "=== $@ ==="
	@${rm} -rf tmp/rpmdb
	@${mkdir} -p tmp/rpmdb/log tmp/rpmdb/tmp
	@${cp} ref/DB_CONFIG tmp/rpmdb
	@${rpm} --import tmp/DSA.pubpem
	@${rpm} --import tmp/RSA.pubpem
	@${echo} -n "--> default: HKP"
	@${thkp} > tmp/hkp || :
	@diff -u tmp/hkp ref/hkp || cp tmp/hkp ref/hkp
	@${tpgp}
	@${echo} -n "--> beecrypt: HKP"
	@${thkp} --usecrypto bc > /dev/null || :
	@${tpgp} --usecrypto bc
#	@${echo} -n "--> cdsa: HKP"
#	@-${thkp} --usecrypto cdsa > /dev/null || :
#	@-${tpgp} --usecrypto cdsa
	@${echo} -n "--> gcrypt: HKP"
	@-${thkp} --usecrypto gc > /dev/null 2>&1 || :
	@-${tpgp} --usecrypto gc
	@${echo} -n "--> tomcrypt: HKP"
	@-${thkp} --usecrypto ltc > /dev/null 2>&1 || :
	@-${tpgp} --usecrypto ltc
	@${echo} -n "--> nss: HKP"
	@-${thkp} --usecrypto nss > /dev/null || :
	@-${tpgp} --usecrypto nss
	@${echo} -n "--> openssl: HKP"
	@-${thkp} --usecrypto ssl > /dev/null || :
	@-${tpgp} --usecrypto ssl
	@${rpm} --import $(top_srcdir)/pubkeys/JBJ-GPG-KEY
	@${rpm} -Kv $(POPTPKGS) > tmp/popt.Kv
	@diff -u tmp/popt.Kv ref/popt.Kv
	@-${rpm} -Kvv --usecrypto bc   $(POPTPKGS) > /dev/null 2>&1
#	@-${rpm} -Kvv --usecrypto cdsa $(POPTPKGS) > /dev/null 2>&1
	@-${rpm} -Kvv --usecrypto gc   $(POPTPKGS) > /dev/null 2>&1
	@-${rpm} -Kvv --usecrypto ltc  $(POPTPKGS) > /dev/null 2>&1
	@-${rpm} -Kvv --usecrypto nss  $(POPTPKGS) > /dev/null 2>&1
	@-${rpm} -Kvv --usecrypto ssl  $(POPTPKGS) > /dev/null 2>&1
	@echo "=== $@ === done."

.PHONY:	check-sign
check-sign:
	@echo "=== $@ ==="
	@echo "--> DSA:"
	@${rpm} --delsign cudf-test/*.rpm > /dev/null
	@${rpm} -D'_gpg_name Donald' --addsign --nopassword cudf-test/*.rpm > /dev/null
	@${rpm} -qp cudf-test/*.rpm	> /dev/null
	@${rpm} -Kv cudf-test/*.rpm	> /dev/null
	@echo "--> RSA:"
	@${rpm} --delsign edos-test/*.rpm > /dev/null
	@${rpm} -D'_gpg_name Ronald' --addsign --nopassword edos-test/*.rpm > /dev/null
	@${rpm} -qp edos-test/*.rpm	> /dev/null
	@${rpm} -Kv edos-test/*.rpm	> /dev/null

.PHONY:	check-ssl
check-ssl: tssl
	@echo "=== $@ ==="

.PHONY:	check-markup
check-markup: $(POPTPKGS)
	@echo "=== $@ ==="
	@${rpm} -qp --qf '[%{*:yaml}\n]' popt*.rpm > tmp/popt.yaml
	@diff -u {tmp,ref}/popt.yaml
	@${rpm} -qp --qf '[%{*:xml}\n]' popt*.rpm > tmp/popt.xml
	@-diff -u {tmp,ref}/popt.xml

.PHONY: check-repo
check-repo: $(POPTPKGS)
	-rm -rf .repodata
	${vg} ${rpmrepo} --database --gzip .
	-${vg} ${rpmrepo} --database --bzip2 .
	-${vg} ${rpmrepo} --database --xz .
	-${vg} ${rpmrepo} --gzip .
	-${vg} ${rpmrepo} --bzip2 .
	-${vg} ${rpmrepo} --xz .
	${vg} ${rpmrepo} --uncompressed .
	@-diff -u repodata/primary.xml ref/primary.xml
	@-diff -u repodata/filelists.xml ref/filelists.xml
	@-diff -u repodata/other.xml ref/other.xml
# XXX repomd.xml always differs because st->st_ctime is used.
	@-diff -u repodata/repomd.xml ref/repomd.xml > /dev/null 2>&1

.PHONY:	check-macros
check-macros:
	@echo "=== $@ ==="
	@${rpm} -D 'foo bar' -E '%dump%trace%foo' > /dev/null 2>&1
	@${rpm} -D 'foo %foo' -E '%foo' > /dev/null 2>&1
	@${rpm} -D 'foo bing' -D 'foo bang' -D 'foo boom' -E '%{@foo}' > /dev/null 2>&1
	@${rpm}  -E '%(/bin/echo "-->       sh: Portable Shar!")'
	@-${rpm} -E '%{lua:print("-->      lua: Hard Rocks!")}'
	@-${rpm} -E '%{js:print("-->       js: Use GPSEE!")}'
	@-${rpm} -E '%{java:print("-->     java: Espresso Beans!");}'
	@-${rpm} -E '%{ruby:print "-->     ruby: Puppet Gems!"}'
	@-${rpm} -E '%{mruby:print "-->    mruby: Homebrew Suds!\n"}'
	@-${rpm} -E '%{tcl:puts "-->      tcl: Porticus Ports!"}'
	@-${rpm} -E '%{squirrel:print("--> squirrel: Eat Nuts!")}'
	@-${rpm} -E '%{perl:print "-->     perl: Artistic Scribbles!"}'
	@-${rpm} -E '%{python:print "-->   python: Snake Eggs!",}'
	@-${rpm} -E '%{ficl:.( -->     ficl: Das OpenBoot!)}'

check-srpm:
	if [ -f rpm.spec -a -f rpm/rpm-@PACKAGE_VERSION@.tar.gz -a -f rpm/cpu-os-macros.tar.gz ]; then \
	  ${rpm} -bs --nodeps rpm.spec && mv rpm/*.src.rpm . ; \
	fi

%.spec: $(wildcard %-*.src.rpm)
	@${rpm2cpio} $(wildcard $*-*.src.rpm) | ${cpio} -i --quiet $@

%.src.rpm: $(wildcard *.spec)
	@${rpm} -bs --nodeps $^

BUILD_DIRS ?= $(shell ${rpm} -qp --qf '%{NAME} ' $(wildcard *.src.rpm))

$(BUILD_DIRS): $(wildcard %-*.src.rpm)
	@echo "=== build $@ ==="
	${rpm} -i --nodeps $@-*.src.rpm
	(cd $@ && ${rpm} -q --specfile $@.spec && ${rpm} -q --specsrpm $@.spec && ${rpm} -q --specfile --specedit $@.spec && ${rpm} -bb --nodeps $@.spec || :) > /dev/null

define IGNORED_BY_GNU_MAKE:
.endif 

# neither .endif nor endef can be followed by a colon; each creates
# warnings or errors in one or other version of make. we need some
# magic to make them both work. Luckily, .endfor ignores the colon.

.for DUMMY in $(NO_SUCH_VARIABLE)
endef
.endfor :

check-build: $(BUILD_DIRS)
	@echo "=== $@ ==="
	@ls -1 */*.rpm | grep -v '^dir2symlink' > tmp/manifest
	@diff -u {tmp,ref}/manifest || ${cp} {tmp,ref}/manifest
	@rm -rf tmp/cachedb
	@${mkdir} -p tmp/cachedb/log tmp/cachedb/tmp
	@${cp} ref/DB_CONFIG tmp/cachedb
	@${rpmsolver} --import tmp/DSA.pubpem
	@${rpmsolver} --import tmp/RSA.pubpem
	@${rpmsolver} -i --justdb --nodeps edos-test/*.rpm
	@rm -rf tmp/hrmib tmp/repackage
	@mkdir -p tmp/hrmib/ tmp/repackage/
	@-${rpm} -e `${rpm} -qa | grep -v gpg-pubkey`
	@${rpm} -U -- edos-test/glass-1-*
	@${rpm} -U -- +car +turbo edos-test/{engine-1-*,wheel-2-*,door-1-*,window-0-*}
	@${rpm} -qaT | grep -v gpg-pubkey > tmp/edos.qa.1
	@diff -u {tmp,ref}/edos.qa.1 || ${cp} {tmp,ref}/edos.qa.1
	@${rpm} -U -- edos-test/engine-2-* -turbo
	@${rpm} -U -- edos-test/{wheel-3,tyre-1}-*
	@${rpm} -F -- edos-test/{door-2,window-1,glass-2}-*
	@${rpm} -qaT | grep -v gpg-pubkey > tmp/edos.qa.2
	@diff -u {tmp,ref}/edos.qa.2 || ${cp} {tmp,ref}/edos.qa.2
	@-${rpm} -e `${rpm} -qa | grep -v gpg-pubkey`

check-depsolver:
	@echo "=== $@ ==="
	@rm -rf tmp/solveA
	@${mkdir} -p tmp/solveA/log tmp/solveA/tmp
	@${cp} ref/DB_CONFIG tmp/solveA
	@${rpmsolverA} --import tmp/DSA.pubpem
	@${rpmsolverA} --import tmp/RSA.pubpem
	@${rpmsolverA} -i --justdb --nodeps edos-test/turbo-1* edos-test/wheel-2* edos-test/door-1*
	@rm -rf tmp/solveB
	@${mkdir} -p tmp/solveB/log tmp/solveB/tmp
	@${cp} ref/DB_CONFIG tmp/solveB
	@${rpmsolverB} --import tmp/DSA.pubpem
	@${rpmsolverB} --import tmp/RSA.pubpem
	@${rpmsolverB} -i --justdb --nodeps edos-test/engine-2*
	@rm -rf tmp/hrmib tmp/repackage
	@mkdir -p tmp/hrmib/ tmp/repackage/
	@-${rpm} -e `${rpm} -qa | grep -v gpg-pubkey`
	@${rpm} -U edos-test/car-1-*
	@${rpm} -qa
	@-${rpm} -e `${rpm} -qa | grep -v gpg-pubkey`

check-poky:
	@echo "=== $@ ==="
	@rm -rf tmp/solveA tmp/solveB tmp/solveC
	${poky_solverA} $(poky_pkgsdirA)/*.rpm
	${poky_solverB} $(poky_pkgsdirB)/*.rpm
	${poky_solverC} $(poky_pkgsdirC)/*.rpm
	@rm -rf tmp/hrmib tmp/repackage
	@mkdir -p tmp/hrmib/ tmp/repackage/
	find $(poky_pkgsdirA) $(poky_pkgsdirB) $(poky_pkgsdirC) -name '*.rpm' | sort -u > poky_manifest
	-for F in `cat poky_manifest`; do \
	    echo "-----> " $$F; \
	    ${rpm} -U --justdb --noscripts --notriggers --noparentdirs --nolinktos --stats $$F || : ; \
	    ${rpm} -Va --nofiles --noaid --noparentdirs --nolinktos || : ; \
	    ${rpm} -e --justdb --nodeps --noscripts --notriggers `${rpm} -qa | grep -v gpg-pubkey` || : ; \
	done

check-ACID:
	@echo "=== $@ ==="
	@${db_upgrade} Packages > /dev/null 2>&1
	@${db_checkpoint} -1	> /dev/null 2>&1
	@${rpm} -U ref/edos.manifest
	@${db_dump} Packages | ${db_load} Packages-NEW
	@${db_verify} Packages-NEW > /dev/null 2>&1
	@rm $(rpmdb)/Packages-NEW
	@${db_verify} Packages > /dev/null 2>&1
	@${db_stat} -CA	> /dev/null 2>&1
	@rm -f $(rpmdb)/Nvra
	@${rpm} -q tyre > /dev/null 2>&1
	@[ -f $(rpmdb)/Nvra ] || echo "NACK: Nvra not re-created!"
	@rm -f $(rpmdb)/Conflictname
	@${rpm} -q --whatconflicts tyre > /dev/null 2>&1
	@[ -f $(rpmdb)/Conflictname ] || echo "NACK: Conflictname not re-created!"
	@rm -f $(rpmdb_Tables)
	@${db_recover} -e
	@${rpm} -e car engine wheel door tyre window glass
	@${db_printlog} -r	> /dev/null 2>&1
	@${db_checkpoint} -1	> /dev/null 2>&1
	@${db_archive} -d

check-install:
	@echo "=== $@ ==="
	${rpm} -U --relocate /tmp/=$(testdir)/tmp/root/ --nodeps devtool-sanity/*.rpm
	${rpm} -U probes-test/probes-2*.rpm

# AL -- AsianLinux
# XXX notyet
#AL_mirror =	http://download.asianlinux.net/pub/AsianLinux
#AL2_uri =	$(AL_mirror)/2/i386/os/AsianLinux/RPMS/
# ALT -- Alt Linux
ALT_mirror =	http://distro.ibiblio.org/pub/linux/distributions/altlinux
ALT51_uri =	$(ALT_mirror)/5.1/branch/files/i586/RPMS/
ALT51_manifest = $(testdir)/ref/alt-minimal.x86_64.manifest
# ARK -- Ark Linux
ARK_uri =	http://arklinux.osuosl.org/dockyard-devel/RPMS/
# BL -- Berry Linux
# BP -- BlackPanther Linux
# XXX notyet
#BP_mirror =	http://ftp.freepark.org/pub/linux/distributions/blackpanther
#BP10_uri =	$(BP_mirror)/All/v10.1/media/ \
#		$(BP_mirror)/All/v10.1/media1/ \
#		$(BP_mirror)/All/v10.1/media2/ \
#		$(BP_mirror)/All/v10.1/media3/ \
#		$(BP_mirror)/All/v10.1/media4/ \
#		$(BP_mirror)/All/v10.1/media5/ \
#		$(BP_mirror)/All/v10.1/media6/ \
#		$(BP_mirror)/All/v10.1/media7/ \
#		$(BP_mirror)/All/v10.1/media8_games/ \
#		$(BP_mirror)/All/v10.1/media8_oooex/
# CM -- Caixa Magica
CM_mirror =	http://ftp.caixamagica.pt
CM12_uri =	$(CM_mirror)/12/oficial/i586/media/main/release/
CM12_manifest =	$(testdir)/ref/cm12-bash.i586.manifest
CM14_uri =	$(CM_mirror)/14/oficial/i586/media/main/release/
CM14_manifest =	$(testdir)/ref/cm14-minimal.i586.manifest
CM15_uri =	$(CM_mirror)/15/oficial/i586/media/main/release/
CM15_manifest =	$(testdir)/ref/cm15-medium.i586.manifest
# CAOS -- cAos
CAOS_mirror =	http://mirror.caoslinux.org
CAOS2_uri =	$(CAOS_mirror)/cAos-2/core/i386/
CAOS2_manifest = $(testdir)/ref/cAos2-minimal.i386.manifest
CAOS3_uri =	$(CAOS_mirror)/Caos-NSA-1.0/packages/i386/
# C -- CentOS
C_mirror =	http://ftp.linux.ncsu.edu/pub/CentOS/
C55_uri =	$(C_mirror)/5.5/os/i386/CentOS/
C54_uri =	http://vault.centos.org/5.4/os/i386/CentOS/
C53_uri =	http://vault.centos.org/5.3/os/i386/CentOS/
C52_uri =	http://vault.centos.org/5.2/os/i386/CentOS/
C51_uri =	http://vault.centos.org/5.1/os/i386/CentOS/
C50_uri =	http://vault.centos.org/5.0/os/i386/CentOS/
# XXX notyet
#C48_uri =	$(C_mirror)/4.8/os/i386/CentOS/RPMS/
#C39_uri =	$(C_mirror)/3.9/os/i386/RedHat/RPMS/

# G -- geode-RHEL6
G_mirror = http://wp3.jbj.org/geode-RHEL6
G_uri = $(SL_mirror)/RPMS
G_manifest = $(testdir)/ref/rhel6-bash.geode.manifest

# IDMS -- IDMS Linux
IDMS_uri =	http://download.idms-linux.org/trunk/i586/RPMS/
IDMS_manifest =	$(testdir)/ref/idms-minimal.i586.manifest
# MYTHDORA -- MythDora
# XXX notyet
#MYTHDORA_mirror = http://mirror.beww.org/mythdora
#MYTHDORA1223 =	$(MYTHDORA_mirror)/12.23/i386/os/Packages/

# MAG -- Mageia
#MAG_mirror =	ftp://mirrors.us.kernel.org/mandriva/Mandrakelinux/official
MAG2011_uri =	/home/i586-mageia-2011.0/i586/media/core/release/
MAG2011_manifest = $(testdir)/ref/mag2011-medium.i586.manifest

# MDV -- Mandriva
MDV_mirror =	http://mirrors.us.kernel.org/mandriva/Mandrakelinux/official
MDV2009_uri =	$(MDV_mirror)/2009.0/i586/media/main/release/
MDV2009_manifest = $(testdir)/ref/mdv2009-minimal.i586.manifest
MDV2010_uri =	$(MDV_mirror)/2010.0/i586/media/main/release/
MDV2010_manifest = $(testdir)/ref/mdv2010-minimal.i586.manifest
MDV20101_uri =	$(MDV_mirror)/2010.1/i586/media/main/release/
MDV20101_manifest = $(testdir)/ref/Mandriva2010.1-bash.i586.manifest
#MDV20102_uri =	$(MDV_mirror)/2010.2/i586/media/main/release/
#MDV20102_manifest = $(testdir)/ref/Mandriva2010.2-bash.i586.manifest
MDV2011_uri =	$(MDV_mirror)/2011/i586/media/main/release/
MDV2011_manifest = $(testdir)/ref/mdv2011-everything.i586.manifest
# MEEGO -- MeeGo
# XXX lt-find: rpmdav.c:1331: htmlParse: Assertion `he > h' failed.
MEEGO_mirror =	http://repo.meego.com/MeeGo/builds/1.0.99/daily/
MEEGO_uri =	$(MEEGO_mirror)/core/repos/ia32/packages/i586/ \
		$(MEEGO_mirror)/core/repos/ia32/packages/i686/ \
		$(MEEGO_mirror)/core/repos/ia32/packages/noarch/
MEEGO_manifest = $(testdir)/ref/MeeGo-everything.manifest
# MOBLIN -- Moblin
MOBLIN_mirror =	http://mirrors.kernel.org/moblin/releases
MOBLIN21_uri =	$(MOBLIN_mirror)/2.1/ia32/os/i586/ \
		$(MOBLIN_mirror)/2.1/ia32/os/noarch/
MOBLIN21_manifest = $(testdir)/ref/moblin21-everything.manifest
# MOMONGA -- Momonga Linux
MOMONGA_mirror = http://dist.momonga-linux.org/pub/momonga
MOMONGA7_uri =	$(MOMONGA_mirror)/7/Momonga/i686/os/Packages/
# OJUBA -- Ojuba Linux
OJUBA_mirror =	http://www.ojuba.org/downloads/releases
OJUBA4_uri =	$(OJUBA_mirror)/4/Everything/i386/os/Packages/
# OWL == OpenWall Linux
OWL_mirror =	http://distro.ibiblio.org/pub/linux/distributions/openwall/Owl
OWL2_uri =	$(OWL_mirror)/2.0-stable/i386/RPMS/
OWL3_uri =	$(OWL_mirror)/3.0-stable/i686/RPMS/
OWL2_manifest =	$(testdir)/ref/owl2-everything.i386.manifest
# PCLOS -- PCLinuxOS
# PLD -- PLD Linux
PLD_mirror =	http://ftp.pld-linux.org/dists
PLD3_uri =	$(PLD_mirror)/3.0/PLD/i486/RPMS/ \
		$(PLD_mirror)/3.0/PLD/i686/RPMS/ \
		$(PLD_mirror)/3.0/PLD/noarch/RPMS/
# RF -- RPM Forge
# SL -- Scientific Linux
SL_mirror =	http://ftp.scientificlinux.org/linux/scientific
SL60_uri =	$(SL_mirror)/6.0/i386/os/Packages
SL60_manifest = $(testdir)/ref/sl60-everything.manifest
SL55_uri =	$(SL_mirror)/55/i386/SL/
SL55_manifest = $(testdir)/ref/sl55-everything.manifest
SL54_uri =	$(SL_mirror)/54/i386/SL/
SL53_uri =	$(SL_mirror)/53/i386/SL/
SL52_uri =	$(SL_mirror)/52/i386/SL/
SL51_uri =	$(SL_mirror)/51/i386/SL/
SL50_uri =	$(SL_mirror)/50/i386/SL/
# XXX notyet
#SL48_uri =	$(SL_mirror)/48/i386/SL/
# SLES -- SuSE
SLES_mirror =	http://mirrors1.kernel.org/opensuse/distribution
SLES11_1_uri =	$(SLES_mirror)/11.1/repo/oss/suse/i586/
SLES11_2_uri =	$(SLES_mirror)/11.2/repo/oss/suse/i586/
SLES11_3_uri =	$(SLES_mirror)/11.3/repo/oss/suse/i586/
# TURBO -- TurboLinux
TURBO_mirror =	http://ftp.turbolinux.co.jp/pub/TurboLinux/TurboLinux
TURBO12_uri =	$(TURBO_mirror)/ia32/Client/12/turbolinux/
# UL -- Unity Linux
UL_mirror =	http://distro.ibiblio.org/pub/linux/distributions/unity/repo
UL_uri =	$(UL_mirror)/2009/x86_64/unity/
UL_manifest =	$(testdir)/ref/ul2011-minimal.i586.manifest
# XXX UL_manifest =	$(testdir)/ref/unity-minimal.x86_64.manifest
# VINE -- Vine Linux
VINE_mirror =	http://ring.u-toyama.ac.jp/pub/linux/Vine
VINE51_uri =	$(VINE_mirror)/Vine-5.1/i386/Vine/RPMS/
VINE51_manifest = $(testdir)/ref/vine51-everything.manifest
# YDL -- Yellow Dog Linug (ppc)
YDL_mirror =	http://ydl.oregonstate.edu/releases
YDL61_uri =	$(YDL_mirror)/yellowdog-6.1/os/YellowDog/
YDL62_uri =	$(YDL_mirror)/yellowdog-6.2/os/YellowDog/
# YL -- Yoper Linux
YL_mirror =	http://ftp.yoper-linux.org/pub/yoper/pkg
YL_uri =	$(YL_mirror)/RPMS.rocketfuel/
#YLDYNA_uri =   $(YL_mirror)/RPMS.dynamite/

DISTROS = \
	ALT51 \
	ARK \
	CM12 CM14 CM15 \
	CAOS2 CAOS3 \
	C55 C54 C53 C52 C51 C50 \
	G IDMS \
	MDV2009 MDV2010 MDV2011 MAG2011 \
	MEEGO MOBLIN21 \
	OJUBA4 \
	OWL2 OWL3 \
	PLD3 \
	SL55 SL54 SL53 SL52 SL51 SL50 \
	SLES11_3 SLES11_2 SLES11_1  \
	UL \
	YDL62 YDL61 \
	YL

now =	date -u --rfc-3339=seconds

manifests:
	@echo "=== $@ ==="
	@for D in ${DISTROS}; do \
	  rm -f $${D}/manifest; \
	  make $${D}/manifest; \
	done

.PRECIOUS: %/manifest
%/manifest:
	@echo "=== $@ ==="
	@mkdir -p $(@D)
	@if [ ! -f $@ ]; then \
	  rm -f ${@D}/uri; \
	  for U in ${$(@D)_uri}; do echo "$${U}" >> ${@D}/uri; done; \
	  if [ -z "${${@D}_manifest}" ]; \
	    then ${find} ${$(@D)_uri} -maxdepth 1 | grep 'rpm$$' > $@; \
	    else grep -v '^#' ${${@D}_manifest} > $@; \
	  fi; \
	  ./spew ${@D} > ${@D}/distros.mongo; \
	  ls -l ${@D}/uri $@ ${@D}/distros.mongo; \
	fi

XXXcache =	/var/www/html/cache
XXXIDMS =	$(XXXcache)/i586-idms-linux/RPMS/
XXXOWL2 =	$(XXXcache)/i386-openwall-OWL2/RPMS/
XXXOWL3 =	$(XXXcache)/i686-openwall-OWL3/RPMS/
XXXALT51 =	$(XXXcache)/x86_64-alt-sisyphus/RPMS/

XXXG =		/var/www/html/geode-RHEL6/RPMS/

XXXCM15 =	/home/i586-caixa-cm15/ftp.caixamagica.pt/15/oficial/i586/
XXXCM14 =	/home/i586-caixa-cm14/ftp.caixamagica.pt/14/oficial/i586/media/main/release/

XXXMAG2011 =	/home/i586-mageia-2011.0/i586/media/core/release/
XXXUL2011 =	/home/i586-unity-linux/repo/current/i586/
XXXMDV2011 =	/home/i586-mandriva-mdv2011.0/i586/media/main/release/
XXXMDV2010 =	/home/i586-mandriva-mdv2010.0/i586/media/main/release/
XXXMDV2009 =	/home/i586-mandriva-mdv2009.0/i586/media/main/release/


.PRECIOUS: %/RPMS
%/RPMS: %/manifest
	@echo "=== $@ ==="
	@if [ ! -d $@ ]; then \
	  mkdir -p $@; \
	  echo "==> Download $@ ..."; \
	  case $(@D) in \
	  G)		$(cp) `cat ${G_manifest}` $@ ;; \
	  IDMS)		$(ln) -s ${XXXIDMS}/* $@ ;; \
	  OWL2)		$(ln) -s ${XXXOWL2}/* $@ ;; \
	  OWL3)		$(ln) -s ${XXXOWL3}/* $@ ;; \
	  ALT51)	$(ln) -s ${XXXALT51}/* $@ ;; \
	  CM15) $(cp) $$(cat $< | sed -e 's,^.*/\(media/[^/]*/release/[^/]*\)/,${XXXCM15}\1,') $@ ;; \
	  CM14)	$(cp) $$(cat $< | sed -e 's,^.*/,${XXXCM14}/,') $@ ;; \
	  MAG2011) $(cp) $$(cat $< | sed -e 's,^.*/,${XXXMAG2011},') $@ ;; \
	  UL) $(cp) $$(cat $< | sed -e 's,^.*/\([^/]*\)/,${XXXUL2011}/\1/,') $@ ;; \
	  MDV2011) $(cp) $$(cat $<) $@ ;; \
	  MDV2010) $(cp) $$(cat $< | sed -e 's,^.*/,${XXXMDV2010}/,') $@ ;; \
	  MDV2009) $(cp) $$(cat $< | sed -e 's,^.*/,${XXXMDV2009}/,') $@ ;; \
	  *)		cd $@ && ${wget} -c -i $(testdir)/$(@D)/manifest ;; \
	  esac; \
	fi

.PRECIOUS: %/todo
%/todo: %/RPMS
	@echo "=== $@ ==="
	@if [ ! -f $@ ]; \
	  then ${find} $< -name '*.rpm' | sort -u > $@; \
	fi

.PRECIOUS: %/json
%/json: %/RPMS
	@echo "=== $@ ==="
	@rm -f ${@D}/export
	@echo "`${now}` ==> ${@D}: Exporting ..." | tee -a ${@D}/export
	for F in $</*.rpm; do \
	  echo "--> $$F"; \
	  ${rpm} -qp --json $$F | tee -a $@ | ${mongo} ${mongoserver}/${@D} > /dev/null; \
	done
	@echo "`${now}` <== ${@D}: Exported." | tee -a ${@D}/export

.PRECIOUS: %/repodata
%/repodata: %/RPMS
	@echo "=== $@ ==="
	@rm -f ${@D}/generate
#	-rm -f ${@D}/repodata
	@echo "`${now}` ==> ${@D}: Generating ..." | tee -a ${@D}/generate
	${rpmrepo} --uncompressed --database --stats ${@D} 2>&1 | tee -a ${@D}/generate
	@ls -al ${@D}/repodata
	@echo "`${now}` <== ${@D}: Generated." | tee -a ${@D}/generate

#	  -D '_dbi_tags Packages:Name:Nvra:Providename:Dirnames:Basenames:Filepaths'

.PRECIOUS: %/solve
%/solve: %/todo
	@echo "=== $@ ==="
	@rm -f ${@D}/import
	@echo "`${now}` ==> ${@D}: Creating ..." | tee -a ${@D}/import
	@mkdir -p $@/log $@/tmp
	@${cp} $(testdir)/ref/DB_CONFIG $@
	@echo "`${now}` <== ${@D}: Created." | tee -a ${@D}/import
	@echo "`${now}` ==> ${@D}: Importing ..." | tee -a ${@D}/import
	@-${rpm} -i -D '_dbpath ${@D}/solve' --justdb --stats \
	  -D '_hrmib_path %{nil}' \
	  -D '__dbi_txn create nofsync lock log mpool txn auto_commit thread thread_count=64' \
	  --noaid --ignoresize --nodeps --noorder \
	  $< \
	  2>&1 | tee -a ${@D}/import
	ls -al ${@D}/solve
	@echo "`${now}` <== ${@D}: Imported." | tee -a ${@D}/import

%/digests: %/RPMS
	@echo "=== $@ ==="
	@rm -f $@
	@touch $@

%/sha256 %/sha224 %/sha384 %/sha512 %/md5 %/sha1: %/digests
	@echo "=== $@ ==="
	@-(echo "`${now}` ==> ${@D}: Generating ..."; \
	  ${rpmdigest} --${@F} ${@D}/RPMS > ${@D}/${@F} 2>&1; \
	  echo "`${now}` <== ${@D}: Generated."; \
	  ls -l ${@D}/${@F}; \
	) | tee -a ${@D}/digests

%/mtree: %/sha256
	@echo "=== $@ ==="
	@-(echo "`${now}` ==> ${@D}: Generating ..."; \
	  (${mtree} -c -p ${@D}/RPMS | tee ${@D}/mtree | ${mtree} -p ${@D}/RPMS) 2>&1; \
	  echo "`${now}` <== ${@D}: Generated."; \
	  ls -l ${@D}/mtree; \
	) | tee -a ${@D}/digests

Digest-%: %/mtree
	@echo "=== $@ ==="

Install-%: %/solve
	@echo "=== $@ ==="
	@rm -f $(<D)/install
	@echo "`$(now)` ==> ${<D}: Installing ..." | tee -a $(<D)/install
	@${sudo} rm -rf $(<D)/ROOT
	@cat -n $(<D)/todo | sort -R | cut -f2- > $(<D)/todo.random
	@mkdir -p $(<D)/ROOT/var/lib/rpm/log $(<D)/ROOT/var/lib/rpm/tmp
	@${cp} $(testdir)/ref/DB_CONFIG $(<D)/ROOT/var/lib/rpm
	@-mkdir -p $(<D)/ROOT/proc && sudo mount -o bind /proc $(<D)/ROOT/proc
#	@-mkdir -p $(<D)/ROOT/sys  && ${sudo} mount -o bind /sys $(<D)/ROOT/sys 
#	@-mkdir -p $(<D)/ROOT/dev/pts
#	@-${sudo} mount -o bind /dev $(<D)/ROOT/dev
#	@-${sudo} mount -o bind /dev/pts $(<D)/ROOT/dev/pts
	@-${sudo} ${rpm} -Uvh --stats --nofdigests \
	  -r $(testdir)/$(<D)/ROOT \
	  -D '__dbi_txn create nofsync lock log mpool txn auto_commit thread thread_count=64' \
	  -D '_dbpath /var/lib/rpm' \
	  -D '_hrmib_path /var/cache/hrmib/%{___NVRA}' \
	  -D '_solve_A $(<D)/solve' \
	  --noparentdirs --nolinktos \
	  $(<D)/todo.random 2>&1 | tee -a $(<D)/install
	-${sudo} rm -f $(<D)/ROOT/var/lib/rpm/Filepaths
	-${sudo} ${rpm} -qf /bin/sh --stats \
	  -D '_dbpath $(testdir)/$(<D)/ROOT/var/lib/rpm'
	-${sudo} rm -f $(<D)/ROOT/var/lib/rpm/Providename
	-${sudo} ${rpm} -q --whatprovides kernel  --stats \
	  -D '_dbpath $(testdir)/$(<D)/ROOT/var/lib/rpm'
	-${sudo} ${rpm} -Va --nofiles --noaid --stats \
	  -D '_dbpath $(testdir)/$(<D)/ROOT/var/lib/rpm' \
	  --noparentdirs --nolinktos \
	  2>&1 | tee -a $(<D)/install
#	@-${sudo} umount $(<D)/ROOT/dev/pts
#	@-${sudo} umount $(<D)/ROOT/dev
#	@-${sudo} umount $(<D)/ROOT/sys 
	@-${sudo} umount $(<D)/ROOT/proc
#	@rm -f $(<D)/todo.random
	@echo "`$(now)` <== $(<D): Installed." | tee -a $(<D)/install
	@-${sudo} rm -rf tmp/solveA tmp/solveB tmp/solveC
	@-[ -d $(<D)/ROOT/tmp/.private ] && ${sudo} $(chattr) -a $(<D)/ROOT/tmp/.private
	@-${sudo} rm -rf $(<D)/ROOT/
	@-./spew $(<D) > $(<D)/install.mongo
#	@-${mongo} ${mongoserver} < ${<D}/install.mongo

Verify-%: %/solve
	@echo "=== $@ ==="
	rm -f $(<D)/verify
	@echo "`$(now)` ==> ${<D}: Verifying ..." | tee -a $(<D)/verify
	@-for F in `cat $(<D)/todo`; do \
	  [ -d $(<D)/VROOT/tmp/.private ] && ${sudo} $(chattr) -a $(<D)/VROOT/tmp/.private ; \
	  ${sudo} rm -rf $(<D)/VROOT; \
	  mkdir -p $(<D)/VROOT/var/lib/rpm/log $(<D)/VROOT/var/lib/rpm/tmp; \
	  ${cp} $(testdir)/ref/DB_CONFIG $(<D)/VROOT/var/lib/rpm; \
	  echo "-----> " $$F; \
	  ${sudo} ${rpm} -Uv --nofdigests --stats \
	    -r $(testdir)/$(<D)/VROOT \
	    -D '_dbpath /var/lib/rpm' \
	    -D '_hrmib_path /var/cache/hrmib/%{___NVRA}' \
	    -D "_solve_dbpath $(testdir)/$(<D)/solve" \
	    --noparentdirs --nolinktos \
	    $$F \
	  2>&1 || : ; \
	  ${rpm} -Va --nofiles --noaid --stats \
	    -D '_dbpath $(testdir)/$(<D)/VROOT/var/lib/rpm' \
	    --noparentdirs --nolinktos \
	  2>&1 || : ; \
	  ${sudo} ${rpm} -e --stats \
	    -r $(testdir)/$(<D)/VROOT \
	    -D '_dbpath /var/lib/rpm' \
	    -D '_hrmib_path /var/cache/hrmib/%{___NVRA}' \
	    -D '_repackage_all_erasures 0' \
	    `${sudo} ${rpm} -D '_dbpath $(testdir)/$(<D)/VROOT/var/lib/rpm' -qa` \
	  2>&1 || : ; \
	  [ -d $(<D)/VROOT/tmp/.private ] && ${sudo} $(chattr) -a $(<D)/VROOT/tmp/.private ; \
	  ${sudo} rm -rf $(<D)/VROOT/ ; \
	done | tee -a $(<D)/verify
	@echo "`$(now)` <== ${<D}: Verified." | tee -a $(<D)/verify
	@-${sudo} rm -rf tmp/solveA tmp/solveB tmp/solveC

Eopts =	--nodeps
Uopts =	$(Eopts)
TUopts = --relocate "/tmp=${testdir}/tmp" --noparentdirs $(Uopts)
TEopts = $(Eopts)
.PHONY:	check-triggers
check-triggers:
	@echo "=== $@ ==="
	@-${rpm} -e --noscripts --notriggers --allmatches '^triggers-.*' > /dev/null 2>&1
	@echo "--> N:"
	@${rpm} -U ${TUopts} triggers-N/triggers-N*.rpm
	@${rpm} -e ${TEopts} triggers-N-a triggers-N-b
	@echo "--> NA:"
	@${rpm} -U ${TUopts} triggers-NA/triggers-NA*.rpm
	@${rpm} -e ${TEopts} triggers-NA-a triggers-NA-b
	@echo "--> P:"
	@${rpm} -U ${TUopts} triggers-P/triggers-P*.rpm
	@${rpm} -e ${TEopts} triggers-P-a triggers-P-b
	@echo "--> F:"
	@${rpm} -U ${TUopts} triggers-F/triggers-F*.rpm
	@${rpm} -e ${TEopts} triggers-F-a triggers-F-b
	@echo "--> FP:"
	@${rpm} -U ${TUopts} triggers-FP/triggers-FP*.rpm
	@${rpm} -e ${TEopts} triggers-FP-a triggers-FP-b
	@echo "--> D:"
	@${rpm} -U ${TUopts} triggers-D/triggers-D*.rpm
	@${rpm} -e ${TEopts} triggers-D-a triggers-D-b
	@echo "--> DP:"
	@${rpm} -U ${TUopts} triggers-DP/triggers-DP*.rpm
	@${rpm} -e ${TEopts} triggers-DP-a triggers-DP-b

check-query:
	@echo "=== $@ ==="
#XXX	@${rpm} -qW . > /dev/null 2>&1
	@${rpm} -qa > /dev/null
	@${rpm} -qa 'arch=noarch' > /dev/null
	@${rpm} -qa 'arch=!noarch' > /dev/null
	@${rpm} -qa -c > /dev/null
	@${rpm} -qa -d > /dev/null
	@${rpm} -qa -s > /dev/null
	@${rpm} -qa --dump > /dev/null
	@${rpm} -q --querybynumber 1 > /dev/null
	@${rpm} -qal > /dev/null
	@${rpm} -qalv > /dev/null
	@${rpm} -qa --qf '%{INSTALLTIME} %{INSTALLTIME:date} %{INSTALLTIME:day} %{INSTALLTIME:dec} %{INSTALLTIME:hex} %{INSTALLTIME:oct}\n' > /dev/null
	@${rpm} -qa --qf '%{DBINSTANCE} %{HDRID} %{PKGID} %{SOURCEPKGID}\n' > /dev/null
	@${rpm} -qa --qf '%{PACKAGESIZE:rpn(511,+,512,/)}\n' > /dev/null
	@${rpm} -qa --qf '%{PACKAGESTAT:base64}\n' > /dev/null
	@${rpm} -qa --qf '%{PACKAGESTAT:stat(atime,ctime,blksize,blocks,dev,gid,gname,ino,link,mode,mtime,nlink,rdev,size,uid,uname)}\n' > /dev/null
	@${rpm} -qa --qf '[%{SUGGESTS}\n]' > /dev/null
	@${rpm} -qa --qf '[%{providesqlentry}\n] [%{requiresqlentry}\n] [%{conflictsqlentry}\n] [%{obsoletesqlentry}\n] [%{filessqlentry1}\n] [%{filessqlentry2}\n]' > /dev/null
	@${rpm} -qa --qf '[%{FILENAMES:shescape}: %{FILEMTIMES:shescape} %{FILEFLAGS:perms} %{FILEMODES:fflags}\n]' > /dev/null
	@${rpm} -qa --qf '[%{FILENAMES}: %{FILECLASS}\n]' > /dev/null
	@${rpm} -qa --qf '[%{FILEPATHS}: %{FILECONTEXTS}\n]' > /dev/null
	@${rpm} -qa --qf '[%{FILENAMES}: %{FSCONTEXTS}\n]' > /dev/null
	@${rpm} -qa --qf '[%{FILENAMES}: %{RECONTEXTS}\n]' > /dev/null
	@${rpm} -qa --qf '[%{FILENAMES}: %{FILEPROVIDE}\n]' > /dev/null
	@${rpm} -qa --qf '[%{FILENAMES}: %{FILEREQUIRE}\n]' > /dev/null
	@${rpm} -qa --qf '[%{FSNAMES}: %{FSSIZES}\n]' > /dev/null

check-rpmv3:
	@echo "=== $@ ==="
	@${rpm} -qlp simplestRPMv3-1.0-2.aix5.3.noarch.rpm

check-verify:
	@echo "=== $@ ==="
	@-${rpm} -e probes devtool-sanity
	@-${rpm} -U --justdb --nodeps tmp/manifest
	@-${rpm} -V probes devtool-sanity > /dev/null
	@-${rpm} -Va > /dev/null
#XXX	@-${rpm} -Vp tmp/manifest
	@-${rpm} -e --noscripts --notriggers --allmatches '^triggers-.*'
	@-${rpm} -e car engine wheel door tyre window glass
	@-${rpm} -e battery bicycle electric-engine gasoline-engine
	@-${rpm} -e solar-collector tire turbo
	@-${rpm} -e --justdb popt
	@-${rpm} -e probes
	@-${rpm} -e devtool-sanity

check-tools:
	@echo "=== $@ ==="
	@-${find} $(POPTURI) -type f -maxdepth 1 > tmp/find
	@-mkdir -p tmp/wget && ${wget} -i tmp/find -P tmp/wget > /dev/null 2>&1
	-mkdir -p tmp/cp && ${cp} -R $(POPTURI) tmp/cp
	-diff -ru tmp/{wget,cp}
	@-${grep} -i HREF $(POPTURI) > tmp/grep

check-sql:
	@echo "=== $@ ==="
	@echo "--> sqlite3:"
	@-${rpm} -e --nodeps --noscripts sql-test
	@rm -rf tmp/sqldb*
	@-${sqlite3} -echo tmp/sqldb ".read ref/sqldb-basic.sql" >& tmp/sqldb-basic.out
	@-diff -u {tmp,ref}/sqldb-basic.out || cp {tmp,ref}/sqldb-basic.out
	@echo "--> rpmsql STRING:"
	@rm -rf tmp/sqldb*
	@-${rpmsql} -echo tmp/sqldb ".read ref/sqldb-basic.sql" >& tmp/sqldb-basic.out
	@-diff -u {tmp,ref}/sqldb-basic.out
	@echo "--> rpmsql STDIN:"
	@rm -rf tmp/sqldb*
	@-${rpmsql} -echo tmp/sqldb < ref/sqldb-basic.sql >& tmp/sqldb-basic.out
	@-diff -u {tmp,ref}/sqldb-basic.out
	@echo "--> rpmsql FILE:"
	@rm -rf tmp/sqldb*
	@-${rpmsql} -echo tmp/sqldb `pwd`/ref/sqldb-basic.sql >& tmp/sqldb-basic.out
	@-diff -u {tmp,ref}/sqldb-basic.out
	@echo "--> rpmsql INTERACTIVE:"
	@rm -rf tmp/sqldb*
	@-echo ".read ref/sqldb-basic.sql" | ${rpmsql} -echo tmp/sqldb >& tmp/sqldb-basic.out
	@-diff -u {tmp,ref}/sqldb-basic.out
	@echo "--> rpm %{sql:...} macro:"
	@rm -rf tmp/sqldb*
	@-${rpm} -E '%{sql -echo tmp/sqldb:.read ref/sqldb-basic.sql}' >& tmp/sqldb-basic.out
	@-diff -u {tmp,ref}/sqldb-basic.out
	@rm -rf tmp/sqldb*
	@echo "--> rpm %post -p <sql> scriptlet:"
	@-${rpm} -U ${Uopts} sql-test/sql-test*.noarch.rpm
	@echo "--> rpm %preun -p <sql> scriptlet:"
	@-${rpm} -e ${Eopts} sql-test 2>&1

check-gsr:
	@echo "=== $@ ==="
	@-${gsr} -c 'print("-->       js: Use GPSEE!")'
	@-${gsr} -f gpsee/gffi.js

check-convert:
	@echo "=== $@ ==="
	case `rpm --version` in \
	*5.4*|*5.3*) echo "Nothing to do." ;; \
	*)	echo "Converting system database."; \
		rm -rf newdb olddb; \
		mkdir -p olddb; \
		echo "--> copy the system rpmdb"; \
		cp /var/lib/rpm/Packages olddb; \
		echo "--> rebuild to renumber the header instances"; \
		rpm \
		  --dbpath $(testdir)/olddb \
		  --rebuilddb; \
		mkdir -p newdb/log newdb/tmp olddb; \
		echo "--> convert header instances to network order"; \
		@__DB_DUMP@ olddb/Packages \
		  | sed \
		    -e 's/^type=hash$$/type=btree/' \
		    -e '/^h_nelem=/d' \
		    -e 's/^ \(..\)\(..\)\(..\)\(..\)$$/ \4\3\2\1/' \
		  | @__DB_LOAD@ newdb/Packages; \
		echo "--> regenerate the indices"; \
		cp ../rpmdb/DB_CONFIG newdb; \
		${rpm} \
		  --dbpath $(testdir)/newdb \
		  --rebuilddb -vv; \
		ls -l newdb; \
		echo "--> test the conversion"; \
		${rpm} --dbpath $(testdir)/newdb -qa; \
		${rpm} --dbpath $(testdir)/newdb -q rpm; \
		;; \
	esac

check-lua:
	@echo "=== $@ ==="
	@-${rpm} -E '%{lua:print("-->      lua: Hard Rocks!")}'
	@-${rpm} -E '%{lua}'
	@-${rpm} -e lua-test
	@-${rpm} -U lua-test/lua-test-*.noarch.rpm
	@-${rpm} -e lua-test

INTERPRETERS =	lua tcl perl # ruby # python
check-dir2symlink:
	@echo "=== $@ ==="
	@-for i in ${INTERPRETERS}; do \
	  ${echo} "--- $$i ---" ; \
	  ${rpm} -e --noscripts \
	    dir2symlink-$${i} || : ; \
	  ${rpm} -U --noparentdirs \
	    dir2symlink/dir2symlink-$${i}-1-1.noarch.rpm ; \
	  ls -l /tmp/$${i}.d ; \
	  ${rpm} -U --noparentdirs \
	    dir2symlink/dir2symlink-$${i}-2-1.noarch.rpm ; \
	  ls -l /tmp/$${i}.d /tmp/.$${i}.d/* ; \
	  ${rpm} -e \
	    dir2symlink-$${i} ; \
	  ls -l /tmp/.$${i}.d /tmp/$${i}.d || : ; \
	done
	@echo ""

check-perl:
	@echo "=== $@ ==="
	-../libtool --mode=execute vg ${rpm} -E '%{perl:print "-->     perl: Artistic Scribbles!"}'

gitdir = $(testdir)/tmp/git
tgit =	${abs_top_builddir}/rpmio/tgit --git-dir="$(gitdir)" --work-tree="$(gitdir)" --
git =	@__GIT@ --git-dir="$(gitdir)/.git" --work-tree="$(gitdir)"

check-git:
	@echo "=== $@ ==="
	@rm -rf ${gitdir} tmp/git.out
	@mkdir -p ${gitdir}
if WITH_LIBGIT2
	@${echo} -n "--> rpmgit:"
	@if test ! -f $${HOME}/.gitconfig; then \
	  { echo "[user]"; \
	    echo "	email = someone@nowhere.org"; \
	    echo "	name = Jack Spratt"; \
	    echo "[color]"; \
	    echo "	ui = auto"; \
	  } > $${HOME}/.gitconfig; \
	fi 
	@${echo} -n " init"
	@${tgit} init --initial-commit >> tmp/git.out
	@-(cd ${gitdir} && ${git} log --stat 2>&1) >> tmp/git.out
	@${echo} -n " add"
	@mkdir -p ${gitdir}/bar ${gitdir}/bing/bang
	@${echo} "foo" > ${gitdir}/foo
	@${echo} "bar/baz" > ${gitdir}/bar/baz
	@${echo} "bing/bang/boom" > ${gitdir}/bing/bang/boom
	@${tgit} add foo bar/baz bing/bang/boom	
	@-(cd ${gitdir} && ${git} log --stat 2>&1) >> tmp/git.out
	@${echo} -n " commit"
	@${tgit} commit -m "RPM add commit"
	@-(cd ${gitdir} && ${git} log --stat 2>&1) >> tmp/git.out
	@${echo} -n " modify"
	@cp ${gitdir}/foo ${gitdir}/bar/baz
	@-(cd ${gitdir} && ${git} log --stat 2>&1) >> tmp/git.out
	@${tgit} diff
	@${tgit} commit -a -m "RPM modify commit"
	@-(cd ${gitdir} && ${git} log --stat 2>&1) >> tmp/git.out
	@${echo}  " checkout(NOTYET)"
	@echo "FOO" > ${gitdir}/foo
	@rm -f ${gitdir}/bar/baz ${gitdir}/bing/bang/boom
	@${tgit} diff
	@${git} checkout foo bar/baz bing/bang/boom	# XXX
	@${tgit} diff
	@ls -1 ${gitdir}/foo ${gitdir}/bar/baz ${gitdir}/bing/bang/boom
	@-(cd ${gitdir} && ${git} log --stat 2>&1) >> tmp/git.out
	@${echo} ""
	@-diff -u tmp/git.out ref/git.out || cp tmp/git.out ref/git.out
	@${tgit} blame foo
	@${tgit} status --long
	@${tgit} log
endif

check-js:
	@echo "=== $@ ==="
	-../libtool --mode=execute vg ${rpm} -E '%{js:print("yadda-yadda")}'

odbc_uri_mysql =	mysql://luser:jasnl@harwich.jbj.org/test
odbc_uri_postgres =	postgres://luser:jasnl@harwich.jbj.org/test

check-odbc:
	@echo "=== $@ ==="
	@rm -f tmp/odbc.out
	@-(${todbc} ${odbc_uri_mysql} 2>&1) >> tmp/odbc.out
	@-(${todbc} ${odbc_uri_postgres} 2>&1) >> tmp/odbc.out
	@-diff -u tmp/odbc.out ref/odbc.out || cp tmp/odbc.out ref/odbc.out

test:	check-init \
	check-pubkeys \
	check-markup \
	check-macros \
	check-build \
	check-sign \
	check-ACID \
	check-install \
	check-query \
	check-verify \
	check-rpmv3 \
	check-triggers \
	check-tools \
	check-sql \
	check-dir2symlink \
	check-lua \
	check-git \
	check-git
#	check-convert
#	check-repo

check-local: check-init

clean-local:
	rm -f genpgp.h genssl.h
	$(sudo) rm -rf */ROOT tmp/repackage tmp/solve*
	rm -rf tmp $(BUILD_DIRS)
	rm -rf repodata/
