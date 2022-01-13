/*
 *  \File: iap_types.h
 *
 *  \Component: iAP types
 *
 */


#ifndef IAP_TYPES_H
#define IAP_TYPES_H

#include <adit_typedef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* ========================================================================== */
/* enums                                                                      */
/* ========================================================================== */

/**
 * \addtogroup Enums
 */

/*\{*/

/*! 
 * \typedef IPOD_CONNECTION_TYPE
 * \par DESCRIPTION
 * This enumeration defines where an Apple device is connected.<br>
 * An Apple device may be connected to the USB Host or USB Function port, via Bluetooth or UART.
 */
typedef enum
{
    IPOD_NO_CONNECT         = 0x00,    /*!< Apple device is not initialized */
    IPOD_USB_HOST_CONNECT   = 0x01,    /*!< Apple device is connected to accessory's USB Host port */
    IPOD_USB_FUNC_CONNECT   = 0x02,    /*!< Apple device is connected to accessory's USB Function port */
    IPOD_BT_CONNECT         = 0x03,    /*!< Apple device is connected to accessory by BT */
    IPOD_UART_CONNECT       = 0x04,    /*!< Apple device is connected to accessory by UART cable */
    IPOD_MAX_CONNECT_TYPE              /*!< Max. value for memory allocation */
} IPOD_CONNECTION_TYPE;


/*!
 * \typedef IPOD_CATEGORY 
 * \par DESCRIPTION
 * This enumeration is kind of category.<br>
 * A playlist can contain either tracks or other playlist,but not both.
 */
typedef enum
{
    IPOD_CAT_TOPLIST = 0x00,   /*!< Category is Toplist */
    IPOD_CAT_PLAYLIST = 0x01,  /*!< Category is Playlist */
    IPOD_CAT_ARTIST = 0x02,    /*!< Category is Artist */
    IPOD_CAT_ALBUM = 0x03,     /*!< Category is Album */
    IPOD_CAT_GENRE = 0x04,     /*!< Category is Genre */
    IPOD_CAT_TRACK = 0x05,     /*!< Category is Track */
    IPOD_CAT_COMPOSER = 0x06,  /*!< Category is Composer */
    IPOD_CAT_AUDIOBOOK = 0x07, /*!< Category is Audiobook; This can be used by 1.06 or more. */
    IPOD_CAT_PODCAST = 0x08,   /*!< Category is Podcast; This can be used by 1.08 or more. */
    IPOD_CAT_NESTED_PLAYLIST = 0x09, /*!< Category is Nested playlist<br> A playlist can contain other playlist; This can be used by 1.13 or more. */
    IPOD_CAT_GENIUS = 0x0A,    /*!< Category is Genius playlist */
    IPOD_CAT_ITUNESU = 0x0B    /*!< Category is iTunes University lecture */

} IPOD_CATEGORY;

/*! 
 * \typedef IPOD_DB_SORT_TYPE 
 * \par DESCRIPTION
 * This enumeration is kind of sort type.<br>
 * The default order of song and audiobook on the iPod is alphabetical by artist.<br>
 * then alphabetical by that artist's albums, then ordered according to the order of the tracks
 * 
 */
typedef enum
{
    IPOD_SORT_DB_BY_GENRE = 0x00,    /*!< Sort type is genre */
    IPOD_SORT_DB_BY_ARTIST,          /*!< Sort type is artist */
    IPOD_SORT_DB_BY_COMPOSER,        /*!< Sort type is composer */
    IPOD_SORT_DB_BY_ALBUM,           /*!< Sort type is album */
    IPOD_SORT_DB_BY_NAME,            /*!< Sort type is name */
    IPOD_SORT_DB_BY_PLAYLIST,        /*!< Sort type is playlist */
    IPOD_SORT_DB_BY_DATE,            /*!< Sort type is date; This can be used by 1.08 or more */
    IPOD_SORT_DB_BY_SERIES,          /*!< Sort type is TV series; This can be used by 1.12 or more (video only). */
    IPOD_SORT_DB_BY_SEASON,          /*!< Sort type is TV season; This can be used by 1.12 or more (video only) */
    IPOD_SORT_DB_BY_EPISODE,         /*!< Sort type is episode from the same season; This can be used by 1.12 or more (video only) */
    IPOD_SORT_DB_BY_DEFAULT = 0xFF   /*!< Sort type is default */
} IPOD_DB_SORT_TYPE;


/*!
 * \typedef IPOD_LINGO
 * \par DESCRIPTION
 * This enumeration is kind of several Lingo.
 */
typedef enum
{
    IPOD_LINGO_GENERAL = 0x00,        /*!< Lingo for general commands*/
    IPOD_LINGO_MICROPHONE,            /*!< Lingo for microphone commands */
    IPOD_LINGO_SIMPLE_REMOTE,         /*!< Lingo for simple remote commands */
    IPOD_LINGO_DISPLAY_REMOTE,        /*!< Lingo for display remote commands */
    IPOD_LINGO_EXTENDED_INTERFACE,    /*!< Lingo for xtended mode commands */
    IPOD_LINGO_RF_TRANSMITTER,        /*!< This Lingo is Accessory Power Lingo in currentry version*/
    IPOD_LINGO_USB_HOST_CONTROL,      /*!< This Lingo is deprecated; do not use it in new products */
    IPOD_LINGO_RF_TUNER,              /*!< Lingo for RF tuner commands */
    IPOD_LINGO_EQUALIZER,             /*!< Lingo for accessory equalizer commands */
    IPOD_LINGO_SPORTS,                /*!< Lingo for sport lingo */
    IPOD_LINGO_DIGITAL_AUDIO = 0x0A,  /*!< Lingo for digital audio commands */
    IPOD_LINGO_STORAGE = 0x0C,        /*!< Lingo for storage commands */
    IPOD_LINGO_OUT,                   /*!< Lingo for iPod out commands */
    IPOD_LINGO_LOCATION,              /*!< Lingo for location commands */
    IPOD_LINGO_MAX                    /*!< Lingo max number */
} IPOD_LINGO;

/*!
 * \typedef IPOD_TRACK_INFO_TYPE
 * \par DESCRIPTION
 * This enumeration is type of track information.
 */
typedef enum
{
    IPOD_TRACK_CAP_INFO = 0x00,       /*!< This is type for getting what track have. */
    IPOD_TRACK_PODCAST_NAME,          /*!< This is type for getting podcast name. */
    IPOD_TRACK_RELEASE_DATE,          /*!< This is type for getting track release date */
    IPOD_TRACK_DESCRIPTION,           /*!< This is type for getting track Description */
    IPOD_TRACK_SONG_LYRICS,           /*!< This is type for getting track song lyrics */
    IPOD_TRACK_GENRE,                 /*!< This is type for getting track genre */
    IPOD_TRACK_COMPOSER,              /*!< This is type for getting track composer */
    IPOD_TRACK_ARTWORK_COUNT          /*!< This is type for getting track artwork count */

} IPOD_TRACK_INFO_TYPE;



/*!
 * \typedef IPOD_PLAYER_STATE
 * \par DESCRIPTION
 * This enumeration is status of player.
 */
typedef enum
{
        IPOD_PLAYER_STATE_STOPPED = 0x00,    /*!< Player state is stop */
        IPOD_PLAYER_STATE_PLAYING,           /*!< Player state is playing */
        IPOD_PLAYER_STATE_PAUSED,            /*!< Player state is pause */
        IPOD_PLAYER_STATE_ERROR = 0xFF       /*!< Player state error */

} IPOD_PLAYER_STATE;

/*!
 * \typedef IPOD_STATUS_CHANGE_NOTIFICATION
 * \par DESCRIPTION
 * This enumeration is value of whether notify or not.
 */
typedef enum
{
    IPOD_STATUS_CHANGE_NOTIFICATION_DISABLE = 0x00,  /*!< Disable all status event notification */
    IPOD_STATUS_CHANGE_NOTIFICATION_ENABLE           /*!< Enable play status notifications for basic play state, <br>
                                                      * track index, track time position, FFW?REW seek stop, and chapter index changes */

} IPOD_STATUS_CHANGE_NOTIFICATION;


/*!
 * \typedef IPOD_CHANGED_PLAY_STATUS
 * \par DESCRIPTION
 * This enumeration is status of play changed
 */
typedef enum
{
    IPOD_STATUS_PLAYBACK_STOPPED = 0x00,    /*!< Status of playback stopped */
    IPOD_STATUS_TRACK_CHANGED = 0x01,       /*!< Status of track index. Currentry version of spec is Track index. */
    IPOD_STATUS_FWD_SEEK_STOP = 0x02,       /*!< Status of Fastforward seek stop */
    IPOD_STATUS_BWD_SEEK_STOP = 0x03,       /*!< Status of FastBackward seek stop */
    IPOD_STATUS_TRACK_POSITION = 0x04,      /*!< Status of track position(ms)  */
    IPOD_STATUS_CHAPTER_CHANGED = 0x05,     /*!< Status of chapter index.Currentry version of spec is Chapter index*/
    IPOD_STATUS_PLAYBACK_EXTENDED = 0x06,   /*!< Status of playback(e.g. stopped,playing,paused etc) */
    IPOD_STATUS_TRACK_TIME_OFFSET = 0x07,   /*!< Status of track time offset(s) */
    IPOD_STATUS_CHAPTER_MS_OFFSET = 0x08,   /*!< Status of chapter time offset(ms) */
    IPOD_STATUS_CHAPTER_SEC_OFFSET = 0x09,  /*!< Status of chapter time offset(s) */
    IPOD_STATUS_TRACK_UID = 0x0A,           /*!< Status of track unique identifier */
    IPOD_STATUS_TRACK_PLAYBACK_MODE = 0x0B, /*!< Status of track playback mode */
    IPOD_STATUS_TRACK_LYRICS_READY = 0x0C,  /*!< Status of lyrics for the currently playing track are available for download */
    IPOD_STATUS_TRACK_CAPABILITIES_CHANGED = 0x0D,  /* Status of track capabilities changed */
    IPOD_STATUS_PLAYBACK_CONTENT_CHANGED = 0x0E     /* Status of number of tracks in new playlist changed */
} IPOD_CHANGED_PLAY_STATUS;

