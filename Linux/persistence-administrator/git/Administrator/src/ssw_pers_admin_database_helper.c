/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ana.Chisca@continental-corporation.com
*
* Implementation of functions declared in ssw_pers_admin_database_helper.h
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.01.08 uidn3591           CSP_WZ#9979:  Cleaning & fix delete and copy keys by filter to conform to requirements
* 2013.01.03 uidl9757           CSP_WZ#2060:  Common interface to be used by both PCL and PAS
* 2012.12.04 uidn3591           CSP_WZ#1280:  Bug fix
* 2012.11.22 uidn3591           CSP_WZ#1280:  Minor bug fix
* 2012.11.19 uidn3591           CSP_WZ#1280:  Creation
*
**********************************************************************************************************************/ 
 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "persComTypes.h"
#include "dlt.h"                                                /* L&T */
#include "ssw_pers_admin_files_helper.h"                        /* defined interface */

#include "persComDataOrg.h"
#include "persComDbAccess.h"
#include "persComRct.h"

#include "ssw_pers_admin_database_helper.h"                     /* defined interface */
#include "ssw_pers_admin_common.h"
#include "persistence_admin_service.h"

/* L&T context */
#define LT_HDR                  "DB >> "
DLT_IMPORT_CONTEXT              (persAdminSvcDLTCtx)

#define PAS_USER_IDENTIFIER     PERS_ORG_USER_FOLDER_NAME
#define PAS_SEAT_IDENTIFIER     PERS_ORG_SEAT_FOLDER_NAME
#define PAS_SHARED_IDENTIFIER   PERS_ORG_SHARED_FOLDER_NAME


#define PrintDbPath( Root, DataPath, ... )                                    \
  if ( bufSize > 0 ) {                                                        \
    int toWrite = snprintf( pchDBPaths_out, bufSize, "%s", Root );            \
    bufSize -= toWrite;                                                       \
    pchDBPaths_out += toWrite;                                                \
    totalAmountWritten += toWrite;                                            \
  }                                                                           \
  if ( bufSize > 0 ) {                                                        \
    int toWrite = snprintf( pchDBPaths_out, bufSize, DataPath, __VA_ARGS__ ); \
    totalAmountWritten += toWrite += sizeof( StringTerminator );              \
    bufSize -= toWrite;                                                       \
    pchDBPaths_out += toWrite;                                                \
  }

static int                  persadmin_priv_copy_keys_by_filter            (PersASSelectionType_e type, char* pchdestDBPath, char* pchsourceDBPath, unsigned int user_no, unsigned int seat_no);
static int                  persadmin_priv_delete_keys_by_filter          (PersASSelectionType_e type, char* pchDBPath, unsigned int user_no, unsigned int seat_no);
static sint_t               persadmin_priv_filter_keys_in_list            (PersASSelectionType_e type, unsigned int user_no, unsigned int seat_no, pstr_t pUnfilteredList, pstr_t pFilteredList_out, sint_t iListSize) ;

static int                  persadmin_priv_get_all_local_db_names         ( const char * pchRootPath, const char * pchAppName, char* pchDBPaths_out, int bufSize );
static int                  persadmin_priv_get_all_shared_db_names        ( const char * pchRootPath, char* pchDBPaths_out, int bufSize );
static int                  persadmin_priv_get_all_shared_group_db_names  ( const char * pchRootPath, const char * pchDataPathFormat, const char * dbName, const char * defaultDbName, char* pchDBPaths_out, int * bufferSize );

/**
 * @brief get the names of an application's databases based on the databases' type (local/shared)
 * @usage:
 *
 * @param pchDBRootPath     [in]    path to the folder where to look for the databases
 * @param pchAppName        [in]    application for which the databases are requested
 * @param eStorage          [in]    the type of the requested databases
 * @param pchDBPaths_out    [out]   pointer to a buffer where to store the databases' names [the databases' names are separated by '\0']
 * @param bufSize           [in]    size of buffer
 *
 * @return positive value: size of data passed in pchDBPaths_out ; negative value - error
 * TODO: define error codes
 */
