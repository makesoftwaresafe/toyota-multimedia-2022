/**
 * \file: AudioStream.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * cinemo audio streaming using backend ALSA API's
 *
 * \component: ipod cinemo
 *
 * \author: Vanitha.channaiah@in.bosch.com
 *
 * \copyright (c) 2019 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/
#ifndef AUDIOSTREAM_H_
#define AUDIOSTREAM_H_

#include "cinemoAudio.h"

namespace adit
{

namespace utility
{

namespace audio
{

class ApplicationStreaming : public Streaming
{

public:
    audioHandle *mCinemoHandle;
    /*Do not use mStream inside backend helper functions as stream is calling those functions*/
    std::shared_ptr<Backend> mStream;

    ApplicationStreaming(void);
    virtual ~ApplicationStreaming(void);

    void error(const std::string& data) const final;
    void warning(const std::string& data) const final;
    void info(const std::string& data) const final;
    void debug(const std::string& data) const final;
/*    void verbose(const std::string& data) const final;*/
    eLogLevel checkLogLevel() const final;

    AudioState processing(unsigned char *in, unsigned char **out, unsigned int &frames) final;
    void statistics(const StreamStatistics& status) final;
    void eostreaming(const AudioError error) final;

    /* To set Cinemo audio transport handle */
    void setApplicationHandle(ctli_handle tphandle);

};

} /* namespace audio */

} /* namespace utility */

} /* namespace adit */



#endif /* AUDIOSTREAM_H_ */
