/*********************************************************************************************************************
*
* Copyright (C) 2013 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Small tool (only for development purpose) to access (part of) functionality provided by Persistence Administrator.
* To be used until SWL will be available.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.09.19 uidl9757           CSP_WZ#5759:   Add support for "restore"
* 2013.05.28 uidl9757           CSP_WZ#12152:  Created
*
**********************************************************************************************************************/

#include "persComTypes.h"
#include "stdio.h"
#include "string.h"
#include "persistence_admin_service.h"
#include "dlt/dlt.h"

#define LT_HDR                          "[Persadmin_Tool]:"
DLT_DECLARE_CONTEXT                      (persadminToolDLTCtx)

typedef enum _pastool_commands_e
{
    PASTOOL_CMD_INSTALL = 0,
    PASTOOL_CMD_BACKUP,
    PASTOOL_CMD_EXPORT,
    PASTOOL_CMD_RECOVERY,
    PASTOOL_CMD_IMPORT,
    PASTOOL_CMD_RESTORE,
    PASTOOL_CMD_SHOW_HELP,
    /* add new entries here */
    PASTOOL_CMD_LAST_ENTRY
}pastool_commands_e ;

typedef struct
{
    pastool_commands_e  eCommand ;
    int                 iExpectedArgsNo ;
    char*               pCommandName ;
}pastool_command_name_s;

typedef struct
{
    pastool_commands_e      eCommand ;
    char                    path[256] ;
    PersASSelectionType_e   eSelectionType ;
    char                    appName[256] ;
    unsigned int            user_no ;
    unsigned int            seat_no ;
}pastool_input_s ;

static const pastool_command_name_s a_sCommandNames[] =
{
    {PASTOOL_CMD_INSTALL,       3,     "install"},
    {PASTOOL_CMD_BACKUP,        7,     "backup"},
    {PASTOOL_CMD_EXPORT,        4,     "export"},
    {PASTOOL_CMD_RECOVERY,      7,     "recovery"},
    {PASTOOL_CMD_IMPORT,        4,     "import"},
    {PASTOOL_CMD_RESTORE,       7,     "restore"},
    {PASTOOL_CMD_SHOW_HELP,     2,     "help"},
    {PASTOOL_CMD_SHOW_HELP,     2,     "-help"},
    {PASTOOL_CMD_SHOW_HELP,     2,     "-h"},    
    {PASTOOL_CMD_SHOW_HELP,     2,     "-?"},
};

static char g_msg[512] ;
static pastool_input_s sInputCommand = {0};

static bool_t pastool_parseArguments(int argc, char *argv[]);
static void pastool_printHelp(void) ;
static bool_t pastool_getCommand(char* arg, pastool_commands_e* peCommand_out) ;
static void pastool_printArguments(pastool_input_s* psArguments, int iNumberOfArguments) ;
static bool_t pastool_executeCommand(pastool_input_s* psArguments) ;

