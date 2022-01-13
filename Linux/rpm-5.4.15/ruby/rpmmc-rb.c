/** \ingroup rb_c
 * \file ruby/rpmmc-rb.c
 */

#include "system.h"
#include "rpm-rb.h"
#include "rpmmc-rb.h"

#define	_MACRO_INTERNAL
#include <rpmmacro.h>
#include <rpmio/argv.h>

#include "../debug.h"


typedef MacroContext rpmmc;

VALUE rpmmcClass;

/*@unchecked@*/
static int _debug = 0;


/**
 * Returns the wrapped C structure ::MacroContext_s.
 */
static rpmmc
_rpmmc_get_mc(VALUE self)
{
    rpmmc mc;
    Data_Get_Struct(self, struct MacroContext_s, mc);
    return mc;
}


/**
 * Adds a new macro definition to the Macro Context.
 *
 * call-seq:
 *  RPM::Mc#add(macro) -> RPM::Mc
 *
 * @param macro The macro definition in string form just like it would be done
 *  in a macro definition file, but minus the %define stanza.
 * @return      The used macro context instance
 * @see         rpmDefineMacro()
 */
static VALUE
rpmmc_add(VALUE self, VALUE macro)
{
    Check_Type(macro, T_STRING);

    rpmmc mc = _rpmmc_get_mc(self);
    int lvl = 0;
    
    if (_debug)
        fprintf(stderr, "==> %s(0x%lx, 0x%lx) ptr %p\n",
            __FUNCTION__, self, macro, mc);
    
    int error = rpmDefineMacro(mc, RSTRING_PTR(macro), lvl);
    if(error)
        rpm_rb_raise(error, "Macro definition failed");
    return self;
}


/**
 * Deletes a macro definition.
 *
 * call-seq:
 *  RPM::Mc#del(macro) -> RPM::Mc
 *
 * @param macro The macro name
 * @return      The Mc object instance
 * @see         rpmUndefineMacro()
 */
static VALUE
rpmmc_del(VALUE self, VALUE macro)
{
    Check_Type(macro, T_STRING);

    rpmmc mc = _rpmmc_get_mc(self);
    
    if (_debug)
        fprintf(stderr, "==> %s(0x%lx, 0x%lx) ptr %p\n",
            __FUNCTION__, self, macro, mc);
    
    int error = rpmUndefineMacro(mc, StringValueCStr(macro));
    if(error)
        rpm_rb_raise(error, "Macro deletion failed");
    return self;
}


/**
 * List all macro definitions in the corresponding Macro Context.
 *
 * call-seq:
 *  RPM::Mc#list() -> Array
 *
 * @return  A list of all macro definitions in form of an array of arrays,
 *  where each nested arry contains the macro's name, arguments (or an empty
 *  string) and the macro body.
 */
static VALUE
rpmmc_list(VALUE self)
{
    rpmmc mc = _rpmmc_get_mc(self);

    void * _mire = NULL;
    VALUE v = rb_ary_new();
    int used = -1;
    const char ** av = NULL;
    int ac = rpmGetMacroEntries(mc, _mire, used, &av);

    if (_debug)
        fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, self, mc);

    if (ac > 0 && av != NULL && av[0] != NULL) {
        int i;
        for (i = 0; i < ac; i++) {
            /* Parse out "%name(opts)\tbody" into n/o/b strings. */

            char *name = (char *)av[i];
            char *body = strchr(name, '\t');
            assert(body != NULL);
            char *opts = ((body > name && body[-1] == ')') ?
                strchr(name, '(') : NULL);

        	if (*name == '%') name++;
        	if (opts != NULL && *opts == '(') {
                body[-1] = '\0';
                opts++;
                opts[-1] = '\0';
            } else {
                body[0] = '\0';
                opts = "";
            }
            body++;

            VALUE nob_ary = rb_ary_new3(3, rb_str_new2(name),
                rb_str_new2(opts), rb_str_new2(body));
            rb_ary_push(v, nob_ary);
        }
    }

    argvFree(av);
    return v;
}


/**
 * Expands a macro.
 *
 * call-seq:
 *  RPM::Mc#expand(macro) -> String
 *
 * @param macro The macro name (with leading % sign)
 * @return      The result of the expansion
 */
static VALUE
rpmmc_expand(VALUE self, VALUE macro)
{
    rpmmc mc = _rpmmc_get_mc(self);
    char *vstr = StringValueCStr(macro);

    if (_debug)
        fprintf(stderr, "==> %s(0x%lx, 0x%lx) ptr %p \"%s\"\n",
            __FUNCTION__, self, macro, mc, vstr);
    return rb_str_new2(rpmMCExpand(mc, vstr, NULL));
}


/**
 * Loads a macro file.
 *
 * call-seq:
 *  RPM::Mc#load_macro_file(fn, nesting) -> RPM::Mc
 *
 * @param fn        The path of the macro file
 * @param nesting   Maximum recursion depth; 0 disables recursion
 * @return          The RPM::Mc instance
 * @see             rpmLoadMacroFile()
 */
