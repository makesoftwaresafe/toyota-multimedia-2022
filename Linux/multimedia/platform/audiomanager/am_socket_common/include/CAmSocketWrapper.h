/**
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2012, BMW AG
 *
 * \author Christian Linke, christian.linke@bmw.de BMW 2011,2012
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *  \file CAmSocketWrapper.h
 *  For further information see http://www.genivi.org/.
 */

#ifndef SOCKETWRAPPER_H_
#define SOCKETWRAPPER_H_

#include "CAmSocketHandler.h"

#define D_AM_SHIFT_1BYTE	(8)
#define D_AM_SHIFT_3BYTE	(24)

/* エンディアン変換マクロ */
#define AM_SWAP_ENDIAN16( x ) (((x & 0xFF) << D_AM_SHIFT_1BYTE) | ((x >> D_AM_SHIFT_1BYTE) & 0xFF))
#define AM_SWAP_ENDIAN32( x ) (((x >> D_AM_SHIFT_3BYTE) & 0xFF) | ((x & 0xFF) << D_AM_SHIFT_3BYTE) | ((x >> D_AM_SHIFT_1BYTE) & 0xFF00) | ((x & 0xFF00) << D_AM_SHIFT_1BYTE))

#define D_AM_MSG_LEN			(256)		/* 受信メッセージ最大長 */
#define D_AM_MSG_ESCM_SIZE		(4)			/* 受信メッセージのESCM格納バッファのサイズ */
#define D_AM_MSG_HEADSIZE		(12)		/* size from ESCM to Forwarding address */
#define D_AM_MSG_RECV_ERROR		(-1)		/* メッセージ受信recv()のエラー値 */
#define D_AM_SOCKET_FD_INVALID	(-1)		/* SOCKETのFD無効値 */

#define D_AM_MSG_DOMAIN_C_LEN	(32)		/* 受信メッセージの文字列最大長 */

/* ヘッダデータ定義 */
#define	D_AM_DOMAIN_ESCM		('E'<<24|('S'<<16)|('C'<<8)|('M'))	/* ESCMヘッダ */
#define	D_AM_DOMAIN_ADDRESS_AM	(0x7FFF)							/* AudioManagerの論理アドレス */
#define	D_AM_DOMAIN_ADDRESS_RAA	(1)									/* Logical address of RA-AMP */

/* 構造体配列最大数 */
#define D_AM_UPDATESINK_LISTNUM			(15)		/* updateSinkのリスト数 */
#define D_AM_SOUNDPROPERTIES_LISTNUM	(10)		/* SoundPropertiesのリスト数 */
#define D_AM_VOLUMES_LISTNUM			(10)		/* SoundPropertiesのリスト数 */