static bool_t pastool_parseArguments(int argc, char *argv[])
{
    bool_t bCanContinue = true ;
    int iNumberOfArguments = argc ;
    if(iNumberOfArguments < 2)
    {
        bCanContinue = false ;
        snprintf(g_msg, sizeof(g_msg), "%s:invalid number of arguments =%d", __FUNCTION__, iNumberOfArguments) ;
        DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bCanContinue)
    {
        if( ! pastool_getCommand(argv[1], &sInputCommand.eCommand))
        {
            bCanContinue = false ;
        }
        else
        {
            /* check if the right number of parameters provided */
            if(iNumberOfArguments != a_sCommandNames[sInputCommand.eCommand].iExpectedArgsNo)
            {
            	bCanContinue = false ;
                snprintf(g_msg, sizeof(g_msg), "%s:invalid number of arguments for <<%s>> %d (expected %d)",
                    __FUNCTION__, argv[1], iNumberOfArguments, a_sCommandNames[sInputCommand.eCommand].iExpectedArgsNo) ;
                DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
    }

    if(bCanContinue && (iNumberOfArguments > 2) )
    {
        /* argv[2] is always <<path>> 
         * Don't check the path (PAS will do it)
         */
         strncpy(sInputCommand.path, argv[2], sizeof(sInputCommand.path)) ;
    }

    if(bCanContinue && (iNumberOfArguments > 3))
    {
        /* argv[3] is always <<selection type>>
         */
    	if(1 == sscanf(argv[3], "%d", (int*)&sInputCommand.eSelectionType))
    	{
    		if((sInputCommand.eSelectionType < PersASSelectionType_All) || (sInputCommand.eSelectionType >= PersASSelectionType_LastEntry))
    		{
    			bCanContinue = false ;
                snprintf(g_msg, sizeof(g_msg), "%s:selection type = <<%d>>",
                    __FUNCTION__, sInputCommand.eSelectionType) ;
                DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    		}
    	}
    }

    if(bCanContinue && (iNumberOfArguments > 4))
    {
        /* argv[4] is always <<application>>
         * Don't check the path (PAS will do it)
         */
        strncpy(sInputCommand.appName, argv[4], sizeof(sInputCommand.appName)) ;
    }

    if(bCanContinue && (iNumberOfArguments > 5))
    {
        /* argv[5] is always <<user number>> */
        if(1 != sscanf(argv[5], "%d", &sInputCommand.user_no))
        {
        	bCanContinue = false ;
            snprintf(g_msg, sizeof(g_msg), "%s:invalid user_no = <<%s>>",
                __FUNCTION__, argv[5]) ;
            DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            if((-1) == sInputCommand.user_no)
            {
                sInputCommand.user_no = PERSIST_SELECT_ALL_USERS ;
            }
        }
    }

    if(bCanContinue && (iNumberOfArguments > 6))
    {
        /* argv[4] is always <<seat number>> */
        if(1 != sscanf(argv[6], "%d", &sInputCommand.seat_no))
        {
        	bCanContinue = false ;
            snprintf(g_msg, sizeof(g_msg), "%s:invalid seat_no = <<%s>>",
                __FUNCTION__, argv[6]) ;
            DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            if((-1) == sInputCommand.seat_no)
            {
                sInputCommand.seat_no = PERSIST_SELECT_ALL_SEATS ;
            }
        }
    }

    return bCanContinue ;
}

static void pastool_printHelp(void)
{

    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("==================== PAS Tool - help ===================================================="));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("Command format:"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("	  /usr/bin/persadmin_tool command path [selection_type][application] [user_no] [seat_no]"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("|   command    |    path    | selection type | application |    user_no     |   seat_no  |"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("|   install    | mandatory  |       na       |     na      |       na       |     na     |"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("|   backup     | mandatory  |    mandatory   |  mandatory  |    mandatory   |  mandatory |"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("|   recovery   | mandatory  |    mandatory   |  mandatory  |    mandatory   |  mandatory |"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("|   export     | mandatory  |    mandatory   |     na      |       na       |     na     |"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("|   import     | mandatory  |    mandatory   |     na      |       na       |     na     |"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("|   restore    | mandatory  |    mandatory   |  mandatory  |    mandatory   |  mandatory |"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("|     help     | mandatory  |    mandatory   |     na      |       na       |     na     |"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("-----------------------------------------------------------------------------------------"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("path meaning:"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      for install  : abs pathname to input installation file (.tar.gz)"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      for backup   : abs pathname to output backup file (.tar.gz)"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      for recovery : abs pathname to input backup file (.tar.gz)"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      for export   : abs path to destination folder"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      for import   : abs path to source folder"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      for restore  : choose one of: FactoryDefault or ConfigDefault"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("selection type meaning:"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      <<0>>  : select all data/files: (node+user)->(application+shared)"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      <<1>>  : select user data/files: (user)->(application+shared)"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      <<2>>  : select application data/files: (application)->(node+user)"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("user_no and seat_no meaning:"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      <<1 to 4>> : select a single user/seat"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("      <<-1>>     : select all users/seats"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("Examples:"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("        /usr/bin/persadmin_tool install /tmp/resource.tar.gz"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("        /usr/bin/persadmin_tool backup /tmp/backup 2 MyApp -1 -1"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("        /usr/bin/persadmin_tool backup /tmp/backup 2 MyApp 2 -1"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("        /usr/bin/persadmin_tool export /tmp/export 0"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("        /usr/bin/persadmin_tool restore FactoryDefault 2 MyApp -1 -1"));
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("=========================================================================================="));
}

static void pastool_printArguments(pastool_input_s* psArguments, int iNumberOfArguments)
{
    snprintf(g_msg, sizeof(g_msg), "Called with:") ;
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    snprintf(g_msg, sizeof(g_msg), "command        = <<%s>>", a_sCommandNames[psArguments->eCommand].pCommandName) ;
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    snprintf(g_msg, sizeof(g_msg), "path           = <<%s>>", psArguments->path) ;
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    snprintf(g_msg, sizeof(g_msg), "selection type = <<%d>>",
    		      (iNumberOfArguments > 3) ? psArguments->eSelectionType : (-1)) ;
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    snprintf(g_msg, sizeof(g_msg), "application    = <<%s>>",
                  (iNumberOfArguments > 4) ? psArguments->appName : "Not Applicable") ;
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    snprintf(g_msg, sizeof(g_msg), "user_no        = <<%d>>",
                  (iNumberOfArguments > 5) ? psArguments->user_no : -1) ;
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    snprintf(g_msg, sizeof(g_msg), "seat_no        = <<%d>>",
                  (iNumberOfArguments > 6) ? psArguments->seat_no : -1) ;
    DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

}

static bool_t pastool_getCommand(char* arg, pastool_commands_e* peCommand_out)
{
    bool_t bFoundCommand = false ;

    if(NULL != arg)
    {
        int i = 0 ;
        for(i = 0 ; i < sizeof(a_sCommandNames)/sizeof(a_sCommandNames[0]) ; i++)
        {
            if(0 == strcmp(a_sCommandNames[i].pCommandName, arg))
            {
                *peCommand_out = a_sCommandNames[i].eCommand ;
                bFoundCommand = true ;
                break ;
            }
        }
    }
    else
    {
        bFoundCommand = false ;
    }

    return bFoundCommand ;
}

static bool_t pastool_executeCommand(pastool_input_s* psArguments)
{
    bool_t bEverythingOK = true ;
    long result = -1 ;

    switch(psArguments->eCommand)
    {
        case PASTOOL_CMD_SHOW_HELP:
        {
            pastool_printHelp() ;
            break ;
        }
        case PASTOOL_CMD_INSTALL:
        {
            result = persAdminResourceConfigAdd(psArguments->path) ;
            snprintf(g_msg, sizeof(g_msg), "persAdminResourceConfigAdd(%s) returned <<%ld>>", psArguments->path, result) ;
            DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            if(result < 0)
            {
                bEverythingOK = false ;
            }
            break ;
        }
        case PASTOOL_CMD_BACKUP:
        {
            result = persAdminDataBackupCreate(psArguments->eSelectionType, psArguments->path, psArguments->appName, psArguments->user_no, psArguments->seat_no);
            snprintf(g_msg, sizeof(g_msg), "persAdminDataBackupCreate(<<%d>>, <<%s>>, <<%s>>, <<%d>>, <<%d>>) returned <<%ld>>",
                    psArguments->eSelectionType, psArguments->path, psArguments->appName, psArguments->user_no, psArguments->seat_no, result) ;
            DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            if(result < 0)
            {
                bEverythingOK = false ;
            }
            break ;
        }
        case PASTOOL_CMD_EXPORT:
        {
            result = persAdminDataFolderExport(psArguments->eSelectionType, psArguments->path);
            snprintf(g_msg, sizeof(g_msg), "persAdminDataFolderExport(<<%d>>, <<%s>>) returned <<%ld>>",
                    psArguments->eSelectionType, psArguments->path, result) ;
            DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            if(result < 0)
            {
                bEverythingOK = false ;
            }
            break ;
        }
        case PASTOOL_CMD_RECOVERY:
        {
            long result = persAdminDataBackupRecovery(psArguments->eSelectionType, psArguments->path, psArguments->appName, psArguments->user_no, psArguments->seat_no);
            snprintf(g_msg, sizeof(g_msg), "persAdminDataBackupRecovery(<<%d>>, <<%s>>, <<%s>>, <<%d>>, <<%d>>) returned <<%ld>>",
                    psArguments->eSelectionType, psArguments->path, psArguments->appName, psArguments->user_no, psArguments->seat_no, result) ;
            DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            if(result < 0)
            {
                bEverythingOK = false ;
            }
            break ;
        }
        case PASTOOL_CMD_IMPORT:
        {
            long result = persAdminDataFolderImport(psArguments->eSelectionType, psArguments->path);
            snprintf(g_msg, sizeof(g_msg), "persAdminDataFolderImport(<<%d>>, <<%s>>) returned <<%ld>>",
                    psArguments->eSelectionType, psArguments->path, result) ;
            DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            if(result < 0)
            {
                bEverythingOK = false ;
            }
            break ;
        }
        case PASTOOL_CMD_RESTORE:
            {
                PersASDefaultSource_e defaultSource = PersASDefaultSource_Configurable;
                if(0 == strcmp(psArguments->path, "ConfigDefault"))
                {
                    defaultSource = PersASDefaultSource_Configurable;
                }
                result = persAdminDataRestore(psArguments->eSelectionType, defaultSource, psArguments->appName, psArguments->user_no, psArguments->seat_no);
                snprintf(g_msg, sizeof(g_msg), "persAdminDataRestore(<<%d>>, <<%s>>, <<%s>>, <<%d>>, <<%d>>) returned <<%ld>>",
                        psArguments->eSelectionType, (PersASDefaultSource_Configurable==defaultSource)?"ConfigDefault":"FactoryDefault",
                        psArguments->appName, psArguments->user_no, psArguments->seat_no, result) ;
                DLT_LOG(persadminToolDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                if(result < 0)
                {
                    bEverythingOK = false ;
                }
                break ;
            }
        default:
        {
            /* not possible, but just to be sure */
            bEverythingOK = false ;
            snprintf(g_msg, sizeof(g_msg), "unexpected command <<%d>> !!!", psArguments->eCommand) ;
            DLT_LOG(persadminToolDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            break ;
        }
    }

    return bEverythingOK ;
}

int main(int argc, char *argv[])
{
    bool_t bEverythingOK = true ;

    DLT_REGISTER_APP("0037","PAS_Tool"); /* 0x0037 <=> OIP_SSW_PERSADMIN_TOOL */
    DLT_REGISTER_CONTEXT(persadminToolDLTCtx,"PASt", "PAS_Tool");
    DLT_ENABLE_LOCAL_PRINT() ;

    if(pastool_parseArguments(argc, argv))
    {
        pastool_printArguments(&sInputCommand, argc);
        if(! pastool_executeCommand(&sInputCommand))
        {
            bEverythingOK = false ;
        }
    }
    else
    {
        bEverythingOK = false ;
        pastool_printHelp() ;
    }

    DLT_UNREGISTER_CONTEXT(persadminToolDLTCtx) ;
    DLT_UNREGISTER_APP() ;

    return bEverythingOK ? 0 : (-1) ;
}
