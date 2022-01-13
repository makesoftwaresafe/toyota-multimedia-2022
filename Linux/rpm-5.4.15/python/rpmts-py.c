/** \ingroup py_c
 * \file python/rpmts-py.c
 */

#include "system-py.h"

#include <rpmio_internal.h>	/* XXX for fdSetOpen */
#include <rpmcb.h>

#define	_RPMPS_INTERNAL	/* XXX almost (but not quite) opaque. */
#include <rpmpgp.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include <pkgio.h>		/* XXX headerCheck() */
#include <rpmdb.h>

#define	_RPMTS_INTERNAL		/* XXX expose rpmtsSetScriptFd */
#include <rpmbuild.h>

#include <rpmcli.h>
#define	_RPMROLLBACK_INTERNAL	/* XXX IDTX et al */
#include <rpmrollback.h>

#include "header-py.h"
#include "rpmds-py.h"	/* XXX for rpmdsNew */
#include "rpmfi-py.h"	/* XXX for rpmfiNew */
#include "rpmmi-py.h"
#include "rpmps-py.h"
#include "rpmte-py.h"
#include "spec-py.h"

#include "rpmts-py.h"

#include "debug.h"

#define rpmtsfree() rpmioFreePoolItem()

/*@unchecked@*/
/*@-shadow@*/
extern int _rpmts_debug;
/*@=shadow@*/

/*@access alKey @*/
/*@access FD_t @*/
/*@access Header @*/
/*@access rpmal @*/
/*@access rpmdb @*/
/*@access rpmds @*/
/*@access rpmts @*/
/*@access rpmtsi @*/

/** \ingroup python
 * \name Class: Rpmts
 * \class Rpmts
 * \brief A python rpm.ts object represents an RPM transaction set.
 *
 * The transaction set is the workhorse of RPM.  It performs the
 * installation and upgrade of packages.  The rpm.ts object is
 * instantiated by the TransactionSet function in the rpm module.
 *
 * The TransactionSet function takes two optional arguments. The first
 * argument is the root path. The second is the verify signature disable flags,
 * a set of the following bits:
 *
 * -    rpm.RPMVSF_NOHDRCHK	if set, don't check rpmdb headers
 * -    rpm.RPMVSF_NEEDPAYLOAD	if not set, check header+payload (if possible)
 * -	rpm.RPMVSF_NOSHA1HEADER	if set, don't check header SHA1 digest
 * -	rpm.RPMVSF_NODSAHEADER	if set, don't check header DSA signature
 * -	rpm.RPMVSF_NORSAHEADER	if set, don't check header RSA signature
 * -	rpm.RPMVSF_NOECDSAHEADER if set, don't check header ECDSA signature
 * -	rpm.RPMVSF_NOMD5	if set, don't check header+payload MD5 digest
 * -	rpm.RPMVSF_NODSA	if set, don't check header+payload DSA signature
 * -	rpm.RPMVSF_NORSA	if set, don't check header+payload RSA signature
 * -	rpm.RPMVSF_NOECDSA	if set, don't check header+payload ECDSA signature
 *
 * A rpm.ts object has the following methods:
 *
 * - addInstall(hdr,data,mode)  Add an install element to a transaction set.
 * @param hdr	the header to be added
 * @param data	user data that will be passed to the transaction callback
 *		during transaction execution
 * @param mode 	optional argument that specifies if this package should
 *		be installed ('i'), upgraded ('u'), or if it is just
 *		available to the transaction when computing
 *		dependencies but no action should be performed with it
 *		('a').
 *
 * - addErase(name) Add an erase element to a transaction set.
 * @param name	the package name to be erased
 *
 * - check()	Perform a dependency check on the transaction set. After
 *		headers have been added to a transaction set, a dependency
 *		check can be performed to make sure that all package
 *		dependencies are satisfied.
 * @return	None If there are no unresolved dependencies
 *		Otherwise a list of complex tuples is returned, one tuple per
 *		unresolved dependency, with
 * The format of the dependency tuple is:
 *     ((packageName, packageVersion, packageRelease),
 *      (reqName, reqVersion),
 *      needsFlags,
 *      suggestedPackage,
 *      sense)
 *     packageName, packageVersion, packageRelease are the name,
 *     version, and release of the package that has the unresolved
 *     dependency or conflict.
 *     The reqName and reqVersion are the name and version of the
 *     requirement or conflict.
 *     The needsFlags is a bitfield that describes the versioned
 *     nature of a requirement or conflict.  The constants
 *     rpm.RPMSENSE_LESS, rpm.RPMSENSE_GREATER, and
 *     rpm.RPMSENSE_EQUAL can be logical ANDed with the needsFlags
 *     to get versioned dependency information.
 *     suggestedPackage is a tuple if the dependency check was aware
 *     of a package that solves this dependency problem when the
 *     dependency check was run.  Packages that are added to the
 *     transaction set as "available" are examined during the
 *     dependency check as possible dependency solvers. The tuple
 *     contains two values, (header, suggestedName).  These are set to
 *     the header of the suggested package and its name, respectively.
 *     If there is no known package to solve the dependency problem,
 *     suggestedPackage is None.
 *     The constants rpm.RPMDEP_SENSE_CONFLICTS and
 *     rpm.RPMDEP_SENSE_REQUIRES are set to show a dependency as a
 *     requirement or a conflict.
 *
 * - ts.order()	Do a topological sort of added element relations.
 * @return	None
 *
 * - ts.setFlags(transFlags) Set transaction set flags.
 * @param transFlags - bit(s) to control transaction operations. The
 *		following values can be logically OR'ed together:
 *	- rpm.RPMTRANS_FLAG_TEST - test mode, do not modify the RPM
 *		database, change any files, or run any package scripts
 *	- rpm.RPMTRANS_FLAG_BUILD_PROBS - only build a list of
 *		problems encountered when attempting to run this transaction
 *		set
 *	- rpm.RPMTRANS_FLAG_NOSCRIPTS - do not execute package scripts
 *	- rpm.RPMTRANS_FLAG_JUSTDB - only make changes to the rpm
 *		database, do not modify files.
 *	- rpm.RPMTRANS_FLAG_NOTRIGGERS - do not run trigger scripts
 *	- rpm.RPMTRANS_FLAG_NODOCS - do not install files marked as %doc
 *	- rpm.RPMTRANS_FLAG_ALLFILES - create all files, even if a
 *		file is marked %config(missingok) and an upgrade is
 *		being performed.
 *	- rpm.RPMTRANS_FLAG_KEEPOBSOLETE - do not remove obsoleted
 *		packages.
 * @return	previous transFlags
 *
 * - ts.setProbFilter(ignoreSet) Set transaction set problem filter.
 * @param problemSetFilter - control bit(s) to ignore classes of problems,
 *		a logical or of one or more of the following bit(s):
 *	- rpm.RPMPROB_FILTER_IGNOREOS -
 *	- rpm.RPMPROB_FILTER_IGNOREARCH -
 *	- rpm.RPMPROB_FILTER_REPLACEPKG -
 *	- rpm.RPMPROB_FILTER_FORCERELOCATE -
 *	- rpm.RPMPROB_FILTER_REPLACENEWFILES -
 *	- rpm.RPMPROB_FILTER_REPLACEOLDFILES -
 *	- rpm.RPMPROB_FILTER_OLDPACKAGE -
 *	- rpm.RPMPROB_FILTER_DISKSPACE -
 * @return	previous ignoreSet
 *
 * - ts.run(callback,data) Attempt to execute a transaction set.
 *	After the transaction set has been populated with install/upgrade or
 *	erase actions, the transaction set can be executed by invoking
 *	the ts.run() method.
 */

