#ifndef PERSISTENCE_FILE_CACHE_ERROR_DEF_H
#define PERSISTENCE_FILE_CACHE_ERROR_DEF_H

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
 * @file           persistence_file_cache_error_def.h
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner
 * @brief          Error definition header
 * @see
 */

#ifdef __cplusplus
extern "C" {
#endif

/// common error, for this error errno will be set
#define EPERS_COMMON                   (-1)
#define EPERS_CREATE_FILE_CACHE        (-2)
#define EPERS_COPY_FILE_CACHE          (-3)
#define EPERS_CREATE_STAT_FILE         (-4)
#define EPERS_CACHE_STATUS_UNDEF       (-5)
#define EPERS_CACHE_FULL               (-6)
#define EPRS_FILE_TO_BIG               (-7)
#define EPRS_MAX_HANDLE                (-8)
#define EPRS_INVALID_HANDLE            (-9)





#ifdef __cplusplus
}
#endif

#endif /* PERSISTENCE_FILE_CACHE_ERROR_DEF_H */
