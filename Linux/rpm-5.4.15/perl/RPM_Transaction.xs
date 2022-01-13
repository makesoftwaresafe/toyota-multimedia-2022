#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#undef Fflush
#undef Mkdir
#undef Stat
#undef Fstat

#include <stdio.h>
#include <string.h>
#include <utime.h>

#include "rpmio.h"
#include "rpmiotypes.h"

#include "rpmtypes.h"
#include "rpmtag.h"
#include "rpmdb.h"

#include "rpmts.h"
#include "rpmte.h"
#include "misc.h"

#include "rpmcli.h"


/* Chip, this is somewhat stripped down from the default callback used by
   the rpmcli.  It has to be here to insure that we open the pkg again. 
   If we don't do this we get segfaults.  I also, kept the updating of some
   of the rpmcli static vars, but I may not have needed to do this.

   Also, we probably want to give a nice interface such that we could allow
   users of RPM to do their own callback, but that will have to come later.
*/
#ifdef NOTYET
static void * _null_callback(
	const void * arg, 
	const rpmCallbackType what, 
	const uint64_t amount, 
	const uint64_t total, 
	fnpyKey key, 
	rpmCallbackData data)
{
	Header h = (Header) arg;
	int flags = (int) ((long)data);
	void * rc = NULL;
	const char * filename = (const char *)key;
	static FD_t fd = NULL;
	int xx;

	/* Code stolen from rpminstall.c and modified */
	switch(what) {
		case RPMCALLBACK_INST_OPEN_FILE:
	 		if (filename == NULL || filename[0] == '\0')
			     return NULL;
			fd = Fopen(filename, "r.ufdio");
			/* FIX: still necessary? */
			if (fd == NULL || Ferror(fd)) {
				fprintf(stderr, "open of %s failed!\n", filename);
				if (fd != NULL) {
					xx = Fclose(fd);
					fd = NULL;
				}
			} else
				fd = fdLink(fd, "persist (showProgress)");
			return (void *)fd;
	 		break;

	case RPMCALLBACK_INST_CLOSE_FILE:
		/* FIX: still necessary? */
		fd = fdFree(fd, "persist (showProgress)");
		if (fd != NULL) {
			xx = Fclose(fd);
			fd = NULL;
		}
		break;

	case RPMCALLBACK_INST_START:
		rpmcliHashesCurrent = 0;
		if (h == NULL || !(flags & INSTALL_LABEL))
			break;
		break;

	case RPMCALLBACK_TRANS_PROGRESS:
	case RPMCALLBACK_INST_PROGRESS:
		break;

	case RPMCALLBACK_TRANS_START:
		rpmcliHashesCurrent = 0;
		rpmcliProgressTotal = 1;
		rpmcliProgressCurrent = 0;
		break;

	case RPMCALLBACK_TRANS_STOP:
		rpmcliProgressTotal = rpmcliPackagesTotal;
		rpmcliProgressCurrent = 0;
		break;

	case RPMCALLBACK_REPACKAGE_START:
		rpmcliHashesCurrent = 0;
		rpmcliProgressTotal = total;
		rpmcliProgressCurrent = 0;
		break;

	case RPMCALLBACK_REPACKAGE_PROGRESS:
		break;

	case RPMCALLBACK_REPACKAGE_STOP:
		rpmcliProgressTotal = total;
		rpmcliProgressCurrent = total;
		rpmcliProgressTotal = rpmcliPackagesTotal;
		rpmcliProgressCurrent = 0;
		break;

	case RPMCALLBACK_UNINST_PROGRESS:
		break;
	case RPMCALLBACK_UNINST_START:
		break;
	case RPMCALLBACK_UNINST_STOP:
		break;
	case RPMCALLBACK_UNPACK_ERROR:
		break;
	case RPMCALLBACK_CPIO_ERROR:
		break;
	case RPMCALLBACK_UNKNOWN:
		break;
	default:
		break;
	}
	
	return rc;	
}
#endif

