/** \ingroup py_c
 * \file python/rpmmodule.c
 */

#include "system-py.h"

#include <rpmio_internal.h>
#include <rpmcb.h>
#include <rpmsq.h>
#include <argv.h>

#define	_RPMTAG_INTERNAL
#include <rpmtag.h>
#define _RPMEVR_INTERNAL
#include <rpmevr.h>
#include <rpmdb.h>
#include <rpmcli.h>	/* XXX for rpmCheckSig */

#include "legacy.h"
#include "misc.h"

#include "header-py.h"
#include "rpmal-py.h"
#include "rpmds-py.h"
#include "rpmfd-py.h"
#include "rpmfts-py.h"
#include "rpmfi-py.h"
#include "rpmkeyring-py.h"
#include "rpmmacro-py.h"
#include "rpmmi-py.h"
#include "rpmps-py.h"
#include "rpmtd-py.h"
#include "rpmte-py.h"
#include "rpmts-py.h"
#include "spec-py.h"

#include "debug.h"

#ifdef __LCLINT__
#undef	PyObject_HEAD
#define	PyObject_HEAD	int _PyObjectHead
#endif

/** \ingroup python
 * \name Module: rpm
 */
/*@{*/

/**
 */
PyObject * pyrpmError;

extern sigset_t rpmsqCaught;

/**
 *  */
static PyObject * archScore(PyObject * s, PyObject * args,
		PyObject * kwds)
{
    char * arch;
    char * platform;
    int score;
    char * kwlist[] = {"arch", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &arch))
	return NULL;

#if defined(RPM_VENDOR_WINDRIVER)
    platform = rpmExpand(arch, "-%{_host_vendor}", "-%{_host_os}%{?_gnu}", NULL);
#else
    platform = rpmExpand(arch, "-", "%{_vendor}", "-", "%{_os}", NULL);
#endif
    score = rpmPlatformScore(platform, NULL, 0);
    platform = _free(platform);

    return Py_BuildValue("i", score);
}

/**
 *  */
static PyObject * platformScore(PyObject * s, PyObject * args,
		PyObject * kwds)
{
    char * platform;
    int score;
    char * kwlist[] = {"platform", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &platform))
	return NULL;

    score = rpmPlatformScore(platform, NULL, 0);

    return Py_BuildValue("i", score);
}

/**
 */
static PyObject * signalsCaught(PyObject * s, PyObject * check)
{
    PyObject *caught, *o;
    Py_ssize_t llen;
    int signum, i;
    sigset_t newMask, oldMask;

    if (!PyList_Check(check)) {
	PyErr_SetString(PyExc_TypeError, "list expected");
	return NULL;
    }

    llen = PyList_Size(check);
    caught = PyList_New(0);
    if (!caught) {
	return NULL;
    }
    /* block signals while checking for them */
    (void) sigfillset(&newMask);
    (void) sigprocmask(SIG_BLOCK, &newMask, &oldMask);

    for (i = 0; i < llen; i++) {
	o = PyList_GetItem(check, i);
	signum = PyInt_AsLong(o);
	if (sigismember(&rpmsqCaught, signum)) {
	    PyList_Append(caught, o);
	}
    }
    (void) sigprocmask(SIG_SETMASK, &oldMask, NULL);

    return caught;
}

/**
 */
static PyObject * checkSignals(PyObject * s, PyObject * args)
{
    if (!PyArg_ParseTuple(args, ":checkSignals")) return NULL;
    rpmdbCheckSignals();
    Py_RETURN_NONE;
}

/**
 */
