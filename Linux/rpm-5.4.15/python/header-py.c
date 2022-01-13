/** \ingroup py_c
 * \file python/header-py.c
 */

#include "system-py.h"

#include "rpmio_internal.h"
#include <rpmcb.h>

#include "legacy.h"
#define	_RPMTAG_INTERNAL
#include "header_internal.h"	/* XXX HEADERFLAG_ALLOCATED */
#include "rpmtypes.h"
#define	_RPMEVR_INTERNAL
#include "rpmevr.h"
#include "pkgio.h"		/* XXX rpmpkgRead */

#include "rpmts.h"	/* XXX rpmtsCreate/rpmtsFree */

#include "rpmcli.h"

#include "header-py.h"
#include "rpmds-py.h"
#include "rpmfi-py.h"

#include "debug.h"

/** \ingroup python
 * \class Rpm
 * \brief START HERE / RPM base module for the Python API
 *
 * The rpm base module provides the main starting point for
 * accessing RPM from Python. For most usage, call
 * the TransactionSet method to get a transaction set (rpmts).
 *
 * For example:
 * \code
 *	import rpm
 *
 *	ts = rpm.TransactionSet()
 * \endcode
 *
 * The transaction set will open the RPM database as needed, so
 * in most cases, you do not need to explicitly open the
 * database. The transaction set is the workhorse of RPM.
 *
 * You can open another RPM database, such as one that holds
 * all packages for a given Linux distribution, to provide
 * packages used to solve dependencies. To do this, use
 * the following code:
 *
 * \code
 * rpm.addMacro('_dbpath', '/path/to/alternate/database')
 * solvets = rpm.TransactionSet()
 * solvets.openDB()
 * rpm.delMacro('_dbpath')
 *
 * # Open default database
 * ts = rpm.TransactionSet()
 * \endcode
 *
 * This code gives you access to two RPM databases through
 * two transaction sets (rpmts): ts is a transaction set
 * associated with the default RPM database and solvets
 * is a transaction set tied to an alternate database, which
 * is very useful for resolving dependencies.
 *
 * The rpm methods used here are:
 *
 * - addMacro(macro, value)
 * @param macro   Name of macro to add
 * @param value   Value for the macro
 *
 * - delMacro(macro)
 * @param macro   Name of macro to delete
 *
 */

/** \ingroup python
 * \class Rpmhdr
 * \brief A python rpm.hdr object represents an RPM package header.
 *
 * All RPM packages have headers that provide metadata for the package.
 * Header objects can be returned by database queries or loaded from a
 * binary package on disk.
 *
 * The ts.hdrFromFdno() function returns the package header from a
 * package on disk, verifying package signatures and digests of the
 * package while reading.
 *
 * Note: The older method rpm.headerFromPackage() which has been replaced
 * by ts.hdrFromFdno() used to return a (hdr, isSource) tuple.
 *
 * If you need to distinguish source/binary headers, do:
 * \code
 * 	import os, rpm
 *
 *	ts = rpm.TransactionSet()
 * 	fdno = os.open("/tmp/foo-1.0-1.i386.rpm", os.O_RDONLY)
 * 	hdr = ts.hdrFromFdno(fdno)
 *	os.close(fdno)
 *	if hdr[rpm.RPMTAG_SOURCERPM]:
 *	   print "header is from a binary package"
 *	else:
 *	   print "header is from a source package"
 * \endcode
 *
 * The Python interface to the header data is quite elegant.  It
 * presents the data in a dictionary form.  We'll take the header we
 * just loaded and access the data within it:
 * \code
 * 	print hdr[rpm.RPMTAG_NAME]
 * 	print hdr[rpm.RPMTAG_VERSION]
 * 	print hdr[rpm.RPMTAG_RELEASE]
 * \endcode
 * in the case of our "foo-1.0-1.i386.rpm" package, this code would
 * output:
\verbatim
  	foo
  	1.0
  	1
\endverbatim
 *
 * You make also access the header data by string name:
 * \code
 * 	print hdr['name']
 * 	print hdr['version']
 * 	print hdr['release']
 * \endcode
 *
 * This method of access is slower because the name must be translated
 * into the tag number dynamically. You also must make sure the strings
 * in header lookups don't get translated, or the lookups will fail.
 */

/** \ingroup py_c
 */
struct hdrObject_s {
    PyObject_HEAD
    Header h;
} ;

