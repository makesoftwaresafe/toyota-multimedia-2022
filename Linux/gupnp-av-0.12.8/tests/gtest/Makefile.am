TESTS_ENVIRONMENT = G_SLICE=debug-blocks \
		    LD_LIBRARY_PATH=$(top_builddir)/libgupnp-av/.libs:$(LD_LIBRARY_PATH) \
		    GUPNP_AV_DATADIR=$(top_srcdir)/data

TESTS=$(check_PROGRAMS)

check_PROGRAMS = \
	test-regression \
	test-didl-lite-object \
	test-media-collection \
	test-last-change-parser \
	test-cds-last-change-parser

test_regression_SOURCES = test-regression.c
test_regression_LDFLAGS = $(WARN_LDFLAGS)

test_didl_lite_object_SOURCES = test-didl-lite-object.c
test_didl_lite_object_LDFLAGS = $(WARN_LDFLAGS)

test_media_collection_SOURCES =  \
	test-media-collection.c
test_media_collection_LDFLAGS = $(WARN_LDFLAGS)

test_last_change_parser_SOURCES = \
	test-last-change-parser.c
test_last_change_parser_LDFLAGS = $(WARN_LDFLAGS)

test_cds_last_change_parser_SOURCES = \
	test-cds-last-change-parser.c
test_cds_last_change_parser_LDFLAGS = $(WARN_LDFLAGS)

LDADD = \
	$(top_builddir)/libgupnp-av/libgupnp-av-1.0.la \
	$(LIBGUPNP_LIBS)

AM_CFLAGS = \
	    $(LIBGUPNP_CFLAGS) \
	    -I $(top_srcdir) \
	    -DDATA_PATH="\"$(srcdir)\"" \
	    $(WARN_CFLAGS)

EXTRA_DIST = data/cds-last-change/00-cds-last-change.xml \
	data/cds-last-change/01-cds-last-change.xml \
	data/cds-last-change/02-cds-last-change.xml \
	data/cds-last-change/03-cds-last-change.xml \
	data/cds-last-change/04-cds-last-change.xml \
	data/cds-last-change/05-cds-last-change.xml \
	data/cds-last-change/06-cds-last-change.xml \
	data/cds-last-change/07-cds-last-change.xml
