/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2015-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 *  \copyright The MIT License (MIT)
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


#include <cassert>
#include <algorithm>
#include <stdexcept>
#include "CAmRoutingAdapterALSAMixerCtrl.h"

using namespace am;

int _elemCallback(snd_mixer_elem_t *elem, unsigned int mask);

static const std::string strDir[CAmRoutingAdapterALSAMixerCtrl::DIR_MAX] = {
    " playback ",
    " capture "
};

int _elemCallback(snd_mixer_elem_t *elem, unsigned int mask)
{
    if ((elem != NULL) && (mask & SND_CTL_EVENT_MASK_VALUE))
    {
        void * This = snd_mixer_elem_get_callback_private(elem);
        if (This != NULL)
        {
            return static_cast<CAmRoutingAdapterALSAMixerCtrl*>(This)->elemCallback(elem);
        }
    }
    return 0;
}


CAmRoutingAdapterALSAMixerCtrl::CAmRoutingAdapterALSAMixerCtrl() : mpHandle(NULL), mpSid(NULL), mpElem(NULL)
{
    if (snd_mixer_open(&mpHandle, 0))
    {
        throw std::runtime_error("snd_mixer_open() error");
    }
    if (snd_mixer_selem_id_malloc(&mpSid))
    {
        throw std::runtime_error("snd_mixer_selem_id_malloc() error");
    }
}

CAmRoutingAdapterALSAMixerCtrl::~CAmRoutingAdapterALSAMixerCtrl()
{
    if (mpElem != NULL)
    {
        snd_mixer_elem_set_callback(mpElem, NULL);
    }
    if (mpSid != NULL)
    {
        snd_mixer_selem_id_free(mpSid);
    }
    if (mpHandle != NULL)
    {
        snd_mixer_close(mpHandle);
    }
}
int CAmRoutingAdapterALSAMixerCtrl::setEnum(const std::string& value)
{
    int err;
    if (snd_mixer_selem_is_enumerated(mpElem))
    {
        bool enumFound = false;
        unsigned int enumIndex = getEnumIndex(mpElem, value, enumFound);
        if (enumFound)
        {
            err = snd_mixer_selem_set_enum_item(mpElem, SND_MIXER_SCHN_FRONT_LEFT, enumIndex);
            if (err < 0)
            {
                return retWithError("snd_mixer_selem_set_enum_item failed", err);
            }
        }
        else
        {
            return retWithError("Enum value not found", -EFAULT);
        }
    }
    else
    {
        return retWithError("Mixer is not enumerated type", -EFAULT);
    }
    return err;
}

unsigned int CAmRoutingAdapterALSAMixerCtrl::getEnumIndex(snd_mixer_elem_t *elem, const std::string& value, bool& enumFound)
{
    enumFound = false;
    unsigned int index = 0;
    char *enumMixer = new char[value.size() + 1];
    for (index = 0; index < snd_mixer_selem_get_enum_items(elem); index++)
    {
        if (snd_mixer_selem_get_enum_item_name(elem, index, value.size() + 1, enumMixer) < 0)
        {
            continue;
        }
        if (value.compare(enumMixer) == 0)
        {
            enumFound = true;
            break;
        }
    }
    delete enumMixer;
    return index;
}

int CAmRoutingAdapterALSAMixerCtrl::openMixer(const std::string & card, const std::string & volume, const mixer_dir_e dir)
{
    mDir = dir;
    mCard = card;
    mVolume = volume;

    int err = snd_mixer_attach(mpHandle, mCard.c_str());
    if (err < 0)
    {
        return retWithError("CALSAMixerCtrl::openMixer Opening of " + mCard + " fails with: ", err);
    }

    err = snd_mixer_selem_register(mpHandle, NULL, NULL);
    if (err < 0)
    {
        return retWithError("CALSAMixerCtrl::openMixer Register element fails with: ", err);
    }

    err = snd_mixer_load(mpHandle);
    if (err < 0)
    {
        return retWithError("CALSAMixerCtrl::openMixer Load of mixer fails with: ", err);
    }

    snd_mixer_selem_id_set_index(mpSid, 0);
    snd_mixer_selem_id_set_name(mpSid, mVolume.c_str());
    mpElem = snd_mixer_find_selem(mpHandle, mpSid);
    if (mpElem == NULL)
    {
        err = retWithError("CALSAMixerCtrl::openMixer Mixer element " + mVolume + " not found ", -ENODEV);
    }

    return err;
}

