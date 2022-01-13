/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**
* \file: iap_database.c
*
*
***************************************************************************** */

#include <adit_typedef.h>
#include <stdlib.h>

#include "ipodcommon.h"
#include "iap_database.h"
#include "iap_commands.h"
#include "iap_types.h"
#include "iap_general.h"
#include "iap_util_func.h"
#include "iap_callback.h"
#include "iap_init.h"
#include "iap_playback.h"
#include "iap1_dlt_log.h"
#include <string.h>


LOCAL S32 iPodGetTrackInfo(U32 iPodID,
                           IPOD_TRACK_INFORMATION_CB_INT callback,
                           const IPOD_TRACK_INFORMATION_DATA *infoInData,
                           U8 *buf,
                           IPOD_TRACK_TYPE type,
                           IPOD_TRACK_INFORMATION_BITFIELD* pbitfield,
                           U32 *chapterCount);

LOCAL S32 internal_iPodRetrieveCategorizedDBRecords(U32 iPodID,
                                                    IPOD_CATEGORY category,
                                                    U32 start,
                                                    U32 count,
                                                    U32 numDBRecords,
                                                    const IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback);

LOCAL S32 internal_iPodGetChapterTotalCount(U32 iPodID, IPOD_TRACK_TYPE type, U16 *totalCount, U64 trackIndex);

/*!
 * \fn iPodBitcount(U32 bitmask)
 * \par INPUT PARAMETERS
 * U32 bitmask - bitmask with activated bits
 * \par REPLY PARAMETERS
 * U8 count - number of activated bits
 * \par DESCRIPTION
 * This function traverse the bitmask to count the number of activated bits.
 */
LOCAL U8 iPodBitcount(U32 bitmask);
LOCAL U8 iPodBitcount(U32 bitmask)
{
    U8 count = 0;
    U32 i = 0;
    for (i = 0; i < (sizeof(bitmask) * IPOD_8BITS); i++)
    {
        if (bitmask & 1)
        {
            count++;
        }
        bitmask >>= 1;
    }
    return count;
}

/**
 * \addtogroup Database_commands
 */
/*\{*/

/*!
 * \fn iPodGetNumberCategorizedDBRecords(U32 iPodID, IPOD_CATEGORY category)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_CATEGORY category - the category id
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b result number of entries
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the number of records in the specified database category.
 * iPodGetNumberCategorizedDBRecords must be called to initialize the category
 * count before selecting a database record using "iPodSelectDBRecord" or
 * "iPodSelectSortDBRecord" commands. A category's record count can change based
 * on the prior categories selected and the database hierarchy.
 * The user is expected to call iPodGetNumberCategorizedDBRecords in order to get
 * the valid range of category entries before selecting a record in that category.
 * \note If category selected an unsupported category, this function returns an error (IPOD_BAD_PARAMETER).
 */
S32 iPodGetNumberCategorizedDBRecords(U32 iPodID, IPOD_CATEGORY category)
{
    U8 msg[]    = {IPOD_GET_NUM_CAT_DB_RECORDS_CMD};
    S32 result  = IPOD_OK;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if (result == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "category = 0x%02x",category);
            msg[IPOD_POS5] = (U8)category;
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_NUM_CATEGORIZED_DB_RECORDS,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            result = iPodSendCommand(iPodHndl, msg);
            if (result == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (result == IPOD_OK)
                {
                    result = (S32)iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS0]);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "NumberCategorizedDBRecords = %d",result);
                }
            }
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);

    return result;
}


/*!
 * \fn iPodSelectSortDBRecords(U32 iPodID, IPOD_CATEGORY category, U32 select_index, IPOD_DB_SORT_TYPE sortType)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_CATEGORY category - the category id <br>
 * U32 select_index - category record index <br>
 * #IPOD_DB_SORT_TYPE sortType - the sortType id
 * \par REPLY PARAMETERS
 * S32 returnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function selects one or more records in the iPod database, based on a category-relative index.
 * The result list is sorted by the specified sort type
 * <br>The default order of song and audiobook tracks on the iPod is alphabetical by artist, then alphabetical
 * by that artist's albums, then ordered according to the order of the tracks on the album. For podcasts,
 * the default order of episode tracks is reverse chronological. That is, the newest ones are first, then
 * alphabetical by podcast name.
 * <br>The SelectSortDBRecord command can be used to sort all the song and audiobook tracks on the
 * iPod alphabetically as follows:
 * <br>1. "Command 0x0016: ResetDBSelection" (page 71)
 * <br>2. "Command 0x0018: GetNumberCategorizedDBRecords" (page 74) for the Playlist category.
 * <br>3. SelectSortDBRecord based on the Playlist category, using a record index of 0 to select the All
 * Tracks playlist and the sort by name (0x04) sort order.
 * <br>4. GetNumberCategorizedDBRecords for the Track category.
 * <br>5. "Command 0x001A: RetrieveCategorizedDatabaseRecords" (page 76) based on the Track category,
 * <br>using a start index of 0 and an end index of the number of records returned by the call to
 * <br>GetNumberCategorizedDBRecords in step 4.
 * <br>The sort order of artist names ignores certain articles such that the artist "The Doors" is sorted under
 * the letter 'D' and not 'T'; this matches the behavior of iTunes. The sort order is different depending
 * on the language setting used in the iPod. The list of ignored articles may change in the future without
 * notice.
 * <br>The SelectDBRecord command may also be used to select a database record with the default sort
 * order.
 * \note If category selected an unsupported category, this function returns an error (IPOD_BAD_PARAMETER).
 * \note SelectSortDBRecords was marked as deprecated by Apple and should not be used in new accessory designs.
 */