sint_t persadmin_get_all_db_paths_with_names(pconststr_t pchDBRootPath, pconststr_t pchAppName, PersistenceStorage_e eStorage, pstr_t pchDBPaths_out, sint_t bufSize)
{
    sint_t result = PAS_FAILURE ;

    if ( ( pchDBRootPath != NIL ) && ( ( pchAppName != NIL ) || ( eStorage != PersistenceStorage_local ) ) && ( pchDBPaths_out != NIL ) && ( bufSize >= sizeof( StringTerminator ) ) && ( eStorage < PersistenceStorage_LastEntry ) )
    {
        int writtenAmount = 0;

        switch ( eStorage ) 
        {
            case PersistenceStorage_local : 
            {
                result = persadmin_priv_get_all_local_db_names( pchDBRootPath, pchAppName, pchDBPaths_out, bufSize );
                break;
            }
            case PersistenceStorage_shared : 
            {
                result = persadmin_priv_get_all_shared_db_names( pchDBRootPath, pchDBPaths_out, bufSize );
                break;
            }
            default : 
            {
                result = PAS_FAILURE_INVALID_PARAMETER;
                break;
            }
        }

        if ( result >= 0 ) 
        {
            writtenAmount += result;
            bufSize -= result;
            if ( bufSize > 0 ) 
            {
                memmove( pchDBPaths_out + writtenAmount, & StringTerminator, sizeof( StringTerminator ) );
                result = writtenAmount + sizeof( StringTerminator );
            }
            else 
            {
                result = PAS_FAILURE_BUFFER_TOO_SMALL ;
            }
        }
    }

    return result;
} /* persadmin_get_all_db_paths_with_names() */


/**
 * @brief deletes keys filtered by user name and seat number
 *
 * @param pchDBPath[in] path to where the DB can be located
 * @param user_no  [in] the user ID
 * @param seat_no  [in] the seat number (seat 1 to 4; 0xFF is for all seats)
 *
 * @return 0 for success, negative value otherwise;
 * TODO: define error codes
 */
int persadmin_delete_keys_by_filter(PersASSelectionType_e type, char* pchDBPath, unsigned int user_no, unsigned int seat_no)
{
    sint_t s32Result = PAS_SUCCESS;

    if( NIL == pchDBPath )
    {
        // set error;
        s32Result = PAS_FAILURE_INVALID_PARAMETER;
        // some info;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_delete_keys_by_filter -"), DLT_STRING("invalid input parameters"));
    }

    if( PAS_SUCCESS == s32Result )
    {
        // some info;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("persadmin_delete_keys_by_filter -"), DLT_INT(user_no), DLT_INT(seat_no), DLT_STRING(pchDBPath));

        // select what keys to delete from the database;
        s32Result = persadmin_priv_delete_keys_by_filter(type, pchDBPath, user_no, seat_no);
    }

    // return result;
    return s32Result;

} /* persadmin_delete_keys_by_filter() */


/**
 * @brief copies keys filtered by user name and seat number
 *
 * @param pchdestDBPath  [in] destination DB path
 * @param pchsourceDBPath[in] source DB path
 * @param user_no        [in] the user ID
 * @param seat_no        [in] the seat number (seat 1 to 4; 0xFF is for all seats)
 *
 * @return 0 for success, negative value otherwise;
 * TODO: define error codes
 */
int persadmin_copy_keys_by_filter(PersASSelectionType_e type, char* pchdestDBPath, char* pchsourceDBPath, unsigned int user_no, unsigned int seat_no)
{
    sint_t s32Result = PAS_SUCCESS;

    if( (NIL == pchsourceDBPath) || (NIL == pchdestDBPath) || (PersASSelectionType_All > type ) || (PersASSelectionType_LastEntry < type ))
    {
        // set error;
        s32Result = PAS_FAILURE_INVALID_PARAMETER;
        // some info;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_copy_keys_by_filter -"),
                DLT_STRING("invalid input parameters"));
    }

    if( PAS_SUCCESS == s32Result )
    {
        // some info;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("persadmin_copy_keys_by_filter -"),
                                DLT_INT(user_no), DLT_INT(seat_no),
                                DLT_STRING("from"),
                                DLT_STRING("\""), DLT_STRING(pchsourceDBPath), DLT_STRING("\" "),
                                DLT_STRING("to"),
                                DLT_STRING("\""), DLT_STRING(pchdestDBPath),   DLT_STRING("\" "));

        if( 0 <= persadmin_check_if_file_exists(pchsourceDBPath, false) )
        {
            // select what keys to copy from one database to another;
            s32Result = persadmin_priv_copy_keys_by_filter(type, pchdestDBPath, pchsourceDBPath, user_no, seat_no);
        }
    }

    // return result;
    return s32Result;

} /* persadmin_copy_keys_by_filter() */