/*!
 * \typedef IPOD_STATE_INFO_TYPE
 * \par DESCRIPTION
 * This enumeration is a list of information types supported by the 
 * Display Remote Lingo commands "GetiPodStateInfo" and "SetiPodStateInfo".
 */
typedef enum
{
    IPOD_STATE_INFO_TRACK_POSITION_MS = 0x00,    /*!< Position of the track in ms */
    IPOD_STATE_INFO_TRACK_INDEX = 0x01,      /*!< Index of the playing track / track to play */
    IPOD_STATE_INFO_CHAPTER_INFO = 0x02,      /*!< Current / new chapter information */
    IPOD_STATE_INFO_PLAY_STATUS = 0x03,      /*!< Play status of the Apple device */
    IPOD_STATE_INFO_MUTE_VOLUME = 0x04,      /*!< Mute setting and volume level */
    IPOD_STATE_INFO_POWER_BATTERY = 0x05,      /*!< Read only, power and battery levels */
    IPOD_STATE_INFO_EQUALIZER_STATE = 0x06,      /*!< Equalizer Setting */
    IPOD_STATE_INFO_SHUFFLE = 0x07,      /*!< State of the shuffle setting */
    IPOD_STATE_INFO_REPEAT = 0x08,      /*!< State of the repeat setting */
    IPOD_STATE_INFO_DATE_TIME = 0x09,      /*!< Setting of date and time, read only on iOS devices */
    IPOD_STATE_INFO_ALARM = 0x0A,      /*!< Deprecated; do not use */
    IPOD_STATE_INFO_BACKLIGHT = 0x0B,      /*!< Level of the backlight setting */
    IPOD_STATE_INFO_HOLD_SWITCH = 0x0C,      /*!< Reserved, can not be set */
    IPOD_STATE_INFO_SOUND_CHECK = 0x0D,      /*!< Sound check state */
    IPOD_STATE_INFO_AUDIOBOOK_SPEED = 0x0E,      /*!< The audiobook playback speed */
    IPOD_STATE_INFO_TRACK_POSITION_S = 0x0F,      /*!< Position of the track in seconds */
    IPOD_STATE_INFO_MUTE_EXTENDED_VOLUME = 0x10,      /*!< Mute setting and UI / absolute volume level */
    IPOD_STATE_INFO_TRACK_CAPABILITIES = 0x11,      /*!< Track capabilities */
    IPOD_STATE_INFO_NUM_OF_TRACKS_IN_PLAYLIST = 0x12    /*!< numer of tracks in new playlist */
} IPOD_STATE_INFO_TYPE;


/*!
 * \typedef IPOD_SHUFFLE_MODE
 * \par DESCRIPTION
 * This enumeration is mode of shuffle
 */
typedef enum
{
    IPOD_SHUFFLE_OFF = 0x00,      /*!< Shuffle mode is off */
    IPOD_SHUFFLE_TRACKS,          /*!< Shuffle mode is track shuffle */
    IPOD_SHUFFLE_ALBUMS           /*!< Shuffle mode is album shuffle */
} IPOD_SHUFFLE_MODE;

/*!
 * \typedef IPOD_REPEAT_MODE
 * \par DESCRIPTION
 * This enumeration is mode of repeat
 */
typedef enum
{
    IPOD_REPEAT_OFF = 0x00,    /*!< Repeat mode is off */
    IPOD_REPEAT_ONE_TRACK,     /*!< Repeat mode is one track */
    IPOD_REPEAT_ALL_TRACKS     /*!< Repeat mode is all tracks */
} IPOD_REPEAT_MODE;

/*!
 * \typedef IPOD_IMAGE_TYPE
 * \par DESCRIPTION
 * This enumeration is type of image
 */
typedef enum
{
    IPOD_IMAGE_MONO = 0x01,                 /*!< Monochrome, 2bits per pixel                                       */
    IPOD_IMAGE_RGB565_LE,                   /*!< RGB 565 color, little-endian, 16 bits per pixel */
    IPOD_IMAGE_RGB565_BE,                   /*!< RGB 565 color, big-endian, 16 bits per pixel    */
    IPOD_IMAGE_MONO_WIN = 0x101             /*!< Windows monochrome */
} IPOD_IMAGE_TYPE;

/*!
 * \typedef IPOD_AUDIOBOOK_SPEED
 * \par DESCRIPTION
 * This enumeration is mode of audiobook speed
 */
typedef enum
{
    IPOD_AUDIOBOOK_SPEED_NORMAL = 0x00,    /*!< Audiobook speed is normal */
    IPOD_AUDIOBOOK_SPEED_FAST = 0x01,      /*!< Audiobook speed is fast */
    IPOD_AUDIOBOOK_SPEED_SLOW = 0xFF       /*!< Audiobook speed is slow */
} IPOD_AUDIOBOOK_SPEED;

/*!
 * \typedef IPOD_STATE_CHANGE
 * \par DESCRIPTION
 * This enumeration is change of iPod's power
 */
typedef enum
{
    IPOD_STATE_NO_STATE_CHANGE = 0x00,      /*!< No state change.                                                     */
    IPOD_STATE_GOING_TO_DEEP_SLEEP,         /*!< Accessory power going to Deep Sleep state (no power).                */
    IPOD_STATE_GOING_TO_HIBERNATE,          /*!< Accessory power going to Hibernate state (no power).                 */
    IPOD_STATE_GOING_TO_LIGHT_SLEEP,        /*!< Accessory power going to Light Sleep state (less than 5 mA current). */
    IPOD_STATE_GOING_TO_POWER_ON            /*!< Accessory power going to the Power On state.                         */

} IPOD_STATE_CHANGE;

/*!
 * \typedef IPOD_SAMPLE_RATES
 * \par DESCRIPTION
 * This enumeration is value of sample rates
 */
typedef enum
{
    IPOD_SAMPLE_RATE_8000  = 0x00001F40,    /*!< Sample rate is 8kHz */
    IPOD_SAMPLE_RATE_11025 = 0x00002B11,    /*!< Sample rate is 11.025kHz */
    IPOD_SAMPLE_RATE_12000 = 0x00002EE0,    /*!< Sample rate is 12kHz */
    IPOD_SAMPLE_RATE_16000 = 0x00003E80,    /*!< Sample rate is 16kHz */
    IPOD_SAMPLE_RATE_22050 = 0x00005622,    /*!< Sample rate is 22.05kHz */
    IPOD_SAMPLE_RATE_24000 = 0x00005DC0,    /*!< Sample rate is 24kHz */
    IPOD_SAMPLE_RATE_32000 = 0x00007D00,    /*!< Sample rate is 32kHz */
    IPOD_SAMPLE_RATE_44100 = 0x0000AC44,    /*!< Sample rate is 44.1kHz */
    IPOD_SAMPLE_RATE_48000 = 0x0000BB80     /*!< Sample rate is 48kHz */

} IPOD_SAMPLE_RATES;

/*!
 * \typedef IPOD_ACCESSORY_INFO_TYPES
 * \par DESCRIPTION
 * This enumeration is type of accessory information
 */
typedef enum
{
    IPOD_ACC_INFO_CAPABILITIES = 0x00,     /*!< Accessory information capabilities*/
    IPOD_ACC_NAME,                         /*!< Accessory name */
    IPOD_ACC_MIN_SUPP_IPOD_FIRMWARE_VER,   /*!< Accessory minimum supported iPod firmware version */
    IPOD_ACC_MIN_SUPP_LINGO_VER,           /*!< Accessory minimum supported lingo version */
    IPOD_ACC_FIRMWARE_VERSION,             /*!< Accessory firmware version */
    IPOD_ACC_HARDWARE_VERSION,             /*!< Accessory hardware version */
    IPOD_ACC_MANUFACTURER,                 /*!< Accessory manufacture */
    IPOD_ACC_MODEL_NUMBER,                 /*!< Accessory model number */
    IPOD_ACC_SERIAL_NUMBER,                /*!< Accessory serial number */
    IPOD_ACC_INCOMING_MAX_PAYLOAD_SIZE,    /*!< Accessory incoming maximum payload size */
    IPOD_ACC_STATUS_TYPES_SUPPORTED = 0x0B /*!< Accessory status types */

} IPOD_ACCESSORY_INFO_TYPES;

/*!
 * \typedef IPOD_ACC_ACK_STATUS
 * \par DESCRIPTION
 * This enumeration is type of status of Accessory ack
 */

typedef enum
{
    IPOD_ACC_ACK_STATUS_SUCCESS = 0x00,        /*!< Command success */
    IPOD_ACC_ACK_STATUS_RESERVED1,             /*!< Command reserved1 */
    IPOD_ACC_ACK_STATUS_CMD_FAILED_ERROR,      /*!< Command failed */
    IPOD_ACC_ACK_STATUS_RESOURCES_ERROR,       /*!< Out of resourcces */
    IPOD_ACC_ACK_STATUS_BAD_PARAM_ERROR,       /*!< Parameter is bad */
    IPOD_ACC_ACK_STATUS_UNKNOWN_ID_ERROR,      /*!< Id is unknown */
    IPOD_ACC_ACK_STATUS_RESERVED2,             /*!< Command reserved2 */
    IPOD_ACC_ACK_STATUS_ACCESSORY_AUTH_ERROR   /*!< Accessory is not authenticated */

} IPOD_ACC_ACK_STATUS;

