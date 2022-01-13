SUBDIRS = gtest

common_cflags = \
	$(LIBGUPNP_CFLAGS) \
	-I$(top_srcdir) \
	$(WARN_CFLAGS) \
	-Wno-unused-parameter
common_ldadd = \
	$(top_builddir)/libgupnp-av/libgupnp-av-1.0.la \
	$(LIBGUPNP_LIBS)

noinst_PROGRAMS = test-search-criteria-parser

test_search_criteria_parser_SOURCES = test-search-criteria-parser.c
test_search_criteria_parser_LDADD = $(common_ldadd)
test_search_criteria_parser_CFLAGS = $(common_cflags)

check_PROGRAMS = \
	check-search \
	check-feature-list-parser \
	fragments

check_search_SOURCES = check-search.c
check_search_LDADD = $(common_ldadd)
check_search_CFLAGS = $(common_cflags)
check_search_LDFLAGS = $(WARN_LDFLAGS)

check_feature_list_parser_SOURCES = check-feature-list-parser.c
check_feature_list_parser_LDADD = $(common_ldadd)
check_feature_list_parser_CFLAGS = $(common_cflags)
check_feature_list_parser_LDFLAGS = $(WARN_LDFLAGS)

fragments_SOURCES = fragments.c
fragments_LDADD = $(common_ldadd)
fragments_CFLAGS = $(common_cflags) -DABS_TOP_SRCDIR="\"$(abs_top_srcdir)\""
fragments_LDFLAGS = $(WARN_LDFLAGS)

TESTS = $(check_PROGRAMS)
