/********************************************************************//**
 * \file iap2_enums.h
 * \brief iAP2 Control session enumerations
 *
 * \version $ $
 *
 * This header file declares enumerations associated with iAP2 implementation.
 *
 * \component global definition file
 *
 * \author Konrad Gerhards/ADITG/ kgerhards@de.adit-jv.com
 *
 * \copyright (c) 2010 - 2016 ADIT Corporation
 *
 * \warning CAUTION: CODES IN THIS FILE ARE AUTOMATCALLY GENERATED BY ENTERPRISE ARCHITECHT,
 *          DO NOT CHANGE THE CODE MANUALLY
 ***********************************************************************/

#ifndef IAP2_ENUMS_H
#define IAP2_ENUMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adit_typedef.h"

typedef enum
{
    IAP2_ACCEPT_CALL_ACCEPT,            /**< Accept/HoldAndAccept */
    IAP2_ACCEPT_CALL_END_AND_ACCEPT     /**< End and Accept */
} iAP2AcceptCallAcceptAction;

/* CAUTION: The enum iAP2AccessoryPowerModes is Deprecated, Don't use it. Kindly refer the Apple Specification. */
typedef enum
{
    IAP2_ACCESSORY_POWER_NO_POWER,                                    /**< No Power */
    IAP2_ACCESSORY_POWER_LOW_POWER,                                   /**< Low Power Mode */
    IAP2_ACCESSORY_POWER_INTERMITTENT_OR_ULTRA_HIGH_POWER = 2,        /**< Intermittent or Ultra Hight Power Mode */
    /* CAUTION: The enum IAP2_ACCESSORY_POWER_HIGH_POWER is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_ACCESSORY_POWER_HIGH_POWER = 2                               /**< Ultra Hight Power Mode */
} iAP2AccessoryPowerModes;

typedef enum
{
    IAP2_LAUNCH_WITH_USER_ALERT,       /**< Launch with user alert (default) */
    IAP2_LAUNCH_WITHOUT_USER_ALERT     /**< Launch without any alert, but device must be unlocked and on Home Screen or Music/Video app */
} iAP2AppLaunchMethod;

typedef enum
{
    IAP2_UNKNOWN,     /**< CarPlay not set up */
    IAP2_DECLINED,    /**< User did not accept */
    IAP2_ACCEPTED     /**< User accepted */
} iAP2AppListAvailable;

typedef enum
{
    IAP2_BATTERY_CHARGING_STATE_DISABLED,                   /**< BatteryChargingState - Disabled */
    IAP2_BATTERY_CHARGING_STATE_CHARGING,                   /**< BatteryChargingState - Charging */
    IAP2_BATTERY_CHARGING_STATE_CHARGED_NEW = 2,            /**< BatteryChargingState - Charged */
    /* CAUTION: The enum IAP2_BATTERY_CHARGING_STATE_TRICKLE is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_BATTERY_CHARGING_STATE_TRICKLE = 2,                /**< BatteryChargingState - Trickle */
    /* CAUTION: The enum IAP2_BATTERY_CHARGING_STATE_CHARGED is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_BATTERY_CHARGING_STATE_CHARGED = 3                 /**< BatteryChargingState - Charged */
} iAP2BatteryChargingState;

/* CAUTION: The enum iAP2CallStateDirectionValue is Deprecated, Don't use it. Kindly refer the Apple Specification. */
typedef enum
{
    IAP2_TELEPHONY_CALL_INCOMING,    /**< Incoming Call */
    IAP2_TELEPHONY_CALL_OUTGOING,    /**< Outgoing Call */
    IAP2_TELEPHONY_CALL_UNKNOWN      /**< Unknown Direction */
} iAP2CallStateDirectionValue;

/* CAUTION: The enum iAP2CallStateStatusValue is Deprecated, Don't use it. Kindly refer the Apple Specification. */
typedef enum
{
    IAP2_TELEPHONY_CALL_DISCONNECTED,    /**< Call disconnected */
    IAP2_TELEPHONY_CALL_ACTIVE,          /**< Call in progress */
    IAP2_TELEPHONY_CALL_WAITING,         /**< Call waiting */
    IAP2_TELEPHONY_CALL_CONNECTING       /**< Call connecting */
} iAP2CallStateStatusValue;

typedef enum
{
    IAP2_CALL_DIRECTION_UNKNOWN,     /**< Unknown Direction */
    IAP2_CALL_DIRECTION_INCOMING,    /**< Incoming Call */
    IAP2_CALL_DIRECTION_OUTGOING     /**< Outgoing Call */
} iAP2CallStateUpdateDirection;

