/** \ingroup py_c
 * \file python/rpmtd-py.c
 */

#include "system-py.h"

#include <rpmio.h>	/* XXX header-py.h needs FD_t typedef */

#define RPM_NULL_TYPE           0
#define RPM_INT8_TYPE           RPM_UINT8_TYPE
#define RPM_INT16_TYPE          RPM_UINT16_TYPE
#define RPM_INT32_TYPE          RPM_UINT32_TYPE
#define RPM_INT64_TYPE          RPM_UINT64_TYPE

#define rpmTagGetType(_tag)     tagType(_tag)

#define	_RPMTD_INTERNAL
#include "rpmtd-py.h"
#include "header-py.h"

#include "debug.h"

/** \ingroup python
 * \class Rpmtd
 * \brief A python rpm.td tag data container object represents header / 
 *        extension tag data.
 *
 */

/** \ingroup python
 * \name Class: Rpmtd
 */

static PyObject *rpmtd_iter(rpmtdObject *self)
{
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *rpmtd_iternext(rpmtdObject *self)
{
    PyObject *next = NULL;

    if (rpmtdNext(self->td) >= 0) {
	Py_INCREF(self);
	next = (PyObject*) self;
    } 
    return next;
}

static PyObject *rpmtd_str(rpmtdObject *self)
{
    char *str = rpmtdFormat(self->td, RPMTD_FORMAT_STRING, NULL);
    if (str) {
	return PyString_FromString(str);
    } else {
	Py_RETURN_NONE;
    }
}

/*
 * Convert single tag data item to python object of suitable type
 */
PyObject * rpmtd_ItemAsPyobj(rpmtd td)
{
    PyObject *res = NULL;
    char *str = NULL;

    switch (rpmtdType(td)) {
    case RPM_I18NSTRING_TYPE:
#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(0);
#endif
    case RPM_STRING_TYPE:
    case RPM_STRING_ARRAY_TYPE:
	res = PyString_FromString(rpmtdGetString(td));
	break;
    case RPM_INT64_TYPE:
	res = PyLong_FromLongLong(*rpmtdGetUint64(td));
	break;
    case RPM_INT32_TYPE:
	res = PyInt_FromLong(*rpmtdGetUint32(td));
	break;
    case RPM_INT16_TYPE:
	res = PyInt_FromLong(*rpmtdGetUint16(td));
	break;
    case RPM_BIN_TYPE:
	str = rpmtdFormat(td, RPMTD_FORMAT_STRING, NULL);
	res = PyString_FromString(str);
	free(str);
	break;
    default:
	PyErr_SetString(PyExc_KeyError, "unhandled data type");
	break;
    }
    return res;
}

PyObject *rpmtd_AsPyobj(rpmtd td)
{
    PyObject *res = NULL;
    rpmTagType type = rpmTagGetType(rpmtdTag(td));
    int array = ((type & RPM_MASK_RETURN_TYPE) == RPM_ARRAY_RETURN_TYPE);

    if (!array && rpmtdCount(td) < 1) {
	Py_RETURN_NONE;
    }
    
    if (array) {
	res = PyList_New(0);
        if (!res) {
            return NULL;
        }
	while (rpmtdNext(td) >= 0) {
	    PyList_Append(res, rpmtd_ItemAsPyobj(td));
	}
    } else {
	res = rpmtd_ItemAsPyobj(td);
    }
    return res;
}

static PyObject *rpmtd_Format(rpmtdObject *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"fmt", NULL};
    char *str;
    rpmtdFormats fmt;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &fmt))
        return NULL;

    str = rpmtdFormat(self->td, fmt, NULL);
    if (str) {
	return PyString_FromString(str);
    }
    Py_RETURN_NONE;
}

static PyObject *rpmtd_setTag(rpmtdObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pytag;
    char *kwlist[] = {"tag", NULL};
    rpmTag tag;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &pytag))
	return NULL;

    tag = tagNumFromPyObject(pytag);
#ifdef	DYING	/* XXX with arbitrary tags, all tags are found. */
    if (tag == RPMTAG_NOT_FOUND) {
	PyErr_SetString(PyExc_KeyError, "unknown header tag");
	return NULL;
    }
#endif
    
    /* tag got just validated, so settag failure must be from type mismatch */
    if (!rpmtdSetTag(self->td, tag)) {
	PyErr_SetString(PyExc_TypeError, "tag type incompatible with data");
	return NULL;
    }
    Py_RETURN_TRUE;
}

/** \ingroup py_c
 */
static struct PyMethodDef rpmtd_methods[] = {
    {"format",	    (PyCFunction) rpmtd_Format,	METH_VARARGS|METH_KEYWORDS,
	NULL },
    {"setTag",	    (PyCFunction) rpmtd_setTag,	METH_VARARGS|METH_KEYWORDS,
	NULL },
    {NULL,		NULL}		/* sentinel */
};

static int rpmtd_length(rpmtdObject *self)
{
    return rpmtdCount(self->td);
}

/** \ingroup py_c
 */
static void rpmtd_dealloc(rpmtdObject * s)
{
    if (s) {
	rpmtdFreeData(s->td);
	rpmtdFree(s->td);
	PyObject_Del(s);
    }
}

static PyObject *rpmtd_new(PyTypeObject *subtype, 
			   PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"tag", NULL};
    PyObject *pytag;
    rpmTag tag;
    rpmtd td;
    rpmtdObject *self = PyObject_New(rpmtdObject, subtype);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &pytag))
	return NULL;

    tag = tagNumFromPyObject(pytag);
#ifdef	DYING	/* XXX with arbitrary tags, all tags are found. */
    if (tag == RPMTAG_NOT_FOUND) {
	PyErr_SetString(PyExc_TypeError, 
			"can't create container for unknown tag");
	return NULL;
    }
#endif

    td = rpmtdNew();
    td->tag = tag;
    td->type = rpmTagGetType(tag) & RPM_MASK_TYPE;

    self->td = td;
    return (PyObject *)self;
}

/**
 */
static char rpmtd_doc[] =
"";

static PyMappingMethods rpmtd_as_mapping = {
    (lenfunc) rpmtd_length,		/* mp_length */
};

/** \ingroup py_c
 */
PyTypeObject rpmtd_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"rpm.td",			/* tp_name */
	sizeof(rpmtdObject),		/* tp_size */
	0,				/* tp_itemsize */
	(destructor) rpmtd_dealloc, 	/* tp_dealloc */
	0,				/* tp_print */
	(getattrfunc)0, 		/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	&rpmtd_as_mapping,		/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	(reprfunc)rpmtd_str,		/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	PyObject_GenericSetAttr,	/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	rpmtd_doc,			/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc) rpmtd_iter,	/* tp_iter */
	(iternextfunc) rpmtd_iternext,	/* tp_iternext */
	rpmtd_methods,			/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	0,				/* tp_alloc */
	rpmtd_new,			/* tp_new */
	0,				/* tp_free */
	0,				/* tp_is_gc */
};

rpmtdObject * rpmtd_Wrap(rpmtd td)
{
    rpmtdObject * tdo = (rpmtdObject *) PyObject_New(rpmtdObject, &rpmtd_Type);

    if (tdo) {
    	tdo->td = td;
    } else {
        PyErr_SetString(PyExc_MemoryError, "out of memory creating rpmtd");
    }
    return tdo;
}

