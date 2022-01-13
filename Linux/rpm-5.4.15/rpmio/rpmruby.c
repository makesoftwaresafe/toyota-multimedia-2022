#include "system.h"
#include <argv.h>

#if defined(WITH_RUBYEMBED)

/* Make sure Ruby's fun stuff has its own xmalloc & Co functions available */
#undef xmalloc
#undef xcalloc
#undef xrealloc

#undef	PACKAGE_NAME
#undef	PACKAGE_TARNAME
#undef	PACKAGE_VERSION
#undef	PACKAGE_STRING
#undef	PACKAGE_BUGREPORT

#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <ruby.h>
#pragma GCC diagnostic warning "-Wstrict-prototypes"

#endif

#define _RPMRUBY_INTERNAL 1
#include "rpmruby.h"

#include "debug.h"

/*@unchecked@*/
int _rpmruby_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmruby _rpmrubyI = NULL;

/**
 * Finalizes a Ruby interpreter instance/pool item
 */
static void rpmrubyFini(void *_ruby)
        /*@globals fileSystem @*/
        /*@modifies *_ruby, fileSystem @*/
{
    rpmruby ruby = (rpmruby) _ruby;

#if defined(WITH_RUBYEMBED)
    ruby_cleanup(0); 
#endif
    ruby->I = NULL;
}

/**
* The pool of Ruby interpreter instances
* @see rpmioPool
*/
/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmrubyPool;

/**
 * Returns the current rpmio pool responsible for Ruby interpreter instances
 *
 * This is a wrapper function that returns the current rpmio pool responsible
 * for embedded Ruby interpreters. It creates and initializes a new pool when
 * there is no current pool.
 *
 * @return The current pool
 * @see rpmioNewPool
 */
static rpmruby rpmrubyGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmrubyPool, fileSystem @*/
        /*@modifies pool, _rpmrubyPool, fileSystem @*/
{
    rpmruby ruby;

    if (_rpmrubyPool == NULL) {
        _rpmrubyPool = rpmioNewPool("ruby", sizeof(*ruby), -1, _rpmruby_debug,
            NULL, NULL, rpmrubyFini);
        pool = _rpmrubyPool;
    } 

    return (rpmruby) rpmioGetPool(pool, sizeof(*ruby));
}

#if defined(WITH_RUBYEMBED)
/** Initializes Ruby's StringIO for storing output in a string. */
/*@unchecked@*/
static const char * rpmrubyInitStringIO = "\
require 'stringio'\n\
$stdout = StringIO.new($result, \"w+\")\n\
";
#endif

static rpmruby rpmrubyI(void)
        /*@globals _rpmrubyI @*/
        /*@modifies _rpmrubyI @*/
{
    if (_rpmrubyI == NULL)
        _rpmrubyI = rpmrubyNew(NULL, 0);
    return _rpmrubyI;
}

rpmruby rpmrubyNew(char **av, uint32_t flags)
{
    static const char *_av[] = { "rpmruby", NULL };
    
    /* XXX FIXME: recurse like every other embedded interpreter. */
    if (_rpmrubyI)
        return _rpmrubyI;

    rpmruby ruby = rpmrubyGetPool(_rpmrubyPool);

    if (av == NULL)
        av = (char **) _av;

#if defined(WITH_RUBYEMBED)
    RUBY_INIT_STACK;
    ruby_init();
    ruby_init_loadpath();

    ruby_script((char *)av[0]);
    rb_gv_set("$result", rb_str_new2(""));
    (void) rpmrubyRun(ruby, rpmrubyInitStringIO, NULL);
#endif

    return rpmrubyLink(ruby);
}


rpmRC rpmrubyRun(rpmruby ruby, const char *str, const char **resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmruby_debug)
fprintf(stderr, "==> %s(%p,%s,%p)\n", __FUNCTION__, ruby, str, resultp);

    if (ruby == NULL)
        ruby = rpmrubyI();

#if defined(WITH_RUBYEMBED)
    if (str) {
	int state = -1;
	ruby->state = rb_eval_string_protect(str, &state);

	/* Check whether the evaluation was successful or not */

	if (state == 0) {
	    rc = RPMRC_OK;
	    if (resultp)
		*resultp = RSTRING_PTR(rb_gv_get("$result"));
	}
    }
#endif

    return rc;
}

