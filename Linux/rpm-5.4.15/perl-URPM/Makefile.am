# Makefile for rpm scripts.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = \
	ChangeLog \
	Makefile.am \
	Makefile.in \
	Makefile.PL \
	Makefile.PL.in \
	MANIFEST \
	META.yml \
	NEWS \
	.perl_checker \
	README \
	t/00prepare.t \
	t/buggy_synthesis.cz \
	t/empty_synthesis.cz \
	t/fatal.t \
	TODO \
	t/parse.t \
	t/pod.t \
	t/res/basesystem-minimal.resolve.nosuggests \
	t/res/basesystem-minimal.resolve.nosuggests.distepoch \
	t/res/basesystem.resolve.suggests \
	t/res/basesystem.resolve.suggests.distepoch \
	t/resolve.t \
	t/res/rpmdb_btreebig/Packages \
	t/res/rpmdb_hashlittle/Packages \
	t/res/synthesis.hdlist_distepoch.xz \
	t/res/synthesis.hdlist_src.xz \
	t/res/synthesis.hdlist.xz \
	t/res/task-kde4.resolve.suggests \
	t/res/task-kde4.resolve.suggests.distepoch \
	t/rpmdb.t \
	t/sort_graph.t \
	t/synthesis.t \
	t/test-rpm.spec \
	typemap \
	URPM/Build.pm \
	URPM/.perl_checker \
	URPM.pm \
	URPM/Query.pm \
	URPM/Resolve.pm \
	URPM/Signature.pm \
	URPM.xs

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

