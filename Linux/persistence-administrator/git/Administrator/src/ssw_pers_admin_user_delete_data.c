/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Alin.Liteanu@continental-corporation.com
*
* Implementation of delete user data process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2012.12.22 uidv2833	        CSP_WZ#1280:  Corrected value if directory is missing
* 2012.12.21 uidv2833           CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/

#include "persComTypes.h"
#include "string.h"
#include "stdio.h"
#include <stdlib.h>
#include "ssw_pers_admin_files_helper.h"
#include "ssw_pers_admin_database_helper.h"
#include "persComDataOrg.h"
#include "persistence_admin_service.h"

#define MNT_C_PATH      PERS_ORG_LOCAL_APP_CACHE_PATH_	// "/Data/mnt-c/"
#define MNT_WT_PATH     PERS_ORG_LOCAL_APP_WT_PATH_		// "/Data/mnt-wt/"

//#define MNT_MAX_LENGTH  15
#define SHARED_FOLDER   PERS_ORG_SHARED_FOLDER_NAME		//"shared"
#define USER_FOLDER     PERS_ORG_USER_FOLDER_NAME		//"user"
#define SEAT_FOLDER     PERS_ORG_SEAT_FOLDER_NAME		//"seat"
#define PUBLIC_FOLDER   PERS_ORG_PUBLIC_FOLDER_NAME		//"public"
#define GROUP_FOLDER    PERS_ORG_GROUP_FOLDER_NAME		//"group"
#define CACHED_DATABASE PERS_ORG_SHARED_CACHE_DB_NAME	//"cached.itz"
#define WT_DATABASE     PERS_ORG_SHARED_WT_DB_NAME		//"wt.itz"

