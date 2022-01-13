dnl  BeeCrypt specific autoconf macros

dnl  Copyright (c) 2003, 2004, 2005, 2006  Bob Deblier <bob.deblier@telenet.be>
dnl
dnl  This file is part of the BeeCrypt crypto library
dnl  
dnl
dnl  LGPL


dnl  BEE_EXPERT_MODE
AC_DEFUN([BEE_EXPERT_MODE],[
  # try to get the architecture from CFLAGS
  bc_target_arch=`echo $CFLAGS | awk '{for (i=1; i<=NF; i++) if (substr($i,0,7)=="-march=") print substr($i,8)}'`
  # examine the other flags
  for flag in `echo $CFLAGS`
  do
    case $flag in
    -mmmx)
       CPPFLAGS="$CPPFLAGS -DOPTIMIZE_MMX"
       ;;
    -msse)
       CPPFLAGS="$CPPFLAGS -DOPTIMIZE_SSE"
       ;;
    -msse2)
       CPPFLAGS="$CPPFLAGS -DOPTIMIZE_SSE2"
       ;;
    -msse3)
       CPPFLAGS="$CPPFLAGS -DOPTIMIZE_SSE3"
       ;;
     esac
  done
  ])


dnl  BEE_WITH_CPU
AC_DEFUN([BEE_WITH_CPU],[
  ac_with_cpu=yes
  bc_target_cpu=$withval
  case $target_cpu in
  i[[3456]]86)
    case $withval in
    i[[3456]]86 | \
    pentium | pentium-m | pentium-mmx | pentiumpro | pentium[[234]] | \
    athlon | athlon-tbird | athlon-4 | athlon-xp | athlon-mp | athlon-fx | athlon64 | k8)
      ;;
    *)
      AC_MSG_WARN([invalid cpu type])
      bc_target_cpu=$target_cpu
      ;;
    esac
    ;;
  powerpc | powerpc64)
    case $withval in
    403 | 505 | \
    60[[1234]] | 60[[34]]e | 6[[23]]0 | \
    7[[45]]0 | 74[[05]]0 | \
    801 | 82[[13]] | 860 | \
    power | power2 | powerpc | powerpc64)
      ;;
    *)
      AC_MSG_WARN([invalid cpu type])
      bc_target_cpu=$target_cpu
      ;;
    esac
    ;;
  sparc | sparc64)
    case $withval in
    sparcv8 | sparcv8plus | sparcv8plusa | sparcv9 | sparcv9a)
      ;;
    *)
      AC_MSG_WARN([invalid cpu type])
      bc_target_cpu=$target_cpu
      ;;
    esac
    ;;
  x86) # QNX Neutrino doesn't list the exact cpu type
    case $withval in
    i[[3456]]86)
      ;;
    *)
      AC_MSG_WARN([unsupported or invalid cpu type])
      bc_target_cpu=$target_cpu
      ;;
    esac
    ;;
  *)
    AC_MSG_WARN([unsupported or invalid cpu type])
    bc_target_cpu=$target_cpu
    ;;
  esac
  ])


dnl  BEE_WITHOUT_CPU
AC_DEFUN([BEE_WITHOUT_CPU],[
  ac_with_cpu=no
  bc_target_cpu=$target_cpu
  ])


dnl  BEE_WITH_ARCH
AC_DEFUN([BEE_WITH_ARCH],[
  ac_with_arch=yes
  bc_target_arch=$withval
  case $target_cpu in
  i[[3456]]86)
    case $withval in
    em64t | \
    i[[3456]]86 | \
    pentium | pentium-m | pentium-mmx | pentiumpro | pentium[[234]] | \
    athlon | athlon-tbird | athlon-4 | athlon-xp | athlon-mp | athlon64 | k8)
      if test "$ac_with_cpu" != yes; then
        bc_target_arch=$withval
      fi
      ;;
    esac
    ;;
  powerpc*)
    case $withval in
    powerpc)
      ;;
    powerpc64)
      ;;
    *)
      AC_MSG_WARN([unsupported on invalid arch type])
      bc_target_arch=powerpc
      ;;
    esac
    ;;
  esac
  ])

dnl  BEE_WITHOUT_ARCH
AC_DEFUN([BEE_WITHOUT_ARCH],[
  # if bc_target_arch hasn't been set (i.e. by expert mode)
  if test "X$bc_target_arch" = "X"; then
    ac_with_arch=no
    case $target_cpu in
    alpha*)
      bc_target_arch=alpha
      ;;
    arm*)
      bc_target_arch=arm
      ;;
    i[[3456]]86)
      bc_target_arch=i386
      ;;
    ia64)
      bc_target_arch=ia64
      ;;
    m68k)
      bc_target_arch=m68k
      ;;
    powerpc)
      bc_target_arch=powerpc
      ;;
    powerpc64)
      bc_target_arch=powerpc64
      ;;
    s390x)
      bc_target_arch=s390x
      ;;
    sparc*)
      bc_target_arch=sparc
      ;;
    x86_64)
      bc_target_arch=x86_64
      ;;
    esac
  fi
  ])


