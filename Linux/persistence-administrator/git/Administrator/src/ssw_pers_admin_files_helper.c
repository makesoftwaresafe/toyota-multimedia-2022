/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Implementation of functions declared in ssw_pers_admin_files_helper.h
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2015.10.09 Cosmin Cernat      Bug 295:  In persadmin_create_symbolic_link() if the symlink already exists, delete it before creating it.
* 2014.02.03 Ionut Ieremie      CSP_WZ#8463:  Added persadmin_get_file_size()
* 2013.05.30 Ionut Ieremie      CSP_WZ#12188: Rework persadmin_get_folder_size()
* 2013.02.07 Petrica Manoila    CSP_WZ#2220:  Added persadmin_check_for_same_file_content to check for identical file content
* 2012.11.16 Alin Liteanu       CSP_WZ#1280:  persadmin_delete_folder and persadmin_delete_file return the number of bytes deleted
* 2012.11.15 Ionut Ieremie      CSP_WZ#1280:  Some extensions:
                                    - persadmin_copy_folder and persadmin_copy_file return the number of bytes copied
                                    - added persadmin_check_if_file_exist and persadmin_check_if_file_exist
                                    - fixed some bugs
* 2012.11.12 uidl9757           CSP_WZ#1280:  Created
*
**********************************************************************************************************************/ 

/* ---------------------- include files  --------------------------------- */
#include "persComTypes.h"
#include <stddef.h>
#include <stdio.h> //TODO: use correct grant
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "persistence_admin_service.h"
#include "ssw_pers_admin_files_helper.h"
#include "ssw_pers_admin_common.h"

#include "dlt/dlt.h"

#define LT_HDR                          "CONF >> "
DLT_IMPORT_CONTEXT                      (persAdminSvcDLTCtx)


#define GetApplicationRootPath( RootPath, FullPath )                                            \
  int pathLength = strlen( RootPath );                                                          \
  char FullPath[ strlen( gLocalCachePath ) + pathLength + sizeof( StringTerminator ) ];         \
  (void)snprintf( FullPath, sizeof( FullPath ), "%s", RootPath );                                     \
  (void)snprintf( FullPath + pathLength, sizeof( FullPath ) - pathLength, gLocalCachePath, "", "" )

const char PathSeparator = '/';
const char StringTerminator = '\0';

/* ---------------------- local types -------------------------------------- */
typedef struct
{
    pstr_t rootPath ;
    pstr_t absPath ;
    sint_t startOfRelativePath ;
}folder_path_s ;

/* ---------------------- local functions ---------------------------------- */
static bool_t persadmin_check_dir_entry_against_filter(struct stat *psb, PersadminFilterMode_e eFilter) ;
static sint_t persadmin_list_subfolder(folder_path_s* psFolderPath, pstr_t buffer_out, sint_t bufSize, sint_t * posInBuffer, PersadminFilterMode_e eFilter, bool_t bRecursive, sint_t posInCompletePath, sint_t nestingLevel) ;
static sint_t persadmin_copy_subfolders(pstr_t absPathSource, sint_t posInSourcePath, pstr_t absPathDestination, sint_t posInDestPath, sint_t nestingCounter, PersadminFilterMode_e eFilter, bool_t bRecursive) ;
static sint_t persadmin_delete_subfolders(pstr_t folderPath, sint_t posInFolderPath, sint_t nestingLevel) ;
static sint_t persadmin_priv_get_file_size( pconststr_t absPathFileName );

/* ---------------------- local variables ---------------------------------- */
static str_t g_msg[512] ; 

static bool_t persadmin_check_dir_entry_against_filter(struct stat *psb, PersadminFilterMode_e eFilter)
{
    bool_t bFileMatchesFilter = false ;
    
    switch(eFilter)
    {
        case PersadminFilterAll:
        {
            bFileMatchesFilter = ( (S_ISREG(psb->st_mode)) || (S_ISDIR(psb->st_mode)) || (S_ISLNK(psb->st_mode)) ) ;
            break ;
        }
        case PersadminFilterFilesAll:
        {
            bFileMatchesFilter = ( (S_ISREG(psb->st_mode))  || (S_ISLNK(psb->st_mode)) ) ;
            break ;
        }
        case PersadminFilterFilesRegular:
        {
            bFileMatchesFilter = (S_ISREG(psb->st_mode)) ;
            break ;
        }
        case PersadminFilterFilesLink:
        {
            bFileMatchesFilter = (S_ISLNK(psb->st_mode)) ;
            break ;
        }
        case PersadminFilterFoldersAndRegularFiles:
        {
            bFileMatchesFilter = ( (S_ISREG(psb->st_mode)) || (S_ISDIR(psb->st_mode)) ) ;
            break ;
        }
        case PersadminFilterFoldersAndLinkFiles:
        {
            bFileMatchesFilter = ( (S_ISLNK(psb->st_mode)) || (S_ISDIR(psb->st_mode)) ) ;
            break ;
        }
        case PersadminFilterFolders:
        {
            bFileMatchesFilter = (S_ISDIR(psb->st_mode)) ;
            break ;
        }
        default:
        {
            break ;
        }
    }

    return bFileMatchesFilter ;
}


