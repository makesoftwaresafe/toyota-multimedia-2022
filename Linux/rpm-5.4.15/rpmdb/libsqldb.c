#include "system.h"

#define _RPMSQL_INTERNAL
#define _RPMVT_INTERNAL
#define _RPMVC_INTERNAL
#include <rpmsql.h>

#include <sqlite3.h>

#include "debug.h"

/*==============================================================*/

static struct rpmvd_s _hdrVD = {
	/* XXX where to map the default? */
	.prefix = "%{?_repodb}%{!?_repodb:http://rpm5.org/files/popt/}",
	.split	= "/-.",
	.parse	= "dir/file-NVRA-N-V-R.A",
	.regex	= "^(.+/)(((.*)-([^-]+)-([^-]+)\\.([^.]+))\\.rpm)$",
	.idx	= 2,
};

static int hdrCreateConnect(void * _db, void * pAux,
		int argc, const char *const * argv,
		rpmvt * vtp, char ** pzErr)
{
fprintf(stderr, "--> %s\n", __FUNCTION__);
    return rpmvtLoadArgv(rpmvtNew(_db, pAux, argv, &_hdrVD), vtp);
}

struct sqlite3_module hdrModule = {
    .iVersion	= 0,
    .xCreate	= (void *) hdrCreateConnect,
    .xConnect	= (void *) hdrCreateConnect,
};

/*==============================================================*/

static struct rpmsqlVMT_s __VMT[] = {
  { "Hdr",		&hdrModule, NULL },
  { NULL,		NULL, NULL }
};

extern int sqlite3_extension_init(void * _db);
int sqlite3_extension_init(void * _db)
{
    int rc = 0;		/* SQLITE_OK */
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, _db);
    rc = _rpmsqlLoadVMT(_db, __VMT);
fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, _db, rc);
    return rc;
}
