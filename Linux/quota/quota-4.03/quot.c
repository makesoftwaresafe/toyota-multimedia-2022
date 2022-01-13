/*
 * Copyright (c) 1980, 1990 Regents of the University of California.
 * Copyright (C) 2000, 2001 Silicon Graphics, Inc. [SGI]
 * All rights reserved.
 *
 * [Extensions to support XFS are copyright SGI]
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <utmp.h>
#include <pwd.h>
#include <grp.h>
#include <stdint.h>

#include "pot.h"
#include "quot.h"
#include "common.h"
#include "mntopt.h"
#include "bylabel.h"
#include "quotasys.h"

#define	TSIZE	500
static uint64_t sizes[TSIZE];
static uint64_t overflow;

static int aflag;
static int cflag;
static int fflag;
static int gflag;
static int uflag;
static int vflag;
static int iflag;
static int qflag;
static int Tflag;
static time_t now;
char *progname;

static void mounttable(void);
static char *idname(uint32_t, int);
static void report(const char *, const char *, int);
static void creport(const char *, const char *);

static void usage(void)
{
	errstr(_("Usage: %s [-acfugvViTq] [filesystem...]\n"), progname);
	exit(1);
}

int main(int argc, char **argv)
{
	int c;

	now = time(0);
	progname = basename(argv[0]);

	while ((c = getopt(argc, argv, "acfguvVTq")) != -1) {
		switch (c) {
		  case 'a':
			  aflag++;
			  break;
		  case 'c':
			  cflag++;
			  break;
		  case 'f':
			  fflag++;
			  break;
		  case 'g':
			  gflag++;
			  break;
		  case 'u':
			  uflag++;
			  break;
		  case 'v':
			  vflag++;
			  break;
		  case 'i':
			  iflag++;
			  break;
		  case 'q':
			  qflag++;
			  break;
		  case 'T':
			  Tflag++;
			  break;
		  case 'V':
			  version();
			  exit(0);
		  default:
			  usage();
		}
	}
	if ((aflag && optind != argc) || (!aflag && optind == argc))
		usage();
	if (!uflag && !gflag)
		uflag++;
	if (init_mounts_scan(aflag ? 0 : argc - optind, argv + optind, (iflag ? MS_NO_AUTOFS : 0)) < 0)
		return 1;
	mounttable();
	end_mounts_scan();
	return 0;
}

static void mounttable(void)
{
	int doit = 0;
	struct mount_entry *mntp;

	while ((mntp = get_next_mount())) {
		/* Currently, only XFS is implemented... */
		if (strcmp(mntp->me_type, MNTTYPE_XFS) == 0) {
			checkXFS(mntp->me_devname, mntp->me_dir);
			doit = 1;
		}
		/* ...additional filesystems types here. */

		if (doit) {
			if (cflag) creport(mntp->me_devname, mntp->me_dir);
			if (!cflag && uflag) report(mntp->me_devname, mntp->me_dir, 0);
			if (!cflag && gflag) report(mntp->me_devname, mntp->me_dir, 1);
		}
	}
}

static int qcmp(du_t * p1, du_t * p2)
{
	if (p1->blocks > p2->blocks)
		return -1;
	if (p1->blocks < p2->blocks)
		return 1;
	if (p1->id > p2->id)
		return 1;
	else if (p1->id < p2->id)
		return -1;
	return 0;
}

static void creport(const char *file, const char *fsdir)
{
	int i;
	uint64_t t = 0;

	printf(_("%s (%s):\n"), file, fsdir);
	for (i = 0; i < TSIZE - 1; i++)
		if (sizes[i] > 0) {
			t += sizes[i] * i;
			printf(_("%d\t%llu\t%llu\n"), i,
			       (unsigned long long) sizes[i],
			       (unsigned long long) t);
		}
	printf(_("%d\t%llu\t%llu\n"), TSIZE - 1,
	       (unsigned long long) sizes[TSIZE - 1],
               (unsigned long long) (overflow + t));
}

static void report(const char *file, const char *fsdir, int type)
{
	du_t *dp;

	printf(_("%s (%s) %ss:\n"), file, fsdir, _(type2name(type)));
	if (!qflag)
		qsort(du[type], ndu[type], sizeof(du[type][0]), (int (*)(const void *, const void *))qcmp);
	for (dp = du[type]; dp < &du[type][ndu[type]]; dp++) {
		char *cp;

		if (dp->blocks == 0)
			return;
		printf(_("%8llu    "), (unsigned long long) dp->blocks);
		if (fflag)
			printf(_("%8llu    "), (unsigned long long) dp->nfiles);
		if ((cp = idname(dp->id, type)) != NULL) {
			if (Tflag)
				printf(_("%s"), cp);
			else
				printf(_("%-8.8s"), cp);
		} else
			printf(_("#%-7d"), dp->id);
		if (vflag)
			printf(_("    %8llu    %8llu    %8llu"),
			       (unsigned long long) dp->blocks30,
			       (unsigned long long) dp->blocks60,
			       (unsigned long long) dp->blocks90);
		putchar('\n');
	}
}

