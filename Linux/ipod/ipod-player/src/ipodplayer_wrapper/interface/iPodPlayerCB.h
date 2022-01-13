

#ifndef IPOD_PALEYR_CB_H
#define IPOD_PALEYR_CB_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup NotificationCB Notification
 */
/*\{*/


/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS)(U32 devID, IPOD_PLAYER_PLAYBACK_STATUS *status)
 * Usually this function pointer is called when iPod playback status is changed.<br>
 * Also this function pointer may be called to prevent the lost of status.<br>
 * If track or chapter index are changed to new index, application can get the detail of track or chapter information by using
 * #iPodPlayerGetTrackInfo or #iPodPlayerGetChapterInfo.
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [out] *status Type is #IPOD_PLAYER_PLAYBACK_STATUS pointer. Current iPod Playing status<br>
 * \warning
 * When track changes, this function may notify with unknown index(#IPOD_PLAYER_PLYABACK_INDEX_UNKNOWN) or unknown time(#IPOD_PLAYER_PLAYBACK_TIME_UNKNOWN).<br>
 * If these values were notified, Application must not use them.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS)(U32 devID, IPOD_PLAYER_PLAYBACK_STATUS *status);


/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_CONNECTION_STATUS)(U32 devID, IPOD_PLAYER_CONNECTION_STATUS *status)
 * Usually this function pointer is called when iPod connection status changed.<br>
 * Also this function pointer may be called to prevent the lost of status.<br>
 * Application can use the all of iPodPlayer API after receive the #IPOD_PLAYER_AUTHENTICATION_SUCCESS in status.<br>
 * If application receive the iPod disconnect, application must not call the iPodPlayer API<br>
 * devID is notified when this function is called with attached.<br>
 * When application registered the this function to iPodPlayer, iPodPlayer notify the current status once.<br>
 * \param [in] devID Type is U32. This indicates the connected Apple device.<br>
 * \param [out] *status Type is #IPOD_PLAYER_CONNECTION_STATUS pointer. This is current connection status of iPod.
 * \note
 * When application registered this callback function, iPodPlayer immediately notify the current connection status.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_CONNECTION_STATUS)(U32 devID, IPOD_PLAYER_CONNECTION_STATUS *status);


/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_TRACK_INFO)(U32 devID, U32 trackIndex, IPOD_PLAYER_TRACK_INFO *info)
 * This function pointer is called when track is changed and application had called the #iPodPlayerSetTrackInfoNotification with non-zero value of bitmask. <br>
 * In default, this function pointer will be never called even if track is changed.<br>
 * \param [in] devID Type is U32. This indicates the connected Apple device.<br>
 * \param [in] trackIndex Type is U32. This is a index of track in playback list.<br>
 * \param [out] *info Type is #IPOD_PLAYER_TRACK_INFO pointer. Information of changed track.<br>
 * \warning
 * If below mask is set at the same time, iPodPlayer is slow to reply to Application. iPodPlayer recommends to get the below information only when Application actually need.<br>
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_LYRIC
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_GENRE
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_UID
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_TRACK_INFO)(U32 devID, U32 trackIndex, IPOD_PLAYER_TRACK_INFO *info);


/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_PLAYBACK_CHANGE)(U32 devID)
 * This function pointer is called when playback list change to new list.<br>
 * \param [in] devID Type is U32. This indicates the connected Apple device.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_PLAYBACK_CHANGE)(U32 devID);


/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_DB_ENTRIES)(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 count, IPOD_PLAYER_ENTRY_LIST *entryList)
 * This function pointer is called when application call #iPodPlayerGetDBEntries<br>
 * iPodPlayer sends this callback after send the result of #iPodPlayerGetDBEntries<br>
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_DB_TYPE. Type of Entries<br>
 * \param [in] count Type is U32.  This value means that list of number of count is included to this callback function.<br>
 * \param [out] *entryList Type is #IPOD_PLAYER_ENTRY_LIST pointer. Structure of entry.
 * \warning
 * This function notifies the list collectively. The maximum number of collection can be decided by changing the configuration.<br>
 * e.g. 12 entries are requested and maximum number of collection is 5, iPodPlayer will notify that the first notification is 5 entries, 
 * second notification is 5 entries and last notification is 2 entries.
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_DB_ENTRIES)(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 count, IPOD_PLAYER_ENTRY_LIST *entryList);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_COVERART_DATA)(U32 devID, U32 trackIndex, U32 time, IPOD_PLAYER_COVERART_HEADER *header, U32 size, U8 *data)
 * This function pointer is called when application call #iPodPlayerGetCoverArt.<br>
 * iPodPlayer sends this callback after send the result of #iPodPlayerGetCoverArt with #IPOD_PLAYER_OK<br>
 * Size of cover art data may be big size. Application must allocate the satisfy memory size.<br>
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [in] trackIndex. Type is U32. CoverArt of this track is gotten.<br>
 * \param [in] time. Type is U32. CoverArt data is displayed at this time.
 * \param [out] *header Type is #IPOD_PLAYER_COVERART_HEADER pointer. Header of coverart that is indicated by #iPodPlayerGetCoverArt<br>
 * \param [out] size Type is U32. This is a size of CoverArt data. If this argument is zero, song does not have CoverArt data.
 * \param [out] *data Type is U8 pointer. This is CoverArt data. If this argument is NULL, song does not have CoverArt data.
 * \note
 * The data of coverart is guranteed while call this callback function.<br>
 * If this function finished, data is not guranteed.
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_COVERART_DATA)(U32 devID, U32 trackIndex, U32 time, IPOD_PLAYER_COVERART_HEADER *header, U32 size, U8 *data);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_OPEN_APP)(U32 devID, U32 appHandle)
 * This function pointer is called when iOS application is opend. <br>
 * appHandle must register to iPod to first and application must understand which ID referenced to iOS application.<br>
 * iOSApplication can be registered by #iPodPlayerInit. Application can send/receive the data after it was notified.<br>
 * also, Application must know which is operated application. handle is equal to array number + 1 of appTable of #iPodPlayerInit.<br>
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [in] appHandle Type is U32. This is a handle with each iOS application. application must know the application referenced with this.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_OPEN_APP)(U32 devID, U32 appHandle);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_CLOSE_APP)(U32 devID, , U32 appHandle)
 * This function pointer is called when iOS application is closed.<br>
 * Opened iOS application is managed by appHandle. Application can not send/received the data after it was notified.
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [in] appID Type is U32. This is a handle with each iOS application. application must know the application referenced with this.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_CLOSE_APP)(U32 devID, U32 appHandle);


