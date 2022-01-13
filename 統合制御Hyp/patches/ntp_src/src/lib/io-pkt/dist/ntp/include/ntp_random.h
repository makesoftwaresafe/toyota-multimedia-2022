
#include <ntp_types.h>

void ntp_crypto_srandom(void);
int ntp_crypto_random_buf(void *buf, size_t nbytes);

long ntp_random (void);
void ntp_srandom (unsigned long);
void ntp_srandomdev (void);
char * ntp_initstate (unsigned long, 	/* seed for R.N.G. */
			char *,		/* pointer to state array */
			long		/* # bytes of state info */
			);
char * ntp_setstate (char *);	/* pointer to state array */




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/include/ntp_random.h $ $Rev: 777476 $")
#endif
