include $(top_srcdir)/Makefile.decl

test_programs = mediaarttest
dist_test_data = \
	543249_King-Kilo---Radium.mp3 \
	cover.png

AM_CPPFLAGS = \
	$(BUILD_CFLAGS) \
	-I$(top_srcdir) \
	$(LIBMEDIAART_CFLAGS)                    

LDADD = \
	$(top_builddir)/libmediaart/libmediaart-@LIBMEDIAART_API_VERSION@.la \
	$(BUILD_LIBS) \
	$(LIBMEDIAART_LIBS)

mediaarttest_SOURCES = mediaarttest.c