S32 iPodSelectSortDBRecords(U32 iPodID, IPOD_CATEGORY category,
                            U32 select_index,
                            IPOD_DB_SORT_TYPE sortType)
{
    S32 result = IPOD_OK;
    U8 msg[] = {IPOD_SELECT_SORT_DB_RECORDS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if(result == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "category = 0x%02x selected Index = %u Sort Type = 0x%02x",category,select_index,sortType);
            /* insert DB Category into msg array */
            msg[IPOD_POS5] = (U8)category;

            /* insert index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS6], select_index);

            /* insert sortType into msg array */
            msg[IPOD_POS10] = (U8)sortType;

            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE, (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            result = iPodSendCommand(iPodHndl, msg);
            if (result == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (result == IPOD_OK)
                {
                    result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                }
            }
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);

    return result;
}

/*!
 * \fn iPodRetrieveCategorizedDBRecords(U32 iPodID, IPOD_CATEGORY category, U32 start, S32 count, const IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_CATEGORY category - the category id
 * U32 start - start record index <br>
 * S32 count - number of records to retrieve <br>
 * #IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback - pointer to callback function
 * \par REPLY PARAMETERS
 * S32 returnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function retrieves from the specified category the number of database records specified by
 * the input param count, starting at the specified start position.
 * To retrieve all records from the given starting record, set the record count to -1 (0xFFFFFFFF).
 * For each found record the callback function will be called. The parameters supplied to the callback
 * are the database record category index and the database record as a character array.
 * \note If category selected an unsupported category, this function returns an error (IPOD_BAD_PARAMETER).
 */
S32 iPodRetrieveCategorizedDBRecords(U32 iPodID, IPOD_CATEGORY category,
                                      U32 start,
                                      S32 count,
                                      const IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback)
{
    S32 rc              = IPOD_OK;
    S32 resCount        = count;
    S32 numDBRecords    = count + (S32)start;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (callback != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if (rc == IPOD_OK)
        {
#ifdef IPOD_FAST_DB
            if(count == IPOD_GET_ALL_CAT_ENTRIES)
            {
                numDBRecords = iPodGetNumberCategorizedDBRecords(iPodID, category);
                if (numDBRecords >= 0)
                {
                    if (start <= (U32)numDBRecords)
                    {
                        resCount = (S32)numDBRecords - (S32)start;
                    }
                }
                else
                {
                    rc = numDBRecords;
                }
            }
#else /* IPOD_FAST_DB */
            numDBRecords = iPodGetNumberCategorizedDBRecords(iPodID, category);
            if(numDBRecords >= 0)
            {
                if (((((S32)start + count) <= numDBRecords) || (count == IPOD_GET_ALL_CAT_ENTRIES))
                    && ((S32)start < numDBRecords) && (numDBRecords > 0))
                {
                    if (count == IPOD_GET_ALL_CAT_ENTRIES)
                    {
                        resCount = (S32)numDBRecords - (S32)start;
                    }
                    else
                    {
                        resCount = count;
                    }
                }
                else
                {
                    rc = IPOD_BAD_PARAMETER;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPod bad Parameter: start = %d count = %d numDBRecords = %d",start,count,numDBRecords);
                }
            }
            else
            {
                rc = numDBRecords;
            }
#endif /* IPOD_FAST_DB */
            if(rc == IPOD_OK)
            {
                rc = internal_iPodRetrieveCategorizedDBRecords(iPodID,
                                                               category,
                                                               start,
                                                               (U32)resCount,
                                                               (U32)numDBRecords,
                                                               callback);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p,callback = %p", iPodHndl, callback);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;

}


/*!
 * \fn iPodResetDBSelection(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 returnCode -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function resets the current database selection to an empty state and
 * invalidates the category count - that is, set the count to 0 - for all
 * categories except the playlist category.
 * This is analogous to pressing the Menu button repeatedly to get to the topmost
 * iPod HMI menu. Any previously selected database items are deselected.
 */
S32 iPodResetDBSelection(U32 iPodID)
{
    S32 result = IPOD_ERROR;
    U8 msg[] = {IPOD_RESET_DB_SELECTION_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if (result == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE, (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            result = iPodSendCommand(iPodHndl, msg);
            if (result == 0)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (result == 0)
                {
                    result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                }
            }
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodSelectDBRecord(U32 iPodID, IPOD_CATEGORY category, U32 idx)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * U32 iPodID - ID of the Apple device<br>
 * \par INPUT PARAMETERS
 * #IPOD_CATEGORY category - the entries category id <br>
 * U32 idx - the entries index
 * \par REPLY PARAMETERS
 * S32 returnCode -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function selects one or more records in the Database Engine, based on a category
 * relative index. For example, selecting category two (artist) and record index one results
 * in a list of selected tracks (or database records) from the second artist in the artist
 * list.
 * iPodSelectDBRecord should be called only once a category count has been initialized through
 * a call to "iPodGetNumberCategorizedDBRecords"
 * Note that the selection of a single record automatically passes it to the Playback Engine
 * and starts its playback.
 * \note If category selected an unsupported category, this function returns an error (IPOD_BAD_PARAMETER).
 */
S32 iPodSelectDBRecord(U32 iPodID, IPOD_CATEGORY category, U32 idx)
{
    S32 result = IPOD_ERROR;
    U8 msg[] = {IPOD_SELECT_DB_RECORD_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if (result == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Category = 0x%02x Index = %u",category,idx);
            /* insert DB Category into msg array */
            msg[IPOD_POS5] = (U8)category;
            /* insert index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS6], idx);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE, (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            result = iPodSendCommand(iPodHndl, msg);
            if (result == 0)
            {
               memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
               result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
               if (result == 0)
               {
                   result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                   IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
               }
            }
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodReturnToPreviousDBRecordSelection(U32 iPodID, IPOD_CATEGORY category)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_CATEGORY category - the category id <br>
 * \par REPLY PARAMETERS
 * S32 returnCode -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function undos the last database selection for the specified category. This
 * has the same effect as pressing the iPod Menu button once and moves the database
 * selection up to the next highest menu level.
 * \note If category selected an unsupported category, this function returns an error (IPOD_BAD_PARAMETER).
 */
S32 iPodReturnToPreviousDBRecordSelection(U32 iPodID, IPOD_CATEGORY category)
{
    S32 result = IPOD_ERROR;
    U8 msg[] = {IPOD_RET_TO_PREV_DB_SELECTION_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if (result == IPOD_OK)
        {
            /* insert DB Category into msg array */
            msg[IPOD_POS5] = (U8)category;
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Category = 0x%02x",category);

            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE, (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            result = iPodSendCommand(iPodHndl, msg);
            if (result == 0)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (result == 0)
                {
                    result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                }
            }
        }
        else
        {
            result = IPOD_ERR_ONLY_IN_ADVANCEDMODE;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "ERROR-- This operation is only available in advanced mode");
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodPlayCurrentSelection(U32 iPodID, U32 idx)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 idx - index of the track to play<br>
 * \par REPLY PARAMETERS
 * S32 returnCode -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function requests playback of the currently selected track or list of tracks.
 * The currently selected tracks are placed in the "Now Playing" playlist, the
 * playback starts with the track at position "idx" in the list.
 * \note The command PlayCurrentSelection is deprecated by Apple and should not be used in new accessory designs.
 */
S32 iPodPlayCurrentSelection(U32 iPodID, U32 idx)
{
    S32 result = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_CURRENT_SELECTION_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if (result == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Selected track record index = %u",idx);
            /* insert index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], idx);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE, (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            result = iPodSendCommand(iPodHndl, msg);
            if (result == 0)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (result == 0)
                {
                    result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                }
            }
        }
        else
        {
            result = IPOD_ERR_ONLY_IN_ADVANCEDMODE;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "ERROR-- This operation is only available in advanced mode");
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodResetDBSelectionHierarchy(U32 iPodID, U8 selection)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.11
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 selection - kind of hierarchy<br>
 * audio hierarcy  = 0x01 <br>
 * video hierarchy = 0x02 <br>
 * \par REPLY PARAMETERS
 * S32 returnCode - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function select audio hierarchy or video hierarchy.
 * \note If the device sends this command while selecting the audio hierarchy,
 * the database resets itself to the audio hierarchy and any video selections are invalidated.
 * Video selection already passed to the Playback Engine are unaffected.
 */
S32 iPodResetDBSelectionHierarchy(U32 iPodID, U8 selection)
{
    S32 rc = IPOD_OK;
    U8 msg[]   = {IPOD_RESET_DB_SELECTION_HIERARCHY};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE, (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            /* insert index into msg array */
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Hierarchy selection = 0x%02x",selection);
            msg[IPOD_POS5] = selection;
            rc = iPodSendCommand(iPodHndl, msg);
        }

        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;

}

/*!
 * \fn iPodGetDBiTunesInfo(U32 iPodID, IPOD_ITUNES_METADATA_TYPE metadataType, IPOD_ITUNES_METADATA_INFO *metadataInfo)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.13
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_ITUNES_METADATA_TYPE metadataType - database metadata type
 * \par INOUT PARAMETERS
 * #IPOD_ITUNES_METADATA_INFO metadataInfo* - database metadata information;Max size is 8
 * \par REPLY PARAMETERS
 * S32 returnCode - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets specified iPod iTunes database metadata information.
 * \note This DB metadata information is updated when the iPod enters remote UI mode.<br>
 * This ensures that the information is updated following an iTunes sync event.<br>
 */
S32 iPodGetDBiTunesInfo(U32 iPodID, IPOD_ITUNES_METADATA_TYPE metadataType, IPOD_ITUNES_METADATA_INFO *metadataInfo)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_DB_ITUNES_INFO_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((metadataInfo == NULL) || (iPodHndl == NULL))
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter: metadataInfo = %p, iPodHndl = %p",metadataInfo,iPodHndl);
    }
    else
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_DB_ITUNES_INFO, (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Metadata type is 0x%02x",metadataType);
            msg[IPOD_POS5] = (U8)metadataType;
            rc = iPodSendCommand(iPodHndl, msg);
        }

        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if(rc == IPOD_OK)
            {
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Metadata type is 0x%02x",metadataType);
                switch(metadataType)
                {
                    case IPOD_ITUNES_UID               : metadataInfo->uID = (U64)iPod_convert_to_little64(&iPodHndl->iAP1Buf[IPOD_POS1]);
                                                         IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "unique Id = %llu",metadataInfo->uID);
                                                         break;
                    case IPOD_ITUNES_LAST_SYNC         : metadataInfo->lastSync.year = (((U16)iPodHndl->iAP1Buf[IPOD_POS6] << IPOD_SHIFT_8) |
                                                                                        (U16)iPodHndl->iAP1Buf[IPOD_POS7]);
                                                         metadataInfo->lastSync.month = iPodHndl->iAP1Buf[IPOD_POS5];
                                                         metadataInfo->lastSync.day = iPodHndl->iAP1Buf[IPOD_POS4];
                                                         metadataInfo->lastSync.hour = iPodHndl->iAP1Buf[IPOD_POS3];
                                                         metadataInfo->lastSync.min = iPodHndl->iAP1Buf[IPOD_POS2];
                                                         metadataInfo->lastSync.sec = iPodHndl->iAP1Buf[IPOD_POS1];
                                                         IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Seconds = %u Minute = %u Hour = %u Day = %u Month = %u Year = %u",
                                                         metadataInfo->lastSync.sec,metadataInfo->lastSync.min,metadataInfo->lastSync.hour,metadataInfo->lastSync.day,
                                                         metadataInfo->lastSync.month,metadataInfo->lastSync.year);
                                                         break;
                    case IPOD_ITUNES_AUDIO_TRACK_COUNT : metadataInfo->audioCount = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS1]);
                                                         IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Audio Track Count = %u",metadataInfo->audioCount);
                                                         break;
                    case IPOD_ITUNES_VIDEO_TRACK_COUNT : metadataInfo->videoCount = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS1]);
                                                         IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Video Count = %u",metadataInfo->videoCount);
                                                         break;
                    case IPOD_ITUNES_AUDIOBOOK_COUNT   : metadataInfo->audiobookCount = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS1]);
                                                         IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Audio Book Count = %u",metadataInfo->audiobookCount);
                                                         break;
                    case IPOD_TOTAL_PHOTO_COUNT        : metadataInfo->photoCount = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS1]);
                                                         IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Photo Count = %u",metadataInfo->photoCount);
                                                         break;
                    default                            : IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unknown Metadata Type");
                                                         break;
                 }
            }
        }
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetUIDTrackInfo(U32 iPodID, U64 uID, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.13
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U64 uID - Unique track identifier<br>
 * #IPOD_TRACK_INFORMATION_TYPE trackType - This is type of track information
 * \par INOUT PARAMETERS
 * #IPOD_TRACK_INFORMATION_CB callback - This is callback of each type
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_UNEXPECTED_ERROR Unexpected error
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets target unique ID information of specified type.
 * \note The device's input buffer must be of sufficient size to accept all requested track information.<br>
 * This function can get only one information.If it want to get the several informations, this function must call several times.
 */
S32 iPodGetUIDTrackInfo(U32 iPodID, U64 uID, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U8 bitmaskLength = ((U8)trackType / IPOD_8BITS);
    U8 length = IPOD_TRACK_INFO_BASE_LENGTH + bitmaskLength;
    U8 bitmask = (U8)(1 << ((U8)trackType % IPOD_8BITS));
    U64 trackIndex = 0;
    U16 count = 1;
    IPOD_TRACK_INFORMATION_DATA infoData;
    U32 chaptCnt = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    memset(&infoData, 0, sizeof(infoData));
    /* Check the parameter */
    if((callback == NULL) || (bitmaskLength > BITMASK_MAX_LENGTH) || (iPodHndl == NULL))
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter: callback = %p, bitmaskLength = %d, iPodHndl = %p",callback,bitmaskLength,iPodHndl);
    }
    else
    {
        /* Allocate for command */
        msg = calloc((length + IPOD_START_LENGTH), sizeof(U8));
        if(msg == NULL)
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "No Memory to allocate for command");
        }
        else
        {
            /*Special operation for chapter times and names*/
            if((trackType == CHAPTER_TIMES) || (trackType == CHAPTER_NAMES))
            {
                U16 chapterCount = 0;
                /* Get the chapter total count of track */
                rc = internal_iPodGetChapterTotalCount(iPodID, TYPE_UID, &chapterCount, uID);
                if(rc == IPOD_OK)
                {
                    if(chapterCount > 0)
                    {
                        infoData.chapterTimes.totalCount = chapterCount;
                        count = chapterCount;
                    }
                    else
                    {
                        infoData.chapterTimes.totalCount = 0;
                    }
                }
                chaptCnt = (U32)chapterCount;
            }

            if(rc == IPOD_OK)
            {
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Unique Id = %llu Tracktype = %d",uID,trackType);
                /* Set command of GetUIDTrackInfo */
                msg[IPOD_POS0] = IPOD_START_OF_PACKET;
                msg[IPOD_POS1] = length;
                msg[IPOD_POS2] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
                msg[IPOD_POS3] = 0;
                msg[IPOD_POS4] = IPOD_EXTENDED_LINGO_GET_UID_TRACK_INFO;
                iPod_convert_to_big64(&msg[IPOD_POS5], uID);
                msg[IPOD_POS13 + bitmaskLength] = bitmask;
                /* expect one more callback to avoid the loss of a callback from the Apple device in the iPodReaderTask
                   (e.g. in case of LYRIC_STRING) */
                iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_UID_TRACK_INFO, (U8)IPOD_LINGO_EXTENDED_INTERFACE, count+1);
                rc = iPodSendCommand(iPodHndl, msg);
            }

            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    trackIndex = iPod_convert_to_little64(iPodHndl->iAP1Buf);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Unique Id = %llu",trackIndex);
                    /* Check the index number */
                    if(trackIndex == uID)
                    {
                        rc = iPodGetTrackInfo(iPodID, (IPOD_TRACK_INFORMATION_CB_INT)callback, &infoData, iPodHndl->iAP1Buf, TYPE_UID, NULL, &chaptCnt);
                    }
                    else
                    {
                        rc = IPOD_ERR_UNEXPECTED_ERROR;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unique Id = %llu returned by device is not same as the one sent by accessory",trackIndex);
                        iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
                    }
                }
            }
            iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
        }
    }

    if(msg != NULL)
    {
        free(msg);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetDBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.13
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 dbIndex - This is start index in database track<br>
 * S32 trackCount - Track count from start index<br>
 * #IPOD_TRACK_INFORMATION_TYPE trackType - Track information type
 * \par INOUT PARAMETERS
 * #IPOD_TRACK_INFORMATION_CB callback - This is callback of each track or chapter or section.
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_UNEXPECTED_ERROR Unexpected error
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the information of specified type of target database index .
 * \note The device's input buffer must be of sufficient size to accept all requested track information.
 */
S32 iPodGetDBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U8 bitmaskLength =(U8)((U8)trackType / IPOD_8BITS);
    U8 length = IPOD_TRACK_INFO_BASE_LENGTH + bitmaskLength;
    U8 bitmask = (U8)(1 << ((U8)trackType % IPOD_8BITS));
    U32 trackIndex = 0;
    U32 chaptCnt = 0;
    U32 loop_index = 0;
    U16 count = 1;
    IPOD_TRACK_INFORMATION_DATA infoData;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    memset(&infoData, 0, sizeof(infoData));

    /* Check the parameter */
    if(((S32)dbIndex < 0) || (callback == NULL) || (trackCount < IPOD_GET_ALL_CAT_ENTRIES) || (trackCount == 0)
       || (bitmaskLength > BITMASK_MAX_LENGTH) || (iPodHndl == NULL))
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter dbIndex = %d, callback = %p, trackCount = %d, bitmaskLength = %d, iPodHndl = %p",
                                           dbIndex,callback,trackCount,bitmaskLength,iPodHndl);
    }
    else
    {
        /* Allocate for command */
        msg = calloc((length + IPOD_START_LENGTH), sizeof(U8));
        if(msg == NULL)
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "No Memory to allocate for command");

        }
        else
        {
            S32 result = 0;
#ifdef IPOD_FAST_DB
            if (trackCount == IPOD_GET_ALL_CAT_ENTRIES)
            {
                /* It gets the all data */
                result = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
                if (result > IPOD_OK)
                {
                    /* get track information for all tracks from the starting DB track index */
                    trackCount = result - (S32)dbIndex;
                }
                else if (result == 0)
                {
                    rc = IPOD_ERROR;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Number of categorized DB records returned by device is 0");
                }
                else
                {
                    rc = result;
                }
            }
            else
            {
                trackCount += (S32)dbIndex;
            }
            
#else /* IPOD_FAST_DB */
            /* It gets the all data */
            result = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
            if(result > IPOD_OK)
            {
                if(trackCount == IPOD_GET_ALL_CAT_ENTRIES)
                {
                    trackCount = result - (S32)dbIndex;
                    rc = IPOD_OK;
                }
                else
                {
                    trackCount += (S32)dbIndex;
                }
            }
            else
            {
                rc = result;
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Number of categorized DB records returned by device is %d",result);
            }
#endif
            /* Loop track total count */
            for(loop_index = dbIndex;
                ((loop_index < (U32)trackCount) && (rc == IPOD_OK));
                loop_index ++)
            {
                /*Special operation for chapter times and names*/
                if((trackType == CHAPTER_TIMES) || (trackType == CHAPTER_NAMES))
                {
                    U16 chapterCount = 0;

                    /* Get the chapter total count of track */
                    rc = internal_iPodGetChapterTotalCount(iPodID, TYPE_DB, &chapterCount, (U64)loop_index);
                    if(rc == IPOD_OK)
                    {
                        if(chapterCount > 0)
                        {
                            infoData.chapterTimes.totalCount = chapterCount;
                            count = chapterCount;
                        }
                        else
                        {
                            infoData.chapterTimes.totalCount = 0;
                        }
                    }
                    chaptCnt = (U32)chapterCount;
                }

                if(rc == IPOD_OK)
                {
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, " Database Start Index = %d trackCount = %u  trackType = %u",dbIndex,trackCount,trackType);
                    /* Set command of GetPBTrackInfo */
                    msg[IPOD_POS0] = IPOD_START_OF_PACKET;
                    msg[IPOD_POS1] = length;
                    msg[IPOD_POS2] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
                    msg[IPOD_POS3] = 0;
                    msg[IPOD_POS4] = IPOD_EXTENDED_LINGO_GET_DB_TRACK_INFO;
                    iPod_convert_to_big32(&msg[IPOD_POS5], loop_index);
                    iPod_convert_to_big32(&msg[IPOD_POS9], 1);
                    msg[IPOD_POS13 + bitmaskLength] = bitmask;
                    /* expect one more callback to avoid the loss of a callback from the Apple device in the iPodReaderTask
                       (e.g. in case of LYRIC_STRING) */
                    iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_DB_TRACK_INFO, (U8)IPOD_LINGO_EXTENDED_INTERFACE, count+1);

                    rc = iPodSendCommand(iPodHndl, msg);
                    if (rc == IPOD_OK)
                    {
                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                        if (rc == IPOD_OK)
                        {
                            trackIndex = iPod_convert_to_little32(iPodHndl->iAP1Buf);
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Database Index= %u",trackIndex);
                            /* Check the index number */
                            if(trackIndex == loop_index)
                            {
                                rc = iPodGetTrackInfo(iPodID, (IPOD_TRACK_INFORMATION_CB_INT)callback, &infoData, iPodHndl->iAP1Buf, TYPE_DB, NULL, &chaptCnt);
                            }
                            else
                            {
                                rc = IPOD_ERR_UNEXPECTED_ERROR;
                                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unexpected error: Track Database Index returned by device = %d",trackIndex);
                            }
                        }
                    }
                    iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
                }
            }
        }
    }

    if(msg != NULL)
    {
        free(msg);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn S32 iPodGetPBTrackInfo(U32 iPodID, U32 pbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.13
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 pbIndex - This is start index in playback track<br>
 * U32 trackCount - This is track count from start index<br>
 * #IPOD_TRACK_INFORMATION_TYPE trackType* - is type of track information
 * \par INOUT PARAMETERS
 * #IPOD_TRACK_INFORMATION_CB callback - is callback of each track or chapter or section.
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_UNEXPECTED_ERROR Unexpected error
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets target playback engine index information of specified type.
 * \note The device's input buffer must be of sufficient size to accept all requested track information.
 */
S32 iPodGetPBTrackInfo(U32 iPodID, U32 pbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U8 bitmaskLength = ((U8)trackType / IPOD_8BITS);
    U8 length = IPOD_TRACK_INFO_BASE_LENGTH + bitmaskLength;
    U8 bitmask = (U8)(1 << ((U8)trackType % IPOD_8BITS));
    U32 trackIndex = 0;
    U32 chaptCnt = 0;
    U32 loop_index = 0;
    U16 count = 1;
    IPOD_TRACK_INFORMATION_DATA infoData;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    memset(&infoData, 0, sizeof(infoData));

   /* Check the parameter */
    if(((S32)pbIndex < 0) || (callback == NULL) || (trackCount < IPOD_GET_ALL_CAT_ENTRIES) || (trackCount == 0)
       || (bitmaskLength > BITMASK_MAX_LENGTH) || (iPodHndl == NULL))
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter pbIndex = %u, callback = %p, trackCount = %d, bitmaskLength = %u, iPodHndl = %p",
                                                   pbIndex,callback,trackCount,bitmaskLength,iPodHndl);
    }

    if(rc == IPOD_OK)
    {
        /* Allocate for command */
        msg = calloc((length + IPOD_START_LENGTH), sizeof(U8));
        if(msg == NULL)
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "No Memory to allocate for command");
        }
    }
    
    if(rc == IPOD_OK)
    {
        S32 result = 0;
        /* It gets the all data */
        result = iPodGetNumPlayingTracks(iPodID);
        if(result > IPOD_OK)
        {
            if(trackCount == IPOD_GET_ALL_CAT_ENTRIES)
            {
                trackCount = result - (S32)pbIndex;
                rc = IPOD_OK;
            }
            else
            {
                trackCount += (S32)pbIndex;
            }
        }
        else if(result == 0)
        {
            rc = IPOD_ERROR;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Number of playing tracks returned by device is zero");
        }
        else
        {
            rc = result;
        }
            
        /* Loop track total count */
        for(loop_index = pbIndex;
            ((loop_index < (U32)trackCount) && (rc == IPOD_OK));
            loop_index++)
        {

            /*Special operation for chapter times and names*/
            if((trackType == CHAPTER_TIMES) || (trackType == CHAPTER_NAMES))
            {
                U16 chapterCount = 0;
                /* Get the chapter total count of track */
                rc = internal_iPodGetChapterTotalCount(iPodID, TYPE_PB, &chapterCount, (U64)loop_index);
                if(rc == IPOD_OK)
                {
                    if(chapterCount > 0)
                    {
                        infoData.chapterTimes.totalCount = chapterCount;
                        count = chapterCount;
                    }
                    else
                    {
                        infoData.chapterTimes.totalCount = 0;
                    }
                }
                chaptCnt = (U32)chapterCount;
            }

            if((rc == IPOD_OK) && (msg != NULL))
            {
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Playing start index = %d Track Count = %d Track Information type = 0x%02x",pbIndex,trackCount,trackType);
                /* Set command of GetPBTrackInfo */
                msg[IPOD_POS0] = IPOD_START_OF_PACKET;
                msg[IPOD_POS1] = length;
                msg[IPOD_POS2] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
                msg[IPOD_POS3] = 0;
                msg[IPOD_POS4] = IPOD_EXTENDED_LINGO_GET_PB_TRACK_INFO;
                iPod_convert_to_big32(&msg[IPOD_POS5], loop_index);
                iPod_convert_to_big32(&msg[IPOD_POS9], 1);
                msg[IPOD_POS13 + bitmaskLength] = bitmask;
                /* expect one more callback to avoid the loss of a callback from the Apple device in the iPodReaderTask
                   (e.g. in case of LYRIC_STRING) */
                iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_PB_TRACK_INFO, (U8)IPOD_LINGO_EXTENDED_INTERFACE, count+1);
                rc = iPodSendCommand(iPodHndl, msg);
            }

            if( (rc == IPOD_OK) && (iPodHndl != NULL) )
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    trackIndex = iPod_convert_to_little32(iPodHndl->iAP1Buf);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Playback Index = %u",trackIndex);
                    /* Check the index number */
                    if(trackIndex == loop_index)
                    {
                        rc = iPodGetTrackInfo(iPodID, (IPOD_TRACK_INFORMATION_CB_INT)callback, &infoData, iPodHndl->iAP1Buf, TYPE_PB, NULL, &chaptCnt);
                    }
                    else
                    {
                        rc = IPOD_ERR_UNEXPECTED_ERROR;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unexpected error: Track Playback Index returned by device = %u",trackIndex);
                        iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
                    }
                }
            }
            iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
        }
    }

    if(msg != NULL)
    {
        free(msg);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetBulkUIDTrackInfo(U32 iPodID, U64 uID, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.13
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U64 uID - Unique track identifier<br>
 * #IPOD_TRACK_INFORMATION_BITFIELD trackType - This is a bitfield of track information
 * \par INOUT PARAMETERS
 * #IPOD_TRACK_INFORMATION_CB callback - This is callback of each type
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_UNEXPECTED_ERROR Unexpected error
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets target unique ID information of specified types.
 * \note The device's input buffer must be of sufficient size to accept all requested track information.<br>
 * This function can get several information with one call.
 */
S32 iPodGetBulkUIDTrackInfo(U32 iPodID, U64 uID, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_OK;
    U8 msg[BITMASK_MAX_LENGTH + IPOD_TRACK_INFO_BASE_LENGTH + IPOD_START_LENGTH] = {0};
    U64 trackIndex = 0;
    U64 prevTrackIndex = 0;
    U32 totalCallback = 0;
    U16 count = 0;
    IPOD_TRACK_INFORMATION_DATA infoData;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    U32 chaptCnt = 0;

    memset(&infoData, 0, sizeof(infoData));

    /* Check the parameter */
    if((callback == NULL) || (iPodHndl == NULL))
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter: callback = %p iPodHndl = %p",callback,iPodHndl);
    }
    else
    {
        if ((trackType.track_info.CHAPTER_NAMES == 1)
            ||(trackType.track_info.CHAPTER_TIMES == 1))
        {
            /* set bitmask to get the chapter count */
            trackType.track_info.CHAPTER_COUNT = 1;
        }

        /* get expected number of callback */
        totalCallback = iPodBitcount(trackType.bitmask);

        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Unique Track Id = %llu ",uID);
        /* Set command of GetUIDTrackInfo */
        msg[IPOD_POS0] = IPOD_START_OF_PACKET;
        msg[IPOD_POS1] = BITMASK_MAX_LENGTH + IPOD_TRACK_INFO_BASE_LENGTH;
        msg[IPOD_POS2] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
        msg[IPOD_POS3] = 0;
        msg[IPOD_POS4] = IPOD_EXTENDED_LINGO_GET_UID_TRACK_INFO;
        iPod_convert_to_big64(&msg[IPOD_POS5], uID);
        memcpy(&msg[IPOD_POS13], &trackType.bitmask, sizeof(trackType.bitmask));
        /* expect one more callback to avoid the loss of a callback from the Apple device in the iPodReaderTask
           (e.g. in case of LYRIC_STRING) */
        iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_UID_TRACK_INFO, (U8)IPOD_LINGO_EXTENDED_INTERFACE, totalCallback+1);

        rc = iPodSendCommand(iPodHndl, msg);
        for (count = 0; ((count < totalCallback) && (rc == IPOD_OK)); count++)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if (rc == IPOD_OK)
            {
                trackIndex = iPod_convert_to_little64(iPodHndl->iAP1Buf);
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Unique Track Id= %llu",trackIndex);
                /* Check the index number */
                if (trackIndex == uID)
                {
                    if(prevTrackIndex != trackIndex)
                    {
                        chaptCnt = 0;
                    }

                    rc = iPodGetTrackInfo(iPodID, (IPOD_TRACK_INFORMATION_CB_INT)callback, &infoData, iPodHndl->iAP1Buf, TYPE_UID, &trackType, &chaptCnt);
                    prevTrackIndex = trackIndex;
                }
                else
                {
                    rc = IPOD_ERR_UNEXPECTED_ERROR;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unique Id = %llu returned by device is not same as the one sent by accessory",trackIndex);
                }
            }
        }
        iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetBulkDBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.13
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 dbIndex - This is start index in database track<br>
 * S32 trackCount - Track count from start index<br>
 * #IPOD_TRACK_INFORMATION_BITFIELD trackType - Track information bitfield
 * \par INOUT PARAMETERS
 * #IPOD_TRACK_INFORMATION_CB callback - This is callback of each track or chapter or section.
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_UNEXPECTED_ERROR Unexpected error
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the information of specified types of target database index.
 * \note The device's input buffer must be of sufficient size to accept all requested track information.
 */
S32 iPodGetBulkDBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_OK;
    U8 msg[BITMASK_MAX_LENGTH + IPOD_TRACK_INFO_BASE_LENGTH + IPOD_START_LENGTH] = {0};
    U32 trackIndex = 0;
    U32 prevTrackIndex = 0;
    U32 totalCallback = 0;
    U16 count = 0;
    IPOD_TRACK_INFORMATION_DATA infoData;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    U32 chaptCnt = 0;

    memset(&infoData, 0, sizeof(infoData));

    /* Check the parameter */
    if (((S32)dbIndex < 0) || (callback == NULL) || (trackCount < IPOD_GET_ALL_CAT_ENTRIES) || (trackCount == 0)
        || (iPodHndl == NULL))
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter dbIndex = %d, callback = %p, trackCount = %u, iPodHndl = %p",
                                                   dbIndex,callback,trackCount,iPodHndl);
    }
    else
    {
        /* Check the extended interface mode */
        rc = iPodIsInAdvancedMode(iPodHndl);

        if (rc == IPOD_OK)
        {
            S32 result = 0;
#ifdef IPOD_FAST_DB
            if (trackCount == IPOD_GET_ALL_CAT_ENTRIES)
            {
                /* It gets the all data */
                result = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
                if (result > IPOD_OK)
                {
                    /* get track information for all tracks from the starting DB track index */
                    trackCount = result - (S32)dbIndex;
                }
                else if (result == 0)
                {
                    rc = IPOD_ERROR;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Number of Categorized DB records returned by device is zero");
                }
                else
                {
                    rc = result;
                }
            }
#else /* IPOD_FAST_DB */
            /* It gets the all data */
            result = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
            if (result > IPOD_OK)
            {
                if (trackCount == IPOD_GET_ALL_CAT_ENTRIES)
                {
                    /* get track information for all tracks from the starting DB track index */
                    trackCount = result - (S32)dbIndex;
                }
                else if (trackCount > result)
                {
                    /* requested more tracks than there are available */
                    rc = IPOD_BAD_PARAMETER;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Requested more tracks than there are available");
                }
                else
                {
                    trackCount = trackCount;
                }
            }
            else if (result == 0)
            {
                rc = IPOD_ERROR;
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Number of Categorized DB records returned by device is zero");
            }
            else
            {
                rc = result;
            }
#endif /* IPOD_FAST_DB */

            if ((trackType.track_info.CHAPTER_NAMES == 1)
                ||(trackType.track_info.CHAPTER_TIMES == 1))
            {
                /* set bitmask to get the chapter count */
                trackType.track_info.CHAPTER_COUNT = 1;
            }

            /* get expected number of callback */
            totalCallback = (iPodBitcount(trackType.bitmask)  * (U32)trackCount);

            if (rc == IPOD_OK)
            {
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Database start index = %d Track Count = %d",dbIndex,trackCount);
                /* Set command of GetDBTrackInfo */
                msg[IPOD_POS0] = IPOD_START_OF_PACKET;
                msg[IPOD_POS1] = BITMASK_MAX_LENGTH + IPOD_TRACK_INFO_BASE_LENGTH;
                msg[IPOD_POS2] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
                msg[IPOD_POS3] = 0;
                msg[IPOD_POS4] = IPOD_EXTENDED_LINGO_GET_DB_TRACK_INFO;
                iPod_convert_to_big32(&msg[IPOD_POS5], dbIndex);
                iPod_convert_to_big32(&msg[IPOD_POS9], (U32)trackCount);
                memcpy(&msg[IPOD_POS13], &trackType.bitmask, sizeof(trackType.bitmask));
                /* expect one more callback to avoid the loss of a callback from the Apple device in the iPodReaderTask
                   (e.g. in case of LYRIC_STRING) */
                iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_DB_TRACK_INFO, (U8)IPOD_LINGO_EXTENDED_INTERFACE, totalCallback+1);

                rc = iPodSendCommand(iPodHndl, msg);
                IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns:rc = %d",rc);
                for (count = 0; ((count < totalCallback) && (rc == IPOD_OK)); count++)
                {
                    memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                    rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "iPodWaitAndGetResponseFixedSize() returns : rc = %d",rc);
                    if (rc == IPOD_OK)
                    {
                        trackIndex = iPod_convert_to_little32(iPodHndl->iAP1Buf);
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Database Index= %d",trackIndex);
                        /* Check the index number */
                        if (trackIndex < (dbIndex + (U32)trackCount))
                        {
                            if(prevTrackIndex != trackIndex)
                            {
                                chaptCnt = 0;
                            }

                            rc = iPodGetTrackInfo(iPodID, (IPOD_TRACK_INFORMATION_CB_INT)callback, &infoData, iPodHndl->iAP1Buf, TYPE_DB, &trackType, &chaptCnt);
                            prevTrackIndex = trackIndex;
                        }
                        else
                        {
                            rc = IPOD_ERR_UNEXPECTED_ERROR;
                            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unexpected error: Track Database Index returned by device = %d",trackIndex);
                        }
                    }
                }
                iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
            }
        }
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn S32 iPodGetBulkPBTrackInfo(U32 iPodID, U32 pbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.13
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 pbIndex - This is start index in playback track<br>
 * U32 trackCount - This is track count from start index<br>
 * #IPOD_TRACK_INFORMATION_BITFIELD trackType - is bitfield of track information
 * \par INOUT PARAMETERS
 * #IPOD_TRACK_INFORMATION_CB callback - is callback of each track or chapter or section.
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_UNEXPECTED_ERROR Unexpected error
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets target playback engine index information of specified types.
 * \note The device's input buffer must be of sufficient size to accept all requested track information.
 */
S32 iPodGetBulkPBTrackInfo(U32 iPodID, U32 pbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_OK;
    U8 msg[BITMASK_MAX_LENGTH + IPOD_TRACK_INFO_BASE_LENGTH + IPOD_START_LENGTH] = {0};
    U32 trackIndex = 0;
    U32 prevTrackIndex = 0;
    U32 totalCallback = 0;
    U16 count = 0;
    IPOD_TRACK_INFORMATION_DATA infoData;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    U32 chaptCnt = 0;

    memset(&infoData, 0, sizeof(infoData));

   /* Check the parameter */
    if (((S32)pbIndex < 0) || (callback == NULL) || (trackCount < IPOD_GET_ALL_CAT_ENTRIES) || (trackCount == 0)
        || (iPodHndl == NULL))
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter pbIndex = %d, callback = %p, trackCount = %d, iPodHndl = %p",
                                                           pbIndex,callback,trackCount,iPodHndl);
    }
    else
    {
        S32 result = 0;
#ifdef IPOD_FAST_DB
        if (trackCount == IPOD_GET_ALL_CAT_ENTRIES)
        {
             /* It gets the all data */
            result = iPodGetNumPlayingTracks(iPodID);
            if (result > IPOD_OK)
            {
                /* get track information for all tracks from the starting PB track index */
                trackCount = result - (S32)pbIndex;
            }
            else if (result == 0)
            {
                rc = IPOD_ERROR;
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Number of playing tracks returned by device is zero");
            }
            else
            {
                rc = result;
            }
        }
#else /* IPOD_FAST_DB */
        /* It gets the all data */
        result = iPodGetNumPlayingTracks(iPodID);
        if (result > IPOD_OK)
        {
            if (trackCount == IPOD_GET_ALL_CAT_ENTRIES)
            {
                /* get track information for all tracks from the starting PB track index */
                trackCount = result - (S32)pbIndex;
            }
            else if (trackCount > result)
            {
                /* requested more tracks than there are available */
                rc = IPOD_BAD_PARAMETER;
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Requested more tracks than there are available");
            }
            else
            {
                trackCount = trackCount;
            }
        }
        else if (result == 0)
        {
            rc = IPOD_ERROR;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Number of playing tracks returned by device is zero");
        }
        else
        {
            rc = result;
        }
#endif /* IPOD_FAST_DB */

        if ((trackType.track_info.CHAPTER_NAMES == 1)
            |(trackType.track_info.CHAPTER_TIMES == 1))
        {
            /* set bitmask to get the chapter count */
            trackType.track_info.CHAPTER_COUNT = 1;
        }

        /* get expected number of callback */
        totalCallback = (iPodBitcount(trackType.bitmask)  * (U32)trackCount);

        if (rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Playing start index = %d Track Count = %d",pbIndex,trackCount);
            /* Set command of GetPBTrackInfo */
            msg[IPOD_POS0] = IPOD_START_OF_PACKET;
            msg[IPOD_POS1] = BITMASK_MAX_LENGTH + IPOD_TRACK_INFO_BASE_LENGTH;
            msg[IPOD_POS2] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
            msg[IPOD_POS3] = 0;
            msg[IPOD_POS4] = IPOD_EXTENDED_LINGO_GET_PB_TRACK_INFO;
            iPod_convert_to_big32(&msg[IPOD_POS5], pbIndex);
            iPod_convert_to_big32(&msg[IPOD_POS9], (U32)trackCount);
            memcpy(&msg[IPOD_POS13], &trackType.bitmask, sizeof(trackType.bitmask));
            /* expect one more callback to avoid the loss of a callback from the Apple device in the iPodReaderTask
               (e.g. in case of LYRIC_STRING) */
            iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_PB_TRACK_INFO, (U8)IPOD_LINGO_EXTENDED_INTERFACE, totalCallback+1);

            rc = iPodSendCommand(iPodHndl, msg);
            for (count = 0; ((count < totalCallback) && (rc == IPOD_OK)); count++)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (rc == IPOD_OK)
                {
                    trackIndex = iPod_convert_to_little32(iPodHndl->iAP1Buf);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Playing start Index = %d",trackIndex);
                    /* Check the index number */
                    if (trackIndex < (pbIndex + (U32)trackCount))
                    {
                        if(prevTrackIndex != trackIndex)
                        {
                            chaptCnt = 0;
                        }

                        rc = iPodGetTrackInfo(iPodID, (IPOD_TRACK_INFORMATION_CB_INT)callback, &infoData, iPodHndl->iAP1Buf, TYPE_PB, &trackType, &chaptCnt);
                        prevTrackIndex = trackIndex;
                    }
                    else
                    {
                        rc = IPOD_ERR_UNEXPECTED_ERROR;
                        IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "Unexpected error: Track Playback Index returned by device = %d",trackIndex);
                    }
                }
            }
            iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
        }
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


