/* rpmarchive: spit out the main archive portion of a package */

#include "system.h"
const char *__progname;

#include <rpmio.h>
#include <rpmiotypes.h>	/* XXX fnpyKey */
#include <rpmurl.h>
#include <rpmtypes.h>
#include <rpmtag.h>
#include <pkgio.h>

#include <rpmts.h>

#include "debug.h"

#ifdef __cplusplus

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

#else	/* __cplusplus */

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

int main(int argc, char **argv)
{
    FD_t fdi, fdo;
    Header h;
    char * rpmio_flags;
    rpmRC rc;
    FD_t gzdi;

    setprogname(argv[0]);	/* Retrofit glibc __progname */
    if (argc == 1 || (argc == 2 && !strcmp(argv[1], "-")))
	fdi = fdDup(STDIN_FILENO);
    else {
	int ut = urlPath(argv[1], NULL);
	if (ut == URL_IS_HTTP || ut == URL_IS_HTTPS) {
	    fprintf(stderr, "%s: %s: HTTP/HTTPS transport is non-functional.\n",
		argv[0], argv[1]);
	    exit(EXIT_FAILURE);
	}
	fdi = Fopen(argv[1], "r");
    }

    if (Ferror(fdi)) {
	fprintf(stderr, "%s: %s: %s\n", argv[0],
		(argc == 1 ? "<stdin>" : argv[1]), Fstrerror(fdi));
	exit(EXIT_FAILURE);
    }
    fdo = fdDup(STDOUT_FILENO);

    {	rpmts ts = rpmtsCreate();
	rpmVSFlags vsflags;

	/* XXX retain the ageless behavior of rpm2cpio */
	vsflags = (rpmVSFlags) 0; /* XXX FIXME: ignore default disablers. */
#if defined(SUPPORT_NOSIGNATURES)
	/* --nodigests */
	VSF_SET(vsflags, NOSHA1HEADER);
	VSF_SET(vsflags, NOMD5HEADER);
	VSF_SET(vsflags, NOSHA1);
	VSF_SET(vsflags, NOMD5);

	/* --nosignature */
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);

	/* --nohdrchk */
	VSF_SET(vsflags, NOHDRCHK);

	VSF_CLR(vsflags, NEEDPAYLOAD);	/* XXX needed? */
#endif
	(void) rpmtsSetVSFlags(ts, vsflags);

	/*@-mustmod@*/      /* LCL: segfault */
	rc = rpmReadPackageFile(ts, fdi, "rpm2cpio", &h);
	/*@=mustmod@*/

	(void)rpmtsFree(ts); 
	ts = NULL;
    }

    switch (rc) {
    case RPMRC_OK:
    case RPMRC_NOKEY:
    case RPMRC_NOTTRUSTED:
	break;
    case RPMRC_NOTFOUND:
	fprintf(stderr, _("argument is not an RPM package\n"));
	exit(EXIT_FAILURE);
	break;
    case RPMRC_FAIL:
    default:
	fprintf(stderr, _("error reading header from package\n"));
	exit(EXIT_FAILURE);
	break;
    }

    /* Retrieve type of payload compression. */
    {	HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
	const char * payload_compressor = NULL;
	char * t;
	int xx;

	he->tag = RPMTAG_PAYLOADCOMPRESSOR;
	xx = headerGet(h, he, 0);
	payload_compressor = (xx ? he->p.str : "gzip");

	rpmio_flags = t = alloca(sizeof("r.gzdio"));
	*t++ = 'r';
	if (!strcmp(payload_compressor, "gzip"))
	    t = stpcpy(t, ".gzdio");
	if (!strcmp(payload_compressor, "bzip2"))
	    t = stpcpy(t, ".bzdio");
	if (!strcmp(payload_compressor, "lzma"))
	    t = stpcpy(t, ".lzdio");
	if (!strcmp(payload_compressor, "xz"))
	    t = stpcpy(t, ".xzdio");
	he->p.ptr = _free(he->p.ptr);
    }

    gzdi = Fdopen(fdi, rpmio_flags);	/* XXX gzdi == fdi */
    if (gzdi == NULL) {
	fprintf(stderr, _("cannot re-open payload: %s\n"), Fstrerror(gzdi));
	exit(EXIT_FAILURE);
    }

    rc = ufdCopy(gzdi, fdo);
    rc = (rc <= 0) ? EXIT_FAILURE : EXIT_SUCCESS;
    Fclose(fdo);

    Fclose(gzdi);	/* XXX gzdi == fdi */

    return rc;
}