void CAmRoutingAdapterALSAMixerCtrl::activateVolumeChangeNotification(void)
{
    snd_mixer_elem_set_callback(mpElem, _elemCallback);
    snd_mixer_elem_set_callback_private(mpElem, this);
}

int CAmRoutingAdapterALSAMixerCtrl::elemCallback(snd_mixer_elem_t *elem)
{
    if (elem == mpElem)
    {
        long int volume;
        int err = getVolume(volume);
        if (err < 0)
        {
            return err;
        }
        return cbVolumeChange(volume);
    }
    return 0;
}

int CAmRoutingAdapterALSAMixerCtrl::setRange(const long int & minVol, const long int & maxVol)
{
    typedef int (*setRange_t)(snd_mixer_elem_t*, long int, long int);
    static const setRange_t snd_mixer_selem_set_volume_range[DIR_MAX] = {
        snd_mixer_selem_set_playback_volume_range,
        snd_mixer_selem_set_capture_volume_range
    };

    int err = snd_mixer_selem_set_volume_range[mDir](mpElem, minVol, maxVol);
    if (err == 0)
    {
        return err;
    }
    return retWithError("CALSAMixerCtrl::setRange Set" + strDir[mDir] + "volume range of " + mCard + " failed with: ", err);
}

long int CAmRoutingAdapterALSAMixerCtrl::getRange(long int & minVol, long int & maxVol)
{
    typedef int (*getRange_t)(snd_mixer_elem_t*, long int*, long int*);
    static const getRange_t snd_mixer_selem_get_volume_range[DIR_MAX] = {
        snd_mixer_selem_get_playback_volume_range,
        snd_mixer_selem_get_capture_volume_range
    };

    int err = snd_mixer_selem_get_volume_range[mDir](mpElem, &minVol, &maxVol);
    if (err == 0)
    {
        return std::abs(maxVol - minVol);
    }
    return retWithError("CALSAMixerCtrl::getRange Get" + strDir[mDir] + "volume range of " + mCard + " failed with: ", err);
}

int CAmRoutingAdapterALSAMixerCtrl::setVolume(const long int & volume)
{
    typedef int (*setVolume_t)(snd_mixer_elem_t*, long int);
    static const setVolume_t snd_mixer_selem_set_volume[DIR_MAX] = {
        snd_mixer_selem_set_playback_volume_all,
        snd_mixer_selem_set_capture_volume_all
    };

    int err = snd_mixer_selem_set_volume[mDir](mpElem, volume);
    if (err == 0)
    {
        return snd_mixer_handle_events(mpHandle);
    }
    return retWithError("CALSAMixerCtrl::setVolume Set" + strDir[mDir] + "volume of " + mCard + " failed with: ", err);
}

int CAmRoutingAdapterALSAMixerCtrl::getVolume(long int & volume)
{
    typedef int (*getVolume_t)(snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long int*);
    static const getVolume_t snd_mixer_selem_get_volume[DIR_MAX] = {
        snd_mixer_selem_get_playback_volume,
        snd_mixer_selem_get_capture_volume
    };

    int err = snd_mixer_selem_get_volume[mDir](mpElem, SND_MIXER_SCHN_MONO , &volume);
    if (err == 0)
    {
        return err;
    }
    return retWithError("CALSAMixerCtrl::getVolume Get" + strDir[mDir] + "volume of " + mCard + " failed with: ", err);
}

std::string CAmRoutingAdapterALSAMixerCtrl::getStrError()
{
    return mStrError;
}

int CAmRoutingAdapterALSAMixerCtrl::retWithError(const std::string str, const int err)
{
    mStrError = str + snd_strerror(err);
    return err;
}

int CAmRoutingAdapterALSAMixerCtrl::cbVolumeChange(const long int & volume)
{
    (void)volume;
    return 0;
}