dnl  BEE_INT_TYPES
AC_DEFUN([BEE_INT_TYPES],[
  AC_TYPE_SIZE_T
  bc_typedef_size_t=
  if test $ac_cv_type_size_t != yes; then
    bc_typedef_size_t="typedef unsigned size_t;"
  else
    AC_CHECK_SIZEOF([size_t])
  fi
  AC_SUBST(TYPEDEF_SIZE_T,$bc_typedef_size_t)
  if test $ac_cv_header_inttypes_h = yes; then
    AC_SUBST(INCLUDE_INTTYPES_H,["#include <inttypes.h>"])
  else
    AC_SUBST(INCLUDE_INTTYPES_H,[ ])
  fi
  if test $ac_cv_header_stdint_h = yes; then
    AC_SUBST(INCLUDE_STDINT_H,["#include <stdint.h>"])
  else
    AC_SUBST(INCLUDE_STDINT_H,[ ])
  fi
  AH_TEMPLATE([HAVE_INT64_T])
  AH_TEMPLATE([HAVE_UINT64_T])
  bc_typedef_int8_t=
  AC_CHECK_TYPE([int8_t],,[
    AC_CHECK_SIZEOF([signed char])
    if test $ac_cv_sizeof_signed_char -eq 1; then
      bc_typedef_int8_t="typedef signed char int8_t;"
    fi
    ])
  AC_SUBST(TYPEDEF_INT8_T,$bc_typedef_int8_t)
  bc_typedef_int16_t=
  AC_CHECK_TYPE([int16_t],,[
    AC_CHECK_SIZEOF([short])
    if test $ac_cv_sizeof_short -eq 2; then
      bc_typedef_int16_t="typedef short int16_t;"
    fi
    ])
  AC_SUBST(TYPEDEF_INT16_T,$bc_typedef_int16_t)
  bc_typedef_int32_t=
  AC_CHECK_TYPE([int32_t],,[
    AC_CHECK_SIZEOF([int])
    if test $ac_cv_sizeof_int -eq 4; then
      bc_typedef_int32_t="typedef int int32_t;"
    fi
    ])
  AC_SUBST(TYPEDEF_INT32_T,$bc_typedef_int32_t)
  bc_typedef_int64_t=
  AC_CHECK_TYPE([int64_t],[
    AC_DEFINE([HAVE_INT64_T],1)
    ],[
    AC_CHECK_SIZEOF([long])
    if test $ac_cv_sizeof_long -eq 8; then
      bc_typedef_int64_t="typedef long int64_t;"
    else
      AC_CHECK_SIZEOF([long long])
      if test $ac_cv_sizeof_long_long -eq 8; then
        AC_DEFINE([HAVE_INT64_T],1)
        bc_typedef_int64_t="typedef long long int64_t;"
      fi
    fi
    ])
  AC_SUBST(TYPEDEF_INT64_T,$bc_typedef_int64_t)
  bc_typedef_uint8_t=
  AC_CHECK_TYPE([uint8_t],,[
    AC_CHECK_SIZEOF([unsigned char])
    if test $ac_cv_sizeof_unsigned_char -eq 1; then
      bc_typedef_uint8_t="typedef unsigned char uint8_t;"
    fi
    ])
  AC_SUBST(TYPEDEF_UINT8_T,$bc_typedef_uint8_t)
  bc_typedef_uint16_t=
  AC_CHECK_TYPE([uint16_t],,[
    AC_CHECK_SIZEOF([unsigned short])
    if test $ac_cv_sizeof_unsigned_short -eq 2; then
      bc_typedef_uint16_t="typedef unsigned short uint16_t;"
    fi
    ])
  AC_SUBST(TYPEDEF_UINT16_T,$bc_typedef_uint16_t)
  bc_typedef_uint32_t=
  AC_CHECK_TYPE([uint32_t],,[
    AC_CHECK_SIZEOF([unsigned int])
    if test $ac_cv_sizeof_unsigned_int -eq 4; then
      bc_typedef_uint32_t="typedef unsigned int uint32_t;"
    fi
    ])
  AC_SUBST(TYPEDEF_UINT32_T,$bc_typedef_uint32_t)
  bc_typedef_uint64_t=
  AC_CHECK_TYPE([uint64_t],[
    AC_DEFINE([HAVE_UINT64_T],1)
    ],[
    AC_CHECK_SIZEOF([unsigned long])
    if test $ac_cv_sizeof_unsigned_long -eq 8; then
      bc_typedef_uint64_t="typedef unsigned long uint64_t;"
    else
      AC_CHECK_SIZEOF([unsigned long long])
      if test $ac_cv_sizeof_unsigned_long_long -eq 8; then
        AC_DEFINE([HAVE_UINT64_T],1)
        bc_typedef_uint64_t="typedef unsigned long long uint64_t;"
      fi
    fi
    ])
  AC_SUBST(TYPEDEF_UINT64_T,$bc_typedef_uint64_t)
  AH_TEMPLATE([HAVE_LONG_LONG])
  AH_TEMPLATE([HAVE_UNSIGNED_LONG_LONG])
  AC_CHECK_TYPE([long long],[
    AC_DEFINE([HAVE_LONG_LONG],1)
    ],[
    AC_DEFINE([HAVE_LONG_LONG],0)
    ])
  AC_CHECK_TYPE([unsigned long long],[
    AC_DEFINE([HAVE_UNSIGNED_LONG_LONG],1)
    ],[
    AC_DEFINE([HAVE_UNSIGNED_LONG_LONG],0)
    ])
  ])


dnl  BEE_CPU_BITS
AC_DEFUN([BEE_CPU_BITS],[
  AC_CHECK_SIZEOF([unsigned long])
  if test $ac_cv_sizeof_unsigned_long -eq 8; then
    AC_SUBST(MP_WBITS,64U)
  elif test $ac_cv_sizeof_unsigned_long -eq 4; then
    AC_SUBST(MP_WBITS,32U)
  else
    AC_MSG_ERROR([Illegal CPU word size])
  fi
  ])


dnl  BEE_WORKING_AIO
AC_DEFUN([BEE_WORKING_AIO],[
  AC_CHECK_HEADERS(aio.h)
  if test "$ac_cv_header_aio_h" = yes; then
    AC_SEARCH_LIBS([aio_read],[c rt aio posix4],[
      AC_CACHE_CHECK([whether aio works],bc_cv_working_aio,[
        cat > conftest.aio << EOF
The quick brown fox jumps over the lazy dog.
EOF
        AC_LANG_PUSH(C)
        AC_RUN_IFELSE([
          AC_LANG_PROGRAM([[
            #if HAVE_ERRNO_H
            # include <errno.h>
            #endif
            #if HAVE_FCNTL_H
            # include <fcntl.h>
            #endif
            #if HAVE_STRING_H
            # include <string.h>
            #endif
            #if HAVE_UNISTD_H
            # include <unistd.h>
            #endif
            #include <stdio.h>
            #include <aio.h>
            ]],[[
                    struct aiocb    a;
              const struct aiocb*   a_list = &a;
                    struct timespec a_timeout;

              char buffer[32];

              int i, rc, fd = open("conftest.aio", O_RDONLY);

              if (fd < 0)
                exit(1);

              memset(&a, 0, sizeof(struct aiocb));

              a.aio_fildes = fd;
              a.aio_offset = 0;
              a.aio_reqprio = 0;
              a.aio_buf = buffer;
              a.aio_nbytes = sizeof(buffer);
              a.aio_sigevent.sigev_notify = SIGEV_NONE;

              a_timeout.tv_sec = 1;
              a_timeout.tv_nsec = 0;

              if (aio_read(&a) < 0)
              {
                perror("aio_read");
                exit(1);
              }
              if (aio_suspend(&a_list, 1, &a_timeout) < 0)
              {
                #if HAVE_ERRNO_H
                /* some linux systems don't await timeout and return instantly */
                if (errno == EAGAIN)
                {
                  nanosleep(&a_timeout, (struct timespec*) 0);

                  if (aio_suspend(&a_list, 1, &a_timeout) < 0)
                  {
                    perror("aio_suspend");
                    exit(1);
                  }
                }
                else
                {
                  perror("aio_suspend");
                  exit(1);
                }
                #else
                exit(1);
                #endif
              }
              if (aio_error(&a) < 0)
                exit(1);

              if (aio_return(&a) < 0)
                exit(1);

              exit(0);
          ]])],[bc_cv_working_aio=yes],[bc_cv_working_aio=no],[
            case $target_os in
              linux* | solaris*)
                bc_cv_working_aio=yes ;;
              *)
                bc_cv_working_aio=no ;;
            esac
          ])
        ],[
        bc_cv_working_aio=no
        ])
        AC_LANG_POP(C)
      ])
    rm -fr conftest.aio
  fi
  ])


dnl  BEE_AGGRESSIVE_OPT
AC_DEFUN([BEE_AGGRESSIVE_OPT],[
  if test "$CFLAGS" = ""; then
    bc_cv_c_aggressive_opt=yes
  else
    bc_cv_c_aggressive_opt=no
  fi
  if test "$CXXFLAGS" = ""; then
    bc_cv_cxx_aggressive_opt=yes
  else
    bc_cv_cxx_aggressive_opt=no
  fi
  ])

