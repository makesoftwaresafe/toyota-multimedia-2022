# Process this file with autoconf to produce a configure script.

AC_INIT([beecrypt],[4.2.1],[bob.deblier@telenet.be])
AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR(include/beecrypt/beecrypt.h)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE

# Checks for AWK - used by expert mode
AC_PROG_AWK

# Checks for package options
AC_ARG_ENABLE(expert-mode, [  --enable-expert-mode      follow user-defined CFLAGS settings [[default=no]]],[
  ac_enable_expert_mode=yes
  ],[
  if test "X$CFLAGS" != "X"; then
    echo "enabling expert mode"
    ac_enable_expert_mode=yes
  else
    ac_enable_expert_mode=no
  fi
  ])

AC_ARG_ENABLE(debug, [  --enable-debug          creates debugging code [[default=no]]],[
  if test "$ac_enable_expert_mode" = yes; then
    AC_MSG_ERROR([--enable-debug cannot be used in conjunction with --enable-expert-mode])
  fi
  ac_enable_debug=yes
  ],[
  ac_enable_debug=no
  ])

AC_ARG_WITH(cpu,[  --with-cpu              optimize for specific cpu],[A
  if test "$ac_enable_expert_mode" = yes; then
    AC_MSG_ERROR([--with-cpu cannot be used in conjunction with --enable-expert-mode])
  fi
  BEE_WITH_CPU
  ],[
  BEE_WITHOUT_CPU
  ])

AC_ARG_WITH(arch,[  --with-arch             optimize for specific architecture (may not run on other cpus of same family)],[
  if test "$ac_enable_expert_mode" = yes; then
    AC_MSG_ERROR([--with-arch cannot be used in conjunction with --enable-expert-mode])
  fi
  BEE_WITH_ARCH
  ],[
  BEE_WITHOUT_ARCH
  ])

AC_ARG_ENABLE(threads,[  --enable-threads        enables multithread support [[default=yes]]],[
  if test "$enableval" = no; then
    ac_enable_threads=no
  else
    ac_enable_threads=yes
  fi
  ],[ac_enable_threads=yes])

AC_ARG_ENABLE(aio,[  --enable-aio            enables asynchronous i/o for entropy gathering [[default=yes]]],[
  if test "$enableval" = no; then
    ac_enable_aio=no
  else
    ac_enable_aio=yes
  fi
  ],[ac_enable_aio=yes])

AH_TEMPLATE([ENABLE_AIO],[Define to 1 if you want to enable asynchronous I/O support])

AC_ARG_WITH(mtmalloc,[  --with-mtmalloc         links against the mtmalloc library [[default=no]]],[
  if test "$withval" = no; then
    ac_with_mtmalloc=no
  else
    ac_with_mtmalloc=yes
  fi
  ],[ac_with_mtmalloc=no])

AC_ARG_WITH(cplusplus,[  --with-cplusplus        creates the C++ API code [[default=no]]],[
  if test "$withval" = no; then
    ac_with_cplusplus=no
  else
    ac_with_cplusplus=yes
  fi
  ],[ac_with_cplusplus=no])

AC_ARG_WITH(java,[  --with-java             creates the Java glue code [[default=no]]],[
  if test "$withval" = no; then
    ac_with_java=no
  else
    ac_with_java=yes
  fi
  ],[ac_with_java=no])

AC_ARG_WITH(python,[  --with-python[[=ARG]]     creates the Python module [[default=yes]]; you can optionally specify the python executable],[
  if test "$withval" = no; then
    ac_with_python=no
  else
    ac_with_python=yes
    ac_with_python_val=$withval
  fi
  ],[
  ac_with_python_val=`which python`
  if test "X$ac_with_python_val" = "X"; then
    ac_with_python=no
  else
    ac_with_python=yes
  fi
  ])

# Check for expert mode
if test "$ac_enable_expert_mode" = yes; then
  BEE_EXPERT_MODE
fi

# Check for aggressive optimization
BEE_AGGRESSIVE_OPT

# Check for Unix variants
AC_AIX