long user_data_delete_from_folder (pstr_t folder_path, unsigned int user_no, unsigned int seat_no)
{
    bool_t bEverythingOK = true;
    sint_t iBytesDeleted = 0;                         // return value
    sint_t iLocalBytesDeleted;
    pstr_t sFoldersAndFiles = NIL ;                  // list of the folders and files found in current folder path
    sint_t iFoldersOrFilesBufSize;                    // size of the buffer containing the list of the folders and files found in current path
    str_t  sFolderOrFile[PERSADMIN_MAX_PATH_LENGHT];  // contains current path subfolder or file
    str_t  sAbsPath[PERSADMIN_MAX_PATH_LENGHT];        // absolute folder or file path that will be deleted
    str_t  seat_no_path[PERSADMIN_MAX_PATH_LENGHT];
    pstr_t sSeatFolders = NIL;
    sint_t iSeatFoldersBufSize;
    str_t  sBufferForSeat[PERSADMIN_MAX_PATH_LENGHT];

    sint_t posInListBuffer;
    sint_t posInListBufferForSeat;
    sint_t len;
    sint_t lenSeat;
    
    printf("user_data_delete_from_folder: folder: %s\n", folder_path);                            

    //get requierd buffer length
    iFoldersOrFilesBufSize = persadmin_list_folder_get_size( folder_path, PersadminFilterAll, false );
    
    printf("user_data_delete_from_folder: persadmin_list_folder_get_size returned : %d\n", iFoldersOrFilesBufSize);                            
    
    if ( 0 <= iFoldersOrFilesBufSize )
    {   
        //alocate required memory
        sFoldersAndFiles = ( pstr_t )malloc( iFoldersOrFilesBufSize );
        if ( NIL == sFoldersAndFiles )
        {
            bEverythingOK = false;
            printf("user_data_delete_from_folder: malloc failed\n");
        }
        else
        {
            printf("user_data_delete_from_folder: malloc successful\n");
        }

        if ( bEverythingOK )
        {
            // get the files and folders inside the curent folder and store them in the sFoldersAndFiles buffer
            if ( iFoldersOrFilesBufSize != persadmin_list_folder( folder_path, sFoldersAndFiles, iFoldersOrFilesBufSize, PersadminFilterAll, false) )
            {
                bEverythingOK = false;
                printf("user_data_delete_from_folder: persadmin_list_folder failed\n");
            }
            else
            {
                printf("user_data_delete_from_folder: persadmin_list_folder successful\n");
            }
        }
        
        if ( bEverythingOK )
        {
            posInListBuffer = 0;
            len = 0;
            while ( ( posInListBuffer < iFoldersOrFilesBufSize ) && ( bEverythingOK ) )
            {
                strcpy(sFolderOrFile, sFoldersAndFiles + posInListBuffer);
                len = strlen(sFolderOrFile) + 1;

                printf("user_data_delete_from_folder: entity: %s\n", sFolderOrFile);
                
                // Folder - check if name = User
                if ( 0 == strcmp( USER_FOLDER, sFolderOrFile ) )
                {
                    // delete user data in .../user/user_no/seat/seat_no/
                    // seat_no_path = .../user/user_no/seat/seat_no/
                    sprintf( seat_no_path, "%s%s/%d/%s/%d/", folder_path, sFolderOrFile, user_no, SEAT_FOLDER, seat_no);
                    
                    if ( 0 == persadmin_check_if_file_exists(seat_no_path, true) )
                    {                   
                        printf("user_data_delete_from_folder: persadmin_list_folder_get_size: %s\n", seat_no_path);                    
                        
                        iSeatFoldersBufSize = persadmin_list_folder_get_size( seat_no_path, PersadminFilterAll, false );
                        
                        printf("user_data_delete_from_folder: persadmin_list_folder_get_size returned : %d\n", iSeatFoldersBufSize);                    
                        
                        if ( 0 > iSeatFoldersBufSize )
                        {
                            bEverythingOK = false;
                        }

                        if ( bEverythingOK )
                        {
                            //alocate required memory
                            sSeatFolders = ( pstr_t )malloc( iSeatFoldersBufSize );
                            if ( NIL == sSeatFolders )
                            {
                                bEverythingOK = false;
                                printf("user_data_delete_from_folder: malloc failed\n");                    
                            }
                            else
                            {
                                printf("user_data_delete_from_folder: malloc successful\n");                    
                            }
                        }
                        
                        if ( bEverythingOK )
                        {
                            // get the files and folders inside the Seat_no folder and store them in the sSeatFolders buffer
                            if ( 0 > persadmin_list_folder( seat_no_path, sSeatFolders, iSeatFoldersBufSize, PersadminFilterAll, false) )
                            {
                                bEverythingOK = false;
                                printf("user_data_delete_from_folder: persadmin_list_folder failed\n");                    
                            }
                            else
                            {
                                printf("user_data_delete_from_folder: persadmin_list_folder successful\n");                    
                            }
                        }

                        if ( bEverythingOK )
                        {
                            posInListBufferForSeat = 0 ;
                            lenSeat = 0;
                            while ( ( posInListBufferForSeat < iSeatFoldersBufSize ) && ( bEverythingOK ) )
                            {
                                strcpy( sBufferForSeat, sSeatFolders + posInListBufferForSeat);
                                lenSeat = strlen( sBufferForSeat ) + 1;
                                
                                // delete user data in Group subfolder
                                // sAbsPath = .../user/user_no/seat/seat_no
                                sprintf( sAbsPath, "%s%s", seat_no_path, sBufferForSeat);
                                
                                printf("user_data_delete_from_folder: User entity: %s\n", sBufferForSeat);
                                
                                if ( 0 == persadmin_check_if_file_exists(sAbsPath, true) )
                                {	
                                    //sAbsPath is a valid folder path
                                    printf("user_data_delete_from_folder: persadmin_delete_folder: %s\n", sAbsPath);
                                    iLocalBytesDeleted = persadmin_delete_folder(sAbsPath);
                                    if ( 0 <= iLocalBytesDeleted )
                                    {
                                        iBytesDeleted += iLocalBytesDeleted;
                                        printf("\nDeleted folder <%s> total size in bytes <%d>\n", sAbsPath, iLocalBytesDeleted);
                                    }
                                    else
                                    {
                                        printf("\nuser_data_delete_from_folder: persadmin_delete_folder(%s) failed\n", sAbsPath) ;
                                    }
                                }
                                else
                                {
                                    //sAbspath is a file                                    
                                    printf("user_data_delete_from_folder: persadmin_delete_file: %s\n", sAbsPath);
                                    iLocalBytesDeleted =  persadmin_delete_file(sAbsPath);
                                    if ( 0 <= iLocalBytesDeleted )
                                    {
                                        iBytesDeleted += iLocalBytesDeleted;
                                        printf("\nDeleted file <%s> total size in bytes <%d>\n", sAbsPath, iLocalBytesDeleted);
                                    }
                                    else
                                    {
                                        printf("\nuser_data_delete_from_folder: persadmin_delete_file(%s) failed\n", sAbsPath) ;
                                    }
                                }

                                // position the index to the next entity in the Seat buffer
                                posInListBufferForSeat += lenSeat;
                            }
                        }

                        if ( NIL != sSeatFolders )
                        {
                            free( sSeatFolders );
                            sSeatFolders = NIL;
                        }
                    }
                    else
                    {
                        printf("user_data_delete_from_folder: folder does not exist: %s\n", seat_no_path);                    
                    }
                }

                // File - check if file name is cached.itz or wt.itz
                if ( ( 0 == strcmp( CACHED_DATABASE, sFolderOrFile) ) || ( 0 == strcmp( WT_DATABASE, sFolderOrFile) ) )
                {
                    sprintf( sAbsPath, "%s%s", folder_path, sFolderOrFile);
                    printf("user_data_delete_from_folder: persadmin_delete_keys_by_filter: %s\n", sAbsPath);
                    if ( 0 != persadmin_delete_keys_by_filter(PersASSelectionType_User, sAbsPath, user_no, seat_no) )
                    {
                        printf("\nuser_data_delete_from_folder: persadmin_delete_keys_by_filter(%s) failed\n", sAbsPath) ;
                    }
                    else
                    {
                        printf("\nuser_data_delete_from_folder: Deleted keys from table <%s> \n", sAbsPath);
                    }
                }

                // position the index to the next entity in the Group buffer
                posInListBuffer += len;
            }
        }

        if ( NIL != sFoldersAndFiles )
        {
            free( sFoldersAndFiles );
            sFoldersAndFiles = NIL;
        }
        
    }
    
    return iBytesDeleted;
}