/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_RECEIVE_FROM_APP)(U32 devID, U32 appHandle,  U32 dataSize, U8 *data)
 * This function pointer is called when iOS application sends the data to application.<br>
 * Application can receive the data from iOS application during it's application is opened.<br>
 * iOS application which send to application can know the appHandle.
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [in] appHandle Type is U32. This is a handle with each iOS application. application must know the application referenced with this.<br>
 * \param [in] dataSize Type is U32. size of receiving data<br>
 * \param [out] *data Type is U8 pointer. data that application receive<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_RECEIVE_FROM_APP)(U32 devID, U32 appHandle,  U32 dataSize, U8 *data);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_GPS_STATUS)(U32 devID, U32 gpsStatusMask)
 * This function pointer is called when Apple device requests to change the status of Application.<br>
 * If application receives this request, it must change the status.<br>
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [in] gpsStatusMask Type is U32. This is a bitmask of GPS status. Application must change to this status.<br>
 * \li \c <b> #IPOD_PLAYER_GPS_POEWR </b><br>
 * \li \c <b> #IPOD_PLAYER_GPS_DATA_SEND </b><br>
 * \li \c <b> #IPOD_PLAYER_GPS_NMEA_FILTER </b><br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_GPS_STATUS)(U32 devID, U32 gpsStatusMask);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_GPS_DATA)(U32 devID, IPOD_PLAYER_GPS_TYPE type, U32 dataSize, U8 *data)
 * This function pointer is called when Apple device sends the data of GPS.<br>
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_GPS_TYPE. This is a type of GPS data.<br>
 * \param [in] dataSize Type is U32. size of receiving data<br>
 * \param [out] *data Type is U8 pointer. data that application receive<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_GPS_DATA)(U32 devID, IPOD_PLAYER_GPS_TYPE type, U32 dataSize, U8 *data);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_POSITION)(U32 devID, IPOD_PLAYER_GPS_TIME *time, IPOD_PLAYER_GPS_ANGLE *angle)
 * This function pointer is called when Apple device sends the current GPS location.<br>
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [out] *time Type is IPOD_PLAYER_GPS_TIME pointer. This strcture notifies the time by week.<br>
 * \param [out] *angle Type is IPOD_PLAYER_GPS_ANGLE pointer. This is structure of current GPS location angle.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_POSITION)(U32 devID, IPOD_PLAYER_GPS_TIME *time, IPOD_PLAYER_GPS_ANGLE *angle);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_TIME)(U32 devID, IPOD_PLAYER_GPS_TIME *time)
 * This function pointer is called when Apple device notifies the current GPS time.<br>
 * \param [in] devID Type is U32. This indicates the notified Apple device.<br>
 * \param [out] *time Type is IPOD_PLAYER_GPS_TIME pointer. This is a structure of GPS time.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_TIME)(U32 devID, IPOD_PLAYER_GPS_TIME *time);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_HMI_EVENT)(U32 devID, IPOD_PLAYER_HMI_STATUS_TYPE type, U32 status, IPOD_PLAYER_HMI_STATUS_DATE *date)
 * This function pointer is called when Appple device notifies the some event status in HMI mode
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] type Type is #IPOD_PLAYER_HMI_STATUS_TYPE. It is acquired status type.<br>
 * \param [in] status Type is U32. This value is status of indicated type. Meaning of value of each type can be described in #IPOD_PLAYER_HMI_STATUS_TYPE.<br>
 * \param [out] date Type is #IPOD_PLAYER_HMI_STATUS_DATE pointer. If application sets the #IPOD_PLAYER_HMI_STATUS_MASK_CURRENT_DATE in #iPodPlayerGetDeviceStatus,
 * this argument is enabled. If other type is set, this argument shall be NULL.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_HMI_EVENT)(U32 devID, IPOD_PLAYER_HMI_STATUS_TYPE type, U32 status, IPOD_PLAYER_HMI_STATUS_DATE *date);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT)(U32 devID, IPOD_PLAYER_DEVICE_EVENT_TYPE type, IPOD_PLAYER_DEVICE_EVENT_INFO *event)
 * This function pointer is called when Appple device notifies the some event status<br>
 * event parameter is an union. Application must decide which member shall be used by type.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] type Type is #IPOD_PLAYER_DEVICE_EVENT_TYPE. It is type of event.<br>
 * \param [out] event Type is #IPOD_PLAYER_DEVICE_EVENT_INFO. This value is status of indicated type. Meaning of value of each type can be described in #IPOD_PLAYER_DEVICE_EVENT_INFO.<br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT)(U32 devID, IPOD_PLAYER_DEVICE_EVENT_TYPE type, IPOD_PLAYER_DEVICE_EVENT_INFO *event);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_LOCATION_INFO_STATUS)(U32 devID, IPOD_PLAYER_LOCATION_INFO_TYPE type, U32 locationMask)
 * This function pointer is called when Appple device notifies the start / stop for location information <br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] status is #IPOD_PLAYER_LOCATION_INFO_STATUS. (start / stop of the location information) <br>
 * \param [in] locationMask shows type of NMEA sentences which can use iPodPlayerLoactionInformation. It is available at only start status.<br>
 * \li \c <b> #IPOD_PLAYER_NMEA_MASK_GPGGA </b><br>
 * \li \c <b> #IPOD_PLAYER_NMEA_MASK_GPRMC </b><br>
 * \li \c <b> #IPOD_PLAYER_NMEA_MASK_GPGSV </b><br>
 * \li \c <b> #IPOD_PLAYER_NMEA_MASK_PASCD </b><br>
 * \li \c <b> #IPOD_PLAYER_NMEA_MASK_PAGCD </b><br>
 * \li \c <b> #IPOD_PLAYER_NMEA_MASK_PAACD </b><br>
 * \li \c <b> #IPOD_PLAYER_NMEA_MASK_GPHDT </b><br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_LOCATION_INFO_STATUS)(U32 devID, IPOD_PLAYER_LOCATION_INFO_STATUS status, U32 locationMask);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NOTIFY_VEHICLE_STATUS)(U32 devID, IPOD_PLAYER_VEHICLE_STATUS_TYPE type)
 * This function pointer is called when Appple device notifies the start / stop for vehicle status  <br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] status is #IPOD_PLAYER_VEHICLE_STATUS. (start / stop of the vehicle status) <br>
 */
typedef void (*IPOD_PLAYER_CB_NOTIFY_VEHICLE_STATUS)(U32 devID, IPOD_PLAYER_VEHICLE_STATUS status);

/*\}*/

/**
 * \defgroup iPodPlayerCallback Result Callback
 */
 
/*\{*/

/**
 * \addtogroup PlaybackResult Playback Result
 */
/*\{*/


