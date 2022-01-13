/** \ingroup rpmio
 * \file rpmio/rpmzlog.c
 * Trace logging (originally swiped from PIGZ).
 */

/* pigz.c -- parallel implementation of gzip
 * Copyright (C) 2007, 2008 Mark Adler
 * Version 2.1.4  9 Nov 2008  Mark Adler
 */

/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu

  Mark accepts donations for providing this software.  Donations are not
  required or expected.  Any amount that you feel is appropriate would be
  appreciated.  You can use this link:

  https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=536055

 */

#include "system.h"
#include <stdarg.h>

#include <rpmiotypes.h>
#include "yarn.h"

#define	_RPMZLOG_INTERNAL
#include "rpmzlog.h"

#include "debug.h"

/*@unchecked@*/
static int _rpmzlog_debug = 0;

/*@access rpmzMsg @*/
/*@access rpmzLog @*/


#ifdef __cplusplus
GENfree(rpmzMsg)
GENfree(rpmzLog)
#endif	/* __cplusplus */

/*==============================================================*/

/* maximum log entry length */
#define _RPMZLOG_MAXMSG 256

rpmzLog rpmzLogLink(rpmzLog zlog)
{
    long nrefs;

    if (zlog == NULL)
	return NULL;
    yarnPossess(zlog->_item.use);
    nrefs = yarnPeekLock(zlog->_item.use);
if (_rpmzlog_debug)
fprintf(stderr, "    ++ zlog %p[%ld]\n", zlog, nrefs+1);
    yarnTwist(zlog->_item.use, BY, 1);
    return zlog;
}

rpmzLog rpmzLogNew(struct timeval *tv)
{
    rpmzLog zlog = (rpmzLog) xcalloc(1, sizeof(*zlog));

    zlog->_item.use = yarnNewLock(0);
    zlog->msg_head = NULL;
    zlog->msg_tail = &zlog->msg_head;

    /* starting time for log entries */
/*@-assignexpose@*/
    if (tv != NULL)
	zlog->start = *tv;	/* structure assignment */
    else
	(void) gettimeofday(&zlog->start, NULL);
/*@=assignexpose@*/
/*@-nullret@*/
    return rpmzLogLink(zlog);
/*@=nullret@*/
}

void rpmzLogAdd(rpmzLog zlog, const char *fmt, ...)
{
    rpmzMsg me;
    struct timeval now;
    va_list ap;
    char msg[_RPMZLOG_MAXMSG];
    int xx;

    if (zlog == NULL)
	return;

    xx = gettimeofday(&now, NULL);
    me = (rpmzMsg) xmalloc(sizeof(*me));
    me->when = now;
    va_start(ap, fmt);
    xx = vsnprintf(msg, sizeof(msg)-1, fmt, ap);
    va_end(ap);
    msg[sizeof(msg)-1] = '\0';

/*@-mustfreeonly@*/
    me->msg = (char *) xmalloc(strlen(msg) + 1);
/*@=mustfreeonly@*/
    strcpy(me->msg, msg);
/*@-mustfreeonly@*/
    me->next = NULL;
/*@=mustfreeonly@*/

assert(zlog->_item.use != NULL);
    yarnPossess(zlog->_item.use);
    *zlog->msg_tail = me;
    zlog->msg_tail = &me->next;
    zlog->msg_count++;
    yarnRelease(zlog->_item.use);
}

/**
 *  * Pull entry from trace log and print it, return false if empty.
 *   */
static int rpmzMsgShow(/*@null@*/ rpmzLog zlog, /*@null@*/ FILE * fp)
        /*@globals fileSystem, internalState @*/
        /*@modifies zlog, *fp, fileSystem, internalState @*/
{
    rpmzMsg me;
    struct timeval diff;

    if (zlog == NULL)
	return 0;
    if (fp == NULL)
	fp = stderr;

    yarnPossess(zlog->_item.use);
    if (zlog->msg_tail == NULL || (me = zlog->msg_head) == NULL) {
	yarnRelease(zlog->_item.use);
	return 0;
    }
    zlog->msg_head = me->next;
    if (me->next == NULL)
	zlog->msg_tail = &zlog->msg_head;
    zlog->msg_count--;
    yarnRelease(zlog->_item.use);

    diff.tv_usec = me->when.tv_usec - zlog->start.tv_usec;
    diff.tv_sec = me->when.tv_sec - zlog->start.tv_sec;
    if (diff.tv_usec < 0) {
	diff.tv_usec += 1000000L;
	diff.tv_sec--;
    }
    fprintf(fp, "trace %ld.%06ld %s\n",
	    (long)diff.tv_sec, (long)diff.tv_usec, me->msg);
    (void) fflush(fp);
    me->msg = _free(me->msg);
    me = _free(me);
    return 1;
}

rpmzLog rpmzLogFree(rpmzLog zlog)
{
    long nrefs;
    rpmzMsg me;

    if (zlog == NULL)
	return NULL;

    yarnPossess(zlog->_item.use);
    nrefs = yarnPeekLock(zlog->_item.use);
if (_rpmzlog_debug)
fprintf(stderr, "    -- zlog %p[%ld]\n", zlog, nrefs);
#ifdef  NOTYET
assert(nrefs > 0);
#else
if (nrefs <= 0)
fprintf(stderr, "==> FIXME: %s: zlog %p[%ld]\n", __FUNCTION__, zlog, nrefs);
#endif
    if (nrefs == 1) {
	yarnLock use = zlog->_item.use;
	if (zlog->msg_tail != NULL) {
	    while ((me = zlog->msg_head) != NULL) {
		zlog->msg_head = me->next;
		me->msg = _free(me->msg);
/*@-compdestroy@*/
		me = _free(me);
/*@=compdestroy@*/
		zlog->msg_count--;
	    }
#ifdef  NOTYET
assert(zlog->msg_count == 0);
#else
if (zlog->msg_count != 0)
fprintf(stderr, "==> FIXME: %s: zlog %p[%ld] count %d\n", __FUNCTION__, zlog, nrefs, zlog->msg_count);
#endif
	    zlog->msg_count = 0;
	    zlog->msg_tail = NULL;
	}
	zlog = _free(zlog);
	yarnTwist(use, BY, -1);
	use = yarnFreeLock(use);
    } else
	yarnTwist(zlog->_item.use, BY, -1);
    return NULL;
}

rpmzLog rpmzLogDump(rpmzLog zlog, FILE * fp)
{
    while (rpmzMsgShow(zlog, fp))
	{;}

    return rpmzLogFree(zlog);
}