dnl  BEE_CFLAGS_REM
AC_DEFUN([BEE_CFLAGS_REM],[
  if test "$CFLAGS" != ""; then
    CFLAGS_save=""
    for flag in $CFLAGS
    do
      if test "$flag" != "$1"; then
        CFLAGS_save="$CFLAGS_save $flag"
      fi
    done
    CFLAGS="$CFLAGS_save"
  fi
  ])


dnl  BEE_CXXFLAGS_REM
AC_DEFUN([BEE_CXXFLAGS_REM],[
  if test "$CXXFLAGS" != ""; then
    CXXFLAGS_save=""
    for flag in $CXXFLAGS
    do
      if test "$flag" != "$1"; then
        CXXFLAGS_save="$CXXFLAGS_save $flag"
      fi
    done
    CXXFLAGS="$CXXFLAGS_save"
  fi
  ])


dnl  BEE_GNU_CC_MTUNE
AC_DEFUN([BEE_GNU_CC_MTUNE],[
  AC_REQUIRE([AC_PROG_CC])
  case $bc_target_arch in
  i[[3456]]86 | \
  pentium | pentium-m | pentium-mmx | pentiumpro | pentium[[234]] | \
  athlon | athlon-tbird | athlon-4 | athlon-xp | athlon-mp)
    AC_MSG_CHECKING([if gcc supports -mtune option])
    AC_LANG_PUSH(C)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[][int x;]])],[
      bc_cv_gcc_mtune=yes
      ],[
      bc_cv_gcc_mtune=no
      ])
    AC_LANG_POP(C)
    AC_MSG_RESULT([$bc_cv_gcc_mtune])
    ;;
  esac
  ])


dnl  BEE_GNU_CXX_MTUNE
AC_DEFUN([BEE_GNU_CXX_MTUNE],[
  AC_REQUIRE([AC_PROG_CXX])
  case $bc_target_arch in
  em64t | \
  i[[3456]]86 | \
  pentium | pentium-m | pentium-mmx | pentiumpro | pentium[[234]] | \
  athlon | athlon-tbird | athlon-4 | athlon-xp | athlon-mp | athlon64 | k8)
    AC_MSG_CHECKING([if g++ supports -mtune option])
    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[][int x;]])],[
      bc_cv_gxx_mtune=yes
      ],[
      bc_cv_gxx_mtune=no
      ])
    AC_LANG_POP(C++)
    ;;
  esac
  ])


dnl  BEE_GNU_CC
AC_DEFUN([BEE_GNU_CC],[
  AC_REQUIRE([AC_PROG_CC])
  if test "$OPENMP_CFLAGS" != ""; then
    AC_SUBST(OPENMP_LIBS,"-lgomp")
  fi
  case $bc_target_arch in
  x86_64 | athlon64 | athlon-fx | k8 | opteron | em64t | nocona)
    CC="$CC -m64"
    ;;
  i[[3456]]86 | \
  pentium* | \
  athlon*)
    CC="$CC -m32"
    CCAS="$CCAS -m32"
    ;;
  ia64)
    case $target_os in
    # HP/UX on Itanium needs to be told that a long is 64-bit!
    hpux*)
      CFLAGS="$CFLAGS -mlp64"
      ;;
    esac
    ;;
  # PowerPC needs a signed char
  powerpc)
    CFLAGS="$CFLAGS -fsigned-char"
    ;;
  powerpc64)
    CFLAGS="$CFLAGS -fsigned-char"
    case $target_os in
    aix*)
      CC="$CC -maix64"
      ;;
    linux*)
      CC="$CC -m64"
      ;;
    esac
    ;;
  sparc | sparcv8*)
    CC="$CC -m32"
    ;;
  sparc64 | sparcv9*)
    CC="$CC -m64"
    ;;
  esac
  # Certain platforms needs special flags for multi-threaded code
  if test "$ac_enable_threads" = yes; then
    case $target_os in
    freebsd*)
      CFLAGS="$CFLAGS -pthread"
      CPPFLAGS="$CPPFLAGS -pthread"
      LDFLAGS="$LDFLAGS -pthread"
      ;;
    osf*)
      CFLAGS="$CFLAGS -pthread"
      CPPFLAGS="$CPPFLAGS -pthread"
      ;;
    esac
  fi
  if test "$ac_enable_debug" = yes; then
    BEE_CFLAGS_REM([-O2])
    CFLAGS="$CFLAGS -Wall -pedantic"
  else
    BEE_GNU_CC_MTUNE
    # Generic optimizations, including cpu tuning
    BEE_CFLAGS_REM([-g])
    CFLAGS="$CFLAGS -DNDEBUG"
    if test "$bc_cv_c_aggressive_opt" = yes; then
      case $bc_target_cpu in
      athlon64 | athlon-fx | k8 | opteron)
        # CFLAGS="$CFLAGS -mtune=k8"
        # -O3 degrades performance
        # -mcpu=athlon64 degrades performance
        ;;
      alpha*)
        BEE_CFLAGS_REM([-O2])
        CFLAGS="$CFLAGS -O3"
        ;;
      athlon*)
        BEE_CFLAGS_REM([-O2])
        if test "$bc_cv_gcc_mtune" = yes; then
          CFLAGS="$CFLAGS -O3 -mtune=pentiumpro"
        else
          CFLAGS="$CFLAGS -O3 -mcpu=pentiumpro"
        fi
        ;;
      i586)
        BEE_CFLAGS_REM([-O2])
        if test "$bc_cv_gcc_mtune" = yes; then
          CFLAGS="$CFLAGS -O3 -mtune=pentium"
        else
          CFLAGS="$CFLAGS -O3 -mcpu=pentium"
        fi
        ;;
      i686)
        BEE_CFLAGS_REM([-O2])
        if test "$bc_cv_gcc_mtune" = yes; then
          CFLAGS="$CFLAGS -O3 -mtune=pentiumpro"
        else
          CFLAGS="$CFLAGS -O3 -mcpu=pentiumpro"
        fi
        ;;
      ia64)
        # no -mcpu=... option on ia64
        ;;
      pentium*)
        BEE_CFLAGS_REM([-O2])
        if test "$bc_cv_gcc_mtune" = yes; then
          CFLAGS="$CFLAGS -O3 -mtune=$bc_target_arch"
        else
          CFLAGS="$CFLAGS -O3 -mcpu=$bc_target_arch"
        fi
        ;;
      powerpc*)
        ;;
      esac
      # Architecture-specific optimizations
      case $bc_target_arch in
      athlon64)
        CFLAGS="$CFLAGS -march=k8"
        # -march=athlon64 degrades performance
        # -msse2 also doesn't help
        ;;
      athlon*)
        CFLAGS="$CFLAGS -march=$bc_target_arch -mmmx"
        ;;
      em64t)
        CFLAGS="$CFLAGS -march=em64t"
        ;;
      i586 | pentium)
        CFLAGS="$CFLAGS -march=pentium"
        ;;
      i686 | pentiumpro)
        CFLAGS="$CFLAGS -march=pentiumpro"
        ;;
      pentium-m)
        CFLAGS="$CFLAGS -march=pentium-m -msse2"
        ;;
      pentium-mmx)
        CFLAGS="$CFLAGS -march=pentium-mmx -mmmx"
        ;;
      pentium2)
        CFLAGS="$CFLAGS -march=pentium2 -mmmx"
        ;;
      pentium3)
        CFLAGS="$CFLAGS -march=pentium3 -msse"
        ;;
      pentium4)
        CFLAGS="$CFLAGS -march=pentium4 -msse2"
        ;;
      powerpc | powerpc64)
        CFLAGS="$CFLAGS -mcpu=$bc_target_arch"
        ;;
      sparcv8)
        CFLAGS="$CFLAGS -mcpu=v8"
        ;;
      sparcv8plus)
        CFLAGS="$CFLAGS -mcpu=v9"
        ;;
      sparcv8plusa | sparcv9 | sparcv9a)
        CFLAGS="$CFLAGS -mcpu=ultrasparc"
        ;;
      esac
    fi
  fi
  ])