static int persadmin_priv_copy_keys_by_filter(PersASSelectionType_e type, char* pchdestDBPath, char* pchsourceDBPath, unsigned int user_no, unsigned int seat_no)
{
    sint_t  s32Result = PAS_FAILURE ;

    bool_t  bEverythingOK = true ;
    pstr_t  pSrcUnfilteredList = NIL ;
    pstr_t  pSrcListFiltered = NIL ;
    sint_t  iSrcListUnfilteredSize = 0 ;
    sint_t  iSrcListFilteredSize = 0 ;
    
    sint_t  iHandlerSrcDB = -1 ;
    sint_t  iHandlerDestDB = -1 ;

    /* get a list of the keys in pchsourceDBPath */
    iHandlerSrcDB = persComDbOpen(pchsourceDBPath, false) ;
    if(iHandlerSrcDB >= 0)
    {
        iSrcListUnfilteredSize = persComDbGetSizeKeysList(iHandlerSrcDB) ;
        if(iSrcListUnfilteredSize > 0)
        {
            pSrcUnfilteredList = (pstr_t)malloc(iSrcListUnfilteredSize) ;
            pSrcListFiltered = (pstr_t)malloc(iSrcListUnfilteredSize) ;
            if((NIL != pSrcUnfilteredList) && (NIL != pSrcListFiltered))
            {
                if(iSrcListUnfilteredSize != persComDbGetKeysList(iHandlerSrcDB, pSrcUnfilteredList, iSrcListUnfilteredSize))
                {
                    bEverythingOK = false ;
                }
            }
            else
            {
                bEverythingOK = false ;
                s32Result = PAS_FAILURE_OUT_OF_MEMORY ;
            }
        }
        else
        {
            if(iSrcListUnfilteredSize < 0)
            {
                bEverythingOK = false ;
            }
        }
    }
    else
    {
        bEverythingOK = false ;
        s32Result = iHandlerSrcDB ;
    }

    if(bEverythingOK && (iSrcListUnfilteredSize > 0))
    {
        iHandlerDestDB = persComDbOpen(pchdestDBPath, true) ;
        if(iHandlerDestDB < 0)
        {
            bEverythingOK = false ;
            s32Result = iHandlerDestDB ;
        }
    }

    if(bEverythingOK && (iSrcListUnfilteredSize > 0))
    {
        iSrcListFilteredSize = persadmin_priv_filter_keys_in_list(type, user_no, seat_no, pSrcUnfilteredList, pSrcListFiltered, iSrcListUnfilteredSize) ;
    }

    if(bEverythingOK && (iSrcListFilteredSize > 0))
    {
        sint_t iPosInList = 0 ;
        pstr_t pCurrentKeyName = NIL ;

        while(bEverythingOK && (iPosInList < iSrcListFilteredSize))        
        {
            pCurrentKeyName = pSrcListFiltered + iPosInList ;
            
            str_t keyData[PERS_DB_MAX_SIZE_KEY_DATA] ;
            sint_t iDataSize = persComDbReadKey(iHandlerSrcDB, pCurrentKeyName, keyData, sizeof(keyData)) ;
            if(iDataSize >= 0)
            {
                sint_t iError = persComDbWriteKey(iHandlerDestDB, pCurrentKeyName, keyData, iDataSize) ;
                if(iError < 0)
                {
                    bEverythingOK = false ;
                    s32Result = iError ;
                }

                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR),
                        DLT_STRING("persadmin_priv_copy_keys_by_filter - persComDbWriteKey"),
                        DLT_STRING(pCurrentKeyName),
                        DLT_STRING("size = "), DLT_INT(iDataSize),
                        DLT_STRING((iError >= 0) ? "OK" : "FAILED")) ;
            }


            iPosInList += strlen(pCurrentKeyName) + 1 ; /* 1 <=> list items separator */
        }
    }

    if(pSrcUnfilteredList != NIL)
    {
        free(pSrcUnfilteredList) ;
    }
    if(pSrcListFiltered!= NIL)
    {
        free(pSrcListFiltered) ;
    }
    if(iHandlerSrcDB >= 0)
    {
        persComDbClose(iHandlerSrcDB) ;
    }
    if(iHandlerDestDB >= 0)
    {
        persComDbClose(iHandlerDestDB) ;
    }

    return bEverythingOK ? PAS_SUCCESS : s32Result ;
} /* persadmin_priv_copy_keys_by_filter() */


