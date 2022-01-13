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

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmcb.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include <rpmevr.h>
#include <pkgio.h>
#include "rpmdb.h"

#include "rpmds.h"
#include "rpmfi.h"
#include "rpmts.h"
#include "rpmte.h"

#include "misc.h"

#include "rpmcli.h"

#ifdef __cplusplus

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

#else	/* __cplusplus */

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

#include "rpmxs.h"

MODULE = RPM::Header		PACKAGE = RPM::Header

PROTOTYPES: ENABLE

void
stream2header(fp, callback = NULL)
    FILE *fp
    SV * callback
    PREINIT:
    FD_t fd = NULL;
    Header header;
    PPCODE:
    if (fp && (fd = fdDup(fileno(fp)))) {
	while (1) {
	    const char item[] = "Header";
	    const char * msg = NULL;
	    rpmRC rc;
	    header = NULL;
	    rc = rpmpkgRead(item, fd, &header, &msg);
	    switch (rc) {
	    default:
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", "rpmpkgRead", item, msg);
		/*@fallthrough@*/
	    case RPMRC_NOTFOUND:
		header = NULL;
		/*@fallthrough@*/
	    case RPMRC_OK:
		break;
	    }
	    msg = _free(msg);
	    if (header == NULL)
		break;
            if (callback != NULL && SvROK(callback)) {
                ENTER;
                SAVETMPS;
                PUSHMARK(SP);
                XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Header", (void *)header)));
                PUTBACK;
                call_sv(callback, G_DISCARD | G_SCALAR);
                SPAGAIN;
                FREETMPS;
                LEAVE;
            } else {
                XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Header", (void *)header)));
            }
        }
        Fclose(fd);
    }

# Read a rpm and return a Header
# Return undef if failed
void
rpm2header(filename, sv_vsflags = NULL)
    char * filename
    SV * sv_vsflags
    PREINIT:
    rpmts ts = rpmtsCreate();
    rpmVSFlags vsflags = (rpmVSFlags) 0; 
    PPCODE:
    if (sv_vsflags == NULL) { /* Nothing has been passed, default is no signature */
	/* --nosignatures */
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);
    } else
        vsflags = (rpmVSFlags) sv2constant(sv_vsflags, "rpmvsflags");
    rpmtsSetVSFlags(ts, vsflags);
    _rpm2header(ts, filename, 0);
    SPAGAIN;
    (void)rpmtsFree(ts);
    ts = NULL;

void
DESTROY(h)
	Header h
    CODE:
	(void)headerFree(h);
	h = NULL;

#
# OO methodes
#

#
# Comparison functions
#

int
compare(h1, h2)
	Header h1
	Header h2
    CODE:
	RETVAL = rpmVersionCompare(h1, h2);
    OUTPUT:
    RETVAL


int
_op_spaceship(h1, h2, ...)
	Header h1
	Header h2
    CODE:
	RETVAL = rpmVersionCompare(h1, h2);
    OUTPUT:
    RETVAL

int
is_source_package(h)
	Header h
    CODE:
	RETVAL = !headerIsEntry(h, RPMTAG_SOURCERPM);
    OUTPUT:
	RETVAL

void
tagformat(h, format)
	Header h
	char * format
    PREINIT:
	const char * s;
    PPCODE:
	s =  headerSprintf(h, format, NULL, rpmHeaderFormats, NULL);
    if (s)
	    PUSHs(sv_2mortal(newSVpv((char *)s, 0)));
	s = _free(s); 

# Write rpm header into file pointer
# fedora use HEADER_MAGIC_NO, too bad, set no_header_magic make the function
# compatible
int
write(h, fp, no_header_magic = 0)
    Header h
    FILE * fp
    int no_header_magic
    PREINIT:
    FD_t fd;
    CODE:
    RETVAL = 0;
    if (h) {
        if ((fd = fdDup(fileno(fp))) != NULL) {
	    const char item[] = "Header";
	    const char * msg = NULL;
	    rpmRC rc = rpmpkgWrite(item, fd, h, &msg);
	    if (rc != RPMRC_OK)
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", "write", item, msg);
	    msg = _free(msg);
            Fclose(fd);
            RETVAL = 1;
        }
    }
    OUTPUT:
    RETVAL

