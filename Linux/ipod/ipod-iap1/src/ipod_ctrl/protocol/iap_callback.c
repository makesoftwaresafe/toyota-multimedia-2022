/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**
* \file: iap_callback.c
*
*
***************************************************************************** */

#include <adit_typedef.h>
#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */
#include <string.h> /* needed to avoid 'implicit declaration of memcopy ...' compiler warning */
#include <stdio.h>

#include "iap_callback.h"
#include "iap_general.h"
#include "ipodcommon.h"
#include "iap_transport_message.h"
#include "iap_digitalaudio.h"
#include "iap_util_func.h"
#include "iap1_dlt_log.h"

/* ========================================================================== */
/*  LOCAL functions                                                            */
/* ========================================================================== */
LOCAL U32 iPodWriteString(FILE *file, const U8 *buffer);
LOCAL U32 iPodWriteU16(FILE *file, U16 s);
LOCAL U32 iPodWriteU32(FILE *file, U32 l);
S32 iPodSaveBitmapImage(FILE* file, const IPOD_ALBUM_ARTWORK* artworkData);
LOCAL   U16  getTelegramIndex(const U8* iPodResponseBuffer);
LOCAL   U8   getDisplayPixelFormatCode(const U8* iPodResponseBuffer);
LOCAL   U16  getImageWidth(const U8* iPodResponseBuffer);
LOCAL   U16  getImageHeight(const U8* iPodResponseBuffer);
LOCAL   U16  getTopLeftPointX(const U8* iPodResponseBuffer);
LOCAL   U16  getTopLeftPointY(const U8* iPodResponseBuffer);
LOCAL   U16  getBottomRightX(const U8* iPodResponseBuffer);
LOCAL   U16  getBottomRightY(const U8* iPodResponseBuffer);
LOCAL   U32  getRowSize(const U8* iPodResponseBuffer);

LOCAL   U32  getNewSampleRate(const U8* iPodResponseBuffer);
LOCAL   U32  getNewSoundCheckValue(const U8* iPodResponseBuffer);
LOCAL   U32  getNewTrackVolume(const U8* iPodResponseBuffer);

/* ========================================================================== */
/*  internal functions                                                        */
/* ========================================================================== */
/* Converts a RGB565 pixel to R, G and B values */
inline static void RGB565toRGB(U16 rgb565, U8* r, U8* g, U8* b)
{
    *r = (U8)(((rgb565 & IPOD_RGB_R_MASK) >> IPOD_RGB_R_SHIFT) * IPOD_RGB_8);
    *g = (U8)(((rgb565 & IPOD_RGB_G_MASK) >> IPOD_RGB_G_SHIFT) * IPOD_RGB_4);
    *b =  (U8)((rgb565 & IPOD_RGB_B_MASK) * IPOD_RGB_8);
}

/* \internal */
/* Converts an RGB565 encoded image data array into an 24bit-RGB image data array */
inline static U32 iPodConvertRGB565To24BitRGB(const IPOD_ALBUM_ARTWORK* artworkData,
                                      U8* rgbBuffer, U32 length)
{
    U32 i     =  0;
    U32 j     =  0;
    U16 value =  0;
    U8 r      =  0;
    U8 g      =  0;
    U8 b      =  0;

    for (i = 0; i < artworkData->rowSize; i += IPOD_RGB_2)
    {
        value =   (U16)((artworkData->pixelData[i + length])
                | ((artworkData->pixelData[(i + length) + 1]) << IPOD_RGB_8));

        RGB565toRGB(value, &r, &g, &b);

        rgbBuffer[j + IPOD_POS0] = b;
        rgbBuffer[j + IPOD_POS1] = g;
        rgbBuffer[j + IPOD_POS2] = r;
        j += IPOD_POS3;
    }

    /* Should return w*h*3 */
    return j;
}

/* ========================================================================== */
/* pointers to registered callback functions                                  */
/* ========================================================================== */

LOCAL IPOD_CB_INT_USB_ATTACH iPodCBUSBAttach = NULL;
LOCAL IPOD_CB_INT_USB_DETACH iPodCBUSBDetach = NULL;

LOCAL IPOD_CB_INT_NOTIFY iPodCBNotify = NULL;
LOCAL IPOD_CB_INT_GET_ARTWORK iPodCBArtwork = NULL;
LOCAL IPOD_CB_INT_NOTIFY_STATE_CHANGE iPodCBNotifyStateChange = NULL;
LOCAL IPOD_CB_INT_GET_ACC_SAMPLE_RATE_CAPS iPodCBGetACCSampleRateCaps = NULL;
LOCAL IPOD_CB_INT_NEW_TRACK_INFO iPodCBNewTrackInfo = NULL;
LOCAL IPOD_CB_INT_LOCATION  iPodCBLocation = NULL;
LOCAL IPOD_CB_INT_NOTIFICATION iPodCBNotification = NULL;
LOCAL IPOD_CB_INT_REMOTE_EVENT_NOTIFICATION iPodCBRemoteEventNotification = NULL;

LOCAL IPOD_CB_INT_OPEN_DATA_SESSION iPodCBOpenDataSession = NULL;
LOCAL IPOD_CB_INT_CLOSE_DATA_SESSION iPodCBCloseDataSession = NULL;
LOCAL IPOD_CB_INT_IPOD_DATA_TRANSFER iPodCBiPodDataTransfer = NULL;