/**
 */
/*@unused@*/ static inline Header headerAllocated(Header h)
	/*@modifies h @*/
{
    h->flags |= HEADERFLAG_ALLOCATED;
    return 0;
}

static PyObject *hdrIsSource(hdrObject *s)
{
    int isSource = !headerIsEntry(s->h, RPMTAG_SOURCERPM);
    return PyBool_FromLong(isSource);
}

/** \ingroup python
 * \name Class: Rpmhdr
 */
/*@{*/

/**
 */
static PyObject * hdrKeyList(hdrObject * s)
	/*@*/
{
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    PyObject * list, *o;
    HeaderIterator hi;

    list = PyList_New(0);
    if (!list) {
	return NULL;
    }
    for (hi = headerInit(s->h);
	headerNext(hi, he, 0);
	he->p.ptr = _free(he->p.ptr))
    {
        if (he->tag == HEADER_I18NTABLE) continue;
	switch (he->t) {
	case RPM_I18NSTRING_TYPE:
#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(0);
#endif
	    break;
	case RPM_BIN_TYPE:
	case RPM_UINT64_TYPE:
	case RPM_UINT32_TYPE:
	case RPM_UINT16_TYPE:
	case RPM_UINT8_TYPE:
	case RPM_STRING_ARRAY_TYPE:
	case RPM_STRING_TYPE:
            o=PyInt_FromLong(he->tag);
            if (!o) {
                hi = headerFini(hi);
                Py_XDECREF(list);
                return NULL;
            }
	    PyList_Append(list, o);
	    Py_XDECREF(o);
	    break;
	}
    }
    hi = headerFini(hi);

    return list;
}

/**
 */
static PyObject * hdrUnload(hdrObject * s, PyObject * args, PyObject *keywords)
	/*@*/
{
    char * buf;
    PyObject * rc;
    int legacy = 0;
    int nb;
    Header h;
    static char *kwlist[] = { "legacyHeader", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywords, "|i", kwlist, &legacy))
	return NULL;

    h = headerLink(s->h);
    /* XXX this legacy switch is a hack, needs to be removed. */
    if (legacy) {
	h = headerCopy(s->h);	/* XXX strip region tags, etc */
	(void)headerFree(s->h);
	s->h = NULL;
    }
    {	size_t len;
	buf = headerUnload(h, &len);
	nb = len;
	nb -= 8;	/* XXX HEADER_MAGIC_NO */
    }
    (void)headerFree(h);
    h = NULL;

    if (buf == NULL || nb == 0) {
	PyErr_SetString(pyrpmError, "can't unload bad header\n");
	return NULL;
    }

    rc = PyString_FromStringAndSize(buf, nb);
    buf = _free(buf);

    return rc;
}

/**
 */
static PyObject * hdrGetOrigin(hdrObject * s)
	/*@*/
{
    const char * origin = NULL;
    if (s->h != NULL)

	origin = headerGetOrigin(s->h);
    if (origin != NULL)
	return Py_BuildValue("s", origin);
    Py_RETURN_NONE;
}

/**
 */
static PyObject * hdrSetOrigin(hdrObject * s, PyObject * args, PyObject * kwds)
	/*@*/
{
    char * kwlist[] = {"origin", NULL};
    const char * origin = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s:SetOrigin", kwlist, &origin))
        return NULL;

    if (s->h != NULL && origin != NULL)
	headerSetOrigin(s->h, origin);

    Py_RETURN_NONE;
}

/**
 */
static PyObject * hdrSprintf(hdrObject * s, PyObject * args, PyObject * kwds)
	/*@*/
{
    char * fmt;
    char * r;
    errmsg_t err;
    PyObject * result;
    char * kwlist[] = {"format", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &fmt))
	return NULL;

    r = headerSprintf(s->h, fmt, NULL, rpmHeaderFormats, &err);
    if (!r) {
	PyErr_SetString(pyrpmError, err);
	return NULL;
    }

    result = Py_BuildValue("s", r);
    r = _free(r);

    return result;
}

/*@}*/

/** \ingroup py_c
 */
