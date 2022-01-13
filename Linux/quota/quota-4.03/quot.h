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

#define	SEC24HR	(60*60*24)	/* seconds per day */

typedef struct {
	uint32_t id;
	char name[UT_NAMESIZE + 1];
} idcache_t;

typedef struct du {
	struct du *next;
	uint64_t blocks;
	uint64_t blocks30;
	uint64_t blocks60;
	uint64_t blocks90;
	uint64_t nfiles;
	uint32_t id;
} du_t;

#define	NDU	60000
#define	DUHASH	8209
static du_t du[2][NDU];
static du_t *duhash[2][DUHASH];
static int ndu[2];

#define NID	256
#define IDMASK	(NID-1)

/*
 *	=== Start XFS specific types and definitions ===
 */
#include <asm/types.h>

/* Structures returned from ioctl XFS_IOC_FSBULKSTAT */
typedef struct xfs_bstime {
	time_t tv_sec;		/* seconds                      */
	__s32 tv_nsec;		/* and nanoseconds              */
} xfs_bstime_t;

typedef struct xfs_bstat {
	__u64 bs_ino;		/* inode number                 */
	__u16 bs_mode;		/* type and mode                */
	__u16 bs_nlink;		/* number of links              */
	__u32 bs_uid;		/* user id                      */
	__u32 bs_gid;		/* group id                     */
	__u32 bs_rdev;		/* device value                 */
	__s32 bs_blksize;	/* block size                   */
	__s64 bs_size;		/* file size                    */
	xfs_bstime_t bs_atime;	/* access time                  */
	xfs_bstime_t bs_mtime;	/* modify time                  */
	xfs_bstime_t bs_ctime;	/* inode change time            */
	int64_t bs_blocks;	/* number of blocks             */
	__u32 bs_xflags;	/* extended flags               */
	__s32 bs_extsize;	/* extent size                  */
	__s32 bs_extents;	/* number of extents            */
	__u32 bs_gen;		/* generation count             */
	__u16 bs_projid;	/* project id                   */
	unsigned char bs_pad[14];	/* pad space, unused            */
	__u32 bs_dmevmask;	/* DMIG event mask              */
	__u16 bs_dmstate;	/* DMIG state info              */
	__u16 bs_aextents;	/* attribute number of extents  */
} xfs_bstat_t;

/* The user-level BulkStat Request interface structure. */
typedef struct xfs_fsop_bulkreq {
	__u64 *lastip;		/* last inode # pointer         */
	__s32 icount;		/* count of entries in buffer   */
	void *ubuffer;		/* user buffer for inode desc.  */
	__s32 *ocount;		/* output count pointer         */
} xfs_fsop_bulkreq_t;

#ifndef XFS_IOC_FSBULKSTAT
#define XFS_IOC_FSBULKSTAT	_IOWR('X', 101, struct xfs_fsop_bulkreq)
#endif

#define NBSTAT  4069		/* XFS bulkstat inodes */
static void checkXFS(const char *file, const char *fsdir);

/*
 *	=== End of XFS specific types and definitions ===
 */
