dnl (c) Copyright IBM Corporation 2010:
dnl 
dnl This documentation is provided with source code (Trusted Platform
dnl Module (TPM) subject to the following license:
dnl 
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions are
dnl met:
dnl 
dnl Redistributions of source code must retain the above copyright notice,
dnl this list of conditions and the following disclaimer.
dnl 
dnl Redistributions in binary form must reproduce the above copyright
dnl notice, this list of conditions and the following disclaimer in the
dnl documentation and/or other materials provided with the distribution.
dnl 
dnl Neither the names of the IBM Corporation nor the names of its
dnl contributors may be used to endorse or promote products derived from
dnl this software without specific prior written permission.
dnl 
dnl THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
dnl "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
dnl LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
dnl A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
dnl HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
dnl SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
dnl LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
dnl DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
dnl THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
dnl (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
dnl OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

dnl $Id: configure.ac,v 1.5 2014/08/13 18:09:13 jbj Exp $

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
m4_define([PACKAGE_BUGREPORT_DEFAULT], [rpm-devel@rpm5.org])
AC_INIT([libtpm],[4665],[PACKAGE_BUGREPORT_DEFAULT])
AC_MSG_TITLE([IBM Software TPM (LIBTPM)], [$PACKAGE_VERSION])

AC_MSG_HEADER([INITIALIZATION])
AC_CONFIG_SRCDIR([libtpm/lib/bind.c])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign tar-ustar subdir-objects color-tests])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_REQUIRE_AUX_FILE([install-sh])
AC_REQUIRE_AUX_FILE([mkinstalldirs])
AM_MAINTAINER_MODE

dnl # allow packagers to override the bugreport address
AC_ARG_WITH(bugreport,
    AS_HELP_STRING([--with-bugreport=ADDRESS], [set packager bugreport Email/Web address (PACKAGE_BUGREPORT_DEFAULT)]), [dnl
    case "$withval" in
        *@*|http*://* ) PACKAGE_BUGREPORT="$withval" ;;
        *   ) AC_MSG_ERROR([invalid bugreport Email/Web address]) ;;
    esac
])


dnl The following line is needed due to RHEL5
mkdir m4 2>/dev/null
AC_CONFIG_MACRO_DIR([m4])

dnl ##
dnl ## ==== TOOL-CHAIN CHECKS ====
dnl ##

AC_MSG_HEADER([TOOL-CHAIN CHECKS])

AC_PROG_CC_STDC
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX

AC_USE_SYSTEM_EXTENSIONS

AC_CPP_FUNC

AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_YACC

AC_PATH_PROG(AS, as, as)

dnl # GCC specifics
AC_PROG_GCC_TRADITIONAL

AC_SUBST(LIBTOOL_DEPS)

AC_DEFINE(TPM_POSIX, 1, [Define FIXME])
AC_DEFINE(TPM_NV_DISK, 1, [Define FIXME])

dnl Checks for build features
AC_MSG_CHECKING(whether to build for CCA)
AC_ARG_ENABLE(cca,
[  --enable-cca            Build the library with CCA support (instead of socket)],
[ AC_DEFINE(XCRYPTO_USE_CCA, 1, [Define as 1 to build with CCA support])
  AC_MSG_RESULT(yes)

  AC_MSG_CHECKING(whether UDXTK_FS_ROOT has been set)
  if test "set$UDXTK_FS_ROOT" != "set"; then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
    echo "You must set the environment variable UDXTK_FS_ROOT."
    exit
  fi
  AC_MSG_CHECKING(whether XCTK_FS_ROOT has been set)
  if test "set$XCTK_FS_ROOT" != "set"; then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
    echo "You must set the environment variable XCTK_FS_ROOT."
    exit
  fi
  AM_CPPFLAGS="-I${UDXTK_FS_ROOT}/inc -I${CROSS_PPC_HOME}/include -I../../cca-host-glue"
  UDXTK_LD_PATHS="-L${UDXTK_FS_ROOT}/lib/common -L${XCTK_FS_ROOT}/lib/host/linux/gcc/redhat" 
  UDXTK_LD_LIBS="-lcsulcall -lcsulsapi -lcsulsecy -lds30 -lxcryp -ldl"
  AC_DEFINE(HOST32BIT, 1, [Define as 1 on a 32 bit host])
  AC_DEFINE(_LINUX_, 1, [Define as 1 on Linux])
  AC_DEFINE(LINUX, 1, [Define as 1 on Linux])
  AC_DEFINE(TPM_VTPM, 1, [Define as 1 if virtual tpm is available])
  vtpm=true
],
[ AC_DEFINE(X86, 1, [Define as 1 on x86 platform])
  AC_MSG_RESULT(no) ])

AC_SUBST(UDXTK_LD_PATHS)
AC_SUBST(UDXTK_LD_LIBS)

