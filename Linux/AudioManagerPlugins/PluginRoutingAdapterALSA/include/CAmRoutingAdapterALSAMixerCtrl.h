/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2015-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 *  \copyright  The MIT License (MIT)
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 ******************************************************************************/


#ifndef ROUTINGADAPTERALSAMIXERCTRL_H_
#define ROUTINGADAPTERALSAMIXERCTRL_H_

#include <string>
#include <alsa/asoundlib.h>


namespace am
{

class CAmRoutingAdapterALSAMixerCtrl
{
public:
    typedef enum __mixer_dir
    {
        DIR_PLAYBACK = 0,
        DIR_CAPTURE,
        DIR_MAX
    } mixer_dir_e;

private:
    snd_mixer_t *mpHandle;
    snd_mixer_selem_id_t *mpSid;
    snd_mixer_elem_t* mpElem;

    mixer_dir_e mDir;       /* ALSA mixer direction */
    std::string mCard;      /* ALSA PCM card name with .ctl interface */
    std::string mVolume;    /* ALSA volume element of the card */

    std::string mStrError;

public:
    CAmRoutingAdapterALSAMixerCtrl();
    virtual ~CAmRoutingAdapterALSAMixerCtrl();

    /**
     * This function opens a given mixer element.
     * @param[in] card      name of the sound card with .ctl interface
     * @param[in] volume    name of the volume element
     * @param[in] dir       direction: DIR_PLAYBACK or DIR_CAPTURE
     * @returns 0 if mixer was opened successfully otherwise <0
     */
    int openMixer(const std::string & card,
            const std::string & volume,
            const mixer_dir_e dir = DIR_PLAYBACK);

    /**
     * This function activates the volume change notification for the mixer element.
     * In case the volume will be changed by setVolume() or other applications
     * the function cbVolumeChange() will be triggered and returns the new volume.
     * @returns nothing
     */
    void activateVolumeChangeNotification();

    /**
     * @warning Don't use this function!
     * This function is used to handle the volume change notification callbacks.
     */
    int elemCallback(snd_mixer_elem_t *elem);

    /**
     * This function will change the volume range of the mixer element.
     * @param[in] minVol    minimal volume which has to be smaller than maxVol
     * @param[in] maxVol    maximal volume which has to be bigger than minVol
     * @returns 0 if range was set successfully otherwise <0
     */
    int setRange(const long int & minVol, const long int & maxVol);

    /**
     * This function returns the range of the mixer element.
     * @param[out] minVol   minimal volume which is smaller than maxVol
     * @param[out] maxVol   maximal volume which is bigger than minVol
     * @returns range of mixer element otherwise <0
     */
    long int getRange(long int & minVol, long int & maxVol);

    /**
     * This function sets the new volume of mixer element.
     * @param[in] volume    new volume
     * @returns 0 if volume was set successfully otherwise <0
     */
    int setVolume(const long int & volume);

    /**
     * This function returns current volume of mixer element.
     * @param[out] volume   current volume
     * @returns 0 on success otherwise <0
     */
    int getVolume(long int & volume);

    /**
     * This function performs the Enum Item setting for given value
     * of already specified mixer element.
     * @param[in] value     string representing the Enum Item following amixer output
     * @returns 0 on success otherwise <0
     */
    int setEnum(const std::string& value);

    /**
     * This function is used by previous setEnum to associate a long to the Enum, if any is matching
     * of already specified mixer element.
     * @param[in] elem          ALSA mixer element to be manipulated
     * @param[in] value         string representing the Enum Item following amixer output
     * @param[out] enumFound    true when a matching Enum Item is found, false otherwise
     * @returns the index found, 0 by default, but user should trigger on enumFound
     */
    unsigned int getEnumIndex(snd_mixer_elem_t *elem, const std::string& value, bool& enumFound);

    /**
     * This function returns the last failure.
     * @returns 0 on success otherwise <0
     */
    std::string getStrError();

protected:
    /**
     * This function will be called when mixer volume changes.
     * @param[in] volume    notified volume
     * @return shall return 0 on success
     */
    virtual int cbVolumeChange(const long int & volume);

private:
    int retWithError(const std::string str, const int err);
};

} /* namespace am */

#endif /* ROUTINGADAPTERALSAMIXERCTRL_H_ */
