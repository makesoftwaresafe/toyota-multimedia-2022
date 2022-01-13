AC_PREREQ([2.63])
AC_INIT([gupnp-av],
        [0.12.8],
        [https://bugzilla.gnome.org/enter_bug.cgi?product=GUPnP],
        [gupnp-av],
        [http://www.gupnp.org/])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR(libgupnp-av/gupnp-av.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 tar-ustar no-dist-gzip dist-xz -Wno-portability])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

AX_REQUIRE_DEFINED([GTK_DOC_CHECK])
AX_REQUIRE_DEFINED([GOBJECT_INTROSPECTION_CHECK])
AX_CHECK_ENABLE_DEBUG([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_HEADER_STDC
AC_FUNC_MMAP

AX_IS_RELEASE([git-directory])
AX_COMPILER_FLAGS([WARN_CFLAGS])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([win32-dll])

PKG_CHECK_MODULES(LIBGUPNP, gupnp-1.0 >= 0.19.0 \
                            gobject-2.0 \
                            libxml-2.0)

# glib-genmarshal
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

# Debugging
AC_ARG_ENABLE(debug,
	[  --enable-debug          enable debugging],,
        enable_debug=no)
if test "x$enable_debug" = "xyes"; then
        CFLAGS="$CFLAGS -g -Wall"
fi

GOBJECT_INTROSPECTION_CHECK([0.9.5])

# vapigen
have_vapigen=no
AS_IF([test "x$found_introspection" = "xyes"],
      [
       dnl output of g-i >= 1.36.0 needs vapigen >= 0.20 to process
       AC_MSG_CHECKING([whether G-I is 1.36 or newer])
       VAPIGEN_MIN_VERSION=0.14
       AS_IF([pkg-config --atleast-version=1.36.0 gobject-introspection-1.0],
             [
              AC_MSG_RESULT([yes])
              VAPIGEN_MIN_VERSION=0.22
             ],[AC_MSG_RESULT([no])]
       )
       GUPNP_PROG_VAPIGEN([$VAPIGEN_MIN_VERSION])
      ]
)
AS_IF([test "x$VAPIGEN" != "x"], [have_vapigen=yes])
AM_CONDITIONAL([HAVE_VAPIGEN], [test "x$VAPIGEN" != "x"])

GTK_DOC_CHECK([1.10],[--flavour no-tmpl])

AC_CONFIG_FILES([
Makefile
libgupnp-av/Makefile
tests/Makefile
tests/gtest/Makefile
doc/Makefile
doc/version.xml
data/Makefile
vala/Makefile
gupnp-av-1.0.pc
gupnp-av-1.0-uninstalled.pc
])
AC_OUTPUT

echo "
    ${PACKAGE} ${VERSION}
    ---------------------

    Prefix:                ${prefix}
    GObject-Introspection: ${found_introspection}
    VALA bindings:         ${have_vapigen}
"
