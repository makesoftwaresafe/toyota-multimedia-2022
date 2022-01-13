/************************************************************************
 * @file: CAmInterface.h
 *
 * @version: 1.0
 *
 * @description: Implements the callback handler triggered by AudioManager
 *               and instantiates the client interface.
 *
 * @component: platform/audiomanager/amcp_test
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#ifndef CAMINTERFACE_H_
#define CAMINTERFACE_H_

#include "IAmApplicationClient.h"


namespace am {

struct IAmInterface
{
    virtual ~IAmInterface() {};
    virtual am_Error_e connect() = 0;
    virtual am_Error_e connect(const am_sourceID_t & src, const am_sinkID_t & sink) = 0;
    virtual am_Error_e disconnect() = 0;
    virtual am_Error_e end() = 0;
    virtual void reconnect(const __time_t seconds) = 0;
};

class IPlayer;

class CAmInterface: public IAmInterface, public IAmApplicationClient
{
public:
    CAmInterface(class CAmSocketHandler *socketHandler, IPlayer *player, const std::string &node,
            const std::string &application, const am_sourceID_t &sourceID, const am_sinkID_t &sinkID);
    virtual ~CAmInterface();

    am_Error_e connect() final;
    am_Error_e connect(const am_sourceID_t & src, const am_sinkID_t & sink) final;
    am_Error_e disconnect() final;
    am_Error_e end() final;
    void reconnect(const __time_t seconds) final;

    /* timer callback */
    void cbReconnect(sh_timerHandle_t handle, void * data);

    am_sourceID_t &getMainSrc();
    am_sinkID_t &getMainSink();

private:
    void cbRemovedMainConnection(const am_mainConnectionID_t connectionID) final;

    am_Error_e    asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                               const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                               const am_CustomConnectionFormat_t connectionFormat) final;

    am_Error_e asyncDisconnect(const am_Handle_s handle,
                               const am_connectionID_t connectionID) final;

    am_Error_e asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID,
                                    const am_volume_t volume, const am_CustomRampType_t ramp,
                                    const am_time_t time) final;

    am_Error_e asyncSetSinkVolume  (const am_Handle_s handle, const am_sinkID_t sinkID,
                                    const am_volume_t volume, const am_CustomRampType_t ramp,
                                    const am_time_t time) final;

    am_Error_e asyncSetSourceState (const am_Handle_s handle, const am_sourceID_t sourceID,
                                    const am_SourceState_e state) final;

private:

    TAmShTimerCallBack<CAmInterface> mpCbFunc;
    sh_timerHandle_t                 mHandle;

    bool                             mExitOnRemove;
    am_domainID_t                    mDomain;
    am_sourceID_t                    mSrc;
    am_sinkID_t                      mSink;
    am_mainConnectionID_t            mMainConnection;
    am_connectionID_t                mConnection;

    CAmSocketHandler                *mpSocketHandler;
    IPlayer                         *mpPlayerInterface;
    CAmSerializer                    mSerializer;
};

}

#endif /* CAMINTERFACE_H_ */
