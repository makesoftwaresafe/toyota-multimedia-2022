# Version format current:revision:age
# If the library source code has changed at all since the last update, then
# increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
# If any interfaces have been added, removed, or changed since the last update,
# increment current, and set revision to 0.
# If any interfaces have been added since the last public release, then
# increment age.
# If any interfaces have been removed since the last public release, then set
# age to 0.
LTVERSION = 2:0:0

AM_CFLAGS = $(LIBGUPNP_CFLAGS) -I$(top_srcdir) -DDATADIR="\"$(pkgdatadir)\"" $(WARN_CFLAGS)

libgupnp_av_incdir = $(includedir)/gupnp-av-1.0/libgupnp-av

lib_LTLIBRARIES = libgupnp-av-1.0.la

libgupnp_av_inc_HEADERS = gupnp-didl-lite-object.h \
			  gupnp-didl-lite-item.h \
			  gupnp-didl-lite-container.h \
			  gupnp-didl-lite-createclass.h \
			  gupnp-didl-lite-parser.h \
			  gupnp-didl-lite-resource.h \
			  gupnp-didl-lite-descriptor.h \
			  gupnp-didl-lite-contributor.h \
			  gupnp-didl-lite-writer.h \
			  gupnp-protocol-info.h \
		          gupnp-search-criteria-parser.h \
			  gupnp-last-change-parser.h \
			  gupnp-cds-last-change-parser.h \
			  gupnp-media-collection.h \
			  gupnp-feature.h \
			  gupnp-feature-list-parser.h \
			  gupnp-dlna.h \
			  gupnp-av-error.h \
			  gupnp-av.h \
			  gupnp-av-enums.h


gupnp-av-marshal.c: gupnp-av-marshal.list
	$(AM_V_GEN) \
	$(GLIB_GENMARSHAL) --prefix=gupnp_av_marshal $(srcdir)/gupnp-av-marshal.list --header --body > gupnp-av-marshal.c

gupnp-av-marshal.h: gupnp-av-marshal.list
	$(AM_V_GEN) \
	$(GLIB_GENMARSHAL) --prefix=gupnp_av_marshal $(srcdir)/gupnp-av-marshal.list --header > gupnp-av-marshal.h

BUILT_SOURCES = gupnp-av-marshal.c gupnp-av-marshal.h

libgupnp_av_1_0_la_LDFLAGS = -version-info $(LTVERSION) -no-undefined $(WARN_LDFLAGS)

libgupnp_av_1_0_la_SOURCES = gupnp-didl-lite-object.c \
			     gupnp-didl-lite-object-private.h \
			     gupnp-didl-lite-item.c \
			     gupnp-didl-lite-container.c \
			     gupnp-didl-lite-createclass.c \
			     gupnp-didl-lite-createclass-private.h \
			     gupnp-didl-lite-parser.c \
			     gupnp-didl-lite-parser-private.h \
			     gupnp-didl-lite-resource.c \
			     gupnp-didl-lite-resource-private.h \
			     gupnp-didl-lite-descriptor.c \
			     gupnp-didl-lite-descriptor-private.h \
			     gupnp-didl-lite-contributor.c \
			     gupnp-didl-lite-contributor-private.h \
			     gupnp-didl-lite-writer.c \
			     gupnp-didl-lite-writer-private.h \
			     gupnp-protocol-info.c \
			     gupnp-search-criteria-parser.c \
			     gupnp-last-change-parser.c \
			     gupnp-cds-last-change-parser.c \
			     gupnp-media-collection.c \
			     gupnp-feature.c \
			     gupnp-feature-list-parser.c \
			     gupnp-dlna.c \
			     gupnp-av-error.c \
			     xml-util.c \
			     xml-util.h \
			     gvalue-util.c \
			     gvalue-util.h \
			     fragment-util.c \
			     fragment-util.h \
			     time-utils.c \
			     time-utils.h \
			     xsd-data.c \
			     xsd-data.h \
			     $(BUILT_SOURCES)

libgupnp_av_1_0_la_LIBADD = $(LIBGUPNP_LIBS)

EXTRA_DIST = gupnp-av-marshal.list

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
export GUPNP_AV_DATADIR := $(top_srcdir)/data

introspection_sources = \
	    $(addprefix $(top_srcdir)/libgupnp-av/, \
			$(libgupnp_av_inc_HEADERS)) \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-object.c \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-item.c \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-container.c \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-parser.c \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-resource.c \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-descriptor.c \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-contributor.c \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-writer.c \
	    $(top_srcdir)/libgupnp-av/gupnp-didl-lite-createclass.c \
	    $(top_srcdir)/libgupnp-av/gupnp-protocol-info.c \
	    $(top_srcdir)/libgupnp-av/gupnp-search-criteria-parser.c \
	    $(top_srcdir)/libgupnp-av/gupnp-last-change-parser.c \
	    $(top_srcdir)/libgupnp-av/gupnp-cds-last-change-parser.c \
	    $(top_srcdir)/libgupnp-av/gupnp-media-collection.c \
	    $(top_srcdir)/libgupnp-av/gupnp-feature.c \
	    $(top_srcdir)/libgupnp-av/gupnp-feature-list-parser.c \
	    $(top_srcdir)/libgupnp-av/gupnp-dlna.c \
	    $(top_srcdir)/libgupnp-av/gupnp-av-error.c

GUPnPAV-1.0.gir: libgupnp-av-1.0.la
GUPnPAV_1_0_gir_INCLUDES = GObject-2.0 GSSDP-1.0 Soup-2.4 libxml2-2.0 GUPnP-1.0
GUPnPAV_1_0_gir_CFLAGS = $(AM_CFLAGS)
GUPnPAV_1_0_gir_LIBS = libgupnp-av-1.0.la
GUPnPAV_1_0_gir_NAMESPACE = GUPnPAV
GUPnPAV_1_0_gir_SCANNERFLAGS = --identifier-prefix=GUPnP --symbol-prefix=gupnp --pkg-export=gupnp-av-1.0 $(WARN_SCANNERFLAGS)
GUPnPAV_1_0_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += GUPnPAV-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)
endif

CLEANFILES = $(BUILT_SOURCES) $(gir_DATA) $(typelib_DATA)
DISTCLEANFILES = $(BUILT_SOURCES)
MAINTAINERCLEANFILES = Makefile.in $(BUILT_SOURCES)

