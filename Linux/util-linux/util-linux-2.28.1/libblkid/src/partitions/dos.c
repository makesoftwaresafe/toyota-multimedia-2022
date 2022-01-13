/*
 * MS-DOS partition parsing code
 *
 * Copyright (C) 2009 Karel Zak <kzak@redhat.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 *
 * Inspired by fdisk, partx, Linux kernel and libparted.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "partitions.h"
#include "aix.h"

/* see superblocks/vfat.c */
extern int blkid_probe_is_vfat(blkid_probe pr);

static const struct dos_subtypes {
	unsigned char type;
	const struct blkid_idinfo *id;
} dos_nested[] = {
	{ MBR_FREEBSD_PARTITION, &bsd_pt_idinfo },
	{ MBR_NETBSD_PARTITION, &bsd_pt_idinfo },
	{ MBR_OPENBSD_PARTITION, &bsd_pt_idinfo },
	{ MBR_UNIXWARE_PARTITION, &unixware_pt_idinfo },
	{ MBR_SOLARIS_X86_PARTITION, &solaris_x86_pt_idinfo },
	{ MBR_MINIX_PARTITION, &minix_pt_idinfo }
};

static inline int is_extended(struct dos_partition *p)
{
	return (p->sys_ind == MBR_DOS_EXTENDED_PARTITION ||
		p->sys_ind == MBR_W95_EXTENDED_PARTITION ||
		p->sys_ind == MBR_LINUX_EXTENDED_PARTITION);
}

static int parse_dos_extended(blkid_probe pr, blkid_parttable tab,
		uint32_t ex_start, uint32_t ex_size, int ssf)
{
	blkid_partlist ls = blkid_probe_get_partlist(pr);
	uint32_t cur_start = ex_start, cur_size = ex_size;
	unsigned char *data;
	int ct_nodata = 0;	/* count ext.partitions without data partitions */
	int i;

	DBG(LOWPROBE, ul_debug("parse EBR [start=%d, size=%d]", ex_start/ssf, ex_size/ssf));
	if (ex_start == 0) {
		DBG(LOWPROBE, ul_debug("Bad offset in primary extended partition -- ignore"));
		return 0;
	}

	while (1) {
		struct dos_partition *p, *p0;
		uint32_t start, size;

		if (++ct_nodata > 100)
			return BLKID_PROBE_OK;
		data = blkid_probe_get_sector(pr, cur_start);
		if (!data) {
			if (errno)
				return -errno;
			goto leave;	/* malformed partition? */
		}

		if (!mbr_is_valid_magic(data))
			goto leave;

		p0 = mbr_get_partition(data, 0);

		/* Usually, the first entry is the real data partition,
		 * the 2nd entry is the next extended partition, or empty,
		 * and the 3rd and 4th entries are unused.
		 * However, DRDOS sometimes has the extended partition as
		 * the first entry (when the data partition is empty),
		 * and OS/2 seems to use all four entries.
		 * -- Linux kernel fs/partitions/dos.c
		 *
		 * See also http://en.wikipedia.org/wiki/Extended_boot_record
		 */

		/* Parse data partition */
		for (p = p0, i = 0; i < 4; i++, p++) {
			uint32_t abs_start;
			blkid_partition par;

			/* the start is relative to the parental ext.partition */
			start = dos_partition_get_start(p) * ssf;
			size = dos_partition_get_size(p) * ssf;
			abs_start = cur_start + start;	/* absolute start */

			if (!size || is_extended(p))
				continue;
			if (i >= 2) {
				/* extra checks to detect real data on
				 * 3rd and 4th entries */
				if (start + size > cur_size)
					continue;
				if (abs_start < ex_start)
					continue;
				if (abs_start + size > ex_start + ex_size)
					continue;
			}

			/* Avoid recursive non-empty links, see ct_nodata counter */
			if (blkid_partlist_get_partition_by_start(ls, abs_start)) {
				DBG(LOWPROBE, ul_debug("#%d: EBR duplicate data partition [abs start=%u] -- ignore",
							i + 1, abs_start));
				continue;
			}

			par = blkid_partlist_add_partition(ls, tab, abs_start, size);
			if (!par)
				return -ENOMEM;

			blkid_partition_set_type(par, p->sys_ind);
			blkid_partition_set_flags(par, p->boot_ind);
			blkid_partition_gen_uuid(par);
			ct_nodata = 0;
		}
		/* The first nested ext.partition should be a link to the next
		 * logical partition. Everything other (recursive ext.partitions)
		 * is junk.
		 */
		for (p = p0, i = 0; i < 4; i++, p++) {
			start = dos_partition_get_start(p) * ssf;
			size = dos_partition_get_size(p) * ssf;

			if (size && is_extended(p)) {
				if (start == 0)
					DBG(LOWPROBE, ul_debug("#%d: EBR link offset is zero -- ignore", i + 1));
				else
					break;
			}
		}
		if (i == 4)
			goto leave;

		cur_start = ex_start + start;
		cur_size = size;
	}
leave:
	return BLKID_PROBE_OK;
}