/*!
 * \ingroup PlaybackResult
 * \typedef void (*IPOD_PLAYER_CB_PLAY_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerPlay and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerPlay by this function.<br>
 * \param [in] devID Type is U32. This indicates the iPod that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_PLAY_RESULT)(U32 devID, S32 result);

/*!
 * \ingroup PlaybackCurrentSelectionResult
 * \typedef void (*IPOD_PLAYER_CB_PLAY_CURRENT_SELECTION_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerPlayCurrentSelection and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerPlayCurrentSelection by this function.<br>
 * \param [in] devID Type is U32. This indicates the iPod that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_PLAY_CURRENT_SELECTION_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_PAUSE_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerPause and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerPause by this function.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_PAUSE_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_STOP_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerStop and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerStop by this function.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_STOP_RESULT)(U32 devID, S32 result);


/*!
 * \typedef void (*IPOD_PLAYER_CB_NEXTTRACK_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerNextTrack and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerNextTrack.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_NEXTTRACK_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_PREVTRACK_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerPrevTrack and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerPrevTrack.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_STATUS Current plaback status is invalid status </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>

 */
typedef void (*IPOD_PLAYER_CB_PREVTRACK_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_NEXT_CHAPTER_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when 
 * the Application send the #iPodPlayerNextChapter and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerNextChapter.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_STATUS Current plaback status is invalid status </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_POSITION Chapter is tail in track </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NO_CHAPTER Track does not have chapter </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
 typedef void (*IPOD_PLAYER_CB_NEXT_CHAPTER_RESULT)(U32 devID, S32 result);
 
 /*!
 * \typedef void (*IPOD_PLAYER_CB_PREV_CHAPTER_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when 
 * the Application send the #iPodPlayerPrevChapter and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerPrevChapter.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_STATUS Current plaback status is invalid status </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_POSITION Chapter is tail in track </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NO_CHAPTER Track does not have chapter </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_PREV_CHAPTER_RESULT)(U32 devID, S32 result);
 

/*!
 * \typedef void (*IPOD_PLAYER_CB_FASTFORWARD_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerFastForward and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerFastForward.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_FASTFORWARD_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_REWIND_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerRewind and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerRewind.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_REWIND_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GOTO_TRACKPOSITION_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGotoTrackPosition and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGotoTrackPosition.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_STATUS Current plaback status is invalid status </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_GOTO_TRACKPOSITION_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_PLAY_TRACK_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerPlayTrack and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerPlayTrack.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used. </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_PLAY_TRACK_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_RELEASE_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerRelease and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerRelease.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used. </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_RELEASE_RESULT)(U32 devID, S32 result);

/*\}*/

/**
 * \addtogroup SetResult Set Property Result
 */
/*\{*/

/*!
 * \ingroup SetResult
 * \typedef void (*IPOD_PLAYER_CB_SET_REPEAT_STATUS_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetRepeatStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetRepeatStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_REPEAT_STATUS_RESULT)(U32 devID, S32 result);

/*!
 * \ingroup SetResult
 * \typedef void (*IPOD_PLAYER_CB_CHANGE_REPEAT_STATUS_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerChangeRepeatStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerChangeRepeatStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_CHANGE_REPEAT_STATUS_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_AUDIO_MODE_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetAudioMode and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetAudioMode.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_AUDIO_MODE_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_SHUFFLE_STATUS_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetShuffleStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetShuffleStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_SHUFFLE_STATUS_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_CHANGE_SHUFFLE_STATUS_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerChangeShuffleStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerChangeShuffleStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_CHANGE_SHUFFLE_STATUS_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_MODE_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetMode and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetMode.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used. </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_MODE_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_VIDEO_SETTING_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetVideoSetting and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetVideoSetting.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function or part of mask is not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_VIDEO_SETTING_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_EQUALIZER_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetEqualizer and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetEqualizer.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_EQUALIZER_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_PLAY_SPEED_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetPlaySpeed and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetPlaySpeed.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_STATUS current playback status is invalid status. </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function is not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_PLAY_SPEED_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_TRACK_INFO_NOTIFICATION_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetTrackInfoNotification and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetTrackInfoNotification.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function is not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_TRACK_INFO_NOTIFICATION_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_VIDEO_DELAY_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetVideoDelay and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetVideoDelay.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function is not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_VIDEO_DELAY_RESULT)(U32 devID, S32 result);


/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_DISPLAY_IMAGE_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetDisplayImage and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetDisplayImage.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function is not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_DISPLAY_IMAGE_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_DEVICE_EVENT_NOTIFICATION_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetDeviceEventNotification and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetDeviceEventNotification.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function is not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_DEVICE_EVENT_NOTIFICATION_RESULT)(U32 devID, S32 result);

/*\}*/

/**
 * \addtogroup GetResult Get Property Result
 */
/*\{*/

/*!
 * \ingroup GetResult
 * \typedef void (*IPOD_PLAYER_CB_GET_PLAYBACK_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_PLAYBACK_STATUS *status)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetPlaybackStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetPlaybackStatus.<br>
 * \param [in] devID Type is U32. This indicates the iPodPlayerGetPlaybackStatus device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [out] *status Type is #IPOD_PLAYER_PLAYBACK_STATUS pointer. status Current iPod status<br>
 */
typedef void (*IPOD_PLAYER_CB_GET_PLAYBACK_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_PLAYBACK_STATUS *status);


/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_TRACK_INFO_RESULT)(U32 devID, S32 result, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, IPOD_PLAYER_TRACK_INFO *info)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetTrackInfo and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetTrackInfo.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Type is kind of track<br>
 * \param [in] trackID Type is U64. Get the information that is indicated by this id.<br>
 * \param [out] *info Type is #IPOD_PLAYER_TRACK_INFO pointer. Track information structure of indicated track index.
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function or part of mask are not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_GET_TRACK_INFO_RESULT)(U32 devID, S32 result, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, IPOD_PLAYER_TRACK_INFO *info);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_CHAPTER_INFO_RESULT)(U32 devID, S32 result, U32 trackIndex, U32 chapterIndex, IPOD_PLAYER_CHAPTER_INFO *info)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetTrackInfo and return the result from iPod.<br>
 * Application can know the result of #iPodPlayerGetTrackInfo.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NO_CHAPTER Indicated track does not have chapter </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Type is kind of track<br>
 * \param [in] trackIndex Type is U32. Get the information that is indicated by this id.<br>
 * \param [in] chapterIndex Type is U32. Chapter index is indicated by #iPodPlayerGetChapterInfo
 * \param [out] *info Type is  #IPOD_PLAYER_CHAPTER_INFO. Information of indicated chapter index<br>
 */
typedef void (*IPOD_PLAYER_CB_GET_CHAPTER_INFO_RESULT)(U32 devID, S32 result, U32 trackIndex, U32 chapterIndex, IPOD_PLAYER_CHAPTER_INFO *info);


/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_MODE_RESULT)(U32 devID, S32 result, IPOD_PLAYER_MODE mode)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetMode and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetMode.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] mode Type is #IPOD_PLAYER_MODE. Current mode of Apple device.
 */
