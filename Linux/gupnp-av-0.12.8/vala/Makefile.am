if HAVE_INTROSPECTION
if HAVE_VAPIGEN
gupnp-av-1.0.vapi: gupnp-av-1.0.stamp
	@true

gupnp-av-1.0.stamp: $(top_builddir)/libgupnp-av/GUPnPAV-1.0.gir \
		$(srcdir)/GUPnPAV-1.0.metadata \
		$(srcdir)/gupnp-av-1.0-custom.vala
	$(AM_V_GEN)$(VAPIGEN) --metadatadir=$(srcdir) \
				  --pkg gupnp-1.0 --pkg=libsoup-2.4 \
			      --library=gupnp-av-1.0 --pkg=gio-2.0 --pkg=libxml-2.0 \
			      $(top_builddir)/libgupnp-av/GUPnPAV-1.0.gir \
				  $(srcdir)/gupnp-av-1.0-custom.vala && \
	echo timestmap > $(@F)

vapidir = $(datadir)/vala/vapi
vapi_DATA = gupnp-av-1.0.vapi $(srcdir)/gupnp-av-1.0.deps
CLEANFILES = gupnp-av-1.0.vapi gupnp-av-1.0.stamp
EXTRA_DIST = gupnp-av-1.0.deps GUPnPAV-1.0.metadata gupnp-av-1.0-custom.vala gupnp-av-1.0.vapi gupnp-av-1.0.stamp
endif
endif