static int persadmin_priv_delete_keys_by_filter(PersASSelectionType_e type, char* pchDBPath, unsigned int user_no, unsigned int seat_no)
{
    sint_t  s32Result = PAS_FAILURE ;

    bool_t  bEverythingOK = true ;
    pstr_t  pListUnfiltered = NIL ;
    sint_t  iListUnfilteredSize = 0 ;
    pstr_t  pListFiltered = NIL ;
    sint_t  iListFilteredSize = 0 ;
    
    sint_t  iHandlerDB = -1 ;

    /* get a list of the keys in pchsourceDBPath */
    iHandlerDB = persComDbOpen(pchDBPath, false) ;
    if(iHandlerDB >= 0)
    {
        iListUnfilteredSize = persComDbGetSizeKeysList(iHandlerDB) ;
        if(iListUnfilteredSize > 0)
        {
            pListUnfiltered = (pstr_t)malloc(iListUnfilteredSize) ;
            pListFiltered = (pstr_t)malloc(iListUnfilteredSize) ;
            if((NIL != pListUnfiltered) && (NIL != pListFiltered))
            {
                if(iListUnfilteredSize != persComDbGetKeysList(iHandlerDB, pListUnfiltered, iListUnfilteredSize))
                {
                    bEverythingOK = false ;
                }
            }
            else
            {
                bEverythingOK = false ;
                s32Result = PAS_FAILURE_OUT_OF_MEMORY ;
            }
        }
        else
        {
            if(iListUnfilteredSize < 0)
            {
                bEverythingOK = false ;
            }
        }
    }
    else
    {
        bEverythingOK = false ;
        s32Result = iHandlerDB ;
    }

    if(bEverythingOK && (iListUnfilteredSize > 0))
    {
        iListFilteredSize = persadmin_priv_filter_keys_in_list(type, user_no, seat_no, pListUnfiltered, pListFiltered, iListUnfilteredSize) ;
    }

    if(bEverythingOK && (iListFilteredSize > 0))
    {
        sint_t iPosInList = 0 ;
        pstr_t pCurrentKeyName = NIL ;

        while(bEverythingOK && (iPosInList < iListFilteredSize))        
        {
            pCurrentKeyName = pListFiltered + iPosInList ;
            
            sint_t iError = persComDbDeleteKey(iHandlerDB, pCurrentKeyName) ;
            if(iError < 0)
            {
                bEverythingOK = false ;
            }

            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR),
                    DLT_STRING("persadmin_priv_delete_keys_by_filter - persComDbDeleteKey"), DLT_STRING(pCurrentKeyName), DLT_STRING((iError >= 0) ? "OK" : "FAILED")) ;

            iPosInList += strlen(pCurrentKeyName) + 1 ; /* 1 <=> list items separator */
        }
    }


    if(pListUnfiltered != NIL)
    {
        free(pListUnfiltered) ;
    }
    if(pListFiltered!= NIL)
    {
        free(pListFiltered) ;
    }
    if(iHandlerDB >= 0)
    {
        persComDbClose(iHandlerDB) ;
    }

    return bEverythingOK ? PAS_SUCCESS : s32Result ;

} /* persadmin_priv_delete_keys_by_filter() */


