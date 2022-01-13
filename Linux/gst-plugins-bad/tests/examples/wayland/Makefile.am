noinst_PROGRAMS = gst-wayland-demo

gst_wayland_demo_SOURCES = gst-wayland-demo.c

gst_wayland_demo_CFLAGS=-I$(top_srcdir)/gst-libs -I$(top_builddir)/gst-libs \
	$(GST_PLUGINS_BASE_CFLAGS) $(GST_CFLAGS) $(WAYLAND_CFLAGS)
gst_wayland_demo_LDADD=$(GST_PLUGINS_BASE_LIBS) $(GST_LIBS) \
	-lgstvideo-$(GST_API_VERSION) \
	$(WAYLAND_LIBS) \
	$(top_builddir)/gst-libs/gst/wayland/libgstwayland-$(GST_API_VERSION).la
gst_wayland_demo_LDFLAGS=-no-install