LOCAL IPOD_CB_INT_SET_ACC_STATUS iPodCBSetAccStatusNotification = NULL;

/* ========================================================================== */
/* Callback registration functions                                            */
/* ========================================================================== */


/**
 * \addtogroup CallbackRegistrationCommands
 */
/*\{*/


/*!
 * \fn iPodRegisterCBUSBAttach(IPOD_CB_USB_ATTACH const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_USB_ATTACH callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod attach event is detected and first part of authentication procedure with iPod was finished.
 * After this callback occurs, the iPod is ready to receive commands.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBUSBAttach(IPOD_CB_USB_ATTACH const callback)
{
    S32  ercd    = IPOD_OK;

    iPodCBUSBAttach = (IPOD_CB_INT_USB_ATTACH)callback;

    return ercd;
}

/*!
 * \fn iPodRegisterCBUSBDetach(IPOD_CB_USB_DETACH const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_USB_DETACH callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod detach event is detected.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBUSBDetach(IPOD_CB_USB_DETACH const callback)
{
    S32  ercd    = IPOD_OK;

    iPodCBUSBDetach = (IPOD_CB_INT_USB_DETACH)callback;

    return ercd;
}


/*!
 * \fn iPodRegisterCBNotifyStatus(IPOD_CB_NOTIFY const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_NOTIFY callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod's play status change notification is enabled. Which events will
 * be sent is documented in the documentation of the callback function
 * (IPOD_CB_NOTIFY).
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBNotifyStatus(IPOD_CB_NOTIFY const callback)
{
    S32  ercd    = IPOD_OK;

    iPodCBNotify = (IPOD_CB_INT_NOTIFY)callback;

    return ercd;
}


/*!
 * \fn iPodRegisterCBNotifyStateChange(IPOD_CB_NOTIFY_STATE_CHANGE const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_NOTIFY_STATE_CHANGE callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - return 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod's play state changes. For example the iPod switches from
 * power on to light sleep. Which events will
 * be sent is documented in the documentation of the callback function
 * (IPOD_CB_NOTIFY_STATE_CHANGE).
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBNotifyStateChange(IPOD_CB_NOTIFY_STATE_CHANGE const callback)
{
    S32  ercd    = IPOD_OK;

    iPodCBNotifyStateChange = (IPOD_CB_INT_NOTIFY_STATE_CHANGE)callback;

    return ercd;
}

/*!
 * \fn iPodRegisterCBGetAccSampleRateCaps(IPOD_CB_GET_ACC_SAMPLE_RATE_CAPS const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_GET_ACC_SAMPLE_RATE_CAPS callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd -  
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod requests the list of supported sample rates.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBGetAccSampleRateCaps(IPOD_CB_GET_ACC_SAMPLE_RATE_CAPS const callback)
{
    S32  ercd    = IPOD_OK;

    iPodCBGetACCSampleRateCaps = (IPOD_CB_INT_GET_ACC_SAMPLE_RATE_CAPS)callback;

    return ercd;
}


/*!
 * \fn iPodRegisterCBNewiPodTrackInfo(IPOD_CB_NEW_TRACK_INFO const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_NEW_TRACK_INFO callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called before
 * a new audio track begins playing.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted (except for non-blocking functions like "iPodAccAck").
 * \note In the case where the application takes care of audio streaming,
 * the application must register for this callback
 * to be notified of Sample Rate changes. When a Sample Rate change is requested by the iPod, the application
 * must change the Sample Rate and acknowledge the command by calling the "iPodAccAck" API.
 */
S32 iPodRegisterCBNewiPodTrackInfo(IPOD_CB_NEW_TRACK_INFO const callback)
{
    S32  ercd    = IPOD_OK;

    iPodCBNewTrackInfo = (IPOD_CB_INT_NEW_TRACK_INFO)callback;

    return ercd;
}