/**
 * @brief Delete the user related data from persistency containers
 *
 * @param user_no the user ID
 * @param seat_no the seat number (seat 0 to 3)
 *
 * @return positive value: number of bytes deleted; negative value: error code
 */
long persadmin_user_data_delete (unsigned int user_no, unsigned int seat_no)
{
    bool_t bEverythingOK = true;
    sint_t iBytesDeleted = 0;                         // return value

    uint8_t i;                                        // a switch used to select curent Mnt folder

    str_t   sUsedMnt[PERSADMIN_MAX_PATH_LENGHT];

    pstr_t  sMntFolders = NIL;                              // list of the folders found in current /Data/mnt path
    pstr_t  sGroupFolders = NIL;                            // list of the folders found in current Group path

    sint_t  iMntFoldersBufSize;                       // size of the buffer containing the list of the folders found in current /Data/mnt path
    sint_t  iGroupFoldersBufSize;                     // size of the buffer containing the list of the folders found in current Group folder

    str_t   sMntFolder[PERSADMIN_MAX_PATH_LENGHT];     // contains current mnt subfolder name
    str_t   sGroupFolder[PERSADMIN_MAX_PATH_LENGHT];   // contains current group subfolder name

    str_t  sAbsPath[PERSADMIN_MAX_PATH_LENGHT]; // absolute folder path that will be sent to user_data_delete_from_folder(...)
    
    sint_t posInListBuffer;
    sint_t posInListBufferGroup;
    sint_t len;
    sint_t lenGroup;

    for ( i = 0; i < 2; i++ ) // 2 - cached and write through
    {
        // set path to current mnt folder
        if ( 0 == i )
        {
            strcpy( sUsedMnt, MNT_C_PATH );
    	  }
        else
        {
            strcpy( sUsedMnt, MNT_WT_PATH );
        }

        printf("\n\npersadmin_user_data_delete: mnt = %s  \n", sUsedMnt);
        
        //get requierd buffer length
        iMntFoldersBufSize = persadmin_list_folder_get_size( sUsedMnt, PersadminFilterFolders, false );
        if ( 0 > iMntFoldersBufSize )
        {
            bEverythingOK = false;
        }
        
        printf("persadmin_user_data_delete: persadmin_list_folder_get_size returned: %d\n", iMntFoldersBufSize);

        if ( bEverythingOK )
        {
        	printf("persadmin_user_data_delete: try malloc\n");

            //alocate required memory
            sMntFolders = ( pstr_t )malloc( iMntFoldersBufSize );
            if ( NIL == sMntFolders )
            {
                bEverythingOK = false;
                printf("persadmin_user_data_delete: malloc failed\n");
            }
            else
            {
                printf("persadmin_user_data_delete: malloc successful\n");
            }
        }

        if ( bEverythingOK )
        {
            // get the files and folders inside the curent mnt folder and store them in the sMntFolders buffer
            if ( 0 > persadmin_list_folder( sUsedMnt, sMntFolders, iMntFoldersBufSize, PersadminFilterFolders, false) )
            {
                bEverythingOK = false;
                printf("persadmin_user_data_delete: persadmin_list_folder failed\n");
            }
            else
            {
                printf("persadmin_user_data_delete: persadmin_list_folder successful\n");
            }
        }

        if ( bEverythingOK )
        {
            posInListBuffer = 0;
            len = 0;
            while ( ( posInListBuffer < iMntFoldersBufSize ) && ( bEverythingOK ) )
            {
                strcpy(sMntFolder, sMntFolders + posInListBuffer);
                len = strlen(sMntFolder) + 1;
                
                printf("persadmin_user_data_delete: curent entity: %s\n", sMntFolder);
                
                // check if fodler name = Shared
                if ( 0 == strcmp( SHARED_FOLDER, sMntFolder) )
                {                    
                    // delete user data in Public folder
                    sprintf( sAbsPath, "%s%s/%s/", sUsedMnt, SHARED_FOLDER, PUBLIC_FOLDER); // sAbsPath = /Data/mnt-?/shared/public/
                    printf("persadmin_user_data_delete: user_data_delete_from_folder: %s\n", sAbsPath);
                    iBytesDeleted += user_data_delete_from_folder(sAbsPath, user_no, seat_no);

                    // delete user data in Group folder
                    sprintf( sAbsPath, "%s%s/%s/", sUsedMnt, SHARED_FOLDER, GROUP_FOLDER); // sAbsPath = /Data/mnt-?/shared/group/
                    printf("persadmin_user_data_delete: user_data_delete_from_folder: %s\n", sAbsPath);
                    iBytesDeleted += user_data_delete_from_folder(sAbsPath, user_no, seat_no);

                    if ( 0 == persadmin_check_if_file_exists(sAbsPath, true) )
                    {
						// delete user data in all folders from Group folder
						// get requierd buffer length for files and folders inside the Group folder
						iGroupFoldersBufSize = persadmin_list_folder_get_size( sAbsPath, PersadminFilterFolders, false );
						if ( 0 > iGroupFoldersBufSize )
						{
							bEverythingOK = false;
						}

						if ( bEverythingOK )
						{
							//alocate required memory
							sGroupFolders = ( pstr_t )malloc( iGroupFoldersBufSize );
							if ( NIL == sGroupFolders )
							{
								bEverythingOK = false;
								printf("persadmin_user_data_delete: group malloc failed!\n");
							}
							else
							{
								printf("persadmin_user_data_delete: group malloc successful!\n");
							}
						}

						if ( bEverythingOK )
						{
							// get the files and folders inside the Group folder and store them in the sGroupFolders buffer
							if ( 0 > persadmin_list_folder( sAbsPath, sGroupFolders, iGroupFoldersBufSize, PersadminFilterFolders, false) )
							{
								bEverythingOK = false;
								printf("persadmin_user_data_delete: group persadmin_list_folder failed!\n");
							}
							else
							{
								printf("persadmin_user_data_delete: group persadmin_list_folder successful!\n");
							}
						}

						if ( bEverythingOK )
						{

							posInListBufferGroup = 0;
							lenGroup = 0;
							while ( ( posInListBufferGroup < iGroupFoldersBufSize ) && ( bEverythingOK ) )
							{
								strcpy(sGroupFolder, sGroupFolders + posInListBufferGroup);
								lenGroup = strlen(sGroupFolder) + 1;

								printf("persadmin_user_data_delete: curent group entity: %s\n", sGroupFolder);

								// delete user data in Group subfolder
								// sAbsPath = /Data/mnt-?/shared/group/group_no/
								sprintf( sAbsPath, "%s%s/%s/%s/", sUsedMnt, SHARED_FOLDER, GROUP_FOLDER, sGroupFolder);
								printf("persadmin_user_data_delete: user_data_delete_from_folder: %s\n", sAbsPath);
								iBytesDeleted += user_data_delete_from_folder(sAbsPath, user_no, seat_no);

								// position the index to the next entity in the Group buffer
								posInListBufferGroup += lenGroup;
							}
						}

						if ( sGroupFolders )
						{
							free( sGroupFolders );
							sGroupFolders = NIL;
						}
                	}
                }
                else
                {
                    // delete user data in Application folder
                    sprintf( sAbsPath, "%s%s/", sUsedMnt, sMntFolder); // sAbsPath = /Data/mnt-?/applicatioName/
                    printf("persadmin_user_data_delete: user_data_delete_from_folder: %s\n", sAbsPath);                            
                    iBytesDeleted += user_data_delete_from_folder(sAbsPath, user_no, seat_no);
                }

                // position the index to the next entity in the Mnt buffer
                posInListBuffer += len;
            }
        }

        if ( sMntFolders )
        {
        	free( sMntFolders );
        	sMntFolders = NIL;
        }
    }
    
    printf("persadmin_user_data_delete: returned : %d\n", iBytesDeleted);

    return ( ( true == bEverythingOK ) ? iBytesDeleted : PAS_FAILURE ) ;
}
