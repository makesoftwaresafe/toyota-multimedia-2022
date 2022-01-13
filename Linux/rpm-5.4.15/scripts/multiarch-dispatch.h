/* Undefine some built-ins */
#ifdef  linux
#undef  linux
#define _multiarch_defined_linux
#endif
#ifdef  i386
#undef  i386
#define _multiarch_defined_i386
#endif
#ifdef  sparc
#undef  sparc
#define _multiarch_defined_sparc
#endif
#ifdef	mips
#undef	mips
#define	_multiarch_defined_mips
#endif
#ifdef	arm
#undef	arm
#define	_multiarch_defined_arm
#endif

/* Undefine some interfering definitions (not built-ins) */
/* We can only redefine value 1, so we let it fail with other values */
#if x86_64 == 1
#define _multiarch_defined_x86_64
#undef x86_64
#endif
#if ppc64 == 1
#define _multiarch_defined_ppc64
#undef ppc64
#endif
#if ppc == 1
#define _multiarch_defined_ppc
#undef ppc
#endif
#if sparc64 == 1
#define _multiarch_defined_sparc64
#undef sparc64
#endif
#if s390x == 1
#define _multiarch_defined_s390x
#undef s390x
#endif
#if s390 == 1
#define _multiarch_defined_s390
#undef s390
#endif
#if ia64 == 1
#define _multiarch_defined_ia64
#undef ia64
#endif
#if alpha == 1
#define _multiarch_defined_alpha
#undef alpha
#endif
#if mipsel == 1
#define _multiarch_defined_mipsel
#undef mipsel
#endif
#if armeb == 1
#define _multiarch_defined_armeb
#undef armeb
#endif

/* Dispatch arch dependent header */
#if defined(__linux__)
#define _MULTIARCH_OS linux
#endif
#define _MULTIARCH_MAKE_HEADER(arch,header) <multiarch-arch-_MULTIARCH_OS/header>
#if defined(__x86_64__)
#include _MULTIARCH_MAKE_HEADER(x86_64,_MULTIARCH_HEADER)
#elif defined(__i386__)
#include _MULTIARCH_MAKE_HEADER(i386,_MULTIARCH_HEADER)
#elif defined(__powerpc64__)
#include _MULTIARCH_MAKE_HEADER(ppc64,_MULTIARCH_HEADER)
#elif defined(__powerpc__)
#include _MULTIARCH_MAKE_HEADER(ppc,_MULTIARCH_HEADER)
#elif defined(__sparc__) && defined(__arch64__)
#include _MULTIARCH_MAKE_HEADER(sparc64,_MULTIARCH_HEADER)
#elif defined(__sparc__)
#include _MULTIARCH_MAKE_HEADER(sparc,_MULTIARCH_HEADER)
#elif defined(__s390x__)
#include _MULTIARCH_MAKE_HEADER(s390x,_MULTIARCH_HEADER)
#elif defined(__s390__)
#include _MULTIARCH_MAKE_HEADER(s390,_MULTIARCH_HEADER)
#elif defined(__ia64__)
#include _MULTIARCH_MAKE_HEADER(ia64,_MULTIARCH_HEADER)
#elif defined(__alpha__)
#include _MULTIARCH_MAKE_HEADER(alpha,_MULTIARCH_HEADER)
#elif defined(__mips__)
#if defined(__BIG_ENDIAN__)
#include _MULTIARCH_MAKE_HEADER(mips,_MULTIARCH_HEADER)
#else
#include _MULTIARCH_MAKE_HEADER(mipsel,_MULTIARCH_HEADER)
#endif
#elif defined(__arm__)
#if defined(__BIG_ENDIAN__)
#include _MULTIARCH_MAKE_HEADER(armeb,_MULTIARCH_HEADER)
#else
#include _MULTIARCH_MAKE_HEADER(arm,_MULTIARCH_HEADER)
#endif
#else
#error "Unknown architecture, please submit bug report"
#endif
#undef _MULTIARCH_MAKE_HEADER
#undef _MULTIARCH_OS
#undef _MULTIARCH_HEADER

/* Redefine built-ins */
#ifdef  _multiarch_defined_linux
#undef  _multiarch_defined_linux
#define linux 1
#endif
#ifdef  _multiarch_defined_i386
#undef  _multiarch_defined_i386
#define i386 1
#endif
#ifdef  _multiarch_defined_sparc
#undef  _multiarch_defined_sparc
#define sparc 1
#endif
#ifdef  _multiarch_defined_mips
#undef  _multiarch_defined_mips
#define mips 1
#endif
#ifdef  _multiarch_defined_arm
#undef  _multiarch_defined_arm
#define arm 1
#endif

/* Redefine interfering defitions */
#ifdef  _multiarch_defined_x86_64
#undef  _multiarch_defined_x86_64
#define x86_64 1
#endif
#ifdef  _multiarch_defined_ppc64
#undef  _multiarch_defined_ppc64
#define ppc64 1
#endif
#ifdef  _multiarch_defined_ppc
#undef  _multiarch_defined_ppc
#define ppc 1
#endif
#ifdef  _multiarch_defined_sparc64
#undef  _multiarch_defined_sparc64
#define sparc64 1
#endif
#ifdef  _multiarch_defined_s390x
#undef  _multiarch_defined_s390x
#define s390x 1
#endif
#ifdef  _multiarch_defined_s390
#undef  _multiarch_defined_s390
#define s390 1
#endif
#ifdef  _multiarch_defined_ia64
#undef  _multiarch_defined_ia64
#define ia64 1
#endif
#ifdef  _multiarch_defined_alpha
#undef  _multiarch_defined_alpha
#define alpha 1
#endif
#ifdef  _multiarch_defined_mipsel
#undef  _multiarch_defined_mipsel
#define mipsel 1
#endif
#ifdef  _multiarch_defined_armeb
#undef  _multiarch_defined_armeb
#define armeb 1
#endif