/*!
 * \fn iPodRegisterCBTrackArtworkData(IPOD_CB_GET_ARTWORK const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_GET_ARTWORK callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod sends a track artwork data telegram.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBTrackArtworkData(IPOD_CB_GET_ARTWORK const callback)
{
    S32  ercd    = IPOD_OK;

    iPodCBArtwork = (IPOD_CB_INT_GET_ARTWORK)callback;

    return ercd;
}

/*!
 * \fn iPodRegisterCBLocation(IPOD_CB_LOCATION const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_LOCATION callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod sends a location Lingo command.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBLocation(IPOD_CB_LOCATION const callback)
{
    S32 ercd = IPOD_OK;

    iPodCBLocation = (IPOD_CB_INT_LOCATION)callback;

    return ercd;
}

/*!
 * \fn iPodRegisterCBNotification(IPOD_CB_NOTIFICATION const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_NOTIFICATION callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod sends an event notification.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBNotification(IPOD_CB_NOTIFICATION const callback)
{
    S32 ercd = IPOD_OK;

    iPodCBNotification = (IPOD_CB_INT_NOTIFICATION)callback;

    return ercd;
}


/*!
 * \fn iPodRegisterCBRemoteEventNotification(IPOD_CB_REMOTE_EVENT_NOTIFICATION const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_REMOTE_EVENT_NOTIFICATION callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod sends an remote event notification.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBRemoteEventNotification(IPOD_CB_REMOTE_EVENT_NOTIFICATION const callback)
{
    S32 ercd = IPOD_OK;

    iPodCBRemoteEventNotification = (IPOD_CB_INT_REMOTE_EVENT_NOTIFICATION)callback;

    return ercd;
}


/*!
 * \fn iPodRegisterCBOpenDataSession(IPOD_CB_OPEN_DATA_SESSION const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_OPEN_DATA_SESSION callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod open session for communicating with iPhone application.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBOpenDataSession(IPOD_CB_OPEN_DATA_SESSION const callback)
{
    S32 ercd = IPOD_OK;

    iPodCBOpenDataSession = (IPOD_CB_INT_OPEN_DATA_SESSION)callback;

    return ercd;
}

/*!
 * \fn iPodRegisterCBCloseDataSession(IPOD_CB_CLOSE_DATA_SESSION const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_CLOSE_DATA_SESSION callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod close session.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBCloseDataSession(IPOD_CB_CLOSE_DATA_SESSION const callback)
{
    S32 ercd = IPOD_OK;

    iPodCBCloseDataSession = (IPOD_CB_INT_CLOSE_DATA_SESSION)callback;

    return ercd;
}



/*!
 * \fn iPodRegisterCBiPodDataTransfer(IPOD_CB_IPOD_DATA_TRANSFER const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_IPOD_DATA_TRANSFER callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod sends data of iPhone application.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBiPodDataTransfer(IPOD_CB_IPOD_DATA_TRANSFER const callback)
{
    S32 ercd = IPOD_OK;

    iPodCBiPodDataTransfer = (IPOD_CB_INT_IPOD_DATA_TRANSFER)callback;

    return ercd;
}


/*!
 * \fn iPodRegisterCBSetAccStatusNotification(IPOD_CB_SET_ACC_STATUS const callback)
 * \par INPUT PARAMETERS
 * #IPOD_CB_SET_ACC_STATUS callback - the callback funcion
 * \par REPLY PARAMETERS
 * S32 ercd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_NOEXS object does not exist
 * \li \c \b #IPOD_ERR_PAR parameter error
 * \li \c \b #IPOD_ERR_OBJ Incorrect object state
 * \par DESCRIPTION
 * This function registers the callback function that will be called when
 * the iPod requests status notification.
 * The user callbacks may not block. Therefore is the call of iPod control
 * functions out of the callbacks not permitted.
 */
S32 iPodRegisterCBSetAccStatusNotification(IPOD_CB_SET_ACC_STATUS const callback)
{
    S32 ercd = IPOD_OK;

    iPodCBSetAccStatusNotification = (IPOD_CB_INT_SET_ACC_STATUS)callback;

    return ercd;
}

/*!
 * \fn iPodSetTrackArtworkDataImageSaveParams(U32 iPodID, U8 saveAsBMP, U8* imagePath)
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 saveAsBMP - Flag whether save BMP
 * \par INOUT PARAMETERS
 * U8* imagePath - Path to save BMP
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function decides whether to save BMP in the specified Path.<br>
 * For performance reasons (to save the time of copying the image to the file system)
 * it is recommended to register a callback using iPodRegisterCBTrackArtworkData
 * and use the image data directly from memory, as provided by the iPod.
 */
void iPodSetTrackArtworkDataImageSaveParams(U32 iPodID, U8 saveAsBMP, U8* imagePath)
{
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    if ((iPodHndl != NULL) && (imagePath != NULL))
    {
        iPodHndl->artwork.saveAsBMP = saveAsBMP;
        iPodHndl->artwork.imagePath = imagePath;
    }
}

/*\}*/

/* ========================================================================== */
/* Callback execution functions (called by the iPod response ReaderTask)      */
/* ========================================================================== */


/* \internal */
S32 iPodExecuteCBUSBAttach(const IPOD_INSTANCE* iPodHndl, const S32 success)
{
    S32          ercd    = IPOD_OK;

    if (iPodCBUSBAttach != NULL)
    {
        iPodCBUSBAttach(success, iPodHndl->connection, iPodHndl->id);
    }

    return ercd;

}

S32 iPodExecuteCBUSBDetach(const IPOD_INSTANCE* iPodHndl)
{
    S32          ercd    = IPOD_OK;

    if (iPodCBUSBDetach != NULL)
    {
        iPodCBUSBDetach(iPodHndl->id);
    }

    return ercd;
}


/* \internal */
S32 iPodExecuteCBNotify(const IPOD_INSTANCE* iPodHndl, IPOD_CHANGED_PLAY_STATUS status, U64 param)
{
    S32          ercd    = IPOD_OK;

    if (iPodCBNotify != NULL)
    {
        iPodCBNotify(status, param, iPodHndl->id);
    }

    return ercd;
}

/* \internal */
S32 iPodExecuteCBNotifyStateChange(const IPOD_INSTANCE* iPodHndl, IPOD_STATE_CHANGE stateChange)
{
    S32          ercd    = IPOD_OK;

    if (iPodCBNotifyStateChange != NULL)
    {
        iPodCBNotifyStateChange(stateChange, iPodHndl->id);
    }

    return ercd;
}