/** \ingroup py_c
 */
struct rpmtsCallbackType_s {
    PyObject * cb;
    PyObject * data;
    rpmtsObject * tso;
    rpmdsObject * dso;
    PyThreadState *_save;
};

/**
 */
/*@exits@*/
static void rpmts_Die(PyObject *cb)
	/*@*/
{
    PyObject * r = PyObject_Repr(cb);
    char *pyfn = (r != NULL ? PyString_AsString(r) : "???");

    if (PyErr_Occurred())
	PyErr_Print();
    rpmlog(RPMLOG_ERR, _("python callback %s failed, aborting!\n"), pyfn);
    rpmdbCheckTerminate(1);
    exit(EXIT_FAILURE);
}

/**
 */
static int
rpmts_SolveCallback(rpmts ts, rpmds ds, const void * data)
	/*@*/
{
    struct rpmtsCallbackType_s * cbInfo = (struct rpmtsCallbackType_s *) data;
    PyObject * args, * result;
    int res = 1;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_SolveCallback(%p,%p,%p) \"%s\"\n", ts, ds, data, rpmdsDNEVR(ds));

    if (cbInfo->tso == NULL) return res;
    if (cbInfo->cb == Py_None) return res;

    PyEval_RestoreThread(cbInfo->_save);

    cbInfo->dso = rpmds_Wrap(ds);	/* XXX perhaps persistent? */
    args = Py_BuildValue("(OO)", cbInfo->tso, cbInfo->dso);
    result = PyEval_CallObject(cbInfo->cb, args);
    Py_XDECREF(cbInfo->dso);
    cbInfo->dso = NULL;
    Py_XDECREF(args);

    if (!result) {
	rpmts_Die(cbInfo->cb);
	/*@notreached@*/
    } else {
	if (PyInt_Check(result))
	    res = PyInt_AsLong(result);
	Py_XDECREF(result);
    }

    cbInfo->_save = PyEval_SaveThread();

    return res;
}

/**
 */
/*@null@*/
static void *
rpmtsCallback(/*@unused@*/ const void * hd, const rpmCallbackType what,
		         const rpmuint64_t amount, const rpmuint64_t total,
	                 fnpyKey pkgKey, rpmCallbackData data)
	/*@globals _Py_NoneStruct @*/
	/*@modifies _Py_NoneStruct @*/
{
/*@-castexpose@*/
    Header h = (Header) hd;
/*@=castexpose@*/
    struct rpmtsCallbackType_s * cbInfo = data;
    PyObject * pkgObj = (PyObject *) pkgKey;
    PyObject * oh = NULL;
    const char * origin = NULL;
    PyObject * args, * result;
    static FD_t fd;

    if (cbInfo->cb == Py_None) return NULL;

    /* Synthesize a python object for callback (if necessary). */
    if (pkgObj == NULL) {
	if (h) {
	    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
	    he->tag = RPMTAG_NAME;
	    if (headerGet(h, he, 0)) {
		pkgObj = Py_BuildValue("s", he->p.str);
		he->p.ptr = _free(he->p.ptr);
	    } else {
		pkgObj = Py_None;
		Py_INCREF(pkgObj);
	    }
	} else {
	    pkgObj = Py_None;
	    Py_INCREF(pkgObj);
	}
    } else {
	Py_INCREF(pkgObj);
	/* XXX yum has (h, rpmloc) tuple as pkgKey. Extract the path. */
	if (!(PyTuple_Check(pkgObj) && PyArg_ParseTuple(pkgObj, "|Os", &oh, &origin)))
	    origin = NULL;
	/* XXX clean up the path, yum paths start "//..." */
	if (origin && origin[0] == '/' && origin[1] == '/')
	    origin++;
    }

    PyEval_RestoreThread(cbInfo->_save);

    args = Py_BuildValue("(iLLOO)", what, amount, total, pkgObj, cbInfo->data);
    result = PyEval_CallObject(cbInfo->cb, args);
    Py_XDECREF(args);
    Py_XDECREF(pkgObj);

    if (!result) {
	rpmts_Die(cbInfo->cb);
	/*@notreached@*/
    }

    if (what == RPMCALLBACK_INST_OPEN_FILE) {
	int fdno;

        if (!PyArg_Parse(result, "i", &fdno)) {
	    rpmts_Die(cbInfo->cb);
	    /*@notreached@*/
	}
	Py_XDECREF(result);
	cbInfo->_save = PyEval_SaveThread();

	fd = fdDup(fdno);
if (_rpmts_debug)
fprintf(stderr, "\t%p = fdDup(%d)\n", fd, fdno);

	fcntl(Fileno(fd), F_SETFD, FD_CLOEXEC);

	if (origin != NULL)
	    (void) fdSetOpen(fd, origin, 0, 0);

	return fd;
    } else
    if (what == RPMCALLBACK_INST_CLOSE_FILE) {
if (_rpmts_debug)
fprintf(stderr, "\tFclose(%p)\n", fd);
	Fclose (fd);
    } else {
if (_rpmts_debug)
fprintf(stderr, "\t%llu:%llu key %p\n", (unsigned long long)amount, (unsigned long long)total, pkgKey);
    }

    Py_XDECREF(result);
    cbInfo->_save = PyEval_SaveThread();

    return NULL;
}

#if Py_TPFLAGS_HAVE_ITER
/**
 */
static PyObject *
rpmts_iter(rpmtsObject * s)
	/*@*/
{
if (_rpmts_debug)
fprintf(stderr, "*** rpmts_iter(%p) ts %p\n", s, s->ts);

    Py_INCREF(s);
    return (PyObject *)s;
}
#endif

/**
 * @todo Add TR_ADDED filter to iterator.
 */
