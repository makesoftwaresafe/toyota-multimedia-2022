/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "system.h"

#include <rpmiotypes.h>
#include <poptIO.h>
#include <ugid.h>

#include "debug.h"

static char * user;		/* user to switch to before running program */
static char * group;		/* group to switch to ... */
static char * grouplist;	/* group list to switch to ... */

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {
  { "user",'u', POPT_ARG_STRING,	&user, 0,
        N_("Set primary USER"), N_("USER") },
  { "group",'g', POPT_ARG_STRING,	&group, 0,
        N_("Set primary GROUP"), N_("GROUP") },
  { "groups",'G', POPT_ARG_STRING,	&grouplist, 0,
        N_("Set primary GROUPS"), N_("GROUPS") },

#ifdef	NOTYET
  POPT_AUTOALIAS
#endif
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        "\
Usage: chroot [-g group] [-G group,group,...] [-u user] newroot [command]\n\
", NULL },

  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext optCon;
    struct group * gp;
    struct passwd * pw;
    char * endp;
    const char * shell;
    gid_t gid = 0;
    uid_t uid = 0;
    int gids;
    long maxgids = sysconf(_SC_NGROUPS_MAX) + 1;
    gid_t * gidlist = xmalloc(sizeof(gid_t) * maxgids);

    __progname = "chroot";
    optCon = rpmioInit(argc, argv, optionsTable);

    if (group != NULL) {
	if (xisdigit((int)*group)) {
	    gid = (gid_t)strtoul(group, &endp, 0);
	    if (*endp == '\0')
		goto gotgroup;
	}
	if ((gp = getgrnam(group)) == NULL)
	    errx(1, "no such group `%s'", group);
	gid = gp->gr_gid;
    }
gotgroup:

    for (gids = 0; gids < maxgids; gids++) {
	char * p;
	if ((p = strsep(&grouplist, ",")) == NULL)
	    break;
	if (*p == '\0')
	    continue;

	if (xisdigit((int)*p)) {
	    gidlist[gids] = (gid_t)strtoul(p, &endp, 0);
	    if (*endp == '\0')
		continue;
	}
	if ((gp = getgrnam(p)) == NULL)
	    errx(1, "no such group `%s'", p);
	gidlist[gids] = gp->gr_gid;
    }
    if (gids == maxgids)
	errx(1, "too many supplementary groups provided");

    if (user != NULL) {
	if (xisdigit((int)*user)) {
	    uid = (uid_t)strtoul(user, &endp, 0);
	    if (*endp == '\0')
		goto gotuser;
	}
	if ((pw = getpwnam(user)) == NULL)
	    errx(1, "no such user `%s'", user);
	uid = pw->pw_uid;
    }
gotuser:

    if (chdir(argv[0]) == -1 || chroot(".") == -1)
	err(1, "%s", argv[0]);

    if (gids && setgroups(gids, gidlist) == -1)
	err(1, "setgroups");
    if (group && setgid(gid) == -1)
	err(1, "setgid");
    if (user && setuid(uid) == -1)
	err(1, "setuid");

    if (argv[1]) {
	execvp(argv[1], &argv[1]);
	err(1, "%s", argv[1]);
    }

#if !defined(_PATH_BSHELL)
#define	_PATH_BSHELL	"/bin/sh"
#endif
    if (!(shell = getenv("SHELL")))
	shell = _PATH_BSHELL;
    execlp(shell, shell, "-i", NULL);
    err(1, "%s", shell);
    /*@notreached@*/
}
