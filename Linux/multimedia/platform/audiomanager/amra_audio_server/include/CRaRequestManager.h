/************************************************************************
* @file: CRaRequestManager.h
*
* @version: 1.1
*
* CRaRequestManager class serialize the requests from the Audio Manager
* 
* @component: platform/audiomanager
*
* @author: Nrusingh Dash <ndash@jp.adit-jv.com>
*
* @copyright (c) 2010, 2011 Advanced Driver Information Technology.
* This code is developed by Advanced Driver Information Technology.
* Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
* All rights reserved.
*
* @see <related items>
*
* @history
*
***********************************************************************/
#ifndef __C_RA_REQUEST_MANAGER_H__
#define __C_RA_REQUEST_MANAGER_H__
#include "audio/rpcontroller/idl/IasAudioRoutingStub.hpp"
#include "audio/rpcontroller/idl/IasAudioProcessingStub.hpp"
#include "CRaASClient.h"
#include "CContextManager.h"

typedef enum {
	eREQUEST_TYPE_UNKNOWN = 0,
	eREQUEST_TYPE_SET_ROUTING_READY = 1,
	eREQUEST_TYPE_SET_ROUTING_RUNDOWN = 2,
	eREQUEST_TYPE_CONNECT = 3,
	eREQUEST_TYPE_DISCONNECT = 4,
	eREQUEST_TYPE_VOLUME = 5,
	eREQUEST_TYPE_MUTE_STATE = 6,
	eREQUEST_TYPE_BALANCE = 7,
	eREQUEST_TYPE_FADER = 8,
	eREQUEST_TYPE_EQUALIZATION = 9,
	eREQUEST_TYPE_LOUDNESS = 10,
	eREQUEST_TYPE_SCV = 11,
	eREQUEST_TYPE_INPUTGAINOFFSET = 12,
	eREQUEST_TYPE_SET_SOURCE_STATE = 13,
	eREQUEST_TYPE_ADD_HOTPLUG_SOURCE = 14,
	eREQUEST_TYPE_ADD_HOTPLUG_SINK = 15,
	eREQUEST_TYPE_REMOVE_HOTPLUG_SOURCE = 16,
	eREQUEST_TYPE_REMOVE_HOTPLUG_SINK = 17,
	eREQUEST_TYPE_FORWARD_HOTPLUG_SOURCE_STATE = 18,
	eREQUEST_TYPE_UPDATE_HOTPLUG_SINK_ALSA_PARAMS = 19,
	eREQUEST_TYPE_UPDATE_HOTPLUG_SOURCE_ALSA_PARAMS = 20,
	eREQUEST_TYPE_HOTPLUG_SOURCE_INTERRUPT_STATUS_CHANGE = 21,
	eREQUEST_TYPE_HOTPLUG_SOURCE_AVAILABILITY_CHANGE = 22,
	eREQUEST_TYPE_HOTPLUG_SINK_AVAILABILITY_CHANGE = 23,
	eREQUEST_TYPE_HOTPLUG_SOURCE_NOTIF_DATA_CHANGE = 24,
	eREQUEST_TYPE_HOTPLUG_SINK_NOTIF_DATA_CHANGE = 25,
	eREQUEST_TYPE_HOTPLUG_SOURCE_UPDATE = 26,
	eREQUEST_TYPE_HOTPLUG_SINK_UPDATE = 27,
	eREQUEST_TYPE_MAX
} RequestType_e;

typedef enum {
	eREQUEST_STATE_UNKNOWN = 0,
	eREQUEST_STATE_NEW = 1,
	eREQUEST_STATE_SENT_TO_AS = 2,
	eREQUEST_STATE_RESPONSE_OK = 3,
	eREQUEST_STATE_RESPONSE_ERROR = 4,
	eREQUEST_STATE_EVENT_OK = 5,
	eREQUEST_STATE_EVENT_ERROR = 6,
	eREQUEST_STATE_MAX
} RequestState_e;

struct ConnectionData {
	am_connectionID_t				am_connection_id;
	am_sourceID_t					am_source_id;
	Ias::Int32					as_source_id;
	IasAudioRouting::IasConnectionState		as_connection_state;
};

struct VolumeData {
	am_volume_t					am_volume;
	am_CustomRampType_t					am_ramp;
	am_time_t					am_time;
	Ias::Int32					as_volume;
	IasAudioProcessing::IasRampShape		as_ramp;
	Ias::UInt32					as_time;
};

struct MuteStateData {
	am_MuteState_e					am_mute_state;
	am_CustomRampType_t					am_ramp;
	am_time_t					am_time;
	IasAudioProcessing::IasMuteState		as_mute_state;
	IasAudioProcessing::IasRampShape		as_ramp;
	Ias::UInt32					as_time;
};

struct BalanceData {
	int16_t						am_balance;
	Ias::Int32					as_balance;
};

struct FaderData {
	int16_t						am_fader;
	Ias::Int32					as_fader;
};

struct EqualizerData {
	uint16_t					am_band;
	int16_t						am_gain;
	Ias::UInt32					as_band;
	Ias::Int32					as_gain;
};

struct LoudnessData {
	int16_t						am_loudness;
	Ias::Bool					as_loudness;
};

struct SCVData {
	int16_t						am_scv;
	Ias::UInt32					as_scv;
};

