AUTOMAKE_OPTIONS = foreign

if DEBUG
AM_CFLAGS =$(DEPS_CFLAGS) $(CHECK_CFLAGS) -g
else
AM_CFLAGS = $(DEPS_CFLAGS) $(CHECK_CFLAGS)
#AM_CFLAGS = -fprofile-arcs -ftest-coverage $(DEPS_CFLAGS) $(CHECK_CFLAGS)
endif

noinst_PROGRAMS = persistence_file_cache_test


   
persistence_file_cache_test_SOURCES = persistence_file_cache_test.c
persistence_file_cache_test_LDADD = $(DEPS_LIBS) $(CHECK_LIBS) \
   $(top_srcdir)/src/libpersistence_file_cache.la
 
  
TESTS=persistence_file_cache_test
