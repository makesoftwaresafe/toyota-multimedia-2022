# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# This file is part of Libmediaart.

AC_PREREQ([2.64])

# Making releases:
#   LIBMEDIAART_MICRO_VERSION += 1;
#   LIBMEDIAART_INTERFACE_AGE += 1;
#   LIBMEDIAART_BINARY_AGE += 1;
# if any functions have been added, set LIBMEDIAART_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set LIBMEDIAART_BINARY_AGE and LIBMEDIAART_INTERFACE_AGE to 0.
m4_define([libmediaart_major_version], [1])
m4_define([libmediaart_minor_version], [9])
m4_define([libmediaart_micro_version], [0])
m4_define([libmediaart_interface_age], [0])
m4_define([libmediaart_binary_age],
          [m4_eval(100 * libmediaart_minor_version + libmediaart_micro_version)])
m4_define([libmediaart_version],
          [libmediaart_major_version.libmediaart_minor_version.libmediaart_micro_version])
# This is the X.Y used in -lliblibmediaart-FOO-X.Y
m4_define([libmediaart_api_version], [2.0])

# libtool version related macros
m4_define([libmediaart_lt_release], [libmediaart_major_version.libmediaart_minor_version])
m4_define([libmediaart_lt_current],
          [m4_eval(100 * libmediaart_minor_version + libmediaart_micro_version - libmediaart_interface_age)])
m4_define([libmediaart_lt_revision], [libmediaart_interface_age])
m4_define([libmediaart_lt_age], [m4_eval(libmediaart_binary_age - libmediaart_interface_age)])
m4_define([libmediaart_lt_current_minus_age],
          [m4_eval(libmediaart_lt_current - libmediaart_lt_age)])

# Define a string for the earliest version that this release has
# backwards binary compatibility with for all interfaces a module
# might. Unless we add module-only API with lower stability
# guarantees, this should be unchanged until we break binary compat
# for Libmediaart.
#
# LIBMEDIAART_BINARY_VERSION=$LIBMEDIAART_MAJOR_VERSION.$LIBMEDIAART_MINOR_VERSION.$LT_CURRENT
m4_define([libmediaart_binary_version], [2.0.0])

AC_INIT([libmediaart],
        [libmediaart_version],
        [http://bugzilla.gnome.org/],
        [libmediaart],
        [https://wiki.gnome.org/MediaArtStorageSpec])

AC_CONFIG_SRCDIR([libmediaart/mediaart.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 no-define tar-ustar -Wno-portability])

AM_SILENT_RULES([yes])

LIBMEDIAART_MAJOR_VERSION=libmediaart_major_version
LIBMEDIAART_MINOR_VERSION=libmediaart_minor_version
LIBMEDIAART_MICRO_VERSION=libmediaart_micro_version
LIBMEDIAART_INTERFACE_AGE=libmediaart_interface_age
LIBMEDIAART_BINARY_AGE=libmediaart_binary_age
LIBMEDIAART_VERSION=libmediaart_version
LIBMEDIAART_API_VERSION=libmediaart_api_version
LIBMEDIAART_BINARY_VERSION=libmediaart_binary_version

AC_SUBST(LIBMEDIAART_MAJOR_VERSION)
AC_SUBST(LIBMEDIAART_MINOR_VERSION)
AC_SUBST(LIBMEDIAART_MICRO_VERSION)
AC_SUBST(LIBMEDIAART_INTERFACE_AGE)
AC_SUBST(LIBMEDIAART_BINARY_AGE)
AC_SUBST(LIBMEDIAART_API_VERSION)
AC_SUBST(LIBMEDIAART_VERSION)
AC_SUBST(LIBMEDIAART_BINARY_VERSION)
AC_SUBST(LIBMEDIAART_API_VERSION_U,[AS_TR_SH([$LIBMEDIAART_API_VERSION])])

AC_DEFINE(LIBMEDIAART_MAJOR_VERSION, [libmediaart_major_version],
          [Define to the Libmediaart major version])
AC_DEFINE(LIBMEDIAART_MINOR_VERSION, [libmediaart_minor_version],
          [Define to the Libmediaart minor version])
AC_DEFINE(LIBMEDIAART_MICRO_VERSION, [libmediaart_micro_version],
          [Define to the Libmediaart micro version])
AC_DEFINE(LIBMEDIAART_INTERFACE_AGE, [libmediaart_interface_age],
          [Define to the Libmediaart interface age])
AC_DEFINE(LIBMEDIAART_BINARY_AGE, [libmediaart_binary_age],
          [Define to the Libmediaart binary age])

# libtool versioning
LT_RELEASE=libmediaart_lt_release
LT_CURRENT=libmediaart_lt_current
LT_REVISION=libmediaart_lt_revision
LT_AGE=libmediaart_lt_age
LT_CURRENT_MINUS_AGE=libmediaart_lt_current_minus_age
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_CURRENT_MINUS_AGE)

IDT_COMPILE_WARNINGS

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_MKDIR_P
AC_PROG_YACC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([setenv strstr utime])

CFLAGS="$CFLAGS"

GLIB_TESTS

# Library required versions
GLIB_REQUIRED=2.38.0
GDKPIXBUF_REQUIRED=2.12.0
QT5_REQUIRED=5.0.0
QT4_REQUIRED=4.7.1

# Check requirements for libmediaart
LIBMEDIAART_REQUIRED="glib-2.0 >= $GLIB_REQUIRED
                      gio-2.0 >= $GLIB_REQUIRED
                      gio-unix-2.0 >= $GLIB_REQUIRED"
PKG_CHECK_MODULES(LIBMEDIAART, [$LIBMEDIAART_REQUIRED])

GLIB_GENMARSHAL=`$PKG_CONFIG glib-2.0 --variable=glib_genmarshal`
AC_SUBST(GLIB_GENMARSHAL)

GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
AC_SUBST(GLIB_PREFIX)

####################################################################
# General CFLAGS/LIBS
####################################################################
BUILD_CFLAGS="${WARN_CFLAGS} -DG_LOG_DOMAIN=\\\"libmediaart\\\" -DLIBMEDIAART_COMPILATION"
BUILD_LIBS="${GCOV_LIBS}"

AC_SUBST(BUILD_VALAFLAGS)
AC_SUBST(BUILD_CFLAGS)
AC_SUBST(BUILD_LIBS)

####################################################################
# Check for gtk-doc and docbook-tools
####################################################################

# Check for GTK_DOC_CHECK availability. The GTK_DOC_CHECK invocation
# must be on its own line, gtkdocize relies on it
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.8])