/*@unchecked@*/ /*@observer@*/
static struct PyMethodDef hdr_methods[] = {
    {"keys",		(PyCFunction) hdrKeyList,	METH_NOARGS,
	NULL },
    {"unload",		(PyCFunction) hdrUnload,	METH_VARARGS|METH_KEYWORDS,
	NULL },
    {"getorigin",	(PyCFunction) hdrGetOrigin,	METH_NOARGS,
	NULL },
    {"setorigin",	(PyCFunction) hdrSetOrigin,	METH_VARARGS|METH_KEYWORDS,
	NULL },
    {"sprintf",		(PyCFunction) hdrSprintf,	METH_VARARGS|METH_KEYWORDS,
	NULL },
    {"isSource",	(PyCFunction)hdrIsSource,	METH_NOARGS, 
	NULL },

    {"dsOfHeader",	(PyCFunction)hdr_dsOfHeader,	METH_NOARGS,
	NULL},
    {"dsFromHeader",	(PyCFunction)hdr_dsFromHeader,	METH_VARARGS|METH_KEYWORDS,
	NULL},
    {"fiFromHeader",	(PyCFunction)hdr_fiFromHeader,	METH_VARARGS|METH_KEYWORDS,
	NULL},

    {NULL,		NULL}		/* sentinel */
};

/**
 */
static int hdr_compare(hdrObject * a, hdrObject * b)
	/*@*/
{
    return rpmVersionCompare(a->h, b->h);
}

static long hdr_hash(PyObject * h)
{
    return (long) h;
}

/** \ingroup py_c
 */
static void hdr_dealloc(hdrObject * s)
	/*@*/
{
    if (s->h) (void ) headerFree(s->h);
    s->h = NULL;
    PyObject_Del(s);
}

/** \ingroup py_c
 */
rpmTag tagNumFromPyObject (PyObject *item)
{
    char * str;

    if (PyInt_Check(item)) {
	return (rpmTag) PyInt_AsLong(item);
    } else if (PyString_Check(item) || PyUnicode_Check(item)) {
	str = PyString_AsString(item);
	return tagValue(str);
    }
    return (rpmTag)0xffffffff;
}

/** \ingroup py_c
 */
