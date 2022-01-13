#include "system.h"

#include <rpmio.h>
#define	_RPMGC_INTERNAL
#include <rpmgc.h>

#include "debug.h"

/* Helper for openpgp_oid_from_str.  */
static size_t
make_flagged_int(unsigned long value, unsigned char * b, size_t nb)
{
    int more = 0;
    int shift;

    /* fixme: figure out the number of bits in an ulong and start with
       that value as shift (after making it a multiple of 7) a more
       straigtforward implementation is to do it in reverse order using
       a temporary buffer - saves a lot of compares */
    for (more = 0, shift = 28; shift > 0; shift -= 7) {
	if (more || value >= (1UL << shift)) {
	    b[nb++] = 0x80 | (value >> shift);
	    value -= (value >> shift) << shift;
	    more = 1;
	}
    }
    b[nb++] = value;
    return nb;
}

/*
 * Convert the OID given in dotted decimal form in STRING to an DER
 * encoding and store it as an opaque value at R_MPI.  The format of
 * the DER encoded is not a regular ASN.1 object but the modified
 * format as used by OpenPGP for the ECC curve description.  On error
 * the function returns and error code an NULL is stored at R_BUG.
 * Note that scanning STRING stops at the first white space
 * character.
 */
static
int openpgp_oid_from_str(const char * s, gcry_mpi_t * r_mpi)
{
    const char * se = s;
    size_t ns = (s ? strlen(s) : 0);
    unsigned char * b = xmalloc(ns + 2 + 1);
    size_t nb = 1;	/* count the length byte */
    const char *endp;
    unsigned long val;
    unsigned long val1 = 0;
    int arcno = 0;
    int rc = -1;	/* assume failure */

    *r_mpi = NULL;

    if (s == NULL || s[0] == '\0')
	goto exit;

    do {
	arcno++;
	val = strtoul(se, (char **) &endp, 10);
	if (!xisdigit(*se) || !(*endp == '.' || *endp == '\0'))
	    goto exit;

	if (*endp == '.')
	    se = endp + 1;

	switch (arcno) {
	case 1:
	    if (val > 2)	/* Not allowed.  */
		goto exit;
	    val1 = val;
	    break;
	case 2:			/* Combine the 1st 2 arcs into octet. */
	    if (val1 < 2) {
		if (val > 39)
		    goto exit;
		b[nb++] = val1 * 40 + val;
	    } else {
		val += 80;
		nb = make_flagged_int(val, b, nb);
	    }
	    break;
	default:
	    nb = make_flagged_int(val, b, nb);
	    break;
	}
    } while (*endp == '.');

    if (arcno == 1 || nb < 2 || nb > 254)	/* Can't encode 1st arc.  */
	goto exit;

    *b = nb - 1;
    *r_mpi = gcry_mpi_set_opaque(NULL, b, nb * 8);
    if (*r_mpi)		/* Success? */
	rc = 0;

exit:
    if (rc)
	b = _free(b);
    return rc;
}

/*
 * Return a malloced string represenation of the OID in the opaque MPI
 * A. In case of an error NULL is returned and ERRNO is set.
 */
static
char *openpgp_oid_to_str(gcry_mpi_t a)
{
    unsigned int nbits = 0;
    const unsigned char * b = (a && gcry_mpi_get_flag(a, GCRYMPI_FLAG_OPAQUE))
    	? gcry_mpi_get_opaque(a, &nbits)
	: NULL;
    size_t nb = (nbits + 7) / 8;
    /* 2 chars in prefix, (3+1) decimalchars/byte, trailing NUL, skip length */
    size_t nt = (2 + ((nb ? nb-1 : 0) * (3+1)) + 1);
    char *t = xmalloc(nt);
    char *te = t;
    size_t ix = 0;
    unsigned long valmask = (unsigned long) 0xfe << (8 * (sizeof(valmask) - 1));
    unsigned long val;

    *te = '\0';

    /* Check oid consistency, skipping the length byte.  */
    if (b == NULL || nb == 0 || *b++ != --nb)
	goto invalid;

    /* Add the prefix and decode the 1st byte */
    if (b[ix] < 40)
	te += sprintf(te, "0.%d", b[ix]);
    else if (b[ix] < 80)
	te += sprintf(te, "1.%d", b[ix] - 40);
    else {
	val = b[ix] & 0x7f;
	while ((b[ix] & 0x80) && ++ix < nb) {
	    if ((val & valmask))
		goto overflow;
	    val <<= 7;
	    val |= b[ix] & 0x7f;
	}
	val -= 80;
	te += sprintf(te, "2.%lu", val);
    }
    *te = '\0';

    /* Append the (dotted) oid integers */
    for (ix++; ix < nb; ix++) {
	val = b[ix] & 0x7f;
	while ((b[ix] & 0x80) && ++ix < nb) {
	    if ((val & valmask))
		goto overflow;
	    val <<= 7;
	    val |= b[ix] & 0x7f;
	}
	te += sprintf(te, ".%lu", val);
    }
    *te = '\0';
    goto exit;

overflow:
    /*
     * Return a special OID (gnu.gnupg.badoid) to indicate the error
     * case.  The OID is broken and thus we return one which can't do
     * any harm.  Formally this does not need to be a bad OID but an OID
     * with an arc that can't be represented in a 32 bit word is more
     * than likely corrupt.
     */
    t = _free(t);
    t = xstrdup("1.3.6.1.4.1.11591.2.12242973");
    goto exit;

invalid:
    errno = EINVAL;
    t = _free(t);
    goto exit;

exit:
    return t;
}

