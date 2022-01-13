# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

LINT = splint
MCCABE = pmccabe
DB_SQL_CODEGEN = ../db3/db_sql_codegen

SUBDIRS = # tests

AM_CPPFLAGS =

AM_CFLAGS = # $(OPENMP_CFLAGS)

EXTRA_DIST = \
	ne_acl3744.c \
	ne_gnutls.c \
	ne_oldacl.c \
	ne_openssl.c \
	ne_sspi.c \
	ne_stubssl.c

EXTRA_PROGRAMS =

noinst_HEADERS = \
	memleak.h \
	ne_207.h \
	ne_acl.h \
	ne_acl3744.h \
	ne_alloc.h \
	ne_auth.h \
	ne_basic.h \
	ne_compress.h \
	ne_dates.h \
	ne_defs.h \
	ne_i18n.h \
	ne_internal.h \
	ne_locks.h \
	ne_md5.h \
	ne_ntlm.h \
	ne_pkcs11.h \
	ne_private.h \
	ne_privssl.h \
	ne_props.h \
	ne_redirect.h \
	ne_request.h \
	ne_session.h \
	ne_socket.h \
	ne_ssl.h \
	ne_sspi.h \
	ne_string.h \
	ne_uri.h \
	ne_utils.h \
	ne_xml.h \
	ne_xmlreq.h

noinst_LTLIBRARIES = libneon.la
libneon_la_SOURCES = \
	ne_207.c \
	ne_alloc.c \
	ne_auth.c \
	ne_basic.c \
	ne_compress.c \
	ne_dates.c \
	ne_i18n.c \
	ne_locks.c \
	ne_md5.c \
	ne_ntlm.c \
	ne_pkcs11.c \
	ne_props.c \
	ne_redirect.c	 \
	ne_request.c \
	ne_session.c \
	ne_socket.c \
	ne_socks.c \
	ne_string.c \
	ne_uri.c \
	ne_utils.c \
	ne_xml.c \
	ne_xmlreq.c
libneon_la_DEPENDENCIES = @NEON_EXTRAOBJS@
libneon_la_LIBADD = @NEON_EXTRAOBJS@ @NEON_LIBS@

libneon_la_LDFLAGS = # -release $(LT_CURRENT).$(LT_REVISION)
#if HAVE_LD_VERSION_SCRIPT
#libneon_la_LDFLAGS += # -Wl,--version-script=$(srcdir)/librpmdb.vers
#endif

libneon_la_LDFLAGS += $(LDFLAGS)

.PHONY:	sources
sources:
	@echo $(splint_SRCS:%=neon/src/%)

.PHONY:	lint
lint:
	$(LINT) $(DEFS) $(INCLUDES) $(splint_SRCS) $(DBLIBSRCS)

.PHONY:	mccabe
mccabe:
	$(MCCABE) $(libneon_la_SOURCES) | sort -n -r | head -n 10

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
