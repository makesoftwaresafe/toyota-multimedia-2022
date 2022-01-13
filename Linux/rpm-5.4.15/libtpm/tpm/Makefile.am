##D CC = /usr/bin/gcc
##D CCFLAGS = -Wall -W -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -c -ggdb \
##D 	-DTPM_NV_DISK -DTPM_V12 -O0 -DTPM_DEBUG -DTPM_AES -DTPM_POSIX -DTPM_PCCLIENT \
##D 	-DTPM_ENABLE_ACTIVATE

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = \
	ibmtpm \
	makefile-common \
	makefile-en-ac \
	makefile-freebl \
	makefile-freebl-ts \
	makefile.mak \
	makefile-tpm \
	makefile-ts \
	makefile-ts.mak \
	PORTING \
	STATUS

AM_CFLAGS = \
	-Wall -W -Wmissing-declarations -Wmissing-prototypes -Wnested-externs \
	-ggdb \
	-DTPM_AES \
	-DTPM_DEBUG \
	-DTPM_ENABLE_ACTIVATE \
	-DTPM_NV_DISK \
	-DTPM_POSIX \
	-DTPM_PCCLIENT \
	-DTPM_V12

# chose between the OpenSSL and FreeBL crypto libraries
CRYPTO_LINKLIBS =
if WITH_OPENSSL
CRYPTO_SOURCE = tpm_crypto.c
else
if WITH_NSS
CRYPTO_SOURCE = tpm_crypto_freebl.c
## CRYPTO_LINKLIBS += -lfreebl -lgmp -lnspr4 -lnssutil3 -lnss3
else
## CRYPTO_SOURCE = tpm_crypto_unknown.c
endif
endif

EXTRA_DIST +=	tpm_crypto.c tpm_crypto_freebl.c

dist_noinst_HEADERS = \
	tpm_admin.h \
	tpm_audit.h \
	tpm_auth.h \
	tpm_commands.h \
	tpm_constants.h \
	tpm_counter.h \
	tpm_crypto.h \
	tpm_cryptoh.h \
	tpm_daa.h \
	tpm_debug.h \
	tpm_delegate.h \
	tpm_digest.h \
	tpm_error.h \
	tpm_global.h \
	tpm_identity.h \
	tpm_init.h \
	tpm_io.h \
	tpm_key.h \
	tpm_load.h \
	tpm_maint.h \
	tpm_memory.h \
	tpm_migration.h \
	tpm_nonce.h \
	tpm_nvfile.h \
	tpm_nvfilename.h \
	tpm_nvram_const.h \
	tpm_nvram.h \
	tpm_owner.h \
	tpm_pcr.h \
	tpm_permanent.h \
	tpm_platform.h \
	tpm_process.h \
	tpm_secret.h \
	tpm_session.h \
	tpm_sizedbuffer.h \
	tpm_startup.h \
	tpm_storage.h \
	tpm_store.h \
	tpm_structures.h \
	tpm_svnrevision.h \
	tpm_ticks.h \
	tpm_time.h \
	tpm_transport.h \
	tpm_types.h \
	tpm_ver.h

noinst_LTLIBRARIES =	libtpmd.la
libtpmd_la_SOURCES = \
	tpm_admin.c \
	tpm_audit.c \
	tpm_auth.c \
	tpm_counter.c \
	tpm_cryptoh.c \
	tpm_daa.c \
	tpm_debug.c \
	tpm_delegate.c \
	tpm_digest.c \
	tpm_error.c \
	tpm_global.c \
	tpm_identity.c \
	tpm_init.c \
	tpm_io.c \
	tpm_key.c \
	tpm_load.c \
	tpm_maint.c \
	tpm_memory.c \
	tpm_migration.c \
	tpm_nonce.c \
	tpm_nvfile.c \
	tpm_nvram.c \
	tpm_owner.c \
	tpm_pcr.c \
	tpm_permanent.c \
	tpm_platform.c \
	tpm_process.c \
	tpm_secret.c \
	tpm_session.c \
	tpm_sizedbuffer.c \
	tpm_startup.c \
	tpm_storage.c \
	tpm_store.c \
	tpm_svnrevision.c \
	tpm_ticks.c \
	tpm_time.c \
	tpm_transport.c \
	tpm_ver.c \
	$(CRYPTO_SOURCE) \
	$(INSTANCE_CFILES)

#etc_init_ddir =	/etc/init.d
#etc_init_d_SCRIPTS = ibmtpm

sbin_PROGRAMS = ibmtpmd
ibmtpmd_SOURCES =	tpm_server.c
ibmtpmd_LDADD =		libtpmd.la $(CRYPTO_LINKLIBS)
ibmtpmd_LDFLAGS =	--all-static $(LDFLAGS)
	
#tpm_server:	$(OBJFILES)
#		$(CC) $(OBJFILES) $(LNFLAGS) -o tpm_server