# Checks for C compiler and preprocessor
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AM_PROG_AS
AC_PROG_LD
AC_PROG_LN_S
AC_PROG_EGREP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_ARG_ENABLE(build-gcov,
    AS_HELP_STRING([--enable-build-gcov], [build BeeCrypt instrumented for gcov]), [dnl
    if test ".$enableval" = .yes; then
        if test ".`$CC --version 2>&1 | grep 'GCC'`" != .; then
            dnl # GNU GCC (usually "gcc")
            CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
        fi
    fi
])

# Checks for OpenMP support
AC_LANG_PUSH(C)
AC_OPENMP
AC_LANG_POP(C)
AC_LANG_PUSH(C++)
AC_OPENMP
AC_LANG_POP(C++)

# Checks for compiler characteristics and flags
if test "$ac_enable_expert_mode" = no; then
  BEE_CC
  BEE_CXX
fi

# Check for stack protection
BEE_CC_NOEXECSTACK
BEE_CXX_NOEXECSTACK

# Checks for program flags needed by libtool
BEE_LIBTOOL

# Predefines for autoheader
BEE_OS_DEFS

AH_TEMPLATE([HAVE_ASSERT_H],[.])
AH_TEMPLATE([HAVE_CTYPE_H],[.])
AH_TEMPLATE([HAVE_ERRNO_H],[.])
AH_TEMPLATE([HAVE_FCNTL_H],[.])
AH_TEMPLATE([HAVE_STDIO_H],[.])
AH_TEMPLATE([HAVE_TERMIO_H],[.])
AH_TEMPLATE([HAVE_TERMIOS_H],[.])
AH_TEMPLATE([HAVE_TIME_H],[.])
AH_TEMPLATE([HAVE_DLFCN_H],[.])

AH_TEMPLATE([HAVE_SYS_AUDIOIO_H],[.])
AH_TEMPLATE([HAVE_SYS_IOCTL_H],[.])
AH_TEMPLATE([HAVE_SYS_MMAN_H],[.])
AH_TEMPLATE([HAVE_SYS_SOUNDCARD_H],[.])
AH_TEMPLATE([HAVE_SYS_STAT_H],[.])
AH_TEMPLATE([HAVE_SYS_TIME_H],[.])
AH_TEMPLATE([HAVE_SYS_TYPES_H],[.])
AH_TEMPLATE([HAVE_ENDIAN_H],[.])
AH_TEMPLATE([HAVE_ASM_BYTEORDER_H],[.])

AH_TEMPLATE([HAVE_AIO_H],[.])

AH_TEMPLATE([HAVE_DEV_AUDIO],[Define to 1 if your system has device /dev/audio])
AH_TEMPLATE([HAVE_DEV_DSP],[Define to 1 if your system has device /dev/dsp])
AH_TEMPLATE([HAVE_DEV_RANDOM],[Define to 1 if your system has device /dev/random])
AH_TEMPLATE([HAVE_DEV_URANDOM],[Define to 1 if your system has device /dev/urandom])
AH_TEMPLATE([HAVE_DEV_TTY],[Define to 1 if your system has device /dev/tty])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([time.h sys/time.h])
AC_HEADER_TIME
AC_CHECK_HEADERS([assert.h ctype.h errno.h fcntl.h malloc.h stdio.h termio.h termios.h])
AC_CHECK_HEADERS([sys/ioctl.h sys/mman.h sys/audioio.h sys/soundcard.h])
AC_CHECK_HEADERS([endian.h sys/endian.h asm/byteorder.h])

bc_include_stdio_h=
bc_include_stdlib_h=
bc_include_malloc_h=
bc_include_string_h=
bc_include_unistd_h=
bc_include_dlfcn_h=
if test "$ac_cv_header_stdio_h" = yes; then
  bc_include_stdio_h="#include <stdio.h>"
fi
if test "$ac_cv_header_stdlib_h" = yes; then
  bc_include_stdlib_h="#include <stdlib.h>"
elif test "$ac_cv_header_malloc_h" = yes; then
  bc_include_malloc_h="#include <malloc.h>"
