#################################################################################
# Makefile for libtpm/lib							#
# $Id: Makefile.am 4514 2011-03-22 18:53:32Z stefanb $				#
################################################################################

EXTRA_DIST = \
	makefile.mak

AM_CPPFLAGS =	-I$(top_srcdir)/

AM_CFLAGS = -Wall -ggdb -Wuninitialized -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -W -Werror
lib_LTLIBRARIES = libtpm.la

AM_LDFLAGS = @LIBTPMS_LIBRARY_PATH@ @LIBTPMS_LIBRARY@ 

include_HEADERS = \
	hmac.h \
	newserialize.h \
	oiaposap.h \
	pcrs.h \
	tpm.h \
	tpmfunc.h \
	tpmkeys.h \
	tpmutil.h \
	tpm_constants.h \
	tpm_error.h \
	tpm_lowlevel.h \
	tpm_structures.h \
	tpm_types.h

libtpm_la_SOURCES = \
	auditing.c \
	bind.c \
	chgauth.c \
	context.c \
	counter.c \
	daa.c \
	debug.c \
	delegation.c \
	dir.c \
	eviction.c \
	hmac.c \
	identity.c \
	keys.c \
	keyswap.c \
	maintenance.c \
	management.c \
	migrate.c \
	miscfunc.c \
	nv.c \
	oiaposap.c \
	optin.c \
	owner.c \
	ownertpmdiag.c \
	pcrs.c \
	raw.c \
	rng.c \
	seal.c \
	serialize.c \
	session.c \
	sha.c \
	signature.c \
	startup.c \
	testing.c \
	ticks.c \
	tpmutil.c \
	tpmutil_libtpms.c \
	tpmutil_sock.c \
	tpmutil_tty.c \
	tpmutil_unixio.c \
	transport.c
