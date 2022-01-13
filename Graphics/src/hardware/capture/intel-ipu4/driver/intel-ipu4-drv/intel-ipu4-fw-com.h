/*
 * Copyright (c) 2013--2016 Intel Corporation.
 * Some modifications (__QNXNTO__) Copyright (c) 2017 QNX Software Systems.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#ifndef __INTEL_IPU4_FW_COM_HEADER__
#define __INTEL_IPU4_FW_COM_HEADER__

struct intel_ipu4_fw_com_context;
struct intel_ipu4_bus_device;

#ifndef __QNXNTO__
struct ia_css_syscom_queue_config {
	unsigned int queue_size; /* tokens per queue */
	unsigned int token_size; /* bytes per token */
};
#else
// Already defined in lib2600, so take that definition to avoid duplicates
#include "ia_css_syscom_config.h"
#endif

struct intel_ipu4_fw_com_cfg {

	unsigned int num_input_queues;
	unsigned int num_output_queues;
	struct ia_css_syscom_queue_config *input;
	struct ia_css_syscom_queue_config *output;

	unsigned int dmem_addr;

	/* firmware-specific configuration data */
	void *specific_addr;
	unsigned int specific_size;
	int (*cell_ready)(struct intel_ipu4_bus_device *adev);
	void (*cell_start)(struct intel_ipu4_bus_device *adev);
};

void *intel_ipu4_fw_com_prepare(struct intel_ipu4_fw_com_cfg *cfg,
				struct intel_ipu4_bus_device *adev,
				void __iomem *base);

int intel_ipu4_fw_com_open(struct intel_ipu4_fw_com_context *ctx);
int intel_ipu4_fw_com_ready(struct intel_ipu4_fw_com_context *ctx);
int intel_ipu4_fw_com_close(struct intel_ipu4_fw_com_context *ctx);
int intel_ipu4_fw_com_release(struct intel_ipu4_fw_com_context *ctx,
			      unsigned int force);

void *intel_ipu4_recv_get_token(struct intel_ipu4_fw_com_context *ctx,
				int q_nbr);
void intel_ipu4_recv_put_token(struct intel_ipu4_fw_com_context *ctx,
			       int q_nbr);
void *intel_ipu4_send_get_token(struct intel_ipu4_fw_com_context *ctx,
				int q_nbr);
void intel_ipu4_send_put_token(struct intel_ipu4_fw_com_context *ctx,
			       int q_nbr);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/intel-ipu4-fw-com.h $ $Rev: 836043 $")
#endif
