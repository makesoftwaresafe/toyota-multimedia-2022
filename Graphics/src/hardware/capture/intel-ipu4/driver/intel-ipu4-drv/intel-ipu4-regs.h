/*
 * Copyright (c) 2014--2016 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef INTEL_IPU4_REGS_H
#define INTEL_IPU4_REGS_H

#define INTEL_IPU4_MRFLD_DATA_IOMMU_OFFSET		0x00070000
#define INTEL_IPU4_MRFLD_ICACHE_IOMMU_OFFSET		0x000a0000

/* Brxoton IPU4 B0 offsets */
#define INTEL_IPU4_BXT_B0_ISYS_IOMMU0_OFFSET		0x000e0000
#define INTEL_IPU4_BXT_B0_ISYS_IOMMU1_OFFSET		0x000e0100

#define INTEL_IPU4_BXT_B0_ISYS_OFFSET			0x00100000
#define INTEL_IPU4_BXT_B0_PSYS_OFFSET			0x00400000

#define INTEL_IPU4_BXT_B0_PSYS_IOMMU0_OFFSET		0x000b0000
#define INTEL_IPU4_BXT_B0_PSYS_IOMMU1_OFFSET		0x000b0100
#define INTEL_IPU4_BXT_B0_PSYS_IOMMU1R_OFFSET		0x000b0600

#define INTEL_IPU4_BXT_B0_TPG0_ADDR_OFFSET		0x64800
#define INTEL_IPU4_BXT_B0_TPG1_ADDR_OFFSET		0x6f400
#define INTEL_IPU4_BXT_B0_CSI2BE_ADDR_OFFSET		0xba000

/* Common for both A0 and B0 */
#define INTEL_IPU4_BXT_PSYS_MMU0_CTRL_OFFSET		0x08

#define INTEL_IPU4_BXT_GPOFFSET				0x67800
#define INTEL_IPU4_BXT_COMBO_GPOFFSET			0x6f000

#define INTEL_IPU4_ISYS_SPC_OFFSET			0x000000
#define INTEL_IPU4_PSYS_SPC_OFFSET			0x000000
#define INTEL_IPU4_ISYS_DMEM_OFFSET			0x008000
#define INTEL_IPU4_PSYS_DMEM_OFFSET			0x008000

/* PKG DIR OFFSET in IMR in secure mode */
#define INTEL_IPU4_PKG_DIR_IMR_OFFSET			0x40

#define INTEL_IPU4_GPREG_MIPI_PKT_GEN0_SEL		0x24
#define INTEL_IPU4_GPREG_MIPI_PKT_GEN1_SEL		0x1c

/* Broxton PCI config registers */
#define INTEL_IPU4_REG_PCI_PCIECAPHDR_PCIECAP		0x70
#define INTEL_IPU4_REG_PCI_DEVICECAP			0x74
#define INTEL_IPU4_REG_PCI_DEVICECTL_DEVICESTS		0x78
#define INTEL_IPU4_REG_PCI_MSI_CAPID			0xac
#define INTEL_IPU4_REG_PCI_MSI_ADDRESS_LO			0xb0
#define INTEL_IPU4_REG_PCI_MSI_ADDRESS_HI			0xb4
#define INTEL_IPU4_REG_PCI_MSI_DATA			0xb8
#define INTEL_IPU4_REG_PCI_PMCAP				0xd0
#define INTEL_IPU4_REG_PCI_PMCS				0xd4
#define INTEL_IPU4_REG_PCI_MANUFACTURING_ID		0xf8
#define INTEL_IPU4_REG_PCI_IUNIT_ACCESS_CTRL_VIOL		0xfc

/* Broxton ISYS registers */
/* Isys DMA CIO info register */
#define INTEL_IPU4_REG_ISYS_INFO_CIO_DMA0(a)		(0x81810 + (a) * 0x40)
#define INTEL_IPU4_REG_ISYS_INFO_CIO_DMA1(a)		(0x93010 + (a) * 0x40)
#define INTEL_IPU4_REG_ISYS_INFO_CIO_DMA_IS(a)		(0xb0610 + (a) * 0x40)
#define INTEL_IPU4_ISYS_NUM_OF_DMA0_CHANNELS		16
#define INTEL_IPU4_ISYS_NUM_OF_DMA1_CHANNELS		32
#define INTEL_IPU4_ISYS_NUM_OF_IS_CHANNELS			4
/*Isys Info register offsets*/
#define INTEL_IPU4_REG_ISYS_INFO_SEG_0_CONFIG_ICACHE_MASTER	0x14
#define INTEL_IPU4_REG_ISYS_INFO_SEG_CMEM_MASTER(a)	(0x2C + (a * 12))
#define INTEL_IPU4_REG_ISYS_INFO_SEG_XMEM_MASTER(a)	(0x5C + (a * 12))

