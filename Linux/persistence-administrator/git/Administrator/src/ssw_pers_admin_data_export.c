/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Marian.Nestor@continental-corporation.com
*
* Implementation of export process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2012.11.16 uidn3565        	CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/ 
 
#include <string.h>

#include "persistence_admin_service.h"
#include "ssw_pers_admin_common.h"
#include "ssw_pers_admin_service.h"

//moved to "ssw_pers_admin_common.h"
//extern const char StringTerminator;

long persadmin_data_folder_export(PersASSelectionType_e type, pconststr_t dst_folder)
{
    long result = 0;

    if ( type == PersASSelectionType_Application )
    {
        int listSize = persadmin_list_application_folders_get_size( "" );

        if ( listSize > 0 )
        {
            char appList[ listSize ];

            listSize = persadmin_list_application_folders( "", appList, listSize );

            if ( listSize > 0 )
            {
                char * crtApp = appList;
                int totalBackupSize = 0;
                result = 0;
                while ( ( ( * crtApp ) != 0 ) && ( result >= 0 ) )
                {
                    int crtAppLen = strlen( crtApp );
                    totalBackupSize += result;
                    result = persadmin_data_backup_create( type, dst_folder, crtApp, PERSIST_SELECT_ALL_USERS, PERSIST_SELECT_ALL_SEATS );
                    crtApp += crtAppLen + sizeof( StringTerminator );
                }
                // The size of the last backup hasn't been added to the overall counter
                if ( result >= 0 )
                {
                    result += totalBackupSize;
                }
            }
        }
    }
    else
    {
        result = persadmin_data_backup_create( type, dst_folder, "", PERSIST_SELECT_ALL_USERS, PERSIST_SELECT_ALL_SEATS );
    }

  return result;

} /* data_folder_export() */