/*!
 * \typedef IPOD_ITUNES_METADATA_TYPE
 * \par DESCRIPTION
 * This enumeration is type of itunes metadata
 */
typedef enum
{
    IPOD_ITUNES_UID = 0x00,             /*!< Database unique ID */
    IPOD_ITUNES_LAST_SYNC,              /*!< Last sync data */
    IPOD_ITUNES_AUDIO_TRACK_COUNT,      /*!< Total audio track count */
    IPOD_ITUNES_VIDEO_TRACK_COUNT,      /*!< Total video track count */
    IPOD_ITUNES_AUDIOBOOK_COUNT,        /*!< Total audiobook count */
    IPOD_TOTAL_PHOTO_COUNT              /*!< Total photo count */
} IPOD_ITUNES_METADATA_TYPE;

/*!
 * \typedef IPOD_FEATURE_TYPE
 * \par DESCRIPTION
 * This enumeration is type of iPod's feature
 */
typedef enum
{
    IPOD_RADIO_TAGGING = 0x01,    /*!< radio tagging*/
    IPOD_GYM_EQUIPMENT_WORKOUT    /*!< Gym equipement workout */
} IPOD_FEATURE_TYPE;

/*!
 * \typedef IPOD_PREFERENCE_CLASS_ID
 * \par DESCRIPTION
 * This enumeration is preference class ID
 */
typedef enum
{
    IPOD_VOUT_SETTING = 0x00,  /*!< Video out setting */
    IPOD_VSCREEN_CFG,          /*!< Screen configuration */
    IPOD_VSIG_FORMAT,          /*!< Video signal format */
    IPOD_VLINE_OUT_USAGE,      /*!< Line-out usage; This is available only if the iPod supports line-out usage. */
    IPOD_VOUT_CONNECT = 0x08,  /*!< Video-out connection */
    IPOD_VCLOSED_CAP,          /*!< Closed captioning */
    IPOD_VASP_RATIO,           /*!< Video monitor aspect radio */
    IPOD_VSUBTITLES = 0x0C,    /*!< Subtitles */
    IPOD_VALT_AUD_CHANNEL      /*!< Video alternate audio channel */
}  IPOD_PREFERENCE_CLASS_ID;

/*!
 * \typedef IPOD_VOUT_SETTING_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for Video out setting.
 */
typedef enum
{
    IPOD_VOUT_SETTING_OFF = 0x00,    /*!< Disables iPod video output of any kind */
    IPOD_VOUT_SETTING_ON,            /*!< Enables iPod video output based on the other video preferences */
    IPOD_VOUT_SETTING_ASK_USER       /*!< If the iPod is simple mode, User select the video output ON/OFF when video playback is started.<br>
                                      *   If the iPod is extended mode, video output mode is automatically enabled */
} IPOD_VOUT_SETTING_SETTING_ID;      

/*!
 * \typedef IPOD_VSCREEN_CFG_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for Screen configuration.
 */
typedef enum
{
    IPOD_VSCREEN_CFG_FILL = 0x00,    /*!< Expand the video image to fill the entire screenn without letterbox or pillarbox balck bars */
    IPOD_VSCREEN_CFG_FIT             /*!< Expand the video image to screen edge */
} IPOD_VSCREEN_CFG_SETTING_ID;

/*!
 * \typedef IPOD_VSIG_FORMAT_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for video signal format.
 */
typedef enum
{
    IPOD_VSIG_FORMAT_NTSC = 0x00,    /*!< NTSC video format and timing */
    IPOD_VSIG_FORMAT_PAL             /*!< PAL video format and timing */
} IPOD_VSIG_FORMAT_SETTING_ID;

/*!
 * \typedef IPOD_VLINE_OUT_USAGE_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for line-out usage.
 */
typedef enum
{
    IPOD_VLINE_OUT_USAGE_NOT_USED = 0x00,    /*!< Line-out disabled*/
    IPOD_VLINE_OUT_USAGE_USED                /*!< Line-out enabled*/
} IPOD_VLINE_OUT_USAGE_SETTING_ID;

/*!
 * \typedef IPOD_VOUT_CONNECT_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for Video-out connection.
 */
typedef enum
{
    IPOD_VOUT_CONNECT_NONE = 0x00,           /*!< No connection */
    IPOD_VOUT_CONNECT_COMPOSIT,              /*!< Composite video connection(interlaced video only) */ 
    IPOD_VOUT_CONNECT_S_VIDEO,               /*!< S-video video connection(interlaced video only) */
    IPOD_VOUT_CONNECT_COMPONENT              /*!< Component Y/Pr/Pb video connection(interlaced or progressive scan, depending on the iPod model) */
} IPOD_VOUT_CONNECT_SETTING_ID;

/*!
 * \typedef IPOD_VCLOSED_CAP_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for Closed captioning.<br>
 * Closed captions and subtitles cannot be enabled at the same time.
 */
typedef enum
{
    IPOD_VCLOSED_CAP_OFF = 0x00,             /*!< Closed caption signalling is disabled */
    IPOD_VCLOSED_CAP_ON                      /*!< The iPod overlays closed caption text on the video content before outputting the video signal */
} IPOD_VCLOSED_CAP_SETTING_ID;

/*!
 * \typedef IPOD_VASP_RATIO_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for Video monitor aspect ratio.
 */
typedef enum
{
    IPOD_VASP_RATIO_FULL = 0x00,              /*!< Aspect ratio is 4 : 3 */
    IPOD_VASP_RATIO_WIDE                      /*!< Aspect ratio is 16 : 9 */
} IPOD_VASP_RATIO_SETTING_ID;

/*!
 * \typedef IPOD_VSUBTITLES_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for Subtitles<br>
 * Closed captions and subtitles cannot be enabled at the same time.
 */
typedef enum
{
    IPOD_VSUBTITLES_OFF = 0x00,               /*!< Subtitles overlays are disabled */
    IPOD_VSUBTITLES_ON                        /*!< The iPod overlays subtitle text on the video content before outputting the video signal*/
} IPOD_VSUBTITLES_SETTING_ID;

/*!
 * \typedef IPOD_VALT_AUD_CHANNEL_SETTING_ID
 * \par DESCRIPTION
 * This enumeration is setting ID for Video alternate audio channel.
 */
typedef enum
{
    IPOD_VALT_AUD_CHANNEL_OFF = 0x00,         /*!< The alternate audio channel is disabled */
    IPOD_VALT_AUD_CHANNEL_ON                  /*!< The alternate audio channel is enabled */
} IPOD_VALT_AUD_CHANNEL_SETTING_ID;

/*!
 * \enum IPOD_TRACK_INFORMATION_TYPE
 * This is a enumeration concerning data of ipod track
 * \note According to iap_specification, #SAMPLE_RATE is bit per second.
 * But, actually is kilo bit per second. <br>
 * According to iap_specification, if the track has never been played, #LAST_PLAYED_DATA is all zeroes.
 * but currently iPod nano3G is 2040/2/6/15:28:16. This is bug of iPod.<br>
 * According to iap_specificaiton, 2 bytes of #CHAPTER_TIMES is chapter index.
 * but currently iPod nano3G is totalCount. This is bug of iPod.
 */
typedef enum
{
    CAPABILITIES = 0x00,      /*!< Capabilities (media kind, skip when shuffle, has artwork,etc)) */
    TRACK_NAME,               /*!< Track name */
    ARTIST_NAME,              /*!< Artist name */
    ALBUM_NAME,               /*!< Album name */
    GENRE_NAME,               /*!< Genre name */
    COMPOSER_NAME,            /*!< Composer name*/
    DURATION,                 /*!< Total track time duration */
    UID,                      /*!< Unique track identifier */
    CHAPTER_COUNT,            /*!< Chapter count */
    CHAPTER_TIMES,            /*!< Chapter times */
    CHAPTER_NAMES,            /*!< Chapter names */
    LYRIC_STRING,             /*!< Lyrics of the song currently playing in the Playback Engine*/
    DESCRIPTION,              /*!< Description */
    ALBUM_TRACK_INDEX,        /*!< Number of album index */
    DISC_SET_ALBUM_INDEX,     /*!< Number of disc set album */
    PLAY_COUNT,               /*!< Count of track play(0 = track not played) */
    SKIP_COUNT,               /*!< Count of track skip(0 = track not skipped) */
    PODCAST_DATA,             /*!< Date/time */
    LAST_PLAYED_DATA,         /*!< Date/time. If data is all zeros, the track has never been played */
    YEAR,                     /*!< Year is which track was released */
    STAR_RATING,              /*!< Star rating of track */
    SERIES_NAME,              /*!< Series name */ 
    SEASON_NUMBER,            /*!< Season number */
    TRACK_VOLUME_ADJUST,      /*!< Track volume attenuation/amplification adjustment */
    EQ_PRESET,                /*!< Track equalizer preset index */
    SAMPLE_RATE,              /*!< Track data sample rate*/
    BOOKMARK_OFFSET,          /*!< Bookmark offset from start of track in milliseconds */
    START_AND_STOP_OFFSET     /*!< Start time */
}  IPOD_TRACK_INFORMATION_TYPE;


/*!
 * \union IPOD_TRACK_INFORMATION_BITFIELD
 * This is a union concerning data of ipod track
 * \note According to iap_specification, #SAMPLE_RATE is bit per second.
 *       But, actually is kilo bit per second. <br>
 * \note According to iap_specification, if the track has never been played, #LAST_PLAYED_DATA is all zeroes.
 *       But currently iPod nano3G is 2040/2/6/15:28:16. This is bug of iPod.<br>
 * \note According to iap_specificaiton, 2 bytes of #CHAPTER_TIMES is chapter index.
 *       But currently iPod nano3G is totalCount. This is bug of iPod.
 */
