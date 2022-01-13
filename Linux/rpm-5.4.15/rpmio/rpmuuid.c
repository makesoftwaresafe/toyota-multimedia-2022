/** \ingroup rpmio
 * \file rpmio/rpmuuid.c
 */

#if defined(__APPLE__)
/* workaround for "uuid_t" type conflict, between <unistd.h> and "uuid.h" */
#define _UUID_T
#define uuid_t __darwin_uuid_t
#include <unistd.h>
#undef uuid_t
#undef _UUID_T
#endif

#include "system.h"
#include <string.h>
#include "rpmlog.h"
#include "rpmuuid.h"
#ifdef WITH_UUID
#include "uuid.h"
#endif
#include "debug.h"

int rpmuuidMake(int version, const char *ns, const char *data, char *buf_str, unsigned char *buf_bin)
{
    int ec = 1;		/* assume error */
#ifdef WITH_UUID
    uuid_rc_t rc;
    uuid_t *uuid = NULL;
    uuid_t *uuid_ns = NULL;
    char *result_ptr;
    size_t result_len;

    /* sanity check version */
    if (!(version == 1 || (version >= 3 && version <= 5))) {
        rpmlog(RPMLOG_ERR, _("invalid UUID version number"));
	goto exit;
    }
    if ((version == 3 || version == 5) && (ns == NULL || data == NULL)) {
        rpmlog(RPMLOG_ERR, _("namespace or data required for requested UUID version\n"));
	goto exit;
    }
    if (buf_str == NULL && buf_bin == NULL) {
        rpmlog(RPMLOG_ERR, _("either string or binary result buffer required\n"));
	goto exit;
    }

    /* create UUID object */
    if ((rc = uuid_create(&uuid)) != UUID_RC_OK) {
        rpmlog(RPMLOG_ERR, _("failed to create UUID object: %s\n"), uuid_error(rc));
	goto exit;
    }

    /* create optional UUID namespace object */
    if (version == 3 || version == 5) {
        if ((rc = uuid_create(&uuid_ns)) != UUID_RC_OK) {
            rpmlog(RPMLOG_ERR, _("failed to create UUID namespace object: %s\n"), uuid_error(rc));
	    goto exit;
        }
        if ((rc = uuid_load(uuid_ns, ns)) != UUID_RC_OK) {
            if ((rc = uuid_import(uuid_ns, UUID_FMT_STR, ns, strlen(ns))) != UUID_RC_OK) {
                rpmlog(RPMLOG_ERR, _("failed to import UUID namespace object: %s\n"), uuid_error(rc));
		goto exit;
            }
        }
    }

    /*  generate UUID  */
    if (version == 1)
        rc = uuid_make(uuid, UUID_MAKE_V1);
    else if (version == 3)
        rc = uuid_make(uuid, UUID_MAKE_V3, uuid_ns, data);
    else if (version == 4)
        rc = uuid_make(uuid, UUID_MAKE_V4);
    else if (version == 5)
        rc = uuid_make(uuid, UUID_MAKE_V5, uuid_ns, data);
    if (rc != UUID_RC_OK) {
        rpmlog(RPMLOG_ERR, _("failed to make UUID object: %s\n"), uuid_error(rc));
	goto exit;
    }

    /*  export UUID */
    if (buf_str != NULL) {
        result_ptr = buf_str;
        result_len = UUID_LEN_STR+1;
        if ((rc = uuid_export(uuid, UUID_FMT_STR, &result_ptr, &result_len)) != UUID_RC_OK) {
            rpmlog(RPMLOG_ERR, _("failed to export UUID object as string representation: %s\n"), uuid_error(rc));
	    goto exit;
        }
    }
    if (buf_bin != NULL) {
        result_ptr = (char *)buf_bin;
        result_len = UUID_LEN_BIN;
        if ((rc = uuid_export(uuid, UUID_FMT_BIN, &result_ptr, &result_len)) != UUID_RC_OK) {
            rpmlog(RPMLOG_ERR, _("failed to export UUID object as binary representation: %s\n"), uuid_error(rc));
	    goto exit;
        }
    }

exit:
    /*  destroy UUID object(s)  */
    if (uuid != NULL)
    (void) uuid_destroy(uuid);
    if (uuid_ns != NULL)
        (void) uuid_destroy(uuid_ns);

    ec = 0;	/* indicate success */
#else
    rpmlog(RPMLOG_ERR, _("UUID generator not available!\n"));
#endif

    return ec;
}

