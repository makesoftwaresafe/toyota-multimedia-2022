/************************************************************************
 * @file: CRaRTSProxy.h
 *
 * @description: A proxy class definition of Routing Adapter. Proxy class does the
 * connect operation between source and sink using ALSA API
 *
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
 *          Vanitha C, vanitha.channaiah@in.bosch.com 2017
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
#ifndef CRARTSPROXY_H_
#define CRARTSPROXY_H_

#include <alsa/asoundlib.h>
#include "CAmRoutingAdapterALSAdb.h"
#include "IAmRoutingAdapterALSAProxy.h"
#include "rts.h"
#include "CThread.h"

#define MAX_STREAMS     16
#define MS_PER_SECOND   1000
#define SRC_DEVICE      0
#define MAX_DEVICES     2
#define MAX_DDUPLEX     4
#define STRERROR(a)     (static_cast<const string&>(strerror(-a)))

namespace am
{

class CRaRTSProxy : public IAmRoutingAdapterALSAProxy, public adit::utility::CThread
{
public:
    CRaRTSProxy(const ra_Proxy_s & proxy);
    virtual ~CRaRTSProxy(void);

    am_timeSync_t getDelay(void) const;

    /* IAmRoutingAdapterALSAProxy */
    am_Error_e openStreaming(void) final;
    am_Error_e startStreaming(void) final;
    am_Error_e stopStreaming(void) final;
    am_Error_e closeStreaming(void) final;

private:
    /* CThread */
    int initThread(void) final;
    int workerThread(void) final;
    void deinitThread(int errInit) final;

    std::string getThreadName(void) const;
    int getFormatSize(void) const;
    void freeCopyBuffers(void);

private:
    tRTS mRts;
    /* In DUPLEX mode, devices can forward 8 channel stream */
    trts_cfgadev mAdevs[MAX_DDUPLEX];
    trts_cfgstream mStreams[MAX_STREAMS];
    trts_cfg mCfg;
    char* mCopyBuffers[MAX_STREAMS / MAX_DEVICES];
};

} /* namespace am */

#endif /* CRARTSPROXY_H_*/