typedef union
{
    U32 bitmask;
    struct _IPOD_TRACK_INFORMATION_BITFIELD
    {
        U32 CAPABILITIES            : 1;    /*!< b0  - Capabilities (media kind, skip when shuffle, has artwork,etc)) */
        U32 TRACK_NAME              : 1;    /*!< b1  - Track name */
        U32 ARTIST_NAME             : 1;    /*!< b2  - Artist name */
        U32 ALBUM_NAME              : 1;    /*!< b3  - Album name */
        U32 GENRE_NAME              : 1;    /*!< b4  - Genre name */
        U32 COMPOSER_NAME           : 1;    /*!< b5  - Composer name*/
        U32 DURATION                : 1;    /*!< b6  - Total track time duration */
        U32 UID                     : 1;    /*!< b7  - Uniqui track identifier */
        U32 CHAPTER_COUNT           : 1;    /*!< b8  - Chapter count */
        U32 CHAPTER_TIMES           : 1;    /*!< b9  - Chapter times */
        U32 CHAPTER_NAMES           : 1;    /*!< b10 - Chapter names */
        U32 LYRIC_STRING            : 1;    /*!< b11 - Lyrics of the song currently playing in the Playback Engine*/
        U32 DESCRIPTION             : 1;    /*!< b12 - Description */
        U32 ALBUM_TRACK_INDEX       : 1;    /*!< b13 - Number of albume index */
        U32 DISC_SET_ALBUM_INDEX    : 1;    /*!< b14 - Number of disc set album */
        U32 PLAY_COUNT              : 1;    /*!< b15 - Count of track play(0 = track not played) */
        U32 SKIP_COUNT              : 1;    /*!< b16 - Count of track skip(0 = track not skipped) */
        U32 PODCAST_DATA            : 1;    /*!< b17 - Date/time */
        U32 LAST_PLAYED_DATA        : 1;    /*!< b18 - Date/time. If data is all zeros, the track has never been played */
        U32 YEAR                    : 1;    /*!< b19 - Year is which track was released */
        U32 STAR_RATING             : 1;    /*!< b20 - Star rating of track */
        U32 SERIES_NAME             : 1;    /*!< b21 - Series name */
        U32 SEASON_NUMBER           : 1;    /*!< b22 - Season number */
        U32 TRACK_VOLUME_ADJUST     : 1;    /*!< b23 - Track volume attenuation/amplification adjestment */
        U32 EQ_PRESET               : 1;    /*!< b24 - Track equalizer preset index */
        U32 SAMPLE_RATE             : 1;    /*!< b25 - Track data sample rate*/
        U32 BOOKMARK_OFFSET         : 1;    /*!< b26 - Bookmark offset from start of track in milliseconds */
        U32 START_AND_STOP_OFFSET   : 1;    /*!< b27 - Start time */
        U32 RESERVED_PART_ONE       : 4;    /*!<b28-b31 - reserved for feature usage */
    } track_info;
} IPOD_TRACK_INFORMATION_BITFIELD;

/*!
 * \enum IPOD_PLAY_CONTROL
 * This is a enumeration concerning type of play control.
 */
typedef enum
{
    TOGGLE = 0x01,    /*!< Switch status of Play and Pause */
    STOP,             /*!< Change into stop status */
    NEXT_TRACK,       /*!< Change into next track */
    PREV_TRACK,       /*!< Change into previous track*/
    START_FF,         /*!< Change into Fastforward */
    START_REW,        /*!< Change into FastBackwaro */
    END_FF_REW,       /*!< Change into end of FF/REW */
    NEXT,             /*!< Change into Next;If current track is not audiobook or podcast with chapters, act like Next track */
    PREVIOUS,         /*!< Change into Previous; If current track is not audiobook or podcast with chapters, act like Prev track */
    PLAY,             /*!< Change into play status */
    PAUSE,            /*!< Change into pause status */
    NEXT_CHAPTER,     /*!< Change into next chapter;If current track is not audiobook or podcast with chapters, have no effect */
    PREV_CHAPTER      /*!< Change into previous chapter; If current track is not audiobook or podcast with chapters, have no effect */
} IPOD_PLAY_CONTROL;

/*!
 * \enum IPOD_FID_ACC_INFO_TYPE
 * This is a enumeration concerning type of FID accessory informaitpon.<br>
 */
typedef enum
{
    FID_ACC_RESERVE = 0x00, /*!< This is reserved by iPod */
    FID_ACC_NAME,           /*!< Type is accessory name. This data sets devconf */
    FID_ACC_FW_VER = 0x04,  /*!< Type is accessory firmware version. This data can set in devconf */
    FID_ACC_HW_VER,         /*!< Type is accessory hardware version. This data can set in devconf */
    FID_ACC_MAN,            /*!< Type is accessory manufacuture. This data can set in devconf */
    FID_ACC_MODEL,          /*!< Type is accessory model. This data can set in devconf */
    FID_ACC_SERIAL,         /*!< Type is accessory serial. This data can set in devconf */
    FID_ACC_INCOMING,       /*!< Type is accessory incoming. This data can set in devconf */
    FID_ACC_STATUS = 0x0B,
    FID_ACC_RF,
    FID_ACC_MAX             /*!< Type is max number of accessory info. This is not used */
} IPOD_FID_ACC_INFO_TYPE;

/*!
 * \enum IPOD_TRACK_TYPE
 * This is a enumeration concerning type of play control.
 */
typedef enum
{
    TYPE_UID = 0x01,
    TYPE_DB = 0x03,
    TYPE_PB = 0x05,
    UNKNOWN_TYPE = 0xFF     /* Create for QAC */
} IPOD_TRACK_TYPE;

/*!
 * \enum IPOD_LOCATION_TYPE
 * This is a enumeration concerning location types.
 */
typedef enum
{
    LOCATION_TYPE_SYS_CAPS = 0x00,  /*!< This is type of system capabilities */
    LOCATION_TYPE_GPS_CAPS,         /*!< This is type of NMEA GPS location capabilities */
    LOCATION_TYPE_ASSIST_CAPS,      /*!< This is type of Location assistance capabilities */
    LOCATION_TYPE_MAX               /*!< This is max number of Location types. This is not used */
} IPOD_LOCATION_TYPE;

/*!
 * \enum IPOD_LOCATION_CMD
 * This is a enumeration concerning commands of location lingo.
 */
typedef enum
{
    IPOD_GET_DEV_CTRL_CMD = 0x00,   /*!< This is command of GetDevCtrl */
    IPOD_SET_DEV_CTRL_CMD = 0x01,   /*!< This is command of SetDevCtrl */
    IPOD_GET_DEV_DATA_CMD = 0x02,   /*!< This is command of GetDevData */
    IPOD_SET_DEV_DATA_CMD = 0x03,   /*!< This is command of SetDevData */
    IPOD_IPOD_ACK_CMD = 0x04,
    IPOD_LOCATION_CMD_MAX = 0x05    /*!< This is max number of Location command */
} IPOD_LOCATION_CMD;

typedef enum
{
    IPOD_EVT_FLOW_CTRL = 0x02,        /*!< This is notification type of flow control, see status.waitMs for requested wait time */
    IPOD_EVT_RADIO_TAG_STATUS = 0x03, /*!< This is notification type of radio tag status, see status.notifyStatus for status information */
    IPOD_EVT_CAMERA_STATUS = 0x04,    /*!< This is notification type of camera status, see status.notifyStatus for status information*/
    IPOD_EVT_CHARGING = 0x05,         /*!< This is a notification about the Apple devices current-sink limit setting in mA, see status.availableCurrent */
    IPOD_EVT_DB_CHANGED = 0x09,       /*!< This ia a notification that the DB content on the Apple device has changed. Track index became invalid and must be refreshed */
    IPOD_EVT_NOW_FOCUS_APP = 0x0A,    /*!< This is notification of now playing focus application, see status.focusApp */
    IPOD_EVT_SESSION = 0x0B,          /*!< This is notification of session ID, see status.sessionId */
    IPOD_EVT_CMD_COMPLETE = 0x0D,     /*!< This is a notification that the specified command was completed */
    IPOD_EVT_IPOD_OUT = 0x0F,         /*!< This is a notification of iPod Out mode, see status.notifyStatus for status information */
    IPOD_EVT_BT_STATUS = 0x11,        /*!< This is a notification of the status of BT connection, see status.BTStatus */
    IPOD_EVT_APP_DISPLAY_NAME = 0x13, /*!< This ia a notification of the display name of the currently active App, see status.focusApp */
    IPOD_EVT_ASSIST_TOUCH = 0x14      /*!< This is a notification of the status of the assistive touch feature, see status.notifyStatus for status information */
} IPOD_NOTIFY_TYPE;

typedef enum
{
    IPOD_SWITCH_RESET = 0x00,       /*!< This is used to devconf setting of USB_AUDIO */
    IPOD_SWITCH_LINE_OUT,           /*!< This is used to ANALOG setting */
    IPOD_SWITCH_DIGITAL             /*!< This is used to DIGITAL setting */
} IPOD_SWITCH_AUDIO_OUT;

/* New Feature 28/07/28 */
typedef enum
{
    IPOD_ACC_STATUS_BT_AUTO_PAIRING_CONNECTION_STATUS = 0x01,   /*!< This is used to Bluetooth Autopairing and Connection Status Notifications */
    IPOD_ACC_STATUS_FAULT = 0x02,   /*!< This is used to fault condition of accessory status */
    IPOD_ACC_UNKNOWN_STATUS = 0xFF
} IPOD_ACC_STATUS_TYPE;