/* \internal */
S32 iPodExecuteCBGetACCSampleRateCaps(const IPOD_INSTANCE* iPodHndl)
{
    S32          ercd    = IPOD_OK;

    if (iPodCBGetACCSampleRateCaps != NULL)
    {
        iPodCBGetACCSampleRateCaps(iPodHndl->id);
    }

    return ercd;
}

/* \internal */
S32 iPodExecuteCBLocation(const IPOD_INSTANCE* iPodHndl, IPOD_LOCATION_CMD locCmd, IPOD_LOCATION_TYPE locType, U8 dataType, U8* locData, U32 size)
{
    S32 rc = IPOD_OK;

    if(iPodCBLocation != NULL)
    {
        rc = iPodCBLocation(locCmd, locType, dataType, locData, size, iPodHndl->id);
        IAP1_CB_LOG(DLT_LOG_DEBUG,"iPodCBLocation() returns : rc = %d",rc);
    }

    return rc;
}

/* \internal */
S32 iPodExecuteCBNotification(const IPOD_INSTANCE* iPodHndl, IPOD_NOTIFY_TYPE type, IPOD_NOTIFY_STATUS status)
{
    S32 ercd = IPOD_OK;

    if(iPodCBNotification != NULL)
    {
        iPodCBNotification(type, status, iPodHndl->id);
    }

    return ercd;

}

/* \internal */
S32 iPodExecuteCBRemoteEventNotify(const IPOD_INSTANCE* iPodHndl,
                                   IPOD_STATE_INFO_TYPE eventNum, IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData)
{
    S32       ercd    = IPOD_OK;

    if((iPodHndl != NULL) && (iPodCBRemoteEventNotification != NULL))
    {
        iPodCBRemoteEventNotification(eventNum, eventData, iPodHndl->id);
    }

    return ercd;
}


/* \internal */
S32 iPodExecuteCBOpenDataSession(const IPOD_INSTANCE* iPodHndl, U8 protocolIndex, U16 sessionId)
{
    S32 ercd = IPOD_OK;

    if(iPodCBOpenDataSession != NULL)
    {
        ercd = iPodCBOpenDataSession(protocolIndex, sessionId, iPodHndl->id);
        IAP1_CB_LOG(DLT_LOG_DEBUG,"iPodCBOpenDataSession() returns : ercd = %d",ercd);
    }

    return ercd;
}

/* \internal */
S32 iPodExecuteCBCloseDataSession(const IPOD_INSTANCE* iPodHndl, U16 sessionId)
{
    S32 ercd = IPOD_OK;

    if(iPodCBCloseDataSession != NULL)
    {
        iPodCBCloseDataSession(sessionId, iPodHndl->id);
    }

    return ercd;
}

S32 iPodExecuteCBiPodDataTransfer(const IPOD_INSTANCE* iPodHndl, U16 sessionId, U8 *data, U16 length)
{
    S32 ercd = IPOD_OK;
    if(iPodCBiPodDataTransfer != NULL)
    {
        ercd = iPodCBiPodDataTransfer(sessionId, data, length, iPodHndl->id);
        IAP1_CB_LOG(DLT_LOG_DEBUG,"iPodCBiPodDataTransfer () returns : ercd = %d",ercd);
    }

    return ercd;
}

S32 iPodExecuteCBSetAccStatusNotification(const IPOD_INSTANCE* iPodHndl, U32 statusMask)
{
    S32 ercd = IPOD_OK;

    if(iPodCBSetAccStatusNotification != NULL)
    {
        iPodCBSetAccStatusNotification(statusMask, iPodHndl->id);
    }

    return ercd;
}

/* \internal */
LOCAL U32 getNewSampleRate(const U8* iPodResponseBuffer)
{
    U32 newSampleRate = 0;

    newSampleRate       = (U32)iPodResponseBuffer[IPOD_POS0];
    newSampleRate       = newSampleRate << IPOD_SHIFT_8;
    newSampleRate      |= (U32)iPodResponseBuffer[IPOD_POS1];
    newSampleRate       = newSampleRate << IPOD_SHIFT_8;
    newSampleRate      |= (U32)iPodResponseBuffer[IPOD_POS2];
    newSampleRate       = newSampleRate << IPOD_SHIFT_8;
    newSampleRate      |= (U32)iPodResponseBuffer[IPOD_POS3];

    return newSampleRate;
}


/* \internal */
LOCAL U32 getNewSoundCheckValue(const U8* iPodResponseBuffer)
{
    U32 newSoundCheckValue = 0;

    newSoundCheckValue  = (U32)iPodResponseBuffer[IPOD_POS4];
    newSoundCheckValue  = newSoundCheckValue<< IPOD_SHIFT_8;
    newSoundCheckValue |= (U32)iPodResponseBuffer[IPOD_POS5];
    newSoundCheckValue  = newSoundCheckValue << IPOD_SHIFT_8;
    newSoundCheckValue |= (U32)iPodResponseBuffer[IPOD_POS6];
    newSoundCheckValue  = newSoundCheckValue << IPOD_SHIFT_8;
    newSoundCheckValue |= (U32)iPodResponseBuffer[IPOD_POS7];

    return newSoundCheckValue;
}


