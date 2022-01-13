/************************************************************************
 * @file: CRaFuncBlock.cpp
 *
 * @version: 1.1
 *
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *
 * @copyright (c) 2010, 2011 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include "CRaFuncBlock.h"
#include "CAmDltWrapper.h"
#include "CRaMockSender.h"

using namespace am;
using namespace std;


const std::string strVolState[3] = { "READY", "UP", "DOWN" };
const std::string strMuteState[3] = { "ON", "OFF", "RUNNING" };


am_volume_t convertStep(unsigned char val)
{
    return (static_cast<am_volume_t>(val) * 100) + AM_MUTE;
};

am_connectionID_t getRoute(const unsigned char *data)
{
    return reinterpret_cast<am_connectionID_t>(getU16(data));
};

unsigned char getStream(am_connectionID_t id)
{
    return static_cast<unsigned char>(id >> 8);
};

unsigned char getSource(am_connectionID_t id)
{
    return static_cast<unsigned char>(id);
};


FBFunc::FBFunc(IAmRoutingSend * sender, unsigned short id, std::string name) :
            mSender(NULL), mFuncId(id), mName(name), mLength(0), mData(NULL),
            mStreams()
{
    if (sender != NULL)
    {
        mSender = sender;
    }
}

FBFunc::~FBFunc()
{
    if (mData)
    {
        delete[] mData;
    }
}

void FBFunc::OperationSet(const unsigned char* msg)
{
    logInfo(mName + "::Set");
    Set(getU16(&msg[offsetof(t_msg, length)]), &msg[offsetof(t_msg, data)]);
}

void FBFunc::OperationGet(const unsigned char* msg)
{
    logInfo(mName + "::Get");
    Get(getU16(&msg[offsetof(t_msg, length)]), &msg[offsetof(t_msg, data)]);
}

void FBFunc::OperationSetGet(const unsigned char *msg)
{
    logInfo(mName + "::SetGet");
    Set(getU16(&msg[offsetof(t_msg, length)]), &msg[offsetof(t_msg, data)], false);
}

void FBFunc::AcknowledgeCommand(const am_Handle_e type, const void* data, const am_Error_e error)
{
    Acknowledge(type, data, error);
}

void FBFunc::SetSender(IRaMockSender* receiver)
{
    mReceiver = receiver;
}

unsigned short FBFunc::GetId(void)
{
    return mFuncId;
}

void FBFunc::SendStatus(unsigned short length, const unsigned char* data)
{
    logInfo(mName + "::SendStatus");
    SendMsg(OP_TYPE_STATUS, length, data);
}

void FBFunc::SendError(eErrCode code, unsigned char info)
{
    logInfo(mName + "::SendError");
    unsigned char msg[2];
    msg[0] = static_cast<unsigned char>(code);
    msg[1] = info;
    SendMsg(OP_TYPE_ERROR, sizeof(msg), msg);
}

void FBFunc::Set(unsigned short length, const unsigned char* data, bool isSet)
{
    (void) isSet;
    mLength = length;
    if (mData)
    {
        delete[] mData;
    }
    mData = new unsigned char[mLength];
    memcpy(mData, data, length);
    SendStatus(mLength, mData);
}

void FBFunc::Get(unsigned short length, const unsigned char* data)
{
    (void) length;
    (void) data;
    SendStatus(mLength, mData);
}

void FBFunc::Acknowledge(const am_Handle_e type, const void* data, const am_Error_e error)
{
    (void) type;
    (void) data;
    (void) error;
}

void FBFunc::SendMsg(unsigned char opType, unsigned short length, const unsigned char* data)
{
    unsigned char * msg = new unsigned char[MSG_HEADER_SIZE + length];

    setU16(MOST_CLIENT_ID, &msg[offsetof(t_msg, clientId)]);
    setU16(MOST_FBLOCK_ID, &msg[offsetof(t_msg, fblock)]);
    msg[offsetof(t_msg, instId)] = MOST_INST_ID;
    setU16(mFuncId, &msg[offsetof(t_msg, funcId)]);
    msg[offsetof(t_msg, opType)] = opType;
    setU16(length, &msg[offsetof(t_msg, length)]);
    memcpy(&msg[offsetof(t_msg, data)], data, length);

    mReceiver->sendData(msg, MSG_HEADER_SIZE + length);

    delete[] msg;
}

/** Section FBPowerState */
void FBPowerState::Set(unsigned short length, const unsigned char* data, bool isSet)
{
    (void) length;
    if (mOpState != static_cast<eOpState>(data[0]))
    {
        mOpState = static_cast<eOpState>(data[0]);
        if (mOpState == STATE_ON)
        {
            mSender->setRoutingReady((u_int16_t) 0xffff);
        }
        else if (mOpState == STATE_OFF)
        {
            mSender->setRoutingRundown((u_int16_t) 0xffff);
        }
        logInfo(mName, getOpState());
    }
    if (!isSet)
    {
        Get(length, data);
    }
}

