/*
* Copyright (c) 2017 QNX Software Systems.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 * ipu4_resmgr.h
 *
 * This module implements a resource manager to handle all dynamic interactions
 * with clients of this service.
 */

#ifndef _IPU4_RESMGR_H_INCLUDED
#define _IPU4_RESMGR_H_INCLUDED

#include <intel/ipu4/ipu4_priv.h>


/**
 * @brief Initializes the resource manager for the IPU4 driver
 * @details
 * This will create a resource manager for the IPU4 drive.
 *
 * @return @c EOK on success, an error code on failure.
 */
int ipu4_resmgr_init(void);

/**
 * @brief De-initializes the resource manager of the IPU4 drive
 * @details
 * This stops and frees up any resources allocated by our resource manager.
 *
 * @return @c EOK on success, an error code on failure.
 */
int ipu4_resmgr_deinit(void);

/**
 * @brief Starts running our resource manager
 * @details
 * After having called @c event_resmgr_init(), call this function to permit
 * the resource manager to start handling user requests.
 *
 * @return @c EOK on success, an error code on failure.
 */
int ipu4_resmgr_start(void);

#endif   /* _IPU4_RESMGR_H_INCLUDED */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/ipu4/ipu4_resmgr.h $ $Rev: 838597 $")
#endif