typedef enum
{
    IAP2_CALL_DIRECTION_LEGACY_INCOMING,    /**< Incoming Call */
    IAP2_CALL_DIRECTION_LEGACY_OUTGOING,    /**< Outgoing Call */
    IAP2_CALL_DIRECTION_LEGACY_UNKNOWN      /**< Unknown Direction */
} iAP2CallStateUpdateDirectionLegacy;

typedef enum
{
    IAP2_CALL_DISCONNECT_REASON_ENDED,       /**< Call ended */
    IAP2_CALL_DISCONNECT_REASON_DECLINED,    /**< Call declined */
    IAP2_CALL_DISCONNECT_REASON_FAILED       /**< Call failed */
} iAP2CallStateUpdateDisconnectReason;

typedef enum
{
    IAP2_CALL_SERVICE_UNKNOWN,           /**< Unknown service */
    IAP2_CALL_SERVICE_TELEPHONY,         /**< Telephony call */
    IAP2_CALL_SERVICE_FACETIME_AUDIO,    /**< FaceTime Audio call */
    IAP2_CALL_SERVICE_FACETIME_VIDEO     /**< FaceTime Video call */
} iAP2CallStateUpdateService;

typedef enum
{
    IAP2_CALL_STATUS_DISCONNECTED,     /**< Call disconnected */
    IAP2_CALL_STATUS_SENDING,          /**< Call in progress */
    IAP2_CALL_STATUS_RINGING,          /**< Call waiting */
    IAP2_CALL_STATUS_CONNECTING,       /**< Call connecting */
    IAP2_CALL_STATUS_ACTIVE,           /**< Call in active */
    IAP2_CALL_STATUS_HELD,             /**< Call in hold */
    IAP2_CALL_STATUS_DISCONNECTING     /**< Call is disconnecting */
} iAP2CallStateUpdateStatus;

typedef enum
{
    IAP2_CALL_STATUS_LEGACY_DISCONNECTED,    /**< Call disconnected */
    IAP2_CALL_STATUS_LEGACY_ACTIVE,          /**< Call in active */
    IAP2_CALL_STATUS_LEGACY_HELD,            /**< Call on hold */
    IAP2_CALL_STATUS_LEGACY_RINGING          /**< Call waiting */
} iAP2CallStateUpdateStatusLegacy;

typedef enum
{
    IAP2_REGISTRATION_STATUS_UNKNOWN,            /**< unknown */
    IAP2_REGISTRATION_STATUS_NOT_REGISTERED,     /**< Device not registered with network */
    IAP2_REGISTRATION_STATUS_SEARCHING,          /**< Device is searching for network */
    IAP2_REGISTRATION_STATUS_DENIED,             /**< Device registration denied */
    IAP2_REGISTRATION_STATUS_HOME,               /**< Home */
    IAP2_REGISTRATION_STATUS_ROAMING,            /**< Device is in roaming */
    IAP2_REGISTRATION_STATUS_EMERGENCY_CALLS     /**< Emergency calls only */
} iAP2CommunicationsUpdateRegistrationStatus;

typedef enum
{
    IAP2_SIGNAL_STRENGTH_0_BARS,    /**< No Signal */
    IAP2_SIGNAL_STRENGTH_1_BARS,    /**< Signal strength 1 bar */
    IAP2_SIGNAL_STRENGTH_2_BARS,    /**< Signal strength 2 bars */
    IAP2_SIGNAL_STRENGTH_3_BARS,    /**< Signal strength 3 bars */
    IAP2_SIGNAL_STRENGTH_4_BARS,    /**< Signal strength 4 bars */
    IAP2_SIGNAL_STRENGTH_5_BARS     /**< Signal strength 5 bars(full) */
} iAP2CommunicationsUpdateSignalStrength;

typedef enum
{
    IAP2_DISTANCE_DISPLAY_UNITS_KM,        /**< Units in kilometers */
    IAP2_DISTANCE_DISPLAY_UNITS_MILES,     /**< Units in miles */
    IAP2_DISTANCE_DISPLAY_UNITS_METERS,    /**< Units in meters */
    IAP2_DISTANCE_DISPLAY_UNITS_YARDS,     /**< Units in yards */
    IAP2_DISTANCE_DISPLAY_UNITS_FT         /**< Units in feet */
} iAP2DistanceDisplayUnits;

