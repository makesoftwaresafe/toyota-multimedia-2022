#include "config.h"
#if defined(WITH_PYTHONEMBED)
#include <Python.h>
#include <cStringIO.h>
#endif

#include "system.h"

#define	_RPMIOB_INTERNAL	/* XXX necessary? */
#include <rpmiotypes.h>
#include <rpmmacro.h>
#include <argv.h>

#define _RPMPYTHON_INTERNAL
#include "rpmpython.h"

#include "debug.h"

/*@unchecked@*/
int _rpmpython_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmpython _rpmpythonI = NULL;

static void rpmpythonFini(void * _python)
        /*@globals fileSystem @*/
        /*@modifies *_python, fileSystem @*/
{
    rpmpython python = (rpmpython) _python;

#if defined(WITH_PYTHONEMBED)
    Py_Finalize();
#endif
    python->I = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmpythonPool;

static rpmpython rpmpythonGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmpythonPool, fileSystem @*/
        /*@modifies pool, _rpmpythonPool, fileSystem @*/
{
    rpmpython python;

    if (_rpmpythonPool == NULL) {
        _rpmpythonPool = rpmioNewPool("python", sizeof(*python), -1, _rpmpython_debug,
                        NULL, NULL, rpmpythonFini);
        pool = _rpmpythonPool;
    }
    return (rpmpython) rpmioGetPool(pool, sizeof(*python));
}

/*@unchecked@*/
#if defined(WITH_PYTHONEMBED)
static const char * _rpmpythonI_init = "\
import sys\n\
from cStringIO import StringIO\n\
sys.stdout = StringIO()\n\
";
#endif

static rpmpython rpmpythonI(void)
	/*@globals _rpmpythonI @*/
	/*@modifies _rpmpythonI @*/
{
    if (_rpmpythonI == NULL)
	_rpmpythonI = rpmpythonNew(NULL, 0);
    return _rpmpythonI;
}

rpmpython rpmpythonNew(char ** av, uint32_t flags)
{
    static char * _av[] = { (char *) "rpmpython", NULL };
#if defined(WITH_PYTHONEMBED)
    int initialize = (!(flags & 0x80000000) || _rpmpythonI == NULL);
#endif
    rpmpython python = (flags & 0x80000000)
	? rpmpythonI() : rpmpythonGetPool(_rpmpythonPool);

if (_rpmpython_debug)
fprintf(stderr, "==> %s(%p, %d) python %p\n", __FUNCTION__, av, flags, python);

    if (av == NULL) av = _av;

#if defined(WITH_PYTHONEMBED)
    if (!Py_IsInitialized()) {
	Py_SetProgramName((char *)_av[0]);
	Py_Initialize();
    }
    if (PycStringIO == NULL)
	PycStringIO = (struct PycStringIO_CAPI *)
			PyCObject_Import("cStringIO", "cStringIO_CAPI");

    if (initialize) {
	static const char _pythonI_init[] = "%{?_pythonI_init}";
	const char * s = rpmExpand(_rpmpythonI_init, _pythonI_init, NULL);
	int ac = argvCount((ARGV_t)av);
	(void) PySys_SetArgv(ac, (char **)av);
if (_rpmpython_debug)
fprintf(stderr, "==========\n%s\n==========\n", s);
	(void) rpmpythonRun(python, s, NULL);
	s = _free(s);
    }
#endif

    return rpmpythonLink(python);
}

rpmRC rpmpythonRunFile(rpmpython python, const char * fn, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmpython_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, python, fn);

    if (python == NULL) python = rpmpythonI();

    if (fn != NULL) {
#if defined(WITH_PYTHONEMBED)
	const char * pyfn = ((fn == NULL || !strcmp(fn, "-")) ? "<stdin>" : fn);
	FILE * pyfp = (!strcmp(pyfn, "<stdin>") ? stdin : fopen(fn, "rb"));
	int closeit = (pyfp != stdin);
	PyCompilerFlags cf = { 0 };
	
	if (pyfp != NULL) {
	    PyRun_AnyFileExFlags(pyfp, pyfn, closeit, &cf);
	    rc = RPMRC_OK;
	}
#endif
    }
    return rc;
}

static const char * rpmpythonSlurp(const char * arg)
	/*@*/
{
    rpmiob iob = NULL;
    const char * val = NULL;
    struct stat sb;
    int xx;

    if (!strcmp(arg, "-")) {	/* Macros from stdin arg. */
	xx = rpmiobSlurp(arg, &iob);
    } else
    if ((arg[0] == '/' || strchr(arg, ' ') == NULL)
     && !Stat(arg, &sb)
     && S_ISREG(sb.st_mode)) {	/* Macros from a file arg. */
	xx = rpmiobSlurp(arg, &iob);
    } else {			/* Macros from string arg. */
	iob = rpmiobAppend(rpmiobNew(strlen(arg)+1), arg, 0);
    }

    val = xstrdup(rpmiobStr(iob));
    iob = rpmiobFree(iob);
    return val;
}

rpmRC rpmpythonRun(rpmpython python, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmpython_debug)
fprintf(stderr, "==> %s(%p,%s,%p)\n", __FUNCTION__, python, str, resultp);

    if (python == NULL) python = rpmpythonI();

    if (str != NULL) {
	const char * val = rpmpythonSlurp(str);
#if defined(WITH_PYTHONEMBED)
	PyCompilerFlags cf = { 0 };
	PyObject * m = PyImport_AddModule("__main__");
	PyObject * d = (m ? PyModule_GetDict(m) : NULL);
	PyObject * v = (m ? PyRun_StringFlags(val, Py_file_input, d, d, &cf) : NULL);

        if (v == NULL) {
	    PyErr_Print();
	} else {
	    if (resultp != NULL) {
		PyObject * sys_stdout = PySys_GetObject((char *)"stdout");
		if (sys_stdout != NULL && PycStringIO_OutputCheck(sys_stdout)) {
		    PyObject * o = (*PycStringIO->cgetvalue)(sys_stdout);
		    *resultp = (PyString_Check(o) ? PyString_AsString(o) : "");
		} else
		    *resultp = "";
	    }
	    Py_XDECREF(v);
	    if (Py_FlushLine())
		PyErr_Clear();
	    rc = RPMRC_OK;
	}
#endif
	val = _free(val);
    }
    return rc;
}