dnl  BEE_GNU_CXX
AC_DEFUN([BEE_GNU_CXX],[
  AC_REQUIRE([AC_PROG_CXX])
  case $bc_target_arch in
  x86_64 | athlon64 | athlon-fx | k8 | opteron | em64t | nocona | core2)
    CXX="$CXX -m64"
    ;;
  i[[3456]]86 | \
  pentium* | \
  athlon*)
    CXX="$CXX -m32"
    ;;
  ia64)
    case $target_os in
    # HP/UX on Itanium needs to be told that a long is 64-bit!
    hpux*)
      CXXFLAGS="$CXXFLAGS -mlp64"
      ;;
    esac
    ;;
  # PowerPC needs a signed char
  powerpc)
    CXXFLAGS="$CXXFLAGS -fsigned-char"
    ;;
  powerpc64)
    CXXFLAGS="$CXXFLAGS -fsigned-char"
    case $target_os in
    aix*)
      CXX="$CXX -maix64"
      ;;
    linux*)
      CXX="$CXX -m64"
      ;;
    esac
    ;;
  sparc | sparcv8*)
    CXX="$CXX -m32"
    ;;
  sparc64 | sparcv9*)
    CXX="$CXX -m64"
    ;;
  esac
  # Certain platforms needs special flags for multi-threaded code
  if test "$ac_enable_threads" = yes; then
    case $target_os in
    freebsd*)
      CXXFLAGS="$CXXFLAGS -pthread"
      LDFLAGS="$LDFLAGS -pthread"
      ;;
    osf*)
      CXXFLAGS="$CXXFLAGS -pthread"
      ;;
    esac
  fi
  if test "$ac_enable_debug" = yes; then
    BEE_CXXFLAGS_REM([-O2])
    CXXFLAGS="$CXXFLAGS -Wall -pedantic"
  else
    # Generic optimizations, including cpu tuning
    BEE_GNU_CXX_MTUNE
    BEE_CXXFLAGS_REM([-g])
    CXXFLAGS="$CXXFLAGS -DNDEBUG"
    if test "$bc_cv_c_aggressive_opt" = yes; then
      case $bc_target_cpu in
      athlon*)
        CXXFLAGS="$CXXFLAGS -mcpu=pentiumpro";
        ;;
      i586)
        CXXFLAGS="$CXXFLAGS -mcpu=pentium"
        ;;
      i686)
        CXXFLAGS="$CXXFLAGS -mcpu=pentiumpro"
        ;;
      ia64)
        # no -mcpu=... option on ia64
        ;;
      pentium*)
        CXXFLAGS="$CXXFLAGS -mcpu=$bc_target_arch"
        ;;
      esac
      # Architecture-specific optimizations
      case $bc_target_arch in
      athlon*)
        CXXFLAGS="$CXXFLAGS -march=$bc_target_arch"
        ;;
      i586)
        CXXFLAGS="$CXXFLAGS -march=pentium"
        ;;
      i686)
        CXXFLAGS="$CXXFLAGS -march=pentiumpro"
        ;;
      pentium*)
        CXXFLAGS="$CXXFLAGS -march=$bc_target_arch"
        ;;
      powerpc | powerpc64)
        CXXFLAGS="$CXXFLAGS -mcpu=$bc_target_arch"
        ;;
      sparcv8)
        CXXFLAGS="$CXXFLAGS -mcpu=v8"
        ;;
      sparcv8plus)
        CXXFLAGS="$CXXFLAGS -mcpu=v9"
        ;;
      esac
    fi
  fi
  ])


dnl  BEE_COMPAQ_CC
AC_DEFUN([BEE_COMPAQ_CC],[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PROG_CPP])
  AC_CACHE_CHECK([whether we are using Compaq's C compiler],bc_cv_prog_COMPAQ_CC,[
    AC_EGREP_CPP(yes,[
      #ifdef __DECC
        yes;
      #endif
      ],bc_cv_prog_COMPAQ_CC=yes,bc_cv_prog_COMPAQ_CC=no)
    ])
  if test "$bc_cv_prog_COMPAQ_CC" = yes; then
    if test "$ac_enable_threads" = yes; then
      CFLAGS="$CFLAGS -pthread"
      CPPFLAGS="$CPPFLAGS -pthread"
    fi
    if test "$ac_enable_debug" != yes; then
      BEE_CFLAGS_REM([-g])
      if test "$bc_cv_c_aggressive_opt" = yes; then
        CFLAGS="$CFLAGS -fast"
      fi
    fi
  fi
  ])


dnl  BEE_COMPAQ_CXX
AC_DEFUN([BEE_COMPAQ_CXX],[
  ])


dnl  BEE_HPUX_CC
AC_DEFUN([BEE_HPUX_CC],[
  if test "$ac_enable_debug" != yes; then
    BEE_CFLAGS_REM([-g])
    if test "$bc_cv_c_aggressive_opt" = yes; then
      CFLAGS="$CFLAGS -fast"
    fi
  fi
  ])


dnl  BEE_HP_CXX
AC_DEFUN([BEE_HP_CXX],[
  ])


dnl  BEE_IBM_CC
AC_DEFUN([BEE_IBM_CC],[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PROG_CPP])
  AC_CACHE_CHECK([whether we are using IBM C],bc_cv_prog_IBM_CC,[
    AC_EGREP_CPP(yes,[
      #ifdef __IBMC__
        yes;
      #endif
      ],bc_cv_prog_IBM_CC=yes,bc_cv_prog_IBM_CC=no)
    ])
  if test "$bc_cv_prog_IBM_CC" = yes; then
    case $bc_target_arch in
    powerpc)
      CC="$CC -q32 -qarch=ppc"
      ;;
    powerpc64)
      CC="$CC -q64 -qarch=ppc64"
      ;;
    esac
    if test "$ac_enable_debug" != yes; then
      BEE_CFLAGS_REM([-g])
      if test "$bc_cv_c_aggressive_opt" = yes; then
        if test "$ac_with_arch" = yes; then
          CFLAGS="$CFLAGS -O5"
        else
          CFLAGS="$CFLAGS -O3"
        fi
      fi
    fi
    # Version 5.0 doesn't have this, but 6.0 does
    AC_CHECK_FUNC([__rotatel4])
  fi
  ])


dnl  BEE_IBM_CXX
AC_DEFUN([BEE_IBM_CXX],[
  ])


