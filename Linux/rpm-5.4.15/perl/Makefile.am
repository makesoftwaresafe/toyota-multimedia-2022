# Makefile for rpm scripts.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = \
	typemap \
	Makefile.PL.in \
	rpmxs.c rpmxs.h \
	RPM.xs RPM.pm t/01.rpm.t \
	RPM/Constant.pm RPM_Constant.xs t/02.rpmconstant.t \
	RPM_Header.xs RPM/Header.pm t/03.header.t \
	RPM_Transaction.xs RPM/Transaction.pm t/04.transaction.t \
	RPM_PackageIterator.xs RPM/PackageIterator.pm t/05.packageiterator.t \
	RPM_Problems.xs RPM/Problems.pm t/06.problems.t \
	RPM_Files.xs RPM/Files.pm t/07.files.t \
	RPM_Dependencies.xs RPM/Dependencies.pm t/08.dependencies.t \
	RPM_Spec.xs RPM/Spec.pm t/09.spec.t \
	RPM/Sign.pm t/10.sign.t
	t/00.pod.coverage.t t/00.pod.t \
	t/gnupg/passphrase t/gnupg/pubring.gpg t/gnupg/random_seed \
	t/gnupg/secring.gpg  t/gnupg/test-key.gpg  t/gnupg/trustdb.gpg \
	hdlist-test.hdr test-rpm-1.0-1.noarch.rpm test-rpm-1.0-1.src.rpm \
	test-rpm.spec

noinst_DATA = .made_mod

.made_mod: Makefile.perl $(EXTRA_DIST) MANIFEST
	$(MAKE) -f Makefile.perl VPATH=$(VPATH) || \
	$(MAKE) -f Makefile.perl VPATH=$(VPATH)

test: .made_mod
	$(MAKE) -f Makefile.perl test

MANIFEST: Makefile
	echo -n > MANIFEST
	for i in $(EXTRA_DIST); do \
		echo $$i >> MANIFEST; \
	done

Makefile.perl: Makefile.PL
	$(PERL) Makefile.PL MAKEFILE=$@

install-data-local:
	$(MAKE) -f Makefile.perl PREFIX=$(prefix) install || \
	$(MAKE) -f Makefile.perl PREFIX=$(prefix) install

CLEANFILES = *~

.PHONY: test

LTLIBS = \
	@abs_top_builddir@/rpmconstant/.libs \
	@abs_top_builddir@/build/.libs \
	@abs_top_builddir@/lib/.libs \
	@abs_top_builddir@/rpmdb/.libs \
	@abs_top_builddir@/rpmio/.libs \
	@abs_top_builddir@/misc/.libs \
	@abs_top_builddir@/popt/.libs

check-local:
	-env LD_LIBRARY_PATH="`echo $(LTLIBS) | tr ' ' ':'`" $(MAKE) -f Makefile.perl test

clean-local:
	if [ -f Makefile.perl ]; then $(MAKE) -f Makefile.perl realclean; fi
	rm -f Makefile.perl Makefile.perl.old

