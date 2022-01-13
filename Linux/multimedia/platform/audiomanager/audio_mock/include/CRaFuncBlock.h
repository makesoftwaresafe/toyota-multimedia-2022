/************************************************************************
 * @file: CRaFuncBlock.h
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


#ifndef CRAFUNCBLOCK_H_
#define CRAFUNCBLOCK_H_

#include <cassert>
#include <stddef.h>
#include <string.h>
#include "IAmRouting.h"


namespace am {

class IRaMockSender;

enum eVolState
{
    RAMP_READY,
    RAMP_UP,
    RAMP_DOWN
};

enum eMuteState
{
    MUTE_ON,
    MUTE_OFF,
    MUTE_RUNNING
};

#define MOST_CLIENT_ID			0x0010
#define MOST_FBLOCK_ID	        0x0091
#define MOST_INST_ID            0x01

/* FUNC_ID */
#define FUNC_ID_PING 			0x0f02
#define FUNC_ID_POST 			0x0f11
#define FUNC_ID_MODE 			0x0f12
#define FUNC_ID_SORC 			0x0f13
#define FUNC_ID_AURO 			0x0f15
#define FUNC_ID_VOLU 			0x0f20
#define FUNC_ID_VOOF 			0x0f21
#define FUNC_ID_SETT 			0x0f30
#define FUNC_ID_MUTE 			0x0f60
#define FUNC_ID_DIRE 			0x0f91
#define FUNC_ID_DISP 			0x0f92
#define FUNC_ID_SCIT 			0x0fa4
#define FUNC_ID_SCVR 			0x0fa5
#define FUNC_ID_CSET 			0x0fb0

/* OP_TYPES */
/* commands */
#define OP_TYPE_SET			    0x00
#define OP_TYPE_GET             0x01
#define OP_TYPE_SETGET          0x02
#define OP_TYPE_INC             0x03
#define OP_TYPE_DEC             0x04
#define OP_TYPE_GETIF			0x05
#define OP_TYPE_STATUS			0x0c
#define OP_TYPE_INTERFACE		0x0e
#define OP_TYPE_ERROR			0x0f

enum eErrCode
{
    ERR_FBLOCK_NOT_AVAIL      = 0x01,
    ERR_INSTANCE_NOT_AVAIL    = 0x02,
    ERR_FUNCID_NOT_AVAIL      = 0x03,
    ERR_OPTPYE_NOT_AVAIL      = 0x04,
    ERR_WRONG_LENGTH          = 0x05,
    ERR_PARAM_OUT_OF_RANGE    = 0x06,
    ERR_BUSY                  = 0x40,
    ERR_FUNC_TEMP_NOT_AVAIL   = 0x41
};

struct __attribute__((packed)) t_msg
{
    unsigned short clientId;
    unsigned short fblock;
    unsigned char instId;
    unsigned short funcId;
    unsigned char opType;
    unsigned short length;
    unsigned char *data;
};
#define MSG_HEADER_SIZE	offsetof(struct t_msg, data)


__inline short get16(unsigned char *data)
{
    return ((short) (data[0] << 8) + data[1]);
};

__inline unsigned short getU16(const unsigned char *data)
{
    return ((unsigned short) (data[0] << 8) + data[1]);
};

__inline void set16(short value, unsigned char* data)
{
    data[0] = (char) (value >> 8);
    data[1] = (char) (value);
};

__inline void setU16(unsigned short value, unsigned char* data)
{
    data[0] = (unsigned char) (value >> 8);
    data[1] = (unsigned char) (value);
};


class FBFunc
{
public:
    FBFunc(IAmRoutingSend * sender, unsigned short id, std::string name);
    virtual ~FBFunc();

    void OperationSet(const unsigned char* msg);
    void OperationGet(const unsigned char* msg);
    void OperationSetGet(const unsigned char *msg);
    void AcknowledgeCommand(const am_Handle_e type, const void* data, const am_Error_e error);

    void SetSender(IRaMockSender* receiver);
    unsigned short GetId(void);

protected:
    virtual void SendStatus(unsigned short length, const unsigned char* data);
    virtual void SendError(eErrCode code, unsigned char info = 0);

    virtual void Set(unsigned short length, const unsigned char* data, bool isSet = true);
    virtual void Get(unsigned short length, const unsigned char* data);
    virtual void Acknowledge(const am_Handle_e type, const void* data, const am_Error_e error);

protected:
    void SendMsg(unsigned char opType, unsigned short length, const unsigned char* data);

protected:
    IAmRoutingSend  *mSender;
    IRaMockSender   *mReceiver;
    unsigned short   mFuncId;
    std::string      mName;

    unsigned short   mLength;
    unsigned char*   mData;

    struct _StreamSource
    {
        _StreamSource() : route(0), source(0), volStep(0),
                volState(RAMP_READY), muteState(MUTE_ON) {};
        ~_StreamSource() {};
        am_connectionID_t route;
        am_sourceID_t source;
        unsigned char volStep;
        eVolState volState;
        eMuteState muteState;
    } mStreams[8];
};

class FBPowerState: public FBFunc
{
public:
    FBPowerState(IAmRoutingSend *sender) :
            FBFunc(sender, FUNC_ID_POST, "PowerState") {};
    virtual ~FBPowerState() {};

private:
    virtual void Set(unsigned short length, const unsigned char* data, bool isSet = true) override;
    virtual void Get(unsigned short length, const unsigned char* data) override;

private:
    enum eOpState
    {
        STATE_OFF,
        STATE_STANDBY,
        STATE_ON
    };
    std::string getOpState(void);

private:
    eOpState mOpState;
};