LOCAL S32 internal_iPodRetrieveCategorizedDBRecords(U32 iPodID,
                                              IPOD_CATEGORY category,
                                              U32 start,
                                              U32 count,
                                              U32 numDBRecords,
                                              const IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback)
{
    U8 msg[]            = {IPOD_RETR_CAT_DB_RECORDS_CMD};
    S32 rc              = IPOD_OK;
    U32 i               = 0;
    U32 resCount        = 0;
    U32 trackIndex      = 0;
    S32 responseMsgLen  = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if ((iPodHndl != NULL) && (callback != NULL) && ((start + count) <= numDBRecords))
    {
        resCount = (U32)count;

        /* insert DB Category into msg array */
        msg[IPOD_POS5] = (U8)category;

#ifdef IPOD_DB_BULK_TRANSFER
        iPod_convert_to_big32(&msg[IPOD_POS6], start);
        iPod_convert_to_big32(&msg[IPOD_POS10], count);
        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "category = 0x%02x start index = %d count = %d",category,start,count);

        iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_CATEGORIZED_DATABASE_RECORD, (U8)IPOD_LINGO_EXTENDED_INTERFACE, resCount);
        rc = iPodSendCommand(iPodHndl, msg);

        for (i = 0; (i < resCount) && (rc == IPOD_OK); i++)
        {
            responseMsgLen = iPodWaitAndGetResponseLength(iPodHndl);
            if (responseMsgLen > IPOD_POS4)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                iPodGetResponseData(iPodHndl, iPodHndl->iAP1Buf);
                trackIndex = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS0]);
                if (responseMsgLen <= iPodHndl->iAP1MaxPayloadSize)
                {
                    responseMsgLen--;
                    /* PRQA: Lint Message 661: The allocated memory size was checked and found to be okay. */
                    /* PRQA: Lint Message 662: The allocated memory size was checked and found to be okay. */
                    iPodHndl->iAP1Buf[responseMsgLen] = 0;/*lint !e661 !e662 */
                }
                else
                {
                    /* PRQA: Lint Message 661: The allocated memory size was checked and found to be okay. */
                    /* PRQA: Lint Message 662: The allocated memory size was checked and found to be okay. */
                    iPodHndl->iAP1Buf[iPodHndl->iAP1MaxPayloadSize - 2] = 0;/*lint !e661 !e662 */
                }
                        
                callback(trackIndex, &iPodHndl->iAP1Buf[IPOD_POS4], iPodID);
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "trackIndex= %u trackname = %s",trackIndex,&iPodHndl->iAP1Buf[IPOD_POS4]);
            }
            else
            {
                if(IPOD_OK > responseMsgLen)
                {
                    rc = responseMsgLen;
                }
                else
                {
                    break;
                }
            }
        }

