/** \ingroup rpmio
 * \file rpmio/rpmsm.c
 */

#include "system.h"

#if defined(WITH_SEMANAGE)
#include <semanage/semanage.h>
#endif

#define	_RPMSM_INTERNAL
#include <rpmsm.h>
#include <rpmlog.h>
#include <rpmmacro.h>
#include <argv.h>
#include <mire.h>

#include "debug.h"

/*@unchecked@*/
int _rpmsm_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmsm _rpmsmI = NULL;

#define F_ISSET(_sm, _FLAG) (((_sm)->flags & ((RPMSM_FLAGS_##_FLAG) & ~0x40000000)) != RPMSM_FLAGS_NONE)

enum rpmsmState_e {
    RPMSM_STATE_CLOSED		= 0,
    RPMSM_STATE_SELECTED	= 1,
    RPMSM_STATE_ACCESSED	= 2,
    RPMSM_STATE_CREATED		= 3,
    RPMSM_STATE_CONNECTED	= 4,
    RPMSM_STATE_TRANSACTION	= 5,
};

/*==============================================================*/
#if defined(WITH_SEMANAGE)
static int rpmsmChk(rpmsm sm, int rc, const char * msg)
{
    if (rc < 0 && msg != NULL) {
	rpmiobAppend(sm->iob, "semanage_", 0);
	rpmiobAppend(sm->iob, msg, 0);
	if (errno) {
	    rpmiobAppend(sm->iob, ": ", 0);
	    rpmiobAppend(sm->iob, strerror(errno), 0);
	}
    }
    return rc;
}

static int rpmsmSelect(rpmsm sm, char * arg)
{
    int rc = 0;
    if (sm->state < RPMSM_STATE_SELECTED) {
	/* Select "targeted" or other store. */
	if (arg) {
	    semanage_select_store(sm->I, arg, SEMANAGE_CON_DIRECT);
	    sm->fn = xstrdup(arg);
	} else
	    sm->fn = _free(sm->fn);
	if (rc >= 0)
	    sm->state = RPMSM_STATE_SELECTED;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
    }
    return rc;
}

static int rpmsmAccess(rpmsm sm, char * arg)
{
    int rc = 0;
    if (sm->state < RPMSM_STATE_ACCESSED) {
	if ((rc = rpmsmSelect(sm, arg)) < 0)
	    return rc;
	/* Select "targeted" or other store. */
	rc = rpmsmChk(sm, semanage_access_check(sm->I), "access_check");
	if (rc >= 0)
	    sm->state = RPMSM_STATE_ACCESSED;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
    }
    return rc;
}

static int rpmsmCreate(rpmsm sm, char * arg)
{
    int rc = 0;
    if (sm->state < RPMSM_STATE_CREATED) {
	if ((sm->access = rc = rpmsmAccess(sm, arg)) < SEMANAGE_CAN_READ) {
	    /* XXX error message */
	    if (rc >= 0)
		rc = -1;
	} else
	    semanage_set_create_store(sm->I, (F_ISSET(sm, CREATE) ? 1 : 0));
	if (rc >= 0)
	    sm->state = RPMSM_STATE_CREATED;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
    }
    return rc;
}

static int rpmsmConnect(rpmsm sm, char * arg)
{
    int rc = 0;
    if (sm->state < RPMSM_STATE_CONNECTED) {
	if ((rc = rpmsmCreate(sm, arg)) < 0)
	    return rc;
	if (!semanage_is_connected(sm->I))
	    rc = rpmsmChk(sm, semanage_connect(sm->I), "connect");
	if (rc >= 0)
	    sm->state = RPMSM_STATE_CONNECTED;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
    }
    return rc;
}
#endif	/* WITH_SEMANAGE */

static int rpmsmBegin(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    if (sm->state < RPMSM_STATE_TRANSACTION) {
	rc = rpmsmChk(sm, semanage_begin_transaction(sm->I), "begin_transaction");
	if (rc >= 0)
	    sm->state = RPMSM_STATE_TRANSACTION;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
    }
#endif
    return rc;
}

static int rpmsmDisconnect(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    if (sm->state >= RPMSM_STATE_CONNECTED) {
	if (semanage_is_connected(sm->I))
	    rc = rpmsmChk(sm, semanage_disconnect(sm->I), "disconnect");
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
	/* XXX check rc? */
	sm->state = RPMSM_STATE_CLOSED;
    }
#endif
    return rc;
}

static int rpmsmReload(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    /* Make sure policy store can be accessed. */
    if ((rc = rpmsmConnect(sm, sm->fn)) < 0)
	return rc;

if (_rpmsm_debug)
fprintf(stderr, "--> %s(%p,%s) I %p\n", __FUNCTION__, sm, arg, sm->I);
    rc = rpmsmChk(sm, semanage_reload_policy(sm->I), "reload_policy");
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
#endif
    return rc;
}