struct InputGainOffsetData {
	int16_t						am_input_gain_offset;
	Ias::Int32					as_input_gain_offset;
};

struct SourceInterruptStateData {
	am::am_sourceID_t	sourceID;
	am::am_InterruptState_e		sourceInterruptState;
};

struct SourceAvailabilityData {
	am::am_sourceID_t	sourceID;
	am::am_Availability_s		availability;
};

struct SourceNotificationData {
	am::am_sourceID_t	sourceID;
	am::am_NotificationPayload_s notifPayload;
};

struct SinkAvailabilityData {
	am::am_sinkID_t	sinkID;
	am::am_Availability_s		availability;
};

struct SinkNotificationData {
	am::am_sinkID_t	sinkID;
	am::am_NotificationPayload_s notifPayload;
};

struct SourceStateData {
	am::am_sourceID_t	sourceID;
	am::am_SourceState_e sourceState;
};

struct UpdateSourceData {
	am::am_sourceID_t sourceID;
	am::am_sourceClass_t sourceClassID;
	std::vector<am::am_SoundProperty_s> listSoundProperties;
	std::vector<am::am_CustomConnectionFormat_t> listConnectionFormats;
	std::vector<am::am_MainSoundProperty_s> listMainSoundProperties;
};

struct UpdateSinkData {
	am::am_sinkID_t sinkID;
	am::am_sinkClass_t sinkClassID;
	std::vector<am::am_SoundProperty_s> listSoundProperties;
	std::vector<am::am_CustomConnectionFormat_t> listConnectionFormats;
	std::vector<am::am_MainSoundProperty_s> listMainSoundProperties;
};

union RequestData {
	ConnectionData				connection;
	VolumeData				volume;
	MuteStateData				mutestate;
	BalanceData				balance;
	FaderData				fader;
	EqualizerData				equalizer;
	LoudnessData				loudness;
	SCVData					scv;
	InputGainOffsetData			inputgain;
	am::am_Source_s				*source;
	am::am_Sink_s				*sink;
	am::am_sourceID_t			sourceID;
	am::am_sinkID_t				sinkID;
	SourceStateData				sourcestate;
	SourceInterruptStateData	sourceIntStateData;
	SourceAvailabilityData		sourceAvailabilityData;
	SourceNotificationData		sourceNotifationData;
	SinkAvailabilityData		sinkAvailabilityData;
	SinkNotificationData		sinkNotificationData;
	UpdateSourceData			*sourceUpdateData;
	UpdateSinkData				*sinkUpdateData;
};

struct Request {
	am_Handle_s					handle;
	RequestType_e					type;
	RequestState_e 					state;
	am_sinkID_t					am_sink_id;
	Ias::Int32					as_sink_id;
	RequestData					data;

};

am::am_Error_e getAlsaParamsFromSoundProperties(const std::vector<am_SoundProperty_s> &listSoundProperties, AlsaParams_s &alsaParams);

class CRaRequestManager {
public:
	CRaRequestManager(CRaASClient *asClient, CRaAMRoutingClient *amClient, CRaConfigManager *configManager, CContextManager *contextManager);
	~CRaRequestManager();
	void EnqueueRequest(Request * request);
	void RemoveRequest(const am_Handle_s handle);
	void DequeueRequest(void);
	Request *PeekRequestQueue(void);
	void SetRequestState(RequestState_e state);
	void SetAmClient(CRaAMRoutingClient *amClient);
	void SetHotplugSender(CRaHotplugSender *hotplugSender);
	void AddDelayedRequest(Request * request);
	void ProcessDelayedRequests(am_sinkID_t sink);
	void RemoveDelayedRequests(am_sinkID_t sink);
private:
	/* we do not want to instanciate the object with the default constructor */
	CRaRequestManager();
	RequestState_e GetRequestState();
	void SendRequestToAS(void);
	void SendDummyAckToAM(Request *request);
	void ProcessRequest(void);
        am::am_Error_e registerDomainData(const uint16_t handle);
	am::am_Error_e unregisterDomainData(const uint16_t handle);
	am::am_Error_e addHotplugSource(am::am_Source_s *source);
	am::am_Error_e addHotplugSink(am::am_Sink_s *sink);
	am::am_Error_e removeHotplugSource(am::am_sourceID_t sourceID);
	am::am_Error_e removeHotplugSink(am::am_sinkID_t sinkID);
	am::am_Error_e updateHotplugSinkAlsaParameters(am::am_sinkID_t sinkID);
	am::am_Error_e updateHotplugSourceAlsaParameters(am::am_sourceID_t sourceID);
	static void *RequestManagerThreadHandler(void *args)
	{
		CRaRequestManager *requestManager = static_cast <CRaRequestManager *> (args);
		requestManager->ProcessRequest();
		pthread_exit(NULL);
		return 0;
	}
private:
	std::list<Request *>				mRequestQueue;
	CRaASClient					*mAsClient;
	CRaAMRoutingClient				*mAmClient;
	CRaConfigManager				*mConfigManager;
	pthread_t					mRequestManagerThread;
	pthread_mutex_t					mLock;
	pthread_cond_t					mCond;
	std::map<am_sinkID_t, std::map<RequestType_e, Request> > mDelayedRequest;
	CContextManager					*mContextManager;
	CRaHotplugSender				*mHotplugSender;
};

#endif //__C_RA_REQUEST_MANAGER_H__
