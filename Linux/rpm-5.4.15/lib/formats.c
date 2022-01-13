/** \ingroup header
 * \file lib/formats.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>		/* XXX fnpyKey */
#include <rpmmacro.h>		/* XXX for %_i18ndomains */

#define	_RPMTAG_INTERNAL
#include <rpmtag.h>
#include <rpmtypes.h>

#define	_RPMEVR_INTERNAL
#include <rpmds.h>
#include <rpmfi.h>

#include "legacy.h"
#include "manifest.h"
#include "argv.h"
#include "fs.h"

#include "debug.h"

#ifdef __cplusplus
GENfree(rpmuint32_t *)
#endif	/* __cplusplus */

/*@access headerSprintfExtension @*/

/**
 * Retrieve mounted file system paths.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int fsnamesTag( /*@unused@*/ Header h, HE_t he)
	/*@globals fileSystem, internalState @*/
	/*@modifies he, fileSystem, internalState @*/
{
    const char ** list;

    if (rpmGetFilesystemList(&list, &he->c))
	return 1;

    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = list;
    he->freeData = 0;

    return 0;
}

/**
 * Retrieve mounted file system space.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int fssizesTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies he, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    rpmTagData fnames = { .ptr = NULL };
    rpmTagData fsizes = { .ptr = NULL };
    rpmTagData p;
    rpmuint64_t *usages;
    int numFiles;
    int rc = 1;		/* assume error */
    int xx, yy;

    p.ptr = he->p.ptr;
    he->tag = RPMTAG_FILESIZES;
    xx = headerGet(h, he, 0);
    fsizes.ptr = he->p.ptr;
    he->tag = RPMTAG_FILEPATHS;
    yy = headerGet(h, he, 0);
    fnames.ptr = he->p.ptr;
    numFiles = he->c;
    he->p.ptr = p.ptr;
    if (!xx || !yy) {
	numFiles = 0;
	fsizes.ui32p = _free(fsizes.ui32p);
	fnames.argv = _free(fnames.argv);
    }

    if (rpmGetFilesystemList(NULL, &he->c))
	goto exit;

    he->t = RPM_UINT64_TYPE;
    he->freeData = 1;

    if (fnames.ptr == NULL)
	usages = (rpmuint64_t *) xcalloc(he->c, sizeof(*usages));
    else
    if (rpmGetFilesystemUsage(fnames.argv, fsizes.ui32p, numFiles, &usages, 0))	
	goto exit;

    he->p.ui64p = usages;
    rc = 0;

exit:
    fnames.ptr = _free(fnames.ptr);
    fsizes.ptr = _free(fsizes.ptr);

    return rc;
}

/**
 * Retrieve file classes.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int fileclassTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, he,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    he->t = RPM_STRING_ARRAY_TYPE;
    rpmfiBuildFClasses(h, &he->p.argv, &he->c);
    he->freeData = 1;
    return 0;
}

#ifdef	DYING
/**
 * Retrieve file contexts from header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int filecontextsTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, he,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    he->t = RPM_STRING_ARRAY_TYPE;
    rpmfiBuildFContexts(h, &he->p.argv, &he->c);
    he->freeData = 1;
    return 0;
}

/**
 * Retrieve file contexts from file system.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int fscontextsTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, he,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    he->t = RPM_STRING_ARRAY_TYPE;
    rpmfiBuildFSContexts(h, &he->p.argv, &he->c);
    he->freeData = 1;
    return 0;
}

/**
 * Retrieve file contexts from policy RE's.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int recontextsTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, he,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    he->t = RPM_STRING_ARRAY_TYPE;
    rpmfiBuildREContexts(h, &he->p.argv, &he->c);
    he->freeData = 1;
    return 0;
}
#endif

/**
 * Retrieve file provides.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int fileprovideTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, he,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    he->t = RPM_STRING_ARRAY_TYPE;
    rpmfiBuildFDeps(h, RPMTAG_PROVIDENAME, &he->p.argv, &he->c);
    he->freeData = 1;
    return 0;
}

/**
 * Retrieve file requires.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int filerequireTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, he,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    he->t = RPM_STRING_ARRAY_TYPE;
    rpmfiBuildFDeps(h, RPMTAG_REQUIRENAME, &he->p.argv, &he->c);
    he->freeData = 1;
    return 0;
}

/**
 * Retrieve Requires(missingok): array for Suggests: or Enhances:.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int missingokTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, he,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmds ds = rpmdsNew(h, RPMTAG_REQUIRENAME, 0);
    ARGV_t av = NULL;
    ARGV_t argv;
    int argc = 0;
    char * t;
    size_t nb = 0;
    int i;

    if (ds == NULL)
	return 1;

    /* Collect dependencies marked as hints. */
    ds = rpmdsInit(ds);
    if (ds != NULL)
    while (rpmdsNext(ds) >= 0) {
	int Flags = rpmdsFlags(ds);
	const char * DNEVR;
	if (!(Flags & RPMSENSE_MISSINGOK))
	    continue;
	DNEVR = rpmdsDNEVR(ds);
	if (DNEVR == NULL)
	    continue;
	nb += sizeof(*argv) + strlen(DNEVR+2) + 1;
	(void) argvAdd(&av, DNEVR+2);
	argc++;
    }
    nb += sizeof(*argv);	/* final argv NULL */

    /* Create contiguous header string array. */
    argv = (ARGV_t) xcalloc(nb, 1);
    t = (char *)(argv + argc);
    for (i = 0; i < argc; i++) {
	argv[i] = t;
	t = stpcpy(t, av[i]);
	*t++ = '\0';
    }
    av = argvFree(av);
    (void)rpmdsFree(ds);
    ds = NULL;

    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = argv;
    he->c = argc;
    he->freeData = 1;
    return 0;
}

/*@-type@*/ /* FIX: cast? */
static struct headerSprintfExtension_s _rpmHeaderFormats[] = {
    { HEADER_EXT_TAG, "RPMTAG_ENHANCES",
	{ .tagFunction = missingokTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILECLASS",
	{ .tagFunction = fileclassTag } },
#ifdef	DYING
    { HEADER_EXT_TAG, "RPMTAG_FILECONTEXTS",
	{ .tagFunction = filecontextsTag } },
#endif
    { HEADER_EXT_TAG, "RPMTAG_FILEPROVIDE",
	{ .tagFunction = fileprovideTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILEREQUIRE",
	{ .tagFunction = filerequireTag } },
#ifdef	DYING
    { HEADER_EXT_TAG, "RPMTAG_FSCONTEXTS",
	{ .tagFunction = fscontextsTag } },
#endif
    { HEADER_EXT_TAG, "RPMTAG_FSNAMES",	
	{ .tagFunction = fsnamesTag } },
    { HEADER_EXT_TAG, "RPMTAG_FSSIZES",
	{ .tagFunction = fssizesTag } },
#ifdef	DYING
    { HEADER_EXT_TAG, "RPMTAG_RECONTEXTS",
	{ .tagFunction = recontextsTag } },
#endif
    { HEADER_EXT_TAG, "RPMTAG_SUGGESTS",
	{ .tagFunction = missingokTag } },
    { HEADER_EXT_MORE, NULL,		{ (void *) &headerCompoundFormats } }
} ;
/*@=type@*/

headerSprintfExtension rpmHeaderFormats = &_rpmHeaderFormats[0];