/* \internal */
LOCAL U32 getNewTrackVolume(const U8* iPodResponseBuffer)
{
    U32 newTrackVolume = 0;

    newTrackVolume      = (U32)iPodResponseBuffer[IPOD_POS8];
    newTrackVolume      = newTrackVolume << IPOD_SHIFT_8;
    newTrackVolume     |= (U32)iPodResponseBuffer[IPOD_POS9];
    newTrackVolume      = newTrackVolume << IPOD_SHIFT_8;
    newTrackVolume     |= (U32)iPodResponseBuffer[IPOD_POS10];
    newTrackVolume      = newTrackVolume << IPOD_SHIFT_8;
    newTrackVolume     |= (U32)iPodResponseBuffer[IPOD_POS11];

    return newTrackVolume;
}

/* \internal */
S32 iPodExecuteCBNewTrackInfo(const IPOD_INSTANCE* iPodHndl, const U8* iPodResponseBuffer)
{
    S32          ercd    = IPOD_OK;

    U32 newSampleRate = 0;
    U32 newSoundCheckValue = 0;
    U32 newTrackVolume = 0;

    if((iPodHndl == NULL) || (iPodResponseBuffer == NULL))
    {
        IAP1_CB_LOG(DLT_LOG_ERROR, "iPodHndl = %p iPodResponseBuffer = %p",iPodHndl,iPodResponseBuffer);
        return IPOD_BAD_PARAMETER;
    }

    iPodAccAck(iPodHndl->id, IPOD_ACC_ACK_STATUS_SUCCESS);
    newSampleRate       =   getNewSampleRate(iPodResponseBuffer);
    newSoundCheckValue  =   getNewSoundCheckValue(iPodResponseBuffer);
    newTrackVolume      =   getNewTrackVolume(iPodResponseBuffer);

    IAP1_CB_LOG(DLT_LOG_DEBUG, "newSampleRate = %u newSoundCheckValue = %u newTrackVolume = %u",newSampleRate,newSoundCheckValue,newTrackVolume);
    if (iPodCBNewTrackInfo != NULL)
    {
            /* Call the callback */
            iPodCBNewTrackInfo(newSampleRate,
                               (S32)newSoundCheckValue,
                               (S32)newTrackVolume,
                               iPodHndl->id);
    }

    return ercd;
}

/* \internal */
LOCAL U16 getTelegramIndex(const U8* iPodResponseBuffer)
{
    U16 tIndex;

    tIndex   = (U16)iPodResponseBuffer[IPOD_POS0];
    tIndex   = tIndex << IPOD_SHIFT_8;
    tIndex  |= (U16)iPodResponseBuffer[IPOD_POS1];
    return tIndex;
}

/* \internal */
LOCAL U8 getDisplayPixelFormatCode(const U8* iPodResponseBuffer)
{
    U8 displayPixelFormatCode;

    displayPixelFormatCode = iPodResponseBuffer[IPOD_POS2];

    return displayPixelFormatCode;
}

/* \internal */
LOCAL U16 getImageWidth(const U8* iPodResponseBuffer)
{
    U16 imageWidth;

    imageWidth     = (U16)iPodResponseBuffer[IPOD_POS3];
    imageWidth     = imageWidth << IPOD_SHIFT_8;
    imageWidth    |= (U16)iPodResponseBuffer[IPOD_POS4];

    return imageWidth;
}

/* \internal */
LOCAL U16 getImageHeight(const U8* iPodResponseBuffer)
{
    U16 imageHeight;

    imageHeight    = (U16)iPodResponseBuffer[IPOD_POS5];
    imageHeight    = imageHeight << IPOD_SHIFT_8;
    imageHeight   |= (U16)iPodResponseBuffer[IPOD_POS6];

    return imageHeight;
}

/* \internal */
LOCAL U16 getTopLeftPointX(const U8* iPodResponseBuffer)
{
    U16 topLeftPointX;

    topLeftPointX  = (U16)iPodResponseBuffer[IPOD_POS7];
    topLeftPointX  = topLeftPointX << IPOD_SHIFT_8;
    topLeftPointX |= (U16)iPodResponseBuffer[IPOD_POS8];

    return topLeftPointX;
}

/* \internal */
LOCAL U16 getTopLeftPointY(const U8* iPodResponseBuffer)
{
    U16 topLeftPointY;

    topLeftPointY  = (U16)iPodResponseBuffer[IPOD_POS9];
    topLeftPointY  = topLeftPointY << IPOD_SHIFT_8;
    topLeftPointY |= (U16)iPodResponseBuffer[IPOD_POS10];

    return topLeftPointY;
}

/* \internal */
LOCAL U16 getBottomRightX(const U8* iPodResponseBuffer)
{
    U16 bottomRightX;

    bottomRightX   = (U16)iPodResponseBuffer[IPOD_POS11];
    bottomRightX   = bottomRightX << IPOD_SHIFT_8;
    bottomRightX  |= (U16)iPodResponseBuffer[IPOD_POS12];

    return bottomRightX;
}

