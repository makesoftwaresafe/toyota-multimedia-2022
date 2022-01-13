#ifndef GUARD_MNTOPT_H
#define GUARD_MNTOPT_H

#include <mntent.h>

/* filesystem type */
#define MNTTYPE_EXT2		"ext2"	/* 2nd Extended file system */
#define MNTTYPE_EXT3		"ext3"	/* ext2 + journaling */
#define MNTTYPE_EXT4		"ext4"	/* ext4 filesystem */
#define MNTTYPE_NEXT3		"next3"	/* next3 filesystem */
#define MNTTYPE_EXT4DEV		"ext4dev"/* ext4dev filesystem */
#define MNTTYPE_MINIX		"minix"	/* MINIX file system */
#define MNTTYPE_UFS		"ufs"	/* UNIX file system */
#define MNTTYPE_UDF		"udf"	/* OSTA UDF file system */
#define MNTTYPE_REISER		"reiserfs"	/* Reiser file system */
#define MNTTYPE_XFS		"xfs"	/* SGI XFS file system */
#define MNTTYPE_AUTOFS		"autofs"	/* Automount mountpoint */
#define MNTTYPE_JFS		"jfs"	/* JFS file system */
#define MNTTYPE_NFS4		"nfs4"	/* NFSv4 filesystem */
#define MNTTYPE_MPFS		"mpfs"  /* EMC Celerra MPFS filesystem */
#define MNTTYPE_OCFS2		"ocfs2"	/* Oracle Cluster filesystem */
#define MNTTYPE_GFS2		"gfs2"	/* Red Hat Global filesystem 2 */

/* mount options */
#define MNTOPT_NOQUOTA		"noquota"	/* don't enforce quota */
#define MNTOPT_QUOTA		"quota"	/* enforce user quota */
#define MNTOPT_USRQUOTA		"usrquota"	/* enforce user quota */
#define MNTOPT_USRJQUOTA	"usrjquota"	/* enforce user quota */
#define MNTOPT_GRPQUOTA		"grpquota"	/* enforce group quota */
#define MNTOPT_GRPJQUOTA	"grpjquota"	/* enforce group quota */
#define MNTOPT_RSQUASH		"rsquash"	/* root as ordinary user */
#define MNTOPT_BIND		"bind"		/* binded mount */
#define MNTOPT_LOOP		"loop"		/* loopback mount */
#define MNTOPT_JQFMT		"jqfmt"		/* journaled quota format */

#endif
