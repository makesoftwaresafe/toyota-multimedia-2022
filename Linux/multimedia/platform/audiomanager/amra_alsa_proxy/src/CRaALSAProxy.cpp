/************************************************************************
 * @file: CRaALSAProxy.cpp
 *
 * @version: 1.1
 *
 * @description: A proxy class implementation of Routing Adapter. Proxy class
 * does the connect operation between source and sink using ALSA API.
 *
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
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

#include <errno.h>
#include <cassert>
#include <string.h>
#include <memory>
#include "CRaALSAProxy.h"
#include "CAmDltWrapper.h"

#include "AudioFactory.h"

#include "AudioHelper.h"

using namespace std;
using namespace am;
using namespace adit::utility;
using namespace adit::utility::audio;

extern "C" IAmRoutingAdapterALSAProxy* AlsaProxyFactory(const ra_Proxy_s& proxy)
{
    return new CRaALSAProxy(proxy);
}

CRaALSAProxy::CRaALSAProxy (const ra_Proxy_s & proxy) : IAmRoutingAdapterALSAProxy(proxy)
{
    string backendCodename = "Alsa";
    mStream = Factory::Instance()->createBackend(backendCodename, *this);
    if(mStream == NULL)
    {
        throw -EFAULT;
    }

    mStream->setThreadSched(mProxy.cpuScheduler.policy, mProxy.cpuScheduler.priority);
    mStream->setFadeTime(FadeMode::IN, StreamDirection::IN, 0);
    mStream->setFadeTime(FadeMode::OUT, StreamDirection::OUT, 0);
}

CRaALSAProxy::~CRaALSAProxy(void)
{
}

am_timeSync_t CRaALSAProxy::getDelay(void) const
{
    return convertFramesToMs(mProxy.rate, mStream->getStreamLatencyInFrames());
}

am_Error_e CRaALSAProxy::openStreaming(void)
{
    uint32_t period_frames = (mProxy.rate * mProxy.msBuffersize) / MS_PER_SECOND;
    AudioError err = mStream->openStream(mProxy.pcmSrc,mProxy.pcmSink,(AudioFormat)mProxy.format,mProxy.rate,mProxy.channels,period_frames);
    if (err != AudioError::OK)
    {
        logError("CRaALSAProxy::openStream failed with", err);
        return am_Error_e::E_UNKNOWN;
    }
    return am_Error_e::E_OK;
}

am_Error_e CRaALSAProxy::startStreaming(void)
{
    AudioError err = mStream->startStream();
    if (err != AudioError::OK)
    {
        logError("CRaALSAProxy::startStreaming failed with", err);
        return am_Error_e::E_UNKNOWN;
    }
    return am_Error_e::E_OK;
}

am_Error_e CRaALSAProxy::stopStreaming(void)
{
    AudioError err = mStream->stopStream();
    if (err != AudioError::OK)
    {
        logError("CRaALSAProxy::stopStream failed with", err);
        return am_Error_e::E_UNKNOWN;
    }
    return am_Error_e::E_OK;
}

am_Error_e CRaALSAProxy::closeStreaming(void)
{
    AudioError err = mStream->closeStream();
    if (err != AudioError::OK && err != AudioError::INVALID)
    {
        logError("CRaALSAProxy::closeStreaming failed with", err);
        return am_Error_e::E_UNKNOWN;
    }
    return am_Error_e::E_OK;
}

void CRaALSAProxy::error(const std::string& data) const
{
    logError(data);
}

void CRaALSAProxy::warning(const std::string& data) const
{
    logWarning(data);
}

void CRaALSAProxy::info(const std::string& data) const
{
    logInfo(data);
}

void CRaALSAProxy::debug(const std::string& data) const
{
    logDebug(data);
}

adit::utility::eLogLevel CRaALSAProxy::checkLogLevel() const
{
    CAmDltWrapper *inst = CAmDltWrapper::instance();

    /* Will use a map between the enums for a better summarized code */
    std::map<DltLogLevelType, eLogLevel> logMapper;
    logMapper[DLT_LOG_DEBUG] = LL_DEBUG;
    logMapper[DLT_LOG_INFO]  = LL_INFO;
    logMapper[DLT_LOG_WARN]  = LL_WARNING;

    /* Watch out, we're gonna use a reverse_iterator from rbegin to rend: as a matter of fact,
     * higher levels include the lower ones (we mean, when DEBUG level is active, all others are as well;
     * when INFO level is active, all levels but DEBUG are active; when WARNING is active, all levels except
     * INFO and DEBUG are active, and so on. Now, since current enum has DEBUG higher than the others, and
     * so on, we need to perform reverse iteration.
     */
    for (auto rIter = logMapper.rbegin(); rIter != logMapper.rend(); rIter++)
    {
        if (true == inst->checkLogLevel(rIter->first))
        {
            return rIter->second;
        }
    }
    return eLogLevel::LL_ERROR;
}

AudioState CRaALSAProxy::processing(unsigned char *in, unsigned char **out, uint32_t &frames)
{
    *out = in;
    (void)frames;
    return AudioState::CONTINUE;
}
void CRaALSAProxy::statistics(const StreamStatistics& status)
{
    if(status.time)
    {

    }
}

void CRaALSAProxy::eostreaming(const AudioError error)
{
    (void)error;
}
