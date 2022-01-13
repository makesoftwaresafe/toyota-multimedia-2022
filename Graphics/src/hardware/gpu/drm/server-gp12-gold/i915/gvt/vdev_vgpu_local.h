/*
 * $QNXLicenseC:
 * Copyright 2015, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable license fees to QNX
 * Software Systems before you may reproduce, modify or distribute this software,
 * or any work that includes all or part of this software.   Free development
 * licenses are available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review this entire
 * file for other proprietary rights or license notices, as well as the QNX
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/
 * for other information.
 * $
 */

/**
 * @file
 * QNX GVT-g Virtual Device API definitions
 *
 */
#ifndef __VDEV_VGPU_H_INCLUDED
#define __VDEV_VGPU_H_INCLUDED

#define QVGPU_VERSION_MAJOR	1u
#define QVGPU_VERSION_MINOR	1u

/** Maximum size of data for emulation by Mediator in Bytes. */
#define QVGPU_IOREQ_MAX_DATA_SIZE 8u

/**
 * Flags for qvgpu_ioreq_msg::dir
 *
 * Represents the direction of IO operation.
 */
enum qvgpu_ioreq_dir_flags {
	QVGPU_IOREQ_DIR_WRITE,
	QVGPU_IOREQ_DIR_READ,
};

/**
 * Flags for qvgpu_ioreq_msg::type
 *
 * Represents type of IO operation.
 */
enum qvgpu_ioreq_types {
	QVGPU_IOREQ_TYPE_PCI,
	QVGPU_IOREQ_TYPE_MEM,
};

/**
 * Layout of the IO emulation message to GVT-g Mediator.
 */
struct qvgpu_ioreq_msg {
	/** Address of the data provided/requested in the IO emulation request. */
	_Uint64t		address;
	/** Length of the data provided/requested in the IO emulation request. */
	_Uint32t		length;
	/** A value from ::qvgpu_ioreq_types. */
	_Uint16t		type;
	/** A value from ::qvgpu_ioreq_dir_flags. */
	_Uint16t		dir;
};
#define QVGPU_PCI_OFFSET(address)          (((unsigned)(address) >> 32) & 0xffffu)

/**
 * Message/command types supported by vGPU VDEV.
 */
enum qvgpu_msg_types {
	/** See ::qvgpu_version */
	QVGPU_VERSION,
	/** See ::qvgpu_ioreq_server_create */
	QVGPU_IOREQ_SERVER_CREATE,
	/** See ::qvgpu_ioreq_server_destroy */
	QVGPU_IOREQ_SERVER_DESTROY,
	/** See ::qvgpu_sysmem_get_info */
	QVGPU_SYSMEM_GET_INFO,
	/** See ::qvgpu_sysmem_set */
	QVGPU_SYSMEM_SET,
	/** See ::qvgpu_msi_inject */
	QVGPU_MSI_INJECT,
};

/**
 * Layout of the qvgpu_msg_types::QVGPU_VERSION message.
 */
struct qvgpu_version {
	/** Must be qvgpu_msg_types::QVGT_VERSION */
	_Uint32t		type;
	_Uint32t		zero[1];
};

/**
 * Layout of the qvgpu_msg_types::QVGPU_VERSION reply.
 */
struct qvgpu_version_reply {
	struct {
		_Uint32t	major;	/**< QVGPU_VERSION_MAJOR */
		_Uint32t	minor;	/**< QVGPU_VERSION_MINOR */
	}				version;
};

/**
 * Layout of the qvgpu_msg_types::QVGT_IOREQ_SERVER_CREATE message.
 */
struct qvgpu_ioreq_server_create {
	/** Must be qvgpu_msg_types::QVGT_IOREQ_SERVER_CREATE */
	_Uint32t		type;
	pid_t			pid;		/**<  */
	_Int32t			chid;		/**<  */
	_Uint32t		zero[1];
	_Uint64t		pci_loc;	/**<  */
};

/**
 * Encode qvgpu_ioreq_server_create::pci_loc
 */