# NOTE: We need to use a separate automake conditional for this
#       to make this work with the tarballs.
AM_CONDITIONAL([ENABLE_GTK_DOC], test "x$enable_gtk_doc" = xyes)
],
[
AM_CONDITIONAL([ENABLE_GTK_DOC], false)
])

####################################################################
# Build introspectable bindings
####################################################################

GOBJECT_INTROSPECTION_CHECK([1.30.0])

m4_ifdef([VAPIGEN_CHECK], [
# Vala bindings
VAPIGEN_CHECK([0.16])
],
[
AM_CONDITIONAL([ENABLE_VAPIGEN], false)
])

##################################################################
# Check for GdkPixbuf for MP3 album art
##################################################################

AC_ARG_ENABLE(gdkpixbuf,
              AS_HELP_STRING([--enable-gdkpixbuf],
                             [enable GdkPixbuf support, (MP3 album art) [[default=auto]]]),,
              [enable_gdkpixbuf=auto])

##################################################################
# Check for libmediaart-extract: Qt for MP3 album art
##################################################################

AC_ARG_ENABLE(qt,
              AS_HELP_STRING([--enable-qt],
                             [enable Qt support, (MP3 album art) [[default=auto]]]),,
              [enable_qt=auto])

AC_ARG_WITH(qt-version,
            AS_HELP_STRING([--with-qt-version=major],
                           [Force specific Qt version [[default=auto]]]),,
            [with_qt_version=auto])

selected_for_media_art="no  (disabled)"

##################################################################
# Choose between backends (GdkPixbuf/Qt/etc)
##################################################################

if test "x$enable_qt" == "xyes" && test "x$enable_gdkpixbuf" == "xyes"; then
   AC_MSG_ERROR([Can not enable both Qt and GdkPixbuf backends, please pick one])
fi

if test "x$enable_qt" != "xno" && test "x$enable_gdkpixbuf" != "xyes"; then
   if test "x$with_qt_version" == "xauto" || test "x$with_qt_version" == "x5"; then
       PKG_CHECK_MODULES(QT5,
                         [Qt5Gui >= $QT5_REQUIRED],
                         [have_qt5=yes],
                         [have_qt5=no])
   fi
   if test "x$with_qt_version" == "xauto" || test "x$with_qt_version" == "x4"; then
       PKG_CHECK_MODULES(QT4,
                         [QtGui >= $QT4_REQUIRED],
                         [have_qt4=yes],
                         [have_qt4=no])
   fi

   if test "x$have_qt5" = "xyes"; then
      BACKEND_CFLAGS="$QT5_CFLAGS -fPIC"
      BACKEND_LIBS="$QT5_LIBS"

      AC_DEFINE(HAVE_QT5, [], [Define if we have Qt5])
      AC_DEFINE(HAVE_QT, [], [Define if we have Qt])
      LIBMEDIAART_BACKEND=Qt5Gui

      selected_for_media_art="yes (qt5)"
   else
      if test "x$have_qt4" = "xyes"; then
         BACKEND_CFLAGS="$QT4_CFLAGS -fPIC"
         BACKEND_LIBS="$QT4_LIBS"

         AC_DEFINE(HAVE_QT4, [], [Define if we have Qt4])
         AC_DEFINE(HAVE_QT, [], [Define if we have Qt])
         LIBMEDIAART_BACKEND=QtGui

         selected_for_media_art="yes (qt4)"
      fi
   fi