static PyObject * setLogFile (PyObject * s, PyObject * args,
		PyObject *kwds)
{
    PyObject * fop = NULL;
    FILE * fp = NULL;
    char * kwlist[] = {"fileObject", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:logSetFile", kwlist, &fop))
	return NULL;

    if (fop) {
	if (!PyFile_Check(fop)) {
	    PyErr_SetString(pyrpmError, "requires file object");
	    return NULL;
	}
	fp = PyFile_AsFile(fop);
    }

    (void) rpmlogSetFile(fp);

    Py_INCREF(Py_None);
    return (PyObject *) Py_None;
}

/**
 */
static PyObject *
setVerbosity (PyObject * s, PyObject * args, PyObject *kwds)
{
    int level;
    char * kwlist[] = {"level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &level))
	return NULL;

    rpmSetVerbosity(level);

    Py_INCREF(Py_None);
    return (PyObject *) Py_None;
}

/**
 */
static PyObject *
setEpochPromote (PyObject * s, PyObject * args, PyObject * kwds)
{
    char * kwlist[] = {"promote", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist,
	    &_rpmds_nopromote))
	return NULL;

    Py_INCREF(Py_None);
    return (PyObject *) Py_None;
}

/**
 */
static PyObject * setStats (PyObject * s, PyObject * args,
		PyObject * kwds)
{
    char * kwlist[] = {"stats", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &_rpmts_stats))
	return NULL;

    Py_INCREF(Py_None);
    return (PyObject *) Py_None;
}
/*@}*/

/**
 */
static PyMethodDef rpmModuleMethods[] = {
    { "TransactionSet", (PyCFunction) rpmts_Create, METH_VARARGS|METH_KEYWORDS,
"rpm.TransactionSet([rootDir, [db]]) -> ts\n\
- Create a transaction set.\n" },

    { "addMacro", (PyCFunction) rpmmacro_AddMacro, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "delMacro", (PyCFunction) rpmmacro_DelMacro, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "expandMacro", (PyCFunction)rpmmacro_ExpandMacro, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "getMacros", (PyCFunction) rpmmacro_GetMacros, METH_VARARGS|METH_KEYWORDS,
	NULL },

    { "archscore", (PyCFunction) archScore, METH_VARARGS|METH_KEYWORDS,
        NULL },
    { "platformscore", (PyCFunction) platformScore, METH_VARARGS|METH_KEYWORDS,
        NULL },

    { "signalsCaught", (PyCFunction) signalsCaught, METH_O,
	NULL },
    { "checkSignals", (PyCFunction) checkSignals, METH_VARARGS,
	NULL },

    { "headerLoad", (PyCFunction) hdrLoad, METH_VARARGS|METH_KEYWORDS,
	NULL },

    { "readHeaderListFromFD", (PyCFunction) rpmHeaderFromFD, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "readHeaderListFromFile", (PyCFunction) rpmHeaderFromFile, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "readHeaderFromFD", (PyCFunction) rpmSingleHeaderFromFD, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "writeHeaderListToFD", (PyCFunction) rpmHeaderToFD, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "writeHeaderListToFile", (PyCFunction) rpmHeaderToFile, METH_VARARGS|METH_KEYWORDS,
	NULL },

    { "setLogFile", (PyCFunction) setLogFile, METH_VARARGS|METH_KEYWORDS,
	NULL },

    { "versionCompare", (PyCFunction) versionCompare, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "labelCompare", (PyCFunction) labelCompare, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "evrCompare", (PyCFunction) evrCompare, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "evrSplit", (PyCFunction) evrSplit, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "setVerbosity", (PyCFunction) setVerbosity, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "setEpochPromote", (PyCFunction) setEpochPromote, METH_VARARGS|METH_KEYWORDS,
	NULL },
    { "setStats", (PyCFunction) setStats, METH_VARARGS|METH_KEYWORDS,
	NULL },

    { "dsSingle", (PyCFunction) rpmds_Single, METH_VARARGS|METH_KEYWORDS,
"rpm.dsSingle(TagN, N, [EVR, [Flags]] -> ds\n\
- Create a single element dependency set.\n" },
    { NULL }
} ;

/*
 * Force clean up of open iterators and dbs on exit.
 */
static void rpm_exithook(void)
{
   rpmdbCheckTerminate(1);
}

/*
 * Add rpm tag dictionaries to the module
 */
