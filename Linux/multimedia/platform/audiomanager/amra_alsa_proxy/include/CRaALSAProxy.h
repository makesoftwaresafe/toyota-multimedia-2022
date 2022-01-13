/************************************************************************
 * @file: CRaALSAProxy.h
 *
 * @version: 1.1
 *
 * @description: A proxy class definition of Routing Adapter. Proxy class does the
 * connect operation between source and sink using ALSA API
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

#ifndef CRAALSAPROXY_H_
#define CRAALSAPROXY_H_

#include <alsa/asoundlib.h>
#include "CAmRoutingAdapterALSAdb.h"

#include "AudioBackend.h"
#include "AudioStreaming.h"

#include "IAmRoutingAdapterALSAProxy.h"

#define MAX_STREAMS     16
#define MS_PER_SECOND   1000
#define INIT_TOUT       1000

namespace am
{

class CRaALSAProxy : public adit::utility::audio::Streaming, public IAmRoutingAdapterALSAProxy
{
public:
    CRaALSAProxy(const ra_Proxy_s & proxy);
    virtual ~CRaALSAProxy(void);

    am_timeSync_t getDelay(void) const;

    /* IAmRoutingAdapterALSAProxy */
    am_Error_e openStreaming(void);
    am_Error_e startStreaming(void);
    am_Error_e stopStreaming(void);
    am_Error_e closeStreaming(void);

    /* Streaming */
    void error(const std::string& data) const final;
    void warning(const std::string& data) const final;
    void info(const std::string& data) const final;
    void debug(const std::string& data) const final;
    adit::utility::eLogLevel checkLogLevel() const final;


    adit::utility::audio::AudioState processing(unsigned char *in, unsigned char **out, uint32_t &frames) final;
    void statistics(const adit::utility::audio::StreamStatistics& status) final;
    void eostreaming(const adit::utility::audio::AudioError error) final;

private:
    std::shared_ptr<adit::utility::audio::Backend> mStream;
};

} /* namespace am */

#endif /* CRAALSAPROXY_H_*/
