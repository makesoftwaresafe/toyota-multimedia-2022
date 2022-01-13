# $Id: Makefile.am,v 1.13.2.6 2014/03/27 19:12:17 jbj Exp $

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = librpmconstant.vers rpmh2tbl

LINT = splint

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/build \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	-I$(top_srcdir)/misc \
	@WITH_DB_CPPFLAGS@ \
	@WITH_PCRE_CPPFLAGS@ \
	@WITH_POPT_CPPFLAGS@

bin_PROGRAMS = rpmconstant
usrlibdir = $(libdir)
usrlib_LTLIBRARIES = librpmconstant.la

pkgincdir = $(pkgincludedir)$(WITH_PATH_VERSIONED_SUFFIX)
pkginc_HEADERS = rpmconstant.h

librpmconstant_la_SOURCES = \
	rpmconstant.c \
	rpmconstant.h \
	rpmconstanttbl.c \
	rpmconstant_internal.c
librpmconstant_la_LDFLAGS = -release $(LT_CURRENT).$(LT_REVISION)
if HAVE_LD_VERSION_SCRIPT
librpmconstant_la_LDFLAGS += -Wl,@LD_VERSION_SCRIPT_FLAG@,@top_srcdir@/rpmconstant/librpmconstant.vers
endif
librpmconstant_la_LIBADD = \
	$(top_builddir)/lib/librpm.la \
	$(top_builddir)/rpmio/librpmio.la \
	$(top_builddir)/misc/librpmmisc.la \
	@LTLIBINTL@
librpmconstant.la: $(librpmconstant_la_OBJECTS) $(librpmconstant_la_DEPENDENCIES) 
	$(librpmconstant_la_LINK) -rpath $(usrlibdir) $(librpmconstant_la_OBJECTS) $(librpmconstant_la_LIBADD)

rpmconstant_SOURCES = constant.c
rpmconstant_LDADD   = librpmconstant.la

BUILT_SOURCES = rpmconstanttbl.c
CLEANFILES = rpmconstanttbl.c

myHEADERS = \
	$(top_srcdir)/lib/*.h \
	$(top_srcdir)/build/*.h \
	$(top_srcdir)/rpmdb/*.h \
	$(top_srcdir)/rpmio/*.h \
	$(top_srcdir)/misc/*.h

rpmconstanttbl.c: $(srcdir)/rpmh2tbl
	$(PERL) $(srcdir)/rpmh2tbl $(myHEADERS) >$@

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