typedef enum
{
    IAP2_DRIVING_SIDE_RIGHT,    /**< Right or Anticlockwise for Roundabouts */
    IAP2_DRIVING_SIDE_LEFT      /**< Left or clockwise for Roundabouts */
} iAP2DrivingSide;

typedef enum
{
    IAP2_END,        /**< End call */
    IAP2_END_ALL     /**< End all calls */
} iAP2EndCallEndAction;

typedef enum
{
    IAP2_GASOLINE,    /**< Gasoline Engine */
    IAP2_DIESEL,      /**< Diesel Engine */
    IAP2_ELECTRIC,    /**< Electrical Engine */
    IAP2_CNG          /**< Compressed Natural Gas Engine */
} iAP2EngineTypes;

typedef enum
{
    IAP2_NO_ACTION = 0,                            /**< Device will not prompt the user to find a matching app, and there will not be a Find App for this Accessory button in Settings &gt; General &gt; About &gt; 'Accessory Name' */
    IAP2_OPTIONAL_ACTION = 1,                      /**< Device may prompt the user to find a matching app, and there will be a Find App for this Accessory button in Settings &gt; General &gt; About &gt; 'Accessory Name' */
    IAP2_NO_ALERT = 2,                             /**< Device will not prompt the user to find a matching app, but there will be a Find App for this Accessory button in Settings &gt; General &gt; About &gt; 'Accessory Name' */
    IAP2_NO_COMMUNICATION_PROTOCOL,                /**< The protocol is not intended for communication and does not require StratExternalAccessoryProtocolSession or StopExternalAccessoryProtocolSession. The device may prompt the user to find a matching app and there will be a FindApp For This Accessory button in Settings &gt; General &gt; About &gt; 'Accessory Name' */
    /* CAUTION: The enum IAP2_NO_APP_MATCH is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_NO_APP_MATCH = 0,                         /**< Device will not prompt the user to find a matching app, and there will not be a Find App for this Accessory button in Settings &gt; General &gt; About &gt; 'Accessory Name' */
    /* CAUTION: The enum IAP2_APP_MATCH is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_APP_MATCH = 1,                            /**< Device may prompt the user to find a matching app, and there will be a Find App for this Accessory button in Settings &gt; General &gt; About &gt; 'Accessory Name' */
    /* CAUTION: The enum IAP2_USER_APP_MATCH is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_USER_APP_MATCH = 2                        /**< Device will not prompt the user to find a matching app, but there will be a Find App for this Accessory button in Settings &gt; General &gt; About &gt; 'Accessory Name' */
} iAP2ExternalAccessoryProtocolMatchAction;

typedef enum
{
    IAP2_SESSION_STATUS_OK,    /**< EAP Session Status - OK */
    IAP2_SESSION_CLOSE         /**< EAP Session Status - Close */
} iAP2ExternalAccessoryProtocolSessionStatus;

typedef enum
{
    IAP2_HID_COMPONENT_KEYBOARD,                                /**< HID function Type: Keyboard */
    IAP2_HID_COMPONENT_PLAYBACK_REMOTE,                         /**< HID function Type: Playback remote */
    IAP2_HID_COMPONENT_ASSISTIVE_TOUCH,                         /**< HID function Type: Assistive touch */
    IAP2_HID_COMPONENT_RESERVED = 3,                            /**< HID function Type: Reserved */
    IAP2_HID_COMPONENT_EXTENDED_GAMEPAD_FORM_FITTING,           /**< HID function Type: Extended Gamepad form fitting */
    IAP2_HID_COMPONENT_EXTENDED_GAMEPAD_NONFORM_FITTING,        /**< HID function Type: Extended Gamepad non-form fitting */
    IAP2_HID_COMPONENT_SWITCH_CONTROL,                          /**< HID function Type: switch control */
    IAP2_HID_COMPONENT_HEADSET,                                 /**< HID function Type: Headset */
    /* CAUTION: The enum IAP2_HID_COMPONENT_STANDARD_GAMEPAD is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_HID_COMPONENT_STANDARD_GAMEPAD = 3                     /**< HID function Type: standard Gamepad */
} iAP2HIDComponentFunction;

typedef enum
{
    IAP2_INITIATE_CALL_SERVICE_TELEPHONY = 1,     /**< Telephony call */
    IAP2_INITIATE_CALL_SERVICE_FACETIME_AUDIO,    /**< FaceTime Audio call */
    IAP2_INITIATE_CALL_SERVICE_FACETIME_VIDEO     /**< FaceTime Video call */
} iAP2InitiateCallService;