typedef enum
{
    IPOD_REMOTE_ARTWORK_DATA = 0x00,
    IPOD_EXTEND_ARTWORK_DATA,
    IPOD_RETRIEVE_CATEGORIZED_DB_RECORDS
} IPOD_CANCEL_COMMAND_TYPE;

typedef enum
{
    IPOD_MOVE_POINT = 0x00,
    IPOD_MOVE_FIRST,
    IPOD_MOVE_LAST,
    IPOD_MOVE_NEXT,
    IPOD_MOVE_PREV,
    IPOD_SCROLL_LEFT,
    IPOD_SCROLL_RIGHT,
    IPOD_SCROLL_UP,
    IPOD_SCROLL_DOWN,
    IPOD_SCROLL_POINT,
    IPOD_SEND_TEXT,
    IPOD_ACC_EVENT_CUT,
    IPOD_ACC_EVENT_COPY,
    IPOD_ACC_EVENT_PASTE,
    IPOD_ACC_EVENT_HOME,
    IPOD_CREATE_TOUCH_EVENT
} IPOD_ACC_EVENT_TYPE;
/*\}*/

/**
 * \addtogroup Unions
 */
/*\{*/

/*!
 * \union IPOD_PREFERENCE_SETTING_ID
 * \par DESCRIPTION
 * This union is setting ID.
 */
typedef union
{
    IPOD_VOUT_SETTING_SETTING_ID      videoOutSetting;      /*!< refer \ref IPOD_VOUT_SETTING_SETTING_ID */
    IPOD_VSCREEN_CFG_SETTING_ID       screenCfg;            /*!< refer \ref IPOD_VSCREEN_CFG_SETTING_ID */
    IPOD_VSIG_FORMAT_SETTING_ID       signalFormat;         /*!< refer \ref IPOD_VSIG_FORMAT_SETTING_ID */
    IPOD_VLINE_OUT_USAGE_SETTING_ID   lineOut;              /*!< refer \ref IPOD_VLINE_OUT_USAGE_SETTING_ID */
    IPOD_VOUT_CONNECT_SETTING_ID      videoOutConnection;   /*!< refer \ref IPOD_VOUT_CONNECT_SETTING_ID */
    IPOD_VCLOSED_CAP_SETTING_ID       closedCaptioning;     /*!< refer \ref IPOD_VCLOSED_CAP_SETTING_ID */
    IPOD_VASP_RATIO_SETTING_ID        aspectRatio;          /*!< refer \ref IPOD_VASP_RATIO_SETTING_ID */
    IPOD_VSUBTITLES_SETTING_ID        subTitles;            /*!< refer \ref IPOD_VSUBTITLES_SETTING_ID */
    IPOD_VALT_AUD_CHANNEL_SETTING_ID  audioChannel;         /*!< refer \ref IPOD_VALT_AUD_CHANNEL_SETTING_ID */
}  IPOD_PREFERENCE_SETTING_ID;

/*!
 * \union IPOD_ITUNES_METADATA_INFO
 * \par DESCRIPTION
 * This is union of itunes metadata information.
 */
typedef union
{
    U64 uID;                        /*!< itunes unique identify */
    struct LAST_SYNC                /*! Last syncronize in itunes*/
    {
        U16 year;
        U8  month;
        U8  day;
        U8  hour;
        U8  min;
        U8  sec;
    } lastSync;
    U32 audioCount;                 /*!< Total audio count */
    U32 videoCount;                 /*!< Total video count */
    U32 audiobookCount;             /*!< Total audio book count*/
    U32 photoCount;                 /*!< Total photo count */
} IPOD_ITUNES_METADATA_INFO;


typedef struct _IPOD_CHAPTER_TIMES            /*! Struct of chapter times */
{
    U16 index;                  /*!< Chapter index (0 = first chapter) */
    U32 offset;                 /*!< Chpater times in milliseconds */
} IPOD_CHAPTER_TIMES;

typedef struct _IPOD_CHAPTER_NAMES            /*! Struct of chapter Names */
{
    U16 index;                  /*!< Chapter index (0 = first chapter) */
    U8 *name;                   /*!< Chpater name as null-terminated UTF8 string */
} IPOD_CHAPTER_NAMES;

/*!
 * \union IPOD_TRACK_INFORMATION_DATA
 * \par DESCRIPTION
 * This is union of track information.
 */
typedef union
{
    struct _CAPABILITIES           /*! Struct of capabilities bits */
    {
        U32 isAudiobook:1;
        U32 hasChapter:1;
        U32 hasArtwork:1;
        U32 hasLyric:1;
        U32 isPodcastEpi:1;
        U32 hasReleaseDate:1;
        U32 hasDescription:1;
        U32 isVideo:1;
        U32 isQueuedAsVideo:1;
        U32 reserved:22;
    } caps;

    U8 *trackName;                  /*!< Track name; Null-terminated UTF8 string */
    U8 *artistName;                 /*!< Artist name; Null-terminated UTF8 string */
    U8 *albumName;                  /*!< Album name; Null-terminated UTF8 string */
    U8 *genreName;                  /*!< Genre name; Null-terminated UTF8 string */
    U8 *composerName;               /*!< Composer name; Nullterminated UTF8 string */
    U32 duration;                   /*!< Total track duration in milliseconds */
    U64 trackUID;                   /*!< Uniquely idetifies an iPod track */
    U16 chapterCount;               /*!< Chapter count(0 = no chapters) */

    struct _CHAPTERTIMES            /*! Struct of chapter times */
    {
        U32 offset;                 /*! Time off set of chapter times */
        U16 chapterIndex;           /*! Chapter index of tarck */
        U32 totalCount;             /*! Total capter count of track */
    } chapterTimes;

    struct _CHAPTERNAMES            /*! Struct of chapter names */
    {
        U8 *name;                   /*! Chapter names; Null-terminated UTF8 string */
        U16 chapterIndex;           /*! Chapter index of track */
        U32 totalCount;             /*! Total chapter count of track */

    } chapterNames;

    struct _LYRICS                  /*! Struct of lyrics */
    {
        U16 section;                /*!< Track lyrics section index(0 = first sectio) */
        U16 maxSection;             /*!< Track lyrics maximum section index */ 
        U8 *lyric;                 /*!< lyrics */
    } lyrics;

    U8 *description;                /*!< Null-terminated UTF8 string */
    U16 albumTrackIndex;            /*!< Album index number */
    U16 discSetAlbumIndex;          /*!< Disc set album index number */
    U32 playCount;                  /*!< Track plya count (0 = track not played ) */
    U32 skipCount;                  /*!< Track skip count (0 = track not skipped) */

    struct _PODCAST_RELEASEDATE
    {
        U8 sec;
        U8 min;
        U8 hour;
        U8 day;
        U8 month;
        U16 year;
    } podcastReleaseDate;           /*!< Podcast release date; Date/time */

    struct _LASTPLAYEDDATE
    {
        U8 sec;
        U8 min;
        U8 hour;
        U8 day;
        U8 month;
        U16 year;
    } lastPlayedDate;             /*!< Last played date/time (allzeros if the track has never been played) */

    U16 yaer;                       /*!< Year in which track released */
    U8 starRating;                  /*!< Star rating of track;00 = No stars, 20 = 1 star, 40 = 2 stars, 60 = 3 stars, 80 = 4 stars, 100 = 5 stars */
    U8 *seriesName;                 /*!< Series name;Null terminated UTF8 string */
    U16 seasonNumber;               /*!< Season number (1 = first season)*/
    U8 Volume;                      /*!< Track volume attenuation/amplification adjustment; 0x9C = -100%, 0x00 = no adjust, 0x6 = +100% */
    U16 EQPreset;                   /*!< Track equalizer preset index */
    U32 dataRate;                   /*!< Track data sample rate(bits per second) */
    U32 bookmarkOffset;             /*!< Bookmark offset from start of track in milliseconds */
    
    struct _TIME_OFFSET             /*! Struct of start/Stop time offset */
    {
        U32 startOffset;            /*!< Start time offset */
        U32 stopOffset;             /*!< Stop time offset */
    } timeOffset;

} IPOD_TRACK_INFORMATION_DATA;

/*!
 * \union IPOD_NOTIFY_STATUS
 * \par DESCRIPTION
 * This is union of notification status type.
 */
typedef union
{
    U32 waitMs;                     /*!< Waiting time until next command */
    U8 notifyStatus;                /*!< Notificaiton status(e.g. radio taga status) */
    U8 *focusApp;                   /*!< Current focus application */
    U16 sessionId;                  /*!< Current using session ID */
    U16 availableCurrent;           /*!< Absolute current-sink limit setting in mA  */
    U8 commandComplete[4];          /*!< first byte: Lingo ID, bytes 2 and 3: Command ID, byte 4: Command status */
    U8 BTStatus[14];                /*!< first 6 bytes: BT MAC address, next 8 bytes: state of BT profiles */
} IPOD_NOTIFY_STATUS;


/*!
 * \enum IPOD_POWER_BATTERY_STATE
 * \par DESCRIPTION
 * This is enum and lists the possible values for the data associated with a Power/Battery event.
 */
typedef enum
{
    IPOD_INTERNAL_BATTERY_LOW_POWER = 0x00,
    IPOD_INTERNAL_BATTERY_POWER = 0x01,
    IPOD_EXTERNAL_POWER_BATTERY_PACK_NO_CHARGING = 0x02,
    IPOD_EXTERNAL_POWER_NO_CHARGING = 0x03,
    IPOD_EXTERNAL_POWER_BATTERY_CHARGING = 0x04,
    IPOD_EXTERNAL_POWER_BATTERY_CHARGED = 0x05
} IPOD_POWER_BATTERY_STATE;

/*!
 * \enum IPOD_PLAY_STATUS_VALUES
 * \par DESCRIPTION
 * This is enum and lists the possible values for the data associated with a Play Status event.
 */
