AUTOMAKE_OPTIONS = foreign

if DEBUG
AM_CFLAGS = $(DEPS_CFLAGS) $(CHECK_CFLAGS) -g
#AM_CFLAGS = -fprofile-arcs -ftest-coverage  $(DEPS_CFLAGS) $(CHECK_CFLAGS) -g
else
AM_CFLAGS = $(DEPS_CFLAGS) $(CHECK_CFLAGS)
#AM_CFLAGS = -fprofile-arcs -ftest-coverage $(DEPS_CFLAGS) $(CHECK_CFLAGS)
endif

noinst_PROGRAMS = persistence_health_monitor_test

persistence_health_monitor_test_SOURCES = persistence_health_monitor_test.c
persistence_health_monitor_test_LDADD = $(DEPS_LIBS) $(CHECK_LIBS)

TESTS=persistence_health_monitor_test

