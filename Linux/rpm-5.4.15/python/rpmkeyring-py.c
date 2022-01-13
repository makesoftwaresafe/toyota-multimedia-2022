/** \ingroup py_c
 * \file python/rpmkeyring-py.c
 */

#include "system-py.h"

#include <rpmio.h>
#include <rpmpgp.h>

#include "rpmkeyring-py.h"

#include "debug.h"

/** \ingroup python
 * \class RpmKeyring
 */

/** \ingroup py_c
 */
static void rpmPubkey_dealloc(rpmPubkeyObject * self)
	/*@*/
{
    if (self) {
	self->pubkey = rpmPubkeyFree(self->pubkey);
	PyObject_Del(self);
    }
}

static PyObject *rpmPubkey_new(PyTypeObject *subtype, 
			   PyObject *args, PyObject *kwds)
	/*@*/
{
    PyObject *arg;
    char *kwlist[] = { "keypath", NULL };
    rpmPubkeyObject *self; 
    rpmPubkey pubkey = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &arg))
	return NULL;

    if (PyString_Check(arg)) {
	pubkey = rpmPubkeyRead(PyString_AsString(arg));
    } else {
	PyErr_SetString(PyExc_TypeError, "string expected");
	return NULL;
    }
    if (pubkey == NULL) {
	PyErr_SetString(PyExc_TypeError, "failure creating pubkey");
	return NULL;
    }

    self = PyObject_New(rpmPubkeyObject, subtype);
    self->pubkey = pubkey;
    return (PyObject *)self;
}


/*@unchecked@*/
static struct PyMethodDef rpmPubkey_methods[] = {
    {NULL,		NULL}		/* sentinel */
};

/*@unchecked@*/
static char rpmPubkey_doc[] = "";

/** \ingroup py_c
 */
static void rpmKeyring_dealloc(rpmKeyringObject * self)
	/*@*/
{
    if (self) {
	rpmKeyringFree(self->keyring);
	PyObject_Del(self);
    }
}

static PyObject *rpmKeyring_new(PyTypeObject *subtype, 
			   PyObject *args, PyObject *kwds)
	/*@*/
{
    rpmKeyringObject *self = PyObject_New(rpmKeyringObject, subtype);
    rpmKeyring keyring;

    keyring = rpmKeyringNew();

    self->keyring = keyring;
    return (PyObject *)self;
}

static PyObject *rpmKeyring_addKey(rpmKeyringObject *self, 
				   PyObject *args, PyObject *kwds)
	/*@*/
{
    int rc = -1;
    rpmPubkeyObject *pubkey;
    char *kwlist[] = { "key", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &pubkey))
	return NULL;

    /* XXX TODO check pubkey type */
    if (!PyObject_TypeCheck(pubkey, &rpmPubkey_Type)) {
	PyErr_SetString(PyExc_TypeError, "pubkey expected");
	return NULL;
    }

    rc = rpmKeyringAddKey(self->keyring, pubkey->pubkey);
    return PyInt_FromLong(rc);
};

/** \ingroup py_c
 */
/*@unchecked@*/
static struct PyMethodDef rpmKeyring_methods[] = {
    { "addKey", (PyCFunction) rpmKeyring_addKey, METH_VARARGS|METH_KEYWORDS,
        NULL },

    {NULL,		NULL}		/* sentinel */
};
/**
 */
/*@unchecked@*/
static char rpmKeyring_doc[] =
"";

/*@unchecked@*/
PyTypeObject rpmPubkey_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"rpm.pubkey",			/* tp_name */
	sizeof(rpmPubkeyObject),	/* tp_size */
	0,				/* tp_itemsize */
	(destructor) rpmPubkey_dealloc,/* tp_dealloc */
	0,				/* tp_print */
	(getattrfunc)0, 		/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	PyObject_GenericSetAttr,	/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	rpmPubkey_doc,			/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iternext */
	rpmPubkey_methods,		/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	0,				/* tp_alloc */
	rpmPubkey_new,			/* tp_new */
	0,				/* tp_free */
	0,				/* tp_is_gc */
};

/** \ingroup py_c
 */
/*@unchecked@*/
PyTypeObject rpmKeyring_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"rpm.keyring",			/* tp_name */
	sizeof(rpmKeyringObject),	/* tp_size */
	0,				/* tp_itemsize */
	(destructor) rpmKeyring_dealloc,/* tp_dealloc */
	0,				/* tp_print */
	(getattrfunc)0, 		/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	PyObject_GenericSetAttr,	/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	rpmKeyring_doc,			/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iternext */
	rpmKeyring_methods,		/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	0,				/* tp_alloc */
	rpmKeyring_new,			/* tp_new */
	0,				/* tp_free */
	0,				/* tp_is_gc */
};

rpmKeyringObject * rpmKeyring_Wrap(rpmKeyring keyring)
{
    rpmKeyringObject *ko = (rpmKeyringObject *) PyObject_New(rpmKeyringObject, &rpmKeyring_Type);

    if (ko) {
    	ko->keyring = keyring;
    } else {
        PyErr_SetString(PyExc_MemoryError, "out of memory creating keyring");
    }
    return ko;
}