typedef enum
{
    IAP2_INITIATE_CALL_TYPE_DESTINATION,    /**< Destination */
    IAP2_INITIATE_CALL_TYPE_VOICEMAIL,      /**< Voicemail */
    IAP2_INITIATE_CALL_TYPE_REDIAL          /**< Redial */
} iAP2InitiateCallType;

typedef enum
{
    IAP2_JUNCTION_TYPE_SINGLE_INTERSECTION_WITH_JUNCTION_ELEMENTS,    /**< Single Intersection with Junction Elements */
    IAP2_JUNCTION_TYPE_ROUNDABOUT_WITH_JUNCTION_ELEMENTS              /**< Roundabout with Junction Elements */
} iAP2JunctionType;

typedef enum
{
    IAP2_NOT_GOOD,     /**< Not Good */
    IAP2_GOOD,         /**< Good */
    IAP2_PREFERRED     /**< Preferred */
} iAP2LaneStatus;

typedef enum
{
    IAP2_RECENTS_LIST_TYPE_UNKNOWN,     /**< Unknown type */
    IAP2_RECENTS_LIST_TYPE_INCOMING,    /**< Incoming */
    IAP2_RECENTS_LIST_TYPE_OUTGOING,    /**< Outgoing */
    IAP2_RECENTS_LIST_TYPE_MISSED       /**< Missed */
} iAP2ListUpdateRecentsListType;

typedef enum
{
    IAP2_LIST_SERVICE_UNKNOWN,           /**< Unknown type */
    IAP2_LIST_SERVICE_TELEPHONY,         /**< Telephony call */
    IAP2_LIST_SERVICE_FACETIME_AUDIO,    /**< FaceTime Audio call */
    IAP2_LIST_SERVICE_FACETIME_VIDEO     /**< FaceTime Video call */
} iAP2ListUpdateService;

typedef enum
{
    IAP2_MANEUVER_STATE_CONTINUE,    /**< Continue along this route until next maneuver */
    IAP2_MANEUVER_STATE_INITIAL,     /**< Notification that the maneuver is in near future */
    IAP2_MANEUVER_STATE_PREPARE,     /**< Notification that the maneuver is in immediate future */
    IAP2_MANEUVER_STATE_EXECUTE      /**< Vehicle is in the maneuver */
} iAP2ManeuverState;