static void addRpmTags(PyObject *module)
{
    PyObject * dict = PyDict_New();

#ifdef	NOTYET

    PyObject *pyval, *pyname;
    rpmtd names = rpmtdNew();
    rpmTagGetNames(names, 1);
    const char *tagname, *shortname;
    rpmTag tagval;

    while ((tagname = rpmtdNextString(names))) {
	shortname = tagname + strlen("RPMTAG_");
	tagval = rpmTagGetValue(shortname);

	PyModule_AddIntConstant(module, tagname, tagval);
	pyval = PyInt_FromLong(tagval);
	pyname = PyString_FromString(shortname);
	PyDict_SetItem(dict, pyval, pyname);
	Py_XDECREF(pyval);
	Py_XDECREF(pyname);
    }
    PyModule_AddObject(module, "tagnames", dict);
    rpmtdFreeData(names);
    rpmtdFree(names);

#else

    PyObject * d = PyModule_GetDict(module);
    PyObject * o;

 {  const struct headerTagTableEntry_s * t;
    PyObject * to;
    for (t = rpmTagTable; t && t->name; t++) {
	PyDict_SetItemString(d, (char *) t->name, to=PyInt_FromLong(t->val));
	Py_XDECREF(to);
        PyDict_SetItem(dict, to, o=PyString_FromString(t->name + 7));
	Py_XDECREF(o);
    }
 }

 {  headerSprintfExtension exts = rpmHeaderFormats;
    headerSprintfExtension ext;
    PyObject * to;
    int extNum;
    for (ext = exts, extNum = 0; ext != NULL && ext->type != HEADER_EXT_LAST;
        ext = (ext->type == HEADER_EXT_MORE ? *ext->u.more : ext+1), extNum++)
    {
	if (ext->name == NULL || ext->type != HEADER_EXT_TAG)
	    continue;
	PyDict_SetItemString(d, (char *) ext->name, to=PyCObject_FromVoidPtr((void *)ext, NULL));
	Py_XDECREF(to);
        PyDict_SetItem(dict, to, o=PyString_FromString(ext->name + 7));
	Py_XDECREF(o);
    }
 }

    PyDict_SetItemString(d, "tagnames", dict);
    Py_XDECREF(dict);

#endif
}

/**
 */
static char rpm__doc__[] =
"";

void init_rpm(void);	/* XXX eliminate gcc warning */
/**
 */
