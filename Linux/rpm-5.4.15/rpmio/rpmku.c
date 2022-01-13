/** \ingroup rpmio
 * \file rpmio/rpmku.c
 */

#include "system.h"

#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio.h>
#if defined(HAVE_KEYUTILS_H)
#include <rpmmacro.h>
#include <argv.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <keyutils.h>
#ifdef __cplusplus
}
#endif

#define _RPMPGP_INTERNAL
#include <rpmpgp.h>
#endif
#include <rpmku.h>

#include "debug.h"

/*@access pgpDigParams@ */
/*@access rpmiob @*/

#if defined(HAVE_KEYUTILS_H)
/*@unchecked@*/
rpmint32_t _kuKeyring;

/*@unchecked@*/
static int _kuCache = 1;

typedef struct _kuItem_s {
/*@observer@*/
    const char *name;
    key_serial_t val;
} * _kuItem;

/* NB: the following table must be sorted lexically for bsearch(3). */
/*@unchecked@*/ /*@observer@*/
static struct _kuItem_s kuTable[] = {
    { "group",		KEY_SPEC_GROUP_KEYRING },
    { "process",	KEY_SPEC_PROCESS_KEYRING },
    { "reqkey_auth",	KEY_SPEC_REQKEY_AUTH_KEY },
    { "session",	KEY_SPEC_SESSION_KEYRING },
    { "thread",		KEY_SPEC_THREAD_KEYRING },
    { "user",		KEY_SPEC_USER_KEYRING },
    { "user_session",	KEY_SPEC_USER_SESSION_KEYRING },
};

/*@unchecked@*/
static size_t nkuTable = sizeof(kuTable) / sizeof(kuTable[0]);

static int
kuCmp(const void * a, const void * b)
	/*@*/
{
    return strcmp(((_kuItem)a)->name, ((_kuItem)b)->name);
}

static key_serial_t
kuValue(const char * name)
	/*@*/
{
    _kuItem k = NULL;

    if (name != NULL && *name != '\0') {
	_kuItem tmp = (_kuItem) memset(alloca(sizeof(*tmp)), 0, sizeof(*tmp));
/*@-temptrans@*/
	tmp->name = name;
/*@=temptrans@*/
	k = (_kuItem)bsearch(tmp, kuTable, nkuTable, sizeof(kuTable[0]), kuCmp);
    }
    return (k != NULL ? k->val :  0);
}
#endif	/* HAVE_KEYUTILS_H */

/*@-globs -internalglobs -mods @*/
char * _GetPass(const char * prompt)
{
    char * pw;

/*@-unrecog@*/
    pw = getpass( prompt ? prompt : "" );
/*@=unrecog@*/

#if defined(HAVE_KEYUTILS_H)
    if (_kuKeyring == 0) {
	const char * _keyutils_keyring
		= rpmExpand("%{?_keyutils_keyring}", NULL);
	_kuKeyring = (rpmuint32_t) kuValue(_keyutils_keyring);
	if (_kuKeyring == 0)
	    _kuKeyring = KEY_SPEC_PROCESS_KEYRING;
	_keyutils_keyring = _free(_keyutils_keyring);
    }

    if (pw && *pw) {
	key_serial_t keyring = (key_serial_t) _kuKeyring;
	size_t npw = strlen(pw);
	(void) add_key("user", "rpm:passwd", pw, npw, keyring);
	(void) memset(pw, 0, npw);	/* burn the password */
	pw = (char *) "@u user rpm:passwd";
    }
#endif	/* HAVE_KEYUTILS_H */

assert(pw != NULL);
/*@-observertrans -statictrans@*/
    return pw;
/*@=observertrans =statictrans@*/
}
/*@=globs =internalglobs =mods @*/