typedef enum
{
    IPOD_PLAY_STOPPED = 0x00,
    IPOD_PLAYING = 0x01,
    IPOD_PLAY_PAUSED = 0x02,
    IPOD_PLAY_FW = 0x03,
    IPOD_PLAY_RW = 0x04,
    IPOD_END_FW_RW = 0x05
} IPOD_PLAY_STATUS_VALUES;

/*!
 * \union IPOD_REMOTE_EVENT_NOTIFY_STATUS
 * \par DESCRIPTION
 * This is union of remote event notification status type.
 */
typedef union
{
    U32 trackPosMS;
    U32 trackIndex;

    struct _CHAPTER_INFO
    {
        U32 trackIndex;
        U16 chapterCount;    /*! 0x0000 indicates that the track does not have chapters */
        U16 chapterIndex;     /*! 0xFFFF indicates that the track does not have chapters */
    } chapterInfo;

    IPOD_PLAY_STATUS_VALUES playStatus;

    struct _MUTE_UI_VOL    /*! Struct of mute settings, UI volume */
    {
        U8 muteState;           /*! 0 indicates muting is of; 1 indicates muting is on */
        U8 uiVol;               /*! 0 min volume; 255 max volume */
    } muteUiVol;

    struct _POWER_BATTERY
    {
        IPOD_POWER_BATTERY_STATE powerState;    /*! possible values for the data associated with a Power/Battery event */
        U8 batteryLevel;                        /*! 0 means fully discharged battery; 255 means battery is fully charged */
    } powerBattery;

    U32 eqState;                /*! Equalizer setting index */
    IPOD_SHUFFLE_MODE shuffle;
    IPOD_REPEAT_MODE repeat;

    struct _DATE_TIME
    {
        U16 year;
        U8 month;
        U8 day;
        U8 hour;
        U8 min;
    } currDateTime;

    U8 backlight;           /*! 0 min brightness; 255 max brightness */
    U8 holdSwitch;          /*! 0 means hold switch is off; 1 means hold switch is on */
    U8 soundCheck;          /*! 0 means sound check is off; 1 means sound check is on */
    IPOD_AUDIOBOOK_SPEED audiobook;
    U16 trackPosSec;

    struct _MUTE_UI_ABSOLUTE_VOL    /*! Struct of mute settings, UI volume, absolute volume bits */
    {
        U8 muteState;        /*! 0 indicates muting is of; 1 indicates muting is on */
        U8 uiVol;            /*! 0 min volume; 255 max volume */
        U8 absVol;           /*! 0 min absolute volume; 255 max absolute volume */
    } muteUiAbsoluteVol;

    struct _TRACK_CAPABILITIES            /*! Struct of capabilities bits */
    {
        U32 isAudiobook:1;
        U32 hasChapter:1;
        U32 hasArtwork:1;
        U32 hasLyric:1;
        U32 isPodcastEpi:1;
        U32 hasReleaseDate:1;
        U32 hasDescription:1;
        U32 isVideo:1;
        U32 isQueuedAsVideo:1;
        U32 reserved:22;
    } trackCaps;

    U32 playEngineContent;          /*!< number of tracks in new playlist */
} IPOD_REMOTE_EVENT_NOTIFY_STATUS;

typedef struct _BTAUTOPAIRING
{
    U32 devtype;            /*!< 32-bit Device Type,  Bit 00: Bluetooth device type classic, Bits 01 - 31: Reserved */
    U32 devstate;           /*!< 32-bit Device State, Bit 00: Bluetooth device type classic on/off bit: 0=off, 1=on */
    U32 devclass;           /*!< A parameter indicating the type of device and the types of service that are supported */
    U8  pairingtype;        /*!< 8-bit Pairing Type, Bit 00: None, Bit 01: PIN code, Bit 02: Secure Simple Pairing (SSP), Bits 03-07: Reserved */
    U8  pincode[16];        /*!< A null terminated UTF-8 string */
    U8  macaddress[6];      /*!< 6 byte Media Access Control address */
}btautopairing_t;

typedef struct
{
    struct _FAULT
    {
        U8 type;
        U8 condition;
    } fault;
    btautopairing_t *btautopairing;
    U32 btautopairing_count;
} IPOD_ACC_STATUS_PARAM;

/*\}*/
/* ========================================================================== */
/* structs                                                                    */
/* ========================================================================== */

/**
 * \addtogroup Structs
 */
/*\{*/

/*!
 * \struct IPOD_TRACK_CAP_INFO_DATA
 * This is a structure concerning track information
 */
typedef struct
{
    U32  capability;                        /*!< Capability is bit set. refer \ref IPOD_TRACK_CAPABILITY */
    U32 trackLength;                        /*!< Totak track length, in milliseconds */
    U16 chapterCount;                       /*!< Chapter count */

} IPOD_TRACK_CAP_INFO_DATA;

/*!
 * \struct IPOD_TRACK_RELEASE_DATE_DATA
 * This is a structure concerning release date
 */
typedef struct
{
    U8 seconds;        /*!< Release seconds(0 - 59) */
    U8 minutes;        /*!< Release minutes(0 - 59) */
    U8 hours;          /*!< Release hours(0 - 23) */
    U8 dayOfMonth;     /*!< Release day of month(1 - 31) */
    U8 month;          /*!< Release month(1 - 12) */
    U16 year;          /*!< Release year */
    U8 weekday;        /*!< 0 = Sunday, and 6 = Saturday */

} IPOD_TRACK_RELEASE_DATE_DATA;

/*!
 * \struct IPOD_TRACK_ARTWORK_COUNT_DATA
 * This is a structure concerning artwork count
 */
typedef struct
{
    U8 isLast;      /*!< TRUE is last artwork count */
    U16 id;           /*!< Format ID */
    U16 count;        /*!< Count of images in that format for this track */

} IPOD_TRACK_ARTWORK_COUNT_DATA;

/*!
 * \struct IPOD_ALBUM_ARTWORK
 * This is a structure concerning album artwork
 */
typedef struct
{
    U16 telegramIndex;             /*!< Index number of artwork data */
    U8 displayPixelFormatCode;     /*!< Format code of artwork */
    U16 imageWidth;                /*!< Width of artwork */
    U16 imageHeight;               /*!< Height of artwork */
    U16 topLeftPointX;             /*!< X of topLeftPoint of artwork */
    U16 topLeftPointY;             /*!< Y of topLeftPoint of artwork */
    U16 bottomRightX;              /*!< X of bottom right of artwork */
    U16 bottomRightY;              /*!< Y of bottom rigth of artwork */
    U32 rowSize;                   /*!< Row size of artwork data */
    U32 pixelDataLength;           /*!< length of artwork data */
    U8* pixelData;                 /*!< data of artwork */

} IPOD_ALBUM_ARTWORK;

/*!
 * \struct IPOD_ARTWORK_FORMAT
 * This is a structure concerning artwork format
 */
typedef struct
{
    U16 formatID;        /*!< Aartwork format id */
    U8  pixelFormat;     /*!< Monochrome or rgb565 color and so on */
    U16 imageWidth;      /*!< Artwork image width */
    U16 imageHeight;     /*!< Artwork image height */

} IPOD_ARTWORK_FORMAT;

/*!
 * \struct IPOD_DISPLAY_IMAGE_LIMITS
 * This is a structure concerning image limit
 */
typedef struct
{
    U16 width;         /*!< Limit of image width */
    U16 height;        /*!< Limit of image height */
    U8 pixelFormat;    /*!< Supported pixel format */ 

} IPOD_DISPLAY_IMAGE_LIMITS;

/*!
 * \struct IPOD_STORAGE_CAPS
 * This is a structure concerning storage capabilities
 */
typedef struct
{
    U64 totalSpace;     /*!< Amount of storage on the iPod in bytes*/
    U32 maxFileSize;    /*!< Largest possible size in the bytes */
    U16 maxWriteSize;   /*!< Largest amount of data in bytes */
    U8 majorVersion;    /*!< Version number of the storage lingo protocol */
    U8 minorVersion;    /*!< Version number of the storage lingo protocol */
} IPOD_STORAGE_CAPS;

/*!
 * \struct IPOD_FILE_OPTIONS_MASK
 * This is a structure of bitmask of file option.
 */
typedef struct
{
    U32 binary:1;            /*!< If set the 1, append the file Data binary file data bytes to the file */
    U32 iPodInfoXML:1;       /*!< If set the 1, append the XML<ipodInfo> element to the file */
    U32 reserved1:1;         /*!< reserved (must be 0) */
    U32 signatureXML:1;      /*!< If set the 1, insert an XML<Signature> element to the file */
    U32 reserved2:28;        /*!< reserved (must be 0) */
} IPOD_FILE_OPTIONS_MASK;    /*!<*/


/*!
 * \struct IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION;
 * This is a structure of bit field of status chage notification
 */
typedef struct
{
    U32 basicPlay:1;         /*!< Basic play state changes*/
    U32 extendedPlay:1;      /*!< Extended play state changes*/
    U32 trackIndex:1;        /*!< Track index */
    U32 trackTimeMs:1;       /*!< Track time offset (ms) */
    U32 trackTimeSec:1;      /*!< Track time offset (sec) */
    U32 chapterIndex:1;      /*!< Chapter index */
    U32 chapterTimeMs:1;     /*!< Chapter time offset (ms) */
    U32 chapterTimeSec:1;    /*!< Chapter time offset (sec) */
    U32 trackUID:1;          /*!< Track unique identifier */
    U32 trackMediaType:1;    /*!< Track media type (audio/video) */
    U32 trackLyrics:1;       /*!< Track lyrics ready (if the track hs lyrics) */
    U32 capChanges:1;        /*!< Track capabilities changed */
    U32 pbChange:1;          /*!< Playback engine contens changed */
    U32 reserved:19;         /*!< reserved the 11-31 bits */
} IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION;