typedef enum
{
    IAP2_MANEUVER_TYPE_NO_TURN,                           /**< No Turn */
    IAP2_MANEUVER_TYPE_LEFT_TURN,                         /**< Left Turn. Angle is between -45 and -135 */
    IAP2_MANEUVER_TYPE_RIGHT_TURN,                        /**< Right Turn. Angle is between 45 and 135 */
    IAP2_MANEUVER_TYPE_STRAIGHT_AHEAD,                    /**< Straight Ahead. Continue straight through the intersection */
    IAP2_MANEUVER_TYPE_MAKE_U_TURN,                       /**< Make a U-Turn */
    IAP2_MANEUVER_TYPE_CONTINUE,                          /**< Continue to follow the road the vehicle is currently on. */
    IAP2_MANEUVER_TYPE_ENTER_ROUNDABOUT,                  /**< Enter Roundabout */
    IAP2_MANEUVER_TYPE_EXIT_ROUNDABOUT,                   /**< Exit Roundabout */
    IAP2_MANEUVER_TYPE_OFF_RAMP,                          /**< Off Ramp. Take a ramp to leave the highway. */
    IAP2_MANEUVER_TYPE_ON_RAMP,                           /**< On Ramp. Take the ramp to merge onto the highway. */
    IAP2_MANEUVER_TYPE_ARRIVE_END_OF_NAVIGATION,          /**< Arrive End of Navigation */
    IAP2_MANEUVER_TYPE_PROCEED_TO_BEGINNING,              /**< Proceed To Beginning of the route. */
    IAP2_MANEUVER_TYPE_ARRIVE_AT_DESTINATION,             /**< Arrive at destination

 */
    IAP2_MANEUVER_TYPE_KEEP_LEFT,                         /**< Keep Left */
    IAP2_MANEUVER_TYPE_KEEP_RIGHT,                        /**< Keep Right */
    IAP2_MANEUVER_TYPE_ENTER_FERRY,                       /**< Enter Ferry */
    IAP2_MANEUVER_TYPE_EXIT_FERRY,                        /**< Exit Ferry */
    IAP2_MANEUVER_TYPE_CHANGE_TO_DIFFERENT_FERRY,         /**< Change to a Different Ferry */
    IAP2_MANEUVER_TYPE_MAKE_U_TURN_AND_PROCEED,           /**< Make a U-Turn and Proceed */
    IAP2_MANEUVER_TYPE_USE_ROUNDABOUT_TO_MAKE_A_UTURN,    /**< Use the roundabout to make a U-turn */
    IAP2_MANEUVER_TYPE_AT_THE_END_OF_ROAD_TURN_LEFT,      /**< At the End of Road, Turn Left */
    IAP2_MANEUVER_TYPE_AT_THE_END_OF_ROAD_TURN_RIGHT,     /**< At the End of Road, Turn Right */
    IAP2_MANEUVER_TYPE_HIGHWAY_OFF_RAMP_LEFT,             /**< Highway off Ramp Left */
    IAP2_MANEUVER_TYPE_HIGHWAY_OFF_RAMP_RIGHT,            /**< Highway off Ramp Right */
    IAP2_MANEUVER_TYPE_ARRIVE_AT_DESTINATION_LEFT,        /**< Arrive at Destination Left */
    IAP2_MANEUVER_TYPE_ARRIVE_AT_DESTINATION_RIGHT,       /**< Arrive at Destination Right */
    IAP2_MANEUVER_TYPE_MAKE_A_UTURN_WHEN_POSSIBLE,        /**< Make a U-turn When Possible */
    IAP2_MANEUVER_TYPE_ARRIVE_END_OF_DIRECTIONS,          /**< Arrive End of Directions */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_1,                 /**< Roundabout Exit 1. Exit the roundabout at the 1st street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_2,                 /**< Roundabout Exit 2. Exit the roundabout at the 2nd street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_3,                 /**< Roundabout Exit 3. Exit the roundabout at the 3rd street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_4,                 /**< Roundabout Exit 4. Exit the roundabout at the 4th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_5,                 /**< Roundabout Exit 5. Exit the roundabout at the 5th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_6,                 /**< Roundabout Exit 6. Exit the roundabout at the 6th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_7,                 /**< Roundabout Exit 7. Exit the roundabout at the 7th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_8,                 /**< Roundabout Exit 8. Exit the roundabout at the 8th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_9,                 /**< Roundabout Exit 9. Exit the roundabout at the 9th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_10,                /**< Roundabout Exit 10. Exit the roundabout at the 10th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_11,                /**< Roundabout Exit 11. Exit the roundabout at the 11th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_12,                /**< Roundabout Exit 12. Exit the roundabout at the 12th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_13,                /**< Roundabout Exit 13. Exit the roundabout at the 13th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_14,                /**< Roundabout Exit 14. Exit the roundabout at the 14th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_15,                /**< Roundabout Exit 15. Exit the roundabout at the 15th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_16,                /**< Roundabout Exit 16. Exit the roundabout at the 16th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_17,                /**< Roundabout Exit 17. Exit the roundabout at the 17th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_18,                /**< Roundabout Exit 18. Exit the roundabout at the 18th street. */
    IAP2_MANEUVER_TYPE_ROUNDABOUT_EXIT_19,                /**< Roundabout Exit 19. Exit the roundabout at the 19th street. */
    IAP2_MANEUVER_TYPE_SHARP_LEFT_TURN,                   /**< Sharp Left Turn. Angle is between -135 and -180 */
    IAP2_MANEUVER_TYPE_SHARP_RIGHT_TURN,                  /**< Sharp Right Turn. Angle is between 135 and 180 */
    IAP2_MANEUVER_TYPE_SLIGHT_LEFT_TURN,                  /**< Slight Left Turn */
    IAP2_MANEUVER_TYPE_SLIGHT_RIGHT_TURN,                 /**< Slight Right Turn */
    IAP2_MANEUVER_TYPE_CHANGE_HIGHWAY,                    /**< Change Highway */
    IAP2_MANEUVER_TYPE_CHANGE_HIGHWAY_LEFT,               /**< Change Highway Left */
    IAP2_MANEUVER_TYPE_CHANGE_HIGHWAY_RIGHT               /**< Change Highway Right */
} iAP2ManeuverType;

