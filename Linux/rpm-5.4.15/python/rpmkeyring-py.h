#ifndef H_RPMKEYRING_PY
#define H_RPMKEYRING_PY

#include "rpmkeyring.h"

/** \ingroup py_c
 * \file python/rpmkeyring-py.h
 */

/** \ingroup py_c
 */
typedef struct rpmPubkeyObject_s rpmPubkeyObject;

/** \ingroup py_c
 */
typedef struct rpmKeyringObject_s rpmKeyringObject;

/** \ingroup py_c
 */
struct rpmPubkeyObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
/*@refcounted@*/
    rpmPubkey pubkey;
};

struct rpmKeyringObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
/*@refcounted@*/
    rpmKeyring keyring;
};

/*@unchecked@*/
extern PyTypeObject rpmKeyring_Type;

/*@unchecked@*/
extern PyTypeObject rpmPubkey_Type;

/**
 */
rpmKeyringObject * rpmKeyring_Wrap(rpmKeyring keyring)
	/*@*/;

#endif	/* H_RPMKEYRING_PY */