#else  /* IPOD_DB_BULK_TRANSFER */

        /* Request in any case only one item, because iPod models sends
           their DB entries continuously without waiting on a master acknowledge.
           This cause on large DB requests data loss */
        msg[IPOD_POS10] = 0;
        msg[IPOD_POS11] = 0;
        msg[IPOD_POS12] = 0;
        msg[IPOD_POS13] = 1;

        for (i = 0; (i < resCount) && (rc == IPOD_OK); i++)
        {
            U32 offset          = 0;

            /* calculate offset and insert new start index into msg array */
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_CATEGORIZED_DATABASE_RECORD, (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            offset = start + i;
            iPod_convert_to_big32(&msg[IPOD_POS6], offset);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "category = 0x%02x offset = %d",category,offset);
            rc = iPodSendCommand(iPodHndl, msg);

            if (rc == IPOD_OK)
            {
                responseMsgLen = iPodWaitAndGetResponseLength(iPodHndl);

                if (responseMsgLen > IPOD_POS4)
                {
                    memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                    iPodGetResponseData(iPodHndl, iPodHndl->iAP1Buf);
                    trackIndex = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS0]);
                    if (responseMsgLen <= iPodHndl->iAP1MaxPayloadSize)
                    {
                        responseMsgLen--;
                        /* PRQA: Lint Message 661: The allocated memory size was checked and found to be okay. */
                        /* PRQA: Lint Message 662: The allocated memory size was checked and found to be okay. */
                        iPodHndl->iAP1Buf[responseMsgLen] = 0;/*lint !e661 !e662 */
                    }
                    else
                    {
                        /* PRQA: Lint Message 661: The allocated memory size was checked and found to be okay. */
                        /* PRQA: Lint Message 662: The allocated memory size was checked and found to be okay. */
                        iPodHndl->iAP1Buf[IPOD_CMD_MAX_MSG_LEN_REPORT] = 0;/*lint !e661 !e662 */
                    }
                        
                    callback(trackIndex, &iPodHndl->iAP1Buf[IPOD_POS4], iPodID);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "trackIndex= %d trackname = %s",trackIndex,iPodHndl->iAP1Buf[IPOD_POS4]);
                }
                else
                {
                    rc = responseMsgLen;
                    break;
                }
            }
        }
