#ifndef H_RPMCB
#define H_RPMCB

/** \ingroup rpmio
 * \file rpmio/rpmcb.h
 */

#include <rpmlog.h>

/**
 * @todo These convenience macros belong somewhere else.
 */
#define	rpmSetVerbosity(_lvl)	\
	((void)rpmlogSetMask( RPMLOG_UPTO( RPMLOG_PRI(_lvl))))
#define	rpmIncreaseVerbosity()	\
    ((void)rpmlogSetMask(((((unsigned)(rpmlogSetMask(0) & 0xff)) << 1) | 1)))
#define	rpmDecreaseVerbosity()	\
	((void)rpmlogSetMask((((int)(rpmlogSetMask(0) & 0xff)) >> 1)))
#define	rpmIsNormal()		\
	(rpmlogSetMask(0) >= RPMLOG_MASK( RPMLOG_NOTICE ))
#define	rpmIsVerbose()		\
	(rpmlogSetMask(0) >= RPMLOG_MASK( RPMLOG_INFO ))
#define	rpmIsDebug()		\
	(rpmlogSetMask(0) >= RPMLOG_MASK( RPMLOG_DEBUG ))

#endif  /* H_RPMCB */
