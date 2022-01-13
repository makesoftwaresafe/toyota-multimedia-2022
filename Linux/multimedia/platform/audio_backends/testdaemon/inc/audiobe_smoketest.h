/************************************************************************
 * @file: audiobe_smoketest.h
 *
 * @version: 0.1
 *
 * @description: Header file for Audio Backend smoketest
 *
 * @authors: Jens Lorenz (jlorenz@de.adit-jv.com) and Thouseef Ahamed (tahamed@de.adit-jv.com) 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/
#ifndef _AUDIOBE_SMOKETEST_H_
#define _AUDIOBE_SMOKETEST_H_

namespace adit
{

namespace utility
{

namespace audio
{

class ApplicationStreaming : public Streaming
{

public:

    ApplicationStreaming(void);
    virtual ~ApplicationStreaming(void);

    void error(const std::string& data) const final;
    void warning(const std::string& data) const final;
    void info(const std::string& data) const final;
    void debug(const std::string& data) const final;
    eLogLevel checkLogLevel() const final;

    AudioState processing(unsigned char *in, unsigned char **out, uint32_t &frames) final;
    void statistics(const StreamStatistics& status) final;
    void eostreaming(const AudioError error) final;
};

} /* namespace audio */

} /* namespace utility */

} /* namespace adit */

#endif /* _AUDIOBE_SMOKETEST_H_ */
