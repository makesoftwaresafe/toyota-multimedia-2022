/**
 * @file IasAudioSubsystemConfiguration_brd2.cpp
 */

#include "audio/rtprocessingfwconfig/IasConfigurationCreatorInterface.hpp"
#include "audio/volume/IasVolumeConfigurationInterface.hpp"
#include "audio/mixer/IasMixerConfigurationInterface.hpp"
#include "audio/equalizer/IasEqualizerConfigurationInterface.hpp"
#include "audio/limiter/IasLimiterConfigurationInterface.hpp"
#include "audio/delay/IasDelayConfigurationInterface.hpp"
#include "audio/up_down_mixer/IasUpDownMixerConfigurationInterface.hpp"
#include "audio/configuration_brd2/IasAudioIdentifiers.hpp"
#include "stdio.h"

namespace IasAudio {

extern "C" {
 
 
IAS_DSO_PUBLIC void configureAudioSubsystem(IasConfigurationCreatorInterface &config)
{
  config.setFrameLength(192);
  config.setSampleRate(48000);
  // configure AudioStreams input
  IasAudioStream *hSinkStereo0 = config.createInputAudioStream("SinkStereo0", eIasSinkStereo0, 2, false);
  IasAudioStream *hSinkStereo1 = config.createInputAudioStream("SinkStereo1", eIasSinkStereo1, 2, false);
  IasAudioStream *hSinkMono0 = config.createInputAudioStream("SinkMono0", eIasSinkMono0, 1, false);
  IasAudioStream *hSinkMono1 = config.createInputAudioStream("SinkMono1", eIasSinkMono1, 1, false);
  IasAudioStream *hSinkStereo2 = config.createInputAudioStream("SinkStereo2", eIasSinkStereo2, 2, false);
  IasAudioStream *hSinkAvbMc1 = config.createInputAudioStream("SinkAvbMc1", eIasSinkAvbMc1, 8, true);
  IasAudioStream *hSinkEntertainment = config.createInputAudioStream("SinkEntertainment", eIasSinkEntertainment, 2, false);
  IasAudioStream *hSinkInterrupt = config.createInputAudioStream("SinkInterrupt", eIasSinkInterrupt, 2, false);
  IasAudioStream *hSinkSystemBeep = config.createInputAudioStream("SinkSystemBeep", eIasSinkSystemBeep, 2, false);
  
  // configure AudioStreams output
  IasAudioStream *hZoneSpeakers0 = config.createOutputAudioStream("ZoneSpeakers0", eIasZoneSpeakers0, 6, false);
  IasAudioStream *hZoneHeadPhones0 = config.createOutputAudioStream("ZoneHeadPhones0", eIasZoneHeadPhones0, 2, false);
  IasAudioStream *hZoneRefChannelStream = config.createOutputAudioStream("ZoneRefChannelStream", cIasIgnoreId, 1, false);
  IasAudioStream *hZoneAvbMc6ch = config.createOutputAudioStream("ZoneAvbMc6ch", cIasIgnoreId, 6, true);
  
  
  // configure intermediate streams output
  IasAudioStream *hZoneHFPDL = config.createIntermediateOutputAudioStream("ZoneHFPDL", cIasIgnoreId, 6, false);
  
  // configure UserEqualizer
  IasEqualizerConfigurationInterface *myUserEqConfig = config.createUserEqualizerConfiguration();
  myUserEqConfig->addStreamToProcess(hSinkStereo0);
  myUserEqConfig->addStreamToProcess(hSinkStereo1);
  myUserEqConfig->addStreamToProcess(hSinkStereo2);
  myUserEqConfig->addStreamToProcess(hSinkEntertainment);
  myUserEqConfig->addStreamToProcess(hSinkInterrupt);
  myUserEqConfig->addStreamToProcess(hSinkSystemBeep);
  myUserEqConfig->setNumFilterStagesMax(3);
  IasGenericAudioComp *myUserEq = config.createUserEqualizer(myUserEqConfig);
  
  // configure CarEqualizer                 
  IasEqualizerConfigurationInterface *myCarEqConfig = config.createCarEqualizerConfiguration();
  myCarEqConfig->addStreamToProcess(hZoneSpeakers0);
  myCarEqConfig->setNumFilterStagesMax(10);
  IasGenericAudioComp *myCarEq = config.createCarEqualizer(myCarEqConfig);
  
  // configure mixer
  IasMixerConfigurationInterface *myMixerConfig = config.createMixerConfiguration();
  // inputs
  myMixerConfig->addStreamMapping(hSinkStereo0, hZoneSpeakers0);
  myMixerConfig->addStreamMapping(hSinkStereo1, hZoneSpeakers0);
  myMixerConfig->addStreamMapping(hSinkMono0, hZoneSpeakers0);
  myMixerConfig->addStreamMapping(hSinkEntertainment, hZoneSpeakers0);
  myMixerConfig->addStreamMapping(hSinkInterrupt, hZoneSpeakers0);
  myMixerConfig->addStreamMapping(hSinkSystemBeep, hZoneSpeakers0);
  myMixerConfig->addStreamMapping(hSinkStereo2, hZoneHeadPhones0);
  myMixerConfig->addStreamMapping(hSinkMono1, hZoneHFPDL);
  IasGenericAudioComp *myMixer = config.createMixer(myMixerConfig);
  
  // configure upDownMixer 
  IasUpDownMixerConfigurationInterface *myUpDownMixerConfig = config.createUpDownMixerConfiguration();
  myUpDownMixerConfig->addStreamMapping(hSinkAvbMc1, hZoneAvbMc6ch);
  IasGenericAudioComp *myUpDownMixer = config.createUpDownMixer(myUpDownMixerConfig);
  
  // configure TelephoneMixer 
  IasMixerConfigurationInterface *myTelephoneMixerConfig = config.createTelephoneMixerConfiguration();
  // Add the intermediate stream HFPDL to hSpeaker.
  // The result is written to Speakers0 (due to the in-place processing).
  myTelephoneMixerConfig->addStreamMapping(hZoneHFPDL, hZoneSpeakers0);
  // Tell the telephone mixer where the reference channel shall be written to
  myTelephoneMixerConfig->addStreamToProcess(hZoneRefChannelStream);
  IasGenericAudioComp *myTelephoneMixer = config.createTelephoneMixer(myTelephoneMixerConfig);
  
  // configure delay
  IasDelayConfigurationInterface *myDelayConfig = config.createDelayConfiguration();
  myDelayConfig->addStreamToProcess(hZoneSpeakers0);
  myDelayConfig->setMaxDelay(1000000);
  IasGenericAudioComp *myDelay = config.createDelay(myDelayConfig);
  
  // configure volume
  IasVolumeConfigurationInterface *myVolumeConfig = config.createVolumeConfiguration();
  myVolumeConfig->addStreamToProcess(hSinkStereo0);
  myVolumeConfig->addStreamToProcess(hSinkStereo1);
  myVolumeConfig->addStreamToProcess(hSinkMono0);
  myVolumeConfig->addStreamToProcess(hSinkMono1);
  myVolumeConfig->addStreamToProcess(hSinkStereo2);
  myVolumeConfig->addStreamToProcess(hSinkEntertainment);
  myVolumeConfig->addStreamToProcess(hSinkInterrupt);
  myVolumeConfig->addStreamToProcess(hSinkSystemBeep);
  myVolumeConfig->setNumFilterBands(2);
  myVolumeConfig->setLengthLoudnessTable(5);
  const IasAudioFilterConfigParams loudnessFilterParams0 = {50, 0.0, 2.0, eIasFilterTypePeak, 2};
  myVolumeConfig->setLoudnessFilterParams(0, loudnessFilterParams0);
  const IasAudioFilterConfigParams loudnessFilterParams1 = {13000, 0.0, 1.1, eIasFilterTypePeak, 2};
  myVolumeConfig->setLoudnessFilterParams(1, loudnessFilterParams1);
  myVolumeConfig->setStreamActiveForFilterband(hSinkStereo0, 0);
  myVolumeConfig->setStreamActiveForFilterband(hSinkStereo0, 1);
  myVolumeConfig->setStreamActiveForFilterband(hSinkStereo1, 0);
  myVolumeConfig->setStreamActiveForFilterband(hSinkStereo1, 1);
  myVolumeConfig->setStreamActiveForFilterband(hSinkStereo2, 0);
  myVolumeConfig->setStreamActiveForFilterband(hSinkStereo2, 1);
  IasGenericAudioComp *myVolume = config.createVolume(myVolumeConfig);
  
  // configure limiter
  IasLimiterConfigurationInterface *myLimiterConfig = config.createLimiterConfiguration();
  myLimiterConfig->addStreamToProcess(hZoneSpeakers0);
  myLimiterConfig->addStreamToProcess(hZoneHeadPhones0);
  myLimiterConfig->setLimiterDelay(5); // delay in frames
  IasGenericAudioComp *myLimiter = config.createLimiter(myLimiterConfig);
  
  
  
  config.addAudioComponent(myUserEq);
  config.addAudioComponent(myVolume);
  config.addAudioComponent(myMixer);
  config.addAudioComponent(myCarEq);
  config.addAudioComponent(myLimiter);
  config.addAudioComponent(myDelay);
  config.addAudioComponent(myTelephoneMixer);
  config.addAudioComponent(myUpDownMixer);
  
  // VirtualAlsaInputs
  config.createVirtualAlsaInput("ANN_mono0", eIasSourceANN_mono0, 1);
  config.createVirtualAlsaInput("ANN_mono1", eIasSourceANN_mono1, 1);
  config.createVirtualAlsaInput("MEDIA_playback_0", eIasSourceMEDIA_playback_0, 2);
  config.createVirtualAlsaInput("MEDIA_playback_1", eIasSourceMEDIA_playback_1, 2);
  config.createVirtualAlsaInput("A2DP_playback0", eIasSourceA2DP_playback0, 2);
  config.createVirtualAlsaInput("A2DP_playback1", eIasSourceA2DP_playback1, 2);
  config.createVirtualAlsaInput("A2DP_playback2", eIasSourceA2DP_playback2, 2);
  config.createVirtualAlsaInput("HFPDL_playback0", eIasSourceHFPDL_playback0, 2);
  config.createVirtualAlsaInput("MediaPlayer1", eIasSourceMediaPlayer1, 2);
  config.createVirtualAlsaInput("Navigation", eIasSourceNavigation, 2);
  config.createVirtualAlsaInput("BT_IN", eIasSourceBT_IN, 2);
  config.createVirtualAlsaInput("Test8ChPlayback", eIasSourceTest8ChPlayback, 8);
  // VirtualAlsaInput with subdevices
  IasAudioDevice *hSourceMEDIA_playbackMc_0 = config.createVirtualAlsaInput("MEDIA_playbackMc_0", cIasIgnoreId, 11);
  config.createAudioSubdevice(hSourceMEDIA_playbackMc_0, "Dm", eIasSourceMEDIA_playbackMc_0Dm, 2);
  config.createAudioSubdevice(hSourceMEDIA_playbackMc_0, "Mc", eIasSourceMEDIA_playbackMc_0Mc, 9);
  // VirtualAlsaInput with subdevices
  IasAudioDevice *hSourceMEDIA_playbackMc_1 = config.createVirtualAlsaInput("MEDIA_playbackMc_1", cIasIgnoreId, 11);
  config.createAudioSubdevice(hSourceMEDIA_playbackMc_1, "Dm", eIasSourceMEDIA_playbackMc_1Dm, 2);
  config.createAudioSubdevice(hSourceMEDIA_playbackMc_1, "Mc", eIasSourceMEDIA_playbackMc_1Mc, 9);
  // VirtualAlsaInput with subdevices
  IasAudioDevice *hSourceAVB = config.createVirtualAlsaInput("AVB", cIasIgnoreId, 20);
  config.createAudioSubdevice(hSourceAVB, "Stereo0", eIasSourceAVBStereo0, 2);
  config.createAudioSubdevice(hSourceAVB, "Stereo1", eIasSourceAVBStereo1, 2);
  config.createAudioSubdevice(hSourceAVB, "mc0", eIasSourceAVBmc0, 9);
  config.createAudioSubdevice(hSourceAVB, "mc1", eIasSourceAVBmc1, 7);
  
  // VirtualAlsaOutputs
  config.createVirtualAlsaOutput("A2DP_capture_0", eIasSinkA2DP_capture_0, 2);
  config.createVirtualAlsaOutput("A2DP_capture_1", eIasSinkA2DP_capture_1, 2);
  config.createVirtualAlsaOutput("A2DP_capture_2", eIasSinkA2DP_capture_2, 2);
  IasAudioDevice *hSinkRefChannel_0 = config.createVirtualAlsaOutput("RefChannel_0", cIasIgnoreId, 1);
  IasAudioDevice *hSinkMIC_capture_0 = config.createVirtualAlsaOutput("MIC_capture_0", cIasIgnoreId, 2);
  config.createVirtualAlsaOutput("SpeechRecog", eIasSinkSpeechRecog, 1);
  config.createVirtualAlsaOutput("HfpSnk", eIasSinkHfpSnk, 2);
  config.createVirtualAlsaOutput("Test8ChCapture", eIasSinkTest8ChCapture, 8);
  // VirtualAlsaOutput with subdevices
  IasAudioDevice *hSinkAVB = config.createVirtualAlsaOutput("AVB", cIasIgnoreId, 20);
  config.createAudioSubdevice(hSinkAVB, "Stereo0", eIasSinkAVBStereo0, 2);
  config.createAudioSubdevice(hSinkAVB, "Stereo1", eIasSinkAVBStereo1, 2);
  config.createAudioSubdevice(hSinkAVB, "Mc0", eIasSinkAVBMc0, 9);
  config.createAudioSubdevice(hSinkAVB, "Mc1", cIasIgnoreId, 7);
  
  // AlsaHwCaptureDriver
  // AlsaHwCaptureDriver with subdevices
  IasAudioDevice *hSourceBYT_capture_0;
  config.createAlsaHwCaptureDriver("BYT_capture_0", cIasIgnoreId, 8, true, eIasSampleFormat_S16_LE, 48000, false, false, &hSourceBYT_capture_0);
  config.createAudioSubdevice(hSourceBYT_capture_0, "MIC0", eIasSourceBYT_capture_0MIC0, 1);
  config.createAudioSubdevice(hSourceBYT_capture_0, "MIC1", eIasSourceBYT_capture_0MIC1, 1);
  config.createAudioSubdevice(hSourceBYT_capture_0, "MIC2", eIasSourceBYT_capture_0MIC2, 1);
  config.createAudioSubdevice(hSourceBYT_capture_0, "Microphone", eIasSourceBYT_capture_0Microphone, 1);
  config.createAudioSubdevice(hSourceBYT_capture_0, "LINE_IN", eIasSourceBYT_capture_0LINE_IN, 2);
  config.createAudioSubdevice(hSourceBYT_capture_0, "dummy", cIasIgnoreId, 2);
  // AlsaHwCaptureDriver with subdevices
  IasAudioDevice *hSourceBYT_capture_2;
  config.createAlsaHwCaptureDriver("BYT_capture_2", cIasIgnoreId, 8, true, eIasSampleFormat_S32_LE, 48000, false, false, &hSourceBYT_capture_2);
  config.createAudioSubdevice(hSourceBYT_capture_2, "TunerPlayback0", eIasSourceBYT_capture_2TunerPlayback0, 2);
  config.createAudioSubdevice(hSourceBYT_capture_2, "dummy", cIasIgnoreId, 6);
  
  // AlsaHwPlaybackDriver
  IasAudioDevice *hSinkSPEAKER_playback_0;
  config.createAlsaHwPlaybackDriver("SPEAKER_playback_0", cIasIgnoreId, 8, true, eIasSampleFormat_S16_LE, 48000, &hSinkSPEAKER_playback_0);
  config.setClockMasterDevice(hSinkSPEAKER_playback_0);
  
  // output mapping
  config.addOutputMapping(hZoneSpeakers0, 0, hSinkSPEAKER_playback_0, 2);
  config.addOutputMapping(hZoneSpeakers0, 1, hSinkSPEAKER_playback_0, 3);
  config.addOutputMapping(hZoneSpeakers0, 2, hSinkSPEAKER_playback_0, 4);
  config.addOutputMapping(hZoneSpeakers0, 3, hSinkSPEAKER_playback_0, 5);
  config.addOutputMapping(hZoneSpeakers0, 4, hSinkSPEAKER_playback_0, 0);
  config.addOutputMapping(hZoneSpeakers0, 5, hSinkSPEAKER_playback_0, 1);
  config.addOutputMapping(hZoneHeadPhones0, 0, hSinkSPEAKER_playback_0, 6);
  config.addOutputMapping(hZoneHeadPhones0, 1, hSinkSPEAKER_playback_0, 7);
  config.addOutputMapping(hZoneRefChannelStream, 0, hSinkRefChannel_0, 0);
  config.addOutputMapping(hZoneAvbMc6ch, 0, hSinkAVB, 13);
  config.addOutputMapping(hZoneAvbMc6ch, 1, hSinkAVB, 14);
  config.addOutputMapping(hZoneAvbMc6ch, 2, hSinkAVB, 15);
  config.addOutputMapping(hZoneAvbMc6ch, 3, hSinkAVB, 16);
  config.addOutputMapping(hZoneAvbMc6ch, 4, hSinkAVB, 17);
  config.addOutputMapping(hZoneAvbMc6ch, 5, hSinkAVB, 18);
  config.addOutputMapping(hZoneAvbMc6ch, 6, hSinkAVB, 19);
  
  // device mapping
  config.addDeviceMapping(hSourceBYT_capture_0, 0, hSinkMIC_capture_0, 0);
  config.addDeviceMapping(hSourceBYT_capture_0, 1, hSinkMIC_capture_0, 1);
}
} // end extern "C"

} // namespace IasAudio