void init_rpm(void)
{
    PyObject * d, * o, * m;

#if Py_TPFLAGS_HAVE_ITER        /* XXX backport to python-1.5.2 */
    if (PyType_Ready(&hdr_Type) < 0) return;
    if (PyType_Ready(&rpmal_Type) < 0) return;
    if (PyType_Ready(&rpmds_Type) < 0) return;
    if (PyType_Ready(&rpmfd_Type) < 0) return;
    if (PyType_Ready(&rpmfts_Type) < 0) return;
    if (PyType_Ready(&rpmfi_Type) < 0) return;
    if (PyType_Ready(&rpmmi_Type) < 0) return;
    if (PyType_Ready(&rpmps_Type) < 0) return;

    if (PyType_Ready(&rpmtd_Type) < 0) return;
    if (PyType_Ready(&rpmKeyring_Type) < 0) return;
    if (PyType_Ready(&rpmPubkey_Type) < 0) return;

    if (PyType_Ready(&rpmte_Type) < 0) return;
    if (PyType_Ready(&rpmts_Type) < 0) return;
    if (PyType_Ready(&spec_Type) < 0) return;
#endif

    m = Py_InitModule3("_rpm", rpmModuleMethods, rpm__doc__);
    if (m == NULL)
	return;

    /* 
     * treat error to register rpm cleanup hook as fatal, tracebacks
     * can and will leave stale locks around if we can't clean up
     */
    if (Py_AtExit(rpm_exithook) == -1)
	return;

    rpmReadConfigFiles(NULL, NULL);

    d = PyModule_GetDict(m);

#ifdef	HACK
    pyrpmError = PyString_FromString("_rpm.error");
    PyDict_SetItemString(d, "error", pyrpmError);
    Py_XDECREF(pyrpmError);
#else
    pyrpmError = PyErr_NewException("_rpm.error", NULL, NULL);
    if (pyrpmError != NULL)
	PyDict_SetItemString(d, "error", pyrpmError);
#endif

#if Py_TPFLAGS_HAVE_ITER        /* XXX backport to python-1.5.2 */
    Py_INCREF(&hdr_Type);
    PyModule_AddObject(m, "hdr", (PyObject *) &hdr_Type);

    Py_INCREF(&rpmal_Type);
    PyModule_AddObject(m, "al", (PyObject *) &rpmal_Type);

    Py_INCREF(&rpmds_Type);
    PyModule_AddObject(m, "ds", (PyObject *) &rpmds_Type);

    Py_INCREF(&rpmfd_Type);
    PyModule_AddObject(m, "fd", (PyObject *) &rpmfd_Type);

    Py_INCREF(&rpmfts_Type);
    PyModule_AddObject(m, "fts", (PyObject *) &rpmfts_Type);

    Py_INCREF(&rpmfi_Type);
    PyModule_AddObject(m, "fi", (PyObject *) &rpmfi_Type);

    Py_INCREF(&rpmKeyring_Type);
    PyModule_AddObject(m, "Keyring", (PyObject *) &rpmKeyring_Type);
    Py_INCREF(&rpmPubkey_Type);
    PyModule_AddObject(m, "Pubkey", (PyObject *) &rpmPubkey_Type);

    Py_INCREF(&rpmmi_Type);
    PyModule_AddObject(m, "mi", (PyObject *) &rpmmi_Type);

    Py_INCREF(&rpmps_Type);
    PyModule_AddObject(m, "ps", (PyObject *) &rpmps_Type);

    Py_INCREF(&rpmtd_Type);
    PyModule_AddObject(m, "td", (PyObject *) &rpmtd_Type);

    Py_INCREF(&rpmte_Type);
    PyModule_AddObject(m, "te", (PyObject *) &rpmte_Type);

    Py_INCREF(&rpmts_Type);
    PyModule_AddObject(m, "ts", (PyObject *) &rpmts_Type);

    Py_INCREF(&spec_Type);
    PyModule_AddObject(m, "spec", (PyObject *) &spec_Type);
#else
    hdr_Type.ob_type = &PyType_Type;
    rpmal_Type.ob_type = &PyType_Type;
    rpmds_Type.ob_type = &PyType_Type;
    rpmfd_Type.ob_type = &PyType_Type;
    rpmfts_Type.ob_type = &PyType_Type;
    rpmfi_Type.ob_type = &PyType_Type;
    rpmmi_Type.ob_type = &PyType_Type;
    rpmps_Type.ob_type = &PyType_Type;
    rpmte_Type.ob_type = &PyType_Type;
    rpmts_Type.ob_type = &PyType_Type;
    spec_Type.ob_type =  &PyType_Type;
#endif

    addRpmTags(m);

#define REGISTER_ENUM(val) \
    PyDict_SetItemString(d, #val, o=PyInt_FromLong( val )); \
    Py_XDECREF(o);

    REGISTER_ENUM(RPMFILE_STATE_NORMAL);
    REGISTER_ENUM(RPMFILE_STATE_REPLACED);
    REGISTER_ENUM(RPMFILE_STATE_NOTINSTALLED);
    REGISTER_ENUM(RPMFILE_STATE_NETSHARED);
    REGISTER_ENUM(RPMFILE_STATE_WRONGCOLOR);

    REGISTER_ENUM(RPMFILE_CONFIG);
    REGISTER_ENUM(RPMFILE_DOC);
    REGISTER_ENUM(RPMFILE_ICON);
    REGISTER_ENUM(RPMFILE_MISSINGOK);
    REGISTER_ENUM(RPMFILE_NOREPLACE);
    REGISTER_ENUM(RPMFILE_GHOST);
    REGISTER_ENUM(RPMFILE_LICENSE);
    REGISTER_ENUM(RPMFILE_README);
    REGISTER_ENUM(RPMFILE_EXCLUDE);
    REGISTER_ENUM(RPMFILE_UNPATCHED);
    REGISTER_ENUM(RPMFILE_PUBKEY);

    REGISTER_ENUM(RPMDEP_SENSE_REQUIRES);
    REGISTER_ENUM(RPMDEP_SENSE_CONFLICTS);

    REGISTER_ENUM(RPMSENSE_LESS);
    REGISTER_ENUM(RPMSENSE_GREATER);
    REGISTER_ENUM(RPMSENSE_EQUAL);
    REGISTER_ENUM(RPMSENSE_NOTEQUAL);
    REGISTER_ENUM(RPMSENSE_FIND_REQUIRES);
#if defined(RPM_VENDOR_MANDRIVA)
    REGISTER_ENUM(RPMSENSE_PREREQ);
    REGISTER_ENUM(RPMSENSE_SCRIPT_PRE);
    REGISTER_ENUM(RPMSENSE_SCRIPT_POST);
    REGISTER_ENUM(RPMSENSE_SCRIPT_PREUN);
    REGISTER_ENUM(RPMSENSE_SCRIPT_POSTUN)
#endif

    REGISTER_ENUM(RPMDEPS_FLAG_NOUPGRADE);
    REGISTER_ENUM(RPMDEPS_FLAG_NOREQUIRES);
    REGISTER_ENUM(RPMDEPS_FLAG_NOCONFLICTS);
    REGISTER_ENUM(RPMDEPS_FLAG_NOOBSOLETES);
    REGISTER_ENUM(RPMDEPS_FLAG_NOPARENTDIRS);
    REGISTER_ENUM(RPMDEPS_FLAG_NOLINKTOS);
    REGISTER_ENUM(RPMDEPS_FLAG_ANACONDA);
    REGISTER_ENUM(RPMDEPS_FLAG_NOSUGGEST);
    REGISTER_ENUM(RPMDEPS_FLAG_ADDINDEPS);
    REGISTER_ENUM(RPMDEPS_FLAG_DEPLOOPS);

    REGISTER_ENUM(RPMTRANS_FLAG_TEST);
    REGISTER_ENUM(RPMTRANS_FLAG_BUILD_PROBS);
    REGISTER_ENUM(RPMTRANS_FLAG_NOSCRIPTS);
    REGISTER_ENUM(RPMTRANS_FLAG_JUSTDB);
    REGISTER_ENUM(RPMTRANS_FLAG_NOTRIGGERS);
    REGISTER_ENUM(RPMTRANS_FLAG_NODOCS);
    REGISTER_ENUM(RPMTRANS_FLAG_ALLFILES);
    REGISTER_ENUM(RPMTRANS_FLAG_NORPMDB);
    REGISTER_ENUM(RPMTRANS_FLAG_REPACKAGE);
    REGISTER_ENUM(RPMTRANS_FLAG_NOPRE);
    REGISTER_ENUM(RPMTRANS_FLAG_NOPOST);
    REGISTER_ENUM(RPMTRANS_FLAG_NOTRIGGERPREIN);
    REGISTER_ENUM(RPMTRANS_FLAG_NOTRIGGERIN);
    REGISTER_ENUM(RPMTRANS_FLAG_NOTRIGGERUN);
    REGISTER_ENUM(RPMTRANS_FLAG_NOPREUN);
    REGISTER_ENUM(RPMTRANS_FLAG_NOPOSTUN);
    REGISTER_ENUM(RPMTRANS_FLAG_NOTRIGGERPOSTUN);
    REGISTER_ENUM(RPMTRANS_FLAG_NOFDIGESTS);
    REGISTER_ENUM(RPMTRANS_FLAG_NOCONFIGS);

    REGISTER_ENUM(RPMPROB_FILTER_IGNOREOS);
    REGISTER_ENUM(RPMPROB_FILTER_IGNOREARCH);
    REGISTER_ENUM(RPMPROB_FILTER_REPLACEPKG);
    REGISTER_ENUM(RPMPROB_FILTER_FORCERELOCATE);
    REGISTER_ENUM(RPMPROB_FILTER_REPLACENEWFILES);
    REGISTER_ENUM(RPMPROB_FILTER_REPLACEOLDFILES);
    REGISTER_ENUM(RPMPROB_FILTER_OLDPACKAGE);
    REGISTER_ENUM(RPMPROB_FILTER_DISKSPACE);
    REGISTER_ENUM(RPMPROB_FILTER_DISKNODES);

    REGISTER_ENUM(RPMCALLBACK_UNKNOWN);
    REGISTER_ENUM(RPMCALLBACK_INST_PROGRESS);
    REGISTER_ENUM(RPMCALLBACK_INST_START);
    REGISTER_ENUM(RPMCALLBACK_INST_OPEN_FILE);
    REGISTER_ENUM(RPMCALLBACK_INST_CLOSE_FILE);
    REGISTER_ENUM(RPMCALLBACK_TRANS_PROGRESS);
    REGISTER_ENUM(RPMCALLBACK_TRANS_START);
    REGISTER_ENUM(RPMCALLBACK_TRANS_STOP);
    REGISTER_ENUM(RPMCALLBACK_UNINST_PROGRESS);
    REGISTER_ENUM(RPMCALLBACK_UNINST_START);
    REGISTER_ENUM(RPMCALLBACK_UNINST_STOP);
    REGISTER_ENUM(RPMCALLBACK_REPACKAGE_PROGRESS);
    REGISTER_ENUM(RPMCALLBACK_REPACKAGE_START);
    REGISTER_ENUM(RPMCALLBACK_REPACKAGE_STOP);
    REGISTER_ENUM(RPMCALLBACK_UNPACK_ERROR);
    REGISTER_ENUM(RPMCALLBACK_CPIO_ERROR);
    REGISTER_ENUM(RPMCALLBACK_SCRIPT_ERROR);

    REGISTER_ENUM(RPMPROB_BADARCH);
    REGISTER_ENUM(RPMPROB_BADOS);
    REGISTER_ENUM(RPMPROB_PKG_INSTALLED);
    REGISTER_ENUM(RPMPROB_BADRELOCATE);
    REGISTER_ENUM(RPMPROB_REQUIRES);
    REGISTER_ENUM(RPMPROB_CONFLICT);
    REGISTER_ENUM(RPMPROB_NEW_FILE_CONFLICT);
    REGISTER_ENUM(RPMPROB_FILE_CONFLICT);
    REGISTER_ENUM(RPMPROB_OLDPACKAGE);
    REGISTER_ENUM(RPMPROB_DISKSPACE);
    REGISTER_ENUM(RPMPROB_DISKNODES);
    REGISTER_ENUM(RPMPROB_BADPRETRANS);

    REGISTER_ENUM(VERIFY_DIGEST);
    REGISTER_ENUM(VERIFY_SIGNATURE);

    REGISTER_ENUM(RPMLOG_EMERG);
    REGISTER_ENUM(RPMLOG_ALERT);
    REGISTER_ENUM(RPMLOG_CRIT);
    REGISTER_ENUM(RPMLOG_ERR);
    REGISTER_ENUM(RPMLOG_WARNING);
    REGISTER_ENUM(RPMLOG_NOTICE);
    REGISTER_ENUM(RPMLOG_INFO);
    REGISTER_ENUM(RPMLOG_DEBUG);

    REGISTER_ENUM(RPMMIRE_DEFAULT);
    REGISTER_ENUM(RPMMIRE_STRCMP);
    REGISTER_ENUM(RPMMIRE_REGEX);
    REGISTER_ENUM(RPMMIRE_GLOB);

    REGISTER_ENUM(RPMVSF_DEFAULT);
    REGISTER_ENUM(RPMVSF_NOHDRCHK);
    REGISTER_ENUM(RPMVSF_NEEDPAYLOAD);
    REGISTER_ENUM(RPMVSF_NOSHA1HEADER);
    REGISTER_ENUM(RPMVSF_NOMD5HEADER);
    REGISTER_ENUM(RPMVSF_NODSAHEADER);
    REGISTER_ENUM(RPMVSF_NORSAHEADER);
    REGISTER_ENUM(RPMVSF_NOECDSAHEADER);
    REGISTER_ENUM(RPMVSF_NOSHA1);
    REGISTER_ENUM(RPMVSF_NOMD5);
    REGISTER_ENUM(RPMVSF_NODSA);
    REGISTER_ENUM(RPMVSF_NORSA);
    REGISTER_ENUM(RPMVSF_NOECDSA);

    REGISTER_ENUM(TR_ADDED);
    REGISTER_ENUM(TR_REMOVED);

    REGISTER_ENUM(RPMDBI_PACKAGES);

    REGISTER_ENUM((long)RPMAL_NOMATCH);
}