static void *
    transCallback(const void *h,
       const rpmCallbackType what,
       const unsigned long long amount,
       const unsigned long long total,
       const void * pkgKey,
       void * data) {
    
    /* The call back is used to open/close file, so we fix value, run the perl callback
 *      * and let rpmShowProgress from rpm rpmlib doing its job.
 *           * This unsure we'll not have to follow rpm code at each change. */
    const char * filename = (const char *)pkgKey;
    const char * s_what = NULL;
    dSP;

    PUSHMARK(SP);
    
    switch (what) {
        case RPMCALLBACK_UNKNOWN:
            s_what = "UNKNOWN";
        break;
        case RPMCALLBACK_INST_OPEN_FILE:
            if (filename != NULL && filename[0] != '\0') {
                XPUSHs(sv_2mortal(newSVpv("filename", 0)));
                XPUSHs(sv_2mortal(newSVpv(filename, 0)));
            }
            s_what = "INST_OPEN_FILE";
        break;
        case RPMCALLBACK_INST_CLOSE_FILE:
            s_what = "INST_CLOSE_FILE";
        break;
        case RPMCALLBACK_INST_PROGRESS:
            s_what = "INST_PROGRESS";
        break;
        case RPMCALLBACK_INST_START:
            s_what = "INST_START";
            if (h) {
                XPUSHs(sv_2mortal(newSVpv("header", 0)));
                XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Header", &h)));
#ifdef HDRPMMEM
                PRINTF_NEW(bless_header, &h, -1);
#endif
            }
        break;
        case RPMCALLBACK_TRANS_PROGRESS:
            s_what = "TRANS_PROGRESS";
        break;
        case RPMCALLBACK_TRANS_START:
            s_what = "TRANS_START";
        break;
        case RPMCALLBACK_TRANS_STOP:
            s_what = "TRANS_STOP";
        break;
        case RPMCALLBACK_UNINST_PROGRESS:
            s_what = "UNINST_PROGRESS";
        break;
        case RPMCALLBACK_UNINST_START:
            s_what = "UNINST_START";
        break;
        case RPMCALLBACK_UNINST_STOP:
            s_what = "UNINST_STOP";
        break;
        case RPMCALLBACK_REPACKAGE_PROGRESS:
            s_what = "REPACKAGE_PROGRESS";
        break;
        case RPMCALLBACK_REPACKAGE_START:
            s_what = "REPACKAGE_START";
        break;
        case RPMCALLBACK_REPACKAGE_STOP:
            s_what = "REPACKAGE_STOP";
        break;
        case RPMCALLBACK_UNPACK_ERROR:
            s_what = "UNPACKAGE_ERROR";
        break;
        case RPMCALLBACK_CPIO_ERROR:
            s_what = "CPIO_ERROR";
        break;
        case RPMCALLBACK_SCRIPT_ERROR:
            s_what = "SCRIPT_ERROR";
        break;
    }
   
    XPUSHs(sv_2mortal(newSVpv("what", 0)));
    XPUSHs(sv_2mortal(newSVpv(s_what, 0)));
    XPUSHs(sv_2mortal(newSVpv("amount", 0)));
    XPUSHs(sv_2mortal(newSViv(amount)));
    XPUSHs(sv_2mortal(newSVpv("total", 0)));
    XPUSHs(sv_2mortal(newSViv(total)));
    PUTBACK;
    call_sv((SV *) data, G_DISCARD | G_SCALAR);
    SPAGAIN;
  
    /* Running rpmlib callback, returning its value */
    return rpmShowProgress(h,
            what, 
            amount, 
            total, 
            pkgKey, 
            (long *) INSTALL_NONE /* shut up */);
}



MODULE = RPM::Transaction		PACKAGE = RPM::Transaction

PROTOTYPES: ENABLE

rpmts
new(class, ...)
    char * class
    CODE:
    RETVAL = rpmtsCreate();
    OUTPUT:
    RETVAL

void
DESTROY(ts)
	rpmts ts
    CODE:
	(void)rpmtsFree(ts);
	ts = NULL;

void
setrootdir(ts, root)
    rpmts ts
    char *root
    PPCODE:
    rpmtsSetRootDir(ts, root);

int
vsflags(ts, sv_vsflags = NULL)
    rpmts ts
    SV * sv_vsflags
    PREINIT:
    rpmVSFlags vsflags; 
    CODE:
    if (sv_vsflags != NULL) {
        vsflags = sv2constant(sv_vsflags, "rpmvsflags");
        RETVAL = rpmtsSetVSFlags(ts, vsflags);
    } else {
        RETVAL = rpmtsVSFlags(ts);
    }
    OUTPUT:
    RETVAL

