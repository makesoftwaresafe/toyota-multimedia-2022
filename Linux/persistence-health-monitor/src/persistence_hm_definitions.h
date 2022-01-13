#ifndef PERSISTENCE_HM_DEFINITIONS_H_
#define PERSISTENCE_HM_DEFINITIONS_H_

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2014
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_hm_definitions.h
 * @ingroup        Persistence health monitor
 * @author         Ingo Huerner
 * @brief          Header of the persistence health monitor dbus service.
 * @see
 */

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>

/// get number of elements for arrays
#define ARRAY_LENGTH(a) (sizeof(a)/sizeof(*a))
/// check if flags set
#define FLAGS_SET(flags, mask) (((flags)&(mask))==(mask))

DLT_DECLARE_CONTEXT(phmContext);


#endif /* PERSISTENCE_HM_DEFINITIONS_H_ */
