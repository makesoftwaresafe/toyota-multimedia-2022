/** \ingroup py_c
 * \file python/rpmrc-py.c
 */

#include "system-py.h"

#include "structmember.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <argv.h>
#include <rpmmacro.h>

#include <rpmtypes.h>
#include <rpmtag.h>

#include "rpmdebug-py.c"
#include "rpmmacro-py.h"

#include "debug.h"

/** \ingroup python
 * \class Rpmmacro
 * \brief A python rpm.macro object encapsulates macro configuration.
 */

/** \ingroup python
 * \name Class: Rpmmacro
 */
/*@{*/

/**
 */
PyObject *
rpmmacro_AddMacro(/*@unused@*/ PyObject * s, PyObject * args,
		PyObject * kwds)
{
    char * name, * val;
    char * kwlist[] = {"name", "value", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ss:AddMacro", kwlist,
	    &name, &val))
	return NULL;

    addMacro(NULL, name, NULL, val, -1);

    Py_RETURN_NONE;
}

/**
 */
PyObject *
rpmmacro_DelMacro(/*@unused@*/ PyObject * s, PyObject * args,
		PyObject * kwds)
{
    char * name;
    char * kwlist[] = {"name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s:DelMacro", kwlist, &name))
	return NULL;

    delMacro(NULL, name);

    Py_RETURN_NONE;
}

/**
 */
PyObject *
rpmmacro_ExpandMacro(/*@unused@*/ PyObject * s, PyObject * args,
		PyObject * kwds)
{
    char * macro, * str;
    char * kwlist[] = {"macro", NULL};
    PyObject *res;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s:ExpandMacro", kwlist, &macro))
	return NULL;

    str = rpmExpand(macro, NULL);
    res = Py_BuildValue("s", str);
    str = _free(str);
    return res;
}

/**
 */
PyObject *
rpmmacro_GetMacros(/*@unused@*/ PyObject * s, PyObject * args,
		PyObject * kwds)
{
    char * kwlist[] = { NULL };
    PyObject * mdict;
    PyObject *oo, *bo;
    const char ** av = NULL;
    int ac = 0;
    int i;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, ":GetMacros", kwlist))
	return NULL;

    mdict = PyDict_New();
    ac = rpmGetMacroEntries(NULL, NULL, -1, &av);
    if (mdict == NULL || ac < 0 || av == NULL) {
	PyErr_SetString(pyrpmError, "out of memory");
	if (av) {
	    int i;
	    for (i = 0; i < ac; i++)
		av[i] = _free(av[i]);
	    av = _free(av);
	}
	return NULL;
    }

    if (ac == 0) {
	av = argvFree(av);
	return mdict;
    }

    oo = PyString_FromString("opts");
    bo = PyString_FromString("body");

    if (oo != NULL && bo != NULL)
    for (i = 0; i < ac; i++) {
	char *n, *o, *b;
	PyObject *ndo, *no, *vo;
	int failed = 0;

	/* Parse out "%name(opts)\tbody" into n/o/b strings. */
	n = (char *) av[i];
	b = strchr(n, '\t');
assert(b != NULL);
	o = ((b > n && b[-1] == ')') ? strchr(n, '(') : NULL);
	if (*n == '%')	n++;
	if (o != NULL && *o == '(') {
	    b[-1] = '\0';
	    o++;
	}
	b++;

	/* Create a "name" dictionary, add "opts" and "body" items. */
	no = PyString_FromString(n);
	if (no == NULL)
	    break;

	ndo = PyDict_New();
	if (ndo == NULL) {
	    Py_XDECREF(no);
	    break;
	}
	PyDict_SetItem(mdict, no, ndo);
	Py_XDECREF(ndo);

	if (o) {
	    if ((vo = PyString_FromString(o)) != NULL)
		PyDict_SetItem(ndo, oo, vo);
	    else
		failed = 1;
	    Py_XDECREF(vo);
	}

	if (b) {
	    if ((vo = PyString_FromString(b)) != NULL)
		PyDict_SetItem(ndo, bo, vo);
	    else
		failed = 1;
	    Py_XDECREF(vo);
	}

	if (failed)
	    PyDict_DelItem(mdict, no);
	Py_XDECREF(no);
    }

   Py_XDECREF(oo);
   Py_XDECREF(bo);

   av = argvFree(av);
   return mdict;
}

/*@}*/
