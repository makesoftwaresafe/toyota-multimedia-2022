# Makefile for neon test suite.

EXTRA_DIST = \
	.gdbinit \
	COPYING \
	ChangeLog \
	README \
	STATUS \
	expired.pem \
	makekeys.sh \
	notvalid.pem \
	nulca.pem \
	nulcn.pem \
	nulsan.pem \
	nulsrv.key \
	openssl.conf \
	run-tests.sh \
	run.sh \
	server.key \
	common/run.sh

SHELL = @SHELL@
CPPFLAGS = @CPPFLAGS@ -I. -I$(top_srcdir)/src -I$(top_srcdir)/test/common -I${top_srcdir}/test

VPATH = @srcdir@

OPENSSL = @OPENSSL@

HELPERS = @HELPERS@ foobar.txt

BASIC_TESTS = uri-tests util-tests string-tests socket \
	session request auth basic stubs redirect
ZLIB_TESTS = compress
ZLIB_HELPERS = file1.gz file2.gz trailing.gz badcsum.gz truncated.gz \
	corrupt1.gz corrupt2.gz empty.gz random.txt
DAV_TESTS = xml xmlreq oldacl acl3744 props lock
SSL_TESTS = socket-ssl ssl

SSL_HELPERS = ca-stamp

NEONTESTS = @TESTS@
TESTS_ENVIRONMENT = env \
	SRCDIR=$(top_srcdir)/test HARNESS="" \
	TESTS="$(NEONTESTS)"
#	TESTS="$(BASIC_TESTS) $(ZLIB_TESTS) $(DAV_TESTS) socket-ssl"
TESTS = run.sh

VALGRIND = valgrind --tool=memcheck --leak-check=yes --trace-children=no --show-reachable=yes

LIBNEON = $(top_builddir)/src/libneon.la

noinst_HEADERS = \
	common/tests.h \
	common/child.h \
	utils.h

noinst_LTLIBRARIES = libtest.la
libtest_la_LIBADD = $(LIBNEON)
libtest_la_SOURCES = \
	common/tests.c \
	common/child.c \
	utils.c \
	util-socks.c

#Z LIBTOOL = @LIBTOOL@ --silent
#Z TEST_LDFLAGS = @TEST_LDFLAGS@

INSTALL_HELPERS = $(HELPERS) server.key makekeys \
	$(top_srcdir)/test/openssl.conf $(top_srcdir)/test/notvalid.pem \
	$(top_srcdir)/test/expired.pem

# By default, compile but don't run the tests.
all: $(NEONTESTS)

