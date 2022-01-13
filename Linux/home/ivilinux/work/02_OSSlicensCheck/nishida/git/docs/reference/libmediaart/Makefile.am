AUTOMAKE_OPTIONS = 1.6

# The name of the module.
DOC_MODULE=libmediaart

# The top-level SGML file.
DOC_MAIN_SGML_FILE=libmediaart-docs.sgml

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS=

# Extra options to pass to gtkdoc-scangobj
# SCANGOBJ_OPTIONS=--type-init-func="gtk_type_init(0)"

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR=$(top_srcdir)/libmediaart

# Used for dependencies
HFILE_GLOB=$(top_srcdir)/libmediaart/*.h
CFILE_GLOB=$(top_srcdir)/libmediaart/*.c

# Header files to ignore when scanning
IGNORE_HFILES=$(top_srcdir)/libmediaart/storage.h

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
AM_CPPFLAGS = \
	-DMEDIAART_COMPILATION \
	$(LIBMEDIAART_CFLAGS)

GTKDOC_LIBS = \
	$(top_builddir)/src/libmediaart/libmediaart-@MEDIAART_API_VERSION@.la \
	$(LIBMEDIAART_LIBS)

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mkhtml
MKHTML_OPTIONS=--path="$(abs_builddir)"

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files = version.xml overview.sgml

expand_content_files =

# Images to copy into HTML directory
HTML_IMAGES =

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=\
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gobject \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/glib \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gio

include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST += version.xml.in