static sint_t persadmin_priv_filter_keys_in_list(PersASSelectionType_e type, unsigned int user_no, unsigned int seat_no, pstr_t pUnfilteredList, pstr_t pFilteredList_out, sint_t iListSize)
{
    str_t   prefixToSearch[64] ; /* enough for something like "/user/1/seat/2/" */
    sint_t  iPrefixLength ;
    sint_t  iPosInUnfilteredList = 0 ;
    sint_t  iPosInFilteredList = 0 ;

    if( PERSIST_SELECT_ALL_USERS != user_no )
    {
        if( PERSIST_SELECT_ALL_SEATS != seat_no )
        {
            // "/user/1/seat/4";
            snprintf(prefixToSearch, sizeof(prefixToSearch), "/%s/%d/%s/%d/",
                    PERS_ORG_USER_FOLDER_NAME, user_no, PERS_ORG_SEAT_FOLDER_NAME, seat_no);
        }
        else
        {
            // "/user/1/";
            snprintf(prefixToSearch, sizeof(prefixToSearch), "/%s/%d/",
                    PERS_ORG_USER_FOLDER_NAME, user_no);
        }
    }
    else
    {
        switch( type )
        {
            case PersASSelectionType_All:
            case PersASSelectionType_Application:
            {
                // let everything pass through (actually we should look for "/node/" or "/user/");
                snprintf(prefixToSearch, sizeof(prefixToSearch), "/");
                break;
            }

            case PersASSelectionType_User:
            {
                // "/user/";
                snprintf(prefixToSearch, sizeof(prefixToSearch), "/%s/",
                        PERS_ORG_USER_FOLDER_NAME);
                break;
            }

            default:
            {
                /* nothing to do */
                break;
            }
        }

    }

    iPrefixLength = strlen(prefixToSearch);
    while( iPosInUnfilteredList < iListSize )
    {
        pstr_t pCurrentKeyName = pUnfilteredList + iPosInUnfilteredList;
        sint_t iKeyLength      = strlen(pCurrentKeyName) ;

        // check if pCurrentKeyName begins with the searched prefix;
        if( 0 == strncmp(pCurrentKeyName, prefixToSearch, iPrefixLength) )
        {
            strcpy(pFilteredList_out + iPosInFilteredList, pCurrentKeyName);
            iPosInFilteredList += (iKeyLength + 1); /* 1 <=> list items separator */
        }

        iPosInUnfilteredList += (iKeyLength + 1); /* 1 <=> list items separator */
    }

    return iPosInFilteredList;

} /* persadmin_priv_filter_keys_in_list() */


static int persadmin_priv_get_all_local_db_names( const char * pchRootPath, const char * pchAppName, char* pchDBPaths_out, int bufSize ) 
{
    int result = PAS_FAILURE, totalAmountWritten = 0;

    PrintDbPath( pchRootPath, gLocalCachePath, pchAppName, gLocalCached );
    PrintDbPath( pchRootPath, gLocalCachePath, pchAppName, gLocalCachedDefault );
    PrintDbPath( pchRootPath, gLocalCachePath, pchAppName, gLocalWt );
    PrintDbPath( pchRootPath, gLocalCachePath, pchAppName, gLocalWtDefault );

    PrintDbPath( pchRootPath, gLocalWtPath, pchAppName, gLocalWt );
    PrintDbPath( pchRootPath, gLocalWtPath, pchAppName, gLocalWtDefault );
    PrintDbPath( pchRootPath, gLocalWtPath, pchAppName, gLocalCached );
    PrintDbPath( pchRootPath, gLocalWtPath, pchAppName, gLocalCachedDefault );

    if ( bufSize >= 0 ) 
    {
        result = totalAmountWritten;
    }

    return result;
} /* persadmin_priv_get_all_local_db_names() */