int
transflags(ts, sv_transflag = NULL)
    rpmts ts
    SV * sv_transflag
    PREINIT:
    rpmtransFlags transflags;
    CODE:
    if (sv_transflag != NULL) {
        transflags = sv2constant(sv_transflag, "rpmtransflags");
        /* Take care to rpm config (userland) */
        if (rpmExpandNumeric("%{?_repackage_all_erasures}"))
            transflags |= RPMTRANS_FLAG_REPACKAGE;
        RETVAL = rpmtsSetFlags(ts, transflags);
    } else {
        RETVAL = rpmtsFlags(ts);
    }
    OUTPUT:
    RETVAL

void
packageiterator(ts, sv_tagname = NULL, sv_tagvalue = NULL, keylen = 0)
    rpmts ts
    SV * sv_tagname
    SV * sv_tagvalue
    int keylen
    PPCODE:
    PUTBACK;
    _newiterator(ts, sv_tagname, sv_tagvalue, keylen);
    SPAGAIN;

int
dbadd(ts, header)
    rpmts ts
    Header header
    PREINIT:
    rpmdb db;
    CODE:
    db = rpmtsGetRdb(ts);
    RETVAL = !rpmdbAdd(db, 0, header, ts);
    OUTPUT:
    RETVAL

int
dbremove(ts, sv_offset)
    rpmts ts
    SV * sv_offset
    PREINIT:
    rpmdb db;
    unsigned int offset = 0;
    CODE:
    offset = SvUV(sv_offset);
    db = rpmtsGetRdb(ts);
    if (offset)
        RETVAL = !rpmdbRemove(db, 0, offset, ts);
    else
        RETVAL = 0;
    OUTPUT:
    RETVAL

# XXX:  Add relocations some day. 
int
add_install(ts, h, fn, upgrade = 1)
	rpmts  ts
	Header h
	char * fn
	int    upgrade
    PREINIT:
	rpmRC rc = 0;
    CODE:
	rc = rpmtsAddInstallElement(ts, h, (fnpyKey) fn, upgrade, NULL);
	RETVAL = (rc == RPMRC_OK) ? 1 : 0;
    OUTPUT:
	RETVAL	

int
add_delete(ts, h, offset)
	rpmts        ts
	Header       h
	unsigned int offset
    PREINIT:
	rpmRC rc = 0;
    CODE:
	rc = rpmtsAddEraseElement(ts, h, offset);
	RETVAL = (rc == RPMRC_OK) ? 1 : 0;
    OUTPUT:
	RETVAL	

int
element_count(ts)
	rpmts ts
    CODE:
	RETVAL    = rpmtsNElements(ts);
    OUTPUT:
	RETVAL

int
check(ts)
	rpmts ts
PREINIT:
	int ret;
CODE:
	ret    = rpmtsCheck(ts);
	RETVAL = (ret == 0) ? 1 : 0;
OUTPUT:
	RETVAL

void
order(ts)
	rpmts ts
    PREINIT:
	int ret;
    I32 gimme = GIMME_V;
    PPCODE:
	ret    = rpmtsOrder(ts);
	/* XXX:  May want to do something different here.  It actually
	         returns the number of non-ordered elements...maybe we
	         want this?
	*/
    XPUSHs(sv_2mortal(newSViv(ret == 0 ? 1 : 0)));
    if (gimme != G_SCALAR) {
        XPUSHs(sv_2mortal(newSViv(ret)));
    }

void
_elements(t, type)
	rpmts t;
	rpmElementType type;
PREINIT:
	rpmtsi       i;
	rpmte        te;
	const char * NEVR;
PPCODE:
	i = rpmtsiInit(t);
	if(i == NULL) {
		printf("Did not get a thing!\n");
		return;	
	} else {
		while((te = rpmtsiNext(i, type)) != NULL) {
			NEVR = rpmteNEVR(te);
			XPUSHs(sv_2mortal(newSVpv(NEVR,	0)));
		}
		i = rpmtsiFree(i);
	}

# get from transaction a problem set
void
problems(ts)
    rpmts ts
    PREINIT:
    rpmps ps;
    PPCODE:
    ps = rpmtsProblems(ts);
    if (ps && rpmpsNumProblems(ps)) /* if no problem, return undef */
        XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Problems", ps)));