#endif   /* IPOD_DB_BULK_TRANSFER */
        iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter iPodHndl = %p callback = %p start = %d count = %d",iPodHndl,callback,start,count);
    }

    return rc;

}


LOCAL S32 iPodGetTrackInfo(U32 iPodID, 
                           IPOD_TRACK_INFORMATION_CB_INT callback,
                           const IPOD_TRACK_INFORMATION_DATA *infoInData,
                           U8 *buf,
                           IPOD_TRACK_TYPE type,
                           IPOD_TRACK_INFORMATION_BITFIELD* pbitfield,
                           U32 *chapterCount)
{
    S32 rc = IPOD_OK;
    U8 dataPos = IPOD_POS1;
    IPOD_TRACK_INFORMATION_TYPE infoType = CAPABILITIES;
    U64 trackIndex = 0;
    U16 expectedId = 0;
    IPOD_TRACK_INFORMATION_DATA infoOutData;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    /* Initialize the structure */
    memset(&infoOutData, 0, sizeof(infoOutData));

    if(type == TYPE_UID)
    {
        trackIndex = iPod_convert_to_little64(buf);
        infoType = (IPOD_TRACK_INFORMATION_TYPE)buf[IPOD_UID_INFO_POS];
        dataPos += IPOD_UID_INFO_POS;
        expectedId = IPOD_EXTENDED_LINGO_RET_UID_TRACK_INFO;
    }
    else if(type == TYPE_DB)
    {
        trackIndex = (U64)iPod_convert_to_little32(buf);
        infoType = (IPOD_TRACK_INFORMATION_TYPE)buf[IPOD_DB_PB_INFO_POS];
        dataPos += IPOD_DB_PB_INFO_POS;
        expectedId = IPOD_EXTENDED_LINGO_RET_DB_TRACK_INFO;
    }
    else if(type == TYPE_PB)
    {
        trackIndex = (U64)iPod_convert_to_little32(buf);
        infoType = (IPOD_TRACK_INFORMATION_TYPE)buf[IPOD_DB_PB_INFO_POS];
        dataPos += IPOD_DB_PB_INFO_POS;
        expectedId = IPOD_EXTENDED_LINGO_RET_PB_TRACK_INFO;
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter: Unknown Track Type");
    }

    if((rc == IPOD_OK) && (iPodHndl != NULL) && (callback != NULL) && (infoInData != NULL))
    {
        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Information Type = 0x%02x",infoType);
        switch(infoType)
        {
        case CAPABILITIES         :
            {
                U32 *tmpData = (U32*) &infoOutData.caps;
                *tmpData = iPod_convert_to_little32(&buf[dataPos]);
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Capabilities = 0x%08x",*tmpData);
                break;
            }

        case TRACK_NAME           :
            infoOutData.trackName = &buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Name = %s",infoOutData.trackName);
            break;

        case ARTIST_NAME          :
            infoOutData.artistName = &buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Artist Name = %s",infoOutData.artistName);
            break;

        case ALBUM_NAME           :
            infoOutData.albumName = &buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Album Name = %s",infoOutData.albumName);
            break;

        case GENRE_NAME           :
            infoOutData.genreName = &buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Genre Name = %s",infoOutData.genreName);
            break;

        case COMPOSER_NAME        :
            infoOutData.composerName = &buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Composer Name = %s",infoOutData.composerName);
            break;

        case DURATION             :
            infoOutData.duration = iPod_convert_to_little32(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Total Track Duration = %d",infoOutData.duration);
            break;

        case UID                  :
            infoOutData.trackUID = iPod_convert_to_little64(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Unique Track Id = %llu",infoOutData.trackUID);
            break;

        case CHAPTER_COUNT        :
            infoOutData.chapterCount = iPod_convert_to_little16(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Chapter Count = %d",infoOutData.chapterCount);
            if(chapterCount != NULL)
            {
                if (infoOutData.chapterCount > 0)
                {
                    *chapterCount = (U32)infoOutData.chapterCount;
                    /* increase number of expected callback in case of bulk request
                       which is indicated by a valid bitfield pointer */
                    if(pbitfield != NULL)
                    {
                        U32 callbackCount = 0;

                        if(pbitfield->track_info.CHAPTER_TIMES == 1)
                        {
                            /* decrease, because one callback is already expected */
                            callbackCount += *chapterCount - 1;
                        }
                        if(pbitfield->track_info.CHAPTER_NAMES == 1)
                        {
                            /* decrease, because one callback is already expected */
                            callbackCount += *chapterCount - 1;
                        }

                        rc = iPodIncreaseExpectedCmdIdEx(iPodHndl,
                                                         expectedId,
                                                         (U8)IPOD_LINGO_EXTENDED_INTERFACE,
                                                         callbackCount);
                    }
                }
                else
                {
                    *chapterCount = 0;
                }
            }
            else
            {
                rc = IPOD_ERR_PAR;
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter: chapterCount is NULL");
            }
            break;

        case CHAPTER_TIMES        :
            {
                S32 result = IPOD_OK;
                U32 count = 0;
                if(chapterCount != NULL)
                {
                    if(pbitfield != NULL)
                    {
                        /* If chapterTimes is requested, also chapterCount must be requested.
                           chapterCount is valid for the current track and is automatically provided to the caller as callback */
                        infoOutData.chapterTimes.totalCount = *chapterCount;
                    }
                    else
                    {
                        /* In case of non-bulk request, chapterCount will be not automatically provided to the caller as callback */
                        *chapterCount = infoInData->chapterTimes.totalCount;
                    }

                    if(*chapterCount == 0)
                    {
                        infoOutData.chapterTimes.chapterIndex = 0;
                        infoOutData.chapterTimes.offset = 0;
                        infoOutData.chapterTimes.totalCount = 0;
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "No Chapters as chapter count is 0");
                        callback(trackIndex, infoType, &infoOutData, iPodHndl->id);
                    }
                    else
                    {
                        /* Loop chapter times total count */
                        do
                        {
                            infoOutData.chapterTimes.chapterIndex = iPod_convert_to_little16(&buf[dataPos]);
                            infoOutData.chapterTimes.offset = iPod_convert_to_little32(&buf[dataPos + IPOD_POS2]);
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Chapter Index = %u Chapter Offset = %u",infoOutData.chapterTimes.chapterIndex, infoOutData.chapterTimes.offset);
                            callback(trackIndex,infoType, &infoOutData, iPodHndl->id);
                            if(count != (*chapterCount -1))
                            {
                                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                                if(result != IPOD_OK)
                                {
                                    rc = result;
                                }
                            }
                            count++;
                        } while((count < *chapterCount) && (rc == IPOD_OK));
                    }
                }
                else
                {
                    rc = IPOD_ERR_PAR;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter : chapterCount is NULL ");
                }
                infoOutData.chapterTimes.chapterIndex = 0;
                infoOutData.chapterTimes.offset = 0;
                callback = NULL;
                break;
            }
        case CHAPTER_NAMES        :
            {
                S32 result = IPOD_OK;
                U32 count = 0;

                if(chapterCount != NULL)
                {
                    if(pbitfield != NULL)
                    {
                        /* If chapterNames is requested, also chapterCount must be requested.
                           chapterCount is valid for the current track and is automatically provided to the caller as callback */
                        infoOutData.chapterNames.totalCount = *chapterCount;
                    }
                    else
                    {
                        /* In case of non-bulk request, chapterCount will be not automatically provided to the caller as callback */
                        *chapterCount = infoInData->chapterNames.totalCount;
                    }

                    if(*chapterCount == 0)
                    {
                        infoOutData.chapterNames.chapterIndex = 0;
                        infoOutData.chapterNames.totalCount = 0;
                        iPodHndl->iAP1Buf[0] = '\0';
                        callback(trackIndex, infoType, &infoOutData, iPodHndl->id);
                    }
                    else
                    {
                        /* Loop chapter names total count*/
                        do
                        {
                            infoOutData.chapterNames.chapterIndex = iPod_convert_to_little16(&buf[dataPos]);
                            infoOutData.chapterNames.name = &buf[dataPos + IPOD_POS2];
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, " Chapter Index = %u Chapter Names = %s",infoOutData.chapterNames.chapterIndex,infoOutData.chapterNames.name);
                            callback(trackIndex,infoType, &infoOutData, iPodHndl->id);

                            if(count != (*chapterCount -1))
                            {
                                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                                if(result != IPOD_OK)
                                {
                                    rc = result;
                                 }
                            }
                            count++;
                        } while((count < *chapterCount) && (rc == IPOD_OK));
                    }
                }
                else
                {
                    rc = IPOD_ERR_PAR;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter : chapterCount is NULL");
                }
                callback = NULL;
                break;
            }

        case LYRIC_STRING         :
            {
                S32 result = IPOD_OK;
                U16 count = 0;
                infoOutData.lyrics.maxSection = iPod_convert_to_little16(&buf[dataPos + IPOD_POS2]);
                if(infoOutData.lyrics.maxSection > 0)
                {
                    rc = iPodIncreaseExpectedCmdIdEx(iPodHndl,
                                                     expectedId,
                                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE,
                                                     (U32)infoOutData.lyrics.maxSection);
                }

                do
                {
                    if(rc == IPOD_OK)
                    {
                        infoOutData.lyrics.section = iPod_convert_to_little16(&buf[dataPos]);
                        infoOutData.lyrics.lyric = &buf[dataPos + IPOD_POS4];
                        callback(trackIndex,infoType, &infoOutData, iPodHndl->id);
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Current Track Lyrics section index = %u Maximum Track Lyrics section Index = %u Lyrics Data String = %s",
                                        infoOutData.lyrics.section,infoOutData.lyrics.maxSection,infoOutData.lyrics.lyric);
                        if(count < infoOutData.lyrics.maxSection)
                        {
                            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                            result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                            IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "iPodWaitAndGetResponseLength() returns:result = %d",result);
                            if(result != IPOD_OK)
                            {
                                rc = result;
                            }
                        }
                    }
                    count++;
                } while((count <= infoOutData.lyrics.maxSection) && (rc == IPOD_OK));
                callback = NULL;
                break;
            }

        case DESCRIPTION          :
            infoOutData.description = &buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Description = %s",infoOutData.description);
            break;

        case ALBUM_TRACK_INDEX    :
            infoOutData.albumTrackIndex = iPod_convert_to_little16(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Album Track Index = %u",infoOutData.albumTrackIndex);
            break;

        case DISC_SET_ALBUM_INDEX :
            infoOutData.discSetAlbumIndex = iPod_convert_to_little16(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Disc Set Album Index = %u",infoOutData.discSetAlbumIndex);
            break;

        case PLAY_COUNT           :
            infoOutData.playCount = iPod_convert_to_little32(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Play Count = %u",infoOutData.playCount);
            break;

        case SKIP_COUNT           :
            infoOutData.skipCount = iPod_convert_to_little32(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Skip Count = %u",infoOutData.skipCount);
            break;

        case PODCAST_DATA         :
            infoOutData.podcastReleaseDate.sec = buf[dataPos];
            infoOutData.podcastReleaseDate.min = buf[dataPos + IPOD_POS1];
            infoOutData.podcastReleaseDate.hour = buf[dataPos + IPOD_POS2];
            infoOutData.podcastReleaseDate.day = buf[dataPos + IPOD_POS3];
            infoOutData.podcastReleaseDate.month = buf[dataPos + IPOD_POS4];
            infoOutData.podcastReleaseDate.year = iPod_convert_to_little16(&buf[dataPos + IPOD_POS5]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Podcast Release date: year = %u month = %u day = %u hour = %u min = %u sec = %u",infoOutData.podcastReleaseDate.year,
                     infoOutData.podcastReleaseDate.month,infoOutData.podcastReleaseDate.day,infoOutData.podcastReleaseDate.hour,infoOutData.podcastReleaseDate.min,infoOutData.podcastReleaseDate.sec);
            break;

        case LAST_PLAYED_DATA     :
            infoOutData.lastPlayedDate.sec = buf[dataPos];
            infoOutData.lastPlayedDate.min = buf[dataPos + IPOD_POS1];
            infoOutData.lastPlayedDate.hour = buf[dataPos + IPOD_POS2];
            infoOutData.lastPlayedDate.day = buf[dataPos + IPOD_POS3];
            infoOutData.lastPlayedDate.month = buf[dataPos + IPOD_POS4];
            infoOutData.lastPlayedDate.year = iPod_convert_to_little16(&buf[dataPos + IPOD_POS5]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Last Played date year = %u month = %u day = %u hour = %u min = %u sec = %u",infoOutData.lastPlayedDate.year,
                    infoOutData.lastPlayedDate.month,infoOutData.lastPlayedDate.day,infoOutData.lastPlayedDate.hour,infoOutData.lastPlayedDate.min,infoOutData.lastPlayedDate.sec);
            break;

        case YEAR                 :
            infoOutData.yaer = iPod_convert_to_little16(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Year in which track was released = %u",infoOutData.yaer);
            break;

        case STAR_RATING          :
            infoOutData.starRating = buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Star Rating = %u",infoOutData.starRating);
            break;

        case SERIES_NAME          :
            infoOutData.seriesName = &buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Series Name = %s",infoOutData.seriesName);
            break;

        case SEASON_NUMBER        :
            infoOutData.seasonNumber = iPod_convert_to_little16(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Season Number = %u",infoOutData.seasonNumber);
            break;

        case TRACK_VOLUME_ADJUST  :
            infoOutData.Volume = buf[dataPos];
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Volume adjust = %u",infoOutData.Volume);
            break;

        case EQ_PRESET            :
            infoOutData.EQPreset = iPod_convert_to_little16(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Equalizer Preset Index = %u",infoOutData.EQPreset);
            break;

        case SAMPLE_RATE          :
            infoOutData.dataRate = iPod_convert_to_little32(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Bit Rate = %u",infoOutData.dataRate);
            break;

        case BOOKMARK_OFFSET      :
            infoOutData.bookmarkOffset = iPod_convert_to_little16(&buf[dataPos]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Bookmark Offset from start of track in ms = %u",infoOutData.bookmarkOffset);
            break;

        case START_AND_STOP_OFFSET:
            infoOutData.timeOffset.startOffset = iPod_convert_to_little32(&buf[dataPos]);
            infoOutData.timeOffset.stopOffset = iPod_convert_to_little32(&buf[dataPos + IPOD_POS4]);
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Start time Offset = %u Stop time Offset = %u",infoOutData.timeOffset.startOffset,infoOutData.timeOffset.stopOffset);
            break;

        default                   :
            rc = IPOD_ERR_UNEXPECTED_ERROR;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unknown Track Information Type ");
            break;

        }

        if(rc != IPOD_OK)
        {
            iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);
        }

        if((callback != NULL) && (rc == IPOD_OK))
        {
            callback(trackIndex,infoType, &infoOutData, iPodHndl->id);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Parameter Error iPodHndl = %p callback = %p infoInData = %p",iPodHndl,callback,infoInData);
    }

    return rc;
}

LOCAL S32 internal_iPodGetChapterTotalCount(U32 iPodID, IPOD_TRACK_TYPE type, U16 *totalCount, U64 trackIndex)
{
    S32 rc = IPOD_ERROR;

    U8 msgUID[] = {IPOD_UID_CHAPTER_TOTAL_CMD};
    U8 msgDB[] = {IPOD_DB_CHAPTER_TOTAL_CMD};
    U8 msgPB[] = {IPOD_PB_CHAPTER_TOTAL_CMD};
    U32 returned_index = 0;
    U64 uID = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (totalCount != NULL))
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)((U16)IPOD_EXTENDED_LINGO_GET_UID_TRACK_INFO + (U16)type), (U8)IPOD_LINGO_EXTENDED_INTERFACE);
        if(type == TYPE_UID)
        {
            iPod_convert_to_big64(&msgUID[IPOD_POS5], trackIndex);
            rc = iPodSendCommand(iPodHndl, msgUID);
        }
        else if(type == TYPE_DB)
        {
            iPod_convert_to_big32(&msgDB[IPOD_POS5], (U32)trackIndex);
            rc = iPodSendCommand(iPodHndl, msgDB);
        }
        else if(type == TYPE_PB)
        {
            iPod_convert_to_big32(&msgPB[IPOD_POS5], (U32)trackIndex);
            rc = iPodSendCommand(iPodHndl, msgPB);
        }
        else
        {
            rc = IPOD_BAD_PARAMETER;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unknown Track Type");
        }
        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Type = 0x%02d Track Index = %llu ",type,trackIndex);

        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if(rc == IPOD_OK)
            {
                if(type == TYPE_UID)
                {
                    uID = iPod_convert_to_little64(iPodHndl->iAP1Buf);
                    if((uID == trackIndex) && (iPodHndl->iAP1Buf[IPOD_POS8] == (U8)CHAPTER_COUNT))
                    {
                        *totalCount = (U16)iPod_convert_to_little16(&iPodHndl->iAP1Buf[IPOD_POS9]);
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Total Chapter Count = %u",*totalCount);
                    }
                    else
                    {
                        rc = IPOD_ERR_UNEXPECTED_ERROR;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unexpected error Unique Id = %llu chapter count = %u",uID,(U8)CHAPTER_COUNT);
                    }
                }
                else
                {
                    returned_index = iPod_convert_to_little32(iPodHndl->iAP1Buf);
                    if((returned_index == (U32)trackIndex) && (iPodHndl->iAP1Buf[IPOD_POS4] == (U8)CHAPTER_COUNT))
                    {
                        *totalCount = (U16)iPod_convert_to_little16(&iPodHndl->iAP1Buf[IPOD_POS5]);
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Total Count = %d",*totalCount);
                    }
                    else
                    {
                        rc = IPOD_ERR_UNEXPECTED_ERROR;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unexpected error returned_index = %u chapter count = %u",returned_index,(U8)CHAPTER_COUNT);
                    }
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error parameter iPodHndl = %p, totalCount = %p",iPodHndl,totalCount );
    }

    return rc;
}