class FBAudioRoute: public FBFunc
{
public:
    FBAudioRoute(IAmRoutingSend *sender) :
            FBFunc(sender, FUNC_ID_AURO, "AudioRoute") {};
    virtual ~FBAudioRoute() {};

private:
    virtual void Set(unsigned short length, const unsigned char* data, bool isSet = true) override;
    virtual void Get(unsigned short length, const unsigned char* data) override;
    virtual void Acknowledge(const am_Handle_e type, const void *data, const am_Error_e error) override;

private:
    enum eSource
    {
        AMPLIFIER = 0x01
    };
    am_connectionID_t mNewConnection;
};

class FBVolume: public FBFunc
{
public:
    FBVolume(IAmRoutingSend *sender) :
            FBFunc(sender, FUNC_ID_VOLU, "Volume") {};
    virtual ~FBVolume() {};

private:
    virtual void Set(unsigned short length, const unsigned char* data, bool isSet = true) override;
    virtual void Get(unsigned short length, const unsigned char* data) override;
    virtual void Acknowledge(const am_Handle_e type, const void *data, const am_Error_e error) override;

private:
    void SendVolumeStatus(am_volume_t volume, unsigned char stream);

private:
    struct __attribute__((packed)) t_VolListStatus
    {
        unsigned char Stream;
        struct
        {
            unsigned char Step;
            short dB;
            unsigned char State;
        } vol;
    };
    struct __attribute__((packed)) t_VolListSet
    {
        unsigned char Stream;
        struct
        {
            unsigned char Step;
            short dB;
        } vol;
        struct
        {
            unsigned short time;
            unsigned short dBps;
        } ramp;
    } mVolume;
    am_volume_t mAmVolume;
};

class FBVolumeOffset: public FBFunc
{
public:
    FBVolumeOffset(IAmRoutingSend *sender) :
            FBFunc(sender, FUNC_ID_VOOF, "VolumeOffset") {};
    virtual ~FBVolumeOffset() {};

private:
    virtual void Set(unsigned short length, const unsigned char* data, bool isSet = true) override;
    virtual void Get(unsigned short length, const unsigned char* data) override;

private:
    enum eOffsetStatus
    {
        RAMP_READY,
        RAMP_RUNNING
    };
    struct __attribute__((packed)) tStatus
    {
        unsigned char Stream;
        unsigned short OffsetLF;
        unsigned short OffsetRF;
        unsigned short OffsetLR;
        unsigned short OffsetRR;
        unsigned short OffsetCE;
        unsigned short OffsetSUB;
        union
        {
            unsigned char OffsetStatus;
            struct
            {
                unsigned short RampLin;
                unsigned short RampdB;
            };
        } u;
    } mStatus;
};

class FBMute: public FBFunc
{
public:
    FBMute(IAmRoutingSend *sender) :
            FBFunc(sender, FUNC_ID_MUTE, "Mute") {};
    virtual ~FBMute() {};

private:
    virtual void Set(unsigned short length, const unsigned char* data, bool isSet = true) override;
    virtual void Get(unsigned short length, const unsigned char* data) override;
    virtual void Acknowledge(const am_Handle_e type, const void *data, const am_Error_e error) override;

private:
    void SendMuteStatus(unsigned char stream);

private:
    enum eMuteAction
    {
        MUTE_DEACTIVATE, MUTE_ACTIVATE
    };
    struct __attribute__((packed)) t_MuteListSet
    {
        unsigned char Stream;
        unsigned char Action;
        struct
        {
            unsigned short time;
            unsigned short dBps;
        } ramp;
    } mMuteListSet;
    struct t_MuteListState
    {
        unsigned char Stream;
        eMuteState State;
    } mState;
};

class FBDiagnosisResult: public FBFunc
{
public:
    FBDiagnosisResult(IAmRoutingSend *sender) :
            FBFunc(sender, FUNC_ID_DIRE, "DiagnosisResult") {};
    virtual ~FBDiagnosisResult() {};

private:
    virtual void Get(unsigned short length, const unsigned char* data) override;

private:
    enum eSite
    {
        SPEAKER_LF = 0x01,
        SPEAKER_RF,
        SPEAKER_LR,
        SPEAKER_RR,
        SPEAKER_CE,
        SPEAKER_SUB,
        AMPLIFIER_DEVICE = 0x20
    };
    enum eErrorCode
    {
        OPEN_WOOFER = 0x01,
        SHORT_GND,
        SHORT_VCC,
        SHORT_WIRE,
        OPEN_TWEETER,
        THERMAL_WARNING = 0x20
    };
    enum eErrorVal
    {
        NO_ERROR = 0x01
    };
    unsigned char mData[3];
};

class FBSoundConfigVersion: public FBFunc
{
public:
    FBSoundConfigVersion(IAmRoutingSend *sender) :
            FBFunc(sender, FUNC_ID_SCVR, "SoundConfigVersion") {};
    virtual ~FBSoundConfigVersion() {};

private:
    virtual void Get(unsigned short length, const unsigned char* data) override;
};

}

#endif /* CRAFUNCBLOCK_H_ */
