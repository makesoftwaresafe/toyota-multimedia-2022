/**
 * @brief config sources and sinks for both routing and processing
 */
#ifndef IASAUDIOIDENTIFIERS_HPP_
#define IASAUDIOIDENTIFIERS_HPP_

namespace IasAudio {

/**
 * @brief definition of sourceIDs used in IasAudioRouting and IasAudioProcessing
 */
enum IasAudioSource
{ 
  eIasSourceBYT_capture_2TunerPlayback0 = 0,
  eIasSourceMEDIA_playback_0 = 1,
  eIasSourceANN_mono0 = 2,
  eIasSourceANN_mono1 = 3,
  eIasSourceA2DP_playback0 = 4,
  eIasSourceBYT_capture_0LINE_IN = 5,
  eIasSourceHFPDL_playback0 = 6,
  eIasSourceMEDIA_playback_1 = 7,
  eIasSourceBYT_capture_0MIC0 = 8,
  eIasSourceBYT_capture_0MIC1 = 9,
  eIasSourceBYT_capture_0MIC2 = 10,
  eIasSourceBYT_capture_0Microphone = 11,
  eIasSourceMEDIA_playbackMc_0Mc = 12,
  eIasSourceMEDIA_playbackMc_1Mc = 13,
  eIasSourceAVBStereo0 = 14,
  eIasSourceAVBStereo1 = 15,
  eIasSourceA2DP_playback1 = 16,
  eIasSourceA2DP_playback2 = 17,
  eIasSourceMEDIA_playbackMc_0Dm = 18,
  eIasSourceMEDIA_playbackMc_1Dm = 19,
  eIasSourceAVBmc0 = 20,
  eIasSourceAVBmc1 = 21,
  eIasSourceMediaPlayer1 = 22,
  eIasSourceNavigation = 23,
  eIasSourceBT_IN = 24,
  eIasSourceTest8ChPlayback = 25,
};

/**
 * @brief definition of sinkIDs used in IasAudioRouting and IasAudioProcessing
 */
enum IasAudioSink
{
  eIasSinkStereo0 = 0,
  eIasSinkStereo1 = 1,
  eIasSinkMono0 = 2,
  eIasSinkMono1 = 3,
  eIasSinkStereo2 = 4,
  eIasSinkAVBStereo0 = 5,
  eIasSinkAVBStereo1 = 6,
  eIasSinkAVBMc0 = 7,
  eIasSinkA2DP_capture_0 = 8,
  eIasSinkA2DP_capture_1 = 9,
  eIasSinkA2DP_capture_2 = 10,
  eIasSinkAvbMc1 = 11,
  eIasSinkSpeechRecog = 12,
  eIasSinkHfpSnk = 13,
  eIasSinkTest8ChCapture = 14,
  eIasSinkEntertainment = 16,
  eIasSinkInterrupt = 17,
  eIasSinkSystemBeep = 18,
};

/**
 * @brief definition of zoneIDs used in IasAudioRouting and IasAudioProcessing
 */
enum IasAudioZone
{
  eIasZoneSpeakers0 = 0,
  eIasZoneHeadPhones0 = 1,
};



/**
 * @brief definition of a value to indicate an invalid source or sink ID.
 */
const Ias::Int32 cIasInvalidId = -1;
const Ias::Int32 cIasIgnoreId = -1;

} // namespace IasAudio

#endif /* IASAUDIOIDENTIFIERS_HPP_ */