static int probe_dos_pt(blkid_probe pr,
		const struct blkid_idmag *mag __attribute__((__unused__)))
{
	int i;
	int ssf;
	blkid_parttable tab = NULL;
	blkid_partlist ls;
	struct dos_partition *p0, *p;
	unsigned char *data;
	uint32_t start, size, id;
	char idstr[37];


	data = blkid_probe_get_sector(pr, 0);
	if (!data) {
		if (errno)
			return -errno;
		goto nothing;
	}

	/* ignore disks with AIX magic number -- for more details see aix.c */
	if (memcmp(data, BLKID_AIX_MAGIC_STRING, BLKID_AIX_MAGIC_STRLEN) == 0)
		goto nothing;

	p0 = mbr_get_partition(data, 0);

	/*
	 * Reject PT where boot indicator is not 0 or 0x80.
	 */
	for (p = p0, i = 0; i < 4; i++, p++)
		if (p->boot_ind != 0 && p->boot_ind != 0x80) {
			DBG(LOWPROBE, ul_debug("missing boot indicator -- ignore"));
			goto nothing;
		}

	/*
	 * GPT uses valid MBR
	 */
	for (p = p0, i = 0; i < 4; i++, p++) {
		if (p->sys_ind == MBR_GPT_PARTITION) {
			DBG(LOWPROBE, ul_debug("probably GPT -- ignore"));
			goto nothing;
		}
	}

	/*
	 * If the current magic string is equal to boot signature (i.e. 55AA)
	 * and a valid vfat filesystem exists, then blkid has already failed
	 * to find a valid partition type identifier in the mbr.
	 *
	 * Stop processing any further as the current magic string is part of
	 * superblock probing (i.e. added by some utility in lieu of vfat
	 * filesystem) instead of dos partition.
	 *
	 * NOTE: "55AA" in hexadecimal is equal to "U\252" in ASCII.
	 */
	const char* bootSig = "U\252";
	if ( (510 == mag->sboff) && (2 == mag->len)
	     && (0 == strncmp(bootSig, mag->magic, sizeof(mag->magic))) )
	{
		/*
		 * Now that the 55aa signature is present, this is probably
		 * either the boot sector of a FAT filesystem or a DOS-type
		 * partition table.
		 */
		if (blkid_probe_is_vfat(pr)) {
			DBG(LOWPROBE, ul_debug("probably FAT -- ignore"));
			goto nothing;
		}
	}

	blkid_probe_use_wiper(pr, MBR_PT_OFFSET, 512 - MBR_PT_OFFSET);

	id = mbr_get_id(data);
	if (id)
		snprintf(idstr, sizeof(idstr), "%08x", id);

	/*
	 * Well, all checks pass, it's MS-DOS partiton table
	 */
	if (blkid_partitions_need_typeonly(pr)) {
		/* Non-binary interface -- caller does not ask for details
		 * about partitions, just set generic varibles only. */
		if (id)
			blkid_partitions_strcpy_ptuuid(pr, idstr);
		return 0;
	}

	ls = blkid_probe_get_partlist(pr);
	if (!ls)
		goto nothing;

	/* sector size factor (the start and size are in the real sectors, but
	 * we need to convert all sizes to 512 logical sectors
	 */
	ssf = blkid_probe_get_sectorsize(pr) / 512;

	/* allocate a new partition table */
	tab = blkid_partlist_new_parttable(ls, "dos", MBR_PT_OFFSET);
	if (!tab)
		return -ENOMEM;

	if (id)
		blkid_parttable_set_id(tab, (unsigned char *) idstr);

	/* Parse primary partitions */
	for (p = p0, i = 0; i < 4; i++, p++) {
		blkid_partition par;

		start = dos_partition_get_start(p) * ssf;
		size = dos_partition_get_size(p) * ssf;

		if (!size) {
			/* Linux kernel ignores empty partitions, but partno for
			 * the empty primary partitions is not reused */
			blkid_partlist_increment_partno(ls);
			continue;
		}
		par = blkid_partlist_add_partition(ls, tab, start, size);
		if (!par)
			return -ENOMEM;

		blkid_partition_set_type(par, p->sys_ind);
		blkid_partition_set_flags(par, p->boot_ind);
		blkid_partition_gen_uuid(par);
	}

	/* Linux uses partition numbers greater than 4
	 * for all logical partition and all nested partition tables (bsd, ..)
	 */
	blkid_partlist_set_partno(ls, 5);

	/* Parse logical partitions */
	for (p = p0, i = 0; i < 4; i++, p++) {
		start = dos_partition_get_start(p) * ssf;
		size = dos_partition_get_size(p) * ssf;

		if (!size)
			continue;
		if (is_extended(p) &&
		    parse_dos_extended(pr, tab, start, size, ssf) == -1)
			goto nothing;
	}

	/* Parse subtypes (nested partitions) on large disks */
	if (!blkid_probe_is_tiny(pr)) {
		for (p = p0, i = 0; i < 4; i++, p++) {
			size_t n;
			int rc;

			if (!dos_partition_get_size(p) || is_extended(p))
				continue;

			for (n = 0; n < ARRAY_SIZE(dos_nested); n++) {
				if (dos_nested[n].type != p->sys_ind)
					continue;

				rc = blkid_partitions_do_subprobe(pr,
						blkid_partlist_get_partition(ls, i),
						dos_nested[n].id);
				if (rc < 0)
					return rc;
				break;
			}
		}
	}
	return BLKID_PROBE_OK;

nothing:
	return BLKID_PROBE_NONE;
}