fi
if test "$ac_with_mtmalloc" = yes; then
  AC_CHECK_HEADERS(mtmalloc.h)
  if test "$ac_cv_header_mtmalloc_h" = yes; then
    bc_include_stdlib_h=
    bc_include_malloc_h="#include <mtmalloc.h>"
  fi
fi
if test "$ac_cv_header_string_h" = yes; then
  bc_include_string_h="#include <string.h>"
fi
if test "$ac_cv_header_unistd_h" = yes; then
  bc_include_unistd_h="#include <unistd.h>"
fi
if test "$ac_cv_header_dlfcn_h" = yes; then
  bc_include_dlfcn_h="#include <dlfcn.h>"
fi
AC_SUBST(INCLUDE_STDIO_H,$bc_include_stdio_h)
AC_SUBST(INCLUDE_STDLIB_H,$bc_include_stdlib_h)
AC_SUBST(INCLUDE_MALLOC_H,$bc_include_malloc_h)
AC_SUBST(INCLUDE_STRING_H,$bc_include_string_h)
AC_SUBST(INCLUDE_UNISTD_H,$bc_include_unistd_h)
AC_SUBST(INCLUDE_DLFCN_H,$bc_include_dlfcn_h)

BEE_DLFCN
BEE_MULTITHREAD
BEE_THREAD_LOCAL_STORAGE

# Checks for libraries.
if test "$ac_enable_aio" = yes; then
  BEE_WORKING_AIO
  if test "$bc_cv_aio_works" = yes; then
    AC_DEFINE([ENABLE_AIO],1)
  fi
fi

if test "$ac_with_mtmalloc" = yes; then
  if test "$ac_cv_have_mtmalloc_h" = yes; then
    AC_CHECK_LIB([mtmalloc],[main]) ac_cv_lib_mtmalloc=ac_cv_lib_mtmalloc_main

  fi
fi

case $target_os in
  cygwin* | mingw*)
    AC_CHECK_LIB([winmm],[main]) ac_cv_lib_winmm=ac_cv_lib_winmm_main
    ;;
esac

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE

AH_TEMPLATE([HAVE_INLINE],[.])
if test "$ac_cv_c_inline" != no; then
  AC_DEFINE([HAVE_INLINE],1)
fi

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_CHECK_FUNCS([memset memcmp memmove strcspn strerror strspn])

AH_TEMPLATE([HAVE_NANOSLEEP],[.])
AH_TEMPLATE([HAVE_GETHRTIME],[.])
AH_TEMPLATE([HAVE_GETTIMEOFDAY],[.])
if test "$ac_cv_header_sys_time_h" = yes; then
  AC_CHECK_FUNCS([nanosleep])
  AC_CHECK_FUNCS([gethrtime])
  # gettimeofday detection fails on HP/UX!
  AC_MSG_CHECKING([for gettimeofday])
  AC_TRY_LINK([#include <sys/time.h> ],[
    struct timeval dummy;
    gettimeofday(&dummy, (void*) 0);
    ],[
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_GETTIMEOFDAY],1)
    ac_cv_func_gettimeofday=yes
    ],[
    AC_MSG_RESULT([no])
    AC_DEFINE([HAVE_GETTIMEOFDAY],0)
    ac_cv_func_gettimeofday=no
    ])
fi

# Predefines and checks for C++ API support
AH_TEMPLATE([CPPGLUE],[Define to 1 if you want to include the C++ code])