/*@null@*/
static PyObject *
rpmts_iternext(rpmtsObject * s)
	/*@modifies s @*/
{
    PyObject * result = NULL;
    rpmte te;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_iternext(%p) ts %p tsi %p %d\n", s, s->ts, s->tsi, s->tsiFilter);

    /* Reset iterator on 1st entry. */
    if (s->tsi == NULL) {
	s->tsi = rpmtsiInit(s->ts);
	if (s->tsi == NULL)
	    return NULL;
	s->tsiFilter = 0;
    }

    te = rpmtsiNext(s->tsi, s->tsiFilter);
/*@-branchstate@*/
    if (te != NULL) {
	result = (PyObject *) rpmte_Wrap(te);
    } else {
	s->tsi = rpmtsiFree(s->tsi);
	s->tsiFilter = 0;
    }
/*@=branchstate@*/

    return result;
}

/** \ingroup python
 * \name Class: Rpmts
 */
/*@{*/

/**
 */
/*@null@*/
static PyObject *
rpmts_Debug(/*@unused@*/ rpmtsObject * s, PyObject * args, PyObject * kwds)
        /*@globals _Py_NoneStruct @*/
        /*@modifies _Py_NoneStruct @*/
{
    char * kwlist[] = {"debugLevel", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:Debug", kwlist,
    	    &_rpmts_debug))
	return NULL;

if (_rpmts_debug < 0)
fprintf(stderr, "*** rpmts_Debug(%p) ts %p\n", s, s->ts);

    Py_RETURN_NONE;
}

/**
 */
/*@null@*/
static PyObject *
rpmts_AddInstall(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies s, rpmGlobalMacroContext, _Py_NoneStruct @*/
{
    hdrObject * h;
    PyObject * key;
    char * how = "u";	/* XXX default to upgrade element if missing */
    int isUpgrade = 0;
    char * kwlist[] = {"header", "key", "how", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!O|s:AddInstall", kwlist,
    	    &hdr_Type, &h, &key, &how))
	return NULL;

    {	PyObject * hObj = (PyObject *) h;
	if (hObj->ob_type != &hdr_Type) {
	    PyErr_SetString(PyExc_TypeError, "bad type for header argument");
	    return NULL;
	}
    }

if (_rpmts_debug < 0 || (_rpmts_debug > 0 && *how != 'a'))
fprintf(stderr, "*** rpmts_AddInstall(%p,%p,%p,%s) ts %p\n", s, h, key, how, s->ts);

    if (how && strcmp(how, "a") && strcmp(how, "u") && strcmp(how, "i")) {
	PyErr_SetString(PyExc_TypeError, "how argument must be \"u\", \"a\", or \"i\"");
	return NULL;
    } else if (how && !strcmp(how, "u"))
    	isUpgrade = 1;

	rpmtsAddInstallElement(s->ts, hdrGetHeader(h), key, isUpgrade, NULL);

    /* This should increment the usage count for me */
    if (key)
	PyList_Append(s->keyList, key);

    Py_RETURN_NONE;
}

/**
 * @todo Permit finer control (i.e. not just --allmatches) of deleted elments.
 */