typedef enum
{
    IAP2_COLLECTION_TYPE_PLAYLIST,       /**< Media collection type is playlist */
    IAP2_COLLECTION_TYPE_ARTIST,         /**< Media collection type is artist */
    IAP2_COLLECTION_TYPE_ALBUM,          /**< Media collection type is album */
    IAP2_COLLECTION_TYPE_ALBUMARTIST,    /**< Media collection type is album artist */
    IAP2_COLLECTION_TYPE_GENRE,          /**< Media collection type is genre */
    IAP2_COLLECTION_TYPE_COMPOSER        /**< Media collection type is composer */
} iAP2MediaLibraryCollectionType;

typedef enum
{
    IAP2_LIBRARY_TYPE_LOCAL_DEVICE,             /**< Media Library is present in apple device */
    /* CAUTION: The enum IAP2_LIBRARY_TYPE_ITUNES_RADIO is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_LIBRARY_TYPE_ITUNES_RADIO = 2,         /**< Media Library is from iTunes Radio */
    IAP2_LIBRARY_TYPE_APPLE_MUSIC_RADIO = 2     /**< Media Library is from Apple Music Radio */
} iAP2MediaLibraryType;

typedef enum
{
    IAP2_MEDIA_TYPE_MUSIC,        /**< Media type is music */
    IAP2_MEDIA_TYPE_PODCAST,      /**< Podcast media */
    IAP2_MEDIA_TYPE_AUDIOBOOK,    /**< Media type is audiobook */
    /* CAUTION: The enum IAP2_MEDIA_TYPE_ITUNES_U is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_MEDIA_TYPE_ITUNES_U      /**< Media type is iTunes */
} iAP2MediaType;

typedef enum
{
    IAP2_REPEAT_OFF,    /**< iAP2 playback repeat mode OFF */
    IAP2_REPEAT_ONE,    /**< iAP2 playback repeat one track */
    IAP2_REPEAT_ALL     /**< iAP2 playback repeat all */
} iAP2PlaybackRepeat;

typedef enum
{
    IAP2_SHUFFLE_OFF,       /**< Playback shuffle OFF */
    IAP2_SHUFFLE_SONGS,     /**< Shuffle media songs */
    IAP2_SHUFFLE_ALBUMS     /**< Shuffle media albums */
} iAP2PlaybackShuffle;

typedef enum
{
    IAP2_PLAYBACK_STATUS_STOPPED,          /**< Playback stopped */
    IAP2_PLAYBACK_STATUS_PLAYING,          /**< Playback playing */
    IAP2_PLAYBACK_STATUS_PAUSED,           /**< Playback paused */
    IAP2_PLAYBACK_STATUS_SEEK_FORWARD,     /**< Playback: seek forward */
    IAP2_PLAYBACK_STATUS_SEEK_BACKWARD     /**< Playback: seek backward */
} iAP2PlaybackStatus;

typedef enum
{
    IAP2_POWER_NONE,                            /**< Accessory does not provide power to device or draw power from device */
    IAP2_POWER_LIGHTNING_RECEPTACLE = 1,        /**< Apple device manages the pass through power source */
    IAP2_POWER_ADVANCED,                        /**< Apple device is self powered */
    /* CAUTION: The enum IAP2_POWER_RESERVED is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_POWER_RESERVED = 1                     /**< Accessory provides power to device or draws power from device */
} iAP2PowerProvidingCapability;

typedef enum
{
    IAP2_ROUTE_GUIDANCE_STATE_NOT_SET,             /**< No route is set */
    IAP2_ROUTE_GUIDANCE_STATE_SET,                 /**< Route is set */
    IAP2_ROUTE_GUIDANCE_STATE_ARRIVED,             /**< Arrived */
    IAP2_ROUTE_GUIDANCE_STATE_LOADING,             /**< Loading */
    IAP2_ROUTE_GUIDANCE_STATE_LOCATING,            /**< Location/Position not available */
    IAP2_ROUTE_GUIDANCE_STATE_REROUTING,           /**< Recalculating the route */
    IAP2_ROUTE_GUIDANCE_STATE_PROCEED_TO_ROUTE     /**< Vehicle needs to join the route */
} iAP2RouteGuidanceState;