AC_ARG_WITH(libtpms,
  [  --with-libtpms          Whether to build with libtpms support
                          Use '--with-libtpms=installed' to use the
                          installed libtpms. Use '--with-libtpms=no'
                          to build without libtpms support.],
  [],
  [with_libtpms=no])

if test "$with_libtpms" != no && test "x$with_libtpms" != x; then
    if test "$with_libtpms" = "installed"; then
        if test ! -r /usr/include/libtpms/tpm_library.h; then
            AC_MSG_ERROR([the libtpms-devel package is not installed on this system])
        fi
        LIBTPMS_LIBRARY_PATH=
        AC_DEFINE(TPM_USE_LIBTPMS, 1, [Define as 1 to use libtpms])
        LIBTPMS_LIBRARY="-ltpms"
    else
	if test "`uname -o 2>/dev/null`" = Cygwin; then
		tmp=`readlink -f ../../src/libtpms.dll.a`
		LINKERPARMS="-Wl,-rpath=${PWD}/../../src"
	elif test "`uname -s`" = AIX; then
		tmp=`ls -lL ../../src/libtpms.so 2>/dev/null |\
		     awk '{ print $NF }'`
		LINKERPARMS="-Wl,-brtl"	
	elif test "`uname -s`" = Linux; then
	        tmp=`readlink -f ../../src/libtpms.so`
		LINKERPARMS="-Wl,-rpath=${PWD}/../../src"
	fi
        if test "x$tmp" != x && test -f $tmp; then
            LIBTPMS_LIBRARY_PATH="-L../../../src ${LINKERPARMS}"
            AC_DEFINE(USE_IN_TREE_LIBTPMS, 1, [Define as 1 to use libtpms in-tree])
            AC_DEFINE(TPM_USE_LIBTPMS)
            LIBTPMS_LIBRARY="-ltpms"
            with_libtpms=yes
        else
            AC_MSG_ERROR([libtpms could not be found; try 'make -f makefile-libtpms' in src/])
        fi
    fi
fi
AC_SUBST(LIBTPMS_LIBRARY)
AC_SUBST(LIBTPMS_LIBRARY_PATH)

AC_MSG_CHECKING([for the size of keys to support])
AC_ARG_WITH(size-keys,
  [  --with-size-keys=NUMBER
                          The maximum size of asymmetric keys supported
                          (default=4096).],
  [number_key_bits="$withval"],
  [number_key_bits="4096"])
AC_DEFINE_UNQUOTED(TPM_MAXIMUM_KEY_SIZE,${number_key_bits}, [Define as the maximum no. of bits in a key])
AC_MSG_RESULT($number_key_bits)
AC_SUBST(TPM_MAXIMUM_KEY_SIZE)

default_transport="TCP/IP"

AC_MSG_CHECKING([whether to send messages to character device])
AC_ARG_ENABLE(chardev,
  [  --enable-chardev        To build for sending to character device.],
  AC_DEFINE(TPM_USE_CHARDEV, 1, [Define as 1 to use a /dev/tpmN character device])
  default_transport="/dev/tpm0"
  AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no)
)

AC_MSG_CHECKING([whether to send messages via UnixIO socket])
AC_ARG_ENABLE(unixio,
  [  --enable-unixio         To build for sending via UnixIO socket.],
  AC_DEFINE(TPM_USE_UNIXIO, 1, [Define as 1 to use unixio device])
  default_transport="UNIXIO socket"
  AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no)
)

AC_MSG_CHECKING([whether to enable virtual TPM support])
AC_ARG_ENABLE(vtpm,
  [  --enable-vtpm           To enable virtual TPM support when using sockets.],
  AC_DEFINE(TPM_VTPM)
  vtpm=true
  AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no)
)

AC_MSG_CHECKING([whether to use serial port])
AC_ARG_ENABLE(serial-port,
  [  --enable-serial-port    To enable communication via serial port.],
  AC_DEFINE(USE_SERIAL_PORT, 1, [Define as 1 to use a /dev/tpmN serial device])
  AC_DEFINE(USE_PARTIAL_READ, 1, [Define as 1 to permit partial reads on serial device])
  AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no)
)

#The following generates a non-fatal error when linking:
# gcc : -lc : linker input file unused because linking not done
#AC_MSG_CHECKING([whether to build statically linked executables])
#AC_ARG_ENABLE(staticexec,
#  [  --enable-staticexec    To build statically linked executables.],
#  STATIC_LINKING_FLAG="-static -lc"
#  AC_DEFINE(USE_STATIC_LINKING)
#  AC_MSG_RESULT(yes),
#  AC_MSG_RESULT(no)
#)


dnl Checks for libraries.
dnl ##
dnl ## ==== API CHECKS ====
dnl ##

AC_MSG_HEADER([API CHECKS])

dnl # standard headers (specific)
AC_HEADER_STDC