/*@null@*/
static PyObject *
rpmts_AddErase(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies s, rpmGlobalMacroContext, _Py_NoneStruct @*/
{
    PyObject * o;
    int count;
    rpmmi mi;
    char * kwlist[] = {"name", NULL};

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_AddErase(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:AddErase", kwlist, &o))
        return NULL;

    if (PyString_Check(o) || PyUnicode_Check(o)) {
	char * name = PyString_AsString(o);

	mi = rpmtsInitIterator(s->ts, RPMTAG_NVRA, name, 0);
	count = rpmmiCount(mi);
	if (count <= 0) {
	    mi = rpmmiFree(mi);
	    PyErr_SetString(pyrpmError, "package not installed");
	    return NULL;
	} else { /* XXX: Note that we automatically choose to remove all matches */
	    Header h;
	    while ((h = rpmmiNext(mi)) != NULL) {
		uint32_t hdrNum = rpmmiInstance(mi);
		if (hdrNum)
		    rpmtsAddEraseElement(s->ts, h, hdrNum);
	    }
	}
	mi = rpmmiFree(mi);
    } else
    if (PyInt_Check(o)) {
	uint32_t instance = PyInt_AsLong(o);

	mi = rpmtsInitIterator(s->ts, RPMDBI_PACKAGES, &instance, sizeof(instance));
	if (instance == 0 || mi == NULL) {
	    mi = rpmmiFree(mi);
	    PyErr_SetString(pyrpmError, "package not installed");
	    return NULL;
	} else {
	    Header h;
	    while ((h = rpmmiNext(mi)) != NULL) {
		uint32_t hdrNum = rpmmiInstance(mi);
		if (hdrNum)
		    rpmtsAddEraseElement(s->ts, h, hdrNum);
		break;
	    }
	}
	mi = rpmmiFree(mi);
    }

    Py_RETURN_NONE;
}

/**
 */
/*@null@*/
static PyObject *
rpmts_Check(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies s, rpmGlobalMacroContext, _Py_NoneStruct @*/
{
    rpmps ps;
    rpmProblem p;
    PyObject * list, * cf;
    struct rpmtsCallbackType_s cbInfo;
    int i;
    int xx;
    char * kwlist[] = {"callback", NULL};

    memset(&cbInfo, 0, sizeof(cbInfo));
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:Check", kwlist,
	    &cbInfo.cb))
	return NULL;

    if (cbInfo.cb != NULL) {
	if (!PyCallable_Check(cbInfo.cb)) {
	    PyErr_SetString(PyExc_TypeError, "expected a callable");
	    return NULL;
	}
	xx = rpmtsSetSolveCallback(s->ts, rpmts_SolveCallback, (void *)&cbInfo);
    }

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_Check(%p) ts %p cb %p\n", s, s->ts, cbInfo.cb);

    cbInfo.tso = s;
    cbInfo.dso = NULL;		/* XXX perhaps persistent? */
    cbInfo._save = PyEval_SaveThread();

    xx = rpmtsCheck(s->ts);
    ps = rpmtsProblems(s->ts);

    if (cbInfo.cb)
	xx = rpmtsSetSolveCallback(s->ts, rpmtsSolve, NULL);

    PyEval_RestoreThread(cbInfo._save);

    if (ps != NULL) {
	list = PyList_New(0);
        if (!list) {
      	 return NULL;
        }

	rpmpsi psi = rpmpsInitIterator(ps);

	while ((i = rpmpsNextIterator(psi)) >= 0) {
#ifdef	DYING
	    cf = Py_BuildValue("((sss)(ss)iOi)", conflicts[i].byName,
			       conflicts[i].byVersion, conflicts[i].byRelease,

			       conflicts[i].needsName,
			       conflicts[i].needsVersion,

			       conflicts[i].needsFlags,
			       conflicts[i].suggestedPkgs ?
				   conflicts[i].suggestedPkgs[0] : Py_None,
			       conflicts[i].sense);
#else
	    char * byName, * byVersion, * byRelease, *byArch;
	    char * needsName, * needsOP, * needsVersion;
	    char * a, * b;
	    int needsFlags, sense;
	    fnpyKey key;

	    p = rpmpsProblem(psi);

            /* XXX autorelocated i386 on ia64, fix system-config-packages! */
	    if (rpmProblemGetType(p) == RPMPROB_BADRELOCATE)
		continue;

	    a = byName = xstrdup(rpmProblemGetPkgNEVR(p));
	    if ((byArch= strrchr(byName, '.')) != NULL)
		*byArch++ = '\0';
	    if ((byRelease = strrchr(byName, '-')) != NULL)
		*byRelease++ = '\0';
	    if ((byVersion = strrchr(byName, '-')) != NULL)
		*byVersion++ = '\0';

	    key = rpmProblemKey(p);

	    b = needsName = xstrdup(rpmProblemGetAltNEVR(p));
	    if (needsName[1] == ' ') {
		sense = (needsName[0] == 'C')
			? RPMDEP_SENSE_CONFLICTS : RPMDEP_SENSE_REQUIRES;
		needsName += 2;
	    } else
		sense = RPMDEP_SENSE_REQUIRES;
	    if ((needsVersion = strrchr(needsName, ' ')) != NULL)
		*needsVersion++ = '\0';

	    needsFlags = 0;
	    if ((needsOP = strrchr(needsName, ' ')) != NULL) {
		for (*needsOP++ = '\0'; *needsOP != '\0'; needsOP++) {
		    if (*needsOP == '<')	needsFlags |= RPMSENSE_LESS;
		    else if (*needsOP == '>')	needsFlags |= RPMSENSE_GREATER;
		    else if (*needsOP == '=')	needsFlags |= RPMSENSE_EQUAL;
		}
	    }

	    cf = Py_BuildValue("((sss)(ss)iOi)", byName, byVersion, byRelease,
			       needsName, needsVersion, needsFlags,
			       (key != NULL ? key : Py_None),
			       sense);
	    a = _free(a);
	    b = _free(b);
#endif
	    PyList_Append(list, (PyObject *) cf);
	    Py_XDECREF(cf);
	}

	psi = rpmpsFreeIterator(psi);
	ps = rpmpsFree(ps);

	return list;
    }

    Py_RETURN_NONE;
}

/**
 */
/*@null@*/
static PyObject *
rpmts_Order(rpmtsObject * s)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{
    int rc;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_Order(%p) ts %p\n", s, s->ts);

    Py_BEGIN_ALLOW_THREADS
    rc = rpmtsOrder(s->ts);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

/**
 */
/*@null@*/
static PyObject *
rpmts_Clean(rpmtsObject * s)
	/*@globals _Py_NoneStruct @*/
	/*@modifies s, _Py_NoneStruct @*/
{
if (_rpmts_debug)
fprintf(stderr, "*** rpmts_Clean(%p) ts %p\n", s, s->ts);

    rpmtsClean(s->ts);

    Py_RETURN_NONE;
}

/**
 */
/*@null@*/
static PyObject *
rpmts_IDTXload(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies s, rpmGlobalMacroContext, _Py_NoneStruct @*/
{
    PyObject * result = NULL;
    rpmTag tag = RPMTAG_INSTALLTID;
    char * kwlist[] = {"rbtid", NULL};
    uint32_t rbtid = 0;
    IDTX idtx;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_IDTXload(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i:IDTXload", kwlist, &rbtid))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    idtx = IDTXload(s->ts, tag, rbtid);
    Py_END_ALLOW_THREADS

/*@-branchstate@*/
    if (idtx == NULL || idtx->nidt <= 0) {
	Py_INCREF(Py_None);
	result = Py_None;
    } else {
	PyObject * tuple;
	PyObject * ho;
	IDT idt;
	int i;

	result = PyTuple_New(idtx->nidt);
	for (i = 0; i < idtx->nidt; i++) {
	    idt = idtx->idt + i;
	    ho = (PyObject *) hdr_Wrap(idt->h);
	    tuple = Py_BuildValue("(iOi)", idt->val.u32, ho, idt->instance);
	    PyTuple_SET_ITEM(result,  i, tuple);
	    Py_XDECREF(ho);
	}
    }
/*@=branchstate@*/

    idtx = IDTXfree(idtx);

    return result;
}

/**
 */
/*@null@*/
static PyObject *
rpmts_IDTXglob(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies s, rpmGlobalMacroContext, _Py_NoneStruct @*/
{
    PyObject * result = NULL;
    const char * globstr;
    rpmTag tag = RPMTAG_REMOVETID;
    char * kwlist[] = {"rbtid", NULL};
    uint32_t rbtid = 0;
    IDTX idtx;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_IDTXglob(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i:IDTXglob", kwlist, &rbtid))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    globstr = rpmExpand("%{_repackage_dir}/*.rpm", NULL);
    idtx = IDTXglob(s->ts, globstr, tag, rbtid);
    globstr = _free(globstr);
    Py_END_ALLOW_THREADS

/*@-branchstate@*/
    if (idtx == NULL || idtx->nidt <= 0) {
	Py_INCREF(Py_None);
	result = Py_None;
    } else {
	PyObject * tuple;
	PyObject * ho;
	IDT idt;
	int i;

	result = PyTuple_New(idtx->nidt);
	for (i = 0; i < idtx->nidt; i++) {
	    idt = idtx->idt + i;
	    ho = (PyObject *) hdr_Wrap(idt->h);
	    tuple = Py_BuildValue("(iOs)", idt->val.u32, ho, idt->key);
	    PyTuple_SET_ITEM(result,  i, tuple);
	    Py_XDECREF(ho);
	}
    }
/*@=branchstate@*/

    idtx = IDTXfree(idtx);

    return result;
}

/**
 */
/*@null@*/
static PyObject *
rpmts_Rollback(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{
    QVA_t ia = memset(alloca(sizeof(*ia)), 0, sizeof(*ia));
    rpmtransFlags transFlags;
    const char ** av = NULL;
    uint32_t rbtid;
    int rc;
    char * kwlist[] = {"transactionId", NULL};

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_Rollback(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:Rollback", kwlist, &rbtid))
    	return NULL;

    Py_BEGIN_ALLOW_THREADS
    ia->qva_flags = (VERIFY_DIGEST|VERIFY_SIGNATURE|VERIFY_HDRCHK);
    ia->transFlags |= (INSTALL_UPGRADE|INSTALL_FRESHEN|INSTALL_INSTALL);
    ia->transFlags |= RPMTRANS_FLAG_NOFDIGESTS;
    ia->installInterfaceFlags = (INSTALL_UPGRADE|INSTALL_FRESHEN|INSTALL_INSTALL);
    ia->rbtid = rbtid;
    ia->relocations = NULL;
    ia->probFilter |= RPMPROB_FILTER_OLDPACKAGE;

    transFlags = rpmtsSetFlags(s->ts, ia->transFlags);
    rc = rpmRollback(s->ts, ia, av);
    transFlags = rpmtsSetFlags(s->ts, transFlags);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

/**
 */
/*@null@*/
static PyObject *
rpmts_OpenDB(rpmtsObject * s)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_OpenDB(%p) ts %p\n", s, s->ts);

    if (rpmtsDBMode(s->ts) == -1)
	(void) rpmtsSetDBMode(s->ts, O_RDONLY);

    return Py_BuildValue("i", rpmtsOpenDB(s->ts, rpmtsDBMode(s->ts)));
}

/**
 */
/*@null@*/
static PyObject *
rpmts_CloseDB(rpmtsObject * s)
	/*@modifies s @*/
{
    int rc;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_CloseDB(%p) ts %p\n", s, s->ts);

    rc = rpmtsCloseDB(s->ts);
    (void) rpmtsSetDBMode(s->ts, -1);		/* XXX disable lazy opens */

    return Py_BuildValue("i", rc);
}

/**
 */
/*@null@*/
static PyObject *
rpmts_InitDB(rpmtsObject * s)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{
    int rc;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_InitDB(%p) ts %p\n", s, s->ts);

    rc = rpmtsInitDB(s->ts, O_RDONLY);
    if (rc == 0)
	rc = rpmtsCloseDB(s->ts);

    return Py_BuildValue("i", rc);
}

/**
 */
/*@null@*/
static PyObject *
rpmts_RebuildDB(rpmtsObject * s)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{
    int rc;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_RebuildDB(%p) ts %p\n", s, s->ts);

    Py_BEGIN_ALLOW_THREADS
    rc = rpmtsRebuildDB(s->ts);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

/**
 */
/*@null@*/
static PyObject *
rpmts_VerifyDB(rpmtsObject * s)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{
    int rc;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_VerifyDB(%p) ts %p\n", s, s->ts);

    Py_BEGIN_ALLOW_THREADS
    rc = rpmtsVerifyDB(s->ts);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

/**
 */
/*@null@*/
static PyObject *
rpmts_HdrFromFdno(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, fileSystem @*/
	/*@modifies s, rpmGlobalMacroContext, fileSystem @*/
{
    PyObject * result = NULL;
    Header h;
    FD_t fd;
    int fdno;
    rpmRC rpmrc;
    char * kwlist[] = {"fd", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:HdrFromFdno", kwlist,
	    &fdno))
    	return NULL;

    fd = fdDup(fdno);
    rpmrc = rpmReadPackageFile(s->ts, fd, "rpmts_HdrFromFdno", &h);
    Fclose(fd);

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_HdrFromFdno(%p) ts %p rc %d\n", s, s->ts, rpmrc);

/*@-branchstate@*/
    switch (rpmrc) {
    case RPMRC_OK:
	if (h)
	    result = Py_BuildValue("N", hdr_Wrap(h));
	(void)headerFree(h);	/* XXX ref held by result */
	h = NULL;
	break;

    case RPMRC_NOKEY:
	PyErr_SetString(pyrpmError, "public key not available");
	break;

    case RPMRC_NOTTRUSTED:
	PyErr_SetString(pyrpmError, "public key not trusted");
	break;

    case RPMRC_NOTFOUND:
    case RPMRC_FAIL:
    default:
	PyErr_SetString(pyrpmError, "error reading package header");
	break;
    }
/*@=branchstate@*/

    return result;
}

/**
 */
/*@null@*/
static PyObject *
rpmts_HdrCheck(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies s, rpmGlobalMacroContext, _Py_NoneStruct @*/
{
    PyObject * blob;
    PyObject * result = NULL;
    const char * msg = NULL;
    const void * uh;
    int uc;
    pgpDig dig;
    rpmRC rpmrc;
    char * kwlist[] = {"headers", NULL};

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_HdrCheck(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:HdrCheck", kwlist, &blob))
    	return NULL;

    if (blob == Py_None) {
	Py_RETURN_NONE;
    }
    if (!(PyString_Check(blob) || PyUnicode_Check(blob))) {
	PyErr_SetString(pyrpmError, "hdrCheck takes a string of octets");
	return result;
    }
    uh = PyString_AsString(blob);
    uc = PyString_Size(blob);

    dig = pgpDigNew(rpmtsVSFlags(s->ts), 0);
    rpmrc = headerCheck(dig, uh, uc, &msg);
    dig = pgpDigFree(dig);

    switch (rpmrc) {
    case RPMRC_OK:
	Py_INCREF(Py_None);
	result = Py_None;
	break;

    case RPMRC_NOKEY:
	/* XXX note "availaiable", the script kiddies need the misspelling. */
	PyErr_SetString(pyrpmError, "public key not availaiable");
	break;

    case RPMRC_NOTTRUSTED:
	PyErr_SetString(pyrpmError, "public key not trusted");
	break;

    case RPMRC_FAIL:
    default:
	PyErr_SetString(pyrpmError, msg);
	break;
    }
    msg = _free(msg);

    return result;
}

/**
 */
static PyObject *
rpmts_GetVSFlags(rpmtsObject * s)
{
    return Py_BuildValue("i", rpmtsVSFlags(s->ts));
}

/**
 */
/*@null@*/
static PyObject *
rpmts_SetVSFlags(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@modifies s @*/
{
    rpmVSFlags vsflags;
    char * kwlist[] = {"flags", NULL};

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_SetVSFlags(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:SetVSFlags", kwlist,
	    &vsflags))
    	return NULL;

    /* XXX FIXME: value check on vsflags, or build pure python object 
     * for it, and require an object of that type */

    return Py_BuildValue("i", rpmtsSetVSFlags(s->ts, vsflags));
}

/**
 */
/*@null@*/
static PyObject *
rpmts_SetColor(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@modifies s @*/
{
    uint32_t tscolor;
    char * kwlist[] = {"color", NULL};

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_SetColor(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:Color", kwlist, &tscolor))
    	return NULL;

    /* XXX FIXME: value check on tscolor, or build pure python object
     * for it, and require an object of that type */

    return Py_BuildValue("i", rpmtsSetColor(s->ts, tscolor));
}

/**
 */
/*@null@*/
static PyObject *
rpmts_PgpPrtPkts(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals _Py_NoneStruct @*/
	/*@modifies _Py_NoneStruct @*/
{
    PyObject * blob;
    unsigned char * pkt;
    unsigned int pktlen;
    int rc;
    char * kwlist[] = {"octets", NULL};

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_PgpPrtPkts(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:PgpPrtPkts", kwlist, &blob))
    	return NULL;

    if (blob == Py_None) {
	Py_RETURN_NONE;
    }
    if (!(PyString_Check(blob) || PyUnicode_Check(blob))) {
	PyErr_SetString(pyrpmError, "pgpPrtPkts takes a string of octets");
	return NULL;
    }
    pkt = (unsigned char *) PyString_AsString(blob);
    pktlen = PyString_Size(blob);

    rc = pgpPrtPkts(pkt, pktlen, NULL, 1);

    return Py_BuildValue("i", rc);
}

/**
 */
/*@null@*/
static PyObject *
rpmts_PgpImportPubkey(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies s, rpmGlobalMacroContext, _Py_NoneStruct @*/
{
    PyObject * blob;
    unsigned char * pkt;
    unsigned int pktlen;
    int rc;
    char * kwlist[] = {"pubkey", NULL};

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_PgpImportPubkey(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:PgpImportPubkey",
    	    kwlist, &blob))
	return NULL;

    if (blob == Py_None) {
	Py_RETURN_NONE;
    }
    if (!(PyString_Check(blob) || PyUnicode_Check(blob))) {
	PyErr_SetString(pyrpmError, "PgpImportPubkey takes a string of octets");
	return NULL;
    }
    pkt = (unsigned char *) PyString_AsString(blob);
    pktlen = PyString_Size(blob);

    rc = rpmcliImportPubkey(s->ts, pkt, pktlen);

    return Py_BuildValue("i", rc);
}

/**
 */
static PyObject *
rpmts_SetFlags(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@modifies s @*/
{
    rpmtransFlags transFlags = 0;
    char * kwlist[] = {"flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:SetFlags", kwlist,
	    &transFlags))
	return NULL;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_SetFlags(%p) ts %p transFlags 0x%x\n", s, s->ts, transFlags);

    /* XXX FIXME: value check on flags, or build pure python object 
     * for it, and require an object of that type */

    return Py_BuildValue("i", rpmtsSetFlags(s->ts, transFlags));
}

/**
 */
static PyObject *
rpmts_SetDFlags(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@modifies s @*/
{
    rpmdepFlags depFlags = 0;
    char * kwlist[] = {"flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:SetDFlags", kwlist,
	    &depFlags))
	return NULL;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_SetDFlags(%p) ts %p depFlags 0x%x\n", s, s->ts, depFlags);

    /* XXX FIXME: value check on flags, or build pure python object 
     * for it, and require an object of that type */

    return Py_BuildValue("i", rpmtsSetDFlags(s->ts, depFlags));
}

/** \ingroup py_c
 */
static PyObject *
rpmts_SetProbFilter(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@modifies s @*/
{
    rpmprobFilterFlags ignoreSet = 0;
    rpmprobFilterFlags oignoreSet;
    char * kwlist[] = {"ignoreSet", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:ProbFilter", kwlist,
	    &ignoreSet))
	return NULL;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_SetProbFilter(%p) ts %p ignoreSet %x\n", s, s->ts, ignoreSet);

    oignoreSet = s->ignoreSet;
    s->ignoreSet = ignoreSet;

    return Py_BuildValue("i", oignoreSet);
}

/**
 */
/*@null@*/
static rpmpsObject *
rpmts_Problems(rpmtsObject * s)
	/*@modifies s @*/
{

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_Problems(%p) ts %p\n", s, s->ts);

    return rpmps_Wrap( rpmtsProblems(s->ts) );
}

/**
 */
static PyObject *
rpmts_Run(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies s, rpmGlobalMacroContext, _Py_NoneStruct @*/
{
    int rc;
    PyObject * list;
    rpmps ps;
    rpmpsi psi;
    struct rpmtsCallbackType_s cbInfo;
    char * kwlist[] = {"callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO:Run", kwlist,
	    &cbInfo.cb, &cbInfo.data))
	return NULL;

    cbInfo.tso = s;
    cbInfo.dso = NULL;
    cbInfo._save = PyEval_SaveThread();

    if (cbInfo.cb != NULL) {
	if (!PyCallable_Check(cbInfo.cb)) {
	    PyErr_SetString(PyExc_TypeError, "expected a callable");
	    return NULL;
	}
	(void) rpmtsSetNotifyCallback(s->ts, rpmtsCallback, (void *) &cbInfo);
    }

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_Run(%p) ts %p ignore %x\n", s, s->ts, s->ignoreSet);

    rc = rpmtsRun(s->ts, NULL, s->ignoreSet);
    ps = rpmtsProblems(s->ts);

    if (cbInfo.cb)
	(void) rpmtsSetNotifyCallback(s->ts, NULL, NULL);

    PyEval_RestoreThread(cbInfo._save);

    if (rc < 0) {
	list = PyList_New(0);
	return list;
    } else if (!rc) {
	Py_RETURN_NONE;
    }

    list = PyList_New(0);
    if (!list) {
	return NULL;
    }
    psi = rpmpsInitIterator(ps);
    while (rpmpsNextIterator(psi) >= 0) {
	rpmProblem p = rpmpsProblem(psi);
	PyObject * prob = Py_BuildValue("s(isN)", rpmProblemString(p),
			     rpmProblemGetType(p),
			     rpmProblemGetStr(p),
			     PyLong_FromLongLong(rpmProblemGetDiskNeed(p)));
	PyList_Append(list, prob);
	Py_XDECREF(prob);
    }
    psi = rpmpsFreeIterator(psi);

    ps = rpmpsFree(ps);

    return list;
}

/**
 * @todo Add TR_ADDED filter to iterator.
 */
static PyObject *
rpmts_Next(rpmtsObject * s)
	/*@globals _Py_NoneStruct @*/
	/*@modifies s, _Py_NoneStruct @*/
{
    PyObject * result;

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_Next(%p) ts %p\n", s, s->ts);

    result = rpmts_iternext(s);

    if (result == NULL) {
	Py_RETURN_NONE;
    }

    return result;
}

/**
 */
/*@null@*/
static specObject *
spec_Parse(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{
    const char * specfile;
    Spec spec;
    int recursing = 0;
    char * passPhrase = "";
    char *cookie = NULL;
    int anyarch = 1;
    int verify = 1;
    int force = 1;
    char * kwlist[] = {"specfile", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s:Parse", kwlist, &specfile))
	return NULL;

    if (parseSpec(s->ts, specfile,"/", recursing, passPhrase,
             cookie, anyarch, force, verify)!=0) {
             PyErr_SetString(pyrpmError, "can't parse specfile\n");
                     return NULL;
   }

    spec = rpmtsSpec(s->ts);
    return spec_Wrap(spec);
}

/**
 */
/*@null@*/
static rpmmiObject *
rpmts_Match(rpmtsObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{
    PyObject *TagN = NULL;
    PyObject *Key = NULL;
    char *key = NULL;
/* XXX lkey *must* be a 32 bit integer, int "works" on all known platforms. */
    int lkey = 0;
    int len = 0;
    int tag = RPMDBI_PACKAGES;
    char * kwlist[] = {"tagNumber", "key", NULL};

if (_rpmts_debug)
fprintf(stderr, "*** rpmts_Match(%p) ts %p\n", s, s->ts);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO:Match", kwlist,
	    &TagN, &Key))
	return NULL;

    if (TagN && (tag = tagNumFromPyObject (TagN)) == -1) {
	PyErr_SetString(PyExc_TypeError, "unknown tag type");
	return NULL;
    }

    if (Key) {
/*@-branchstate@*/
	if (PyString_Check(Key) || PyUnicode_Check(Key)) {
	    key = PyString_AsString(Key);
	    len = PyString_Size(Key);
	} else if (PyInt_Check(Key)) {
	    lkey = PyInt_AsLong(Key);
	    key = (char *)&lkey;
	    len = sizeof(lkey);
	} else {
	    PyErr_SetString(PyExc_TypeError, "unknown key type");
	    return NULL;
	}
/*@=branchstate@*/
    }

    /* XXX If not already opened, open the database O_RDONLY now. */
    /* XXX FIXME: lazy default rdonly open also done by rpmtsInitIterator(). */
    if (rpmtsGetRdb(s->ts) == NULL) {
	int rc = rpmtsOpenDB(s->ts, O_RDONLY);
	if (rc || rpmtsGetRdb(s->ts) == NULL) {
	    PyErr_SetString(PyExc_TypeError, "rpmdb open failed");
	    return NULL;
	}
    }

    return rpmmi_Wrap( rpmtsInitIterator(s->ts, tag, key, len) );
}

/*@}*/

/** \ingroup py_c
 */
/*@-fullinitblock@*/
/*@unchecked@*/ /*@observer@*/
static struct PyMethodDef rpmts_methods[] = {
 {"Debug",	(PyCFunction)rpmts_Debug,	METH_VARARGS|METH_KEYWORDS,
        NULL},

 {"addInstall",	(PyCFunction) rpmts_AddInstall,	METH_VARARGS|METH_KEYWORDS,
	NULL },
 {"addErase",	(PyCFunction) rpmts_AddErase,	METH_VARARGS|METH_KEYWORDS,
	NULL },
 {"setDFlags",	(PyCFunction) rpmts_SetDFlags,	METH_VARARGS|METH_KEYWORDS,
"ts.setDFlags(depFlags) -> previous depFlags\n\
- Set control bit(s) for executing ts.check() and ts.order().\n" },
 {"check",	(PyCFunction) rpmts_Check,	METH_VARARGS|METH_KEYWORDS,
	NULL },
 {"order",	(PyCFunction) rpmts_Order,	METH_NOARGS,
	NULL },
 {"setFlags",	(PyCFunction) rpmts_SetFlags,	METH_VARARGS|METH_KEYWORDS,
"ts.setFlags(transFlags) -> previous transFlags\n\
- Set control bit(s) for executing ts.run().\n\
  Note: This method replaces the 1st argument to the old ts.run()\n" },
 {"setProbFilter",	(PyCFunction) rpmts_SetProbFilter,	METH_VARARGS|METH_KEYWORDS,
"ts.setProbFilter(ignoreSet) -> previous ignoreSet\n\
- Set control bit(s) for ignoring problems found by ts.run().\n\
  Note: This method replaces the 2nd argument to the old ts.run()\n" },
 {"problems",	(PyCFunction) rpmts_Problems,	METH_NOARGS,
"ts.problems() -> ps\n\
- Return current problem set.\n" },
 {"run",	(PyCFunction) rpmts_Run,	METH_VARARGS|METH_KEYWORDS,
"ts.run(callback, data) -> (problems)\n\
- Run a transaction set, returning list of problems found.\n\
  Note: The callback may not be None.\n" },
 {"clean",	(PyCFunction) rpmts_Clean,	METH_NOARGS,
	NULL },
 {"IDTXload",	(PyCFunction) rpmts_IDTXload,	METH_VARARGS|METH_KEYWORDS,
"ts.IDTXload(rbtid=iid) -> ((tid,hdr,instance)+)\n\
- Return list of installed packages reverse sorted by transaction id.\n" },
 {"IDTXglob",	(PyCFunction) rpmts_IDTXglob,	METH_VARARGS|METH_KEYWORDS,
"ts.IDTXglob(rbtid=rid) -> ((tid,hdr,instance)+)\n\
- Return list of removed packages reverse sorted by transaction id.\n" },
 {"rollback",	(PyCFunction) rpmts_Rollback,	METH_VARARGS|METH_KEYWORDS,
	NULL },
 {"openDB",	(PyCFunction) rpmts_OpenDB,	METH_NOARGS,
"ts.openDB() -> None\n\
- Open the default transaction rpmdb.\n\
  Note: The transaction rpmdb is lazily opened, so ts.openDB() is seldom needed.\n" },
 {"closeDB",	(PyCFunction) rpmts_CloseDB,	METH_NOARGS,
"ts.closeDB() -> None\n\
- Close the default transaction rpmdb.\n\
  Note: ts.closeDB() disables lazy opens, and should hardly ever be used.\n" },
 {"initDB",	(PyCFunction) rpmts_InitDB,	METH_NOARGS,
"ts.initDB() -> None\n\
- Initialize the default transaction rpmdb.\n\
 Note: ts.initDB() is seldom needed anymore.\n" },
 {"rebuildDB",	(PyCFunction) rpmts_RebuildDB,	METH_NOARGS,
"ts.rebuildDB() -> None\n\
- Rebuild the default transaction rpmdb.\n" },
 {"verifyDB",	(PyCFunction) rpmts_VerifyDB,	METH_NOARGS,
"ts.verifyDB() -> None\n\
- Verify the default transaction rpmdb.\n" },
 {"hdrFromFdno",(PyCFunction) rpmts_HdrFromFdno,METH_VARARGS|METH_KEYWORDS,
"ts.hdrFromFdno(fdno) -> hdr\n\
- Read a package header from a file descriptor.\n" },
 {"hdrCheck",	(PyCFunction) rpmts_HdrCheck,	METH_VARARGS|METH_KEYWORDS,
	NULL },
{"getVSFlags",(PyCFunction) rpmts_GetVSFlags,	METH_NOARGS,
"ts.getVSFlags() -> vsflags\n\
- Retrieve current signature verification flags from transaction\n" },
 {"setVSFlags",(PyCFunction) rpmts_SetVSFlags,	METH_VARARGS|METH_KEYWORDS,
"ts.setVSFlags(vsflags) -> ovsflags\n\
- Set signature verification flags. Values for vsflags are:\n\
    rpm.RPMVSF_NOHDRCHK      if set, don't check rpmdb headers\n\
    rpm.RPMVSF_NEEDPAYLOAD   if not set, check header+payload (if possible)\n\
    rpm.RPMVSF_NOSHA1HEADER  if set, don't check header SHA1 digest\n\
    rpm.RPMVSF_NODSAHEADER   if set, don't check header DSA signature\n\
    rpm.RPMVSF_NORSAHEADER   if set, don't check header RSA signature\n\
    rpm.RPMVSF_NOECDSAHEADER if set, don't check header ECDSA signature\n\
    rpm.RPMVSF_NOMD5         if set, don't check header+payload MD5 digest\n\
    rpm.RPMVSF_NODSA         if set, don't check header+payload DSA signature\n\
    rpm.RPMVSF_NORSA         if set, don't check header+payload RSA signature\n\
    rpm.RPMVSF_NOECDSA       if set, don't check header+payload ECDSA signature\n\
" },
 {"setColor",(PyCFunction) rpmts_SetColor,	METH_VARARGS|METH_KEYWORDS,
	NULL },
 {"pgpPrtPkts",	(PyCFunction) rpmts_PgpPrtPkts,	METH_VARARGS|METH_KEYWORDS,
	NULL },
 {"pgpImportPubkey",	(PyCFunction) rpmts_PgpImportPubkey,	METH_VARARGS|METH_KEYWORDS,
	NULL },
 {"parseSpec",	(PyCFunction) spec_Parse,	METH_VARARGS|METH_KEYWORDS,
"ts.parseSpec(\"/path/to/foo.spec\") -> spec\n\
- Parse a spec file.\n" },
 {"dbMatch",	(PyCFunction) rpmts_Match,	METH_VARARGS|METH_KEYWORDS,
"ts.dbMatch([TagN, [key, [len]]]) -> mi\n\
- Create a match iterator for the default transaction rpmdb.\n" },
 {"next",		(PyCFunction)rpmts_Next,	METH_NOARGS,
"ts.next() -> te\n\
- Retrieve next transaction set element.\n" },
    {NULL,		NULL}		/* sentinel */
};
/*@=fullinitblock@*/

/** \ingroup py_c
 */
static void rpmts_dealloc(/*@only@*/ rpmtsObject * s)
	/*@modifies *s @*/
{

if (_rpmts_debug)
fprintf(stderr, "%p -- ts %p db %p\n", s, s->ts, rpmtsGetRdb(s->ts));
    (void)rpmtsFree(s->ts);
    s->ts = NULL;

    if (s->scriptFd) Fclose(s->scriptFd);
    /* this will free the keyList, and decrement the ref count of all
       the items on the list as well :-) */
    Py_XDECREF(s->keyList);
    PyObject_Del((PyObject *)s);
}

/** \ingroup py_c
 */
static int rpmts_setattro(PyObject * o, PyObject * n, PyObject * v)
	/*@*/
{
    rpmtsObject *s = (rpmtsObject *)o;
    char * name = PyString_AsString(n);
    int fdno;

    if (!strcmp(name, "scriptFd")) {
	if (!PyArg_Parse(v, "i", &fdno)) return 0;
	if (fdno < 0) {
	    PyErr_SetString(PyExc_TypeError, "bad file descriptor");
	    return -1;
	} else {
	    s->scriptFd = fdDup(fdno);
	    rpmtsSetScriptFd(s->ts, s->scriptFd);
	}
    } else {
	PyErr_SetString(PyExc_AttributeError, name);
	return -1;
    }

    return 0;
}

/** \ingroup py_c
 */
static int rpmts_init(rpmtsObject * s, PyObject *args, PyObject *kwds)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies s, rpmGlobalMacroContext @*/
{
    /* nothing to do atm... */
    return 0;
}

/** \ingroup py_c
 */
static void rpmts_free(/*@only@*/ rpmtsObject * s)
	/*@modifies s @*/
{
if (_rpmts_debug)
fprintf(stderr, "%p -- ts %p db %p\n", s, s->ts, rpmtsGetRdb(s->ts));
    (void)rpmtsFree(s->ts);
    s->ts = NULL;

    if (s->scriptFd)
	Fclose(s->scriptFd);

    /* this will free the keyList, and decrement the ref count of all
       the items on the list as well :-) */
    Py_XDECREF(s->keyList);

    PyObject_Del((PyObject *)s);
}

/** \ingroup py_c
 */
static PyObject * rpmts_alloc(PyTypeObject * subtype, int nitems)
	/*@*/
{
    PyObject * s = PyType_GenericAlloc(subtype, nitems);

if (_rpmts_debug < 0)
fprintf(stderr, "*** rpmts_alloc(%p,%d) ret %p\n", subtype, nitems, s);
    return s;
}

/** \ingroup py_c
 */
static PyObject * rpmts_new(PyTypeObject * subtype, PyObject *args, PyObject *kwds)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies rpmGlobalMacroContext @*/
{
    rpmtsObject * s = (void *) PyObject_New(rpmtsObject, subtype);

    char * rootDir = "/";
    int vsflags = rpmExpandNumeric("%{?_vsflags}");
    char * kwlist[] = {"rootdir", "vsflags", 0};

if (_rpmts_debug < 0)
fprintf(stderr, "*** rpmts_init(%p,%p,%p)\n", s, args, kwds);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|si:rpmts_init", kwlist,
	    &rootDir, &vsflags))
	return NULL;

    s->ts = rpmtsCreate();
    /* XXX: Why is there no rpmts_SetRootDir() ? */
    (void) rpmtsSetRootDir(s->ts, rootDir);
    /* XXX: make this use common code with rpmts_SetVSFlags() to check the
     *      python objects */
    (void) rpmtsSetVSFlags(s->ts, vsflags);
    s->keyList = PyList_New(0);
    s->ignoreSet = 0;
    s->scriptFd = NULL;
    s->tsi = NULL;
    s->tsiFilter = 0;

if (_rpmts_debug)
fprintf(stderr, "%p ++ ts %p db %p\n", s, s->ts, rpmtsGetRdb(s->ts));

    return (PyObject *)s;
}

/**
 */
/*@unchecked@*/ /*@observer@*/
static char rpmts_doc[] =
"";

/** \ingroup py_c
 */
/*@-fullinitblock@*/
PyTypeObject rpmts_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"rpm.ts",			/* tp_name */
	sizeof(rpmtsObject),		/* tp_size */
	0,				/* tp_itemsize */
	(destructor) rpmts_dealloc, 	/* tp_dealloc */
	0,				/* tp_print */
	(getattrfunc)0, 		/* tp_getattr */
	(setattrfunc)0,			/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr, 	/* tp_getattro */
	(setattrofunc) rpmts_setattro,	/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	rpmts_doc,			/* tp_doc */
#if Py_TPFLAGS_HAVE_ITER
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc) rpmts_iter,	/* tp_iter */
	(iternextfunc) rpmts_iternext,	/* tp_iternext */
	rpmts_methods,			/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	(initproc) rpmts_init,		/* tp_init */
	(allocfunc) rpmts_alloc,	/* tp_alloc */
	(newfunc) rpmts_new,		/* tp_new */
	(freefunc) rpmts_free,		/* tp_free */
	0,				/* tp_is_gc */
#endif
};
/*@=fullinitblock@*/

/**
 */
PyObject *
rpmts_Create(/*@unused@*/ PyObject * s, PyObject * args,
		PyObject * kwds)
{
    return PyObject_Call((PyObject *) &rpmts_Type, args, kwds);
}