typedef void (*IPOD_PLAYER_CB_GET_MODE_RESULT)(U32 devID, S32 result, IPOD_PLAYER_MODE mode);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_REPEAT_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_REPEAT_STATUS status)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetRepeatStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetRepeatStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] status Type is #IPOD_PLAYER_REPEAT_STATUS.  current Apple device's repeat status<br>
 */
typedef void (*IPOD_PLAYER_CB_GET_REPEAT_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_REPEAT_STATUS status);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_SHUFFLE_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_SHUFFLE_STATUS status)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetShuffleStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetShuffleStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple devic that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param status Type is #IPOD_PLAYER_SHUFFLE_STATUS. current Apple device's shuffle status
 */
typedef void (*IPOD_PLAYER_CB_GET_SHUFFLE_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_SHUFFLE_STATUS status);


/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_VIDEO_SETTING_RESULT)(U32 devID, S32 result, IPOD_PLAYER_VIDEO_SETTING *setting)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetVideoSetting and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetVideoSetting.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function or part of mask are not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [out] *setting Type is IPOD_PLAYER_VIDEO_SETTING pointer. Structure of current video settings.
 */
typedef void (*IPOD_PLAYER_CB_GET_VIDEO_SETTING_RESULT)(U32 devID, S32 result, IPOD_PLAYER_VIDEO_SETTING *setting);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_EQUALIZER_RESULT)(U32 devID, S32 result, U8 value)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetEqualizer and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetEqualizer.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] value Type is U8. Value of current Apple device's equalizer setting
 */
typedef void (*IPOD_PLAYER_CB_GET_EQUALIZER_RESULT)(U32 devID, S32 result, U8 value);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_EQUALIZER_NAME_RESULT)(U32 devID, S32 result, U8 *name)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetEqualizer and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetEqualizer.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [out] *name Type is U8 pointer. Name of equalizer that is indicated . It is NULL terminated UTF8 strings.<br>
 */
typedef void (*IPOD_PLAYER_CB_GET_EQUALIZER_NAME_RESULT)(U32 devID, S32 result, U8 *name);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_PLAY_SPEED_RESULT)(U32 devID, S32 result, IPOD_PLAYER_PLAYING_SPEED speed)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetPlaySpeed and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetPlaySpeed.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param speed Type is #IPOD_PLAYER_PLAYING_SPEED. Value of current Apple device's playing speed.
 */
typedef void (*IPOD_PLAYER_CB_GET_PLAY_SPEED_RESULT)(U32 devID, S32 result, IPOD_PLAYER_PLAYING_SPEED speed);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_TRACK_TOTAL_COUNT_RESULT)(U32 devID, S32 result, IPOD_PLAYER_TRACK_TYPE type, U32 count)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetTrackTotalCount and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetTrackTotalCount.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used. </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Get the total count by this type.<br>
 * \param [in] count Type is U32. Number of tracks in indicated type.<br>
 */
typedef void (*IPOD_PLAYER_CB_GET_TRACK_TOTAL_COUNT_RESULT)(U32 devID, S32 result, IPOD_PLAYER_TRACK_TYPE type, U32 count);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_MEDIA_ITEM_INFO_RESULT)(U32 devID, S32 result, U64 trackID, S32 mediaType)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetMediaItemInformation and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetMediaItemInformation.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID/track ID is selected. </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used. </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] trackID type is U64. Get the track ID that used with #iPodPlayerGetMediaItemInformation.<br>
 * \param [in] mediaType type is S32. Get media type of media item using specified Track ID.<br>
 * If medialibraryupdate in progress, madiaType is -1. <br>
 */
typedef void (*IPOD_PLAYER_CB_GET_MEDIA_ITEM_INFO_RESULT)(U32 devID, S32 result, U64 trackID, S32 mediaType);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_COVERART_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetCoverArt and return the result from Apple device.<br>
 * Actual coverart data can get by #IPOD_PLAYER_CB_NOTIFY_COVERART_DATA.<br>
 * Application can know the result of #iPodPlayerGetCoverArt.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_GET_COVERART_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_DEVICE_PROPERTY_RESULT)(U32 devID, S32 result, IPOD_PLAYER_DEVICE_PROPERTY *property)
 * This is prototype of callback function that will be called when 
 * the Application send the #iPodPlayerGetDeviceProperty and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetDeviceProperty.<br>
 * Argument of *info sets the masked parameter.
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_APPLICABLE This feature is not applicable on iAP2</b><br>
 * \param [out] *property Tupe is #IPOD_PLAYER_DEVICE_PROPERTY pointer. structure of Apple device information.
 */
typedef void (*IPOD_PLAYER_CB_GET_DEVICE_PROPERTY_RESULT)(U32 devID, S32 result, IPOD_PLAYER_DEVICE_PROPERTY *property);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_DEVICE_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_DEVICE_STATUS *status)
 * This is prototype of callback function that will be called when 
 * the Application send the #iPodPlayerGetDeviceStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetDeviceStatus.<br>
 * Argument of *status sets the masked parameter.
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [out] *status Type is #IPOD_PLAYER_DEVICE_STATUS pointer. Structure of current Apple device's status.
 */
typedef void (*IPOD_PLAYER_CB_GET_DEVICE_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_DEVICE_STATUS *status);


/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_COVERART_INFO_RESULT)(U32 devID, S32 result, U32 timeCount, U32 *time)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetCoverArtInfo and return the result from iPod.<br>
 * Application can know the result of #iPodPlayerGetCoverArtInfo.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT This function or part of mask are not supported </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] timeCount Type is U32. This indicates the number of time. 
 * \param [out] *time Type is U32. This indicates the each time that coverart is set.<br>
 */
typedef void (*IPOD_PLAYER_CB_GET_COVERART_INFO_RESULT)(U32 devID, S32 result, U32 timeCount, U32 *time);


/*\}*/


/**
 * \addtogroup DatabaseResult Database Browsing Result
 */
/*\{*/

/*!
 * \ingroup DatabaseResult
 * \typedef void (*IPOD_PLAYER_CB_CANCEL_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when 
 * the Application send the #iPodPlayerCancel and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerCancel.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_CANCEL_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SELECT_DB_ENTRY_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSelectDBEntry and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSelectDBEntry.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SELECT_DB_ENTRY_RESULT)(U32 devID, S32 result);


/*!
 * \typedef void (*IPOD_PLAYER_CB_CLEAR_SELECTION)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerClearSelection and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerClearSelection.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_CLEAR_SELECTION)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SELECT_AV_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSelectAV and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSelectAV.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SELECT_AV_RESULT)(U32 devID, S32 result);


/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_DB_ENTRIES)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetDBEntries and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetDBEntries.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_GET_DB_ENTRIES)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_DB_COUNT_RESULT)(U32 devID, S32 result, U32 num)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetDBCount and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetDBCount.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_GET_DB_COUNT_RESULT)(U32 devID, S32 result, U32 num);


/*\}*/



