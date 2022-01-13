/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains code to implement the "sqlite" command line
** utility for accessing SQLite databases.
*/

/*
** Include the configuration header output by 'configure' if we're using the
** autoconf-based build
*/

#include "system.h"

#include <stdarg.h>

#if defined(WITH_DBSQL)
#include <dbsql.h>
#elif defined(WITH_SQLITE)
#define SQLITE_OS_UNIX 1	/* XXX needed? */
#define SQLITE_THREADSAFE 1
#define SQLITE_TEMP_STORE 1
#include <sqlite3.h>
#endif	/* WITH_SQLITE */

#define	_RPMSQL_INTERNAL
#include <rpmsql.h>

#include "debug.h"

/*==============================================================*/

#ifdef SIGINT
/*
** This routine runs when the user presses Ctrl-C
*/
static void interrupt_handler(int NotUsed)
{
    NotUsed = NotUsed;
    _rpmsqlSeenInterrupt = 1;
#if defined(WITH_SQLITE)
    if (_rpmsqlI && _rpmsqlI->I) {
	sqlite3 * db = (sqlite3 *) _rpmsqlI->I;
	sqlite3_interrupt(db);
    }
#endif
}
#endif

int main(int argc, char **argv)
{
    int _flags = (isatty(0) ? RPMSQL_FLAGS_INTERACTIVE : 0);
    rpmsql sql = rpmsqlNew(argv, _flags | 0x80000000);
    const char * zFirstCmd = NULL;
    const char ** av = NULL;
    int ac;
    int ec = 1;		/* assume error */

    /* Make sure we have a valid signal handler early, before anything
     ** else is done.
     */
#ifdef SIGINT
    signal(SIGINT, interrupt_handler);
#endif

    /* Do an initial pass through the command-line argument to locate
     ** the name of the database file, the name of the initialization file,
     ** and the first command to execute.
     */
    av = rpmsqlArgv(sql, &ac);
    if (ac > 2) {
	fprintf(stderr, "%s: Error: too many options: \"%s ...\"\n", __progname,
		av[2]);
	goto exit;
    }
    if (ac > 1)
	zFirstCmd = av[1];

    if (zFirstCmd) {
	/* Run just the command that follows the database name */
	ec = (int) rpmsqlRun(NULL, zFirstCmd, NULL);	/* FILE | STRING */
    } else {
	/* Run commands received from standard input */
	if (F_ISSET(sql, INTERACTIVE))
	    ec = (int) rpmsqlRun(NULL, "", NULL);	/* INTERACTIVE */
	else
	    ec = (int) rpmsqlRun(NULL, "-", NULL);	/* STDIN */
    }

exit:

    sql = rpmsqlFree(sql);
    rpmioClean();

    return ec;
}