void FBPowerState::Get(unsigned short length, const unsigned char* data)
{
    (void) length;
    (void) data;
    unsigned char state = static_cast<unsigned char>(mOpState);
    SendStatus(sizeof(mOpState), &state);
}

std::string FBPowerState::getOpState(void)
{
    std::string state;
    switch (mOpState)
    {
        case STATE_ON:
            state = "ON";
            break;
        case STATE_OFF:
            state = "OFF";
            break;
        case STATE_STANDBY:
            state = "STANDBY";
            break;
        default:
            state = "UNDEFINED";
            break;
    }
    return state;
}

/** Section FBAudioRoute */
void FBAudioRoute::Set(unsigned short length, const unsigned char* data, bool isSet)
{
    (void) isSet;
    (void) length;
    unsigned char stream = data[0];
    unsigned char source = data[1];
    mNewConnection = getRoute(data);
    if (mStreams[stream].route != 0)
    {
        am_Handle_s handle = { H_DISCONNECT, mFuncId };
        mSender->asyncDisconnect(handle, mStreams[stream].route);
    }
    else
    {
        am_Handle_s handle = { H_CONNECT, mFuncId };
        mSender->asyncConnect(handle, mNewConnection, source, AMPLIFIER, CF_GENIVI_STEREO);
    }
}

void FBAudioRoute::Get(unsigned short length, const unsigned char* data)
{
    (void) length;
    unsigned char msg[2];
    msg[0] = mStreams[data[0]].route;
    msg[1] = mStreams[data[0]].source;
    SendStatus(sizeof(msg), msg);
}

void FBAudioRoute::Acknowledge(const am_Handle_e type, const void *data, const am_Error_e error)
{
    const am_connectionID_t connection = *(reinterpret_cast<const am_connectionID_t*>(data));
    if (type == H_DISCONNECT)
    {
        if (error == E_OK)
        {
            am_Handle_s handle = { H_CONNECT, mFuncId };
            mSender->asyncConnect(handle, mNewConnection, getSource(mNewConnection), AMPLIFIER, CF_GENIVI_STEREO);
        }
        else
        {
            SendError(ERR_PARAM_OUT_OF_RANGE);
        }
    }
    else
    {
        if (error == E_OK)
        {
            unsigned char msg[2];
            mStreams[getStream(connection)].route = connection;
            mStreams[getStream(connection)].source = getSource(connection);
            setU16(connection, msg);
            SendStatus(sizeof(msg), msg);
        }
        else
        {
            SendError(ERR_PARAM_OUT_OF_RANGE);
        }
    }
}

/** Section FBVolume */
void FBVolume::Set(unsigned short length, const unsigned char* data, bool isSet)
{
    (void) length;
    (void) isSet;
    am_Handle_s handle = { H_SETSOURCEVOLUME, mFuncId };

    mVolume.Stream = data[offsetof(t_VolListSet, Stream)];
    mVolume.vol.Step = data[offsetof(t_VolListSet, vol.Step)];
    mVolume.vol.dB = getU16(&data[offsetof(t_VolListSet, vol.dB)]);
    mVolume.ramp.time = getU16(&data[offsetof(t_VolListSet, ramp.time)]);

    /* convert for comparison */
    mAmVolume = convertStep(mVolume.vol.Step);

    _StreamSource *pStream = &mStreams[mVolume.Stream];

    /* if route is configure or volume is equal... */
    if (pStream->route == 0)
    {
        pStream->volStep = mVolume.vol.Step;
        SendVolumeStatus(mAmVolume, mVolume.Stream);
        return;
    }

    if (pStream->volStep == mVolume.vol.Step)
    {
        /* then return immediately with pass */
        SendVolumeStatus(mAmVolume, mVolume.Stream);
        return;
    }

    mSender->asyncSetSourceVolume(handle, pStream->source, mAmVolume, RAMP_GENIVI_EXP_INV,
            mVolume.ramp.time / 1000);
}

