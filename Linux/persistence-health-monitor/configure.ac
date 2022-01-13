#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.


# create tag version information
m4_define([pers_health_monitor_tag_version_major], [0])
m4_define([pers_health_monitor_tag_version_minor], [0])
m4_define([pers_health_monitor_tag_version_micro], [1])
m4_define([pers_health_monitor_tag_version], [pers_health_monitor_tag_version_major().pers_health_monitor_tag_version_minor().pers_health_monitor_tag_version_micro()])


AC_INIT([Persistence Health Monitor], [pers_health_monitor_tag_version], [not-valid@xse.de])
AC_COPYRIGHT([Copyright (c) 2014 XS Embedded GmbH])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([dist-bzip2])
AC_GNU_SOURCE()


m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


# Overwrite the build in defaults
CFLAGS="-Wall -O2"

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_ARG_ENABLE([tests],
              [AS_HELP_STRING([--enable-tests],[Enable tests])],
              [enable_tests=$enableval],[enable_tests="no"])

AM_CONDITIONAL([WANT_TESTS], [test x"$enable_tests" = "xyes"])

 
# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MMAP
AC_CHECK_FUNCS([fdatasync ftruncate mkdir munmap rmdir strerror utime dlopen])

PKG_CHECK_MODULES(DEPS,
                  automotive-dlt
                  dbus-1
                  node-state-manager)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_ARG_WITH([localcheck],
              [AS_HELP_STRING([--with-localcheck],[Path to local check])],
              [localcheck=$withval],[localcheck=""])

if test "x$enable_tests" != "xno" ; then
   if test -z "$localcheck" ; then
      PKG_CHECK_MODULES(CHECK, check)
   else
      CHECK_CFLAGS="-I$localcheck/include"
      CHECK_LIBS="-L$localcheck/lib -lcheck"
   fi
   AC_SUBST(CHECK_CFLAGS)
   AC_SUBST(CHECK_LIBS)

   AC_DEFINE_UNQUOTED([TESTS_ENABLED], [1], [tests enabled])
else
   AC_DEFINE_UNQUOTED([TESTS_ENABLED], [0], [tests enabled])
fi
AC_MSG_NOTICE([Tests enabled: $enable_tests])
AC_MSG_NOTICE([Local check enabled: $localcheck])


AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")


AC_CONFIG_FILES([Makefile
                 persistence_health_monitor.pc
                 src/Makefile
                 test/Makefile])
              
AC_OUTPUT