void
hsize(h, no_header_magic = 0)
    Header h
    int no_header_magic
    PPCODE:
    XPUSHs(sv_2mortal(newSViv(headerSizeof(h))));
    
void
copy(h)
    Header h
    PREINIT:
    Header hcopy;
    PPCODE:
    hcopy = headerCopy(h);
    XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Header", (void *)hcopy)));
#ifdef HDRPMMEM
    PRINTF_NEW("RPM::Header", hcopy, hcopy->nrefs);
#endif

void
string(h, no_header_magic = 0)
    Header h
    int no_header_magic
    PREINIT:
    char * string = NULL;
    char * ptr = NULL;
    int hsize = 0;
    PPCODE:
    hsize = headerSizeof(h);		/* XXX hsize includes nmagic */
    string = headerUnload(h, NULL);
    if (! no_header_magic) {
	unsigned char * hmagic = NULL;
	size_t nmagic = 0;
	(void) headerGetMagic(h, &hmagic, &nmagic);
        ptr = malloc(hsize);		/* XXX hsize includes nmagic */
        memcpy(ptr, hmagic, nmagic);
        memcpy(ptr + nmagic, string, hsize - nmagic);
    }
    XPUSHs(sv_2mortal(newSVpv(ptr ? ptr : string, hsize)));
    free(string);
    free(ptr);

int
removetag(h, sv_tag)
    Header h
    SV * sv_tag
    PREINIT:
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    CODE:
    he->tag = 0xffffffff;
    if (SvIOK(sv_tag)) {
        he->tag = SvIV(sv_tag);
    } else if (SvPOK(sv_tag)) {
        he->tag = tagValue(SvPV_nolen(sv_tag));
    }
    if (he->tag > 0 && he->tag < 0xffffffff)
        RETVAL = headerDel(h, he, 0);
    else
        RETVAL = 1;
    OUTPUT:
    RETVAL

int
addtag(h, sv_tag, sv_tagtype, ...)
    Header h
    SV * sv_tag
    SV * sv_tagtype
    PREINIT:
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    char * value;
    uint32_t ivalue;
    int i;
    STRLEN len;
    CODE:
    he->tag = 0xffffffff;
    if (SvIOK(sv_tag)) {
        he->tag = SvIV(sv_tag);
    } else if (SvPOK(sv_tag)) {
        he->tag = tagValue(SvPV_nolen(sv_tag));
    }
    he->t = sv2constant(sv_tagtype, "rpmtagtype");
    if (he->tag > 0 && he->tag < 0xffffffff)
        RETVAL = 1;
    else
        RETVAL = 0;
    /* if (he->tag == RPMTAG_OLDFILENAMES)
        expandFilelist(h); */
    for (i = 3; (i < items) && RETVAL; i++) {
        switch (he->t) {
            case RPM_UINT8_TYPE:
            case RPM_UINT16_TYPE:
            case RPM_UINT32_TYPE:
                ivalue = SvUV(ST(i));
		he->p.ui32p = &ivalue;
		he->c = 1;
		he->append = 1;
                RETVAL = headerPut(h, he, 0);
		he->append = 0;
                break;
            case RPM_BIN_TYPE:
                value = (char *)SvPV(ST(i), len);
		he->p.ptr = &value;
		he->c = len;
                RETVAL = headerPut(h, he, 0);
                break;
            case RPM_STRING_ARRAY_TYPE:
                value = SvPV_nolen(ST(i));
		he->p.ptr = &value;
		he->c = 1;
		he->append = 1;
                RETVAL = headerPut(h, he, 0);
		he->append = 0;
                break;
            default:
                value = SvPV_nolen(ST(i));
		he->p.ptr = value;
		he->c = 1;
		he->append = 1;
                RETVAL = headerPut(h, he, 0); 
		he->append = 0;
                break;
        }
    }
    /* if (he->tag == RPMTAG_OLDFILENAMES) {
        compressFilelist(h); 
    } */
    OUTPUT:
    RETVAL
    