char * _RequestPass(/*@unused@*/ const char * prompt)
{
/*@only@*/ /*@relnull@*/
    static char * password = NULL;
#if defined(HAVE_KEYUTILS_H)
    const char * foo = "user rpm:yyyy spoon";
    ARGV_t av = NULL;
    int xx = argvSplit(&av, foo, NULL);
    key_serial_t dest = 0;
    key_serial_t key = 0;

    if (password != NULL) {
	free(password);
	password = NULL;
    }
assert(av != NULL);
assert(av[0] != NULL);
assert(av[1] != NULL);
assert(av[2] != NULL);
    key = request_key(av[0], av[1], av[2], dest);
    av = argvFree(av);

/*@-nullstate@*/	/* XXX *password may be null. */
    xx = keyctl_read_alloc(key, (void **)&password);
/*@=nullstate@*/
    if (password == NULL)
	password = (char *) "";
#endif	/* HAVE_KEYUTILS_H */

/*@-statictrans@*/
    return password;
/*@=statictrans@*/
}

/*@-redecl@*/
char * (*Getpass) (const char * prompt) = _GetPass;
/*@=redecl@*/

rpmRC rpmkuFindPubkey(pgpDigParams sigp, /*@out@*/ rpmiob * iobp)
{
    if (iobp != NULL)
	*iobp = NULL;

#if defined(HAVE_KEYUTILS_H)
    if (_kuCache) {
/*@observer@*/
	static const char krprefix[] = "rpm:gpg:pubkey:";
	key_serial_t keyring = (key_serial_t) _kuKeyring;
	char krfp[32];
	char * krn = (char *) alloca(strlen(krprefix) + sizeof("12345678"));
	long key;
	int xx;

	(void) snprintf(krfp, sizeof(krfp), "%08X", pgpGrab(sigp->signid+4, 4));
	krfp[sizeof(krfp)-1] = '\0';
	*krn = '\0';
	(void) stpcpy( stpcpy(krn, krprefix), krfp);

	key = keyctl_search(keyring, "user", krn, 0);
	xx = keyctl_read(key, NULL, 0);
	if (xx > 0) {
	    rpmiob iob = rpmiobNew(xx);
	    xx = keyctl_read(key, (char *)iob->b, iob->blen);
	    if (xx > 0) {
#ifdef	NOTYET
		pubkeysource = xstrdup(krn);
		_kuCache = 0;	/* XXX don't bother caching. */
#endif
	    } else
		iob = rpmiobFree(iob);

	    if (iob != NULL && iobp != NULL) {
		*iobp = iob;
		return RPMRC_OK;
	    } else {
		iob = rpmiobFree(iob);
		return RPMRC_NOTFOUND;
	    }
	} else
	    return RPMRC_NOTFOUND;
    } else
#endif	/* HAVE_KEYUTILS_H */
    return RPMRC_NOTFOUND;
}

rpmRC rpmkuStorePubkey(pgpDigParams sigp, /*@only@*/ rpmiob iob)
{
#if defined(HAVE_KEYUTILS_H)
    if (_kuCache) {
/*@observer@*/
	static const char krprefix[] = "rpm:gpg:pubkey:";
	key_serial_t keyring = (key_serial_t) _kuKeyring;
	char krfp[32];
	char * krn = (char *) alloca(strlen(krprefix) + sizeof("12345678"));

	(void) snprintf(krfp, sizeof(krfp), "%08X", pgpGrab(sigp->signid+4, 4));
	krfp[sizeof(krfp)-1] = '\0';
	*krn = '\0';
	(void) stpcpy( stpcpy(krn, krprefix), krfp);
/*@-moduncon -noeffectuncon @*/
	(void) add_key("user", krn, iob->b, iob->blen, keyring);
/*@=moduncon =noeffectuncon @*/
    }
#endif	/* HAVE_KEYUTILS_H */
    iob = rpmiobFree(iob);
    return RPMRC_OK;
}

const char * rpmkuPassPhrase(const char * passPhrase)
{
    const char * pw;

#if defined(HAVE_KEYUTILS_H)
    if (passPhrase && !strcmp(passPhrase, "@u user rpm:passwd")) {
	key_serial_t keyring = (key_serial_t) _kuKeyring;
	long key;
	int xx;

/*@-moduncon@*/
	key = keyctl_search(keyring, "user", "rpm:passwd", 0);
	pw = NULL;
	xx = keyctl_read_alloc(key, (void **)&pw);
/*@=moduncon@*/
	if (xx < 0)
	    pw = NULL;
    } else
#endif	/* HAVE_KEYUTILS_H */
	pw = xstrdup(passPhrase);
    return pw;
}