if test "$ac_with_cplusplus" = yes; then
  AC_MSG_CHECKING([for IBM's ICU library version >= 2.8])
  AC_LANG_PUSH(C)
  AC_RUN_IFELSE([
    AC_LANG_PROGRAM([[#include <unicode/uversion.h>]],[[
      #if U_ICU_VERSION_MAJOR_NUM < 2
      exit(1);
      #elif U_ICU_VERSION_MAJOR_NUM == 2
      # if U_ICU_VERSION_MINOR_NUM < 8
      exit(1);
      # else
      exit(0);
      # endif
      #else
      exit(0);
      #endif
    ]])],[
    AC_MSG_RESULT([yes])
    ],[
    AC_MSG_RESULT([no])
    AC_MSG_WARN([disabling cplusplus])
    ac_with_cplusplus=no
    ])
  AC_LANG_POP(C)
fi

AM_CONDITIONAL([WITH_CPLUSPLUS],[test "$ac_with_cplusplus" = yes])

if test "$ac_with_cplusplus" = yes ; then
  AC_DEFINE([CPPGLUE],1)
fi


# Predefines and checks for Java API support
AH_TEMPLATE([JAVAGLUE],[Define to 1 if you want to include the Java code])

if test "$ac_with_java" = yes ; then
  AC_CHECK_PROG(ac_cv_have_gcj, gcj, yes, no)
  AC_CHECK_PROG(ac_cv_have_gcjh, gcjh, yes, no)
  if test "$ac_cv_have_gcj" = yes; then
    AC_SUBST(javac,gcj)
    AC_CACHE_CHECK([for java native interface headers], ac_cv_java_include, [
      cat > conftest.java << EOF
public class conftest
{
  public static void main(String[[]] argv)
  {
     System.out.println(System.getProperty("java.home"));
  }
}
EOF
      java_home="`gcj --main=conftest -o conftest conftest.java; ./conftest`"
      if test X"$java_home" = X; then
        java_home=/usr
      fi
      if test -d "$java_home" -a -d "$java_home/include"; then
        ac_cv_java_headers=yes
        ac_cv_java_include="-I$java_home/include"
        gcjpath="$java_home/lib/gcc-lib/`gcj -dumpmachine`/`gcj -dumpversion`"
        if test -d "$gcjpath" -a -d "$gcjpath/include"; then
          ac_cv_java_include="$ac_cv_java_include -I$gcjpath/include"
        fi
      else
        # we have a non-working gcj
        ac_cv_have_gcj=no
      fi
      rm -fr conftest*
    ])
  fi

  # gcj may have failed; in this case we want to try for a real java
  if test "$ac_cv_have_gcj" != yes; then
    AC_CHECK_PROG(ac_cv_have_java, java, yes, no)
    if test "$ac_cv_have_java" = yes; then
      AC_CHECK_PROG(ac_cv_have_javac, javac, yes, no)
      AC_CHECK_PROG(ac_cv_have_javah, javah, yes, no)
      if test "$ac_cv_have_javac" = yes; then
        AC_SUBST(javac,javac)
        AC_CACHE_CHECK([for java native interface headers],ac_cv_java_include,[
          cat > conftest.java << EOF
public class conftest
{
  public static void main(String[[]] argv)
  {
     System.out.println(System.getProperty("java.home"));
  }
}
EOF
          java_home=`javac conftest.java; java -classpath . conftest`
          case $target_os in
          cygwin*)
            java_home=`cygpath -u -p "$java_home"` ;;
          esac
          if test -d "$java_home"; then
            case $target_os in
            darwin*)
              java_include="$java_home/../../../Headers" ;;
            *)
              java_include="$java_home"/../include ;;
            esac
            if test -d "$java_include"; then
              ac_cv_java_headers=yes
              ac_cv_java_include="-I$java_include"
              case $target_os in
              aix*)
                ac_cv_java_include="-I$java_include -I$java_include/aix" ;;
              cygwin*)
                ac_cv_java_include="-I$java_include -I$java_include/win32" ;;
              darwin*) ;;
              hpux*)
                ac_cv_java_include="-I$java_include -I$java_include/hpux" ;;
              linux*)
                ac_cv_java_include="-I$java_include -I$java_include/linux" ;;
              osf*)
                ac_cv_java_include="-I$java_include -I$java_include/osf" ;;
              solaris*)
                ac_cv_java_include="-I$java_include -I$java_include/solaris" ;;
              *)
                AC_MSG_WARN([please add appropriate -I$java_include/<operating system> flag])
                ac_cv_java_include="-I$java_include" ;;
              esac
            else
              AC_MSG_WARN([java headers not found, disabling java])
              ac_cv_java_headers=no
              ac_cv_java_include=
              ac_with_java=no
            fi
          fi
          rm -fr conftest*
        ])
      else
        AC_MSG_WARN([javac not found, disabling java])
        ac_cv_java_headers=no
        ac_cv_java_include=
        ac_with_java=no
      fi
    else
      AC_MSG_WARN([java not found, disabling java])
      ac_cv_java_headers=no
      ac_with_java=no
    fi
  fi