static int rpmsmInstallBase(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    /* Make sure policy store can be accessed. */
    if ((rc = rpmsmConnect(sm, sm->fn)) < 0)
	return rc;

    rc = rpmsmChk(sm, semanage_module_install_base_file(sm->I, arg), "module_install_base_file");
    if (rc >= 0)
	sm->flags |= RPMSM_FLAGS_COMMIT;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
#endif
    return rc;
}

static int rpmsmInstall(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    /* Make sure policy store can be accessed. */
    if ((rc = rpmsmConnect(sm, sm->fn)) < 0)
	return rc;

if (_rpmsm_debug)
fprintf(stderr, "--> %s(%p,%s) I %p\n", __FUNCTION__, sm, arg, sm->I);
    rc = rpmsmChk(sm, semanage_module_install_file(sm->I, arg), "module_install_file");
    if (rc >= 0)
	sm->flags |= RPMSM_FLAGS_COMMIT;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
#endif
    return rc;
}

static int rpmsmUpgrade(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    /* Make sure policy store can be accessed. */
    if ((rc = rpmsmConnect(sm, sm->fn)) < 0)
	return rc;

if (_rpmsm_debug)
fprintf(stderr, "--> %s(%p,%s) I %p\n", __FUNCTION__, sm, arg, sm->I);
    rc = rpmsmChk(sm, semanage_module_upgrade_file(sm->I, arg), "module_upgrade_file");
    if (rc >= 0)
	sm->flags |= RPMSM_FLAGS_COMMIT;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
#endif
    return rc;
}

static int rpmsmRemove(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    /* Make sure policy store can be accessed. */
    if ((rc = rpmsmConnect(sm, sm->fn)) < 0)
	return rc;

    rc = rpmsmChk(sm, semanage_module_remove(sm->I, arg), "module_remove");
    if (rc >= 0)
	sm->flags |= RPMSM_FLAGS_COMMIT;
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
#endif
    return rc;
}

static int rpmsmList(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    semanage_module_info_t *modules = NULL;
    int nmodules = 0;

    /* Make sure policy store can be accessed. */
    if ((rc = rpmsmConnect(sm, sm->fn)) < 0)
	return rc;

    rc = rpmsmChk(sm, semanage_module_list(sm->I, &modules, &nmodules), "module_list");
    if (rc >= 0) {
	miRE include = NULL;
	int ninclude = 0;
	int j;

	if (arg && *arg) {
	    include = mireNew(RPMMIRE_REGEX, 0);
	    (void) mireRegcomp(include, arg);
	    ninclude++;
	}

	for (j = 0; j < nmodules; j++) {
	    semanage_module_info_t * m = semanage_module_list_nth(modules, j);
	    char * NV = rpmExpand(semanage_module_get_name(m), "-",
			   semanage_module_get_version(m), NULL);
	    if (include == NULL
	     || !(mireApply(include, ninclude, NV, 0, +1) < 0))
		rpmiobAppend(sm->iob, NV, 1);
	    NV = _free(NV);
	    semanage_module_info_datum_destroy(m);
	}

	include = mireFree(include);
	modules = _free(modules);
    }
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%s) I %p rc %d\n", __FUNCTION__, sm, arg, sm->I, rc);
#endif
    return rc;
}

static int rpmsmCommit(rpmsm sm, char * arg)
{
    int rc = 0;
#if defined(WITH_SEMANAGE)
    if (F_ISSET(sm, COMMIT)) {
	semanage_set_reload(sm->I, (F_ISSET(sm, RELOAD) ? 1 : 0));
	semanage_set_rebuild(sm->I, (F_ISSET(sm, REBUILD) ? 1 : 0));
	semanage_set_disable_dontaudit(sm->I, (F_ISSET(sm, NOAUDIT) ? 1 : 0));
	rc = rpmsmChk(sm, semanage_commit(sm->I), "commit");
	/* XXX check rc? */
	sm->flags &= ~RPMSM_FLAGS_COMMIT;
    }
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p) I %p rc %d\n", __FUNCTION__, sm, sm->I, rc);
#endif
    return rc;
}

/*==============================================================*/