/*!
 * \struct IPOD_TRACK_INFORMATION
 * This is a structure of Track information
 */
typedef struct
{
    IPOD_TRACK_INFORMATION_DATA infoData;  /*!< union of track data;see #IPOD_TRACK_INFORMATION_DATA */
    U8 index;                              /*!< database track index */
} IPOD_TRACK_INFORMATION;
/*!
 * \struct IPOD_OPTIONS_BIT
 * This is a structure of bit field of ipod option
 */
typedef struct
{
    U32 videoOut:1;        /*!< iPod supports video output */
    U32 lineOut:1;         /*!< iPod supports using SetiPodPreferences to control line-out usage */
    U32 reserved_0:30;       /*!< reserved 62 bit */
    U32 reserved_1:32;
} IPOD_OPTIONS_BIT;

/*!
 * \struct VERSION
 * This is a structure of major and minor version for each lingo.
 */

typedef struct
{
    U8 major_ver;
    U8 minor_ver;
} VERSION;


/*!
 * \struct IPOD_ARTWORK_DATA
 * This is a structure of artwork data.
 */
typedef struct
{
    U32 imageDataCounter;
    U32 totalImageData;
    U8 *imageDataBuffer;
    IPOD_ALBUM_ARTWORK collectedArtworkData;
} IPOD_ARTWORK_DATA;


/*!
 * \struct IPOD_VIDEO_SCREEN_INFO
 * This is a structure of artwork data.
 */

typedef struct
{
    U16 totalWidthInches;
    U16 totalHeightInches;
    U16 totalWidthPixels;
    U16 totalHeightPixels;
    U16 widthPixels;
    U16 heightPixels;
    U8 featuresMask;
    U8 gammaValue;
} IPOD_VIDEO_SCREEN_INFO;

typedef struct
{
    U32 durationMs;
    U8 source;
    U8 controllerType;
    U8 direction;
    U8 action;
    U8 type;
    U16 moved;
    U16 total;
} IPOD_ROTATION_INFO;

typedef struct
{
    U16 x;
    U16 y;
    U8 type;
    U8 *text;
    U16 textLen;
} IPOD_ACC_EVENT_DATA;

typedef struct _IPOD_IOS_APP
{
    U8 *protocol;
    U8 *bundle;
    U8 metaData;
} IPOD_IOS_APP;

/*!
 * \struct VERSION_NUMBER
 */
typedef struct
{
    U8 Major_Number;
    U8 Minor_Number;
    U8 Revision_Number;
}VERSION_NUMBER;

/*!
 * \struct IPOD_ACC_INFO_CONFIGURATION
 * This structure contains Accessory information configuration details
*/
typedef struct
{
    U8 *Name;
    VERSION_NUMBER Hardware_version;
    VERSION_NUMBER Software_version;
    U8 *Manufacturer;
    U8 *ModelNumber;
    U8 *SerialNumber;
}IPOD_ACC_INFO_CONFIGURATION;

/*\}*/

/* ========================================================================== */
/* callback functions                                                         */
/* ========================================================================== */


/**
 * \addtogroup Callback
 */
/*\{*/

/*!
 * \typedef void (*IPOD_CB_USB_ATTACH) (const S32 success, const U32 iPodID)
 * \par INPUT PARAMETERS
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * S32 success -
 * \li \c \b #IPOD_OK iPod was detected and initialized successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_AUTHENTICATION authentication of iPod failed
 * \li \c \b #IPOD_ERR_APPLE_CP communication with Apple Authentication CP failed
 * \li \c \b #IPOD_ERR_UNSUP_DEV unsupported device attached
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called when
 * the iPod USB attach event is detected and first part of authentication procedure was done.
 * After this callback occurs, the iPod is ready to receive commands.
 */

typedef void (*IPOD_CB_USB_ATTACH) (const S32 success, IPOD_CONNECTION_TYPE connection, const U32 iPodID);

/*!
 * \typedef void (*IPOD_CB_USB_DETACH) (U32 iPodID)
 * \par INPUT PARAMETERS
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called when
 * the iPod USB detach event is detected.
 */
typedef void (*IPOD_CB_USB_DETACH) (U32 iPodID);


/*!
 * \typedef void (*IPOD_CB_PLAYING_TRACK_INFO) (IPOD_TRACK_INFO_TYPE infoType, const IPOD_TRACK_CAP_INFO_DATA* capInfoData,
                                            const IPOD_TRACK_RELEASE_DATE_DATA* releaseData,
                                            const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData,
                                            U8* stringBuf,
                                            const U32 iPodID);
 * \par INPUT PARAMETERS
 * #IPOD_TRACK_INFO_TYPE infoType - the type of information <br>
 * #IPOD_TRACK_CAP_INFO_DATA capInfoData* - pointer to a IPOD_TRACK_CAP_INFO_DATA structure <br>
 * #IPOD_TRACK_RELEASE_DATE_DATA releaseData* - pointer to a IPOD_TRACK_RELEASE_DATE_DATA structure <br>
 * #IPOD_TRACK_ARTWORK_COUNT_DATA artworkCountData* - pointer to a IPOD_TRACK_ARTWORK_COUNT_DATA structure <br>
 * U8* stringBuf - pointer to a nullterminated UTF-8 string <br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called from
 * the function iPodGetIndexedPlayingTrackInfo.<br>
 * Depending on for what kind of track info type the callback function is called, one of
 * pointers is valid.<br>
 * If the type is IPOD_TRACK_CAP_INFO the pointer IPOD_TRACK_CAP_INFO_DATA* is valid.<br>
 * If the type is IPOD_TRACK_RELEASE_DATE the pointer IPOD_TRACK_RELEASE_DATE_DATA* is valid.<br>
 * If the type is IPOD_TRACK_ARTWORK_COUNT the pointer IPOD_TRACK_ARTWORK_COUNT_DATA* is valid.<br>
 * For all other types the pointer to the nullterminated UTF-8 string is valid.<br>
 *<br>
 * please also refer to "iPod Extended Interface Specification" page 61 ff.
 */
typedef void (*IPOD_CB_PLAYING_TRACK_INFO) (IPOD_TRACK_INFO_TYPE infoType,
                                            const IPOD_TRACK_CAP_INFO_DATA* capInfoData,
                                            const IPOD_TRACK_RELEASE_DATE_DATA* releaseData,
                                            const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData,
                                            U8* stringBuf,
                                            const U32 iPodID);


/*!
 * \typedef void (*IPOD_CB_RETRIEVE_CAT_DB_RECORDS) (U32 index, U8* string, const U32 iPodID)
 * \par INPUT PARAMETERS
 * U32 index - database record category index <br>
 * U8* string - database record as a nullterminated UTF-8 string <br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called from
 * the function iPodRetrieveCategorizedDBRecords.
 */
typedef void (*IPOD_CB_RETRIEVE_CAT_DB_RECORDS) (U32 index, U8* string, const U32 iPodID);


/*!
 * \typedef void (*IPOD_CB_NOTIFY) (IPOD_CHANGED_PLAY_STATUS status, U64 param, const U32 iPodID)
 * \par INPUT PARAMETERS
 * #IPOD_CHANGED_PLAY_STATUS status - the status change info <br>
 * U64 param - value depending on the status info: <br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * for IPOD_STATUS_TRACK_CHANGED = new track record index <br>
 * for IPOD_STATUS_TRACK_POSITION = new track position in ms <br>
 * for IPOD_STATUS_CHAPTER_CHANGED = new chapter index <br>
 * for IPOD_STATUS_PLAYBACK_EXTENDED = extended playback status; see<br>
 * for IPOD_STATUS_TRACK_TIME_OFFSET = new track position in sec<br>
 * for IPOD_STATUS_CHAPTER_MS_OFFSET = new chapter position in ms<br>
 * for IPOD_STATUS_CHAPTER_SEC_OFFSET = new chapter position in sec<br>
 * for IPOD_TRACK_UID = new track unique identifier<br>
 * for IPOD_TRACK_LIRICS_READY = new track lyrics<br>
 * for IPOD_STATUS_PLAYBACK_STOPPED = param invalid <br>
 * for IPOD_STATUS_FWD_SEEK_STOP = param invalid <br>
 * for IPOD_STATUS_BWD_SEEK_STOP = param invalid <br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called if
 * the iPod's play status change notification is enabled.
 */
typedef void (*IPOD_CB_NOTIFY) (IPOD_CHANGED_PLAY_STATUS status, U64 param , const U32 iPodID);


/*!
 * \typedef void (*IPOD_CB_GET_ARTWORK) (IPOD_ALBUM_ARTWORK* artworkData, const U32 iPodID)
 * \par INPUT PARAMETERS
 * #IPOD_ALBUM_ARTWORK artworkData* - pointer to IPOD_ALBUM_ARTWORK structure <br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called when
 * the iPod sends artwork data as a result to a call to "iPodGetTrackArtworkData".<br>
 *
 * This callback is called only once, after all packages from iPod were received.
 * It will contain all of the artwork image data (the complete image) in memory,
 * which will be freed after 
 * the callback returns. So if the MC application intends to use that data at
 * a later point in time, it must make a copy of that image data.
 */
typedef void (*IPOD_CB_GET_ARTWORK) (IPOD_ALBUM_ARTWORK* artworkData , const U32 iPodID);


/*!
 * \typedef void (*IPOD_CB_NOTIFY_STATE_CHANGE) (IPOD_STATE_CHANGE stateChange, const U32 iPodID)
 * \par INPUT PARAMETERS
 * #IPOD_STATE_CHANGE stateChange - the new state <br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called when
 * the iPod's state changes. When the iPod is connected to a USB port with active 
 * USB power, it does not go in to Sleep or Hibernate mode, so if this callback 
 * is called, it means there must be some problem with the power provided to 
 * the iPod on USB (e.g. current too low).
 */