fi

AM_CONDITIONAL([WITH_JAVA],[test "$ac_with_java" = yes])

if test "$ac_with_java" = yes ; then
  AC_DEFINE([JAVAGLUE],1)
  CPPFLAGS="$CPPFLAGS $ac_cv_java_include"
fi


# Predefines and checks for Python API support
AH_TEMPLATE([PYTHONGLUE],[Define to 1 if you want to include the Python code])

if test "$ac_with_python" = yes ; then
  AC_PATH_PROG([PYTHON], `basename $ac_with_python_val`, [no], `AS_DIRNAME(["$ac_with_python_val"])`)
  if test "$PYTHON" = no; then
    ac_with_python=no
  else
    AC_CACHE_CHECK([for python headers], ac_cv_python_include, [
      ac_cv_python_include="-I`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_inc()'`"
    ])
    AC_CACHE_CHECK([where to install python libraries], ac_cv_python_libdir, [
      ac_cv_python_libdir=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_lib()'`
      ac_cv_python_libdir=`echo $ac_cv_python_libdir | sed -e "s,${prefix}/lib,${libdir},"`
    ])
  fi
fi

AM_CONDITIONAL([WITH_PYTHON],[test "$ac_with_python" = yes])

if test "$ac_with_python" = yes; then
  AC_DEFINE([PYTHONGLUE],1)
  AC_SUBST(PYTHONINC,$ac_cv_python_include)
  AC_SUBST(PYTHONLIB,$ac_cv_python_libdir)
fi


# Checks for entropy sources.
AC_MSG_CHECKING([for platform-specific entropy devices])
AC_MSG_RESULT()
case $target_os in
cygwin* | mingw*)
  AC_MSG_CHECKING([for wavein])
  AC_MSG_RESULT(yes)
  AC_MSG_CHECKING([for wincrypt])
  AC_MSG_RESULT(yes)
  AC_MSG_CHECKING([for console])
  AC_MSG_RESULT(yes)
  ;;
linux*)
  AC_CACHE_CHECK([for /dev/dsp],ac_cv_have_dev_dsp,[
    if test -r /dev/dsp; then
      ac_cv_have_dev_dsp=yes
    else
      ac_cv_have_dev_dsp=no
    fi
    ])
  if test "$ac_cv_have_dev_dsp" = yes; then
    AC_DEFINE([HAVE_DEV_DSP], 1)
  fi
  ;;
solaris*)
  AC_CACHE_CHECK([for /dev/audio],ac_cv_have_dev_audio,[
    if test -r /dev/audio; then
      ac_cv_have_dev_audio=yes
    else
      ac_cv_have_dev_audio=no
    fi
    ])
  if test "$ac_cv_have_dev_audio" = yes; then
    AC_DEFINE([HAVE_DEV_AUDIO], 1)
  fi
  ;;
*)
  AC_MSG_WARN(no specific entropy devices present)
  ;;
esac

case $target_os in
cygwin* | mingw*)
  ;;
*)
  AC_MSG_CHECKING([for generic entropy devices])
  AC_MSG_RESULT()
  AC_CACHE_CHECK([for /dev/random],ac_cv_have_dev_random,[
    if test -r /dev/random; then
      ac_cv_have_dev_random=yes
    else
      ac_cv_have_dev_random=no
    fi
    ])
  AC_CACHE_CHECK([for /dev/urandom],ac_cv_have_dev_urandom,[
    if test -r /dev/urandom; then
      ac_cv_have_dev_urandom=yes
    else
      ac_cv_have_dev_urandom=no
    fi
    ])
  AC_CACHE_CHECK([for /dev/tty],ac_cv_have_dev_tty,[
    if test -r /dev/tty; then
      ac_cv_have_dev_tty=yes
    else
      ac_cv_have_dev_tty=no
    fi
    ])
  ;;