/* \internal */
LOCAL U16 getBottomRightY(const U8* iPodResponseBuffer)
{
    U16 bottomRightY;

    bottomRightY   = (U16)iPodResponseBuffer[IPOD_POS13];
    bottomRightY   = bottomRightY << IPOD_SHIFT_8;
    bottomRightY  |= (U16)iPodResponseBuffer[IPOD_POS14];

    return bottomRightY;
}

/* \internal */
LOCAL U32 getRowSize(const U8* iPodResponseBuffer)
{
    U32 rowSize;

    rowSize  = (U32)iPodResponseBuffer[IPOD_POS15];
    rowSize  = rowSize << IPOD_SHIFT_8;
    rowSize |= (U32)iPodResponseBuffer[IPOD_POS16];
    rowSize  = rowSize << IPOD_SHIFT_8;
    rowSize |= (U32)iPodResponseBuffer[IPOD_POS17];
    rowSize  = rowSize << IPOD_SHIFT_8;
    rowSize |= (U32)iPodResponseBuffer[IPOD_POS18];

    return rowSize;
}

/* If iPod is disconnected while getting artwrkData, this function is called by reader task. */
/* And free memory */
/* \internal */
void iPodFreeArtworkData(IPOD_INSTANCE* iPodHndl)
{
    if(iPodHndl != NULL)
    {
        if(iPodHndl->artwork.data.imageDataBuffer != NULL)
        {
            free(iPodHndl->artwork.data.imageDataBuffer);
            iPodHndl->artwork.data.imageDataBuffer = NULL;
        }
        memset(&iPodHndl->artwork.data, 0, sizeof(iPodHndl->artwork.data));
    }
}

/* \internal */
S32 iPodExecuteCBGetTrackArtworkData(IPOD_INSTANCE* iPodHndl, const U8* iPodResponseBuffer, U16 len)
{
    S32       ercd    = IPOD_OK;
    U16 telegramIndex = 0;
    U32 pixelDataLength = 0;
    U8* pixelData = NULL;
    IPOD_ARTWORK* artwork;

    telegramIndex = getTelegramIndex(iPodResponseBuffer);
    artwork = &iPodHndl->artwork;

    if (telegramIndex == 0)
    {
        /* Make sure old ArtworkData was deleted before allocating the new one to avoid memory leak */
        iPodFreeArtworkData(iPodHndl);
        /* here comes the first piece of artwork data. So we need to reset our data counter */
        /* and calculate the total amount of data we expect to receive.                     */
        artwork->data.collectedArtworkData.displayPixelFormatCode = getDisplayPixelFormatCode(iPodResponseBuffer);
        artwork->data.collectedArtworkData.imageWidth             = getImageWidth(iPodResponseBuffer);
        artwork->data.collectedArtworkData.imageHeight            = getImageHeight(iPodResponseBuffer);
        artwork->data.collectedArtworkData.topLeftPointX          = getTopLeftPointX(iPodResponseBuffer);
        artwork->data.collectedArtworkData.topLeftPointY          = getTopLeftPointY(iPodResponseBuffer);
        artwork->data.collectedArtworkData.bottomRightX           = getBottomRightX(iPodResponseBuffer);
        artwork->data.collectedArtworkData.bottomRightY           = getBottomRightY(iPodResponseBuffer);
        artwork->data.collectedArtworkData.rowSize                = getRowSize(iPodResponseBuffer);
        pixelDataLength = (len - IPOD_ARTWORK_DATA_OFFSET_1);
        pixelData = (U8*)&iPodResponseBuffer[IPOD_POS19];

        artwork->data.imageDataCounter = 0;
        artwork->data.totalImageData = (artwork->data.collectedArtworkData.imageHeight * artwork->data.collectedArtworkData.rowSize);
      
        artwork->data.imageDataBuffer = (U8*)calloc(artwork->data.totalImageData, sizeof(U8));
    }
    else
    {
        pixelDataLength = (len - IPOD_ARTWORK_DATA_OFFSET_2);
        pixelData = (U8*)&iPodResponseBuffer[IPOD_POS2];
    }

    if ( artwork->data.imageDataBuffer != NULL)
    {
        /* JIRA[SWGII-3948] */
        /* if total data length is bigger than expected data length, no memcpy to prevent memory broken */
        if(artwork->data.imageDataCounter + pixelDataLength <= artwork->data.totalImageData)
        {
            memcpy(&artwork->data.imageDataBuffer[artwork->data.imageDataCounter],
                   pixelData,
                   pixelDataLength);
       }

        /* keep track of the amount of data received from the iPod so far */
        artwork->data.imageDataCounter += pixelDataLength;

        /* once the complete data is collected send it to the callback and save the */
        /* image as a bitmap if necessary.                                          */
        if (artwork->data.imageDataCounter >= artwork->data.totalImageData)
        {
            artwork->data.collectedArtworkData.pixelData = artwork->data.imageDataBuffer;
            artwork->data.collectedArtworkData.pixelDataLength = artwork->data.totalImageData;

            if (iPodCBArtwork != NULL)
            {
                if(artwork->data.imageDataCounter == artwork->data.totalImageData)
                {
                    iPodCBArtwork(&artwork->data.collectedArtworkData, iPodHndl->id);
                }
                else
                {
                    /* JIRA[SWGII-3949] */
                    /* When data length is broken, it should reply NULL */
                    IAP1_CB_LOG(DLT_LOG_DEBUG,"As data length is broken, NULL will be replied");
                    iPodCBArtwork(NULL, iPodHndl->id);
                }
            }

            if (   (artwork->saveAsBMP == TRUE)
                && (artwork->imagePath != NULL))
            {
                FILE* imageFile = fopen((VP)artwork->imagePath,
                                        IPOD_WRITE_BINARY);
                if (imageFile != NULL)
                {
                    ercd = iPodSaveBitmapImage(imageFile, &artwork->data.collectedArtworkData);
                    fclose(imageFile);
                }
                else
                {
                    IAP1_CB_LOG(DLT_LOG_ERROR,"Opening the image file from the path %s is failed",artwork->imagePath);
                }
            }
            iPodFreeArtworkData(iPodHndl);
        }
        else
        {
            ercd = IPOD_ERR_NOMEM;
            IAP1_CB_LOG(DLT_LOG_ERROR,"No Memory: Image Data Counter is less than total image Data");
        }

    }
    return ercd;
}