void FBVolume::Get(unsigned short length, const unsigned char* data)
{
    (void) length;
    SendVolumeStatus(mAmVolume, data[offsetof(t_VolListStatus, Stream)]);
}

void FBVolume::Acknowledge(const am_Handle_e type, const void *data, const am_Error_e error)
{
    (void) type;
    am_volume_t volume = *(reinterpret_cast<const am_volume_t*>(data));
    if (error == E_OK)
    {
        _StreamSource *pStream = &mStreams[mVolume.Stream];
        if (volume < mAmVolume)
        {
            pStream->volState = RAMP_UP;
        }
        else if (volume > mAmVolume)
        {
            pStream->volState = RAMP_DOWN;
        }
        else //(volume == mAmVolume)
        {
            pStream->volStep = mVolume.vol.Step;
            pStream->volState = RAMP_READY;
        }
        SendVolumeStatus(volume, mVolume.Stream);
    }
    else if (error != E_ABORTED)
    {
        SendError(ERR_PARAM_OUT_OF_RANGE);
    }
    else
    {
        /* nothing to do */
    }
}

void FBVolume::SendVolumeStatus(am_volume_t volume, unsigned char stream)
{
    _StreamSource *pStream = &mStreams[stream];
    unsigned char msg[sizeof(t_VolListStatus)];
    msg[offsetof(t_VolListStatus, Stream)] = stream;
    msg[offsetof(t_VolListStatus, vol.Step)] = pStream->volStep;
    set16(mVolume.vol.dB, &msg[offsetof(t_VolListStatus, vol.dB)]);
    msg[offsetof(t_VolListStatus, vol.State)] = static_cast<unsigned char>(pStream->volState);
    SendStatus(sizeof(t_VolListStatus), msg);
    logInfo(mName + "::SendVolumeStatus AMVolume:", volume, "State:", strVolState[pStream->volState]);
}

/** Section FBVolumeOffset */
void FBVolumeOffset::Set(unsigned short length, const unsigned char* data, bool isSet)
{
    (void) isSet;
    memcpy(&mStatus, data, length);
    Get(length, data);
}

void FBVolumeOffset::Get(unsigned short length, const unsigned char* data)
{
    (void) length;
    (void) data;
    mStatus.u.OffsetStatus = RAMP_READY;
    SendStatus(offsetof(tStatus, u.OffsetStatus), reinterpret_cast<unsigned char*>(&mStatus));
}

/** Section FBMute */
void FBMute::Set(unsigned short length, const unsigned char* data, bool isSet)
{
    (void) length;
    (void) isSet;
    am_Handle_s handle = { H_SETSOURCEVOLUME, mFuncId };

    mMuteListSet.Stream = data[offsetof(t_MuteListSet, Stream)];
    mMuteListSet.Action = data[offsetof(t_MuteListSet, Action)];
    mMuteListSet.ramp.time = getU16(&data[offsetof(t_MuteListSet, ramp.time)]);

    _StreamSource *pStream = &mStreams[mMuteListSet.Stream];

    /* if route is not configured initialize state and send answer directly */
    if (pStream->route == 0)
    {
        pStream->muteState = (mMuteListSet.Action == MUTE_ACTIVATE) ? MUTE_ON : MUTE_OFF;
        SendMuteStatus(mMuteListSet.Stream);
        return;
    }
    /* if action leads into current state send current state directly */
    if ((mMuteListSet.Action == MUTE_ACTIVATE && pStream->muteState == MUTE_ON)
            || (mMuteListSet.Action == MUTE_DEACTIVATE && pStream->muteState == MUTE_OFF))
    {
        SendMuteStatus(mMuteListSet.Stream);
        return;
    }

    mSender->asyncSetSourceVolume(handle, pStream->source,
            (mMuteListSet.Action == MUTE_ACTIVATE) ? AM_MUTE : convertStep(pStream->volStep), RAMP_GENIVI_EXP_INV,
            mMuteListSet.ramp.time / 1000);
}

void FBMute::Get(unsigned short length, const unsigned char* data)
{
    (void) length;
    SendMuteStatus(data[offsetof(t_MuteListState, Stream)]);
}