/* IRQ-related registers relative to ISYS_OFFSET */
#define INTEL_IPU4_REG_ISYS_UNISPART_IRQ_EDGE		0x7c000
#define INTEL_IPU4_REG_ISYS_UNISPART_IRQ_MASK		0x7c004
#define INTEL_IPU4_REG_ISYS_UNISPART_IRQ_STATUS		0x7c008
#define INTEL_IPU4_REG_ISYS_UNISPART_IRQ_CLEAR		0x7c00c
#define INTEL_IPU4_REG_ISYS_UNISPART_IRQ_ENABLE		0x7c010
#define INTEL_IPU4_REG_ISYS_UNISPART_IRQ_LEVEL_NOT_PULSE 0x7c014
/* CDC Burst collector thresholds for isys - 3 FIFOs i = 0..2 */
#define INTEL_IPU4_REG_ISYS_CDC_THRESHOLD(i)		(0x7c400 + ((i) * 4))
#define INTEL_IPU4_REG_ISYS_UNISPART_SW_IRQ_REG		0x7c414
#define INTEL_IPU4_REG_ISYS_UNISPART_SW_IRQ_MUX_REG	0x7c418

#define INTEL_IPU4_ISYS_UNISPART_IRQ_CSI2_A0(bus)	(0x10 << (bus))

#define INTEL_IPU4_ISYS_UNISPART_IRQ_CSI2_B0(bus)		\
	((bus) < INTEL_IPU4_ISYS_MAX_CSI2_LEGACY_PORTS ?	\
	((0x8) << (bus)) :					\
	(0x800 << ((bus) - INTEL_IPU4_ISYS_MAX_CSI2_LEGACY_PORTS)))

#define INTEL_IPU4_ISYS_UNISPART_IRQ_SW			(1 << 30)

/*Iunit Info bits*/
#define INTEL_IPU4_REG_PSYS_INFO_SEG_CMEM_MASTER(a)	(0x2C + ((a) * 12))
#define INTEL_IPU4_REG_PSYS_INFO_SEG_XMEM_MASTER(a)	(0x5C + ((a) * 12))
#define INTEL_IPU4_REG_PSYS_INFO_SEG_DATA_MASTER(a)	(0x8C + ((a) * 12))

#define INTEL_IPU4_ISYS_REG_SPC_STATUS_CTRL			0x0

#define INTEL_IPU4_ISYS_SPC_STATUS_START			(1 << 1)
#define INTEL_IPU4_ISYS_SPC_STATUS_RUN				(1 << 3)
#define INTEL_IPU4_ISYS_SPC_STATUS_READY			(1 << 5)
#define INTEL_IPU4_ISYS_SPC_STATUS_CTRL_ICACHE_INVALIDATE	(1 << 12)
#define INTEL_IPU4_ISYS_SPC_STATUS_ICACHE_PREFETCH		(1 << 13)

#define INTEL_IPU4_PSYS_REG_SPC_STATUS_CTRL			0x0

#define INTEL_IPU4_PSYS_SPC_STATUS_START			(1 << 1)
#define INTEL_IPU4_PSYS_SPC_STATUS_RUN				(1 << 3)
#define INTEL_IPU4_PSYS_SPC_STATUS_READY			(1 << 5)
#define INTEL_IPU4_PSYS_SPC_STATUS_CTRL_ICACHE_INVALIDATE	(1 << 12)
#define INTEL_IPU4_PSYS_SPC_STATUS_ICACHE_PREFETCH		(1 << 13)

#define INTEL_IPU4_PSYS_REG_SPC_START_PC		0x4
#define INTEL_IPU4_PSYS_REG_SPC_ICACHE_BASE		0x10
#define INTEL_IPU4_PSYS_REG_SPP0_STATUS_CTRL		0x20000
#define INTEL_IPU4_PSYS_REG_SPP1_STATUS_CTRL		0x30000
#define INTEL_IPU4_PSYS_REG_SPF_STATUS_CTRL		0x40000
#define INTEL_IPU4_PSYS_REG_ISP0_STATUS_CTRL		0x1C0000
#define INTEL_IPU4_PSYS_REG_ISP1_STATUS_CTRL		0x240000
#define INTEL_IPU4_PSYS_REG_ISP2_STATUS_CTRL		0x2C0000
#define INTEL_IPU4_PSYS_REG_ISP3_STATUS_CTRL		0x340000
#define INTEL_IPU4_REG_PSYS_INFO_SEG_0_CONFIG_ICACHE_MASTER	0x14

