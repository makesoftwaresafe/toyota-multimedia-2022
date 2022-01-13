/** \ingroup py_c
 * \file python/rpmal-py.c
 */

#include "system-py.h"

#include <rpmio.h>		/* XXX rpmRC returns */
#include <rpmiotypes.h>		/* XXX fnpyKey */
#include <rpmtypes.h>
#include <rpmtag.h>

#include "rpmal-py.h"
#include "rpmds-py.h"
#include "rpmfi-py.h"

#include "debug.h"

/*@null@*/
static PyObject *
rpmal_Add(rpmalObject * s, PyObject * args, PyObject * kwds)
	/*@modifies s @*/
{
    rpmdsObject * dso;
    rpmfiObject * fio;
    PyObject * key;
    alKey pkgKey;
    char * kwlist[] = {"packageKey", "key", "dso", "fileInfo", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iOO!O!:Add", kwlist,
	    &pkgKey, &key, &rpmds_Type, &dso, &rpmfi_Type, &fio))
	return NULL;

    /* XXX errors */
    /* XXX transaction colors */
    pkgKey = rpmalAdd(&s->al, pkgKey, key, dso->ds, fio->fi, 0);

    return Py_BuildValue("i", pkgKey);
}

/*@null@*/
static PyObject *
rpmal_Del(rpmalObject * s, PyObject * args, PyObject * kwds)
	/*@globals _Py_NoneStruct @*/
	/*@modifies s, _Py_NoneStruct @*/
{
    alKey pkgKey;
    char * kwlist[] = {"key", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:Del", kwlist, &pkgKey))
	return NULL;

    rpmalDel(s->al, pkgKey);

    Py_RETURN_NONE;
}

/*@null@*/
static PyObject *
rpmal_AddProvides(rpmalObject * s, PyObject * args, PyObject * kwds)
	/*@globals _Py_NoneStruct @*/
	/*@modifies s, _Py_NoneStruct @*/
{
    rpmdsObject * dso;
    alKey pkgKey;
    char * kwlist[] = {"index", "packageIndex", "dso", NULL};

    /* XXX: why is there an argument listed in the format string that
     *      isn't handled?  Is that for transaction color? */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iOO!O!:AddProvides", kwlist,
	    &pkgKey, &rpmds_Type, &dso))
	return NULL;

    /* XXX transaction colors */
    rpmalAddProvides(s->al, pkgKey, dso->ds, 0);

    Py_RETURN_NONE;
}

/*@null@*/
static PyObject *
rpmal_MakeIndex(rpmalObject * s)
	/*@globals _Py_NoneStruct @*/
	/*@modifies s, _Py_NoneStruct @*/
{
    rpmalMakeIndex(s->al);

    Py_RETURN_NONE;
}

/*@-fullinitblock@*/
/*@unchecked@*/ /*@observer@*/
static struct PyMethodDef rpmal_methods[] = {
 {"add",	(PyCFunction)rpmal_Add,		METH_VARARGS|METH_KEYWORDS,
	NULL},
 {"delete",	(PyCFunction)rpmal_Del,		METH_VARARGS|METH_KEYWORDS,
	NULL},
 {"addProvides",(PyCFunction)rpmal_AddProvides,	METH_VARARGS|METH_KEYWORDS,
	NULL},
 {"makeIndex",(PyCFunction)rpmal_MakeIndex,	METH_NOARGS,
	NULL},
 {NULL,		NULL }		/* sentinel */
};
/*@=fullinitblock@*/

/* ---------- */

static void
rpmal_dealloc(rpmalObject * s)
	/*@modifies s @*/
{
    if (s) {
	s->al = rpmalFree(s->al);
	PyObject_Del(s);
    }
}

/**
 */
/*@unchecked@*/ /*@observer@*/
static char rpmal_doc[] =
"";

/*@-fullinitblock@*/
/*@unchecked@*/
PyTypeObject rpmal_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"rpm.al",			/* tp_name */
	sizeof(rpmalObject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor) rpmal_dealloc,	/* tp_dealloc */
	(printfunc)0,			/* tp_print */
	(getattrfunc)0,			/* tp_getattr */
	(setattrfunc)0,			/* tp_setattr */
	(cmpfunc)0,			/* tp_compare */
	(reprfunc)0,			/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	(hashfunc)0,			/* tp_hash */
	(ternaryfunc)0,			/* tp_call */
	(reprfunc)0,			/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	PyObject_GenericSetAttr,	/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	rpmal_doc,			/* tp_doc */
#if Py_TPFLAGS_HAVE_ITER
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)0,			/* tp_iter */
	(iternextfunc)0,		/* tp_iternext */
	rpmal_methods,			/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	0,				/* tp_alloc */
	0,				/* tp_new */
	0,				/* tp_free */
	0,				/* tp_is_gc */
#endif
};
/*@=fullinitblock@*/

/* ---------- */

rpmalObject *
rpmal_Wrap(rpmal al)
{
    rpmalObject *s = PyObject_New(rpmalObject, &rpmal_Type);
    if (s == NULL)
	return NULL;
    s->al = al;
    return s;
}