typedef enum
{
    IAP2_DTMF_TONE_NUMBER_0,    /**< DTMF Tone for 0 */
    IAP2_DTMF_TONE_NUMBER_1,    /**< DTMF Tone for 1 */
    IAP2_DTMF_TONE_NUMBER_2,    /**< DTMF Tone for 2 */
    IAP2_DTMF_TONE_NUMBER_3,    /**< DTMF Tone for 3 */
    IAP2_DTMF_TONE_NUMBER_4,    /**< DTMF Tone for 4 */
    IAP2_DTMF_TONE_NUMBER_5,    /**< DTMF Tone for 5 */
    IAP2_DTMF_TONE_NUMBER_6,    /**< DTMF Tone for 6 */
    IAP2_DTMF_TONE_NUMBER_7,    /**< DTMF Tone for 7 */
    IAP2_DTMF_TONE_NUMBER_8,    /**< DTMF Tone for 8 */
    IAP2_DTMF_TONE_NUMBER_9,    /**< DTMF Tone for 9 */
    IAP2_DTMF_TONE_STAR,        /**< DTMF Tone for * */
    IAP2_DTMF_TONE_POUND        /**< DTMF Tone for # */
} iAP2SendDTMFTone;

/* CAUTION: The enum iAP2TelephonyRegistrationStatusValue is Deprecated, Don't use it. Kindly refer the Apple Specification. */
typedef enum
{
    IAP2_TELEPHONY_REGISTRATION_UNKNOWN,            /**< unknown */
    IAP2_TELEPHONY_REGISTRATION_NOT_REGISTERED,     /**< Device not registered with network */
    IAP2_TELEPHONY_REGISTRATION_SEARCHING,          /**< Device is searching for network */
    IAP2_TELEPHONY_REGISTRATION_DENIED,             /**< Device registration denied */
    IAP2_TELEPHONY_REGISTRATION_HOME,               /**< Home */
    IAP2_TELEPHONY_REGISTRATION_ROAMING,            /**< Device is in roaming */
    IAP2_TELEPHONY_REGISTRATION_EMERGENCY_CALLS     /**< Emergency calls only */
} iAP2TelephonyRegistrationStatusValue;

/* CAUTION: The enum iAP2TelephonySignalStrengthValue is Deprecated, Don't use it. Kindly refer the Apple Specification. */
typedef enum
{
    IAP2_TELEPHONY_SIGNAL_STRENGTH_0_BARS,    /**< No Signal */
    IAP2_TELEPHONY_SIGNAL_STRENGTH_1_BARS,    /**< Signal strength 1 bar */
    IAP2_TELEPHONY_SIGNAL_STRENGTH_2_BARS,    /**< Signal strength 2 bars */
    IAP2_TELEPHONY_SIGNAL_STRENGTH_3_BARS,    /**< Signal strength 3 bars */
    IAP2_TELEPHONY_SIGNAL_STRENGTH_4_BARS,    /**< Signal strength 4 bars */
    IAP2_TELEPHONY_SIGNAL_STRENGTH_5_BARS     /**< Signal strength 5 bars(full) */
} iAP2TelephonySignalStrengthValue;

typedef enum
{
    IAP2_SAMPLERATE_8000HZ,     /**< Audio Sample rate 8000Hz */
    IAP2_SAMPLERATE_11025HZ,    /**< Audio Sample rate 11025Hz */
    IAP2_SAMPLERATE_12000HZ,    /**< Audio Sample rate 12000Hz */
    IAP2_SAMPLERATE_16000HZ,    /**< Audio Sample rate 16000Hz */
    IAP2_SAMPLERATE_22050HZ,    /**< Audio Sample rate 22050Hz */
    IAP2_SAMPLERATE_24000HZ,    /**< Audio Sample rate 24000Hz */
    IAP2_SAMPLERATE_32000HZ,    /**< Audio Sample rate 32000Hz */
    IAP2_SAMPLERATE_44100HZ,    /**< Audio Sample rate 44100Hz */
    IAP2_SAMPLERATE_48000HZ     /**< Audio Sample rate 48000Hz */
} iAP2USBDeviceModeAudioSampleRate;

typedef enum
{
    IAP2_VOICE_OVER_CURSOR_NEXT,        /**< Voice over next cursor */
    IAP2_VOICE_OVER_CURSOR_PREVIOUS,    /**< Voice over previous cursor */
    IAP2_VOICE_OVER_CURSOR_ESCAPE       /**< Voice over escape cursor */
} iAP2VoiceOverCursorDirection;