dnl  BEE_INTEL_CC
AC_DEFUN([BEE_INTEL_CC],[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PROG_CPP])
  AC_CACHE_CHECK([whether we are using Intel C],bc_cv_prog_INTEL_CC,[
    AC_EGREP_CPP(yes,[
      #ifdef __INTEL_COMPILER
        yes;
      #endif
      ],bc_cv_prog_INTEL_CC=yes,bc_cv_prog_INTEL_CC=no)
    ])
  if test "$bc_cv_prog_INTEL_CC" = yes; then
    if test "$ac_enable_debug" != yes; then
      BEE_CFLAGS_REM([-g])
      if test "$bc_cv_c_aggressive_opt" = yes; then
        case $bc_target_cpu in
        i586 | pentium | pentium-mmx)
          CFLAGS="$CFLAGS -mcpu=pentium"
          ;;
        i686 | pentiumpro | pentium[[23]])
          CFLAGS="$CFLAGS -mcpu=pentiumpro"
          ;;
        pentium4 | pentium-m)
          CFLAGS="$CFLAGS -mcpu=pentium4"
          ;;
        esac
        case $bc_target_arch in
        i586 | pentium | pentium-mmx)
          CFLAGS="$CFLAGS -tpp5"
          ;;
        i686 | pentiumpro)
          CFLAGS="$CFLAGS -tpp6 -march=pentiumpro"
          ;;
        pentium2)
          CFLAGS="$CFLAGS -tpp6 -march=pentiumii"
          ;;
        pentium3)
          CFLAGS="$CFLAGS -tpp6 -march=pentiumiii"
          ;;
        pentium4 | pentium-m)
          CFLAGS="$CFLAGS -tpp7 -march=pentium4"
          ;;
        esac
      fi
    fi
    AC_CHECK_FUNC([_rotl])
    AC_CHECK_FUNC([_rotr])
    if test "$OPENMP_CFLAGS" != ""; then
      AC_SUBST(OPENMP_LIBS,"-liomp5")
    fi
  fi
  ])


dnl  BEE_INTEL_CXX
AC_DEFUN([BEE_INTEL_CXX],[
  AC_REQUIRE([AC_PROG_CXX])
  AC_REQUIRE([AC_PROG_CPP])
  AC_CACHE_CHECK([whether we are using Intel C++],bc_cv_prog_INTEL_CXX,[
    AC_EGREP_CPP(yes,[
      #ifdef __INTEL_COMPILER
        yes;
      #endif
      ],bc_cv_prog_INTEL_CXX=yes,bc_cv_prog_INTEL_CXX=no)
    ])
  if test "$bc_cv_prog_INTEL_CXX" = yes; then
    if test "$ac_enable_debug" != yes; then
      BEE_CXXFLAGS_REM([-g])
      if test "$bc_cv_c_aggressive_opt" = yes; then
        case $bc_target_cpu in
        i586 | pentium | pentium-mmx)
          CXXFLAGS="$CXXFLAGS -mcpu=pentium"
          ;;
        i686 | pentiumpro | pentium[[23]])
          CXXFLAGS="$CXXFLAGS -mcpu=pentiumpro"
          ;;
        pentium4 | pentium-m)
          CXXFLAGS="$CXXFLAGS -mcpu=pentium4"
          ;;
        esac
        case $bc_target_arch in
        i586 | pentium | pentium-mmx)
          CXXFLAGS="$CXXFLAGS -tpp5"
          ;;
        i686 | pentiumpro)
          CXXFLAGS="$CXXFLAGS -tpp6 -march=pentiumpro"
          ;;
        pentium2)
          CXXFLAGS="$CXXFLAGS -tpp6 -march=pentiumii"
          ;;
        pentium3)
          CXXFLAGS="$CXXFLAGS -tpp6 -march=pentiumiii"
          ;;
        pentium4)
          CXXFLAGS="$CXXFLAGS -tpp7 -march=pentium4"
          ;;
        esac
      fi
    fi
  fi
  ])


dnl  BEE_SUN_FORTE_CC
AC_DEFUN([BEE_SUN_FORTE_CC],[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PROG_CPP])
  AC_CACHE_CHECK([whether we are using Sun Forte C],bc_cv_prog_SUN_FORTE_CC,[
    AC_LANG_PUSH(C)
    AC_RUN_IFELSE([
      AC_LANG_PROGRAM([[
        ]],[[
        #ifdef __SUNPRO_C
        return 0;
        #else
        return 1;
        #endif
        ]])
      ],[bc_cv_prog_SUN_FORTE_CC=yes],[bc_cv_prog_SUN_FORTE_CC=no])
    AC_LANG_POP(C)
    ])
  if test "$bc_cv_prog_SUN_FORTE_CC" = yes; then
    if test "$ac_enable_threads" = yes; then
      CFLAGS="$CFLAGS -mt"
    fi
    if test "$ac_enable_debug" != yes; then
      BEE_CFLAGS_REM([-g])
      if test "$bc_cv_c_aggressive_opt" = yes; then
        CFLAGS="$CFLAGS -fast"
        case $bc_target_arch in
        sparc)
          CFLAGS="$CFLAGS -m32 -xarch=generic"
          ;;
        sparcv8)
          CFLAGS="$CFLAGS -m32 -xarch=v8"
          ;;
        sparcv8plus*)
          CFLAGS="$CFLAGS -m32 -xarch=v8plus"
          ;;
        sparcv9*)
          CFLAGS="$CFLAGS -m64 -xarch=generic"
          ;;
        esac
      fi
    fi
  fi
  ])


dnl  BEE_SUN_FORTE_CXX
AC_DEFUN([BEE_SUN_FORTE_CXX],[
  AC_REQUIRE([AC_PROG_CXX])
  AC_REQUIRE([AC_PROG_CPP])
  AC_CACHE_CHECK([whether we are using Sun Forte C++],bc_cv_prog_SUN_FORTE_CXX,[
    AC_LANG_PUSH(C++)
    AC_RUN_IFELSE([
      AC_LANG_PROGRAM([[
        ]],[[
        #ifdef __SUNPRO_CC
        return 0;
        #else
        return 1;
        #endif
        ]])
      ],[bc_cv_prog_SUN_FORTE_CXX=yes],[bc_cv_prog_SUN_FORTE_CXX=no])
    AC_LANG_POP(C++)
    ])
  if test "$bc_cv_prog_SUN_FORTE_CXX" = yes; then
    if test "$ac_enable_threads" = yes; then
      CXXFLAGS="$CXXFLAGS -mt"
    fi
    if test "$ac_enable_debug" != yes; then
      BEE_CFLAGS_REM([-g])
      if test "$bc_cv_c_aggressive_opt" = yes; then
        CXXFLAGS="$CXXFLAGS -fast"
        case $bc_target_arch in
        sparc)
          CXXFLAGS="$CXXFLAGS -m32 -xarch=generic"
          ;;
        sparcv8)
          CXXFLAGS="$CXXFLAGS -m32 -xarch=v8"
          ;;
        sparcv8plus*)
          CXXFLAGS="$CXXFLAGS -m32 -xarch=v8plus"
          ;;
        sparcv9*)
          CXXFLAGS="$CXXFLAGS -m64 -xarch=generic"
          ;;
        esac
      fi
    fi
  fi
  ])