typedef void (*IPOD_CB_NOTIFY_STATE_CHANGE) (IPOD_STATE_CHANGE stateChange , const U32 iPodID);


/*!
 * \typedef void (*IPOD_CB_GET_ACC_SAMPLE_RATE_CAPS) (U32 iPodID)
 * \par INPUT PARAMETERS
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called when
 * the iPod requests the list of supported sample rates from the accessory. The
 * iPod sends it after the accessory announces that it supports the digital audio
 * lingo using the general lingo "IdentifyDeviceLingoes" command.
 * At the end of the callback function a "RetAccSampleRateCaps" must be send.
 */
typedef void (*IPOD_CB_GET_ACC_SAMPLE_RATE_CAPS) (U32 iPodID);


/*!
 * \typedef void (*IPOD_CB_NEW_TRACK_INFO) (U32 newSampleRate, S32 newSoundCheckValue, S32 newTrackVolAdjustment, const U32 iPodID);
 * \par INPUT PARAMETERS
 * U32 newSampleRate - the track's sample rate
 * S32 newSoundCheckValue - the track's sound check value
 * S32 newTrackVolAdjustment - the track's volume adjustment
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called before the first audio track
 * begins playing. The callback is executed again whenever the iPod starts playing a track with
 * different sample rate, sound check, or track volume parameters. In response to this command,
 * accessories should prepare themselves to receive audio data with the new parameters.
 * At the end of the callback function an AccAck command must be send, but the iPod will not
 * wait for this acknowledgment before allowing digital audio to be transferred to the accessory.
 *
 * The Sound Check value and track volume adjustment value are the corresponding values set by
 * iTunes, rounded to the nearest integer. The values represent gain (in dB) and may be positive or
 * negative. Note that if the Sound Check option in the iPod is disabled, the parameter "newSoundCheckValue"
 * will always be 0.
 *
 * This callback function should not be used as a general mechanism to detect track changes.
 * Use appropriate commands from the Display Remote or Extended Mode lingoes instead.
 *
 * If the MC application implements the audio streaming, this callback must be used to determine the 
 * current sample rate. If the ADIT iPod Control audio streaming implementation is used, the 
 * change of sample rate is done internally and the only action to be done by the MC application 
 * is to adjust the output volume, if SoundCheckValue or TrackVolAdjustment must be supported.
 */
typedef void (*IPOD_CB_NEW_TRACK_INFO) (U32 newSampleRate,
                                        S32 newSoundCheckValue,
                                        S32 newTrackVolAdjustment , const U32 iPodID);


/*!
 * \typedef void (*IPOD_CB_IPOD_ACK) (IPOD_ACC_ACK_STATUS status, const U32 iPodID)
 * \par INPUT PARAMETERS
 * #IPOD_ACC_ACK_STATUS status - type of status of accesspry ack<br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * The iPod sends the iPodAck command when it receives an invalid or unsupported command
 * or a bad parameter.
 */
typedef void (*IPOD_CB_IPOD_ACK) (IPOD_ACC_ACK_STATUS status , const U32 iPodID);


/*!
 * \typedef void(*IPOD_TRACK_INFORMATION_CB) (U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData, const U32 iPodID)
 * \par INPUT PARAMETERS
 * U64 trackIndex - If it is callback for GetUIDTrackInfo, this is U64 Unique ID.<br>
 * If it is callback for GetDBTrackInfo, this is U32 Database Index.<br>
 * If it is callback for GetPBTrackInfo, this is U32 Playback Index.<br>
 * #IPOD_TRACK_INFORMATION_TYPE - type of several track information(e.g track name, chapter count).<br>
 * #IPOD_TRACK_INFORMATION_DATA - data of several trackinformation.(e.g if type is name, data is string. if type is chapter count, data is U16)
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This is the prototype of the callback funtion that will be called 
 * when tha iPods sends a track Information as a result to a call to "iPodGetUIDTrackInfo", "iPodGetDBTrackInfo", and "iPodGetPBTrackInfo".<br>
 * This callback may be called in few times by one request.
 */
typedef void(*IPOD_TRACK_INFORMATION_CB) (U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData, const U32 iPodID);


/*!
 * \typedef void(*IPOD_CB_LOCATION) (#IPOD_LOCATION_CMD locCmd, #IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 size, const U32 iPodID)
 * \par INPUT PARAMETERS
 * #IPOD_LOCATION_CMD - Specifies the Location command sent by the iPod.<br>
 * #IPOD_LOCATION_TYPE - Specifies the location type for this command.<br>
 * U8 dataType - Specifies the type of data. This is different for each location type. <br>
 * U32 locSize - Specifies the size of location data.<br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * INOUT_PARAMETERS
 * U8 *locData - is the location data. This is different for each location type and data type.<br>
 * \par REPLY PARAMETERS
 * #IPOD_OK - command success<br>
 * other error - application reply NON IPOD_OK error.<br>
 * \par DESCRIPTION
 * This is the prototype of the callback function that will be called
 * when the iPods send a Location command.<br>
 * This callback may be called if support for the Location Lingo was enabled.<br>
 * The Location Lingo and how to use it is a rather complex topic. It is not possible to explain in detail at this point.
 * To correctly use the Location Lingo and correctly reply to the iPod commands, ADIT strongly recommends 
 * to refer to the specification by Apple.<br>
 */
typedef S32(*IPOD_CB_LOCATION) (IPOD_LOCATION_CMD locCmd, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 size , const U32 iPodID);

/*!
 * \typedef void(*IPOD_CB_NOTIFICATION) (IPOD_NOTIFY_TYPE type, IPOD_NOTIFY_STATUS status, const U32 iPodID)
 * \par INPUT PARAMETERS
 * #IPOD_NOTIFY_TYPE type - Specifies the command send by the iPod<br>
 * #IPOD_NOTIFY_STATUS status - Specifies the information by the iPod. The meaning varies depending on the command type.<br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * INOUT_PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This callback is used to get a notification when:<br>
 * - iPod's command queue is full (please wait before you send next command)<br>
 * - A new radio tag is available (to be used if the radio tagging feature is implemented)<br>
 * - The camera status has changes (only relevant if control for the build-in camera of the 
 * iPod nano 5G was implemented, a feature which is currently not supported by ADIT iPod Control).
 */
typedef void(*IPOD_CB_NOTIFICATION) (IPOD_NOTIFY_TYPE type, IPOD_NOTIFY_STATUS status , const U32 iPodID);


/*!
 * \typedef void(*IPOD_CB_REMOTE_EVENT_NOTIFICATION) (IPOD_STATE_INFO_TYPE eventNum,
 *                                                    IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData, const U32 iPodID)
 * \par INPUT PARAMETERS
 * #IPOD_STATE_INFO_TYPE eventNum - Specifies the command send by the iPod <br>
 * #IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData - Specifies the information by the iPod. The meaning varies depending on the command type.<br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * INOUT_PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This callback is used to get a remote event notification whenever an enabled event change has occurred. <br>
 */
typedef void(*IPOD_CB_REMOTE_EVENT_NOTIFICATION) (IPOD_STATE_INFO_TYPE eventNum, IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData, const U32 iPodID);


/*!
 * \typedef S32(*IPOD_CB_OPEN_DATA_SESSION) (U8 protocolIndex, U16 sessionId, const U32 iPodID)
 * \par INPUT PARAMETERS
 * U8 protocolIndex - This is given from iPod for each application.<br>
 * U16 sessionId - This is given to target protocolIndex for communicating with iPhone application.<br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * INOUT_PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This callback is used to communicate with iPhone application.<br>
 * To communicate with Application, accessory use given sessionId together with data.
 * Protocol Index is incremented by one from head of devconf value.
 */
typedef S32(*IPOD_CB_OPEN_DATA_SESSION) (U8 protocolIndex, U16 sessionId , const U32 iPodID);

/*!
 * \typedef void(*IPOD_CB_CLOSE_DATA_SESSION) (U16 sessionId, const U32 iPodID)
 * \par INPUT PARAMETERS
 * U16 sessionId - This is given to target protocolIndex for communicating with iPhone application.<br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * INOUT_PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This callback is used to stop the communication with iPhone application.<br>
 * Session ID can not use to communicate with iPhone application after call this function.
 */
typedef void(*IPOD_CB_CLOSE_DATA_SESSION) (U16 sessionId , const U32 iPodID);

/*!
 * \typedef S32(*IPOD_CB_IPOD_DATA_TRANSFER) (U8 *data, U16 length, const U32 iPodID)
 * \par INPUT PARAMETERS
 * U16 sessionId - This is given to target protocolIndex for communicating with iPhone application.<br>
 * U8 data* - This is data of application.<br>
 * U16 length - This is length of application data including NULL-terminated.<br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * INOUT_PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This callback is used that iPod sends the data to accessory.<br>
 */
typedef S32(*IPOD_CB_IPOD_DATA_TRANSFER) (U16 sessionId, U8 *data, U16 length , const U32 iPodID);

/*!
 * \typedef S32(*IPOD_CB_SET_ACC_STATUS) (U32 statusMask, const U32 iPodID)
 * \par INPUT PARAMETERS
 * U32 statusMask - This is requested status..<br>
 * U32 iPodID - Apple device ID inside iPod Ctrl<br>
 * INOUT_PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This callback is used that iPod requests indicated status.<br>
 */
typedef void(*IPOD_CB_SET_ACC_STATUS) (U32 statusMask , const U32 iPodID);

#ifdef __cplusplus
}
#endif

#endif /* IAP_TYPES_H */

/*\}*/