/* IRQ-related registers in PSYS, relative to INTEL_IPU4_BXT_xx_PSYS_OFFSET */
#define INTEL_IPU4_REG_PSYS_GPDEV_IRQ_EDGE		0x60200
#define INTEL_IPU4_REG_PSYS_GPDEV_IRQ_MASK		0x60204
#define INTEL_IPU4_REG_PSYS_GPDEV_IRQ_STATUS		0x60208
#define INTEL_IPU4_REG_PSYS_GPDEV_IRQ_CLEAR		0x6020c
#define INTEL_IPU4_REG_PSYS_GPDEV_IRQ_ENABLE		0x60210
#define INTEL_IPU4_REG_PSYS_GPDEV_IRQ_LEVEL_NOT_PULSE	0x60214
/* There are 8 FW interrupts, n = 0..7 */
#define INTEL_IPU4_PSYS_GPDEV_IRQ_FWIRQ(n)		(BIT(17) << (n))
#define INTEL_IPU4_REG_PSYS_GPDEV_FWIRQ(n)		(4 * (n) + 0x60100)
/* CDC Burst collector thresholds for psys - 4 FIFOs i= 0..3 */
#define INTEL_IPU4_REG_PSYS_CDC_THRESHOLD(i)           (0x60600 + ((i) * 4))

/*VCO*/
#define INTEL_IPU4_INFO_ENABLE_SNOOP			BIT(0)
#define INTEL_IPU4_INFO_IMR_DESTINED			BIT(1)
#define INTEL_IPU4_INFO_REQUEST_DESTINATION_BUT_REGS	0
#define INTEL_IPU4_INFO_REQUEST_DESTINATION_PRIMARY	BIT(4)
#define INTEL_IPU4_INFO_REQUEST_DESTINATION_P2P		(BIT(4) | BIT(5))
/*VC1*/
#define INTEL_IPU4_INFO_DEADLINE_PTR                      BIT(1)
#define INTEL_IPU4_INFO_ZLW                               BIT(2)
#define INTEL_IPU4_INFO_STREAM_ID_SET(a)	((a & 0xF) << 4)
#define INTEL_IPU4_INFO_ADDRESS_SWIZZ                     BIT(8)

/* ISYS trace registers - offsets to isys base address */
/* Trace unit base offset */
#define TRACE_REG_IS_TRACE_UNIT_BASE			0x07d000
/* Trace monitors */
#define TRACE_REG_IS_SP_EVQ_BASE			0x001000
/* GPC blocks */
#define TRACE_REG_IS_SP_GPC_BASE			0x000800
#define TRACE_REG_IS_ISL_GPC_BASE			0x0bd400
#define TRACE_REG_IS_MMU_GPC_BASE			0x0e0B00
/* CSI2 receivers */
#define TRACE_REG_CSI2_TM_BASE				0x067a00
#define TRACE_REG_CSI2_3PH_TM_BASE			0x06f200

/* Trace timers */
#define TRACE_REG_PS_GPREG_TRACE_TIMER_RST_N		0x060614
#define TRACE_REG_IS_GPREG_TRACE_TIMER_RST_N		0x07c410
#define TRACE_REG_GPREG_TRACE_TIMER_RST_OFF		BIT(0)

/* SIG2CIO */
/* 0 < n <= 8 */
#define TRACE_REG_CSI2_SIG2SIO_GRn_BASE(n)		(0x067c00 + (n) * 0x20)
#define TRACE_REG_CSI2_SIG2SIO_GR_NUM			9

/* 0 < n <= 8 */
#define TRACE_REG_CSI2_PH3_SIG2SIO_GRn_BASE(n)		(0x06f600 + (n) * 0x20)
#define TRACE_REG_CSI2_PH3_SIG2SIO_GR_NUM		9

/* PSYS trace registers - offsets to isys base address */
/* Trace unit base offset */
#define TRACE_REG_PS_TRACE_UNIT_BASE			0x3e0000
/* Trace monitors */
#define TRACE_REG_PS_SPC_EVQ_BASE			0x001000
#define TRACE_REG_PS_SPP0_EVQ_BASE			0x021000
#define TRACE_REG_PS_SPP1_EVQ_BASE			0x031000
#define TRACE_REG_PS_SPF_EVQ_BASE			0x041000
#define TRACE_REG_PS_ISP0_EVQ_BASE			0x1c1000
#define TRACE_REG_PS_ISP1_EVQ_BASE			0x241000
#define TRACE_REG_PS_ISP2_EVQ_BASE			0x2c1000
#define TRACE_REG_PS_ISP3_EVQ_BASE			0x341000

/* GPC blocks */
#define TRACE_REG_PS_SPC_GPC_BASE			0x000800
#define TRACE_REG_PS_SPP0_GPC_BASE			0x020800
#define TRACE_REG_PS_SPP1_GPC_BASE			0x030800
#define TRACE_REG_PS_SPF_GPC_BASE			0x040800
#define TRACE_REG_PS_MMU_GPC_BASE			0x0b0b00
#define TRACE_REG_PS_ISL_GPC_BASE			0x0fe800
#define TRACE_REG_PS_ISP0_GPC_BASE			0x1c0800
#define TRACE_REG_PS_ISP1_GPC_BASE			0x240800
#define TRACE_REG_PS_ISP2_GPC_BASE			0x2c0800
#define TRACE_REG_PS_ISP3_GPC_BASE			0x340800

#endif /* INTEL_IPU4_REGS_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/intel-ipu4-regs.h $ $Rev: 838597 $")
#endif
