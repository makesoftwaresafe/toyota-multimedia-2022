/*
 * apple-gmux.h - microcontroller built into dual GPU MacBook Pro & Mac Pro
 * Copyright (C) 2015 Lukas Wunner <lukas@wunner.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LINUX_APPLE_GMUX_H
#define LINUX_APPLE_GMUX_H

#include <linux/acpi.h>

#define GMUX_ACPI_HID "APP000B"

#if IS_ENABLED(CONFIG_APPLE_GMUX)

/**
 * apple_gmux_present() - detect if gmux is built into the machine
 *
 * Drivers may use this to activate quirks specific to dual GPU MacBook Pros
 * and Mac Pros, e.g. for deferred probing, runtime pm and backlight.
 *
 * Return: %true if gmux is present and the kernel was configured
 * with CONFIG_APPLE_GMUX, %false otherwise.
 */
static inline bool apple_gmux_present(void)
{
	return acpi_dev_present(GMUX_ACPI_HID);
}

#else  /* !CONFIG_APPLE_GMUX */

static inline bool apple_gmux_present(void)
{
	return false;
}

#endif /* !CONFIG_APPLE_GMUX */

#endif /* LINUX_APPLE_GMUX_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/apple-gmux.h $ $Rev: 836322 $")
#endif