namespace am
{

typedef enum _OPC_ROUTING_TYPE
{
	/* AM→DOMAINのOPC */
	D_OPC_SETROUTINGREADY = 0,						/* setRoutingReady */
	D_OPC_SETROUTINGRUNDOWN,						/* setRoutingRundown */
	D_OPC_ASYNCABORT,								/* asyncAbort */
	D_OPC_ASYNCCONNECT,								/* asyncConnect */
	D_OPC_ASYNCDISCONNECT,							/* asyncDisconnect */
	D_OPC_ASYNCSETSINKVOLUME,						/* asyncSetSinkVolume */
	D_OPC_ASYNCSETSOURCEVOLUME,						/* asyncSetSourceVolume */
	D_OPC_ASYNCSETSOURCESTATE,						/* asyncSetSourceState */
	D_OPC_ASYNCSETSINKSOUNDPROPERTIES,				/* asyncSetSinkSoundProperties */
	D_OPC_ASYNCSETSINKSOUNDPROPERTY,				/* asyncSetSinkSoundProperty */
	D_OPC_ASYNCSETSOURCESOUNDPROPERTIES,			/* asyncSetSourceSoundProperties */
	D_OPC_ASYNCSETSOURCESOUNDPROPERTY,				/* asyncSetSourceSoundProperty */
	D_OPC_ASYNCCROSSFADE,							/* asyncCrossFade */
	D_OPC_SETDOMAINSTATE,							/* setDomainState */
	D_OPC_ASYNCSETVOLUMES,							/* asyncSetVolumes */
	D_OPC_ASYNCSETSINKNOTIFICATIONCONFIGURATION,	/* asyncSetSinkNotificationConfiguration */
	D_OPC_ASYNCSETSOURCENOTIFICATIONCONFIGURATION,	/* asyncSetSourceNotificationConfiguration */
	/* AM→DOMAINの定義はここより上に追加すること */

	/* DOMAIN→AMのOPC */
	D_OPC_ACKCONNECT,								/* ackConnect */
	D_OPC_ACKDISCONNECT,							/* ackDisconnect */
	D_OPC_ACKSETSINKVOLUMECHANGE,					/* ackSetSinkVolumeChange */
	D_OPC_ACKSETSOURCEVOLUMECHANGE,					/* ackSetSourceVolumeChange */
	D_OPC_ACKSETSOURCESTATE,						/* ackSetSourceState */
	D_OPC_ACKSINKVOLUMETICK,						/* ackSinkVolumeTick */
	D_OPC_ACKSOURCEVOLUMETICK,						/* ackSourceVolumeTick */
	D_OPC_ACKSETSINKSOUNDPROPERTY,					/* ackSetSinkSoundProperty */
	D_OPC_ACKSETSOURCESOUNDPROPERTY,				/* ackSetSourceSoundProperty */
	D_OPC_ACKSETSINKSOUNDPROPERTIES,				/* ackSetSinkSoundProperties */
	D_OPC_ACKSETSOURCESOUNDPROPERTIES,				/* ackSetSourceSoundProperties */
	D_OPC_ACKCROSSFADING,							/* ackCrossFading */
	D_OPC_REGISTERDOMAIN,							/* registerDomain */
	D_OPC_REGISTERSOURCE,							/* registerSource */
	D_OPC_REGISTERSINK,								/* registerSink */
	D_OPC_REGISTERGATEWAY,							/* registerGateway */
	D_OPC_PEEKDOMAIN,								/* peekDomain */
	D_OPC_DEREGISTERDOMAIN,							/* deregisterDomain */
	D_OPC_DEREGISTERGATEWAY,						/* deregisterGateway */
	D_OPC_PEEKSINK,									/* peekSink */
	D_OPC_DEREGISTERSINK,							/* deregisterSink */
	D_OPC_PEEKSOURCE,								/* peekSource */
	D_OPC_DEREGISTERSOURCE,							/* deregisterSource */
	D_OPC_REGISTERCROSSFADER,						/* registerCrossfader */
	D_OPC_DEREGISTERCROSSFADER,						/* deregisterCrossfader */
	D_OPC_PEEKSOURCECLASSID,						/* peekSourceClassID */
	D_OPC_PEEKSINKCLASSID,							/* peekSinkClassID */
	D_OPC_HOOKINTERRUPTSTATUSCHANGE,				/* hookInterruptStatusChange */
	D_OPC_HOOKDOMAINREGISTRATIONCOMPLETE,			/* hookDomainRegistrationComplete */
	D_OPC_HOOKSINKAVAILABLITYSTATUSCHANGE,			/* hookSinkAvailablityStatusChange */
	D_OPC_HOOKSOURCEAVAILABLITYSTATUSCHANGE,		/* hookSourceAvailablityStatusChange */
	D_OPC_HOOKDOMAINSTATECHANGE,					/* hookDomainStateChange */
	D_OPC_HOOKTIMINGINFORMATIONCHANGED,				/* hookTimingInformationChanged */
	D_OPC_SENDCHANGEDDATA,							/* sendChangedData */
	D_OPC_CONFIRMROUTINGREADY,						/* confirmRoutingReady */
	D_OPC_CONFIRMROUTINGRUNDOWN,					/* confirmRoutingRundown */
	D_OPC_UPDATEGATEWAY,							/* updateGateway */
	D_OPC_UPDATESINK,								/* updateSink */
	D_OPC_UPDATESOURCE,								/* updateSource */
	D_OPC_ACKSETVOLUMES,							/* ackSetVolumes */
	D_OPC_ACKSINKNOTIFICATIONCONFIGURATION,			/* ackSinkNotificationConfiguration */
	D_OPC_ACKSOURCENOTIFICATIONCONFIGURATION,		/* ackSourceNotificationConfiguration */
	D_OPC_HOOKSINKNOTIFICATIONDATACHANGE,			/* hookSinkNotificationDataChange */
	D_OPC_HOOKSOURCENOTIFICATIONDATACHANGE,			/* hookSourceNotificationDataChange */
	D_OPC_GETINTERFACEVERSION,						/* getInterfaceVersion */
	D_OPC_GETROUTINGREADY,							/* getRoutingReady */
	/*DOMAIN→AMNの定義はここより上に追加すること */
	D_OPC_ROUTING_MAX
}OPC_ROUTING_TYPE;

/* ファイルディスクリプタータイプ(Accept or Connect) */
typedef enum _FD_TYPE_SOCKET
{
	EN_FD_TYPE_SOCKET_ACCEPT = 0,
	EN_FD_TYPE_SOCKET_CONNECT
} FD_TYPE_SOCKET;

/* SocketType */
typedef enum _SOCKET_TYPE
{
	EN_SOCKET_TYPE_INVALID = -1,
	EN_SOCKET_TYPE_AM,
	EN_SOCKET_TYPE_RAA,
} SOCKET_TYPE;

/* ソケット使用タイプ */
typedef enum _SOCKET_USED_TYPE
{
	EN_SOCKET_USED_TYPE_INVALID = -1,
	EN_SOCKET_USED_TYPE_PLUGIN,
	EN_SOCKET_USED_TYPE_CLIENT
} SOCKET_USED_TYPE;

/* PluginからClientへ送信 */

/* setRoutingReady */
typedef struct _ST_SETROUTINGREADY{
	uint16_t handle;					/* handleデータ */
}ST_SETROUTINGREADY;

/* setRoutingRundown */
typedef struct _ST_SETROUTINGRUNDOWN{
	uint16_t handle;					/* handleデータ */
}ST_SETROUTINGRUNDOWN;

/* asyncAbort */
typedef struct _ST_ASYNCABORT{
	am_Handle_s handle;					/* handleデータ */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCABORT;

/* asyncConnect */
typedef struct _ST_ASYNCCONNECT{
	am_Handle_s handle;								/* handleデータ */
	am_connectionID_t connectionID;					/* connectionID */
	am_sinkID_t sinkID;								/* SinkID */
	am_sourceID_t sourceID;							/* sourceID */
	am_CustomConnectionFormat_t connectionFormat;	/* connectionFormat */
	am_Error_e error;								/* 処理結果 */
}ST_ASYNCCONNECT;

/* asyncDisconnect */
typedef struct _ST_ASYNCDISCONNECT{
	am_Handle_s handle;					/* handleデータ */
	am_connectionID_t connectionID;		/* connectionID */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCDISCONNECT;

/* asyncSetSinkVolume */
typedef struct _ST_ASYNCSETSINKVOLUME{
	am_Handle_s handle;					/* handleデータ */
	am_sinkID_t sinkID;					/* SinkID */
	am_volume_t volume;					/* volume */
	am_CustomRampType_t ramp;			/* rampType */
	am_time_t time;
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSINKVOLUME;

/* asyncSetSourceVolume */
typedef struct _ST_ASYNCSETSOURCEVOLUME{
	am_Handle_s handle;					/* handleデータ */
	am_sourceID_t sourceID;				/* sourceID */
	am_volume_t volume;					/* volume */
	am_CustomRampType_t ramp;			/* rampType */
	am_time_t time;
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSOURCEVOLUME;

/* asyncSetSourceState */
typedef struct _ST_ASYNCSETSOURCESTATE{
	am_Handle_s handle;					/* handleデータ */
	am_sourceID_t sourceID;				/* sourceID */
	am_SourceState_e state;				/* sourceState */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSOURCESTATE;

/* asyncSetSinkSoundProperties */
typedef struct _ST_ASYNCSETSINKSOUNDPROPERTIES{
	am_Handle_s handle;					/* handleデータ */
	am_sinkID_t sinkID;					/* SinkID */
	am_SoundProperty_s listSoundProperties[D_AM_SOUNDPROPERTIES_LISTNUM];	/* SoundPropertyList */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSINKSOUNDPROPERTIES;

/* asyncSetSinkSoundProperty */
typedef struct _ST_ASYNCSETSINKSOUNDPROPERTY{
	am_Handle_s handle;					/* handleデータ */
	am_sinkID_t sinkID;					/* SinkID */
	am_SoundProperty_s soundProperty;	/* SoundProperty */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSINKSOUNDPROPERTY;

/* asyncSetSourceSoundProperties */
typedef struct _ST_ASYNCSETSOURCESOUNDPROPERTIES{
	am_Handle_s handle;					/* handleデータ */
	am_sourceID_t sourceID;				/* sourceID */
	am_SoundProperty_s listSoundProperties[D_AM_SOUNDPROPERTIES_LISTNUM];	/* SoundPropertyList */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSOURCESOUNDPROPERTIES;

/* asyncSetSourceSoundProperty */
typedef struct _ST_ASYNCSETSOURCESOUNDPROPERTY{
	am_Handle_s handle;					/* handleデータ */
	am_sourceID_t sourceID;				/* sourceID */
	am_SoundProperty_s soundProperty;	/* SoundProperty */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSOURCESOUNDPROPERTY;

/* asyncSetVolumes */
typedef struct _ST_ASYNCSETVOLUMES{
	am_Handle_s handle;					/* handleデータ */
	am_Volumes_s listVolumes[D_AM_VOLUMES_LISTNUM];			/* VolumeList */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETVOLUMES;

/* asyncSetSinkNotificationConfiguration */
typedef struct _ST_ASYNCSETSINKNOTIFICATIONCONFIGURATION{
	am_Handle_s handle;					/* handleデータ */
	am_sinkID_t sinkID;					/* SinkID */
	am_NotificationConfiguration_s notifiConf;	/* NotifiConf */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSINKNOTIFICATIONCONFIGURATION;

/* asyncSetSourceNotificationConfiguration */
typedef struct _ST_ASYNCSETSOURCENOTIFICATIONCONFIGURATION{
	am_Handle_s handle;					/* handleデータ */
	am_sourceID_t sourceID;				/* sourceID */
	am_NotificationConfiguration_s notifiConf;	/* NotifiConf */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCSETSOURCENOTIFICATIONCONFIGURATION;

/* asyncCrossFade */
typedef struct _ST_ASYNCCROSSFADE{
	am_Handle_s handle;					/* handleデータ */
	am_crossfaderID_t crossfaderID;		/* crossfaderID */
	am_HotSink_e hotSink;				/* hotSink */
	am_CustomRampType_t rampType;		/* rampType */
	am_time_t amTime;					/* amTime */
	am_Error_e error;					/* 処理結果 */
}ST_ASYNCCROSSFADE;



/* ClientからPluginへ送信 */

/* ackConnect */
typedef struct _ST_ACKCONNECT{
	am_Handle_s handle;					/* handleデータ */
	am_connectionID_t connectionID;		/* connectionID */
	am_Error_e error;					/* 処理結果 */
}ST_ACKCONNECT;

/* ackDisconnect */
typedef struct _ST_ACKDISCONNECT{
	am_Handle_s handle;					/* handleデータ */
	am_connectionID_t connectionID;		/* connectionID */
	am_Error_e error;					/* 処理結果 */
}ST_ACKDISCONNECT;

/* ackSetSinkVolumeChange */
typedef struct _ST_ACKSETSINKVOLUMECHANGE{
	am_Handle_s handle;					/* handleデータ */
	am_volume_t volume;					/* volume */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSETSINKVOLUMECHANGE;

/* ackSetSourceVolumeChange */
typedef struct _ST_ACKSETSOURCEVOLUMECHANGE{
	am_Handle_s handle;					/* handleデータ */
	am_volume_t volume;					/* volume */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSETSOURCEVOLUMECHANGE;

/* ackSetSourceState */
typedef struct _ST_ACKSETSOURCESTATE{
	am_Handle_s handle;					/* handleデータ */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSETSOURCESTATE;

/* ackSinkVolumeTick */
typedef struct _ST_ACKSINKVOLUMETICK{
	am_Handle_s handle;					/* handleデータ */
	am_sinkID_t sinkID;					/* sinkID */
	am_volume_t volume;					/* volume */
}ST_ACKSINKVOLUMETICK;

/* ackSourceVolumeTick */
typedef struct _ST_ACKSOURCEVOLUMETICK{
	am_Handle_s handle;					/* handleデータ */
	am_sourceID_t sourceID;				/* sourceID */
	am_volume_t volume;					/* volume */
}ST_ACKSOURCEVOLUMETICK;

/* ackSetSinkSoundProperties */
typedef struct _ST_ACKSETSINKSOUNDPROPERTIES{
	am_Handle_s handle;					/* handleデータ */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSETSINKSOUNDPROPERTIES;

/* ackSetSinkSoundProperty */
typedef struct _ST_ACKSETSINKSOUNDPROPERTY{
	am_Handle_s handle;					/* handleデータ */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSETSINKSOUNDPROPERTY;

/* ackSetSourceSoundProperties */
typedef struct _ST_ACKSETSOURCESOUNDPROPERTIES{
	am_Handle_s handle;					/* handleデータ */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSETSOURCESOUNDPROPERTIES;

/* ackSetSourceSoundProperty */
typedef struct _ST_ACKSETSOURCESOUNDPROPERTY{
	am_Handle_s handle;					/* handleデータ */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSETSOURCESOUNDPROPERTY;

/* ackCrossFading */
typedef struct _ST_ACKCROSSFADING{
	am_Handle_s handle;					/* handleデータ */
	am_HotSink_e hotSink;				/* アクティブなクロスフェードSinkID */
	am_Error_e error;					/* 処理結果 */
}ST_ACKCROSSFADING;


/* registerDomain */
typedef struct _ST_REGISTERDOMAIN{
	am_domainID_t domainID;						/* domainID */
	bool early;									/* early */
	bool complete;								/* complete */
	am_DomainState_e state;						/* domainState */
	am_Error_e error;							/* 処理結果 */
	char name[D_AM_MSG_DOMAIN_C_LEN];			/* domainName */
	char busname[D_AM_MSG_DOMAIN_C_LEN];		/* busName */
	char nodename[D_AM_MSG_DOMAIN_C_LEN];		/* nodeName */
}ST_REGISTERDOMAIN;

/* registerSource */
typedef struct _ST_REGISTERSOURCE{
	am_Source_s sourceData;				/* sourceData */
	am_sourceID_t sourceID;				/* sourceID */
}ST_REGISTERSOURCE;

/* registerSink */
typedef struct _ST_REGISTERSINK{
	am_Sink_s sinkData;					/* sinkData */
	am_sinkID_t sinkID;					/* sinkID */
}ST_REGISTERSINK;

/* registerGateway */
typedef struct _ST_REGISTERGATEWAY{
	am_Gateway_s gatewayData;			/* gatewayData */
	am_gatewayID_t gatewayID;			/* gatewayID */
}ST_REGISTERGATEWAY;

/* peekDomain */
typedef struct _ST_PEEKDOMAIN{
	std::string name;					/* name */
	am_domainID_t domainID;				/* domainID */
}ST_PEEKDOMAIN;

/* deregisterDomain */
typedef struct _ST_DEREGISTERDOMAIN{
	am_domainID_t domainID;				/* domainID */
}ST_DEREGISTERDOMAIN;

/* deregisterGateway */
typedef struct _ST_DEREGISTERGATEWAY{
	am_gatewayID_t gatewayID;			/* gatewayID */
}ST_DEREGISTERGATEWAY;

/* peekSink */
typedef struct _ST_PEEKSINK{
	std::string name;					/* name */
	am_sinkID_t sinkID;					/* sinkID */
}ST_PEEKSINK;

/* deregisterSink */
typedef struct _ST_DEREGISTERSINK{
	am_sinkID_t sinkID;					/* sinkID */
}ST_DEREGISTERSINK;

/* peekSource */
typedef struct _ST_PEEKSOURCE{
	std::string name;					/* name */
	am_sourceID_t sourceID;				/* sourceID */
}ST_PEEKSOURCE;

/* deregisterSource */
typedef struct _ST_DEREGISTERSOURCE{
	am_sourceID_t sourceID;				/* sourceID */
}ST_DEREGISTERSOURCE;

/* registerCrossfader */
typedef struct _ST_REGISTERCROSSFADER{
	am_Crossfader_s crossfaderData;		/* crossfaderData */
	am_crossfaderID_t crossfaderID;		/* crossfaderID */
}ST_REGISTERCROSSFADER;

/* deregisterCrossfader */
typedef struct _ST_DEREGISTERCROSSFADER{
	am_crossfaderID_t crossfaderID;		/* crossfaderID_10 */
}ST_DEREGISTERCROSSFADER;

/* peekSourceClassID */
typedef struct _ST_PEEKSOURCECLASSID{
	std::string name;					/* name */
	am_sourceClass_t sourceClassID;		/* sourceClassID */
}ST_PEEKSOURCECLASSID;

/* peekSinkClassID */
typedef struct _ST_PEEKSINKCLASSID{
	std::string name;					/* name */
	am_sinkClass_t sinkClassID;			/* sinkClassID */
}ST_PEEKSINKCLASSID;

/* hookInterruptStatusChange */
typedef struct _ST_HOOKINTERRUPTSTATUSCHANGE{
	am_sourceID_t sourceID;				/* sourceID */
	am_InterruptState_e interruptState;	/* interruptState */
}ST_HOOKINTERRUPTSTATUSCHANGE;

/* hookDomainRegistrationComplete */
typedef struct _ST_HOOKDOMAINREGISTRATIONCOMPLETE{
	am_domainID_t domainID;				/* domainID */
}ST_HOOKDOMAINREGISTRATIONCOMPLETE;

/* hookSinkAvailablityStatusChange */
typedef struct _ST_HOOKSINKAVAILABLITYSTATUSCHANGE{
	am_sinkID_t sinkID;					/* sinkID */
	am_Availability_s availability;		/* availability */
}ST_HOOKSINKAVAILABLITYSTATUSCHANGE;

/* hookSourceAvailablityStatusChange */
typedef struct _ST_HOOKSOURCEAVAILABLITYSTATUSCHANGE{
	am_sourceID_t sourceID;				/* sourceID */
	am_Availability_s availability;		/* availability */
}ST_HOOKSOURCEAVAILABLITYSTATUSCHANGE;

/* hookDomainStateChange */
typedef struct _ST_HOOKDOMAINSTATECHANGE{
	am_domainID_t domainID;				/* domainID */
	am_DomainState_e domainState;		/* domainState */
}ST_HOOKDOMAINSTATECHANGE;

/* hookTimingInformationChanged */
typedef struct _ST_HOOKTIMINGINFORMATIONCHANGED{
	am_connectionID_t connectionID;		/* connectionID */
	am_timeSync_t delay;				/* delay */
}ST_HOOKTIMINGINFORMATIONCHANGED;

/* sendChangedData */
typedef struct _ST_SENDCHANGEDDATA{
	am_EarlyData_s earlyData;			/* delay */
}ST_SENDCHANGEDDATA;

/* confirmRoutingReady */
typedef struct _ST_CONFIRMROUTINGREADY{
	uint16_t handle;					/* handle値 */
	am_Error_e error;					/* 処理結果 */
}ST_CONFIRMROUTINGREADY;

/* confirmRoutingRundown */
typedef struct _ST_CONFIRMROUTINGRUNDOWN{
	uint16_t handle;					/* handle値 */
	am_Error_e error;					/* 処理結果 */
}ST_CONFIRMROUTINGRUNDOWN;

/* updateGateway */
typedef struct _ST_UPDATEGATEWAY{
	am_gatewayID_t gatewayID;						/* gatewayID */
	am_CustomConnectionFormat_t listSourceFormats;	/* listSourceFormats */
	am_CustomConnectionFormat_t listSinkFormats;	/* listSinkFormats */
	bool convertionMatrix;							/* convertionMatrix */
}ST_UPDATEGATEWAY;

/* updateSink */
typedef struct _ST_UPDATESINK{
	am_sinkID_t sinkID;					/* sinkID */
	am_sinkClass_t sinkclassID;			/* sinkclassID */
	am_SoundProperty_s listsoundproperties[D_AM_UPDATESINK_LISTNUM];				/* SoundProperty */
	am_CustomConnectionFormat_t listconnectionformats[D_AM_UPDATESINK_LISTNUM];	/* 接続音声形式 */
	am_MainSoundProperty_s listmainsoundproperties[D_AM_UPDATESINK_LISTNUM];		/* MainSoundProperty */
	am_Error_e error;					/* 処理結果 */
}ST_UPDATESINK;

/* updateSource */
typedef struct _ST_UPDATESOURCE{
	am_sourceID_t sourceID;				/* sourceID */
	am_sourceClass_t sourceclassID;		/* sourceclassID */
	am_SoundProperty_s listsoundproperties;				/* SoundProperty */
	am_CustomConnectionFormat_t listconnectionformats;	/* 接続音声形式 */
	am_MainSoundProperty_s listmainsoundproperties;		/* MainSoundProperty */
}ST_UPDATESOURCE;

/* updateConverter */
typedef struct _ST_UPDATECONVERTER{
	am_converterID_t converterID;		/* sourceID */
	am_CustomConnectionFormat_t listSourceFormats;		/* SourceFormat */
	am_CustomConnectionFormat_t listSinkFormats;		/* SinkFormat */
	bool convertionMatrix;								/* convertionMatrix */
}ST_UPDATECONVERTER;

/* ackSetVolumes */
typedef struct _ST_ACKSETVOLUMES{
	am_Handle_s handle;									/* handleデータ */
	am_Volumes_s listvolumes[D_AM_VOLUMES_LISTNUM];		/* Volumes */
	am_Error_e error;									/* 処理結果 */
}ST_ACKSETVOLUMES;

/* ackSinkNotificationConfiguration */
typedef struct _ST_ACKSINKNOTIFICATIONCONFIGURATION{
	am_Handle_s handle;					/* handleデータ */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSINKNOTIFICATIONCONFIGURATION;

/* ackSourceNotificationConfiguration */
typedef struct _ST_ACKSOURCENOTIFICATIONCONFIGURATION{
	am_Handle_s handle;					/* handleデータ */
	am_Error_e error;					/* 処理結果 */
}ST_ACKSOURCENOTIFICATIONCONFIGURATION;

/* hookSinkNotificationDataChange */
typedef struct _ST_HOOKSINKNOTIFICATIONDATACHANGE{
	am_sinkID_t sinkid;					/* sinkID */
	am_NotificationPayload_s payload;	/* PayloadData */
}ST_HOOKSINKNOTIFICATIONDATACHANGE;

/* hookSourceNotificationDataChange */
typedef struct _ST_HOOKSOURCENOTIFICATIONDATACHANGE{
	am_sourceID_t sourceid;				/* sourceID */
	am_NotificationPayload_s payload;	/* PayloadData */
}ST_HOOKSOURCENOTIFICATIONDATACHANGE;

/* getInterfaceVersion */
typedef struct _ST_GETINTERFACEVERSION{
	std::string version;				/* version */
}ST_GETINTERFACEVERSION;

/* getRoutingReady */
typedef struct _ST_GETROUTINGREADY{
	bool routingReady;					/* 処理結果 */
}ST_GETROUTINGREADY;



/* ソケットで転送するデータ（各機能ごとに構造体を作成し配置する) */
typedef union
{
	/* PluginからClientへ送信 */
	ST_SETROUTINGREADY st_setRoutingReady;
	ST_SETROUTINGRUNDOWN st_setRoutingRundown;
	ST_ASYNCABORT st_asyncAbort;
	ST_ASYNCCONNECT st_asyncConnect;
	ST_ASYNCDISCONNECT st_asyncDisconnect;
	ST_ASYNCSETSINKVOLUME st_asyncSetSinkVolume;
	ST_ASYNCSETSOURCEVOLUME st_asyncSetSourceVolume;
	ST_ASYNCSETSOURCESTATE st_asyncSetSourceState;
	ST_ASYNCSETSINKSOUNDPROPERTIES st_asyncSetSinkSoundProperties;
	ST_ASYNCSETSINKSOUNDPROPERTY st_asyncSetSinkSoundProperty;
	ST_ASYNCSETSOURCESOUNDPROPERTIES st_asyncSetSourceSoundProperties;
	ST_ASYNCSETSOURCESOUNDPROPERTY st_asyncSetSourceSoundProperty;
	ST_ASYNCSETVOLUMES st_asyncSetVolumes;
	ST_ASYNCSETSINKNOTIFICATIONCONFIGURATION st_asyncSetSinkNotifiConf;
	ST_ASYNCSETSOURCENOTIFICATIONCONFIGURATION st_asyncSetSourceNotifiConf;

	/* ClientからPluginへ送信(不足分は後で追加) */
	ST_GETROUTINGREADY st_getRoutingReady;
	ST_ACKCONNECT st_ackConnect;
	ST_ACKDISCONNECT st_ackDisconnect;
	ST_ACKSETSINKVOLUMECHANGE st_ackSetSinkVolumeChange;
	ST_ACKSETSOURCESTATE st_ackSetSourceState;
	ST_REGISTERDOMAIN st_registerDomain;
	ST_HOOKDOMAINREGISTRATIONCOMPLETE st_hookDomainRegistrationComplete;
	ST_CONFIRMROUTINGREADY st_confirmRoutingReady;
	ST_UPDATESINK st_updateSink;
	ST_ACKSINKNOTIFICATIONCONFIGURATION st_ackSinkNotifiConf;

}UN_SEND_DATA;

/* ソケットで転送するヘッダ */
typedef struct _ST_SOCKET_MSG
{
	unsigned long escm_heada;	/*'E''S''C''M'*/
	unsigned short datasize;	/* Source data size */
	unsigned short srcaddr;		/* Source address */
	unsigned short destaddr;	/* Forwarding address */
	unsigned short opc;			/* opc */
	UN_SEND_DATA data;
} ST_SOCKET_MSG;

/* 受信データ振り分け関数用Callback(インターフェースクラス) */
class IRecieveCallBack
{
public:
	virtual void Call(void *pmsg) = 0;
	virtual ~IRecieveCallBack() {};
};

/* 受信データ振り分け関数用Callback(本体) */
template<class TClass> class TRecieveCallBack : public IRecieveCallBack
{
private:
	TClass *mInstance;
    void (TClass::*mFunction)(void *pmsg);
public:
	TRecieveCallBack(TClass *instance, void (TClass::*function)(void *pmsg)) :
					mInstance(instance),
					mFunction(function) {};
	virtual void Call(void *pmsg)
	{
		(*mInstance.*mFunction)(pmsg);
	}
};

/* CallBack関数の登録方法                                                                             */
/* 宣言：Txxx<CallBack関数を持っているクラス名> pxxx;                                                 */
/* 変数初期化：pxxx(CallBack関数を持つクラスのインスタンスポインタ, メソッドのポインタ:&xxx::yyyの形) */
/* 登録時:pxxxを登録関数の引数に渡し、登録関数のクラスメンバへ保存                                    */
/* 使用時：pxxx->Call(引数)でCallback関数を呼び出す                                                   */

/**
 * This wraps socket and provides everything needded to anyone who want to use socket (including plugins).
 */
class CAmSocketWrapper
{
public:
	//コンストラクタ・デストラクタ
	CAmSocketWrapper( SOCKET_USED_TYPE usedtype, SOCKET_TYPE type );
	virtual ~CAmSocketWrapper();
	//コールバックメソッド
	void registerCallback(IRecieveCallBack *recieveCb);												/* 受信データ振り分けコールバック		*/
	void socketAcceptCallback(const pollfd pollfd, const sh_pollHandle_t handle, void* userData);	/* Accept通知コールバック				*/
	bool socketDispatchCallback(const sh_pollHandle_t handle, void* userData);						/* Dispatch通知コールバック				*/
	void socketFireCallback(const pollfd pollfd, const sh_pollHandle_t handle, void* userData);		/* イベント発火通知コールバック			*/
	bool socketCheckCallback(const sh_pollHandle_t handle, void* userData);							/* チェック結果通知コールバック			*/
	//コールバックポインタ
	TAmShPollFired<CAmSocketWrapper>	pSocketAcceptCallback;										/* Accept通知コールバックポインタ		*/
	TAmShPollDispatch<CAmSocketWrapper>	pSocketDispatchCallback;									/* Dispatch通知コールバックポインタ		*/
	TAmShPollFired<CAmSocketWrapper>	pSocketFireCallback;										/* イベント発火通知コールバックポインタ	*/
	TAmShPollCheck<CAmSocketWrapper>	pSocketCheckCallback;										/* チェック結果通知コールバックポインタ	*/
	//処理メソッド
	bool requestAddFDpoll( CAmSocketHandler*& socketHandler );										/* passive socket登録依頼		*/
	bool requestConnect( unsigned short destaddr = D_AM_DOMAIN_ADDRESS_AM);							/* Connect要求依頼				*/
	bool socketInitialize( int *pSock );															/* リスニングSocket初期処理		*/
	bool socketCreate( int *pSock );																/* Socket生成					*/
	bool checkSocketFD( void );																		/* SocketFD生成確認				*/
	void socketClose( void );																		/* Socket解放					*/
	void socketCloseforSingleFd( FD_TYPE_SOCKET type, int fd );										/* SocketRelease(per FileDescrepter	*/
	bool socketReConnect( int AcceptFD );															/* Socket再接続					*/
	int getFileDescripter( FD_TYPE_SOCKET type, unsigned short destaddr = D_AM_DOMAIN_ADDRESS_AM);	/* FileDiscripter値取得 		*/
	void setSocketSetting( SOCKET_USED_TYPE usedtype, SOCKET_TYPE type );							/* Socket通信確立用アドレス・ポートの設定 */
	unsigned short getSrcAddress( void );															/* 転送元論理アドレス取得		*/
	void notificationRecvDest( unsigned short destaddr );											/* Notification receive of source address	*/
	int getCurrentAcceptFD( void );																	/* Get processing AcceptFD	*/

private:
	/* Management table for FD used at Connect and FD with Acccept */
	typedef struct _stFileDescripterTbl
	{
		const char *	ConnectAddress;		/* address specified at connect							*/
		unsigned short	ConnectPort;		/* port specified at connect							*/
		int				AcceptFD;			/* FD created by accept									*/
		int 			ConnectFD;			/* FD created by socket for connect						*/
		sh_pollHandle_t	AcceptPollHandle;	/* poll handle for Accept								*/
	}stFileDescripterTbl;

	/* Management temporary table before tying with Forwarding address */
	typedef struct _stAcceptFDTempTbl
	{
		int				AcceptFD;			/* change notification by 'listen->poll' > FD created by accept	*/
		sh_pollHandle_t	AcceptPollHandle;	/* poll handle for Accept										*/
		unsigned short	padding;			/* padding 														*/
	}stAcceptFDTempTbl;
	
	typedef std::map<unsigned short, stFileDescripterTbl> FileDescripterMap_t;
	typedef std::list<stAcceptFDTempTbl> listTempAcceptFD_t;

	ST_SOCKET_MSG		mMsg;			 		/* 受信メッセージバッファ								*/
	int					mRecvDatalen;
	CAmSocketHandler	*mpSocketHandler; 		/* pointer to the sockethandler							*/
	sh_pollHandle_t		mListenPollHandle;		/* poll handle for Listen								*/
	const char *		mListenAddress;			/* address specified at listen							*/
	unsigned short		mListenPort;			/* port specified at listen								*/
	int					mListenFD; 				/* socket->bind->litenによって生成されたFD				*/
	FileDescripterMap_t	mFileDescripterMap;		/* map for managing AcceptFD and ConnectFD				*/
	listTempAcceptFD_t	mTempAcceptFD;			/* Temporary save table for AcceptFD					*/
	unsigned short		mSrcAddress;			/* 転送元論理アドレス									*/
	SOCKET_USED_TYPE	mSocketUsedType;		/* Domain type using socket								*/
	bool				mReConnectFlag;			/* Reconnect Flag										*/
	IRecieveCallBack*	mpRecievCallback;		/* 受信メッセージ振り分け用Callback関数					*/
	int					mCurrentAcceptFD;		/* save AcceptFD processed recieve one now				*/
};

}

#endif /* SOCKETWRAPPER_H_ */