/**
 * \addtogroup HMIControlResult HMI Control Result
 */
/*\{*/


/*!
 * \ingroup HMIControlResult
 * \typedef void (*IPOD_PLAYER_CB_HMI_SET_SUPPORTED_FEATURE_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMISetSupportedFeature and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMISetSupportedFeature.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_HMI_SET_SUPPORTED_FEATURE_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_HMI_GET_SUPPORTED_FEATURE_RESULT)(U32 devID, S32 result, IPOD_PLAYER_HMI_FEATURE_TYPE type, U32 optionsBits)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMIGetSupportedFeature and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMIGetSupportedFeature.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] type Type is #IPOD_PLAYER_HMI_FEATURE_TYPE. It indicates the kind of optionsBits.<br>
 * \param [in] optionsBits Type is U32. Options that current Apple device is set.Please see \ref HMIFeatureMask
 */
typedef void (*IPOD_PLAYER_CB_HMI_GET_SUPPORTED_FEATURE_RESULT)(U32 devID, S32 result, IPOD_PLAYER_HMI_FEATURE_TYPE type, U32 optionsBits);

/*!
 * \typedef void (*IPOD_PLAYER_CB_HMI_BUTTON_INPUT_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMIButtonInput and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMIButtonInput.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_HMI_BUTTON_INPUT_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_HMI_PLAYBACK_INPUT_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMIPlaybackInput and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMIPlaybackInput.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_HMI_PLAYBACK_INPUT_RESULT)(U32 devID, S32 result);


/*!
 * \typedef void (*IPOD_PLAYER_CB_HMI_ROTATION_INPUT_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMIRotationInput and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMIRotationInput.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_HMI_ROTATION_INPUT_RESULT)(U32 devID, S32 result);


/*!
 * \typedef void (*IPOD_PLAYER_CB_HMI_SET_APPLICATION_STATUS_RESULT )(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMISetApplicationStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMISetApplicationStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_HMI_SET_APPLICATION_STATUS_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_HMI_SET_EVENT_NOTIFICATION_RESULT )(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMISetEventNotification and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMISetEventNotification.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_HMI_SET_EVENT_NOTIFICATION_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_HMI_GET_EVENT_CHANGE_RESULT )(U32 devID, S32 result, U32 hmiEventMask)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMIGetEventChange and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMIGetEventChange.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] hmiEventMask Type is U32. This mask is current chaged status on bitmask.
 * if application could get the these bit, application can get the changed status by #iPodPlayerGetDeviceStatus. Please see \ref HMIEventMask<br>
 */
typedef void (*IPOD_PLAYER_CB_HMI_GET_EVENT_CHANGE_RESULT)(U32 devID, S32 result, U32 hmiEventMask);

/*!
 * \typedef void (*IPOD_PLAYER_CB_HMI_GET_DEVICE_STATUS_RESULT )(U32 devID, S32 result, IPOD_PLAYER_HMI_STATUS_TYPE type, U32 status, IPOD_PLAYER_HMI_STATUS_DATE *date)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerHMISetApplicationStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerHMISetApplicationStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] type Type is #IPOD_PLAYER_HMI_STATUS_TYPE It is acquired status type.<br>
 * \param [in] status Type is U32. This value is status of indicated type. Meaning of value of each type can be described in #IPOD_PLAYER_HMI_STATUS_TYPE.<br>
 * \param [out] date Type is #IPOD_PLAYER_HMI_STATUS_DATE pointer. If application sets the #IPOD_PLAYER_HMI_STATUS_MASK_CURRENT_DATE in #iPodPlayerGetDeviceStatus,
 * this argument is enabled. If other type is set, this argument shall be NULL.<br>
 */
typedef void (*IPOD_PLAYER_CB_HMI_GET_DEVICE_STATUS_RESULT)(U32 devID, S32 result, IPOD_PLAYER_HMI_STATUS_TYPE type, U32 status, IPOD_PLAYER_HMI_STATUS_DATE *date);



/*\}*/

/**
 * \addtogroup NonPlayerResult NonPlayer Result
 */
/*\{*/


/*!
 * \typedef void (*IPOD_PLAYER_CB_REQUEST_APP_START)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerRequestAppStart and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerRequestAppStart.<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_REQUEST_APP_START)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SEND_TO_APP_RESULT)(U32 devID, S32 result, U32 handle)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSendToApp and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSendToApp.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [in] handle Type is U32. This handle is that application indicated iOS Application handle by #iPodPlayerSendToApp.
 */
typedef void (*IPOD_PLAYER_CB_SEND_TO_APP_RESULT)(U32 devID, S32 result, U32 handle);

/*!
 * \typedef void (*IPOD_PLAYER_CB_LOCATION_INFO_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerLocationInformation and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerLocationInformation.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_LOCATION_INFO_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_VEHICLE_STATUS_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerVehicleStatus and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerVehicleStatus.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API<br>
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_VEHICLE_STATUS_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_OPEN_SONG_TAG_RESULT)(U32 devID, S32 result, U32 handle)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerOpenSongTagFile and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerOpenSongTagFile.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [out] handle Type is U32. This handle is used when Applcation calls #iPodPlayerSongTag<br>
 */
typedef void (*IPOD_PLAYER_CB_OPEN_SONG_TAG_RESULT)(U32 devID, S32 result, U32 handle);

/*!
 * \typedef void (*IPOD_PLAYER_CB_CLOSE_SONG_TAG_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerCloseSongTagFile and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerCloseSongTagFile.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_CLOSE_SONG_TAG_RESULT)(U32 devID, S32 result);


/*!
 * \typedef void (*IPOD_PLAYER_CB_SONG_TAG_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSongTag and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSongTag.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SONG_TAG_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_POWER_SUPPLY_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetPowerSupply and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetPowerSupply.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_POWER_SUPPLY_RESULT)(U32 devID, S32 result);


/*!
 * \typedef void (*IPOD_PLAYER_CB_SET_VOLUME_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetVolume and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetVolume.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_SET_VOLUME_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_VOLUME_RESULT)(U32 devID, S32 result, U8 *volume)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetVolume and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetVolume.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 * \param [out] volume Type is U8 pointer. This is the current voulme. 
 * The range of this value is between from 0 to 100. 0 means the mute, 100 menas the maximum volume.<br>
 */
typedef void (*IPOD_PLAYER_CB_GET_VOLUME_RESULT)(U32 devID, S32 result, U8 *volume);