esac

if test "$ac_cv_have_dev_random" = yes; then
  AC_DEFINE([HAVE_DEV_RANDOM],1)
fi
if test "$ac_cv_have_dev_urandom" = yes; then
  AC_DEFINE([HAVE_DEV_URANDOM],1)
fi
if test "$ac_cv_have_dev_tty" = yes; then
  AC_DEFINE([HAVE_DEV_TTY],1)
fi

# Figure out extra flags
if test "$ac_enable_debug" != yes; then
  case $bc_target_arch in
  alpha*)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_ALPHA"
    ;;
  arm*)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_ARM"
    ;;
  x86_64 | athlon64 | athlon-fx | k8 | opteron | em64t | nocona)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_X86_64"
    ;;
  athlon*)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I686 -DOPTIMIZE_MMX"
    ;;
  i386)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I386"
    ;;
  i486)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I486"
    ;;
  i586)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I586"
    ;;
  i686)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I686"
    ;;
  ia64)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_IA64"
    ;;
  m68k)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_M68K"
    ;;
  pentium)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I586"
    ;;
  pentium-m)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I686 -DOPTIMIZE_MMX -DOPTIMIZE_SSE -DOPTIMIZE_SSE2"
    ;;
  pentium-mmx)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I586 -DOPTIMIZE_MMX"
    ;;
  pentiumpro)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I686"
    ;;
  pentium2)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I686 -DOPTIMIZE_MMX"
    ;;
  pentium3)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I686 -DOPTIMIZE_MMX -DOPTIMIZE_SSE"
    ;;
  pentium4)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_I686 -DOPTIMIZE_MMX -DOPTIMIZE_SSE -DOPTIMIZE_SSE2"
    ;;
  powerpc)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_POWERPC"
    ;;
  powerpc64)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_POWERPC64"
    ;;
  s390x)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_S390X"
    ;;
  sparcv8)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_SPARCV8"
    ;;
  sparcv8plus*)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_SPARCV8PLUS"
    ;;
  sparcv9*)
    CPPFLAGS="$CPPFLAGS -DOPTIMIZE_SPARCV9"
    ;;
  esac
fi

if test "$ac_enable_debug" != yes; then
  # find out how to use assembler
  BEE_ASM_DEFS
  BEE_ASM_TEXTSEG
  BEE_ASM_GLOBL
  BEE_ASM_GSYM_PREFIX
  BEE_ASM_LSYM_PREFIX
  BEE_ASM_ALIGN
fi

# generate assembler source files from m4 files
BEE_ASM_SOURCES

# Check for standard types and integers of specific sizes
BEE_INT_TYPES
BEE_CPU_BITS

# Generate output files.
AC_CONFIG_FILES([
    Makefile
    config.m4
	include/Makefile
	include/beecrypt/Doxyfile
	include/beecrypt/c++/Doxyfile
    c++/Makefile
    c++/beeyond/Makefile
    c++/crypto/Makefile
    c++/crypto/spec/Makefile
    c++/io/Makefile
    c++/lang/Makefile
    c++/math/Makefile
    c++/nio/Makefile
    c++/provider/Makefile
    c++/security/Makefile
	c++/security/auth/Makefile
    c++/security/cert/Makefile
    c++/security/spec/Makefile
    c++/util/Makefile
    c++/util/concurrent/Makefile
    c++/util/concurrent/locks/Makefile
    docs/Makefile
    gas/Makefile
    java/Makefile
    java/build.xml
    masm/Makefile
    python/Makefile
    tests/Makefile
  ])
#   python/test/Makefile

AC_CONFIG_FILES([gnu.h],[
    cp gnu.h $ac_top_srcdir/include/beecrypt/gnu.h
  ])
AC_OUTPUT