dnl  BEE_CC
AC_DEFUN([BEE_CC],[
  # set flags for large file support
  case $target_os in
  linux* | solaris*)
    CPPFLAGS="$CPPFLAGS `getconf LFS_CFLAGS`"
    LDFLAGS="$LDFLAGS `getconf LFS_LDFLAGS`"
    ;;
  esac
  if test "$ac_cv_c_compiler_gnu" = yes; then
    # Intel's icc can be mistakenly identified as gcc
    case $target_os in
    linux*)
      BEE_INTEL_CC
      ;;
    esac
    if test "$bc_cv_prog_INTEL_CC" != yes; then
      BEE_GNU_CC
    fi
  else
    case $target_os in
    aix*)
      BEE_IBM_CC
      ;;
    hpux*)
      BEE_HPUX_CC
      ;;
    linux*)
      BEE_INTEL_CC
      ;;
    solaris*)
      BEE_SUN_FORTE_CC
      ;;
    osf*)
      BEE_COMPAQ_CC
      ;;
    esac
  fi
  ])


dnl  BEE_CXX
AC_DEFUN([BEE_CXX],[
  if test "$ac_cv_cxx_compiler_gnu" = yes; then
    # Intel's icc can be mistakenly identified as gcc
    case $target_os in
    linux*)
      BEE_INTEL_CXX
      ;;
    esac
    if test "$bc_cv_prog_INTEL_CXX" != yes; then
      BEE_GNU_CXX
    fi
  else
    case $target_os in
    aix*)
      BEE_IBM_CXX
      ;;
    hpux*)
      BEE_HPUX_CXX
      ;;
    linux*)
      BEE_INTEL_CXX
      ;;
    solaris*)
      BEE_SUN_FORTE_CXX
      ;;
    osf*)
      BEE_COMPAQ_CXX
      ;;
    esac
  fi
  ])


dnl BEE_CC_NOEXECSTACK
AC_DEFUN([BEE_CC_NOEXECSTACK],[
  AC_CACHE_CHECK([whether we can use noexecstack flag in C],bc_cv_cc_noexecstack,[
    CFLAGS_save=$CFLAGS
    if test "$bc_cv_prog_INTEL_CC" = yes; then
      CFLAGS="$CFLAGS -Qoption,asm,--noexecstack"
    else
      CFLAGS="$CFLAGS -Wa,--noexecstack"
    fi
    AC_LANG_PUSH(C)
    # first try to compile it
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[]],[[int x = 0]])
      ],[
      # did compile
      $CC -S $CFLAGS conftest.$ac_ext > /dev/null 2>&1
      $CC -o conftest$ac_exeext $CFLAGS conftest.s > /dev/null 2>&1
      if test $? -eq 0; then
        # did assemble
        bc_cv_cc_noexecstack=yes
        bc_gnu_stack=`$EGREP -e '\.section.*GNU-stack' conftest.s`
      else
        # didn't assemble
        CFLAGS=$CFLAGS_save
        bc_cv_cc_noexecstack=no
        bc_gnu_stack=''
      fi
      ],[
      # didn't compile
      CFLAGS=$CFLAGS_save
      bc_cv_cc_noexecstack=no
      bc_gnu_stack=''
      ])
    AC_LANG_POP(C)
    ])
  ])


dnl BEE_CXX_NOEXECSTACK
AC_DEFUN([BEE_CXX_NOEXECSTACK],[
  AC_CACHE_CHECK([whether we can use noexecstack flag in C++],bc_cv_cxx_noexecstack,[
    CXXFLAGS_save=$CXXFLAGS
    if test "$bc_cv_prog_INTEL_CXX" = yes; then
      CXXFLAGS="$CXXFLAGS -Qoption,asm,--noexecstack"
    else
      CXXFLAGS="$CXXFLAGS -Wa,--noexecstack"
    fi
    AC_LANG_PUSH(C++)
    # first try to compile it
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[]],[[int x = 0]])
      ],[
      # did compile
      $CXX -S $CXXFLAGS conftest.$ac_ext > /dev/null 2>&1
      $CXX -o conftest$ac_exeext $CXXFLAGS conftest.s > /dev/null 2>&1
      if test $? -eq 0; then
        # did assemble
        bc_cv_cxx_noexecstack=yes
        bc_gnu_stack=`$EGREP -e '\.section.*GNU-stack' conftest.s`
      else
        # didn't assemble
        CXXFLAGS=$CXXFLAGS_save
        bc_cv_cxx_noexecstack=no
      fi
      ],[
      # didn't compile
      CXXFLAGS=$CXXFLAGS_save
      bc_cv_cxx_noexecstack=no
      ])
    AC_LANG_POP(C++)
    ])
  ])


dnl BEE_LIBTOOL
AC_DEFUN([BEE_LIBTOOL],[
  case $target_os in
  aix*)
    case $bc_target_arch in
    powerpc64)
      AR="ar -X64"
      NM="/usr/bin/nm -B -X64"
      ;;
    esac
    ;;
  solaris*)
    case $bc_target_arch in
    sparcv9*)
      LD="/usr/ccs/bin/ld"
      LDFLAGS="$LDFLAGS -Wl,-64"
      ;;
    esac
    ;;
  esac
  ])


dnl  BEE_OS_DEFS
AC_DEFUN([BEE_OS_DEFS],[
  AH_TEMPLATE([AIX],[Define to 1 if you are using AIX])
  AH_TEMPLATE([CYGWIN],[Define to 1 if you are using Cygwin])
  AH_TEMPLATE([DARWIN],[Define to 1 if you are using Darwin/MacOS X])
  AH_TEMPLATE([FREEBSD],[Define to 1 if you are using FreeBSD])
  AH_TEMPLATE([HPUX],[Define to 1 if you are using HPUX])
  AH_TEMPLATE([LINUX],[Define to 1 if you are using GNU/Linux])
  AH_TEMPLATE([MINGW],[Define to 1 if you are using MinGW])
  AH_TEMPLATE([NETBSD],[Define to 1 if you are using NetBSD])
  AH_TEMPLATE([OPENBSD],[Define to 1 if you are using OpenBSD])
  AH_TEMPLATE([OSF],[Define to 1 if you are using OSF])
  AH_TEMPLATE([QNX],[Define to 1 if you are using QNX])
  AH_TEMPLATE([SCO_UNIX],[Define to 1 if you are using SCO Unix])
  AH_TEMPLATE([SOLARIS],[Define to 1 if you are using Solaris])
  AH_VERBATIM([WIN32],[
#ifndef WIN32
 #undef WIN32
#endif
    ])

  case $target_os in
    aix*)
      AC_DEFINE([AIX])
      ;;
    cygwin*)
      AC_DEFINE([CYGWIN])
      AC_DEFINE([WIN32])
      ;;
    darwin*)
      AC_DEFINE([DARWIN])
      ;;
    freebsd*)
      AC_DEFINE([FREEBSD])
      ;;
    hpux*)
      AC_DEFINE([HPUX])
      ;;
    linux*)
      AC_DEFINE([LINUX])
      ;;
    mingw*)
      AC_DEFINE([MINGW])
      AC_DEFINE([WIN32])
      ;;
    netbsd*)
      AC_DEFINE([NETBSD])
      ;;
    openbsd*)
      AC_DEFINE([OPENBSD])
      ;;
    osf*)
      AC_DEFINE([OSF])
      ;;
    *qnx)
      AC_DEFINE([QNX])
      ;;
    solaris*)
      AC_DEFINE([SOLARIS])
      ;;
    sysv*uv*)
      AC_DEFINE([SCO_UNIX])
      ;;
    *)
      AC_MSG_WARN([Operating system type $target_os currently not supported and/or tested])
      ;;
  esac
  ])