static idcache_t *getnextent(int type, uint32_t id, int byid)
{
	struct passwd *pw;
	struct group  *gr;
	static idcache_t idc;

	if (type) {	/* /etc/group */
		if ((gr = byid? getgrgid(id) : getgrent()) == NULL)
			return NULL;
		idc.id = gr->gr_gid;
		strncpy(idc.name, gr->gr_name, UT_NAMESIZE);
		return &idc;
	}
	/* /etc/passwd */
	if ((pw = byid? getpwuid(id) : getpwent()) == NULL)
		return NULL;
	idc.id = pw->pw_uid;
	strncpy(idc.name, pw->pw_name, UT_NAMESIZE);
	return &idc;
}

static char *idname(uint32_t id, int type)
{
	idcache_t *ncp, *idp;
	static idcache_t nc[2][NID];
	static int entriesleft[2] = { NID, NID };

	/* check cache for name first */
	ncp = &nc[type][id & IDMASK];
	if (ncp->id == id && ncp->name[0])
		return ncp->name;
	if (entriesleft[type]) {
		/*
		 * If we haven't gone through the passwd/group file
		 * then fill the cache while seaching for name.
		 * This lets us run through passwd/group serially.
		 */
		if (entriesleft[type] == NID)
			type? setgrent() : setpwent();
		while (((idp = getnextent(type, id, 0)) != NULL) && entriesleft[type]) {
			entriesleft[type]--;
			ncp = &nc[type][idp->id & IDMASK];
			if (ncp->name[0] == '\0' || idp->id == id)
				memcpy(ncp, idp, sizeof(idcache_t));
			if (idp->id == id)
				return ncp->name;
		}
		type? endgrent() : endpwent();
		entriesleft[type] = 0;
		ncp = &nc[type][id & IDMASK];
	}

	/* Not cached - do it the slow way & insert into cache */
	if ((idp = getnextent(type, id, 1)) == NULL)
		return NULL;
	memcpy(ncp, idp, sizeof(idcache_t));
	return ncp->name;
}

/*
 *	=== XFS specific code follows ===
 */

static void acctXFS(xfs_bstat_t *p)
{
	register du_t *dp;
	du_t **hp;
	uint64_t size;
	uint32_t i, id;

	if ((p->bs_mode & S_IFMT) == 0)
		return;
	size = howmany((p->bs_blocks * p->bs_blksize), 0x400ULL);

	if (cflag) {
		if (!(S_ISDIR(p->bs_mode) || S_ISREG(p->bs_mode)))
			return;
		if (size >= TSIZE) {
			overflow += size;
			size = TSIZE - 1;
		}
		sizes[(int)size]++;
		return;
	}
	for (i = 0; i < 2; i++) {
		id = (i == 0)? p->bs_uid : p->bs_gid;
		hp = &duhash[i][id % DUHASH];
		for (dp = *hp; dp; dp = dp->next)
			if (dp->id == id)
				break;
		if (dp == 0) {
			if (ndu[i] >= NDU)
				return;
			dp = &du[i][(ndu[i]++)];
			dp->next = *hp;
			*hp = dp;
			dp->id = id;
			dp->nfiles = 0;
			dp->blocks = 0;
			dp->blocks30 = 0;
			dp->blocks60 = 0;
			dp->blocks90 = 0;
		}
		dp->blocks += size;

		if (now - p->bs_atime.tv_sec > 30 * SEC24HR)
			dp->blocks30 += size;
		if (now - p->bs_atime.tv_sec > 60 * SEC24HR)
			dp->blocks60 += size;
		if (now - p->bs_atime.tv_sec > 90 * SEC24HR)
			dp->blocks90 += size;
		dp->nfiles++;
	}
}

static void checkXFS(const char *file, const char *fsdir)
{
	xfs_fsop_bulkreq_t bulkreq;
	__u64 last = 0;
	__s32 count;
	int i;
	int sts;
	int fsfd;
	du_t **dp;
	xfs_bstat_t *buf;

	/*
	 * Initialize tables between checks; because of the qsort
	 * in report() the hash tables must be rebuilt each time.
	 */
	for (sts = 0; sts < TSIZE; sts++)
		sizes[sts] = 0;
	overflow = 0;
	for (i = 0; i < 2; i++)
		for (dp = duhash[i]; dp < &duhash[i][DUHASH]; dp++)
			*dp = 0;
	ndu[0] = ndu[1] = 0;

	fsfd = open(fsdir, O_RDONLY);
	if (fsfd < 0) {
		errstr(_("cannot open %s: %s\n"), fsdir, strerror(errno));
		exit(1);
	}
	sync();

	buf = (xfs_bstat_t *) smalloc(NBSTAT * sizeof(xfs_bstat_t));
	memset(buf, 0, NBSTAT * sizeof(xfs_bstat_t));

	bulkreq.lastip = &last;
	bulkreq.icount = NBSTAT;
	bulkreq.ubuffer = buf;
	bulkreq.ocount = &count;

	while ((sts = ioctl(fsfd, XFS_IOC_FSBULKSTAT, &bulkreq)) == 0) {
		if (count == 0)
			break;
		for (i = 0; i < count; i++)
			acctXFS(&buf[i]);
	}
	if (sts < 0) {
		errstr(_("XFS_IOC_FSBULKSTAT ioctl failed: %s\n"),
			strerror(errno));
		exit(1);
	}
	free(buf);
	close(fsfd);
}