static PyObject * hdr_subscript(hdrObject * s, PyObject * item)
	/*@*/
{
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmTag tag = (rpmTag)0xffffffff;
    uint32_t i;
    PyObject * o, * metao;
    int forceArray = 0;
    const struct headerSprintfExtension_s * ext = NULL;
    int xx;

    if (PyCObject_Check (item))
        ext = PyCObject_AsVoidPtr(item);
    else
	tag = tagNumFromPyObject (item);

    if (tag == (rpmTag)0xffffffff && (PyString_Check(item) || PyUnicode_Check(item))) {
	const struct headerSprintfExtension_s * extensions = rpmHeaderFormats;
	char * str;
	/* if we still don't have the tag, go looking for the header
	   extensions */
	str = PyString_AsString(item);
	while (extensions->name) {
	    if (extensions->type == HEADER_EXT_TAG
	     && !xstrcasecmp(extensions->name + 7, str)) {
		ext = extensions;
	    }
	    extensions++;
	    if (extensions->type == HEADER_EXT_MORE)
		extensions = *extensions->u.more;
	}
    }

    /* Retrieve data from extension or header. */
    if (ext) {
        ext->u.tagFunction(s->h, he);
    } else {
        if (tag == (rpmTag)0xffffffff) {
            PyErr_SetString(PyExc_KeyError, "unknown header tag");
            return NULL;
        }
        
	he->tag = tag;
	xx = headerGet(s->h, he, 0);
	if (!xx) {
	    he->p.ptr = _free(he->p.ptr);
	    switch (tag) {
	    case RPMTAG_EPOCH:
	    case RPMTAG_NAME:
	    case RPMTAG_VERSION:
	    case RPMTAG_RELEASE:
	    case RPMTAG_DISTEPOCH:
	    case RPMTAG_ARCH:
	    case RPMTAG_OS:
		Py_RETURN_NONE;
		break;
	    default:
		return PyList_New(0);
		break;
	    }
	}
    }

    switch (tag) {
    case RPMTAG_FILEPATHS:
    case RPMTAG_ORIGPATHS:
    case RPMTAG_OLDFILENAMES:
    case RPMTAG_FILESIZES:
    case RPMTAG_FILESTATES:
    case RPMTAG_FILEMODES:
    case RPMTAG_FILEUIDS:
    case RPMTAG_FILEGIDS:
    case RPMTAG_FILERDEVS:
    case RPMTAG_FILEMTIMES:
    case RPMTAG_FILEMD5S:
    case RPMTAG_FILELINKTOS:
    case RPMTAG_FILEFLAGS:
    case RPMTAG_ROOT:
    case RPMTAG_FILEUSERNAME:
    case RPMTAG_FILEGROUPNAME:
    case RPMTAG_REQUIRENAME:
    case RPMTAG_REQUIREFLAGS:
    case RPMTAG_REQUIREVERSION:
    case RPMTAG_PROVIDENAME:
    case RPMTAG_PROVIDEFLAGS:
    case RPMTAG_PROVIDEVERSION:
    case RPMTAG_OBSOLETENAME:
    case RPMTAG_OBSOLETEFLAGS:
    case RPMTAG_OBSOLETEVERSION:
    case RPMTAG_CONFLICTNAME:
    case RPMTAG_CONFLICTFLAGS:
    case RPMTAG_CONFLICTVERSION:
    case RPMTAG_CHANGELOGTIME:
    case RPMTAG_FILEVERIFYFLAGS:
	forceArray = 1;
	break;
    default:
        break;
    }

    switch (he->t) {
    case RPM_BIN_TYPE:
	o = PyString_FromStringAndSize(he->p.str, he->c);
	break;

    case RPM_UINT8_TYPE:
	if (he->c != 1 || forceArray) {
	    metao = PyList_New(0);
            if (!metao) {
             return NULL;
            }
	    for (i = 0; i < he->c; i++) {
		o = PyInt_FromLong(he->p.ui8p[i]);
                if (!o) {
                   Py_XDECREF(metao);
                   return NULL;
                }
		PyList_Append(metao, o);
		Py_XDECREF(o);
	    }
	    o = metao;
	} else {
	    o = PyInt_FromLong(he->p.ui8p[0]);
	}
	break;

    case RPM_UINT16_TYPE:
	if (he->c != 1 || forceArray) {
	    metao = PyList_New(0);
            if (!metao) {
              return NULL;
	    }
	    for (i = 0; i < he->c; i++) {
		o = PyInt_FromLong(he->p.ui16p[i]);
                if (!o) {
                   Py_XDECREF(metao);
                   return NULL;
                }
		PyList_Append(metao, o);
		Py_XDECREF(o);
	    }
	    o = metao;
	} else {
	    o = PyInt_FromLong(he->p.ui16p[0]);
	}
	break;

    case RPM_UINT32_TYPE:
	if (he->c != 1 || forceArray) {
	    metao = PyList_New(0);
            if (!metao) {
       		return NULL;
            }
	    for (i = 0; i < he->c; i++) {
		o = PyInt_FromLong(he->p.ui32p[i]);
                if (!o) {
                   Py_XDECREF(metao);
                   return NULL;
                }
		PyList_Append(metao, o);
		Py_XDECREF(o);
	    }
	    o = metao;
	} else {
	    o = PyInt_FromLong(he->p.ui32p[0]);
	}
	break;

    case RPM_UINT64_TYPE:
	if (he->c != 1 || forceArray) {
	    metao = PyList_New(0);
            if (!metao) {
       		return NULL;
            }
	    for (i = 0; i < he->c; i++) {
		o = PyInt_FromLong(he->p.ui64p[i]);
                if (!o) {
                   Py_XDECREF(metao);
                   return NULL;
                }
		PyList_Append(metao, o);
		Py_XDECREF(o);
	    }
	    o = metao;
	} else {
	    o = PyInt_FromLong(he->p.ui64p[0]);
	}
	break;

    case RPM_STRING_ARRAY_TYPE:
	metao = PyList_New(0);
        if (!metao) {
       	   return NULL;
        }
	for (i = 0; i < he->c; i++) {
	    o = PyString_FromString(he->p.argv[i]);
            if (!o) {
                  Py_XDECREF(metao);
                  return NULL;
            }
	    PyList_Append(metao, o);
	    Py_XDECREF(o);
	}
	o = metao;
	break;

    case RPM_STRING_TYPE:
	if (he->p.str != NULL)
	    o = PyString_FromString(he->p.str);
	else
	    o = PyString_FromString("");
	break;

    default:
	PyErr_SetString(PyExc_TypeError, "unsupported type in header");
	return NULL;
    }
    if (he->freeData)
	he->p.ptr = _free(he->p.ptr);

    return o;
}