#define QVGPU_PCI_LOC(bus, dev, func)   (((bus) << 16) | ((dev) << 8) | ((func) << 0))
#define QVGPU_PCI_LOC_BUS(loc)          (((unsigned)(loc) >> 16) & 0xffffu)
#define QVGPU_PCI_LOC_DEV(loc)          (((unsigned)(loc) >> 8)  & 0x00ffu)
#define QVGPU_PCI_LOC_FUNC(loc)         (((unsigned)(loc) >> 0)  & 0x00ffu)

/**
 * Layout of the qvgpu_msg_types::QVGT_IOREQ_SERVER_CREATE message.
 */
struct qvgpu_ioreq_server_destroy {
	/** Must be qvgpu_msg_types::QVGT_IOREQ_SERVER_DESTROY */
	_Uint32t		type;
	_Uint32t		zero[1];
};

/**
 * Information about one system memory block.
 */
struct qvgpu_sysmem_blk {
	_Uint64t		location;	/**<  guest physical address */
	_Uint32t		length;		/**<  length of system memory block */
	_Uint32t		host_pgnum;	/**<  host paddr page number */
};

/**
 * Layout of the qvgpu_msg_types::QVGPU_SYSMEM_GET_INFO message.
 */
struct qvgpu_sysmem_get_info {
	/** Must be qvgpu_msg_types::QVGPU_SYSMEM_GET_INFO */
	_Uint32t		type;
	_Uint16t		num_mem_blks;	/**< number of memory blocks */
	_Uint16t		zero[1];
};

/**
 * Layout of the qvgpu_msg_types::QVGPU_SYSMEM_GET_INFO reply.
 */
struct qvgpu_sysmem_get_info_reply {
	int				memory_fd;		/**<  */
	pid_t			pid;			/**<  */
	_Uint16t		num_mem_blks;	/**<  number of RAM regions in the message */
	_Uint16t		zero[3];

	struct qvgpu_sysmem_blk	mem_blk[];	/**<  */
};

/**
 * Flags for the qvgpu_sysmem_set message.
 */
enum qvgpu_sysmem_flags {
	QVGPU_SYSMEM_NONE		= 0x00000000,
	/** A read access is allowed through with no vmexit. */
	QVGPU_SYSMEM_PASS_RD	= 0x00000001,
	/** A write access is allowed through with no vmexit. */
	QVGPU_SYSMEM_PASS_WR	= 0x00000002,
	/** Passthru mappings should be marked as no-cache */
	QVGPU_SYSMEM_NOCACHE	= 0x00000004,
	/** The region should already exist */
	QVGPU_SYSMEM_MODIFY     = 0x00000008,
	/** A read access gets trapped. */
	QVGPU_SYSMEM_TRAP_RD    = 0x00000010,
	/** A write access gets trapped. */
	QVGPU_SYSMEM_TRAP_WR    = 0x00000020,
};

/**
 * Layout of the qvgpu_msg_types::QVGPU_SYSMEM_SET message.
 */
struct qvgpu_sysmem_set {
	/** Must be qvgpu_msg_types::QVGPU_SYSMEM_SET */
	_Uint32t		type;
	/** A bitset of values from ::qvgpu_sysmem_flags. */
	_Uint32t		flags;
	_Uint64t		host_location;	/**< host physical address */
	_Uint64t		guest_location;	/**< guest physical address */
	_Uint32t		length;			/**< length of modifying region */
	_Uint32t		zero[1];
};

/**
 * Layout of the qvgpu_msg_types::QVGPU_MSI_INJECT message.
 */
struct qvgpu_msi_inject {
	/** Must be qvgpu_msg_types::QVGPU_MSI_INJECT */
	_Uint32t		type;
	_Uint32t		data;	/**<  */
	_Uint64t		addr;	/**<  */
};

/**
 * Union of possible message layouts for GVT-g Mediator <-> vGPU VDEV interface.
 */
union qvgpu_op {
	struct qvgpu_version				version;
	struct qvgpu_ioreq_server_create	ioreq_server_create;
	struct qvgpu_ioreq_server_destroy	ioreq_server_destroy;
	struct qvgpu_sysmem_get_info		sysmem_get_info;
	struct qvgpu_sysmem_set				sysmem_set;
	struct qvgpu_msi_inject				msi_inject;
};
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/gvt/vdev_vgpu_local.h $ $Rev: 840286 $")
#endif
