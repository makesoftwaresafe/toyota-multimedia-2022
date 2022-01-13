/*
 *  Copyright (C) 2012 Collabora Ltd. All rights reserved.
 *  Copyright (C) 2014, 2015 Igalia S.L. All rights reserved.
 *  Copyright (C) 2015 Metrological All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include "config.h"

#if ENABLE(MEDIA_STREAM) && USE(GSTREAMER)
#include "MediaPlayerPrivateGStreamerWebRTC.h"

#include "GStreamerUtilities.h"
#include "MediaPlayer.h"
#include "MediaStreamPrivate.h"
#include "NotImplemented.h"
#include "URL.h"

#include <wtf/MainThread.h>

#if PLATFORM(QT)
// Clear out offending Qt macro so the following header, gio.h, can be included.
// https://bugs.webkit.org/show_bug.cgi?id=95081
#undef signals
#undef emit
#endif

#include "QtWebRTC.h"

#include <wtf/text/CString.h>

namespace WebCore {

MediaPlayerPrivateGStreamerWebRTC::MediaPlayerPrivateGStreamerWebRTC(MediaPlayer* player)
    : MediaPlayerPrivateGStreamerBase(player)
    , m_paused(true)
    , m_stopped(true)
    , m_hasVideo(false)
    , m_hasAudio(false)
{
    m_startTime = time(NULL);
}

MediaPlayerPrivateGStreamerWebRTC::~MediaPlayerPrivateGStreamerWebRTC()
{
    stop();
}

void MediaPlayerPrivateGStreamerWebRTC::play()
{
    if (!m_streamPrivate) {
        m_readyState = MediaPlayer::HaveNothing;
        loadingFailed(MediaPlayer::Empty);
        return;
    }
    m_paused = false;
    internalLoad();
}

void MediaPlayerPrivateGStreamerWebRTC::pause()
{
    m_paused = true;
    stop();
}

float MediaPlayerPrivateGStreamerWebRTC::currentTime() const
{
/*    gint64 position = GST_CLOCK_TIME_NONE;
    GstQuery* query= gst_query_new_position(GST_FORMAT_TIME);

    if (m_videoSource && gst_element_query(m_videoSink.get(), query))
        gst_query_parse_position(query, 0, &position);
    else if (m_audioSource && gst_element_query(m_audioSink.get(), query))
        gst_query_parse_position(query, 0, &position);

    float result = 0.0f;
    if (static_cast<GstClockTime>(position) != GST_CLOCK_TIME_NONE)
        result = static_cast<double>(position) / GST_SECOND;

    LOG_MEDIA_MESSAGE("Position %" GST_TIME_FORMAT, GST_TIME_ARGS(position));
    gst_query_unref(query);

    return result;*/
  float result = (time(NULL)-m_startTime);
  return result;
}

void MediaPlayerPrivateGStreamerWebRTC::load(const String &url)
{
    notImplemented();
}

void MediaPlayerPrivateGStreamerWebRTC::load(MediaStreamPrivate* streamPrivate)
{
    if (!initializeGStreamer())
        return;

    m_streamPrivate = streamPrivate;
    if (!m_streamPrivate) {
        loadingFailed(MediaPlayer::NetworkError);
        return;
    }

    m_readyState = MediaPlayer::HaveNothing;
    m_networkState = MediaPlayer::Loading;
    m_player->networkStateChanged();
    m_player->readyStateChanged();

    if (!internalLoad())
        return;

    // If the stream contains video, wait for first video frame before setting
    // HaveEnoughData
    if (!hasVideo())
        m_readyState = MediaPlayer::HaveEnoughData;

    m_player->readyStateChanged();

}

#if ENABLE(MEDIA_SOURCE)
void MediaPlayerPrivateGStreamerWebRTC::load(const String& url, PassRefPtr<HTMLMediaSource>)
{
	printf(" *** TODO: implement %s:%d\n", __FILE__, __LINE__);
}
#endif

void MediaPlayerPrivateGStreamerWebRTC::loadingFailed(MediaPlayer::NetworkState error)
{

    if (m_networkState != error) {
        m_networkState = error;
        m_player->networkStateChanged();
    }
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_player->readyStateChanged();
    }
}

bool MediaPlayerPrivateGStreamerWebRTC::didLoadingProgress() const
{
    return true;
}