/** \ingroup py_c
 */
/*@unchecked@*/ /*@observer@*/
static PyMappingMethods hdr_as_mapping = {
	(lenfunc) 0,			/* mp_length */
	(binaryfunc) hdr_subscript,	/* mp_subscript */
	(objobjargproc) 0,		/* mp_ass_subscript */
};

static PyObject * hdr_getattro(hdrObject * o, PyObject * n)
	/*@*/
{
    PyObject * res;
    res = PyObject_GenericGetAttr((PyObject *)o, n);
    if (res == NULL)
	res = hdr_subscript(o, n);
    return res;
}

static int hdr_setattro(hdrObject * o, PyObject * n, PyObject * v)
	/*@*/
{
    return PyObject_GenericSetAttr((PyObject *)o, n, v);
}

/**
 */
static char hdr_doc[] =
"";

/** \ingroup py_c
 */
/*@unchecked@*/ /*@observer@*/
PyTypeObject hdr_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"rpm.hdr",			/* tp_name */
	sizeof(hdrObject),		/* tp_size */
	0,				/* tp_itemsize */
	(destructor) hdr_dealloc, 	/* tp_dealloc */
	0,				/* tp_print */
	(getattrfunc) 0, 		/* tp_getattr */
	0,				/* tp_setattr */
	(cmpfunc) hdr_compare,		/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,	 			/* tp_as_sequence */
	&hdr_as_mapping,		/* tp_as_mapping */
	hdr_hash,			/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	(getattrofunc) hdr_getattro,	/* tp_getattro */
	(setattrofunc) hdr_setattro,	/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	hdr_doc,			/* tp_doc */
#if Py_TPFLAGS_HAVE_ITER
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iternext */
	hdr_methods,			/* tp_methods */
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

hdrObject * hdr_Wrap(Header h)
{
    hdrObject * hdr = PyObject_New(hdrObject, &hdr_Type);
    hdr->h = headerLink(h);
    return hdr;
}

Header hdrGetHeader(hdrObject * s)
{
    return s->h;
}

/**
 */
PyObject * hdrLoad(PyObject * self, PyObject * args, PyObject * kwds)
{
    hdrObject * hdr;
    char * copy = NULL;
    char * obj;
    Header h;
    int len;
    char * kwlist[] = {"headers", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s#", kwlist, &obj, &len))
	return NULL;

    /* malloc is needed to avoid surprises from data swab in headerLoad(). */
    copy = malloc(len);
    if (copy == NULL) {
	PyErr_SetString(pyrpmError, "out of memory");
	return NULL;
    }
    memcpy (copy, obj, len);

    h = headerLoad(copy);
    if (!h) {
	PyErr_SetString(pyrpmError, "bad header");
	return NULL;
    }
    headerAllocated(h);

    hdr = hdr_Wrap(h);
    (void)headerFree(h);	/* XXX ref held by hdr */
    h = NULL;

    return (PyObject *) hdr;
}

/**
 */
PyObject * rpmReadHeaders (FD_t fd)
{
    PyObject * list;
    Header h;
    hdrObject * hdr;

    if (!fd) {
	PyErr_SetFromErrno(pyrpmError);
	return NULL;
    }

    list = PyList_New(0);
    if (!list) {
    	return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    {   const char item[] = "Header";
	const char * msg = NULL;
	rpmRC rc = rpmpkgRead(item, fd, &h, &msg);
	if(rc == RPMRC_NOTFOUND) {
		Py_INCREF(Py_None);
		list = Py_None;
	}
	else if (rc != RPMRC_OK)
	    rpmlog(RPMLOG_ERR, "%s: %s: %s : error code: %d\n", "rpmpkgRead", item, msg, rc);
	msg = _free(msg);
    }
    Py_END_ALLOW_THREADS

    while (h) {
	hdr = hdr_Wrap(h);
	if (PyList_Append(list, (PyObject *) hdr)) {
	    Py_XDECREF(list);
	    Py_XDECREF(hdr);
	    return NULL;
	}
	Py_XDECREF(hdr);

	(void)headerFree(h);	/* XXX ref held by hdr */
	h = NULL;

	Py_BEGIN_ALLOW_THREADS
	{   const char item[] = "Header";
	    const char * msg = NULL;
	    rpmRC rc = rpmpkgRead(item, fd, &h, &msg);
	    if(rc == RPMRC_NOTFOUND) {
		    Py_INCREF(Py_None);
		    list = Py_None;
	    }
	    else if (rc != RPMRC_OK)
		rpmlog(RPMLOG_ERR, "%s: %s: %s : error code: %d\n", "rpmpkgRead", item, msg, rc);
	    msg = _free(msg);
	}
	Py_END_ALLOW_THREADS
    }

    return list;
}

/**
 */
PyObject * rpmHeaderFromFD(PyObject * self, PyObject * args,
		PyObject * kwds)
{
    FD_t fd;
    int fileno;
    PyObject * list;
    char * kwlist[] = {"fd", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &fileno))
	return NULL;

    fd = fdDup(fileno);

    list = rpmReadHeaders (fd);
    Fclose(fd);

    return list;
}

/**
 */
PyObject * rpmHeaderFromFile(PyObject * self, PyObject * args,
		PyObject *kwds)
{
    char * filespec;
    FD_t fd;
    PyObject * list;
    char * kwlist[] = {"file", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &filespec))
	return NULL;

    fd = Fopen(filespec, "r.fdio");

    if (!fd) {
	PyErr_SetFromErrno(pyrpmError);
	return NULL;
    }

    list = rpmReadHeaders (fd);
    Fclose(fd);

    return list;
}

