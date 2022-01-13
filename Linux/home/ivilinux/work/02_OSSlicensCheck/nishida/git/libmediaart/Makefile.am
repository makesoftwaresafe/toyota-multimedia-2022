AM_CPPFLAGS = \
	$(BUILD_CFLAGS) \
	$(LIBMEDIAART_CFLAGS)

#
# Binaries
#
lib_LTLIBRARIES = libmediaart-@LIBMEDIAART_API_VERSION@.la

libmediaartdir = $(libdir)/libmediaart-$(LIBMEDIAART_API_VERSION)/
libmediaartincludedir=$(includedir)/libmediaart-$(LIBMEDIAART_API_VERSION)/libmediaart/

libmediaart_sources = \
	cache.c \
	cache.h \
	extract.c \
	extract.h \
	extractgeneric.h \
	storage.c \
	storage.h

libmediaartinclude_HEADERS = \
	mediaart.h \
	cache.h \
	extract.h \
	extractgeneric.h

libmediaart_@LIBMEDIAART_API_VERSION@_la_SOURCES = \
	$(libmediaart_sources) \
	marshal.c \
	marshal.h

if HAVE_GDKPIXBUF
libmediaart_@LIBMEDIAART_API_VERSION@_la_SOURCES += extractpixbuf.c
libmediaart_@LIBMEDIAART_API_VERSION@_la_LINK = $(LINK) $(libmediaart_@LIBMEDIAART_API_VERSION@_la_LDFLAGS)
else
if HAVE_QT
libmediaart_@LIBMEDIAART_API_VERSION@_la_SOURCES += extractqt.cpp
libmediaart_@LIBMEDIAART_API_VERSION@_la_LINK = $(CXXLINK) $(libmediaart_@LIBMEDIAART_API_VERSION@_la_LDFLAGS)
else
libmediaart_@LIBMEDIAART_API_VERSION@_la_SOURCES += extractdummy.c
libmediaart_@LIBMEDIAART_API_VERSION@_la_LINK = $(LINK) $(libmediaart_@LIBMEDIAART_API_VERSION@_la_LDFLAGS)
endif
endif

libmediaart_@LIBMEDIAART_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-symbols-regex '^media_art_.*'

libmediaart_@LIBMEDIAART_API_VERSION@_la_LIBADD = \
	$(BUILD_LIBS) \
	$(LIBMEDIAART_LIBS)


marshal.h: marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --prefix=media_art_marshal --header > $@

marshal.c: marshal.list
	$(AM_V_GEN)(echo "#include \"marshal.h\""; \
	    $(GLIB_GENMARSHAL) $< --prefix=media_art_marshal --body) > $@


BUILT_SOURCES = \
	marshal.c \
	marshal.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	marshal.list


-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --symbol-prefix=media_art
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_sources = $(libmediaart_sources) extractdummy.c

MediaArt-@LIBMEDIAART_API_VERSION@.gir: libmediaart-@LIBMEDIAART_API_VERSION@.la
MediaArt_@LIBMEDIAART_API_VERSION_U@_gir_INCLUDES = GObject-2.0 Gio-2.0
MediaArt_@LIBMEDIAART_API_VERSION_U@_gir_CFLAGS = $(INCLUDES) -DLIBMEDIAART_COMPILATION
MediaArt_@LIBMEDIAART_API_VERSION_U@_gir_LIBS = libmediaart-@LIBMEDIAART_API_VERSION@.la
MediaArt_@LIBMEDIAART_API_VERSION_U@_gir_SCANNERFLAGS = --c-include=libmediaart/mediaart.h
MediaArt_@LIBMEDIAART_API_VERSION_U@_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += MediaArt-@LIBMEDIAART_API_VERSION@.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

BUILT_SOURCES += $(gir_DATA) $(typelib_DATA)
endif

if ENABLE_VAPIGEN
-include $(VAPIGEN_MAKEFILE)

libmediaart-@LIBMEDIAART_API_VERSION@.vapi: MediaArt-@LIBMEDIAART_API_VERSION@.gir

VAPIGEN_VAPIS = libmediaart-@LIBMEDIAART_API_VERSION@.vapi

libmediaart_@LIBMEDIAART_API_VERSION_U@_vapi_METADATADIRS = $(srcdir)
libmediaart_@LIBMEDIAART_API_VERSION_U@_vapi_DEPS = gio-2.0
libmediaart_@LIBMEDIAART_API_VERSION_U@_vapi_FILES = MediaArt-@LIBMEDIAART_API_VERSION@.gir

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS)

BUILT_SOURCES += $(vapi_DATA)
endif