int
run(ts, callback, ...)
    rpmts ts
    SV * callback
    PREINIT:
    int i;
    rpmprobFilterFlags probFilter = RPMPROB_FILTER_NONE;
    rpmInstallInterfaceFlags install_flags = INSTALL_NONE;
    rpmps ps;
    CODE:
    ts = rpmtsLink(ts, "RPM4 Db::transrun()");
    if (!SvOK(callback)) { /* undef value */
        rpmtsSetNotifyCallback(ts,
                rpmShowProgress,
                (void *) ((long) INSTALL_LABEL | INSTALL_HASH | INSTALL_UPGRADE));
    } else if (SvTYPE(SvRV(callback)) == SVt_PVCV) { /* ref sub */
        rpmtsSetNotifyCallback(ts,
               transCallback,
               (void *) callback);
    } else if (SvTYPE(SvRV(callback)) == SVt_PVAV) { /* array ref */
        install_flags = sv2constant(callback, "rpminstallinterfaceflags");
        rpmtsSetNotifyCallback(ts,
                rpmShowProgress,
                (void *) ((long) install_flags));
    } else {
        croak("Wrong parameter given");
    }

    for (i = 2; i < items; i++)
        probFilter |= sv2constant(ST(i), "rpmprobfilterflags");

    ps = rpmtsProblems(ts);
    RETVAL = rpmtsRun(ts, ps, probFilter);
    ps = rpmpsFree(ps);
    (void)rpmtsFree(ts);
    ts = NULL;
    OUTPUT:
    RETVAL

int
closedb(ts)
    rpmts ts
    CODE:
    RETVAL = !rpmtsCloseDB(ts);
    OUTPUT:
    RETVAL

int
opendb(ts, write = 0)
    rpmts ts
    int write
    CODE:
    RETVAL = !rpmtsOpenDB(ts, write ? O_RDWR : O_RDONLY);
    OUTPUT:
    RETVAL

int
initdb(ts, write = 0)
    rpmts ts
    int write
    CODE:
    RETVAL = 1;
    OUTPUT:
    RETVAL

int
rebuilddb(ts)
    rpmts ts
    CODE:
    RETVAL = !rpmtsRebuildDB(ts);
    OUTPUT:
    RETVAL

int
verifydb(ts)
    rpmts ts
    CODE:
    RETVAL = 1;
    OUTPUT:
    RETVAL

void
readheader(ts, filename)
    rpmts ts
    const char * filename
    PPCODE:
    PUTBACK;
    _rpm2header(ts, filename, 0);
    SPAGAIN;

int
importpubkey(ts, filename)
    rpmts ts
    char * filename
    PREINIT:
    uint8_t *pkt = NULL;
    size_t pktlen = 0;
    int rc;
    CODE:
    rpmtsClean(ts);
    
    if ((rc = pgpReadPkts(filename, (uint8_t ** ) &pkt, &pktlen)) <= 0) {
        RETVAL = 1;
    } else if (rc != PGPARMOR_PUBKEY) {
        RETVAL = 1;
    } else if (rpmcliImportPubkey(ts, pkt, pktlen) != RPMRC_OK) {
        RETVAL = 1;
    } else {
        RETVAL = 0;
    }
    free(pkt);
    OUTPUT:
    RETVAL

void
checkrpm(ts, filename, sv_vsflags = NULL)
    rpmts ts
    char * filename
    SV * sv_vsflags
    PREINIT:
    rpmVSFlags vsflags = RPMVSF_DEFAULT;
    rpmVSFlags oldvsflags = RPMVSF_DEFAULT;
    PPCODE:
    oldvsflags = rpmtsVSFlags(ts); /* keep track of old settings */
    if (sv_vsflags != NULL) {
	    vsflags = sv2constant((sv_vsflags), "rpmvsflags");
        rpmtsSetVSFlags(ts, vsflags);
    }
    PUTBACK;
    _rpm2header(ts, filename, 1); /* Rpmread header is not the most useful, 
                                   * but no other function in rpmlib allow this :( */
    SPAGAIN;
    rpmtsSetVSFlags(ts, oldvsflags); /* resetting in case of change */