/**
 */
PyObject *
rpmSingleHeaderFromFD(PyObject * self, PyObject * args,
		PyObject * kwds)
{
    FD_t fd;
    int fileno;
    off_t offset;
    PyObject * tuple;
    Header h;
    char * kwlist[] = {"fd", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &fileno))
	return NULL;

    offset = lseek(fileno, 0, SEEK_CUR);

    fd = fdDup(fileno);

    if (!fd) {
	PyErr_SetFromErrno(pyrpmError);
	return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    {   const char item[] = "Header";
	const char * msg = NULL;
	rpmRC rc = rpmpkgRead(item, fd, &h, &msg);
	if(rc == RPMRC_NOTFOUND) {
		Py_INCREF(Py_None);
		tuple = Py_None;
	}
	else if (rc != RPMRC_OK)
	    rpmlog(RPMLOG_ERR, "%s: %s: %s : error code: %d\n", "rpmpkgRead", item, msg, rc);
	msg = _free(msg);
    }
    Py_END_ALLOW_THREADS

    Fclose(fd);

    tuple = PyTuple_New(2);

    if (h && tuple) {
	PyTuple_SET_ITEM(tuple, 0, (PyObject *) hdr_Wrap(h));
	PyTuple_SET_ITEM(tuple, 1, PyLong_FromLong(offset));
	(void)headerFree(h);
	h = NULL;
    } else {
	Py_INCREF(Py_None);
	Py_INCREF(Py_None);
	PyTuple_SET_ITEM(tuple, 0, Py_None);
	PyTuple_SET_ITEM(tuple, 1, Py_None);
    }

    return tuple;
}

/**
 */
PyObject * rpmWriteHeaders (PyObject * list, FD_t fd)
{
    int count;

    if (!fd) {
	PyErr_SetFromErrno(pyrpmError);
	return NULL;
    }

    for(count = 0; count < PyList_Size(list); count++){
	Py_BEGIN_ALLOW_THREADS
	const char item[] = "Header";
	const char * msg = NULL;
	hdrObject * hdr = (hdrObject *)PyList_GetItem(list, count);
	rpmRC rc = rpmpkgWrite(item, fd, hdr->h, &msg);
	if (rc != RPMRC_OK)
	    rpmlog(RPMLOG_ERR, "%s: %s: %s : error code: %d\n", "rpmpkgWrite", item, msg, rc);
	msg = _free(msg);
	Py_END_ALLOW_THREADS
    }
    
    Py_RETURN_TRUE;
}

/**
 */
PyObject * rpmHeaderToFD(PyObject * self, PyObject * args,
		PyObject * kwds)
{
    FD_t fd;
    int fileno;
    PyObject * list;
    PyObject * ret;
    char * kwlist[] = {"headers", "fd", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist, &list, &fileno))
	return NULL;

    fd = fdDup(fileno);

    ret = rpmWriteHeaders (list, fd);
    Fclose(fd);

    return list;
}

/**
 */
