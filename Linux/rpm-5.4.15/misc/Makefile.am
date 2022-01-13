# Makefile for misc library.

AUTOMAKE_OPTIONS = 1.4 foreign

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir)

EXTRA_DIST = \
	librpmmisc.vers alloca.c \
	asprintf.c	basename.c	err.c		error.c		\
	fakefork.c	getcwd.c	getdelim.c	getline.c	\
	getmntent.c	getwd.c				memcmp.c	\
	mktime.c	myrealloc.c	putenv.c	realpath.c	\
	setenv.c	setproctitle.c	stpcpy.c	stpncpy.c	\
			strcspn.c	strdup.c	strerror.c	\
	strftime.c	strcspn.c	strstr.c	strtol.c	\
	strtoul.c \
        tempname.h \
        mkdtemp.c 

noinst_HEADERS = \
	err.h		error.h tempname.h

usrlibdir = $(libdir)
usrlib_LTLIBRARIES = librpmmisc.la

librpmmisc_la_SOURCES = librpmmisc.c
librpmmisc_la_LDFLAGS = -release $(LT_CURRENT).$(LT_REVISION)
if HAVE_LD_VERSION_SCRIPT
librpmmisc_la_LDFLAGS += -Wl,@LD_VERSION_SCRIPT_FLAG@,@top_srcdir@/misc/librpmmisc.vers
endif
librpmmisc_la_LDFLAGS += \
	@WITH_DB_LDFLAGS@ \
	@WITH_FILE_LDFLAGS@ \
	@WITH_BEECRYPT_LDFLAGS@ \
	@WITH_FICL_LDFLAGS@ \
	@WITH_GPSEE_LDFLAGS@ \
	@WITH_LIBTPM_LDFLAGS@ \
	@WITH_LIBGIT2_LDFLAGS@ \
	@WITH_LUA_LDFLAGS@ \
	@WITH_NEON_LDFLAGS@ \
	@WITH_PCRE_LDFLAGS@ \
	@WITH_POPT_LDFLAGS@ \
	@WITH_RC_LDFLAGS@ \
	@WITH_SYCK_LDFLAGS@ \
	@WITH_XAR_LDFLAGS@ \
	@WITH_XZ_LDFLAGS@ \
	@WITH_ZLIB_LDFLAGS@
librpmmisc_la_LIBADD = \
	@ALLOCA@ \
	@WITH_DB_LIBS@ \
	@WITH_FICL_LIBS@ \
	@WITH_FILE_LIBS@ \
	@WITH_BEECRYPT_LIBS@ \
	@WITH_GPSEE_LIBS@ \
	@WITH_LIBTPM_LIBS@ \
	@WITH_LIBGIT2_LIBS@ \
	@WITH_LUA_LIBS@ \
	@WITH_NEON_LIBS@ \
	@WITH_PCRE_LIBS@ \
	@WITH_POPT_LIBS@ \
	@WITH_RC_LIBS@ \
	@WITH_SYCK_LIBS@ \
	@WITH_XAR_LIBS@ \
	@WITH_XZ_LIBS@ \
	@WITH_ZLIB_LIBS@
if ENABLE_BUILD_EXTLIBDEP
librpmmisc_la_LDFLAGS += $(LDFLAGS)
librpmmisc_la_LIBADD  += $(LIBS)
endif
librpmmisc_la_DEPENDENCIES = \
	@ALLOCA@
librpmmisc.la: $(librpmmisc_la_OBJECTS) $(librpmmisc_la_DEPENDENCIES) 
	$(librpmmisc_la_LINK) -rpath $(usrlibdir) $(librpmmisc_la_OBJECTS) $(librpmmisc_la_LIBADD)

install-data-hook:
if !ENABLE_BUILD_LAFILES
	-for l in $(usrlib_LTLIBRARIES); do \
	    rm -f $(DESTDIR)$(usrlibdir)/$${l}; \
	done
endif
if WITH_PATH_VERSIONED
	-for l in $(usrlib_LTLIBRARIES); do \
	    base=`echo "$${l}" | sed -e 's;\.la$$;;'`; \
	    if [ -f $(DESTDIR)$(usrlibdir)/$${base}.a ]; then \
	        mv $(DESTDIR)$(usrlibdir)/$${base}.a \
	           $(DESTDIR)$(usrlibdir)/$${base}-$(VERSION).a; \
	    fi; \
	done
endif

