#ifndef	H_SYSTEM_RUBY
#define	H_SYSTEM_RUBY

#include "../system.h"

/* XXX ruby-1.8.6 grrr, ruby.h includes its own config.h too. */
#undef  PACKAGE_NAME
#undef  PACKAGE_TARNAME
#undef  PACKAGE_VERSION
#undef  PACKAGE_STRING
#undef  PACKAGE_BUGREPORT

#define	_save		_
#undef	_
#define	_xmalloc	xmalloc
#undef	xmalloc
#define	_xcalloc	xcalloc
#undef	xcalloc
#define	_xrealloc	xrealloc
#undef	xrealloc
#define	_xfree		xfree
#undef	xfree

#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <ruby.h>
#pragma GCC diagnostic warning "-Wstrict-prototypes"

#undef	_
#define	_		_save
#undef	_save
#undef	xmalloc
#define	xmalloc		_xmalloc
#undef	_xmalloc
#undef	xrealloc
#define	xrealloc	_xrealloc
#undef	_xrealloc
#undef	xfree
#define	xfree		_xfree
#undef	xfree

#endif	/* H_SYSTEM_RUBY */