static VALUE
rpmmc_load_macro_file(VALUE self, VALUE fn_v, VALUE nesting_v)
{
    rpmmc mc = _rpmmc_get_mc(self);

    Check_Type(fn_v, T_STRING);
    Check_Type(nesting_v, T_FIXNUM);

    int error = rpmLoadMacroFile(mc, RSTRING_PTR(fn_v), FIX2INT(nesting_v));
    if(error)
        rpm_rb_raise(error, "Loading macro file failed");
    return self;
}


/**
 * Initializes a macro context from a list of files
 *
 * call-seq:
 *  RPM::Mc#init_macros(files) -> RPM::Mc
 *
 * @param files A list of files to add, separated by colons
 * @return      The RPM::Mc instance
 * @see         rpmInitMacros()
 */
static VALUE
rpmmc_init_macros(VALUE self, VALUE macrofiles_v)
{
    Check_Type(macrofiles_v, T_STRING);

    rpmmc mc = _rpmmc_get_mc(self);

    rpmInitMacros(mc, RSTRING_PTR(macrofiles_v));
    return self;
}


static void
initMethods(VALUE klass)
{
    rb_define_method(klass, "add", &rpmmc_add, 1);
    rb_define_method(klass, "del", &rpmmc_del, 1);
    rb_define_method(klass, "list", &rpmmc_list, 0);
    rb_define_method(klass, "expand", &rpmmc_expand, 1);
    rb_define_method(klass, "load_macro_file", &rpmmc_load_macro_file, 2);
    rb_define_method(klass, "init_macros", &rpmmc_init_macros, 1);
}


/* --- Object properties */

/**
 * Get debugging log level
 *
 * call-seq:
 *  RPM::Mc#debug -> Fixnum
 *
 * @return The debugging level
 */
static VALUE
rpmmc_debug_get(VALUE s)
{
    if (_debug)
        fprintf(stderr, "==> %s(0x%lx)\n", __FUNCTION__, s);
    return INT2FIX(_debug);
}


/**
 * Set debugging log level
 *
 * call-seq:
 *  RPM::Mc.debug = LEVEL -> Fixnum
 *
 * @return The new debug level
 */
static VALUE
rpmmc_debug_set(VALUE s, VALUE v)
{
    if (_debug)
        fprintf(stderr, "==> %s(0x%lx, 0x%lx)\n", __FUNCTION__, s, v);
    return INT2FIX(_debug = FIX2INT(v));
}


/**
 * Return the global macro context.
 *
 * call-seq:
 *  RPM::Mc.global_context -> RPM::Mc
 *
 * @return  An RPM::Mc object representing the global mc.
 */

static VALUE
rpmmc_get_global_mc(void)
{
    return rpmmc_wrap(rpmGlobalMacroContext);
}


/**
 * Return the CLI macro context.
 *
 * call-seq:
 *  RPM::Mc.cli_context -> RPM::Mc
 *
 * @return  An RPM::Mc object representing the CLI mc.
 */
static VALUE
rpmmc_get_cli_mc(void)
{
    return rpmmc_wrap(rpmCLIMacroContext);
}


static void
initProperties(VALUE klass)
{
    rb_define_method(klass, "debug", rpmmc_debug_get, 0);
    rb_define_method(klass, "debug=", rpmmc_debug_set, 1);
    rb_define_singleton_method(klass, "global_context", 
        rpmmc_get_global_mc, 0);
    rb_define_singleton_method(klass, "cli_context", rpmmc_get_cli_mc, 0);
}


/* --- Object ctors/dtors */

static void
_rpmmc_free(rpmmc mc)
{
    /* Don't free the global or CLI macro context */
    if(mc == rpmGlobalMacroContext || mc == rpmCLIMacroContext) return;

    rpmFreeMacros(mc);
    mc = _free(mc);
}


VALUE
rpmmc_wrap(rpmmc mc)
{
    if (_debug)
        fprintf(stderr, "==> %s(%p)\n", __FUNCTION__, mc);
    return Data_Wrap_Struct(rpmmcClass, 0, &_rpmmc_free, mc);
}


static VALUE
rpmmc_alloc(VALUE klass)
{
    rpmmc mc = xcalloc(1, sizeof(*mc));
    VALUE obj = rpmmc_wrap(mc);
    if (_debug)
        fprintf(stderr, "==> %s(0x%lx) obj 0x%lx mc %p\n",
            __FUNCTION__, klass, obj, mc);
    return obj;
}


/* --- Class initialization */

void
Init_rpmmc(void)
{
    rpmmcClass = rb_define_class_under(rpmModule, "Mc", rb_cObject);
    if (_debug)
        fprintf(stderr, "==> %s() rpmmcClass 0x%lx\n", __FUNCTION__, rpmmcClass);
    rb_define_alloc_func(rpmmcClass, rpmmc_alloc);
    initProperties(rpmmcClass);
    initMethods(rpmmcClass);
}