dnl  BEE_ASM_DEFS
AC_DEFUN([BEE_ASM_DEFS],[
  AC_SUBST(ASM_OS,$target_os)
  AC_SUBST(ASM_CPU,$bc_target_cpu)
  AC_SUBST(ASM_ARCH,$bc_target_arch)
  AC_SUBST(ASM_BIGENDIAN,$ac_cv_c_bigendian)
  AC_SUBST(ASM_GNU_STACK,$bc_gnu_stack)
  ])


dnl  BEE_ASM_TEXTSEG
AC_DEFUN([BEE_ASM_TEXTSEG],[
  AC_CACHE_CHECK([how to switch to text segment],
    bc_cv_asm_textseg,[
      case $target_os in
      aix*)
        bc_cv_asm_textseg=[".csect .text[PR]"] ;;
      hpux*)
        if test "$bc_target_arch" = ia64; then
          bc_cv_asm_textseg=[".section .text"]
        else
          bc_cv_asm_textseg=".code"
        fi
        ;;
      *)
        bc_cv_asm_textseg=".text" ;;
      esac
    ])
  AC_SUBST(ASM_TEXTSEG,$bc_cv_asm_textseg)
  ])


dnl  BEE_ASM_GLOBL
AC_DEFUN([BEE_ASM_GLOBL],[
  AC_CACHE_CHECK([how to declare a global symbol],
    bc_cv_asm_globl,[
      case $target_os in
      hpux*)
        bc_cv_asm_globl=".export" ;;
      *)
        bc_cv_asm_globl=".globl" ;;
      esac
    ])
  AC_SUBST(ASM_GLOBL,$bc_cv_asm_globl)
  ])


dnl  BEE_ASM_GSYM_PREFIX
AC_DEFUN([BEE_ASM_GSYM_PREFIX],[
  AC_CACHE_CHECK([if global symbols need leading underscore],
    bc_cv_asm_gsym_prefix,[
      case $target_os in
      cygwin* | mingw* | darwin*)
        bc_cv_asm_gsym_prefix="_" ;;
      *)
        bc_cv_asm_gsym_prefix="" ;;
      esac
    ])
  AC_SUBST(ASM_GSYM_PREFIX,$bc_cv_asm_gsym_prefix)
  ])


dnl  BEE_ASM_LSYM_PREFIX
AC_DEFUN([BEE_ASM_LSYM_PREFIX],[
  AC_CACHE_CHECK([how to declare a local symbol],
    bc_cv_asm_lsym_prefix,[
      case $target_os in
      aix* | darwin*) bc_cv_asm_lsym_prefix="L" ;;
      hpux* | osf*)   bc_cv_asm_lsym_prefix="$" ;;
      linux*)
        case $target_cpu in
        alpha*)       bc_cv_asm_lsym_prefix="$" ;;
        *)            bc_cv_asm_lsym_prefix=".L" ;;
        esac
        ;;
      *)              bc_cv_asm_lsym_prefix=".L" ;;
      esac
    ])
  AC_SUBST(ASM_LSYM_PREFIX,$bc_cv_asm_lsym_prefix)
  ])


dnl  BEE_ASM_ALIGN
AC_DEFUN([BEE_ASM_ALIGN],[
  AC_CACHE_CHECK([how to align symbols],
    bc_cv_asm_align,[
      case $target_cpu in
      alpha*)
        bc_cv_asm_align=".align 5" ;;
      x86_64 | athlon64 | em64t | k8)
        bc_cv_asm_align=".align 16" ;;
      i[[3456]]86 | athlon*)
        bc_cv_asm_align=".align 4" ;;
      ia64)
        bc_cv_asm_align=".align 16" ;;
      powerpc*)
        bc_cv_asm_align=".align 2" ;;
      s390x)
        bc_cv_asm_align=".align 4" ;;
      sparc*)
        bc_cv_asm_align=".align 4" ;;
      esac
    ])
  AC_SUBST(ASM_ALIGN,$bc_cv_asm_align)
  ])


dnl  BEE_ASM_SOURCES
AC_DEFUN([BEE_ASM_SOURCES],[
  echo > mpopt.s
  echo > blowfishopt.s
  echo > sha1opt.s
  if test "$ac_enable_debug" != yes; then
    case $bc_target_arch in
    arm)
      AC_CONFIG_COMMANDS([mpopt.arm],[
        m4 $srcdir/gas/mpopt.arm.m4 > mpopt.s
        ])
      ;;
    alpha*)
      AC_CONFIG_COMMANDS([mpopt.alpha],[
        m4 $srcdir/gas/mpopt.alpha.m4 > mpopt.s
        ])
      ;;
    x86_64 | athlon64 | athlon-fx | k8 | opteron | em64t | nocona | core2)
      AC_CONFIG_COMMANDS([mpopt.x86_64],[
        m4 $srcdir/gas/mpopt.x86_64.m4 > mpopt.s
        ])
      ;;
    i[[3456]]86 | pentium* | \
    athlon*)
      AC_CONFIG_COMMANDS([mpopt.x86],[
        m4 $srcdir/gas/mpopt.x86.m4 > mpopt.s
        ])
      AC_CONFIG_COMMANDS([sha1opt.x86],[
        m4 $srcdir/gas/sha1opt.x86.m4 > sha1opt.s
        ])
      ;;
    ia64)
      AC_CONFIG_COMMANDS([mpopt.ia64],[
        m4 $srcdir/gas/mpopt.ia64.m4 > mpopt.s
        ])
      ;;
    m68k)
      AC_CONFIG_COMMANDS([mpopt.m68k],[
        m4 $srcdir/gas/mpopt.m68k.m4 > mpopt.s
        ])
      ;;
    powerpc)
      AC_CONFIG_COMMANDS([mpopt.ppc],[
        m4 $srcdir/gas/mpopt.ppc.m4 > mpopt.s
        ])
      ;;
    powerpc64)
      AC_CONFIG_COMMANDS([mpopt.ppc64],[
        m4 $srcdir/gas/mpopt.ppc64.m4 > mpopt.s
        ])
      ;;
    s390x)
      AC_CONFIG_COMMANDS([mpopt.s390x],[
        m4 $srcdir/gas/mpopt.s390x.m4 > mpopt.s
        ])
      ;;
    sparcv8)
      AC_CONFIG_COMMANDS([mpopt.sparcv8],[
        m4 $srcdir/gas/mpopt.sparcv8.m4 > mpopt.s
        ])
      ;;
    sparcv8plus)
      AC_CONFIG_COMMANDS([mpopt.sparcv8plus],[
        m4 $srcdir/gas/mpopt.sparcv8plus.m4 > mpopt.s
        ])
      ;;
    esac
    case $bc_target_arch in
    x86_64 | athlon64 | athlon-fx | k8 | opteron | em64t | nocona | core2) 
      ;;
    i[[56]]86 | pentium* | athlon*)
      AC_CONFIG_COMMANDS([blowfishopt.i586],[
        m4 $srcdir/gas/blowfishopt.i586.m4 > blowfishopt.s
        ])
      ;;
    powerpc)
      AC_CONFIG_COMMANDS([blowfishopt.ppc],[
        m4 $srcdir/gas/blowfishopt.ppc.m4 > blowfishopt.s
        ])
      ;;
    esac
  fi
  ])