else
   have_qt4="no  (disabled)"
   have_qt5="no  (disabled)"
fi

if test "x$enable_qt" = "xyes"; then
   if test "x$have_qt5" != "xyes"; then
      if test "x$have_qt4" != "xyes"; then
         AC_MSG_ERROR([Couldn't find Qt4 >= $QT4_REQUIRED or Qt5 >= $QT5_REQUIRED.])
      fi
   fi
fi

AM_CONDITIONAL(HAVE_QT4, test "x$have_qt4" = "xyes")
AM_CONDITIONAL(HAVE_QT5, test "x$have_qt5" = "xyes")
AM_CONDITIONAL(HAVE_QT, test "x$have_qt5" = "xyes" || test "x$have_qt4" = "xyes")

if test "x$enable_gdkpixbuf" != "xno" && test "x$enable_qt" != "xyes"; then
   PKG_CHECK_MODULES(GDKPIXBUF,
                     [gdk-pixbuf-2.0 >= $GDKPIXBUF_REQUIRED],
                     [have_gdkpixbuf=yes],
                     [have_gdkpixbuf=no])

   BACKEND_CFLAGS="$GDKPIXBUF_CFLAGS"
   BACKEND_LIBS="$GDKPIXBUF_LIBS"

   if test "x$have_gdkpixbuf" = "xyes"; then
      AC_DEFINE(HAVE_GDKPIXBUF, [], [Define if we have GdkPixbuf])
      LIBMEDIAART_BACKEND=gdk-pixbuf-2.0
      selected_for_media_art="yes (GdkPixbuf)"
   fi
else
   have_gdkpixbuf="no  (disabled)"
fi

if test "x$enable_gdkpixbuf" = "xyes"; then
   if test "x$have_gdkpixbuf" != "xyes"; then
      AC_MSG_ERROR([Couldn't find GdkPixbuf >= $GDKPIXBUF_REQUIRED.])
   fi
fi

AM_CONDITIONAL(HAVE_GDKPIXBUF, test "x$have_gdkpixbuf" = "xyes")

LIBMEDIAART_CFLAGS="$LIBMEDIAART_CFLAGS $BACKEND_CFLAGS"
LIBMEDIAART_LIBS="$LIBMEDIAART_LIBS $BACKEND_LIBS"
AC_SUBST(LIBMEDIAART_BACKEND)

#################################################################
# Check if we should install nemo specific features
#################################################################

AC_ARG_ENABLE(nemo,
              AS_HELP_STRING([--enable-nemo],
                             [enable nemo specific [[default=no]]]),
             [enable_nemo="$enableval"],
             [enable_nemo=no])

AM_CONDITIONAL(HAVE_NEMO, test "x$enable_nemo" = "xyes")

if test "x$enable_nemo" = "xyes" ; then
   AC_DEFINE(HAVE_NEMO, 1, [Define if we enable Nemo specific features])
fi

####################################################################
# Should we build GLib based unit tests
####################################################################

AC_ARG_ENABLE(unit_tests,
              AS_HELP_STRING([--enable-unit-tests],
                             [enable unit tests [[default=yes]]]), ,
              [enable_unit_tests=yes])

if test "x$enable_unit_tests" != "xno" ; then
   have_unit_tests=yes

   AC_DEFINE(HAVE_UNIT_TEST, 1, [Define if we have GLib unit test framework])

   AC_PATH_PROG(gtester, gtester)
   if test -z $gtester; then
      AC_MSG_ERROR([Could not find 'gtester'])
   fi
else
   have_unit_tests=no
fi

AM_CONDITIONAL(HAVE_UNIT_TESTS, test "x$have_unit_tests" = "xyes")

##################################################################
# Write generated files
##################################################################

AC_CONFIG_FILES([
	Makefile
	libmediaart.pc
	libmediaart/Makefile
	tests/Makefile
	docs/Makefile
	docs/reference/Makefile
	docs/reference/libmediaart/Makefile
	docs/reference/libmediaart/version.xml
])

AC_OUTPUT

echo "
Build Configuration:

	Prefix:                                 ${prefix}
	Source code location:                   ${srcdir}
	Compiler:                               ${CC}
	Compiler Warnings:                      $enable_compile_warnings

	Enable unit tests:                      $have_unit_tests
	Enable documentation:                   $enable_gtk_doc

	Support media art processing:           $selected_for_media_art

        Support Nemo                            $enable_nemo
"