/* \internal */
S32 iPodExecuteCBGetTrackArtworkDataEx(IPOD_INSTANCE* iPodHndl, const U8* iPodResponseBuffer, U16 len)
{
    S32       ercd    = IPOD_OK;
    U16 telegramIndex = 0;
    U32 pixelDataLength = 0;
    U8* pixelData = NULL;
    IPOD_ARTWORK* artwork;
    U64 trackIndex = 0;
    U8 *dataBuf = NULL;
    
    telegramIndex = getTelegramIndex(iPodResponseBuffer);
    artwork = &iPodHndl->artwork;

    if (telegramIndex == 0)
    {
        if(iPodResponseBuffer[IPOD_POS4] == 0x00)
        {
            trackIndex = iPod_convert_to_little64(&iPodResponseBuffer[IPOD_POS5]);
            dataBuf = (U8 *)&iPodResponseBuffer[IPOD_POS11];
            len = len - (13 + 17);
        }
        else
        {
            trackIndex = (U32)iPod_convert_to_little32(&iPodResponseBuffer[IPOD_POS5]);
            dataBuf = (U8 *)&iPodResponseBuffer[IPOD_POS7];
            len = len - (9 + 17);
        }
        /* Compiler warning */
        trackIndex = trackIndex;
        
        /* Make sure old ArtworkData was deleted before allocating the new one to avoid memory leak */
        iPodFreeArtworkData(iPodHndl);
        /* here comes the first piece of artwork data. So we need to reset our data counter */
        /* and calculate the total amount of data we expect to receive.                     */
        artwork->data.collectedArtworkData.displayPixelFormatCode = getDisplayPixelFormatCode(dataBuf);
        artwork->data.collectedArtworkData.imageWidth             = getImageWidth(dataBuf);
        artwork->data.collectedArtworkData.imageHeight            = getImageHeight(dataBuf);
        artwork->data.collectedArtworkData.topLeftPointX          = getTopLeftPointX(dataBuf);
        artwork->data.collectedArtworkData.topLeftPointY          = getTopLeftPointY(dataBuf);
        artwork->data.collectedArtworkData.bottomRightX           = getBottomRightX(dataBuf);
        artwork->data.collectedArtworkData.bottomRightY           = getBottomRightY(dataBuf);
        artwork->data.collectedArtworkData.rowSize                = getRowSize(dataBuf);
        pixelDataLength = len;
        pixelData = (U8*)&dataBuf[IPOD_POS19];

        artwork->data.imageDataCounter = 0;
        artwork->data.totalImageData = (artwork->data.collectedArtworkData.imageHeight * artwork->data.collectedArtworkData.rowSize);
      
        artwork->data.imageDataBuffer = (U8*)calloc(artwork->data.totalImageData, sizeof(U8));
    }
    else
    {
        len = len - 4;
        dataBuf = (U8 *)&iPodResponseBuffer[IPOD_POS4];
        pixelDataLength = len;
        pixelData = dataBuf;
    }

    if ( artwork->data.imageDataBuffer != NULL)
    {
        /* JIRA[SWGII-3948] */
        /* if total data length is bigger than expected data length, no memcpy to prevent memory broken */
        if(artwork->data.imageDataCounter + pixelDataLength <= artwork->data.totalImageData)
        {
            memcpy(&artwork->data.imageDataBuffer[artwork->data.imageDataCounter],
                   pixelData,
                   pixelDataLength);
       }

        /* keep track of the amount of data received from the iPod so far */
        artwork->data.imageDataCounter += pixelDataLength;

        /* once the complete data is collected send it to the callback and save the */
        /* image as a bitmap if necessary.                                          */
        if (artwork->data.imageDataCounter >= artwork->data.totalImageData)
        {
            artwork->data.collectedArtworkData.pixelData = artwork->data.imageDataBuffer;
            artwork->data.collectedArtworkData.pixelDataLength = artwork->data.totalImageData;

            if (iPodCBArtwork != NULL)
            {
                if(artwork->data.imageDataCounter == artwork->data.totalImageData)
                {
                    iPodCBArtwork(&artwork->data.collectedArtworkData, iPodHndl->id);
                }
                else
                {
                    /* JIRA[SWGII-3949] */
                    /* When data length is broken, it should reply NULL */
                    IAP1_CB_LOG(DLT_LOG_DEBUG,"As data length is broken, NULL will be replied");
                    iPodCBArtwork(NULL, iPodHndl->id);
                }
            }

            if (   (artwork->saveAsBMP == TRUE)
                && (artwork->imagePath != NULL))
            {
                FILE* imageFile = fopen((VP)artwork->imagePath,
                                        IPOD_WRITE_BINARY);
                if (imageFile != NULL)
                {
                    ercd = iPodSaveBitmapImage(imageFile, &artwork->data.collectedArtworkData);
                    fclose(imageFile);
                }
                else
                {
                    IAP1_CB_LOG(DLT_LOG_ERROR,"Opening the image file from the path %s is failed",artwork->imagePath);
                }
            }
            iPodFreeArtworkData(iPodHndl);
        }
        else
        {
            ercd = IPOD_ERR_NOMEM;
            IAP1_CB_LOG(DLT_LOG_ERROR,"No Memory,Image Data Counter is less than Total Image Data ");
        }

    }
    return ercd;
}


