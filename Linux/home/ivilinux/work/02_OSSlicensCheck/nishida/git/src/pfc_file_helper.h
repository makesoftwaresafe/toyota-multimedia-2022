#ifndef PERSISTENCE_FILE_CACHE_BACKUP_H
#define PERSISTENCE_FILE_CACHE_BACKUP_H

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
 * @file           persistence_file_cache_backup.h
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner
 * @brief          Header of the persistence file cache backup
 * @see
 */


#include "pfc_def.h"
#include "pfc_handle.h"


/**
 * @brief create the file including all the folders given by the path string
 *
 * @param path directory and filename
 *
 * @return a handle to the file
 */
int helperCreateAndOpenFile(const char* path);


/**
 * @brief create a backup of a file including a checksum
 *
 * @param
 *
 * @return
 */
int helperCreateBackup(const char* srcPath, int srcfd, const char* csumPath, const char* csumBuf);


/**
 * @brief verify file for consistency
 *
 * @param
 *
 * @return
 */
int helperVerifyConsistency(const char* origPath, const char* backupPath, const char* csumPath, int openFlags);


/**
 * @brief format directory string.
 *        remove '/' at the end of the directory string if there is one
 *
 * @param
 *
 * @return
 */
void helperFormatDirString(char* dirString);


/**
 * @brief Copy a file
 *
 * @param srcFd the source
 * @param dstFd the destination
 *
 * @return
 */
int helperDoFileCopy(int srcFd, int dstFd);


/**
 * @brief remove backup
 *
 * @param
 *
 * @return
 */
void helperRemoveBackup(int handle);


/**
 * @brief calculate the crc32 checkusm
 *
 * @param fd the file to generate the checkusm from
 * @param crc32sum the buffer to store the checksum
 *
 * @return
 */
int helperCalcCrc32Csum(int fd, char crc32sum[]);


#endif /* PERSISTENCE_FILE_CACHE_BACKUP_H */