static void rpmsmFini(void * _sm)
	/*@globals fileSystem @*/
	/*@modifies *_sm, fileSystem @*/
{
    rpmsm sm = (rpmsm) _sm;

#if defined(WITH_SEMANAGE)
    if (sm->I) {
	rpmsmDisconnect(sm, NULL);
	semanage_handle_destroy(sm->I);
    }
#endif
    (void) rpmiobFree(sm->iob);
    sm->iob = NULL;
    sm->fn = _free(sm->fn);
    sm->flags = 0;
    sm->state = RPMSM_STATE_CLOSED;
    sm->access = 0;
    sm->av = argvFree(sm->av);
    sm->I = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmsmPool = NULL;

static rpmsm rpmsmGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmsmPool, fileSystem @*/
	/*@modifies pool, _rpmsmPool, fileSystem @*/
{
    rpmsm sm;

    if (_rpmsmPool == NULL) {
	_rpmsmPool = rpmioNewPool("sm", sizeof(*sm), -1, _rpmsm_debug,
			NULL, NULL, rpmsmFini);
	pool = _rpmsmPool;
    }
    return (rpmsm) rpmioGetPool(pool, sizeof(*sm));
}

rpmsm rpmsmNew(const char * fn, unsigned int flags)
{
    rpmsm sm = rpmsmGetPool(_rpmsmPool);

    sm->fn = NULL;
    sm->flags = 0;
    sm->state = 0;
    sm->access = 0;
    sm->av = NULL;
    sm->I = NULL;
    sm->iob = rpmiobNew(0);

#if defined(WITH_SEMANAGE)
  { int rc;

    /* Connect to the policy store immediately in known (if requested). */
    if ((sm->I = semanage_handle_create()) == NULL)
	rc = -1;
    else if (flags & RPMSM_FLAGS_BEGIN)
	rc = rpmsmBegin(sm, (char *)fn);
    else if (flags & RPMSM_FLAGS_CONNECT)
	rc = rpmsmConnect(sm, (char *)fn);
    else if (flags & RPMSM_FLAGS_CREATE)
	rc = rpmsmCreate(sm, (char *)fn);
    else if (flags & RPMSM_FLAGS_ACCESS)
	rc = rpmsmAccess(sm, (char *)fn);
    else if (flags & RPMSM_FLAGS_SELECT)
	rc = rpmsmSelect(sm, (char *)fn);
    else
	rc = 0;

    if (rc < 0) {
	(void)rpmsmFree(sm);
	return NULL;
    }
  }
#endif

    return rpmsmLink(sm);
}

/*@unchecked@*/ /*@null@*/
static const char _rpmsmI_fn[] = "minimum";
/*@unchecked@*/
static int _rpmsmI_flags;

static rpmsm rpmsmI(void)
	/*@globals _rpmsmI @*/
	/*@modifies _rpmsmI @*/
{
    if (_rpmsmI == NULL)
	_rpmsmI = rpmsmNew(_rpmsmI_fn, _rpmsmI_flags);
    return _rpmsmI;
}

/*==============================================================*/

rpmRC rpmsmRun(rpmsm sm, char ** av, const char ** resultp)
{
    int ncmds = argvCount((ARGV_t)av);
    int rc = 0;
    int i;

if (_rpmsm_debug)
fprintf(stderr, "--> %s(%p,%p,%p) av[0] \"%s\"\n", __FUNCTION__, sm, av, resultp, (av ? av[0] : NULL));

    if (sm == NULL) sm = rpmsmI();

    (void) rpmiobEmpty(sm->iob);

    for (i = 0; i < ncmds; i++) {
	char * cmd = (char *)av[i];
	char * arg = strchr(cmd+1, ' ');

	if (arg)
	while (xisspace(*arg)) arg++;

	switch (*cmd) {
	case 'l':
	    rc = rpmsmList(sm, arg);
	    break;
	case 'b':
	    rc = rpmsmInstallBase(sm, arg);
	    break;
	case 'i':
	    rc = rpmsmInstall(sm, arg);
	    break;
	case 'u':
	    rc = rpmsmUpgrade(sm, arg);
	    break;
	case 'r':
	    rc = rpmsmRemove(sm, arg);
	    if (rc == -2) {	/* XXX WTF? */
		rc = 0;
		continue;
	    }
	    break;
	case 'R':
	    rc = rpmsmReload(sm, arg);
	    break;
	case 'B':
	    rc = rpmsmBegin(sm, arg);
	    sm->flags |= RPMSM_FLAGS_COMMIT;	/* XXX from semanage CLI */
	    sm->flags &= ~RPMSM_FLAGS_NOAUDIT;	/* XXX for rpmsmCommit() */
	    break;
	default:
	    rpmiobAppend(sm->iob, "Unknown cmd: \"", 0);
	    rpmiobAppend(sm->iob, cmd, 0);
	    rpmiobAppend(sm->iob, "\"", 0);
	    goto exit;
	    /*@notreached@*/ break;
	}
    }

exit:
    /* Commit any pending transactions. */
    if (!rc && F_ISSET(sm, COMMIT))
	rc = rpmsmCommit(sm, sm->fn);

    /* Disconnect from policy store (if connected). */
    if (F_ISSET(sm, CONNECT)) {
	int xx = rpmsmDisconnect(sm, sm->fn);
	if (xx && !rc)
	    rc = xx;
    }

    /* Return any/all spewage to caller. */
    (void) rpmiobRTrim(sm->iob);
    if (resultp)	/* XXX xstrdup? */
	*resultp = (rpmiobLen(sm->iob) > 0 ? rpmiobStr(sm->iob) : NULL);
    
if (_rpmsm_debug)
fprintf(stderr, "<-- %s(%p,%p,%p) av[0] \"%s\" rc %d\n", __FUNCTION__, sm, av, resultp, (av ? av[0] : NULL), rc);
    /* XXX impedance match to OK or FAIL return codes. */
    return (rc >= 0 ? RPMRC_OK : RPMRC_FAIL);
}