bool MediaPlayerPrivateGStreamerWebRTC::internalLoad()
{
    if (!m_stopped)
        return false;

    m_stopped = false;
    if (!m_streamPrivate) {
        loadingFailed(MediaPlayer::NetworkError);
        return false;
    }
    
    MediaStreamSource *source = NULL;
    if(m_streamPrivate->numberOfVideoSources() >= 1)
        source = m_streamPrivate->videoSources(0);
    else if (m_streamPrivate->numberOfAudioSources() >= 1)
        source = m_streamPrivate->audioSources(0);

    webrtc::MediaStreamInterface* stream = NULL;
    if(source)
        stream = (static_cast<WebKit::QtMediaStreamSource*>(source))->getMediaStream();

    if(stream) {
        webrtc::VideoTrackVector video_tracks = stream->GetVideoTracks();
        webrtc::AudioTrackVector audio_tracks = stream->GetAudioTracks();

        if(!video_tracks.empty()) {
            m_hasVideo = true;
            if((static_cast<WebKit::QtMediaStreamSource*>(source))->isLocal()) {
                StartLocalRenderer(video_tracks[0]);
            } else {
                StartRemoteRenderer(video_tracks[0]);
            }
        }

        if(!audio_tracks.empty()) {
            m_hasAudio = true;
            audioTrack_ = audio_tracks[0];
        }
    }

    m_readyState = MediaPlayer::HaveEnoughData;
    m_player->readyStateChanged();
    return true;
}

void MediaPlayerPrivateGStreamerWebRTC::stop()
{
    if (m_stopped)
        return;

    m_stopped = true;

    if(audioTrack_)
        audioTrack_->set_enabled(false);

    if(local_renderer_.get())
        StopLocalRenderer();
    if(remote_renderer_.get())
        StopRemoteRenderer();
}

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivateGStreamerWebRTC::create(MediaPlayer* player)
{
    return adoptPtr(new MediaPlayerPrivateGStreamerWebRTC(player));
}

void MediaPlayerPrivateGStreamerWebRTC::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
}

void MediaPlayerPrivateGStreamerWebRTC::getSupportedTypes(HashSet<String>& types)
{
    // FIXME
}

MediaPlayer::SupportsType MediaPlayerPrivateGStreamerWebRTC::supportsType(const MediaEngineSupportParameters& parameters)
{
    // FIXME
    return MediaPlayer::IsNotSupported;
}

bool MediaPlayerPrivateGStreamerWebRTC::isAvailable()
{
    if (!initializeGStreamer())
        return false;

    return true;
}

void MediaPlayerPrivateGStreamerWebRTC::StartLocalRenderer(webrtc::VideoTrackInterface* local_video) {
    local_renderer_.reset(new VideoRenderer(this, local_video, true));
}

void MediaPlayerPrivateGStreamerWebRTC::StopLocalRenderer() {
    local_renderer_.reset();
}

void MediaPlayerPrivateGStreamerWebRTC::StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video) {
    remote_renderer_.reset(new VideoRenderer(this, remote_video, false));
}

void MediaPlayerPrivateGStreamerWebRTC::StopRemoteRenderer() {
    remote_renderer_.reset();
}

MediaPlayerPrivateGStreamerWebRTC::VideoRenderer::VideoRenderer(
    MediaPlayerPrivateGStreamerWebRTC* player,
    webrtc::VideoTrackInterface* track_to_render,
    bool isLocal)
    : width_(0),
      height_(0),
      player_(player),
      rendered_track_(track_to_render),
      isLocal_(isLocal){
    rendered_track_->AddRenderer(this);
}

MediaPlayerPrivateGStreamerWebRTC::VideoRenderer::~VideoRenderer() {
    rendered_track_->RemoveRenderer(this);
}

void MediaPlayerPrivateGStreamerWebRTC::VideoRenderer::SetSize(int width, int height) {
    width_ = width;
    height_ = height;
    image_.reset(new uint8[width * height * 4]);
}

static void repaintOnMainThread(void *context) {
    MediaPlayerPrivateGStreamerWebRTC* player_ = (MediaPlayerPrivateGStreamerWebRTC* )context;
    player_->mediaPlayer()->repaint();
}

//by chinhin
Mutex MediaPlayerPrivateGStreamerBase::m_syncLock;
void MediaPlayerPrivateGStreamerWebRTC::VideoRenderer::RenderFrame(const cricket::VideoFrame* video_frame) {
    m_syncLock.lock();
    const cricket::VideoFrame* frame = video_frame->GetCopyWithRotationApplied();

    SetSize(static_cast<int>(frame->GetWidth()),
            static_cast<int>(frame->GetHeight()));

    int size = width_ * height_ * 4;
    frame->ConvertToRgbBuffer(cricket::FOURCC_ARGB,
                              image_.get(),
                              size,
                              width_ * 4);

    if(isLocal_)
        player_->setPaintType(PaintForLocalRenderer);
    else
        player_->setPaintType(PaintForRemoteRenderer);
    
#if 0	//  WebCore operations must execute on main thread.
    player_->mediaPlayer()->repaint();
#else
    callOnMainThread(repaintOnMainThread, player_);
#endif
    m_syncLock.unlock();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM) && USE(GSTREAMER)