/* ========================================================================== */
/* private helper functions                                                   */
/* ========================================================================== */


/* \internal */
/* Writes a string from a buffer to a file. */
LOCAL U32 iPodWriteString(FILE *file, const U8 *buffer)
{
    return fwrite(buffer, 1, strlen((VP)buffer), file);
}


/* \internal */
/* Writes an unsigned short to a file. */
LOCAL U32 iPodWriteU16(FILE *file, U16 s)
{
    return fwrite(&s, sizeof(U16), 1, file);
}


/* \internal */
/* Writes an unsigned long to a file. */
LOCAL U32 iPodWriteU32(FILE *file, U32 l)
{
    return fwrite(&l, sizeof(U32), 1, file);
}

/* \internal */
/* Saves an IPOD_ALBUM_ARTWORK structure as a 24-Bit BMP image */
S32 iPodSaveBitmapImage(FILE* file, const IPOD_ALBUM_ARTWORK* artworkData)
{
    U32 rgb24bitBufferSize = 0;
    U32 i = 0;
    S32 j = 0;
    U8* buf = NULL;
    U16 count = 0;

    if(artworkData != NULL)
    {
        rgb24bitBufferSize =   artworkData->imageWidth
                             * IPOD_BMP_24BIT;

        buf = (U8 *)calloc(artworkData->imageWidth * IPOD_RGB_LENGTH, sizeof(U8));
        if (buf != NULL)
        {
            /* Write the bitmap file header ---------------------------------------------- */
            iPodWriteString(file, (const U8*)IPOD_BMP_FILE_ID);                                        /* bfType          */
            iPodWriteU32(file, IPOD_BMP_OFFSET + (artworkData->imageHeight * rgb24bitBufferSize));       /* bfSize          */
            iPodWriteU32(file, 0);                                                                          /* bfReserved      */
            iPodWriteU32(file, IPOD_BMP_OFFSET);                                                /* bfOffbits       */

            /* Write bitmap info header -------------------------------------------------- */
            iPodWriteU32(file, IPOD_BMP_HEADER_SIZE);                                       /* biSize          */
            iPodWriteU32(file, artworkData->imageWidth);                                /* biWidth         */
            iPodWriteU32(file, (U32)(S32)(-1 * artworkData->imageHeight));                  /* biHeight        */
            iPodWriteU16(file, 1);                                                                          /* biPlanes        */
            iPodWriteU16(file, IPOD_BMP_BIT_COUNT);                                         /* biBitCount      */
            iPodWriteU32(file, 0);                                                                          /* biCompression   */
            iPodWriteU32(file, 0);                                                                          /* biSizeImage     */
            iPodWriteU32(file, 0);                                                                          /* biXPelsPerMeter */
            iPodWriteU32(file, 0);                                                                          /* biYPelsPerMeter */
            iPodWriteU32(file, 0);                                                                          /* biClrUsed       */
            iPodWriteU32(file, 0);

            for(count = 0; count < artworkData->imageHeight; count++)
            {
                iPodConvertRGB565To24BitRGB(artworkData, buf, count * artworkData->rowSize);
                fwrite(buf, 1, rgb24bitBufferSize, file);
                /* insert padding bytes if necessary */
                if ( (artworkData->imageWidth % IPOD_BMP_ALIGN) != 0)
                {
                    i = artworkData->imageWidth % IPOD_BMP_ALIGN;
                    if(i > 0)
                    {
                        U8 temp[IPOD_BMP_ALIGN] = {0};
                        fwrite(temp, 1, i,file);
                    }
                }
            }
        }
        else
        {
            j = IPOD_ERR_NOMEM;
            IAP1_CB_LOG(DLT_LOG_ERROR,"No Memory buf is NULL");
        }

        if(buf != NULL)
        {
            free(buf);
            buf = NULL;
        }
    }
    else
    {
        j = IPOD_ERR_PAR;
        IAP1_CB_LOG(DLT_LOG_ERROR,"No Memory artworkData is NULL");
    }

    /* Should return the amount of written image data bytes (w*h*3) */
    return j;
}

