SUBDIRS=src

if WANT_TESTS
SUBDIRS+=test
endif


sysconfdir = "/etc/"

# Add config file to distribution 
EXTRA_DIST = $(sysconf_DATA) 

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=persistence_health_monitor.pc

MAINTAINERCLEANFILES = \
      Makefile.in aclocal.m4 compile config.guess config.h.in config.sub \
      configure depcomp install-sh ltmain.sh persistence_health_monitory.pc config.h.in~ Makefile missing