void
listtag(h)
    Header h
    PREINIT:
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    HeaderIterator hi;
    PPCODE:
    for (hi = headerInit(h);
	headerNext(hi, he, 0);
	he->p.ptr = _free(he->p.ptr))
    {
        XPUSHs(sv_2mortal(newSViv(he->tag)));
    }
    hi = headerFini(hi);
    
int
hastag(h, sv_tag)
    Header h
    SV * sv_tag
    PREINIT:
    rpmTag tag = 0xffffffff;
    CODE:
    if (SvIOK(sv_tag)) {
        tag = SvIV(sv_tag);
    } else if (SvPOK(sv_tag)) {
        tag = tagValue(SvPV_nolen(sv_tag));
    }    
    if (tag < 0xffffffff)
        RETVAL = headerIsEntry(h, tag);
    else
        RETVAL = -1;
    OUTPUT:
    RETVAL
 
# Return the tag value in headers
void
tag(h, sv_tag)
    Header h
    SV * sv_tag
    PREINIT:
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    PPCODE:
    if (SvIOK(sv_tag)) {
        he->tag = SvIV(sv_tag);
    } else if (SvPOK(sv_tag)) {
        he->tag = tagValue(SvPV_nolen(sv_tag));
    }
    if (he->tag > 0) {
        int xx = headerGet(h, he, 0);
        if (xx) {
            switch(he->t) {
                case RPM_STRING_ARRAY_TYPE:
                    {
                        int i;

                        EXTEND(SP, he->c);
        
                        for (i = 0; i < (int)he->c; i++) {
                            PUSHs(sv_2mortal(newSVpv(he->p.argv[i], 0)));
                        }
                    }
                break;
                case RPM_STRING_TYPE:
                    PUSHs(sv_2mortal(newSVpv(he->p.str, 0)));
                break;
                case RPM_UINT8_TYPE:
                case RPM_UINT16_TYPE:
                case RPM_UINT32_TYPE:
                    {
                        int i;

                        EXTEND(SP, he->c);

                        for (i = 0; i < (int)he->c; i++) {
                            PUSHs(sv_2mortal(newSViv(he->p.ui32p[i])));
                        }
                    }
                break;
                case RPM_BIN_TYPE:
                    PUSHs(sv_2mortal(newSVpv(he->p.ptr, he->c)));
                break;
                default:
                    croak("unknown rpm tag type %d", he->t);
            }
	    he->p.ptr = _free(he->p.ptr);
        }
    }

int
tagtype(h, sv_tag)
    Header h
    SV * sv_tag
    PREINIT:
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    CODE:
    if (SvIOK(sv_tag)) {
        he->tag = SvIV(sv_tag);
    } else if (SvPOK(sv_tag)) {
        he->tag = tagValue(SvPV_nolen(sv_tag));
    }
    RETVAL = 0;
    if (he->tag > 0) {
        int xx = headerGet(h, he, 0);
        if (xx) {
            RETVAL = he->t;
	    he->p.ptr = _free(he->p.ptr);
	}
    }
    OUTPUT:
    RETVAL
    
rpmds
dependencies(header, sv_tag)
    Header header
    SV * sv_tag
    PREINIT:
    rpmTag tag;
    CODE:
    tag = sv2constant(sv_tag, "rpmtag");
    RETVAL = rpmdsNew(header, tag, 0);
    if (RETVAL) RETVAL = rpmdsInit(RETVAL);
    OUTPUT:
    RETVAL

rpmfi
files(header, ts = NULL)
    Header header
    rpmts ts
    PREINIT:
    CODE:
    if (ts)
        ts = rpmtsLink(ts, "RPM::Header::files");
    else
        ts = rpmtsCreate();
    RETVAL = rpmfiNew(ts, header, RPMTAG_BASENAMES, 0);
    if (RETVAL != NULL) RETVAL = rpmfiInit(RETVAL, 0);
    (void)rpmtsFree(ts);
    ts = NULL;
    OUTPUT:
    RETVAL
 
