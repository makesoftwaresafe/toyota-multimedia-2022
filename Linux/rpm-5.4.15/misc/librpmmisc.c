/**
 * \file misc/librpmmisc.c
 */

#include "system.h"

#if !defined(HAVE_BASENAME)
#include "basename.c"
#endif

#if !defined(HAVE_GETCWD)
#include "getcwd.c"
#endif

#if !defined(HAVE_GETWD)
#include "getwd.c"
#endif

#if !defined(HAVE_PUTENV)
#include "putenv.c"
#endif

#if defined(USE_GETMNTENT)
#include "getmntent.c"
#endif

#if !defined(HAVE_REALPATH)
#include "realpath.c"
#endif

#if !defined(HAVE_SETENV)
#include "setenv.c"
#endif

/* XXX limit the fiddle up to linux for now. */
#if !defined(HAVE_SETPROCTITLE) && defined(__linux__)
#include "setproctitle.c"
#endif

#if !defined(HAVE_STPCPY)
#include "stpcpy.c"
#endif

#if !defined(HAVE_STPNCPY)
#include "stpncpy.c"
#endif

#if !defined(HAVE_STRCSPN)
#include "strcspn.c"
#endif

#if !defined(HAVE_STRSPN)
#include "strdup.c"
#endif

#if !defined(HAVE_STRERROR)
#include "error.c"
#endif

#if !defined(HAVE_STRTOL)
#include "strtol.c"
#endif

#if !defined(HAVE_STRTOUL)
#include "strtoul.c"
#endif

#if !defined(HAVE_STRSPN)
#include "strspn.c"
#endif

#if !defined(HAVE_STRSTR)
#include "strstr.c"
#endif

#if !defined(HAVE_MKDTEMP)
#include "mkdtemp.c"
#endif