clean:
	rm -f $(NEONTESTS) $(HELPERS) *.*o common/*.*o libtest.*a *.log
	rm -f *.gc* *.da *.bb* common/*.bb* common/*.gc* common/*.da
	rm -rf ca ca2 .libs nssdb*
	rm -f ca-stamp client.key *.csr ssigned.pem wrongcn.pem \
	   server.cert client.cert *.p12 *.cert sparse.bin

distclean: clean
	rm -f calist.pem
	rm -f ca3.pem
	rm -f ca3/02.pem
	rm -f ca3/cert.csr
	rm -f ca3/serial.old
	rm -f ca3/cert.pem
	rm -f ca3/01.pem
	rm -f ca3/index.txt
	rm -f ca3/index.txt.attr
	rm -f ca3/key.pem
	rm -f ca3/index.txt.attr.old
	rm -f ca3/serial
	rm -f ca3/index.txt.old
	rm -f common/.dirstamp
	rm -rf common/.libs
	rm -rf common/.deps
	rm -f client.dsap
	rm -f ca2.pem
	rm -f clientdsa.key
	rm -f Makefile
	rm -f chain.pem
	rm -f output.pem
	rm -f run.sh.trs
	rm -f makekeys
	rm -rf .deps
	rm -f ca1/02.pem
	rm -f ca1/cert.csr
	rm -f ca1/serial.old
	rm -f ca1/cert.pem
	rm -f ca1/01.pem
	rm -f ca1/index.txt
	rm -f ca1/index.txt.attr
	rm -f ca1/key.pem
	rm -f ca1/index.txt.attr.old
	rm -f ca1/serial
	rm -f ca1/index.txt.old
	rm -f ca2server.pem
	rm -f ca4.pem
	rm -f ca1.pem
	rm -f makekeys.out

## check: $(NEONTESTS) $(HELPERS)
## 	@SRCDIR=$(top_srcdir)/test $(SHELL) $(top_srcdir)/test/run.sh $(NEONTESTS)

grind: $(NEONTESTS) $(HELPERS)
	@SRCDIR=$(top_srcdir) HARNESS="$(VALGRIND)" $(SHELL) $(top_srcdir)/test/run.sh $(NEONTESTS)

lfs-check: largefile $(LFS_HELPERS)
	@SRCDIR=$(top_srcdir)/test $(SHELL) $(top_srcdir)/test/run.sh largefile

#B install: $(NEONTESTS) $(HELPERS)
#B 	@$(INSTALL) -d $(DESTDIR)$(libdir)/neon-tests
#B 	@$(INSTALL) -d $(DESTDIR)$(libdir)/neon-tests/bin
#B 	@$(INSTALL) -d $(DESTDIR)$(libdir)/neon-tests/data
#B 	@for t in $(NEONTESTS); do \
#B 	 $(LIBTOOL) --mode=install \
#B 	      $(INSTALL_PROGRAM) $$t $(DESTDIR)$(libdir)/neon-tests/bin; \
#B 	done
#B 	@for h in $(INSTALL_HELPERS); do \
#B 	 $(INSTALL_DATA) $$h $(DESTDIR)$(libdir)/neon-tests/data; \
#B 	done
#B 	@$(INSTALL_PROGRAM) $(top_srcdir)/run-tests.sh $(DESTDIR)$(libdir)/neon-tests

NEWS = $(top_srcdir)/NEWS

file1.gz: $(NEWS)
	gzip -c --no-name $(NEWS) > $@

file2.gz: $(NEWS)
	gzip -c --name $(NEWS) > $@

# gzip file with trailing bytes.
trailing.gz: $(NEWS)
	gzip -c --no-name $(NEWS) > $@
	echo "hello, world" >> $@

truncated.gz: file1.gz
	dd if=file1.gz of=$@ bs=2048 count=2

badcsum.gz: file1.gz
	dd of=$@ if=file1.gz bs=1 count=`perl -e 'printf "%d", (stat("file1.gz"))[7] - 8;'`
	echo 'broken!' >> $@

corrupt1.gz: file1.gz
	dd of=$@ if=file1.gz bs=1 count=500
	cat $(NEWS) >> $@

corrupt2.gz: $(NEWS)
	cat $(NEWS) > $@

empty.gz:
	touch $@

random.txt: $(NEWS)
	cat $(NEWS) > $@

foobar.txt:
	echo foobar > $@

# Dummy target to create the CA keys etc.  makekeys stderr is redirected
# since it changes for every invocation; not helpful for regression
# testing.
ca-stamp: makekeys $(top_srcdir)/test/openssl.conf
	rm -rf ca ca2
	OPENSSL=$(OPENSSL) \
	 $(SHELL) makekeys $(top_srcdir)/test 2>makekeys.out
	@echo timestamp > ca-stamp

##J Makefile: $(top_srcdir)/test/Makefile.in
##J 	cd $(top_srcdir) && ./config.status $(top_builddir)/test/Makefile

##J makekeys: $(top_srcdir)/test/makekeys.sh
##J 	cd $(top_srcdir) && ./config.status $(top_builddir)/test/makekeys

check_PROGRAMS = \
	$(NEONTESTS)

EXTRA_PROGRAMS = \
	socket-ssl \
	resolve \
	auth \
	basic \
	uri-tests \
	util-tests \
	string-tests \
	socket \
	request \
	compress \
	oldacl \
	acl3744 \
	stubs \
	props \
	session \
	redirect \
	ssl \
	xml \
	xmlreq \
	lock \
	largefile

check_SCRIPTS = run.sh

run.sh: $(HELPERS)

LDADD =	libtest.la

# Recompile socket.c with SOCKET_SSL defined
socket_ssl_SOURCES =	socket.c
socket_ssl_LDADD =	$(LIBNEON) $(LDADD)

resolve_LDADD =		$(LIBNEON)

auth_LDADD =		$(LDADD)
basic_LDADD =		$(LDADD)
uri_tests_LDADD =	$(LDADD)
util_tests_LDADD =	$(LDADD)
string_tests_LDADD =	$(LDADD)
socket_LDADD =		$(LDADD)
request_LDADD =		$(LDADD)
compress_LDADD =	$(LDADD)
oldacl_LDADD =		$(LDADD)
acl3744_LDADD =		$(LDADD)
stubs_LDADD =		$(LDADD)
props_LDADD =		$(LDADD)
session_LDADD =		$(LDADD)
redirect_LDADD =	$(LDADD)
ssl_LDADD =		$(LIBNEON) $(LDADD)
xml_LDADD =		$(LDADD)
xmlreq_LDADD =		$(LDADD)
lock_LDADD =		$(LDADD)
largefile_LDADD =	$(LDADD)

#YY largefile_SOURCES =	largefile.c
#YY largefile_LDADD =	$(LDADD)
