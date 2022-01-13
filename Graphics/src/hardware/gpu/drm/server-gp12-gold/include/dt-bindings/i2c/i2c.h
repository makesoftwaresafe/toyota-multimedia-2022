/*
 * This header provides constants for I2C bindings
 *
 * Copyright (C) 2015 by Sang Engineering
 * Copyright (C) 2015 by Renesas Electronics Corporation
 *
 * Wolfram Sang <wsa@sang-engineering.com>
 *
 * GPLv2 only
 */

#ifndef _DT_BINDINGS_I2C_I2C_H
#define _DT_BINDINGS_I2C_I2C_H

#define I2C_TEN_BIT_ADDRESS	(1 << 31)
#define I2C_OWN_SLAVE_ADDRESS	(1 << 30)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/dt-bindings/i2c/i2c.h $ $Rev: 836322 $")
#endif