/*
* filePath can be absolute or relative
* it is not checked if the file indicated by filePath actually exist
* returns in fileName_out the part of filePath after the last '/' (or the complete filePath if there is no '/') 
*/
sint_t persadmin_get_filename(pstr_t filePath, pstr_t fileName_out, sint_t size)
{
    bool_t bEverythingOK = true ;
    bool_t bFilenameFound = false ;
    sint_t filePathSize = 0 ;

    if( (NIL == filePath) || (NIL == fileName_out))
    {
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_filename: invalid params") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        bEverythingOK = false ;
    }
    else
    {
        filePathSize = (sint_t)strlen(filePath) ;
    }

    if(bEverythingOK)
    {
        /* filename is the part after the last '/' in the path (but path must not end with '/') */
        sint_t posInFilePath = filePathSize-1 ;
        if('/' != filePath[posInFilePath])
        {
            while((posInFilePath >= 0) && bEverythingOK && (! bFilenameFound))
            {
                if('/' == filePath[posInFilePath])
                {
                    if((filePathSize - (posInFilePath+1)) < size)
                    {
                        (void)strcpy(fileName_out, filePath+posInFilePath+1) ;
                        bFilenameFound = true ;
                    }
                    else
                    {
                        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_filename: size too large (%d, available %d)", (filePathSize - (posInFilePath+1)), size) ;
                        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                        bEverythingOK = false ;
                    }
                }
                posInFilePath-- ;
            }
        }
        else
        {
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_filename: filePath ends with '/'") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            bEverythingOK = false ;
        }
    }

    if(bEverythingOK && (! bFilenameFound))
    {        
        /* no '/' found in the path, so return the complete path */
        if((filePathSize < size) && (bEverythingOK))
        {
            (void)strcpy(fileName_out, filePath) ;
        }
        else
        {
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_filename: size too large (%d, available %d)", (filePathSize+1), size) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                bEverythingOK = false ;
        }
    }    

    return (bEverythingOK ? 0 : PAS_FAILURE) ;
}

/*
* filePath can be absolute or relative
* it is not checked if the file indicated by filePath actually exist
* returns the part of filePath after the last '.' (but must be after the last '/') 
*/
sint_t persadmin_get_file_extension(pstr_t filePath, pstr_t fileExtension_out, sint_t size)
{
    bool_t bEverythingOK = true ;
    bool_t bExtensionFound = false ;

    if( (NIL == filePath) || (NIL == fileExtension_out))
    {
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_file_extension: invalid params") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        bEverythingOK = false ;
    }
    else
    {
        sint_t filePathSize = (sint_t)strlen(filePath) ;
        sint_t posInFilePath = filePathSize-1 ;
        if(('/' != filePath[posInFilePath]) && ('.' != filePath[posInFilePath]))
        {
            while((posInFilePath >= 0) && bEverythingOK && (! bExtensionFound))
            {
                if(('/' == filePath[posInFilePath]) || ('.' == filePath[posInFilePath]))
                {
                    if('.' == filePath[posInFilePath])
                    {
                        if((filePathSize - (posInFilePath+1)) < size)
                        {
                            (void)strcpy(fileExtension_out, filePath+posInFilePath+1) ;
                            bExtensionFound = true ;
                        }
                        else
                        {
                            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_file_extension: size too large (%d, available %d)", (filePathSize - (posInFilePath+1)), size) ;
                            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                            bEverythingOK = false ;
                        }
                    }
                    else
                    {
                        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_file_extension: no extension found") ;
                        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                        bEverythingOK = false ;
                    }
                }
                posInFilePath-- ;
            }
        }
        else
        {
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_file_extension: filePath ends with '%c'", filePath[posInFilePath]) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            bEverythingOK = false ;
        }
    }


    return (bEverythingOK ? 0 : PAS_FAILURE) ;
}

/*
* get the size of the file
* returns size of the file, or a negative value in case of error
*/
sint_t persadmin_get_file_size(pconststr_t pathname)
{
    sint_t iFileSize = 0 ;

    if(NIL != pathname)
    {
        if(0 <= persadmin_check_if_file_exists(pathname, false))
        {
            iFileSize = persadmin_priv_get_file_size(pathname) ;
        }
        else
        {
            iFileSize = PAS_FAILURE_NOT_FOUND ;
        }
    }
    else
    {
        iFileSize = PAS_FAILURE_INVALID_PARAMETER ;
    }

    return iFileSize ;
}

/*
* filePath can be absolute or relative
* it is not checked if the file indicated by filePath actually exist
* returns the part of filePath before (and including) the last '/'
*/
sint_t persadmin_get_folder_path(pstr_t filePath, pstr_t folderPath_out, sint_t size)
{
    bool_t bEverythingOK = true ;
    bool_t bFolderPathFound = false ;
    
    if( (NIL == filePath) || (NIL == folderPath_out))
    {
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_folder_path: invalid params") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        bEverythingOK = false ;
    }
    else
    {
        sint_t filePathSize = (sint_t)strlen(filePath) ;
        sint_t posInFilePath = filePathSize-1 ;
        if('/' != filePath[posInFilePath])
        {
            while((posInFilePath >= 0) && bEverythingOK && (! bFolderPathFound))
            {
                if('/' == filePath[posInFilePath])
                {
                    if((filePathSize - (posInFilePath+1)) < size)
                    {
                        (void)strncpy(folderPath_out, filePath, (uint_t)posInFilePath) ;
                        folderPath_out[posInFilePath] = '\0' ;
                        bFolderPathFound = true ;
                    }
                    else
                    {
                        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_folder_path: size too large (%d, available %d)", posInFilePath+1, size) ;
                        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                        bEverythingOK = false ;
                    }
                }
                posInFilePath-- ;
            }
            if(posInFilePath < 0)
            {
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_folder_path: <%s> is not a valid path", filePath) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                bEverythingOK = false ;
            }
        }
        else
        {
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_get_filename: filePath ends with '/'") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            bEverythingOK = false ;
        }
    }

    return (bEverythingOK ? 0 : PAS_FAILURE) ;
}

/*
* check if file/folder indicated by pathname exist
* if (bIsFolder==true) it is checked the existence of the folder, otherway the existence of the file
* returns 0 if the file/folder exist, a negative value other way
*/
sint_t persadmin_check_if_file_exists(pstr_t pathname, bool_t bIsFolder)
{
    bool_t bEverythingOK = true ;
    if(NIL == pathname)
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_check_if_file_exist: NIL pathname") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        if('/' != pathname[0])
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_check_if_file_exist: not an absolute path(%s)", pathname) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        struct stat sb;

        if (0 == lstat(pathname, &sb))
        {
            /* pathname exist*/
            if(bIsFolder)
            {
                /* check if it is a foler */
                if( ! S_ISDIR(sb.st_mode))
                {
                    /* not a folder */
                    bEverythingOK = false ;
                }
            }
            else
            {
                /* check if it is a file */
                if(S_ISDIR(sb.st_mode))
                {
                    /* it is a folder */
                    bEverythingOK = false ;
                }
            }
        }
        else
        {
            bEverythingOK = false ;
        }
    }

    return bEverythingOK ? 0 : PAS_FAILURE ;
}