const struct blkid_idinfo dos_pt_idinfo =
{
	.name		= "dos",
	.probefunc	= probe_dos_pt,
	.magics		=
	{
		/* DOS master boot sector:
		 *
		 *     0 | Code Area
		 *   440 | Optional Disk signature
		 *   446 | Partition table
		 *   510 | 0x55
		 *   511 | 0xAA
		 */
		//{ .magic = "\x00", .len = 1, .sboff = 450 },    // EMPTY
		{ .magic = "\x01", .len = 1, .sboff = 450 },    // FAT12
		{ .magic = "\x02", .len = 1, .sboff = 450 },    // XENIX root
		{ .magic = "\x03", .len = 1, .sboff = 450 },    // XENIX usr
		{ .magic = "\x04", .len = 1, .sboff = 450 },    // FAT16 <32M
		{ .magic = "\x05", .len = 1, .sboff = 450 },    // Extended
		{ .magic = "\x06", .len = 1, .sboff = 450 },    // FAT16
		{ .magic = "\x07", .len = 1, .sboff = 450 },    // HPFS/NTFS/exFAT
		{ .magic = "\x08", .len = 1, .sboff = 450 },    // AIX
		{ .magic = "\x09", .len = 1, .sboff = 450 },    // AIX bootable
		{ .magic = "\x0a", .len = 1, .sboff = 450 },    // OS/2 Boot Manager
		{ .magic = "\x0b", .len = 1, .sboff = 450 },    // W95 FAT32
		{ .magic = "\x0c", .len = 1, .sboff = 450 },    // W95 FAT32 (LBA)
		{ .magic = "\x0e", .len = 1, .sboff = 450 },    // W95 FAT16 (LBA)
		{ .magic = "\x0f", .len = 1, .sboff = 450 },    // W95 Ext'd (LBA)
		{ .magic = "\x10", .len = 1, .sboff = 450 },    // OPUS
		{ .magic = "\x11", .len = 1, .sboff = 450 },    // Hidden FAT12
		{ .magic = "\x12", .len = 1, .sboff = 450 },    // Compaq diagnostics
		{ .magic = "\x14", .len = 1, .sboff = 450 },    // Hidden FAT16 <3
		{ .magic = "\x16", .len = 1, .sboff = 450 },    // Hidden FAT16
		{ .magic = "\x17", .len = 1, .sboff = 450 },    // Hidden IFS/HPFS/NTFS/exFAT
		{ .magic = "\x18", .len = 1, .sboff = 450 },    // AST SmartSleep
		{ .magic = "\x1b", .len = 1, .sboff = 450 },    // Hidden W95 FAT3
		{ .magic = "\x1c", .len = 1, .sboff = 450 },    // Hidden W95 FAT3
		{ .magic = "\x1e", .len = 1, .sboff = 450 },    // Hidden W95 FAT1
		{ .magic = "\x24", .len = 1, .sboff = 450 },    // NEC DOS
		{ .magic = "\x27", .len = 1, .sboff = 450 },    // Hidden NTFS Win
		{ .magic = "\x39", .len = 1, .sboff = 450 },    // Plan 9
		{ .magic = "\x3c", .len = 1, .sboff = 450 },    // PartitionMagic
		{ .magic = "\x40", .len = 1, .sboff = 450 },    // Venix 80286
		{ .magic = "\x41", .len = 1, .sboff = 450 },    // PPC PReP Boot
		{ .magic = "\x42", .len = 1, .sboff = 450 },    // SFS
		{ .magic = "\x4d", .len = 1, .sboff = 450 },    // QNX4.x
		{ .magic = "\x4e", .len = 1, .sboff = 450 },    // QNX4.x 2nd part
		{ .magic = "\x4f", .len = 1, .sboff = 450 },    // QNX4.x 3rd part
		{ .magic = "\x50", .len = 1, .sboff = 450 },    // OnTrack DM
		{ .magic = "\x51", .len = 1, .sboff = 450 },    // OnTrack DM6 Aux
		{ .magic = "\x52", .len = 1, .sboff = 450 },    // CP/M
		{ .magic = "\x53", .len = 1, .sboff = 450 },    // OnTrack DM6 Aux
		{ .magic = "\x54", .len = 1, .sboff = 450 },    // OnTrackDM6
		{ .magic = "\x55", .len = 1, .sboff = 450 },    // EZ-Drive
		{ .magic = "\x56", .len = 1, .sboff = 450 },    // Golden Bow
		{ .magic = "\x5c", .len = 1, .sboff = 450 },    // Priam Edisk
		{ .magic = "\x61", .len = 1, .sboff = 450 },    // SpeedStor
		{ .magic = "\x63", .len = 1, .sboff = 450 },    // GNU HURD or Sys
		{ .magic = "\x64", .len = 1, .sboff = 450 },    // Novell Netware
		{ .magic = "\x65", .len = 1, .sboff = 450 },    // Novell Netware
		{ .magic = "\x70", .len = 1, .sboff = 450 },    // DiskSecure Multiboot
		{ .magic = "\x75", .len = 1, .sboff = 450 },    // PC/IX
		{ .magic = "\x80", .len = 1, .sboff = 450 },    // Old Minix
		{ .magic = "\x81", .len = 1, .sboff = 450 },    // Minix / old Linux
		{ .magic = "\x82", .len = 1, .sboff = 450 },    // Linux swap / Solaris
		{ .magic = "\x83", .len = 1, .sboff = 450 },    // Linux
		{ .magic = "\x84", .len = 1, .sboff = 450 },    // OS/2 hidden C:
		{ .magic = "\x85", .len = 1, .sboff = 450 },    // Linux extended
		{ .magic = "\x86", .len = 1, .sboff = 450 },    // NTFS volume set
		{ .magic = "\x87", .len = 1, .sboff = 450 },    // NTFS volume set
		{ .magic = "\x88", .len = 1, .sboff = 450 },    // Linux plaintext
		{ .magic = "\x8e", .len = 1, .sboff = 450 },    // Linux LVM
		{ .magic = "\x93", .len = 1, .sboff = 450 },    // Amoeba
		{ .magic = "\x94", .len = 1, .sboff = 450 },    // Amoeba BBT
		{ .magic = "\x9f", .len = 1, .sboff = 450 },    // BSD/OS
		{ .magic = "\xa0", .len = 1, .sboff = 450 },    // IBM Thinkpad hibernation
		{ .magic = "\xa5", .len = 1, .sboff = 450 },    // FreeBSD
		{ .magic = "\xa6", .len = 1, .sboff = 450 },    // OpenBSD
		{ .magic = "\xa7", .len = 1, .sboff = 450 },    // NeXTSTEP
		{ .magic = "\xa8", .len = 1, .sboff = 450 },    // Darwin UFS
		{ .magic = "\xa9", .len = 1, .sboff = 450 },    // NetBSD
		{ .magic = "\xab", .len = 1, .sboff = 450 },    // Darwin boot
		{ .magic = "\xaf", .len = 1, .sboff = 450 },    // HFS / HFS+
		{ .magic = "\xb7", .len = 1, .sboff = 450 },    // BSDI fs
		{ .magic = "\xb8", .len = 1, .sboff = 450 },    // BSDI swap
		{ .magic = "\xbb", .len = 1, .sboff = 450 },    // Boot Wizard hid
		{ .magic = "\xbe", .len = 1, .sboff = 450 },    // Solaris boot
		{ .magic = "\xbf", .len = 1, .sboff = 450 },    // Solaris
		{ .magic = "\xc1", .len = 1, .sboff = 450 },    // DRDOS/sec (FAT-12)
		{ .magic = "\xc4", .len = 1, .sboff = 450 },    // DRDOS/sec (FAT-16)
		{ .magic = "\xc6", .len = 1, .sboff = 450 },    // DRDOS/sec (FAT-16B)
		{ .magic = "\xc7", .len = 1, .sboff = 450 },    // Syrinx
		{ .magic = "\xda", .len = 1, .sboff = 450 },    // Non-FS data
		{ .magic = "\xdb", .len = 1, .sboff = 450 },    // CP/M / CTOS / .
		{ .magic = "\xde", .len = 1, .sboff = 450 },    // Dell Utility
		{ .magic = "\xdf", .len = 1, .sboff = 450 },    // BootIt
		{ .magic = "\xe1", .len = 1, .sboff = 450 },    // DOS access
		{ .magic = "\xe3", .len = 1, .sboff = 450 },    // DOS R/O
		{ .magic = "\xe4", .len = 1, .sboff = 450 },    // SpeedStor
		{ .magic = "\xeb", .len = 1, .sboff = 450 },    // BeOS fs
		{ .magic = "\xee", .len = 1, .sboff = 450 },    // GPT
		{ .magic = "\xef", .len = 1, .sboff = 450 },    // EFI (FAT-12/16/32)
		{ .magic = "\xf0", .len = 1, .sboff = 450 },    // Linux/PA-RISC boot loader
		{ .magic = "\xf1", .len = 1, .sboff = 450 },    // SpeedStor
		{ .magic = "\xf4", .len = 1, .sboff = 450 },    // SpeedStor
		{ .magic = "\xf2", .len = 1, .sboff = 450 },    // DOS secondary
		{ .magic = "\xfb", .len = 1, .sboff = 450 },    // VMware VMFS
		{ .magic = "\xfc", .len = 1, .sboff = 450 },    // VMware VMKCORE
		{ .magic = "\xfd", .len = 1, .sboff = 450 },    // Linux raid auto
		{ .magic = "\xfe", .len = 1, .sboff = 450 },    // LANstep
		{ .magic = "\xff", .len = 1, .sboff = 450 },    // BBT
		//{ .magic = "\x00", .len = 1, .sboff = 466 },    // EMPTY
		{ .magic = "\x01", .len = 1, .sboff = 466 },    // FAT12
		{ .magic = "\x02", .len = 1, .sboff = 466 },    // XENIX root
		{ .magic = "\x03", .len = 1, .sboff = 466 },    // XENIX usr
		{ .magic = "\x04", .len = 1, .sboff = 466 },    // FAT16 <32M
		{ .magic = "\x05", .len = 1, .sboff = 466 },    // Extended
		{ .magic = "\x06", .len = 1, .sboff = 466 },    // FAT16
		{ .magic = "\x07", .len = 1, .sboff = 466 },    // HPFS/NTFS/exFAT
		{ .magic = "\x08", .len = 1, .sboff = 466 },    // AIX
		{ .magic = "\x09", .len = 1, .sboff = 466 },    // AIX bootable
		{ .magic = "\x0a", .len = 1, .sboff = 466 },    // OS/2 Boot Manager
		{ .magic = "\x0b", .len = 1, .sboff = 466 },    // W95 FAT32
		{ .magic = "\x0c", .len = 1, .sboff = 466 },    // W95 FAT32 (LBA)
		{ .magic = "\x0e", .len = 1, .sboff = 466 },    // W95 FAT16 (LBA)
		{ .magic = "\x0f", .len = 1, .sboff = 466 },    // W95 Ext'd (LBA)
		{ .magic = "\x10", .len = 1, .sboff = 466 },    // OPUS
		{ .magic = "\x11", .len = 1, .sboff = 466 },    // Hidden FAT12
		{ .magic = "\x12", .len = 1, .sboff = 466 },    // Compaq diagnostics
		{ .magic = "\x14", .len = 1, .sboff = 466 },    // Hidden FAT16 <3
		{ .magic = "\x16", .len = 1, .sboff = 466 },    // Hidden FAT16
		{ .magic = "\x17", .len = 1, .sboff = 466 },    // Hidden IFS/HPFS/NTFS/exFAT
		{ .magic = "\x18", .len = 1, .sboff = 466 },    // AST SmartSleep
		{ .magic = "\x1b", .len = 1, .sboff = 466 },    // Hidden W95 FAT3
		{ .magic = "\x1c", .len = 1, .sboff = 466 },    // Hidden W95 FAT3
		{ .magic = "\x1e", .len = 1, .sboff = 466 },    // Hidden W95 FAT1
		{ .magic = "\x24", .len = 1, .sboff = 466 },    // NEC DOS
		{ .magic = "\x27", .len = 1, .sboff = 466 },    // Hidden NTFS Win
		{ .magic = "\x39", .len = 1, .sboff = 466 },    // Plan 9
		{ .magic = "\x3c", .len = 1, .sboff = 466 },    // PartitionMagic
		{ .magic = "\x40", .len = 1, .sboff = 466 },    // Venix 80286
		{ .magic = "\x41", .len = 1, .sboff = 466 },    // PPC PReP Boot
		{ .magic = "\x42", .len = 1, .sboff = 466 },    // SFS
		{ .magic = "\x4d", .len = 1, .sboff = 466 },    // QNX4.x
		{ .magic = "\x4e", .len = 1, .sboff = 466 },    // QNX4.x 2nd part
		{ .magic = "\x4f", .len = 1, .sboff = 466 },    // QNX4.x 3rd part
		{ .magic = "\x50", .len = 1, .sboff = 466 },    // OnTrack DM
		{ .magic = "\x51", .len = 1, .sboff = 466 },    // OnTrack DM6 Aux
		{ .magic = "\x52", .len = 1, .sboff = 466 },    // CP/M
		{ .magic = "\x53", .len = 1, .sboff = 466 },    // OnTrack DM6 Aux
		{ .magic = "\x54", .len = 1, .sboff = 466 },    // OnTrackDM6
		{ .magic = "\x55", .len = 1, .sboff = 466 },    // EZ-Drive
		{ .magic = "\x56", .len = 1, .sboff = 466 },    // Golden Bow
		{ .magic = "\x5c", .len = 1, .sboff = 466 },    // Priam Edisk
		{ .magic = "\x61", .len = 1, .sboff = 466 },    // SpeedStor
		{ .magic = "\x63", .len = 1, .sboff = 466 },    // GNU HURD or Sys
		{ .magic = "\x64", .len = 1, .sboff = 466 },    // Novell Netware
		{ .magic = "\x65", .len = 1, .sboff = 466 },    // Novell Netware
		{ .magic = "\x70", .len = 1, .sboff = 466 },    // DiskSecure Multiboot
		{ .magic = "\x75", .len = 1, .sboff = 466 },    // PC/IX
		{ .magic = "\x80", .len = 1, .sboff = 466 },    // Old Minix
		{ .magic = "\x81", .len = 1, .sboff = 466 },    // Minix / old Linux
		{ .magic = "\x82", .len = 1, .sboff = 466 },    // Linux swap / Solaris
		{ .magic = "\x83", .len = 1, .sboff = 466 },    // Linux
		{ .magic = "\x84", .len = 1, .sboff = 466 },    // OS/2 hidden C:
		{ .magic = "\x85", .len = 1, .sboff = 466 },    // Linux extended
		{ .magic = "\x86", .len = 1, .sboff = 466 },    // NTFS volume set
		{ .magic = "\x87", .len = 1, .sboff = 466 },    // NTFS volume set
		{ .magic = "\x88", .len = 1, .sboff = 466 },    // Linux plaintext
		{ .magic = "\x8e", .len = 1, .sboff = 466 },    // Linux LVM
		{ .magic = "\x93", .len = 1, .sboff = 466 },    // Amoeba
		{ .magic = "\x94", .len = 1, .sboff = 466 },    // Amoeba BBT
		{ .magic = "\x9f", .len = 1, .sboff = 466 },    // BSD/OS
		{ .magic = "\xa0", .len = 1, .sboff = 466 },    // IBM Thinkpad hibernation
		{ .magic = "\xa5", .len = 1, .sboff = 466 },    // FreeBSD
		{ .magic = "\xa6", .len = 1, .sboff = 466 },    // OpenBSD
		{ .magic = "\xa7", .len = 1, .sboff = 466 },    // NeXTSTEP
		{ .magic = "\xa8", .len = 1, .sboff = 466 },    // Darwin UFS
		{ .magic = "\xa9", .len = 1, .sboff = 466 },    // NetBSD
		{ .magic = "\xab", .len = 1, .sboff = 466 },    // Darwin boot
		{ .magic = "\xaf", .len = 1, .sboff = 466 },    // HFS / HFS+
		{ .magic = "\xb7", .len = 1, .sboff = 466 },    // BSDI fs
		{ .magic = "\xb8", .len = 1, .sboff = 466 },    // BSDI swap
		{ .magic = "\xbb", .len = 1, .sboff = 466 },    // Boot Wizard hid
		{ .magic = "\xbe", .len = 1, .sboff = 466 },    // Solaris boot
		{ .magic = "\xbf", .len = 1, .sboff = 466 },    // Solaris
		{ .magic = "\xc1", .len = 1, .sboff = 466 },    // DRDOS/sec (FAT-12)
		{ .magic = "\xc4", .len = 1, .sboff = 466 },    // DRDOS/sec (FAT-16)
		{ .magic = "\xc6", .len = 1, .sboff = 466 },    // DRDOS/sec (FAT-16B)
		{ .magic = "\xc7", .len = 1, .sboff = 466 },    // Syrinx
		{ .magic = "\xda", .len = 1, .sboff = 466 },    // Non-FS data
		{ .magic = "\xdb", .len = 1, .sboff = 466 },    // CP/M / CTOS / .
		{ .magic = "\xde", .len = 1, .sboff = 466 },    // Dell Utility
		{ .magic = "\xdf", .len = 1, .sboff = 466 },    // BootIt
		{ .magic = "\xe1", .len = 1, .sboff = 466 },    // DOS access
		{ .magic = "\xe3", .len = 1, .sboff = 466 },    // DOS R/O
		{ .magic = "\xe4", .len = 1, .sboff = 466 },    // SpeedStor
		{ .magic = "\xeb", .len = 1, .sboff = 466 },    // BeOS fs
		{ .magic = "\xee", .len = 1, .sboff = 466 },    // GPT
		{ .magic = "\xef", .len = 1, .sboff = 466 },    // EFI (FAT-12/16/32)
		{ .magic = "\xf0", .len = 1, .sboff = 466 },    // Linux/PA-RISC boot loader
		{ .magic = "\xf1", .len = 1, .sboff = 466 },    // SpeedStor
		{ .magic = "\xf4", .len = 1, .sboff = 466 },    // SpeedStor
		{ .magic = "\xf2", .len = 1, .sboff = 466 },    // DOS secondary
		{ .magic = "\xfb", .len = 1, .sboff = 466 },    // VMware VMFS
		{ .magic = "\xfc", .len = 1, .sboff = 466 },    // VMware VMKCORE
		{ .magic = "\xfd", .len = 1, .sboff = 466 },    // Linux raid auto
		{ .magic = "\xfe", .len = 1, .sboff = 466 },    // LANstep
		{ .magic = "\xff", .len = 1, .sboff = 466 },    // BBT
		//{ .magic = "\x00", .len = 1, .sboff = 482 },    // EMPTY
		{ .magic = "\x01", .len = 1, .sboff = 482 },    // FAT12
		{ .magic = "\x02", .len = 1, .sboff = 482 },    // XENIX root
		{ .magic = "\x03", .len = 1, .sboff = 482 },    // XENIX usr
		{ .magic = "\x04", .len = 1, .sboff = 482 },    // FAT16 <32M
		{ .magic = "\x05", .len = 1, .sboff = 482 },    // Extended
		{ .magic = "\x06", .len = 1, .sboff = 482 },    // FAT16
		{ .magic = "\x07", .len = 1, .sboff = 482 },    // HPFS/NTFS/exFAT
		{ .magic = "\x08", .len = 1, .sboff = 482 },    // AIX
		{ .magic = "\x09", .len = 1, .sboff = 482 },    // AIX bootable
		{ .magic = "\x0a", .len = 1, .sboff = 482 },    // OS/2 Boot Manager
		{ .magic = "\x0b", .len = 1, .sboff = 482 },    // W95 FAT32
		{ .magic = "\x0c", .len = 1, .sboff = 482 },    // W95 FAT32 (LBA)
		{ .magic = "\x0e", .len = 1, .sboff = 482 },    // W95 FAT16 (LBA)
		{ .magic = "\x0f", .len = 1, .sboff = 482 },    // W95 Ext'd (LBA)
		{ .magic = "\x10", .len = 1, .sboff = 482 },    // OPUS
		{ .magic = "\x11", .len = 1, .sboff = 482 },    // Hidden FAT12
		{ .magic = "\x12", .len = 1, .sboff = 482 },    // Compaq diagnostics
		{ .magic = "\x14", .len = 1, .sboff = 482 },    // Hidden FAT16 <3
		{ .magic = "\x16", .len = 1, .sboff = 482 },    // Hidden FAT16
		{ .magic = "\x17", .len = 1, .sboff = 482 },    // Hidden IFS/HPFS/NTFS/exFAT
		{ .magic = "\x18", .len = 1, .sboff = 482 },    // AST SmartSleep
		{ .magic = "\x1b", .len = 1, .sboff = 482 },    // Hidden W95 FAT3
		{ .magic = "\x1c", .len = 1, .sboff = 482 },    // Hidden W95 FAT3
		{ .magic = "\x1e", .len = 1, .sboff = 482 },    // Hidden W95 FAT1
		{ .magic = "\x24", .len = 1, .sboff = 482 },    // NEC DOS
		{ .magic = "\x27", .len = 1, .sboff = 482 },    // Hidden NTFS Win
		{ .magic = "\x39", .len = 1, .sboff = 482 },    // Plan 9
		{ .magic = "\x3c", .len = 1, .sboff = 482 },    // PartitionMagic
		{ .magic = "\x40", .len = 1, .sboff = 482 },    // Venix 80286
		{ .magic = "\x41", .len = 1, .sboff = 482 },    // PPC PReP Boot
		{ .magic = "\x42", .len = 1, .sboff = 482 },    // SFS
		{ .magic = "\x4d", .len = 1, .sboff = 482 },    // QNX4.x
		{ .magic = "\x4e", .len = 1, .sboff = 482 },    // QNX4.x 2nd part
		{ .magic = "\x4f", .len = 1, .sboff = 482 },    // QNX4.x 3rd part
		{ .magic = "\x50", .len = 1, .sboff = 482 },    // OnTrack DM
		{ .magic = "\x51", .len = 1, .sboff = 482 },    // OnTrack DM6 Aux
		{ .magic = "\x52", .len = 1, .sboff = 482 },    // CP/M
		{ .magic = "\x53", .len = 1, .sboff = 482 },    // OnTrack DM6 Aux
		{ .magic = "\x54", .len = 1, .sboff = 482 },    // OnTrackDM6
		{ .magic = "\x55", .len = 1, .sboff = 482 },    // EZ-Drive
		{ .magic = "\x56", .len = 1, .sboff = 482 },    // Golden Bow
		{ .magic = "\x5c", .len = 1, .sboff = 482 },    // Priam Edisk
		{ .magic = "\x61", .len = 1, .sboff = 482 },    // SpeedStor
		{ .magic = "\x63", .len = 1, .sboff = 482 },    // GNU HURD or Sys
		{ .magic = "\x64", .len = 1, .sboff = 482 },    // Novell Netware
		{ .magic = "\x65", .len = 1, .sboff = 482 },    // Novell Netware
		{ .magic = "\x70", .len = 1, .sboff = 482 },    // DiskSecure Multiboot
		{ .magic = "\x75", .len = 1, .sboff = 482 },    // PC/IX
		{ .magic = "\x80", .len = 1, .sboff = 482 },    // Old Minix
		{ .magic = "\x81", .len = 1, .sboff = 482 },    // Minix / old Linux
		{ .magic = "\x82", .len = 1, .sboff = 482 },    // Linux swap / Solaris
		{ .magic = "\x83", .len = 1, .sboff = 482 },    // Linux
		{ .magic = "\x84", .len = 1, .sboff = 482 },    // OS/2 hidden C:
		{ .magic = "\x85", .len = 1, .sboff = 482 },    // Linux extended
		{ .magic = "\x86", .len = 1, .sboff = 482 },    // NTFS volume set
		{ .magic = "\x87", .len = 1, .sboff = 482 },    // NTFS volume set
		{ .magic = "\x88", .len = 1, .sboff = 482 },    // Linux plaintext
		{ .magic = "\x8e", .len = 1, .sboff = 482 },    // Linux LVM
		{ .magic = "\x93", .len = 1, .sboff = 482 },    // Amoeba
		{ .magic = "\x94", .len = 1, .sboff = 482 },    // Amoeba BBT
		{ .magic = "\x9f", .len = 1, .sboff = 482 },    // BSD/OS
		{ .magic = "\xa0", .len = 1, .sboff = 482 },    // IBM Thinkpad hibernation
		{ .magic = "\xa5", .len = 1, .sboff = 482 },    // FreeBSD
		{ .magic = "\xa6", .len = 1, .sboff = 482 },    // OpenBSD
		{ .magic = "\xa7", .len = 1, .sboff = 482 },    // NeXTSTEP
		{ .magic = "\xa8", .len = 1, .sboff = 482 },    // Darwin UFS
		{ .magic = "\xa9", .len = 1, .sboff = 482 },    // NetBSD
		{ .magic = "\xab", .len = 1, .sboff = 482 },    // Darwin boot
		{ .magic = "\xaf", .len = 1, .sboff = 482 },    // HFS / HFS+
		{ .magic = "\xb7", .len = 1, .sboff = 482 },    // BSDI fs
		{ .magic = "\xb8", .len = 1, .sboff = 482 },    // BSDI swap
		{ .magic = "\xbb", .len = 1, .sboff = 482 },    // Boot Wizard hid
		{ .magic = "\xbe", .len = 1, .sboff = 482 },    // Solaris boot
		{ .magic = "\xbf", .len = 1, .sboff = 482 },    // Solaris
		{ .magic = "\xc1", .len = 1, .sboff = 482 },    // DRDOS/sec (FAT-12)
		{ .magic = "\xc4", .len = 1, .sboff = 482 },    // DRDOS/sec (FAT-16)
		{ .magic = "\xc6", .len = 1, .sboff = 482 },    // DRDOS/sec (FAT-16B)
		{ .magic = "\xc7", .len = 1, .sboff = 482 },    // Syrinx
		{ .magic = "\xda", .len = 1, .sboff = 482 },    // Non-FS data
		{ .magic = "\xdb", .len = 1, .sboff = 482 },    // CP/M / CTOS / .
		{ .magic = "\xde", .len = 1, .sboff = 482 },    // Dell Utility
		{ .magic = "\xdf", .len = 1, .sboff = 482 },    // BootIt
		{ .magic = "\xe1", .len = 1, .sboff = 482 },    // DOS access
		{ .magic = "\xe3", .len = 1, .sboff = 482 },    // DOS R/O
		{ .magic = "\xe4", .len = 1, .sboff = 482 },    // SpeedStor
		{ .magic = "\xeb", .len = 1, .sboff = 482 },    // BeOS fs
		{ .magic = "\xee", .len = 1, .sboff = 482 },    // GPT
		{ .magic = "\xef", .len = 1, .sboff = 482 },    // EFI (FAT-12/16/32)
		{ .magic = "\xf0", .len = 1, .sboff = 482 },    // Linux/PA-RISC boot loader
		{ .magic = "\xf1", .len = 1, .sboff = 482 },    // SpeedStor
		{ .magic = "\xf4", .len = 1, .sboff = 482 },    // SpeedStor
		{ .magic = "\xf2", .len = 1, .sboff = 482 },    // DOS secondary
		{ .magic = "\xfb", .len = 1, .sboff = 482 },    // VMware VMFS
		{ .magic = "\xfc", .len = 1, .sboff = 482 },    // VMware VMKCORE
		{ .magic = "\xfd", .len = 1, .sboff = 482 },    // Linux raid auto
		{ .magic = "\xfe", .len = 1, .sboff = 482 },    // LANstep
		{ .magic = "\xff", .len = 1, .sboff = 482 },    // BBT
		//{ .magic = "\x00", .len = 1, .sboff = 498 },    // EMPTY
		{ .magic = "\x01", .len = 1, .sboff = 498 },    // FAT12
		{ .magic = "\x02", .len = 1, .sboff = 498 },    // XENIX root
		{ .magic = "\x03", .len = 1, .sboff = 498 },    // XENIX usr
		{ .magic = "\x04", .len = 1, .sboff = 498 },    // FAT16 <32M
		{ .magic = "\x05", .len = 1, .sboff = 498 },    // Extended
		{ .magic = "\x06", .len = 1, .sboff = 498 },    // FAT16
		{ .magic = "\x07", .len = 1, .sboff = 498 },    // HPFS/NTFS/exFAT
		{ .magic = "\x08", .len = 1, .sboff = 498 },    // AIX
		{ .magic = "\x09", .len = 1, .sboff = 498 },    // AIX bootable
		{ .magic = "\x0a", .len = 1, .sboff = 498 },    // OS/2 Boot Manager
		{ .magic = "\x0b", .len = 1, .sboff = 498 },    // W95 FAT32
		{ .magic = "\x0c", .len = 1, .sboff = 498 },    // W95 FAT32 (LBA)
		{ .magic = "\x0e", .len = 1, .sboff = 498 },    // W95 FAT16 (LBA)
		{ .magic = "\x0f", .len = 1, .sboff = 498 },    // W95 Ext'd (LBA)
		{ .magic = "\x10", .len = 1, .sboff = 498 },    // OPUS
		{ .magic = "\x11", .len = 1, .sboff = 498 },    // Hidden FAT12
		{ .magic = "\x12", .len = 1, .sboff = 498 },    // Compaq diagnostics
		{ .magic = "\x14", .len = 1, .sboff = 498 },    // Hidden FAT16 <3
		{ .magic = "\x16", .len = 1, .sboff = 498 },    // Hidden FAT16
		{ .magic = "\x17", .len = 1, .sboff = 498 },    // Hidden IFS/HPFS/NTFS/exFAT
		{ .magic = "\x18", .len = 1, .sboff = 498 },    // AST SmartSleep
		{ .magic = "\x1b", .len = 1, .sboff = 498 },    // Hidden W95 FAT3
		{ .magic = "\x1c", .len = 1, .sboff = 498 },    // Hidden W95 FAT3
		{ .magic = "\x1e", .len = 1, .sboff = 498 },    // Hidden W95 FAT1
		{ .magic = "\x24", .len = 1, .sboff = 498 },    // NEC DOS
		{ .magic = "\x27", .len = 1, .sboff = 498 },    // Hidden NTFS Win
		{ .magic = "\x39", .len = 1, .sboff = 498 },    // Plan 9
		{ .magic = "\x3c", .len = 1, .sboff = 498 },    // PartitionMagic
		{ .magic = "\x40", .len = 1, .sboff = 498 },    // Venix 80286
		{ .magic = "\x41", .len = 1, .sboff = 498 },    // PPC PReP Boot
		{ .magic = "\x42", .len = 1, .sboff = 498 },    // SFS
		{ .magic = "\x4d", .len = 1, .sboff = 498 },    // QNX4.x
		{ .magic = "\x4e", .len = 1, .sboff = 498 },    // QNX4.x 2nd part
		{ .magic = "\x4f", .len = 1, .sboff = 498 },    // QNX4.x 3rd part
		{ .magic = "\x50", .len = 1, .sboff = 498 },    // OnTrack DM
		{ .magic = "\x51", .len = 1, .sboff = 498 },    // OnTrack DM6 Aux
		{ .magic = "\x52", .len = 1, .sboff = 498 },    // CP/M
		{ .magic = "\x53", .len = 1, .sboff = 498 },    // OnTrack DM6 Aux
		{ .magic = "\x54", .len = 1, .sboff = 498 },    // OnTrackDM6
		{ .magic = "\x55", .len = 1, .sboff = 498 },    // EZ-Drive
		{ .magic = "\x56", .len = 1, .sboff = 498 },    // Golden Bow
		{ .magic = "\x5c", .len = 1, .sboff = 498 },    // Priam Edisk
		{ .magic = "\x61", .len = 1, .sboff = 498 },    // SpeedStor
		{ .magic = "\x63", .len = 1, .sboff = 498 },    // GNU HURD or Sys
		{ .magic = "\x64", .len = 1, .sboff = 498 },    // Novell Netware
		{ .magic = "\x65", .len = 1, .sboff = 498 },    // Novell Netware
		{ .magic = "\x70", .len = 1, .sboff = 498 },    // DiskSecure Multiboot
		{ .magic = "\x75", .len = 1, .sboff = 498 },    // PC/IX
		{ .magic = "\x80", .len = 1, .sboff = 498 },    // Old Minix
		{ .magic = "\x81", .len = 1, .sboff = 498 },    // Minix / old Linux
		{ .magic = "\x82", .len = 1, .sboff = 498 },    // Linux swap / Solaris
		{ .magic = "\x83", .len = 1, .sboff = 498 },    // Linux
		{ .magic = "\x84", .len = 1, .sboff = 498 },    // OS/2 hidden C:
		{ .magic = "\x85", .len = 1, .sboff = 498 },    // Linux extended
		{ .magic = "\x86", .len = 1, .sboff = 498 },    // NTFS volume set
		{ .magic = "\x87", .len = 1, .sboff = 498 },    // NTFS volume set
		{ .magic = "\x88", .len = 1, .sboff = 498 },    // Linux plaintext
		{ .magic = "\x8e", .len = 1, .sboff = 498 },    // Linux LVM
		{ .magic = "\x93", .len = 1, .sboff = 498 },    // Amoeba
		{ .magic = "\x94", .len = 1, .sboff = 498 },    // Amoeba BBT
		{ .magic = "\x9f", .len = 1, .sboff = 498 },    // BSD/OS
		{ .magic = "\xa0", .len = 1, .sboff = 498 },    // IBM Thinkpad hibernation
		{ .magic = "\xa5", .len = 1, .sboff = 498 },    // FreeBSD
		{ .magic = "\xa6", .len = 1, .sboff = 498 },    // OpenBSD
		{ .magic = "\xa7", .len = 1, .sboff = 498 },    // NeXTSTEP
		{ .magic = "\xa8", .len = 1, .sboff = 498 },    // Darwin UFS
		{ .magic = "\xa9", .len = 1, .sboff = 498 },    // NetBSD
		{ .magic = "\xab", .len = 1, .sboff = 498 },    // Darwin boot
		{ .magic = "\xaf", .len = 1, .sboff = 498 },    // HFS / HFS+
		{ .magic = "\xb7", .len = 1, .sboff = 498 },    // BSDI fs
		{ .magic = "\xb8", .len = 1, .sboff = 498 },    // BSDI swap
		{ .magic = "\xbb", .len = 1, .sboff = 498 },    // Boot Wizard hid
		{ .magic = "\xbe", .len = 1, .sboff = 498 },    // Solaris boot
		{ .magic = "\xbf", .len = 1, .sboff = 498 },    // Solaris
		{ .magic = "\xc1", .len = 1, .sboff = 498 },    // DRDOS/sec (FAT-12)
		{ .magic = "\xc4", .len = 1, .sboff = 498 },    // DRDOS/sec (FAT-16)
		{ .magic = "\xc6", .len = 1, .sboff = 498 },    // DRDOS/sec (FAT-16B)
		{ .magic = "\xc7", .len = 1, .sboff = 498 },    // Syrinx
		{ .magic = "\xda", .len = 1, .sboff = 498 },    // Non-FS data
		{ .magic = "\xdb", .len = 1, .sboff = 498 },    // CP/M / CTOS / .
		{ .magic = "\xde", .len = 1, .sboff = 498 },    // Dell Utility
		{ .magic = "\xdf", .len = 1, .sboff = 498 },    // BootIt
		{ .magic = "\xe1", .len = 1, .sboff = 498 },    // DOS access
		{ .magic = "\xe3", .len = 1, .sboff = 498 },    // DOS R/O
		{ .magic = "\xe4", .len = 1, .sboff = 498 },    // SpeedStor
		{ .magic = "\xeb", .len = 1, .sboff = 498 },    // BeOS fs
		{ .magic = "\xee", .len = 1, .sboff = 498 },    // GPT
		{ .magic = "\xef", .len = 1, .sboff = 498 },    // EFI (FAT-12/16/32)
		{ .magic = "\xf0", .len = 1, .sboff = 498 },    // Linux/PA-RISC boot loader
		{ .magic = "\xf1", .len = 1, .sboff = 498 },    // SpeedStor
		{ .magic = "\xf4", .len = 1, .sboff = 498 },    // SpeedStor
		{ .magic = "\xf2", .len = 1, .sboff = 498 },    // DOS secondary
		{ .magic = "\xfb", .len = 1, .sboff = 498 },    // VMware VMFS
		{ .magic = "\xfc", .len = 1, .sboff = 498 },    // VMware VMKCORE
		{ .magic = "\xfd", .len = 1, .sboff = 498 },    // Linux raid auto
		{ .magic = "\xfe", .len = 1, .sboff = 498 },    // LANstep
		{ .magic = "\xff", .len = 1, .sboff = 498 },    // BBT
		{ .magic = "\x55\xAA", .len = 2, .sboff = 510 },// Boot Signature
		{ NULL }
	}
};