dnl # standard headers (generic)
AC_CHECK_HEADERS(assert.h fcntl.h sys/ioctl.h string.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl check for missing typedefs
AC_TYPE_SIZE_T

dnl checks for library functions (specific)
AC_FUNC_MEMCMP

dnl ##
dnl ## ==== THIRD-PARTY LIBRARIES (2/2) ====
dnl ##

AC_MSG_HEADER([THIRD-PARTY LIBRARIES (2/2)])

dnl # POPT
WITH_LIBTPM_UTILS_SUBDIR="utils"
RPMIO_CFLAGS="-I/X/src/wdj54/rpmio"
RPMIO_LDADD="/X/src/wdj54/rpmio/librpmio.la /X/src/wdj54/misc/librpmmisc.la"
RPM_CHECK_LIB(
    [POPT], [popt],
    [popt], [poptGetContext], [popt.h],
    [yes,external:internal:none], [../popt],
    [ if test ".$RPM_CHECK_LIB_LOCATION" = .internal; then
         WITH_LIBTPM_UTILS_SUBDIR="# utils"
         RPMIO_LDADD="../rpmio/librpmio.la ../misc/librpmmisc.la"
         RPMIO_CFLAGS="-I../rpmio"
      fi
    ], [])
AC_SUBST(WITH_LIBTPM_UTILS_SUBDIR)
AC_SUBST(RPMIO_CFLAGS)
AC_SUBST(RPMIO_LDADD)

dnl # Mozilla NSS
RPM_CHECK_LIB(
    [Mozilla NSS], [nss],
    [nss3 nss], [NSS_NoDB_Init], [nss.h],
    [no,external:none], [],
    [ AC_DEFINE(WITH_NSS, 1, [Define if building with Mozilla NSS])
      AM_CONDITIONAL([WITH_NSS], [true])
    ],
    [ AM_CONDITIONAL([WITH_NSS], [false])
    ])

dnl # OpenSSL
RPM_CHECK_LIB(
    [OpenSSL], [openssl],
    [crypto], [ERR_load_CRYPTO_strings], [openssl/crypto.h],
    [yes,external], [],
    [ AC_DEFINE(WITH_SSL, 1, [Define if building with OpenSSL])
      AC_CHECK_LIB([crypto],[HMAC_CTX_cleanup],[
          AC_DEFINE(HAVE_HMAC_CTX_CLEANUP, 1, [Define to 1 if HMAC_CTX_cleanup() is available.])
      ])
      AC_CHECK_LIB([openssl], [RSA_generate_key_ex], [
          AC_DEFINE(HAVE_RSA_GENERATE_KEY_EX, 1, [Define to 1 if RSA_generate_key_ex() is available.])
      ])
      AC_CHECK_LIB([openssl], [DSA_generate_parameters_ex], [
          AC_DEFINE(HAVE_DSA_GENERATE_PARAMETERS_EX, 1, [Define to 1 if DSA_generate_parameters_ex() is available.])
      ])
      AM_CONDITIONAL([WITH_OPENSSL], [true])
    ],
    [ AC_MSG_ERROR([mandatory OpenSSL library not found])
      AM_CONDITIONAL([WITH_OPENSSL], [false])
    ])

AC_DEFINE(TPM_V12, 1, [Define as 1 to enable TPM 1.2])
AC_DEFINE(TPM_AES, 1, [Define as 1 to enable AES])
AC_DEFINE(TPM_USE_TAG_IN_STRUCTURE, 1, [Define as 1 FIXME])

AM_CONDITIONAL(VTPM_TOOLS, test x$vtpm = xtrue)
AC_SUBST(VTPM_TOOLS)

AC_SUBST(STATIC_LINKING_FLAG)
AC_SUBST(AM_CPPFLAGS)

AM_CONDITIONAL(LIBTPMS_AVAILABLE,
               test x$with_libtpms = xyes || test x$with_libtpms = xinstalled)
AC_SUBST(LIBTPMS_AVAILABLE)

AC_CONFIG_FILES([
    Makefile
    libtpm/Makefile libtpm/lib/Makefile libtpm/utils/Makefile
    tpm/Makefile
    tpm_proxy/Makefile tpm_proxy/linux/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([-------------------------------])

if test "x$number_key_bits" != "x"; then
	AC_MSG_NOTICE([max. key size: $number_key_bits (in client lib.)])
fi

	AC_MSG_NOTICE([def.transport: $default_transport (change with TPM_DEVICE env. var.)])

if test "$with_libtpms" = "yes"; then
	AC_MSG_NOTICE([      libtpms: using in-source-tree libtpms])
elif test "$with_libtpms" = "installed"; then
	AC_MSG_NOTICE([      libtpms: using system-installed libtpms])
elif test "$with_libtpms" = "notfound"; then
	AC_MSG_NOTICE([      libtpms: not found (run 'make -f makefile-libtpms' in src/)])
else
	AC_MSG_NOTICE([      libtpms: no])
fi