void FBMute::Acknowledge(const am_Handle_e type, const void *data, const am_Error_e error)
{
    (void) type;
    const am_volume_t volume = *(reinterpret_cast<const am_volume_t*>(data));
    if (error == E_OK)
    {
        _StreamSource *pStream = &mStreams[mMuteListSet.Stream];
        if (volume == AM_MUTE)
        {
            pStream->muteState = MUTE_ON;
        }
        else if (volume == convertStep(pStream->volStep))
        {
            pStream->muteState = MUTE_OFF;
        }
        else // (volume != convertStep(pStream->volStep))
        {
            if (pStream->muteState != MUTE_RUNNING)
            {
                pStream->muteState = MUTE_RUNNING;
            }
            else
            {
                return;
            }
        }
        SendMuteStatus(mMuteListSet.Stream);
    }
    else if (error != E_ABORTED)
    {
        SendError(ERR_PARAM_OUT_OF_RANGE);
    }
    else
    {
        /* nothing to do */
    }
}

void FBMute::SendMuteStatus(unsigned char stream)
{
    _StreamSource *pStream = &mStreams[stream];
    unsigned char msg[sizeof(t_MuteListState)];
    msg[offsetof(t_MuteListState, Stream)] = stream;
    msg[offsetof(t_MuteListState, State)] = pStream->muteState;
    SendStatus(sizeof(t_MuteListState), msg);
    logInfo(mName + "::SendMuteStatus", strMuteState[pStream->muteState]);
}

/** Section FBDiagnosisResult */
void FBDiagnosisResult::Get(unsigned short length, const unsigned char* data)
{
    (void) length;
    mData[0] = data[0]; /* repeat of device */
    if (data[0] == AMPLIFIER_DEVICE)
    {
        mData[1] = THERMAL_WARNING;
        mData[2] = NO_ERROR;
    }
    else
    {
        mData[1] = OPEN_WOOFER;
        mData[2] = NO_ERROR;
    }
    SendStatus(sizeof(mData), mData);
}

/** Section FBSoundConfigVersion */
void FBSoundConfigVersion::Get(unsigned short length, const unsigned char* data)
{
    enum eRespMsg { RESP_MSGS = 18 };
    enum eRespLength { RESP_LENGTH = 9 };
    unsigned char resp[RESP_MSGS][RESP_LENGTH] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x01, 0x08, 0x01, 0x0c, 0x0b, 0x99, 0xca, 0x5a, 0x42 },
        { 0x02, 0x08, 0x01, 0x0c, 0x0b, 0x77, 0xe8, 0xab, 0x01 },
        { 0x03, 0x08, 0x01, 0x0c, 0x0b, 0x60, 0x73, 0x87, 0xf1 },
        { 0x04, 0x06, 0x01, 0x0e, 0x01, 0x9f, 0x81, 0x78, 0x71 },
        { 0x05, 0x08, 0x01, 0x0c, 0x0b, 0x23, 0x66, 0x98, 0xec },
        { 0x06, 0x08, 0x01, 0x0c, 0x0b, 0x90, 0x2d, 0x87, 0xf6 },
        { 0x07, 0x08, 0x01, 0x0c, 0x0b, 0x07, 0x96, 0xed, 0xa1 },
        { 0x08, 0x08, 0x01, 0x0c, 0x0b, 0xf0, 0x89, 0x6c, 0x59 },
        { 0x09, 0x06, 0x01, 0x0e, 0x01, 0xef, 0x93, 0x4b, 0x0f },
        { 0x0a, 0x08, 0x01, 0x0c, 0x0b, 0xb8, 0xa4, 0x2a, 0x05 },
        { 0x0b, 0x06, 0x01, 0x0e, 0x01, 0x54, 0x24, 0xf4, 0xa2 },
        { 0x0c, 0x06, 0x01, 0x0e, 0x01, 0xcb, 0x00, 0x3a, 0xd1 },
        { 0x0d, 0x06, 0x01, 0x0e, 0x01, 0x09, 0xe8, 0xc2, 0xcc },
        { 0x0e, 0x08, 0x01, 0x0c, 0x0b, 0x38, 0xec, 0xe2, 0x52 },
        { 0x0f, 0x06, 0x01, 0x0e, 0x01, 0x86, 0xb1, 0x60, 0xf0 },
        { 0x10, 0x06, 0x01, 0x0e, 0x01, 0xa7, 0xa2, 0xc8, 0x46 },
        { 0x11, 0x06, 0x01, 0x0e, 0x01, 0x1c, 0x1b, 0x6f, 0x13 }
    };

    if (length == 1 && data[0] < RESP_MSGS)
    {
        SendStatus(RESP_LENGTH, resp[data[0]]);
    }
    else
    {
        logError("SoundConfigVersion response not available for:", data[0]);
    }
}