static int persadmin_priv_get_all_shared_db_names( const char * pchRootPath, char* pchDBPaths_out, int bufSize ) 
{
    int result = PAS_FAILURE, totalAmountWritten = 0;

    PrintDbPath( pchRootPath, gSharedPublicCachePath, gSharedCached );
    PrintDbPath( pchRootPath, gSharedPublicCachePath, gSharedCachedDefault );
    PrintDbPath( pchRootPath, gSharedPublicCachePath, gSharedWt );
    PrintDbPath( pchRootPath, gSharedPublicCachePath, gSharedWtDefault );

    PrintDbPath( pchRootPath, gSharedPublicWtPath, gSharedWt );
    PrintDbPath( pchRootPath, gSharedPublicWtPath, gSharedWtDefault );
    PrintDbPath( pchRootPath, gSharedPublicWtPath, gSharedCached );
    PrintDbPath( pchRootPath, gSharedPublicWtPath, gSharedCachedDefault );

    if ( bufSize > 0 ) 
    {
        result = persadmin_priv_get_all_shared_group_db_names( pchRootPath, gSharedCachePathString, gSharedCached, gSharedCachedDefault, pchDBPaths_out, & bufSize );
        if ( result >= 0 ) 
        {
            totalAmountWritten += result;
            pchDBPaths_out += result;
        }
    }
    if ( bufSize > 0 ) 
    {
        result = persadmin_priv_get_all_shared_group_db_names( pchRootPath, gSharedWtPathString, gSharedWt, gSharedWtDefault, pchDBPaths_out, & bufSize );
        if ( result >= 0 ) 
        {
            totalAmountWritten += result;
            pchDBPaths_out += result;
        }
    }
    if ( bufSize > 0 )
    {
        result = persadmin_priv_get_all_shared_group_db_names( pchRootPath, gSharedCachePathString, gSharedWt, gSharedWtDefault, pchDBPaths_out, & bufSize );
        if ( result >= 0 )
        {
            totalAmountWritten += result;
            pchDBPaths_out += result;
        }
    }
    if ( bufSize > 0 )
    {
        result = persadmin_priv_get_all_shared_group_db_names( pchRootPath, gSharedWtPathString, gSharedCached, gSharedCachedDefault, pchDBPaths_out, & bufSize );
        if ( result >= 0 )
        {
            totalAmountWritten += result;
            pchDBPaths_out += result;
        }
    }

    if ( bufSize >= 0 ) 
    {
        result = totalAmountWritten;
    }

    return result;
} /* persadmin_priv_get_all_shared_db_names() */


static int persadmin_priv_get_all_shared_group_db_names( const char * pchRootPath, const char * pchDataPathFormat, const char * dbName, const char * defaultDbName, char* pchDBPaths_out, int * bufferSize ) 
{
    int result = PAS_FAILURE, totalAmountWritten = 0;

    int rootPathLen = strlen( pchRootPath );
    char sharedPath[ rootPathLen + sizeof( PathSeparator ) + strlen( pchDataPathFormat ) + sizeof( StringTerminator ) ];
    snprintf( sharedPath, sizeof( sharedPath ), "%s%c", pchRootPath, PathSeparator );
    snprintf( sharedPath + rootPathLen + sizeof( PathSeparator ), sizeof( sharedPath ) - rootPathLen - sizeof( PathSeparator ), pchDataPathFormat, "", "" );

    result = persadmin_list_folder_get_size( sharedPath, PersadminFilterFolders, false );
    if ( result > 0 ) 
    {
        char fileList[ result + sizeof( StringTerminator ) ];
        memset( fileList, 0, sizeof( fileList ) );
        result = persadmin_list_folder( sharedPath, fileList, result, PersadminFilterFolders, false );

        if ( result > 0 ) 
        {
            char * crtPath = fileList;
            int bufSize = ( * bufferSize );
            while ( ( bufSize >= 0 ) && ( ( * crtPath ) != StringTerminator ) ) 
            {
                PrintDbPath( pchRootPath, pchDataPathFormat, crtPath, dbName );
                PrintDbPath( pchRootPath, pchDataPathFormat, crtPath, defaultDbName );
                crtPath += strlen( crtPath ) + sizeof( StringTerminator );
            }
            ( * bufferSize ) = bufSize;

            if ( bufSize >= 0 ) 
            {
                result = totalAmountWritten;
            }
        }
    }

    return result;
} /* persadmin_priv_get_all_shared_group_db_names() */