/*!
 * \typedef void (*IPOD_PLAYER_CB_CREATE_INTELLIGENT_PL_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerCreateIntelligentPL and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerCreateIntelligentPL.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT Apple device does not suppport this function</b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_CREATE_INTELLIGENT_PL_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_GET_PL_PROPERTIES_RESULT)(U32 devID, S32 result, U32 capble)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerGetPLProperties and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerGetPLProperties.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT Apple device does not suppport this function</b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_GET_PL_PROPERTIES_RESULT)(U32 devID, S32 result, U32 capble);

/*!
 * \typedef void (*IPOD_PLAYER_CB_REFRESH_INTELLIGENT_PL_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerRefreshIntelligentPL and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerRefreshIntelligentPL.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT Apple device does not suppport this function</b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_REFRESH_INTELLIGENT_PL_RESULT)(U32 devID, S32 result);

/*!
 * \typedef void (*IPOD_PLAYER_CB_TRACK_SUPPORTS_INTELLIGENT_PL_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerTrackSupportsIntelligentPL and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerTrackSupportsIntelligentPL.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_SUPPORT Apple device does not suppport this function</b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_TRACK_SUPPORTS_INTELLIGENT_PL_RESULT)(U32 devID, S32 result);

/*\}*/

/**
 * \addtogroup DeviceDetectionResult DeviceDetection Result
 */
/*\{*/



/*!
 * \ingroup DeviceDetectionResult
 * \typedef void (*IPOD_PLAYER_CB_DEVICE_DETECTION_RESULT)(U32 devID, S32 result)
 * This is prototype of callback function that will be called when
 * the Application send the #iPodPlayerSetDeviceDetection and return the result from Apple device.<br>
 * Application can know the result of #iPodPlayerSetDeviceDetection by this function.<br>
 * \param [in] devID Type is U32. This indicates the Apple device that received command.<br>
 * \param [in] result Type is S32. Result of iPodPlayer Player API
 * \li \c <b> #IPOD_PLAYER_OK Function Success </b><br>
 * \li \c <b> #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected </b><br>
 * \li \c <b> #IPOD_PLAYER_ERROR Other error </b><br>
 */
typedef void (*IPOD_PLAYER_CB_DEVICE_DETECTION_RESULT)(U32 devID, S32 result);

/*\}*/

/*!
 * \struct IPOD_PLAYER_REGISTER_CB_TABLE
 * This is an structure of register function table.<br>
 * This structure is used to register the function result and notify function in #iPodPlayerInit.
 */
