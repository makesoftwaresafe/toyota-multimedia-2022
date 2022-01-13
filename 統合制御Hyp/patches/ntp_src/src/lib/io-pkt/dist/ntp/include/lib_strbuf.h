/*
 * lib_strbuf.h - definitions for routines which use the common string buffers
 */
#ifndef LIB_STRBUF_H
#define LIB_STRBUF_H

#include <ntp_types.h>
#include <ntp_malloc.h>			/* for ZERO() */

/*
 * Sizes of things
 */
#define LIB_NUMBUF	16
#define	LIB_BUFLENGTH	128

typedef char libbufstr[LIB_BUFLENGTH];
extern libbufstr lib_stringbuf[LIB_NUMBUF];
extern int lib_nextbuf;
extern int lib_inited;


/*
 * Macro to get a pointer to the next buffer
 */
#define	LIB_GETBUF(bufp)					\
	do {							\
		ZERO(lib_stringbuf[lib_nextbuf]);		\
		(bufp) = &lib_stringbuf[lib_nextbuf++][0];	\
		lib_nextbuf %= COUNTOF(lib_stringbuf);		\
	} while (FALSE)

#endif	/* LIB_STRBUF_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/include/lib_strbuf.h $ $Rev: 777476 $")
#endif