PyObject * rpmHeaderToFile(PyObject * self, PyObject * args,
		PyObject *kwds)
{
    char * filespec;
    FD_t fd;
    PyObject * list;
    PyObject * ret;
    char * kwlist[] = {"headers", "file", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Os", kwlist, &list, &filespec))
	return NULL;

    fd = Fopen(filespec, "w.fdio");
    if (!fd) {
	PyErr_SetFromErrno(pyrpmError);
	return NULL;
    }

    ret = rpmWriteHeaders (list, fd);
    Fclose(fd);

    return ret; 
} 

/**
 */
PyObject * versionCompare (PyObject * self, PyObject * args,
		PyObject * kwds)
{
    hdrObject * h1, * h2;
    char * kwlist[] = {"version0", "version1", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!O!", kwlist, &hdr_Type,
	    &h1, &hdr_Type, &h2))
	return NULL;

    return Py_BuildValue("i", hdr_compare(h1, h2));
}

PyObject * labelCompare (PyObject * self, PyObject * args)
{
    EVR_t a = rpmEVRnew(RPMSENSE_EQUAL, 1);
    EVR_t b = rpmEVRnew(RPMSENSE_EQUAL, 1);
    int rc;
    PyObject *aTuple, *bTuple;

    if (!PyArg_ParseTuple(args, "OO", &aTuple, &bTuple) ||
	    !PyArg_ParseTuple(aTuple, "zzz|z",
		&a->F[RPMEVR_E], &a->F[RPMEVR_V], &a->F[RPMEVR_R], &a->F[RPMEVR_D]) ||
	    !PyArg_ParseTuple(bTuple, "zzz|z",
		&b->F[RPMEVR_E], &b->F[RPMEVR_V], &b->F[RPMEVR_R], &b->F[RPMEVR_D]))
    {
	a = rpmEVRfree(a);
	b = rpmEVRfree(b);
	return NULL;
    }

    /* XXX HACK: postpone committing to single "missing" value for now. */
    if (a->F[RPMEVR_E] == NULL)	a->F[RPMEVR_E] = "0";
    if (b->F[RPMEVR_E] == NULL)	b->F[RPMEVR_E] = "0";
    if (a->F[RPMEVR_V] == NULL)	a->F[RPMEVR_V] = "";
    if (b->F[RPMEVR_V] == NULL)	b->F[RPMEVR_V] = "";
    if (a->F[RPMEVR_R] == NULL)	a->F[RPMEVR_R] = "";
    if (b->F[RPMEVR_R] == NULL)	b->F[RPMEVR_R] = "";
    if (a->F[RPMEVR_D] == NULL)	a->F[RPMEVR_D] = "";
    if (b->F[RPMEVR_D] == NULL)	b->F[RPMEVR_D] = "";

    rc = rpmEVRcompare(a, b);

    a = rpmEVRfree(a);
    b = rpmEVRfree(b);

    return Py_BuildValue("i", rc);
}

PyObject * evrCompare (PyObject * self, PyObject * args,
		PyObject * kwds)
{
    EVR_t lEVR = NULL;
    EVR_t rEVR = NULL;
    int rc;
    char * evr1, * evr2;
    char * kwlist[] = {"evr0", "evr1", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ss", kwlist, &evr1, &evr2))
	return NULL;

    lEVR = rpmEVRnew(RPMSENSE_EQUAL, 0);
    rEVR = rpmEVRnew(RPMSENSE_EQUAL, 0);
    rpmEVRparse(evr1, lEVR);
    rpmEVRparse(evr2, rEVR);
    rc = rpmEVRcompare(lEVR, rEVR);
    lEVR = rpmEVRfree(lEVR);
    rEVR = rpmEVRfree(rEVR);

    return PyLong_FromLong(rc);
}

PyObject * evrSplit (PyObject * self, PyObject * args, PyObject * kwds)
{
    EVR_t EVR = rpmEVRnew(RPMSENSE_EQUAL, 0);
    char * evr;
    char * kwlist[] = {"evr", NULL};
    PyObject * tuple = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &evr))
	goto exit;

    rpmEVRparse(evr, EVR);
    tuple = Py_BuildValue("(Isss)", EVR->F[RPMEVR_E] ? atoi(EVR->F[RPMEVR_E]) : 0, EVR->F[RPMEVR_V], EVR->F[RPMEVR_R], EVR->F[RPMEVR_D]);

exit:
    EVR = rpmEVRfree(EVR);

    return tuple;
}