dnl  BEE_DLFCN
AC_DEFUN([BEE_DLFCN],[
  AH_TEMPLATE([HAVE_DLFCN_H],[.])
  AC_CHECK_HEADERS([dlfcn.h])
  if test "$ac_cv_header_dlfcn_h" = yes; then
    AC_SEARCH_LIBS([dlopen],[dl dld],[
      ])
  fi
  ])


dnl  BEE_MULTITHREAD
AC_DEFUN([BEE_MULTITHREAD],[
  AH_TEMPLATE([ENABLE_THREADS],[Define to 1 if you want to enable multithread support])
  AH_TEMPLATE([HAVE_THREAD_H],[.])
  AH_TEMPLATE([HAVE_PTHREAD_H],[.])
  AH_TEMPLATE([HAVE_SYNCH_H],[.])
  AH_TEMPLATE([HAVE_SEMAPHORE_H],[.])
  AH_TEMPLATE([HAVE_SCHED_H],[.])

  if test "$ac_enable_threads" = yes; then
    AC_CHECK_HEADERS([synch.h thread.h pthread.h semaphore.h sched.h])
  fi

  bc_include_synch_h=
  bc_include_thread_h=
  bc_include_pthread_h=
  bc_include_semaphore_h=
  bc_include_sched_h=
  bc_typedef_bc_cond_t=
  bc_typedef_bc_mutex_t=
  bc_typedef_bc_sema_t=
  bc_typedef_bc_thread_t=
  bc_typedef_bc_threadid_t=
  if test "$ac_enable_threads" = yes; then
    if test "$ac_cv_header_thread_h" = yes -a "$ac_cv_header_synch_h" = yes; then
      bc_include_synch_h="#include <synch.h>"
      bc_include_thread_h="#include <thread.h>"
      bc_typedef_bc_cond_t="typedef cond_t bc_cond_t;"
      bc_typedef_bc_mutex_t="typedef mutex_t bc_mutex_t;"
      bc_typedef_bc_sema_t="typedef sema_t bc_sema_t;"
      bc_typedef_bc_thread_t="typedef thread_t bc_thread_t;"
      bc_typedef_bc_threadid_t="typedef thread_t bc_threadid_t;"
      AC_SEARCH_LIBS([mutex_lock],[thread],[
        AC_DEFINE([ENABLE_THREADS],1)
        ])
    else
      if test "$ac_cv_header_semaphore_h" = yes; then
        bc_include_semaphore_h="#include <semaphore.h>"
        bc_typedef_bc_sema_t="typedef sem_t bc_sema_t;"
      fi
      if test "$ac_cv_header_sched_h" = yes; then
        bc_include_sched_h="#include <sched.h>"
      fi
      if test "$ac_cv_header_pthread_h" = yes; then
        bc_include_pthread_h="#include <pthread.h>"
        bc_typedef_bc_cond_t="typedef pthread_cond_t bc_cond_t;"
        bc_typedef_bc_mutex_t="typedef pthread_mutex_t bc_mutex_t;"
        bc_typedef_bc_thread_t="typedef pthread_t bc_thread_t;"
        bc_typedef_bc_threadid_t="typedef pthread_t bc_threadid_t;"
        # On most systems this tests will say 'none required', but that doesn't
        # mean that the linked code will work correctly!
        case $target_os in
        linux* | solaris* )
          AC_DEFINE([ENABLE_THREADS],1)
          LIBS="-lpthread $LIBS"
          ;;
        osf*)
          AC_DEFINE([ENABLE_THREADS],1)
          LIBS="-lpthread -lmach -lexc $LIBS"
          ;;
        *)
          AC_SEARCH_LIBS([pthread_mutex_lock],[pthread],[
            AC_DEFINE([ENABLE_THREADS],1)
            ])
          ;;
        esac
      else
        case $target_os in
        mingw*)
          bc_typedef_bc_cond_t="typedef HANDLE bc_cond_t;"
          bc_typedef_bc_mutex_t="typedef HANDLE bc_mutex_t;"
          bc_typedef_bc_thread_t="typedef HANDLE bc_thread_t;"
          bc_typedef_bc_threadid_t="typedef DWORD bc_threadid_t;"
          ;;
        *)
          AC_MSG_WARN([Don't know which thread library to check for])
          ;;
        esac
      fi
    fi
  fi
  AC_SUBST(INCLUDE_SYNCH_H,$bc_include_synch_h)
  AC_SUBST(INCLUDE_THREAD_H,$bc_include_thread_h)
  AC_SUBST(INCLUDE_PTHREAD_H,$bc_include_pthread_h)
  AC_SUBST(INCLUDE_SEMAPHORE_H,$bc_include_semaphore_h)
  AC_SUBST(INCLUDE_SCHED_H,$bc_include_sched_h)
  AC_SUBST(TYPEDEF_BC_COND_T,$bc_typedef_bc_cond_t)
  AC_SUBST(TYPEDEF_BC_MUTEX_T,$bc_typedef_bc_mutex_t)
  AC_SUBST(TYPEDEF_BC_THREAD_T,$bc_typedef_bc_thread_t)
  AC_SUBST(TYPEDEF_BC_THREADID_T,$bc_typedef_bc_threadid_t)
  ])

AH_BOTTOM([
#if ENABLE_THREADS
# ifndef _REENTRANT
#  define _REENTRANT
# endif
# if LINUX
#  define _LIBC_REENTRANT
# endif
#else
# ifdef _REENTRANT
#  undef _REENTRANT
# endif
#endif
])


dnl  BEE_THREAD_LOCAL_STORAGE
AC_DEFUN([BEE_THREAD_LOCAL_STORAGE],[
  AH_TEMPLATE([ENABLE_THREAD_LOCAL_STORAGE],[Define to 1 if you want to enable thread-local-storage support])
  if test "$ac_enable_threads" = yes; then
    AC_MSG_CHECKING([if your compiler supports thread-local-storage])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([__thread int a = 0;])],[
      AC_DEFINE([ENABLE_THREAD_LOCAL_STORAGE],1)
      AC_MSG_RESULT([yes])
      ],[
      AC_DEFINE([ENABLE_THREAD_LOCAL_STORAGE],0)
      AC_MSG_RESULT([no])
      ])
  else
    AC_DEFINE([ENABLE_THREAD_LOCAL_STORAGE],0)
  fi
  ])

AH_BOTTOM([
#if !ENABLE_THREAD_LOCAL_STORAGE
# define __thread
#endif
])