/*==============================================================*/

#define pass()  do { ; } while(0)
#define fail(a,e)                                                       \
  do { fprintf (stderr, "%s:%d: test %d failed (%s)\n",                 \
                __FILE__,__LINE__, (a), gpg_strerror (e));              \
    exit (1);                                                           \
  } while(0)


static void test_openpgp_oid_from_str(void)
{
    static char *sample_oids[] = {
	"0.0",
	"1.0",
	"1.2.3",
	"1.2.840.10045.3.1.7",
	"1.3.132.0.34",
	"1.3.132.0.35",
	NULL
    };
    int err;
    gcry_mpi_t a;
    int idx;
    char *s;
    unsigned char *p;
    unsigned int nbits;
    size_t length;

    errno = 0;
    err = openpgp_oid_from_str("", &a);
    if (err && errno == EINVAL)
	fail(0, err);
    gcry_mpi_release(a);
    errno = 0;

    err = openpgp_oid_from_str(".", &a);
    if (err && errno == EINVAL)
	fail(0, err);
    gcry_mpi_release(a);
    errno = 0;

    err = openpgp_oid_from_str("0", &a);
    if (err && errno == EINVAL)
	fail(0, err);
    gcry_mpi_release(a);
    errno = 0;

    for (idx = 0; sample_oids[idx]; idx++) {
	err = openpgp_oid_from_str(sample_oids[idx], &a);
	if (err)
	    fail(idx, err);

	s = openpgp_oid_to_str(a);
	if (s == NULL)
	    fail(idx, errno);
	if (strcmp(s, sample_oids[idx]))
	    fail(idx, 0);
	s = _free(s);

	p = gcry_mpi_get_opaque(a, &nbits);
	length = (nbits + 7) / 8;
	if (!p || !length || p[0] != length - 1)
	    fail(idx, 0);

	gcry_mpi_release(a);
    }

}

static void test_openpgp_oid_to_str(void)
{
    static struct {
	const char *string;
	unsigned char der[10];
    } samples[] = {
	{
	    "1.2.840.10045.3.1.7", {
	8, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07}}, {
	    "1.3.132.0.34", {
	5, 0x2B, 0x81, 0x04, 0x00, 0x22}}, {
	    "1.3.132.0.35", {
	5, 0x2B, 0x81, 0x04, 0x00, 0x23}}, {
    NULL}};
    gcry_mpi_t a;
    int idx;
    char *s;
    unsigned char *p;

    for (idx = 0; samples[idx].string; idx++) {
	p = xmalloc(samples[idx].der[0] + 1);
	memcpy(p, samples[idx].der, samples[idx].der[0] + 1);
	a = gcry_mpi_set_opaque(NULL, p, (samples[idx].der[0] + 1) * 8);
	if (!a)
	    fail(idx, gpg_error_from_syserror());

	s = openpgp_oid_to_str(a);
	if (s == NULL)
	    fail(idx, gpg_error_from_syserror());
	if (strcmp(s, samples[idx].string))
	    fail(idx, 0);
	s = _free(s);
	gcry_mpi_release(a);
    }

}

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    test_openpgp_oid_from_str();
    test_openpgp_oid_to_str();

    return 0;
}