typedef enum
{
    IAP2_VOICE_OVER_TRAIT_BUTTON,                  /**<  */
    IAP2_VOICE_OVER_TRAIT_LINK,                    /**<  */
    IAP2_VOICE_OVER_TRAIT_SEARCH_FIELD,            /**<  */
    IAP2_VOICE_OVER_TRAIT_IMAGE,                   /**<  */
    IAP2_VOICE_OVER_TRAIT_SELECTED,                /**<  */
    IAP2_VOICE_OVER_TRAIT_SOUND,                   /**<  */
    IAP2_VOICE_OVER_TRAIT_KEYBOARD_KEY,            /**<  */
    IAP2_VOICE_OVER_TRAIT_STATIC_TEXT,             /**<  */
    IAP2_VOICE_OVER_TRAIT_SUMMARY_ELEMENT,         /**<  */
    IAP2_VOICE_OVER_TRAIT_NOT_ENABLED,             /**<  */
    IAP2_VOICE_OVER_TRAIT_UPDATES_FREQUENTLY,      /**<  */
    IAP2_VOICE_OVER_TRAIT_STARTS_MEDIA_SESSION,    /**<  */
    IAP2_VOICE_OVER_TRAIT_ADJUSTABLE,              /**<  */
    IAP2_VOICE_OVER_TRAIT_BLACK_BUTTON,            /**<  */
    IAP2_VOICE_OVER_TRAIT_MAP,                     /**<  */
    IAP2_VOICE_OVER_TRAIT_DELETE_KEY               /**<  */
} iAP2VoiceOverCursorUpdateTraits;

typedef enum
{
    IAP2_VOICE_OVER_SCROLL_LEFT,     /**< Voice Over Scroll direction; left */
    IAP2_VOICE_OVER_SCROLL_RIGHT,    /**< Voice Over Scroll direction; right */
    IAP2_VOICE_OVER_SCROLL_UP,       /**< Voice Over Scroll direction; Up */
    IAP2_VOICE_OVER_SCROLL_DOWN      /**< Voice Over Scroll direction; Down */
} iAP2VoiceOverScrollDirection;

typedef enum
{
    IAP2_WIFI_REQUEST_SUCCESS,        /**< WiFi Request success */
    IAP2_WIFI_REQUEST_DECLINED,       /**< WiFi Request declined */
    IAP2_WIFI_REQUEST_UNAVAILABLE     /**< WiFi information not available */
} iAP2WiFiRequestStatus;

typedef enum
{
    IAP2_WIFI_SECURITY_NONE = 0,                            /**< Not secured */
    IAP2_WIFI_SECURITY_WEP_NEW = 1,                         /**< Wired Equivalent Policy */
    IAP2_WIFI_SECURITY_WPA_WPA2 = 2,                        /**< WPA/WPA2 Personal */
    /* CAUTION: The enum IAP2_WIFI_SECURITY_WEP is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_WIFI_SECURITY_WEP = 0,                             /**< Wired Equivalent Policy */
    /* CAUTION: The enum IAP2_WIFI_SECURITY_WPA is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_WIFI_SECURITY_WPA = 1,                             /**< WiFi Protected Access(RC4 encryption mechanism) */
    /* CAUTION: The enum IAP2_WIFI_SECURITY_WPA2 is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_WIFI_SECURITY_WPA2 = 2,                            /**< WiFi Protected Access 2(AES encryption ) */
    /* CAUTION: The enum IAP2_WIFI_SECURITY_MIXED is Deprecated, Don't use it. Kindly refer the Apple Specification. */
    IAP2_WIFI_SECURITY_MIXED = 3                            /**< Mixed WPA and WPA2 */
} iAP2WiFiSecurityType;

typedef enum
{
    IAP2_WIRELESS_CARPLAY_STATUS_UNAVAILABLE,    /**< Wireless Carplay Update Unavailable */
    IAP2_WIRELESS_CARPLAY_STATUS_AVAILABLE       /**< Wireless Carplay Update Available */
} iAP2WirelessCarPlayUpdateStatus;

typedef enum
{
    iAP2_int8,
    iAP2_uint8,
    iAP2_int16,
    iAP2_uint16,
    iAP2_int32,
    iAP2_uint32,
    iAP2_int64,
    iAP2_uint64,
    iAP2_secs16,
    iAP2_secs32,
    iAP2_secs64,
    iAP2_msecs16,
    iAP2_msecs32,
    iAP2_msecs64,
    iAP2_usecs16,
    iAP2_usecs32,
    iAP2_usecs64,
    iAP2_enum,
    iAP2_bool,
    iAP2_blob,
    iAP2_utf8,
    iAP2_group,
    iAP2_none
/* Currently there are no parameters of below type
 * defined in the Apple specification. Necessary care
 * has to be taken when using these types in future
 * iAP2_array,
 * iAP2_rat16,
 * iAP2_rat32,
 * iAP2_urat16,
 * iAP2_urat32,
 */
}iAP2_Type;

#ifdef __cplusplus
}
#endif

#endif