typedef struct
{
    /*! See #IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS */
    IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS               cbNotifyPlaybackStatus;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_CONNECTION_STATUS */
    IPOD_PLAYER_CB_NOTIFY_CONNECTION_STATUS             cbNotifyConnectionStatus;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_PLAYBACK_CHANGE */
    IPOD_PLAYER_CB_NOTIFY_PLAYBACK_CHANGE               cbNotifyPlaybackChange;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_TRACK_INFO */
    IPOD_PLAYER_CB_NOTIFY_TRACK_INFO                    cbNotifyTrackInfo;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_DB_ENTRIES */
    IPOD_PLAYER_CB_NOTIFY_DB_ENTRIES                    cbNotifyDBEntries;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_COVERART_DATA */
    IPOD_PLAYER_CB_NOTIFY_COVERART_DATA                 cbNotifyCoverartData;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_OPEN_APP */
    IPOD_PLAYER_CB_NOTIFY_OPEN_APP                      cbNotifyOpenApp;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_CLOSE_APP */
    IPOD_PLAYER_CB_NOTIFY_CLOSE_APP                     cbNotifyCloseApp;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_RECEIVE_FROM_APP */
    IPOD_PLAYER_CB_NOTIFY_RECEIVE_FROM_APP              cbNotifyReceiveFromApp;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_GPS_STATUS */
    IPOD_PLAYER_CB_NOTIFY_GPS_STATUS                    cbNotifyGPSStatus;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_GPS_DATA */
    IPOD_PLAYER_CB_NOTIFY_GPS_DATA                      cbNotifyGPSData;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_POSITION */
    IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_POSITION          cbNotifyGPSCurrentPosition;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_TIME */
    IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_TIME              cbNotifyGPSCurrentTime;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_HMI_EVENT */
    IPOD_PLAYER_CB_NOTIFY_HMI_EVENT                     cbNotifyHMIEvent;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT */
    IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT                  cbNotifyDeviceEvent;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_LOCATION_INFO_STATUS */
    IPOD_PLAYER_CB_NOTIFY_LOCATION_INFO_STATUS          cbNotifyLocationInfoStatus;
    
    /*! See #IPOD_PLAYER_CB_NOTIFY_VEHICLE_STATUS */
    IPOD_PLAYER_CB_NOTIFY_VEHICLE_STATUS                cbNotifyVehicleStatus;
    
    /*! See #IPOD_PLAYER_CB_PLAY_RESULT */
    IPOD_PLAYER_CB_PLAY_RESULT                          cbPlayResult;
    
    /*! See #IPOD_PLAYER_CB_PLAY_CURRENT_SELECTION_RESULT */
    IPOD_PLAYER_CB_PLAY_CURRENT_SELECTION_RESULT        cbPlayCurrentSelectionResult;
    
    /*! See #IPOD_PLAYER_CB_PAUSE_RESULT */
    IPOD_PLAYER_CB_PAUSE_RESULT                         cbPauseResult;
    
    /*! See #IPOD_PLAYER_CB_STOP_RESULT */
    IPOD_PLAYER_CB_STOP_RESULT                          cbStopResult;
    
    /*! See #IPOD_PLAYER_CB_NEXTTRACK_RESULT */
    IPOD_PLAYER_CB_NEXTTRACK_RESULT                     cbNextTrackResult;
    
    /*! See #IPOD_PLAYER_CB_PREVTRACK_RESULT */
    IPOD_PLAYER_CB_PREVTRACK_RESULT                     cbPrevTrackResult;
    
    /*! See #IPOD_PLAYER_CB_NEXT_CHAPTER_RESULT */
    IPOD_PLAYER_CB_NEXT_CHAPTER_RESULT                  cbNextChapterResult;
    
    /*! See #IPOD_PLAYER_CB_PREV_CHAPTER_RESULT */
    IPOD_PLAYER_CB_PREV_CHAPTER_RESULT                  cbPrevChapterResult;
    
    /*! See #IPOD_PLAYER_CB_FASTFORWARD_RESULT */
    IPOD_PLAYER_CB_FASTFORWARD_RESULT                   cbFastforwardResult;
    
    /*! See #IPOD_PLAYER_CB_REWIND_RESULT */
    IPOD_PLAYER_CB_REWIND_RESULT                        cbRewindResult;
    
    /*! See #IPOD_PLAYER_CB_GOTO_TRACKPOSITION_RESULT */
    IPOD_PLAYER_CB_GOTO_TRACKPOSITION_RESULT            cbGotoTrackPositionResult;
    
    /*! See #IPOD_PLAYER_CB_PLAY_TRACK_RESULT */
    IPOD_PLAYER_CB_PLAY_TRACK_RESULT                    cbPlayTrackResult;
    
    /*! See #IPOD_PLAYER_CB_RELEASE_RESULT */
    IPOD_PLAYER_CB_RELEASE_RESULT                       cbReleaseResult;
    
    /*! See #IPOD_PLAYER_CB_SET_AUDIO_MODE_RESULT */
    IPOD_PLAYER_CB_SET_AUDIO_MODE_RESULT                cbSetAudioModeResult;
    
    /*! See #IPOD_PLAYER_CB_SET_MODE_RESULT */
    IPOD_PLAYER_CB_SET_MODE_RESULT                      cbSetModeResult;
    
    /*! See #IPOD_PLAYER_CB_SET_REPEAT_STATUS_RESULT */
    IPOD_PLAYER_CB_SET_REPEAT_STATUS_RESULT             cbSetRepeatStatusResult;
    
    /*! See #IPOD_PLAYER_CB_SET_SHUFFLE_STATUS_RESULT */
    IPOD_PLAYER_CB_SET_SHUFFLE_STATUS_RESULT            cbSetShuffleStatusResult;
    
    /*! See #IPOD_PLAYER_CB_CHANGE_REPEAT_STATUS_RESULT */
    IPOD_PLAYER_CB_CHANGE_REPEAT_STATUS_RESULT          cbChangeRepeatStatusResult;
    
    /*! See #IPOD_PLAYER_CB_CHANGE_SHUFFLE_STATUS_RESULT */
    IPOD_PLAYER_CB_CHANGE_SHUFFLE_STATUS_RESULT         cbChangeShuffleStatusResult;
    
    /*! See #IPOD_PLAYER_CB_SET_EQUALIZER_RESULT */
    IPOD_PLAYER_CB_SET_EQUALIZER_RESULT                 cbSetEqualizerResult;
    
    /*! See #IPOD_PLAYER_CB_SET_VIDEO_DELAY_RESULT */
    IPOD_PLAYER_CB_SET_VIDEO_DELAY_RESULT               cbSetVideoDelayResult;
    
    /*! See #IPOD_PLAYER_CB_SET_VIDEO_SETTING_RESULT */
    IPOD_PLAYER_CB_SET_VIDEO_SETTING_RESULT             cbSetVideoSettingResult;
    
    /*! See #IPOD_PLAYER_CB_SET_DISPLAY_IMAGE_RESULT */
    IPOD_PLAYER_CB_SET_DISPLAY_IMAGE_RESULT             cbSetDisplayImageResult;
    
    /*! See #IPOD_PLAYER_CB_SET_PLAY_SPEED_RESULT */
    IPOD_PLAYER_CB_SET_PLAY_SPEED_RESULT                cbSetPlaySpeedResult;
    
    /*! See #IPOD_PLAYER_CB_SET_TRACK_INFO_NOTIFICATION_RESULT */
    IPOD_PLAYER_CB_SET_TRACK_INFO_NOTIFICATION_RESULT   cbSetTrackInfoNotificationResult;
    
    /*! See #IPOD_PLAYER_CB_SET_DEVICE_EVENT_NOTIFICATION_RESULT */
    IPOD_PLAYER_CB_SET_DEVICE_EVENT_NOTIFICATION_RESULT cbSetDeviceEventNotificationResult;
    
    /*! See #IPOD_PLAYER_CB_GET_VIDEO_SETTING_RESULT */
    IPOD_PLAYER_CB_GET_VIDEO_SETTING_RESULT             cbGetVideoSettingResult;
    
    /*! See #IPOD_PLAYER_CB_GET_COVERART_INFO_RESULT */
    IPOD_PLAYER_CB_GET_COVERART_INFO_RESULT             cbGetCoverartInfoResult;
    
    /*! See #IPOD_PLAYER_CB_GET_COVERART_RESULT */
    IPOD_PLAYER_CB_GET_COVERART_RESULT                  cbGetCoverartResult;
    
    /*! See #IPOD_PLAYER_CB_GET_PLAYBACK_STATUS_RESULT */
    IPOD_PLAYER_CB_GET_PLAYBACK_STATUS_RESULT           cbGetPlaybackStatusResult;
    
    /*! See #IPOD_PLAYER_CB_GET_TRACK_INFO_RESULT */
    IPOD_PLAYER_CB_GET_TRACK_INFO_RESULT                cbGetTrackInfoResult;
    
    /*! See #IPOD_PLAYER_CB_GET_CHAPTER_INFO_RESULT */
    IPOD_PLAYER_CB_GET_CHAPTER_INFO_RESULT              cbGetChapterInfoResult;
    
    /*! See #IPOD_PLAYER_CB_GET_MODE_RESULT */
    IPOD_PLAYER_CB_GET_MODE_RESULT                      cbGetModeResult;
    
    /*! See #IPOD_PLAYER_CB_GET_REPEAT_STATUS_RESULT */
    IPOD_PLAYER_CB_GET_REPEAT_STATUS_RESULT             cbGetRepeatStatusResult;
    
    /*! See #IPOD_PLAYER_CB_GET_SHUFFLE_STATUS_RESULT */
    IPOD_PLAYER_CB_GET_SHUFFLE_STATUS_RESULT            cbGetShuffleStatusResult;
    
    /*! See #IPOD_PLAYER_CB_GET_PLAY_SPEED_RESULT */
    IPOD_PLAYER_CB_GET_PLAY_SPEED_RESULT                cbGetPlaySpeedResult;

    /*! See #IPOD_PLAYER_CB_GET_TRACK_TOTAL_COUNT_RESULT */
    IPOD_PLAYER_CB_GET_TRACK_TOTAL_COUNT_RESULT         cbGetTrackTotalCountResult;
    
    /*! See #IPOD_PLAYER_CB_GET_MEDIA_ITEM_INFO_RESULT */
    IPOD_PLAYER_CB_GET_MEDIA_ITEM_INFO_RESULT           cbGetMediaItemInfoResult;

    /*! See #IPOD_PLAYER_CB_GET_EQUALIZER_RESULT */
    IPOD_PLAYER_CB_GET_EQUALIZER_RESULT                 cbGetEqualizerResult;
    
    /*! See #IPOD_PLAYER_CB_GET_EQUALIZER_NAME_RESULT */
    IPOD_PLAYER_CB_GET_EQUALIZER_NAME_RESULT            cbGetEqualizerNameResult;
    
    /*! See #IPOD_PLAYER_CB_GET_DEVICE_PROPERTY_RESULT */
    IPOD_PLAYER_CB_GET_DEVICE_PROPERTY_RESULT           cbGetDevicePropertyResult;
    
    /*! See #IPOD_PLAYER_CB_GET_DEVICE_STATUS_RESULT */
    IPOD_PLAYER_CB_GET_DEVICE_STATUS_RESULT             cbGetDeviceStatusResult;
    
    /*! See #IPOD_PLAYER_CB_GET_DB_ENTRIES */
    IPOD_PLAYER_CB_GET_DB_ENTRIES                       cbGetDBEntriesResult;

    /*! See #IPOD_PLAYER_CB_GET_DB_COUNT_RESULT */
    IPOD_PLAYER_CB_GET_DB_COUNT_RESULT                  cbGetDBCountResult;
    
    /*! See #IPOD_PLAYER_CB_SELECT_DB_ENTRY_RESULT */
    IPOD_PLAYER_CB_SELECT_DB_ENTRY_RESULT               cbSelectDBEntryResult;
    
    /*! See #IPOD_PLAYER_CB_CANCEL_RESULT */
    IPOD_PLAYER_CB_CANCEL_RESULT                        cbCancelResult;
    
    /*! See #IPOD_PLAYER_CB_CLEAR_SELECTION */
    IPOD_PLAYER_CB_CLEAR_SELECTION                      cbClearSelectionResult;
    
    /*! See #IPOD_PLAYER_CB_SELECT_AV_RESULT */
    IPOD_PLAYER_CB_SELECT_AV_RESULT                     cbSelectAVResult;
    
    /*! See #IPOD_PLAYER_CB_HMI_SET_SUPPORTED_FEATURE_RESULT */
    IPOD_PLAYER_CB_HMI_SET_SUPPORTED_FEATURE_RESULT     cbHMISetSupportedFeatureResult;
    
    /*! See #IPOD_PLAYER_CB_HMI_GET_SUPPORTED_FEATURE_RESULT */
    IPOD_PLAYER_CB_HMI_GET_SUPPORTED_FEATURE_RESULT     cbHMIGetSupportedFeatureResult;
    
    /*! See #IPOD_PLAYER_CB_HMI_BUTTON_INPUT_RESULT */
    IPOD_PLAYER_CB_HMI_BUTTON_INPUT_RESULT              cbHMIButtonInputResult;
    
    /*! See #IPOD_PLAYER_CB_HMI_PLAYBACK_INPUT_RESULT */
    IPOD_PLAYER_CB_HMI_PLAYBACK_INPUT_RESULT            cbHMIPlaybackInputResult;
    
    /*! See #IPOD_PLAYER_CB_HMI_ROTATION_INPUT_RESULT */
    IPOD_PLAYER_CB_HMI_ROTATION_INPUT_RESULT            cbHMIRotationInputResult;
    
    /*! See #IPOD_PLAYER_CB_HMI_SET_APPLICATION_STATUS_RESULT */
    IPOD_PLAYER_CB_HMI_SET_APPLICATION_STATUS_RESULT    cbHMISetApplicationStatusResult;
    
    /*! See #IPOD_PLAYER_CB_HMI_SET_EVENT_NOTIFICATION_RESULT */
    IPOD_PLAYER_CB_HMI_SET_EVENT_NOTIFICATION_RESULT    cbHMISetEventNotificationResult;
    
    /*! See #IPOD_PLAYER_CB_REQUEST_APP_START */
    IPOD_PLAYER_CB_REQUEST_APP_START                    cbRequestAppStartResult;
    
    /*! See #IPOD_PLAYER_CB_SET_VOLUME_RESULT */
    IPOD_PLAYER_CB_SET_VOLUME_RESULT                    cbSetVolumeResult;
    
    /*! See #IPOD_PLAYER_CB_GET_VOLUME_RESULT */
    IPOD_PLAYER_CB_GET_VOLUME_RESULT                    cbGetVolumeResult;
    
    /*! See #IPOD_PLAYER_CB_SEND_TO_APP_RESULT */
    IPOD_PLAYER_CB_SEND_TO_APP_RESULT                   cbSendToAppResult;
    
    /*! See #IPOD_PLAYER_CB_LOCATION_INFO_RESULT */
    IPOD_PLAYER_CB_LOCATION_INFO_RESULT                 cbLocationInfoResult;
    
    /*! See #IPOD_PLAYER_CB_VEHICLE_STATUS_RESULT */
    IPOD_PLAYER_CB_VEHICLE_STATUS_RESULT                cbVehicleStatusResult;
    
    /*! See #IPOD_PLAYER_CB_OPEN_SONG_TAG_RESULT */
    IPOD_PLAYER_CB_OPEN_SONG_TAG_RESULT                 cbOpenSongTagResult;
    
    /*! See #IPOD_PLAYER_CB_CLOSE_SONG_TAG_RESULT */
    IPOD_PLAYER_CB_CLOSE_SONG_TAG_RESULT                cbCloseSongTagResult;
    
    /*! See #IPOD_PLAYER_CB_SONG_TAG_RESULT */
    IPOD_PLAYER_CB_SONG_TAG_RESULT                      cbSongTagResult;
    
    /*! See #IPOD_PLAYER_CB_SET_POWER_SUPPLY_RESULT */
    IPOD_PLAYER_CB_SET_POWER_SUPPLY_RESULT              cbSetPowerSupplyResult;
    
    /*! See #IPOD_PLAYER_CB_CREATE_INTELLIGENT_PL_RESULT */
    IPOD_PLAYER_CB_CREATE_INTELLIGENT_PL_RESULT         cbCreateIntelligentPLResult;
    
    /*! See #IPOD_PLAYER_CB_GET_PL_PROPERTIES_RESULT */
    IPOD_PLAYER_CB_GET_PL_PROPERTIES_RESULT                cbGetPLPropertiesResult;
    
    /*! See #IPOD_PLAYER_CB_REFRESH_INTELLIGENT_PL_RESULT */
    IPOD_PLAYER_CB_REFRESH_INTELLIGENT_PL_RESULT        cbRefreshIntelligentPLResult;
    
    /*! See #IPOD_PLAYER_CB_TRACK_SUPPORTS_INTELLIGENT_PL_RESULT */
    IPOD_PLAYER_CB_TRACK_SUPPORTS_INTELLIGENT_PL_RESULT cbTrackSupportsIntelligentPLResult; 
    
    /*! See #IPOD_PLAYER_CB_DEVICE_DETECTION_RESULT */
    IPOD_PLAYER_CB_DEVICE_DETECTION_RESULT              cbDeviceDetectionResult;
} IPOD_PLAYER_REGISTER_CB_TABLE;

/*\}*/

#ifdef __cplusplus
}
#endif

#endif /* IPOD_PALEYR_CB_H */