/*
* create a link in the path indicated by pathLink which points to path indeicated by pathTarget
* there is no check if pathTarget actually exist (or is valid)
* the intermediate folders in pathLink are created if they do not exist
* returns 0 in case of success, a negative value other way
*/
sint_t persadmin_create_symbolic_link(pstr_t pathLink, pstr_t pathTarget)
{
    bool_t bEverythingOK = true ;

    if((NIL == pathLink) || (NIL == pathTarget))
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_symbolic_link: NIL pathLink or pathTarget ") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        /* check if paths are absolute */
        if(('/' != pathLink[0]) || ('/' != pathTarget[0]))
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_symbolic_link: not absolute paths (<%s>, <%s>) ", pathLink, pathTarget) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        /* create the intermediary folders in pathLink */
        str_t folderPathLink[PERSADMIN_MAX_PATH_LENGHT] ;

        if(0 == persadmin_get_folder_path(pathLink, folderPathLink, sizeof(folderPathLink)))
        {
            if(0 != persadmin_create_folder(folderPathLink))
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_symbolic_link: persadmin_create_folder(<%s>) failed ", folderPathLink) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_symbolic_link: persadmin_get_folder_path(<%s>) failed ", pathLink) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        /* if the symlink already exists, delete it */
        if(0 <= persadmin_check_if_file_exists(pathLink, false))
        {
            if(0 > persadmin_delete_file(pathLink))
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), ": persadmin_delete_file(%s) failed", pathLink) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(__FUNCTION__), DLT_STRING(g_msg));
            }
        }
    }

    if(bEverythingOK)
    {
        if(0 != symlink(pathTarget, pathLink))
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_symbolic_link: symlink(<%s>, <%s>) errno=<%s> ", pathTarget, pathLink, strerror(errno)) ;  /*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    return bEverythingOK ? 0 : PAS_FAILURE ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

static sint_t persadmin_copy_subfolders(pstr_t absPathSource, sint_t posInSourcePath, pstr_t absPathDestination, sint_t posInDestPath, sint_t nestingCounter, PersadminFilterMode_e eFilter, bool_t bRecursive)
{
    bool_t bEverythingOK = true ;
    DIR *dp;
    struct dirent *ep;
    sint_t bytesCopied = 0 ;

    if((++nestingCounter) < PERSADMIN_MAX_NESTING_DEPTH)
    {
        if(absPathSource[posInSourcePath-1] != '/')
        {
            absPathSource[posInSourcePath] = '/' ;
            posInSourcePath++ ;
            absPathSource[posInSourcePath] = '\0' ;
        }

        if(absPathDestination[posInDestPath-1] != '/')
        {
            absPathDestination[posInDestPath] = '/' ;
            posInDestPath++ ;
            absPathDestination[posInDestPath] = '\0' ;
        }
    }
    else
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_subfolders(<<%s>>, <<%s>>) too much nesting !!! ", absPathSource, absPathDestination) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        dp = opendir (absPathSource);
        if(NIL == dp)
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_subfolders: opendir(%s) errno=<%s>", absPathSource, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if (bEverythingOK)
    {
        ep = readdir (dp);
        while ((ep  != NIL) && (bEverythingOK))
        {
            int cmp = strcmp(ep->d_name, "..");
            if( (0 != strcmp(ep->d_name, ".")) && (0 != cmp))
            {
                sint_t err ;
                struct stat sb;
                sint_t len = (sint_t)strlen(ep->d_name) ;

                if((posInSourcePath + len > PERSADMIN_MAX_PATH_LENGHT) || (posInDestPath + len > PERSADMIN_MAX_PATH_LENGHT)) 
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_subfolders: path too long(%s%s)", absPathSource, ep->d_name) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                    break ;
                }
                else
                {
                    (void)strcpy(absPathSource+posInSourcePath, ep->d_name) ;
                    (void)strcpy(absPathDestination+posInDestPath, ep->d_name) ;
                }
                
                err = lstat(absPathSource, &sb) ;
                if(err >= 0)
                {
                    bool_t bFilterPassed = persadmin_check_dir_entry_against_filter(&sb, eFilter) ;
                    if(S_ISDIR(sb.st_mode))
                    {
                        if(bFilterPassed)
                        {
                            if(0 != persadmin_create_folder(absPathDestination))
                            {
                                bEverythingOK = false ;
                                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_subfolders: persadmin_copy_subfolders(%s, %s) failed", absPathSource, absPathDestination) ;
                                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                                break ;
                            }
                        }
                        if(bRecursive)
                        {
                            sint_t bytesCopiedLocal = persadmin_copy_subfolders(absPathSource, posInSourcePath+len, absPathDestination, posInDestPath+len, nestingCounter, eFilter, bRecursive) ;
                            if(bytesCopiedLocal >= 0)
                            {
                                bytesCopied += bytesCopiedLocal ;
                            }
                            else
                            {
                                bEverythingOK = false ;
                                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_subfolders: persadmin_copy_subfolders(%s, %s) failed", absPathSource, absPathDestination) ;
                                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                            }                                                    
                        }
                    }
                    else
                    {
                        if(bFilterPassed)
                        {
                            sint_t bytesCopiedLocal = persadmin_copy_file(absPathSource, absPathDestination) ;
                            if(bytesCopiedLocal >= 0)
                            {
                                bytesCopied += bytesCopiedLocal ;
                            }
                            else
                            {
                                bEverythingOK = false ;
                                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_subfolders: persadmin_copy_file(%s, %s) failed", absPathSource, absPathDestination) ;
                                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                            }
                        }
                    }
                }
            }
            ep = readdir (dp);
        }
        (void)closedir (dp);
    }

    return (bEverythingOK ? bytesCopied : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*//*DG C8ISQP-ISQP Metric 7-SSW_Administrator_0004*/


/**
 * \brief copy the folder indicated by absPathSource into the folder indicated by absPathDestination
 * \usage : persadmin_copy_folder("/Data/mnt-c/App-1/", "/var/Pers/bkup/App_1_bkup/")
 *        : copy the content of /Data/mnt-c/App-1/ into /var/Pers/bkup/App_1_bkup/
 *
 * \param absPathSource         [in] absolute path of source folder
 * \param absPathDestination    [in] absolute path of destination folder
 * \return number of bytes copied, negative value for error
 * TODO: define error codes
 */
sint_t persadmin_copy_folder(pstr_t absPathSource, pstr_t absPathDestination, PersadminFilterMode_e eFilter, bool_t bRecursive)
{
    bool_t bEverythingOK = true ;
    str_t sourcePath[PERSADMIN_MAX_PATH_LENGHT+1] ;
    str_t destPath[PERSADMIN_MAX_PATH_LENGHT+1] ;
    sint_t bytesCopied = 0 ;

    if( (NIL != absPathSource) && (NIL != absPathDestination))
    {
        if((absPathSource[0] != '/') || (absPathDestination[0]!= '/'))
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_folder: invalid params <<%s>> , <<%s>>", absPathSource, absPathDestination) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }
    else
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_folder: invalid params") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        sint_t lenghtSourcePath = (sint_t)strlen(absPathSource) ;
        sint_t lenghtDestPath = (sint_t)strlen(absPathDestination) ;
        if((lenghtSourcePath < PERSADMIN_MAX_PATH_LENGHT) && (lenghtDestPath < PERSADMIN_MAX_PATH_LENGHT))
        {
            if(0 == persadmin_create_folder(absPathDestination))
            {
                (void)strcpy(sourcePath, absPathSource) ;
                (void)strcpy(destPath, absPathDestination) ;
                bytesCopied = persadmin_copy_subfolders(sourcePath, lenghtSourcePath, destPath, lenghtDestPath, 0, eFilter, bRecursive) ;
                if(bytesCopied < 0)
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_folder: persadmin_copy_subfolders(<%s>, <%s>) failed", absPathSource, absPathDestination) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
            else
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_folder: persadmin_create_folder(%s) failed", absPathDestination) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_folder: too long paths") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    return (bEverythingOK ? bytesCopied : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 7-SSW_Administrator_0004*//*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief create a folder as indicated by absPath
 * \usage : persadmin_create_folder("/var/Pers/bkup/App_1_bkup/")
 * \note : all the missing folders in the path are created (i.e. /var/Pers/, /var/Pers/bkup/)
 *
 * \param absPath               [in] absolute path to folder to be created
 * \return 0 for success, negative value otherwise;
 * TODO: define error codes
 */
sint_t persadmin_create_folder(pstr_t absPath)
{
    bool_t bEverythingOK = true ;
    const mode_t mode  = S_IRWXU|S_IRWXG|S_IRWXO ;
    char opath[PERSADMIN_MAX_PATH_LENGHT+1];
    pstr_t p;
    size_t len;

    if(NIL == absPath)
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_folder: NIL path") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        if('/' == *absPath)
        {
            
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_folder: not an absolute path (%s)!!!", absPath) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        } 
    }

    if(bEverythingOK)
    {
        len = strlen(absPath) ;
        if(len >= sizeof(opath))
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_folder: path too long (%s)", absPath) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }          
    }
    
    if(bEverythingOK)
    {
        (void)strcpy(opath, absPath); /* the lenght is check above */
        if(opath[len - 1] == '/')
        {
            opath[len - 1] = '\0';
        }
        
        /* create intermediate folders (if not alreade created) */
        for(p = opath+1; *p; p++)
        {
                if(*p == '/') 
                {
                    *p = '\0';
                    if(access(opath, F_OK))
                    {
                        sint_t err = mkdir(opath, mode);
                        if((err < 0) && (err != EEXIST ))
                        {
                            bEverythingOK = false ;
                            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_folder: mkdir(%s) errcode = <%s>", opath, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
                            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                            break ;
                        }
                    }
                    *p = '/';
                }
        }

        /* create the actual folder (if not already created) */
        if(access(opath, F_OK))         /* if path is not terminated with '/' */
        { 
            sint_t err = mkdir(opath, mode);
            if((err < 0) && (err != EEXIST ))
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_create_folder: mkdir(%s) errcode = <%s>", opath, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
     }

    return (bEverythingOK ? 0 : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static sint_t persadmin_delete_subfolders(pstr_t folderPath, sint_t posInFolderPath, sint_t nestingLevel)
{
    bool_t bEverythingOK = true ;
    DIR *dp;
    struct dirent *ep;
    sint_t lLenBytes;
    sint_t lBytesDeleted = 0;

    if((nestingLevel++) < PERSADMIN_MAX_NESTING_DEPTH)
    {
        if(folderPath[posInFolderPath-1] != '/')
        {
            folderPath[posInFolderPath] = '/' ;
            posInFolderPath += 1 ;
            folderPath[posInFolderPath] = '\0' ;
        }
    }
    else
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_subfolders(%s) too deep hierarchy", folderPath) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        dp = opendir(folderPath);
        if (NIL == dp)
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_subfolders: opendir(%s) errno=<%s>", folderPath, strerror(errno));/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        ep = readdir (dp);
        while ((ep != NIL) && (bEverythingOK))
        {
            int cmp = strcmp(ep->d_name, "..");//for QAC
            if( (0 != strcmp(ep->d_name, ".")) && (0 != cmp))
            {
                sint_t err ;
                struct stat sb;
                sint_t len = (sint_t)strlen(ep->d_name) ;

                if(posInFolderPath + len < PERSADMIN_MAX_PATH_LENGHT)
                {
                    (void)strcpy(folderPath+posInFolderPath, ep->d_name) ;
                
                    err = lstat(folderPath, &sb) ;
                    if(err >= 0)
                    {
                        if(S_ISDIR(sb.st_mode))
                        {
                            lLenBytes = persadmin_delete_subfolders(folderPath, posInFolderPath+len, nestingLevel);
                            if(0 <= lLenBytes)
                            {
                                lBytesDeleted += lLenBytes;
                                folderPath[posInFolderPath+len] = '\0' ;
                                if(0 != remove(folderPath))
                                {
                                    bEverythingOK = false ;
                                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_subfolders: remove(%s) errno=<%s>", folderPath, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
                                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                                }
                                else
                                {
                                    /* (void)snprintf(g_msg, sizeof(g_msg), "deleted >>%s<<", folderPath) ; */
                                }
                            }
                            else
                            {
                                bEverythingOK = false ;
                                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_subfolders: persadmin_delete_subfolders(%s) failed", folderPath) ;
                                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                            }                                                    
                        }
                        else
                        {
                            if( (S_ISREG(sb.st_mode)) || (S_ISLNK(sb.st_mode)) )
                            {
                                lLenBytes = sb.st_size;
                                if(0 != remove(folderPath))
                                {                                    
                                    bEverythingOK = false ;
                                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_subfolders: remove(%s) errno=<%s>", folderPath, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
                                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                                }
                                else
                                {
                                    lBytesDeleted += lLenBytes;
                                    /* (void)snprintf(g_msg, sizeof(g_msg), "deleted >>%s<<", folderPath) ; */
                                }
                            }
                            else
                            {
                                bEverythingOK = false ;
                                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_subfolders: unexpected mode for <%s> st_mode=0x%X", folderPath, sb.st_mode) ;
                                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                            }
                        }
                    }
                }
                else
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_subfolders: path too long (%s)", folderPath) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
            ep = readdir (dp);
        }
        (void)closedir (dp);
    }

    return (bEverythingOK ? lBytesDeleted : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/**
 * \brief delete a folder
 * \usage : persadmin_delete_folder("/var/Pers/bkup/App_1_bkup/")
 *
 * \param absPath               [in] absolute path to folder to be deleted
 * \return positive value: number of bytes deleted; negative value: error
 * TODO: define error codes
 */
sint_t persadmin_delete_folder(pstr_t absPath)
{
    bool_t bEverythingOK = true ;
    str_t absFolderPath[PERSADMIN_MAX_PATH_LENGHT+1] ;
    sint_t lBytesDeleted = 0;  
    
    if(NIL == absPath)
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_folder: invalid params") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        sint_t len = (sint_t)strlen(absPath) ;
        if(len < PERSADMIN_MAX_PATH_LENGHT)
        {
            (void)strcpy(absFolderPath, absPath) ;
            lBytesDeleted = persadmin_delete_subfolders(absFolderPath, len, 0);
            if(0 <= lBytesDeleted)
            {
                if(0 == remove(absPath))
                {
                    (void)snprintf(g_msg, sizeof(g_msg), "deleted >>%s<<", absPath) ;
                }
                else
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_folder: remove(%s) errno=<%s>", absPath, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
            else
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_delete_folder: persadmin_delete_subfolders(%s) failed", absPath) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                lBytesDeleted = 0;
            }
        }
    }

    return (bEverythingOK ? lBytesDeleted : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief copy a file (with posibility to rename the copy)
 * \usage : persadmin_copy_file("/Data/mnt-c/shared/public/cached.itz", "/var/Pers/bkup/CACHED.itz")
 *        : content of /Data/mnt-c/shared/public/cached.itz is copied into /var/Pers/bkup/CACHED.itz
 *
 * \param absPathSource         [in] absolute path to the source file
 * \param absPathDestination    [in] absolute path to the destination file
 * \return number of bytes copied, negative value for error
 * TODO: define error codes
 */
sint_t persadmin_copy_file(pstr_t absPathSource, pstr_t absPathDestination)
{
    bool_t bEverythingOK = true ;
    FILE *fileSource = NIL ;
    FILE *fileDest = NIL ;
    str_t destFolderPath[256] ;
    sint_t bytesCopied = 0 ;

    if((NIL == absPathSource) || (NIL == absPathDestination))
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_file: Invalid params ") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        fileSource = fopen(absPathSource, "r") ;
        if(NIL == fileSource)
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_file: fopen(%s, r) errno=<%s>", absPathSource, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        if(0 == persadmin_get_folder_path(absPathDestination, destFolderPath, sizeof(destFolderPath)))
        {
            if(0 != persadmin_create_folder(destFolderPath))
            {
                bEverythingOK = false; 
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_file: persadmin_create_folder(%s) failed", destFolderPath) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
        else
        {
            bEverythingOK = false; 
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_file: persadmin_get_folder_path(%s) faield", destFolderPath) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        fileDest = fopen(absPathDestination, "w+") ;
        if(NIL == fileDest)
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_file: fopen(%s, w+) errno=<%s>", absPathDestination, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        str_t buffer[1024] ;
        int readSize = 0 ;

        do
        {
            readSize = (int)fread(buffer, 1, sizeof(buffer), fileSource) ;
            if(readSize > 0)
            {
                if(readSize != (int)fwrite(buffer, 1, (size_t)readSize, fileDest))
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_file: fwrite(%s) errno=<%s>", absPathDestination, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
                else
                {
                    bytesCopied += readSize ;
                }
            }
            else
            {
                if(0 == feof(fileSource))
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_copy_file: fread(%s) errno=<%s>", absPathSource, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
        }while(bEverythingOK && (readSize > 0)) ;        
    }

    if(fileSource)
    {
        (void)fclose(fileSource) ;
    }

    if(fileDest)
    {
        //fsync(fileDest) ;
        (void)fclose(fileDest) ;
    }

    return (bEverythingOK ? bytesCopied : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/**
 * \brief delete a file
 * \usage : persadmin_delete_file("/Data/mnt-c/shared/public/cached.itz")
 *
 * \param absPath               [in] absolute path to the file to be deleted
 * \return positive value: number of bytes deleted; negative value: error
 * TODO: define error codes
 */
sint_t persadmin_delete_file(pstr_t absPath)
{
    bool_t bEverythingOK = true ;
    struct stat sb;        
    sint_t lBytesDeleted = 0;    

    if ( 0 == lstat(absPath, &sb) )
    {
      lBytesDeleted = sb.st_size;
      if ( 0 != remove(absPath) )
      {
          (void)snprintf(g_msg, sizeof(g_msg), "remove(%s) errno=<%s>", absPath, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
          DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
          bEverythingOK = false ;
          lBytesDeleted = 0;
      }
    }
    else
    {
      (void)snprintf(g_msg, sizeof(g_msg), "remove(%s) errno=<%s>", absPath, strerror(errno)) ;/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
      DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
      bEverythingOK = false;
    }
    
    return (bEverythingOK ? lBytesDeleted : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/* if (0 == bufSize) then listing is not done, only the size needed to accomodate the listing is reported */
static sint_t persadmin_list_subfolder(folder_path_s* psFolderPath, pstr_t buffer_out, sint_t bufSize, sint_t * posInBuffer, PersadminFilterMode_e eFilter, bool_t bRecursive, sint_t posInCompletePath, sint_t nestingLevel)
{
    bool_t bEverythingOK = true ;
    DIR *dp;
    struct dirent *ep;

    if(nestingLevel++ > PERSADMIN_MAX_NESTING_DEPTH)
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_subfolder(%s) too much nesting !!! ", psFolderPath->absPath + psFolderPath->startOfRelativePath) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        if(psFolderPath->absPath[posInCompletePath-1] != '/')
        {
            psFolderPath->absPath[posInCompletePath] = '/' ;
            posInCompletePath += 1 ;
            psFolderPath->absPath[posInCompletePath] = '\0' ;
        }
    }


    if(bEverythingOK)
    {
        dp = opendir(psFolderPath->absPath);
        if(NIL == dp)
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_subfolder: opendir(%s) errno=<%s>", psFolderPath->absPath, strerror(errno));/*DG C8ISQP-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }
    
    if(bEverythingOK)
    {
        ep = readdir (dp);
        while ((ep != NIL) && (bEverythingOK))
        {
            int cmp = strcmp(ep->d_name, "..");//for QAC
            if( (0 != strcmp(ep->d_name, ".")) && (0 != cmp))
            {
                sint_t err ;
                struct stat sb;
                sint_t len = (sint_t)strlen(ep->d_name) ;
                if(posInCompletePath + len < PERSADMIN_MAX_PATH_LENGHT)
                {
                    (void)strcpy(psFolderPath->absPath + posInCompletePath, ep->d_name) ;
                    err = lstat(psFolderPath->absPath, &sb) ;
                    if(err >= 0)
                    {
                        bool_t bListFile = persadmin_check_dir_entry_against_filter(&sb, eFilter) ;
                        
                        if(bListFile)
                        {
                            //printf("\n%s", psFolderPath->absPath + psFolderPath->startOfRelativePath);

                            /* list the files */
                            if(0 != bufSize)
                            {
                                /* other way, we are interested only about the size needed to accomodate the listing */

                                /* hardcoded value '1' is used below because paths are separated by '\0' */
                                if( ((posInCompletePath + len - psFolderPath->startOfRelativePath) + *posInBuffer) <= bufSize)
                                {
                                    (void)strcpy((buffer_out + (*posInBuffer)), psFolderPath->absPath + psFolderPath->startOfRelativePath + 1) ;
                                    *posInBuffer += (posInCompletePath + len - psFolderPath->startOfRelativePath)  ;
                                }
                                else
                                {
                                    bEverythingOK = false;
                                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_subfolder: buffer too small (already reached %d, available %d)!!", (*posInBuffer + len + 1), bufSize) ;
                                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                                }
                            }
                            else
                            {
                                *posInBuffer += (posInCompletePath + len - psFolderPath->startOfRelativePath)  ;
                            }
                        }
                        
                        if(bEverythingOK && (bRecursive && S_ISDIR(sb.st_mode)))
                        {
                            if(0 == persadmin_list_subfolder(psFolderPath, buffer_out, bufSize, posInBuffer, eFilter, bRecursive, posInCompletePath+len, nestingLevel))
                            {
                                psFolderPath->absPath[posInCompletePath] = '\0' ;
                            }
                            else
                            {
                                bEverythingOK = false;
                                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_subfolder: failed for (%s)", psFolderPath->absPath + psFolderPath->startOfRelativePath) ;
                                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                                psFolderPath->absPath[posInCompletePath] = '\0' ;
                            }
                        }
                    }
                }
                else
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_subfolder: path too long (%s)", psFolderPath->absPath) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }                
            }
            ep = readdir (dp);
        }
        (void)closedir (dp);
    }

    return (bEverythingOK ? 0 : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*//*DG C8ISQP-ISQP Metric 7-SSW_Administrator_0004*/


sint_t persadmin_list_folder_get_size(pconststr_t folderPath, PersadminFilterMode_e eListingMode, bool_t bRecursive)
{
    bool_t bEverythingOK = true ;
    str_t rootPath[PERSADMIN_MAX_PATH_LENGHT+1] ;
    str_t absPath[PERSADMIN_MAX_PATH_LENGHT+1] ;
    str_t folderPathLocal[PERSADMIN_MAX_PATH_LENGHT+1] ;
    sint_t listingSize = -1 ;
    
    folder_path_s sFolderPath;

    sFolderPath.rootPath = rootPath;
    sFolderPath.absPath  = absPath;
    sFolderPath.startOfRelativePath = 0;

    if(NIL == folderPath)
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder_get_size: invalid params") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        if(folderPath[0] != '/')
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder_get_size: not an absolute path (%s)", folderPath) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            /* make sure path ends with '/' */
            sint_t len = (sint_t)strlen(folderPath) ;
            if(len < (PERSADMIN_MAX_PATH_LENGHT-1)) /* to allow adding of ending '/' */
            {
                (void)strcpy(folderPathLocal, folderPath) ;
                if('/' != folderPathLocal[len-1])
                {
                    (void)strcat(folderPathLocal, "/") ;
                }
            }
            else
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder_get_size: path too long (%s)", folderPath) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
    }

    if(bEverythingOK)
    {
        sint_t len = (sint_t)strlen(folderPathLocal) ;
        if(len < PERSADMIN_MAX_PATH_LENGHT)
        {
            sint_t posInBuffer = 0 ;

            (void)strcpy(sFolderPath.rootPath, folderPathLocal) ;
            (void)strcpy(sFolderPath.absPath, folderPathLocal) ;
            sFolderPath.startOfRelativePath = len-1 ;

            if(0 == persadmin_list_subfolder(&sFolderPath, NIL, 0, &posInBuffer, eListingMode, bRecursive, len-1, 0))
            {
                listingSize = posInBuffer ;
            }
            else
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder_get_size: persadmin_list_subfolder(%s) failed", sFolderPath.rootPath) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder_get_size: path too long (%s)", folderPathLocal) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    return (bEverythingOK ? listingSize : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

sint_t persadmin_list_folder(pconststr_t folderPath, pstr_t buffer_out, sint_t bufSize, PersadminFilterMode_e eFilter, bool_t bRecursive)
{
    bool_t bEverythingOK = true ;
    str_t rootPath[PERSADMIN_MAX_PATH_LENGHT+1] ;
    str_t absPath[PERSADMIN_MAX_PATH_LENGHT+1] ;
    str_t folderPathLocal[PERSADMIN_MAX_PATH_LENGHT+1] ;
    sint_t listingSize = -1 ;

    folder_path_s sFolderPath;

    sFolderPath.rootPath = rootPath;
    sFolderPath.absPath  = absPath;
    sFolderPath.startOfRelativePath = 0;


    if( (NIL == folderPath) || (NIL == buffer_out) || (bufSize < 1))
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder: invalid params") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        if(folderPath[0] != '/')
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder: not an absolute path (%s)", folderPath) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            /* make sure path ends with '/' */
            sint_t len = (sint_t)strlen(folderPath) ;
            if(len < (PERSADMIN_MAX_PATH_LENGHT-1)) /* to allow adding of ending '/' */
            {
                (void)strcpy(folderPathLocal, folderPath) ;
                if('/' != folderPathLocal[len-1])
                {
                    (void)strcat(folderPathLocal, "/") ;
                }
            }
            else
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder: path too long (%s)", folderPath) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
    }
    
    if(bEverythingOK)
    {
        sint_t len = (sint_t)strlen(folderPathLocal) ;
        if(len < PERSADMIN_MAX_PATH_LENGHT)
        {
            sint_t posInBuffer = 0 ;

            (void)strcpy(sFolderPath.rootPath, folderPathLocal) ;
            (void)strcpy(sFolderPath.absPath, folderPathLocal) ;
            sFolderPath.startOfRelativePath = len-1 ;

            if(0 == persadmin_list_subfolder(&sFolderPath, buffer_out, bufSize, &posInBuffer, eFilter, bRecursive, len-1, 0))
            {
                listingSize = posInBuffer ;
            }
            else
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder: persadmin_list_subfolder(%s) failed", sFolderPath.rootPath) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_list_folder: path too long (%s)", folderPathLocal) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    return (bEverythingOK ? listingSize : PAS_FAILURE) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

sint_t persadmin_get_folder_size( pconststr_t folderPath )
{
    sint_t iFolderSize = 0 ;
    bool_t bEverythingOK = true ;
    str_t completePathFilename[PERSADMIN_MAX_PATH_LENGHT+1] ;
    pstr_t pFilesList = NIL ;
    sint_t iSizeOfFilesList = 0 ;

    /* check param */
    if(NIL == folderPath)
    {
        iFolderSize = PAS_FAILURE_INVALID_PARAMETER ;
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s: invalid params", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        /* check that folder path is valid */
        if(0 > persadmin_check_if_file_exists((pstr_t)folderPath, true))
        {
            iFolderSize = PAS_FAILURE_INVALID_PARAMETER ;
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s: Folder <<%s>> not found", __FUNCTION__, folderPath) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        /* get the size of the buffer needed to accommodate the list with all the files in the folder (recursive)
         * the list contains the (relative path + filename) for each file */
        iSizeOfFilesList = persadmin_list_folder_get_size(folderPath, PersadminFilterFilesRegular, true) ;
        if(iSizeOfFilesList > 0)
        {
        	 /* allocate the buffer where the list will be created */
             pFilesList = (pstr_t)malloc(iSizeOfFilesList);
             if(NIL == pFilesList)
             {
                 bEverythingOK = false ;
                 iFolderSize = PAS_FAILURE_OUT_OF_MEMORY ;
                 (void)snprintf(g_msg, sizeof(g_msg), "%s: malloc(%d) failed", __FUNCTION__, iSizeOfFilesList) ;
                 DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
             }
        }
        else
        {
            if(iSizeOfFilesList < 0)
            {
                bEverythingOK = false ;
                iFolderSize = iSizeOfFilesList ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s: persadmin_list_folder_get_size(%s) failed err=(%X)", __FUNCTION__, folderPath, iFolderSize) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            /* for iSizeOfFilesList==0, nothing to do;
             * iFolderSize = 0 will be returned */
        }
    }

    if(bEverythingOK && (iSizeOfFilesList > 0))
    {
        /* we have the buffer needed to accommodate the list with all the files in the folder (recursive)
         * now fill the buffer with the list contains the (relative path + filename) for each file */
        sint_t iSizeTemp = persadmin_list_folder(folderPath, pFilesList, iSizeOfFilesList, PersadminFilterFilesRegular, true) ;
        if(iSizeTemp != iSizeOfFilesList)
        {
            bEverythingOK = false ;
            iFolderSize = (iSizeTemp < 0) ? iSizeTemp : PAS_FAILURE ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s: persadmin_list_folder(%s) returned=(%d) - expected (%d)", __FUNCTION__, folderPath, iSizeTemp, iSizeOfFilesList) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK && (iSizeOfFilesList > 0))
    {
        /* pFilesList contains the relative paths of all the files
         * for each file:
         *     - build the complete(absolute) path
         *     - get size and add it to cumulative size
         */
        sint_t iPosInList = 0 ;
        sint_t iOffsetRelativePath ;

        (void)strncpy(completePathFilename, folderPath, sizeof(completePathFilename)-1);
        iOffsetRelativePath = strlen(completePathFilename) ;
        if('/' != completePathFilename[iOffsetRelativePath-1])
        {
            completePathFilename[iOffsetRelativePath] = '/' ;
            iOffsetRelativePath++ ;
        }

        while(bEverythingOK && (iPosInList < iSizeOfFilesList))
        {
            pstr_t pCurrentRelativePathFilename = pFilesList + iPosInList ;
            sint_t iSizeOfCurrentRelativePathFilename = strlen(pCurrentRelativePathFilename) ;
            sint_t iSizeOfCurrentFile ;

            (void)strncpy(&completePathFilename[iOffsetRelativePath], pCurrentRelativePathFilename, (sizeof(completePathFilename) - iOffsetRelativePath)) ;
            iSizeOfCurrentFile = persadmin_priv_get_file_size(completePathFilename) ;
            if(iSizeOfCurrentFile >= 0)
            {
                iFolderSize += iSizeOfCurrentFile ;
            }
            else
            {
                bEverythingOK = false ;
                iFolderSize = PAS_FAILURE ;
                break ;
            }

            iPosInList += (iSizeOfCurrentRelativePathFilename + 1) ; /* 1 <=> '\0' */
        }
    }

    if(NIL != pFilesList)
    {
        free(pFilesList);
    }

    return iFolderSize ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

static sint_t persadmin_priv_get_file_size( pconststr_t absPathFileName )
{
    sint_t iFileSize = 0;

    struct stat fileInfo;

    if ( stat( absPathFileName, & fileInfo ) == 0 )
    {
        iFileSize = (sint_t)fileInfo.st_size;//TODO: see if uint_t is enough ->st_size is long
    }
    else
    {
        iFileSize = PAS_FAILURE ;
    }

    return iFileSize;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


bool_t persadmin_check_for_same_file_content(pstr_t file1Path, pstr_t file2Path)
{
    uint64_t     file1Size    = 0;
    uint64_t     file2Size    = 0;
    uint8_t      *file1Buff    = NULL;
    uint8_t      *file2Buff    = NULL;
    struct stat  fileInfo;

    int            handleFile1 = -1;    // invalid file handle
    int            handleFile2 = -1;    // invalid file handle

    if((NIL == file1Path) && (NIL == file2Path))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("Invalid parameters in persadmin_check_for_same_file_content call."));
        return false;
    }

    /* check if the files have the same sizes */
    (void)memset(&fileInfo, 0, sizeof(fileInfo));
    if ( stat( file1Path, & fileInfo ) == 0 )
    {
        file1Size = (uint64_t)fileInfo.st_size;

        (void)memset(&fileInfo, 0, sizeof(fileInfo));
        if ( stat( file2Path, & fileInfo ) == 0 )
        {
            file2Size = (uint64_t)fileInfo.st_size;
        }
        else
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("Failed getting file1 size in persadmin_check_for_same_file_content call."));
            return false;
        }
    }
    else
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("Failed getting file2 size in persadmin_check_for_same_file_content call."));
        return false;
    }

    if(file1Size != file2Size)
    {
        return false;
    }

    /* check if the file content matches */
    handleFile1 = open (file1Path, O_RDONLY);
    if(handleFile1 < 0)    // invalid file handle
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),     DLT_STRING("Failed opening file "),
                                                                        DLT_STRING(file1Path));
        return false;
    }
    else
    {
        handleFile2 = open (file2Path, O_RDONLY);
        if(handleFile2 < 0)    // invalid file handle
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),     DLT_STRING("Failed opening file "),
                                                                            DLT_STRING(file2Path));
            (void)close (handleFile1);
            return false;
        }
    }

    /* map file1 to memory */
    file1Buff = mmap(    NIL,
                        (size_t)file1Size,
                        PROT_READ,
                        MAP_PRIVATE,
                        handleFile1,
                        0);

    if(MAP_FAILED == file1Buff)
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),     DLT_STRING("Failed mapping file "),
                                                                        DLT_STRING(file1Path));
        (void)close (handleFile1);
        (void)close (handleFile2);
        return false;
    }
    else
    {
        /* map file2 to memory */
        file2Buff = mmap(    NIL,
                            (size_t)file2Size,
                            PROT_READ,
                            MAP_PRIVATE,
                            handleFile2,
                            0);

        if(MAP_FAILED == file2Buff)
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),     DLT_STRING("Failed mapping file "),
                                                                            DLT_STRING(file2Path));
            (void)munmap (file1Buff, (size_t)file1Size);
            (void)close (handleFile1);
            (void)close (handleFile2);
            return false;
        }
    }

    /* compare file contents in memory */
    if(0 != memcmp(file1Buff, file2Buff, (size_t)file1Size))
    {
        (void)munmap (file1Buff, (size_t)file1Size);
        (void)munmap (file2Buff, (size_t)file2Size);

        (void)close (handleFile1);
        (void)close (handleFile2);
        return false;
    }

    (void)munmap (file1Buff, (size_t)file1Size);
    (void)munmap (file2Buff, (size_t)file2Size);

    (void)close (handleFile1);
    (void)close (handleFile2);

    return true;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/
